/***************************************************************************
 *   Copyright (C) 2009 by the qGo project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "stdio.h"
#include "defines.h"
#include "mainwindow.h"
#include "boardwindow.h"
#include "sgfparser.h"
#include "tree.h"
#include "listviews.h"
#include "network/serverliststorage.h"

#include <QtGui>

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags )
	: QMainWindow( parent,  flags )
{
	qDebug( "Home Path : %s" ,QDir::homePath().toLatin1().constData());	
	qDebug( "Current Path : %s" ,QDir::currentPath ().toLatin1().constData());

	ui.setupUi(this);
	//hide by default
	ui.changeServerPB->hide();
	ui.createRoomPB->hide();
	setWindowTitle(QString(PACKAGE) + " " + QString(VERSION));
	initStatusBar();
	/* FIXME, really need a list of such things, 0s */
	connection = 0;
	logindialog = 0;

	/* We need to integrate this room list with the new room code FIXME */
	connect(ui.RoomList,SIGNAL(currentIndexChanged( const QString &)), SLOT(slot_roomListClicked(const QString &)));
	connect(ui.channelsCB,SIGNAL(currentIndexChanged( const QString &)), SLOT(slot_channelListClicked(const QString &)));

	GameLoaded = NULL;
	SGFloaded = "";

	// loads the settings
	loadSettings();

	//creates the connection code
	seekMenu = new QMenu();
	ui.toolSeek->setMenu(seekMenu);

	connect(seekMenu,SIGNAL(triggered(QAction*)), SLOT(slot_seek(QAction*)));		
	connect(ui.toolSeek, SIGNAL( toggled(bool) ), SLOT( slot_seek(bool) ) );

	// filling the file view
	QStringList filters = (QStringList() << "*.sgf" << "*.SGF");
	model = new QDirModel(filters,  QDir::AllEntries | QDir::AllDirs , QDir::Name,0);

	ui.dirView_1->setModel(model);

	ui.dirView_1->hideColumn(1);
	ui.dirView_1->hideColumn(2);
	ui.dirView_1->setColumnWidth(0,300);
	
	ui.dirView_2->setModel(model);

	ui.dirView_2->hideColumn(1);
	ui.dirView_2->hideColumn(2);
	ui.dirView_2->setColumnWidth(0,300); 
	
	if(currentWorkingDir.isEmpty())
	{
		ui.dirView_1->setCurrentIndex(model->index(QDir::homePath()));
		ui.dirView_2->setCurrentIndex(model->index(QDir::homePath()));
	} else {
		ui.dirView_1->setCurrentIndex(model->index(currentWorkingDir));
		ui.dirView_2->setCurrentIndex(model->index(currentWorkingDir));
	}
	if (model->isDir(ui.dirView_1->currentIndex()))
		ui.dirView_1->expand(ui.dirView_1->currentIndex());
	if (model->isDir(ui.dirView_2->currentIndex()))
		ui.dirView_2->expand(ui.dirView_2->currentIndex());
	
	//init the small board display
	ui.displayBoard->init(19);
	ui.displayBoard2->init(19);
	// Make preview boards aware of their size
	ui.displayBoard->changeSize();
	ui.displayBoard2->changeSize();
//	ui.displayBoard->setShowCoords(FALSE);

	// connecting the Go server tab buttons and signals
	connect( ui.pb_connect, SIGNAL( toggled(bool) ), SLOT( slot_connect(bool) ) );

	connect( ui.cb_cmdLine, SIGNAL( activated(const QString&) ), this, SLOT( slot_cmdactivated(const QString&) ) );
//	connect( ui.cb_cmdLine, SIGNAL( activated(int) ), this, SLOT( slot_cmdactivated_int(int) ) );

	// connecting the new game button
	connect(ui.button_newGame,SIGNAL(pressed()),SLOT(slot_fileNewBoard()));
	connect(ui.button_loadGame,SIGNAL(pressed()),SLOT(slot_fileOpenBoard()));

	connect(ui.button_newComputerGame,SIGNAL(pressed()),SLOT(slot_computerNewBoard()));
	connect(ui.button_loadComputerGame,SIGNAL(pressed()),SLOT(slot_fileOpenBoard()));

	connect(ui.dirView_1, SIGNAL(expanded(const QModelIndex &)), this, SLOT(slot_expanded(const QModelIndex &)));
	connect(ui.dirView_2, SIGNAL(expanded(const QModelIndex &)), this, SLOT(slot_expanded(const QModelIndex &)));
	connect(ui.dirView_1, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(slot_fileOpenBoard(const QModelIndex &)));
	connect(ui.dirView_2, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(slot_fileOpenBoard(const QModelIndex &)));
	connect(ui.dirView_1->selectionModel(),  
		SIGNAL(currentChanged ( const QModelIndex & , const QModelIndex &  )),
		this,
		SLOT(slot_displayFileHeader(const QModelIndex & , const QModelIndex &  )));

	connect(ui.dirView_2->selectionModel(),  
		SIGNAL(currentChanged ( const QModelIndex & , const QModelIndex &  )),
		this,
		SLOT(slot_loadComputerFile(const QModelIndex & , const QModelIndex &  )));
	connect( ui.computerPathButton, SIGNAL( clicked() ), this, SLOT( slot_getComputerPath() ) );
	connect(ui.LineEdit_computer, SIGNAL(textChanged (const QString &)), this, SLOT(slot_computerPathChanged(const QString &)));

	
	// connecting the server tab buttons
	connect(ui.cb_connect, SIGNAL(currentIndexChanged ( int )), SLOT(slot_cbconnectChanged(int )));
	connect( ui.setQuietMode, SIGNAL( clicked(bool) ), this, SLOT( slot_cbquiet() ) );
	connect( ui.setOpenMode, SIGNAL( clicked(bool) ), this, SLOT( slot_cbopen() ) );
	connect( ui.setLookingMode, SIGNAL( clicked(bool) ), this, SLOT( slot_cblooking() ) );
	
	connect(ui.newFile_Handicap, SIGNAL(valueChanged(int)), this, SLOT(slot_newFile_HandicapChange(int)));
	connect(ui.newComputer_Handicap, SIGNAL(valueChanged(int)), this, SLOT(slot_newComputer_HandicapChange(int)));
	connect(ui.cancelButtonPrefs,SIGNAL(pressed()),SLOT(slot_cancelPressed()));
	connect(ui.cancelButtonServer,SIGNAL(pressed()),SLOT(slot_cancelPressed()));
	connect(ui.stackedWidget, SIGNAL(currentChanged ( int )), SLOT(slot_currentChanged(int )));

	//coneects the preference buttons
	connect( ui.gobanPathButton, SIGNAL( clicked() ), this, SLOT( slot_getGobanPath() ) );
	connect( ui.tablePathButton, SIGNAL( clicked() ), this, SLOT( slot_getTablePath() ) );
	connect(ui.comboBox_language, SIGNAL(currentIndexChanged ( int )), SLOT(slot_languageChanged(int )));
	connect( ui.alternateListColorsCB, SIGNAL( clicked(bool) ), this, SLOT( slot_alternateListColorsCB(bool) ) );

	// Creates the SGF parser for displaying the file infos
	MW_SGFparser = new SGFParser(NULL);
	
	// for saving server ip lists
	serverliststorage = new ServerListStorage();

	//sound
	connectSound = 	SoundFactory::newSound(SOUND_PATH_PREFIX"static.wav");
}

MainWindow::~MainWindow()
{
	delete serverliststorage;
	cleanupServerData();
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	/* Close connection if open */
	if(closeConnection() < 0)
	{
		e->ignore();
		return;
	}
	if(checkForOpenBoards() < 0)
	{
		e->ignore();
		return;
	}
	saveSettings();
}

#ifdef FIXME
	/* We're not sure yet what to do with the status bar,
	 * how to divy it up */
#endif //FIXME
void MainWindow::initStatusBar()
{
//	statusBar = new QStatusBar(parent);
//	statusBar->resize(-1, 20);
	statusBar()->show();
//	statusBar()->setSizeGripEnabled(false);
	statusBar()->showMessage(tr("Ready."));  // Normal indicator

	// Standard Text instead of "message" cause WhatsThisButten overlaps
	statusMessage = new QLabel(statusBar());
	statusMessage->setAlignment(Qt::AlignCenter /*| SingleLine*/);
	statusMessage->setText("");
	statusBar()->addPermanentWidget(statusMessage/*, 0, true*/);  // Permanent indicator
/*
	// What's this
	statusWhatsThis = new QLabel(statusBar);
	statusWhatsThis->setAlignment(AlignCenter | SingleLine);
	statusWhatsThis->setText("WHATSTHIS");
	statusBar->addWidget(statusWhatsThis, 0, true);  // Permanent indicator
	QWhatsThis::whatsThisButton(statusWhatsThis);
*/
	// The users widget
	statusUsers = new QLabel(statusBar());
	statusUsers->setAlignment(Qt::AlignCenter /* | SingleLine*/);
	statusUsers->setText(" P: 0");// / 0 ");
	statusBar()->addPermanentWidget(statusUsers /*, 0, true*/);  // Permanent indicator
	statusUsers->setToolTip( tr("Current online players / watched players"));
	statusUsers->setWhatsThis( tr("Displays the number of current online players\nand the number of online players you are watching.\nA player you are watching has an entry in the 'watch player:' field."));

	// The games widget
	statusGames = new QLabel(statusBar());
	statusGames->setAlignment(Qt::AlignCenter /*| SingleLine*/);
	statusGames->setText(" G: 0");// / 0 ");
	statusBar()->addPermanentWidget(statusGames /*, 0, true*/);  // Permanent indicator
	statusGames->setToolTip( tr("Current online games / observed games + matches"));
	statusGames->setWhatsThis( tr("Displays the number of games currently played on this server and the number of games you are observing or playing"));

	// The server widget
	statusServer = new QLabel(statusBar());
	statusServer->setAlignment(Qt::AlignCenter /*| SingleLine*/);
	statusServer->setText(" OFFLINE ");
	statusBar()->addPermanentWidget(statusServer /*, 0, true*/);  // Permanent indicator
	statusServer->setToolTip( tr("Current server"));
	statusServer->setWhatsThis( tr("Displays the current server's name or OFFLINE if you are not connected to the internet."));
/*
	// The channel widget
	statusChannel = new QLabel(statusBar());
	statusChannel->setAlignment(Qt::AlignCenter | SingleLine);
	statusChannel->setText("");
	statusBar()->addWidget(statusChannel, 0, true);  // Permanent indicator
	QToolTip::add(statusChannel, tr("Current channels and users"));
	QWhatsThis::add(statusChannel, tr("Displays the current channels you are in and the number of users inthere.\nThe tooltip text contains the channels' title and users' names"));
*/
	// Online Time
	statusOnlineTime = new QLabel(statusBar());
	statusOnlineTime->setAlignment(Qt::AlignCenter);
	statusOnlineTime->setText(" 00:00 ");
	statusBar()->addPermanentWidget(statusOnlineTime /*, 0, true*/);  // Permanent indicator
	statusOnlineTime->setToolTip( tr("Online Time"));
	statusOnlineTime->setWhatsThis( tr("Displays the current online time.\n(A) -> auto answer\n(Hold) -> hold the line"));

}

bool MainWindow::loadSGF(QString fileName)
{
	fileLoaded = fileName.toLatin1().constData();
	SGFloaded = MW_SGFparser->loadFile(fileLoaded);
	
	if (SGFloaded == NULL)
		return false;
	
	if (GameLoaded != NULL)
		delete GameLoaded;
	
	GameLoaded = MW_SGFparser->initGame(SGFloaded, fileLoaded);
	if (GameLoaded == NULL)
		return false;
	
	GameLoaded->gameMode = modeNormal;
	
	return (GameLoaded != NULL);
}

/* 
 * Loads the file header data from the item selected in the directory display
 */
bool MainWindow::selectFile(const QModelIndex &index)
{
	if (model->isDir(index))
		return false;

	return loadSGF( model->filePath(index) );
}

void MainWindow::slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/ )
{
	GameLoaded = NULL;
	SGFloaded = QString();
	ui.displayBoard->clearData();
	QVariant v = topLeft.data(QDirModel::FilePathRole);
	//qDebug( "Selected item : %s" ,v.toString().toLatin1().constData());

	if (!selectFile(topLeft))
	{
		ui.button_loadGame->setDisabled(true);
		
		ui.File_WhitePlayer->setText("");
		ui.File_BlackPlayer->setText("");
		ui.File_Date->setText("");
		ui.File_Handicap->setText("");
		ui.File_Result->setText("");
		ui.File_Komi->setText("");
		ui.File_Size->setText("");
		
		return ;
	} else {
		GameLoaded->gameMode = modeNormal;
		ui.button_loadGame->setEnabled(true);
		
		QString komi, hcp, sz;
		komi.setNum(GameLoaded->komi);	
		hcp.setNum(GameLoaded->handicap);
		sz.setNum(GameLoaded->board_size);

		ui.File_WhitePlayer->setText(GameLoaded->white_name);
		ui.File_BlackPlayer->setText(GameLoaded->black_name);
		ui.File_Date->setText(GameLoaded->date);
		ui.File_Handicap->setText(hcp);
		ui.File_Result->setText(GameLoaded->result);
		ui.File_Komi->setText(komi);
		ui.File_Size->setText(sz);
		
		displayGame(ui.displayBoard);
	}
}

/*
 *
 */
void MainWindow::displayGame(DisplayBoard *board)
{
	board->clearData();
	if (board->getSize() != GameLoaded->board_size)
		board->init(GameLoaded->board_size);
		
	board->displayHandicap(GameLoaded->handicap);

	QString s = SGFloaded.trimmed();
	int end_main = s.indexOf(")(");
	if (end_main == -1)
		end_main = s.size();
	int a_offset = QChar::fromAscii('a').unicode() - 1 ;
	int cursor = 0;
	int x,y;
	int nb_displayed = 20;
	QString coords;

	cursor = s.indexOf(";B[");

	while ((cursor >0) && (cursor < end_main) && (nb_displayed--) )
	{
		x = s.at(cursor+3).unicode() - a_offset;
		y = s.at(cursor+4).unicode() - a_offset;
		board->updateStone(stoneBlack,x,y);
		cursor = s.indexOf(";B[",cursor +1);

	}

	cursor = s.indexOf(";W[");
	nb_displayed = 20;

	while ( (cursor >0) &&  (cursor < end_main) && (nb_displayed--) )
	{
		x = s.at(cursor+3).unicode() - a_offset;
		y = s.at(cursor+4).unicode() - a_offset;
		board->updateStone(stoneWhite,x,y);
		cursor = s.indexOf(";W[",cursor +1);

	}
}

/* 
 * Loads file from the item selected in the directory display
 */
void MainWindow::slot_loadComputerFile(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/ )
{
	QVariant v = topLeft.data(QDirModel::FilePathRole);

	if (!selectFile(topLeft))
	{
		ui.button_loadComputerGame->setDisabled(true);
	} else {
		GameLoaded->gameMode = modeComputer;
		ui.button_loadComputerGame->setEnabled(true);
		
		QString komi, hcp, sz;
		komi.setNum(GameLoaded->komi);	
		hcp.setNum(GameLoaded->handicap);
		sz.setNum(GameLoaded->board_size);

		ui.File_WhitePlayer->setText(GameLoaded->white_name);
		ui.File_BlackPlayer->setText(GameLoaded->black_name);
		ui.File_Date->setText(GameLoaded->date);
		ui.File_Handicap->setText(hcp);
		ui.File_Result->setText(GameLoaded->result);
		ui.File_Komi->setText(komi);
		ui.File_Size->setText(sz);

		/* FIXME, displayGame does displayBoard not displayBoard2, and
		 * why are there two of these ?!?!? */
		displayGame(ui.displayBoard2);
	}	
}

void MainWindow::slot_expanded(const QModelIndex & i)
{
	//refresh file system info
	//before I was calling it on "i" but it didn't like that
	// in 4.4.1, catually still crashes on this.
	//model->refresh(i);
	//FIXME
	//really would like a refresh here	
	currentWorkingDir = model->filePath(i);
}

/*
 * The 'New Game' button in 'sgf editor' tab has been pressed.
 */
void MainWindow::slot_fileNewBoard()
{
	
	GameData *gd = new GameData();
	gd->gameMode = modeNormal;

	gd->board_size = ui.newFile_Size->value();
	gd->handicap = ui.newFile_Handicap->value();
	gd->black_name = ui.newFile_BlackPlayer->text();
	gd->white_name = ui.newFile_WhitePlayer->text();
	gd->komi = ui.newFile_Komi->text().toFloat();
	new BoardWindow(gd, TRUE, TRUE);
}


void MainWindow::slot_newFile_HandicapChange(int a)
{
	if(a == 1)
		ui.newFile_Handicap->setValue(0);
}

void MainWindow::slot_newComputer_HandicapChange(int a)
{
	if(a == 1)
		ui.newComputer_Handicap->setValue(0);
}

void MainWindow::slot_fileOpenBoard()
{
	if(GameLoaded)
		new BoardWindow(new GameData(GameLoaded), TRUE, TRUE);
}

void MainWindow::slot_fileOpenBoard(const QModelIndex & i)
{
	if (selectFile(i))
		slot_fileOpenBoard();
}

/*
 * The 'New Game' button in 'Go Engine' tab has been pressed.
 */
void MainWindow::slot_computerNewBoard()
{
	
	GameData *gd = new GameData();
	
	gd->gameMode = modeComputer;
	gd->board_size = ui.newComputer_Size->text().toInt();
	gd->handicap = ui.newComputer_Handicap->text().toInt();
	gd->komi = ui.newComputer_Komi->text().toFloat();
	gd->oneColorGo = ui.OneColorGoCheckBox->isChecked();
	
	bool imBlack = (ui.computerPlaysWhite->isChecked());
	bool imWhite = (ui.computerPlaysBlack->isChecked());
	gd->black_name = (imBlack ? "Human" : "Computer");
	gd->white_name = (imWhite ? "Human" : "Computer");

	if (imBlack == imWhite)
	{
		QMessageBox::warning(this, PACKAGE, tr("*** Both players are the same ! ***"));
		delete gd; gd = NULL;
		return;
	}

	if(!new BoardWindow(gd , imBlack, imWhite ))
	{
		delete gd; gd = NULL;
	}
}

void MainWindow::addBoardWindow(BoardWindow * bw)
{
	boardWindowList.push_back(bw);
}

void MainWindow::removeBoardWindow(BoardWindow * bw)
{
	std::vector<BoardWindow *>::iterator i;
	for(i = boardWindowList.begin(); i != boardWindowList.end(); i++)
	{
		if(*i == bw)
		{
			boardWindowList.erase(i);
			return;
		}
	}
}

int MainWindow::checkForOpenBoards(void)
{
	std::vector<BoardWindow *>::iterator i;
	for(i = boardWindowList.begin(); i != boardWindowList.end(); i++)
	{
		if(!(*i)->okayToQuit())
			return -1;
	}
	//close all open, since boardwindow no longer has parent because of windows task bar issue:
	i = boardWindowList.begin();
	while(i != boardWindowList.end())
	{
		delete (*i);
		i = boardWindowList.begin();
	}
	return 0;
}
