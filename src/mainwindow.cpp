/***************************************************************************
 *   Copyright (C) 2006 by Emmanuel Béranger                               *
 *   yfh2@hotmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "stdio.h"
#include "globals.h"
#include "mainwindow.h"
#include "boardwindow.h"
#include "sgfparser.h"
#include "tree.h"
#include "parser.h"

#include <QtGui>

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags )
	: QMainWindow( parent,  flags )
{
	qDebug( "Home Path : %s" ,QDir::homePath().toLatin1().constData());	
	qDebug( "Current Path : %s" ,QDir::currentPath ().toLatin1().constData());

	ui.setupUi(this);

	initStatusBar();

	//setting the lists layout on the server tab
	ui.ListView_games->header()->setSortIndicatorShown ( FALSE );
	ui.ListView_games->hideColumn(12);
	ui.ListView_games->hideColumn(13);
	ui.ListView_games->setColumnWidth ( 0, 35 );
	ui.ListView_games->setColumnWidth ( 1, 100 );
	ui.ListView_games->setColumnWidth ( 2, 35 );
	ui.ListView_games->setColumnWidth ( 3, 100 );
	ui.ListView_games->setColumnWidth ( 4, 35 );
	ui.ListView_games->setColumnWidth ( 5, 30 );
	ui.ListView_games->setColumnWidth ( 6, 25 );
	ui.ListView_games->setColumnWidth ( 7, 20 );
	ui.ListView_games->setColumnWidth ( 8, 30 );
	ui.ListView_games->setColumnWidth ( 9, 25 );
	ui.ListView_games->setColumnWidth ( 10, 20 );
	ui.ListView_games->setColumnWidth ( 11, 25 );
	
	ui.ListView_players->header()->setSortIndicatorShown ( FALSE );
	ui.ListView_players->hideColumn(6);
	ui.ListView_players->hideColumn(7);
	ui.ListView_players->hideColumn(8);
	ui.ListView_players->hideColumn(9);
	ui.ListView_players->hideColumn(12);
	ui.ListView_players->setColumnWidth ( 0, 30 );
	ui.ListView_players->setColumnWidth ( 1, 100 );
	ui.ListView_players->setColumnWidth ( 2, 30 );
	ui.ListView_players->setColumnWidth ( 3, 30 );
	ui.ListView_players->setColumnWidth ( 4, 30);
	ui.ListView_players->setColumnWidth ( 5, 30 );
//	ui.ListView_players->setColumnWidth ( 6, 80 );
	ui.ListView_players->setColumnWidth ( 7, 80 );
//	ui.ListView_players->setColumnWidth ( 8, 30 );
//	ui.ListView_players->setColumnWidth ( 9, 25 );
	ui.ListView_players->setColumnWidth ( 10, 50 );
//	ui.ListView_players->setColumnWidth ( 11, 25 );


	//connectig the slots on the lists and else on the server tab
	connect(ui.ListView_games->header(),SIGNAL( sectionClicked ( int ) ), SLOT(slot_sortGames (int)));
	connect(ui.ListView_players->header(),SIGNAL( sectionClicked ( int ) ), SLOT(slot_sortPlayers (int)));
	// doubleclick
	connect(ui.ListView_games, SIGNAL(itemDoubleClicked ( QTreeWidgetItem *, int )), SLOT(slot_gamesDoubleClicked(QTreeWidgetItem*)));
	connect(ui.ListView_players, SIGNAL(itemDoubleClicked ( QTreeWidgetItem *, int )), SLOT(slot_playersDoubleClicked(QTreeWidgetItem*)));
	
	connect(ui.pbRefreshPlayers,SIGNAL(pressed()),SLOT(slot_RefreshPlayers()));
	connect(ui.pbRefreshGames,SIGNAL(pressed()),SLOT(slot_RefreshGames()));
	connect(ui.RoomList,SIGNAL(currentIndexChanged( const QString &)), SLOT(slot_roomListClicked(const QString &)));

	SGFloaded = "";
	SGFloaded2 = "";

	// loads the settings
	loadSettings();

	//creates the connection code
	myAccount = new Account(this);
	igsConnection = new IGSConnection();
	parser = new Parser();
	seekMenu = new QMenu();
	ui.toolSeek->setMenu(seekMenu);
	connect(seekMenu,SIGNAL(triggered(QAction*)), SLOT(slot_seek(QAction*)));		
	connect(ui.toolSeek, SIGNAL( toggled(bool) ), SLOT( slot_seek(bool) ) );

	// create qGo Interface for board handling
	qgoif = new qGoIF(this);

	// filling the file view
	QStringList filters = (QStringList() << "*.sgf" << "*.SGF");
	model = new QDirModel(filters,  QDir::AllEntries | QDir::AllDirs , QDir::Name,0);

	ui.dirView_1->setModel(model);

	ui.dirView_1->hideColumn(1);
	ui.dirView_1->hideColumn(2);
	ui.dirView_1->setColumnWidth(0,300); 
	ui.dirView_1->setCurrentIndex(model->index( QDir::homePath () ));

	ui.dirView_2->setModel(model);

	ui.dirView_2->hideColumn(1);
	ui.dirView_2->hideColumn(2);
	ui.dirView_2->setColumnWidth(0,300); 
	ui.dirView_2->setCurrentIndex(model->index( QDir::homePath () ));

	//init the small board display
	ui.displayBoard->init(19);
	ui.displayBoard2->init(19);
//	ui.displayBoard->setShowCoords(FALSE);

	// connecting the Go server tab buttons and signals
	connect( ui.pb_connect, SIGNAL( toggled(bool) ), SLOT( slot_connect(bool) ) );
	connect (igsConnection, SIGNAL(signal_textReceived (const QString&)), SLOT(slot_textReceived (const QString&)));
	connect( ui.cb_cmdLine, SIGNAL( activated(const QString&) ), this, SLOT( slot_cmdactivated(const QString&) ) );
//	connect( ui.cb_cmdLine, SIGNAL( activated(int) ), this, SLOT( slot_cmdactivated_int(int) ) );

	// connecting the new game button
	connect(ui.button_newGame,SIGNAL(pressed()),SLOT(slot_fileNewBoard()));
	connect(ui.button_loadGame,SIGNAL(pressed()),SLOT(slot_fileOpenBoard()));

	connect(ui.button_newComputerGame,SIGNAL(pressed()),SLOT(slot_computerNewBoard()));
	connect(ui.button_loadComputerGame,SIGNAL(pressed()),SLOT(slot_computerOpenBoard()));

	connect(ui.dirView_1->selectionModel(),  
		SIGNAL(currentChanged ( const QModelIndex & , const QModelIndex &  )),
		this,
		SLOT(slot_displayFileHeader(const QModelIndex & , const QModelIndex &  )));

	connect(ui.dirView_2->selectionModel(),  
		SIGNAL(currentChanged ( const QModelIndex & , const QModelIndex &  )),
		this,
		SLOT(slot_loadComputerFile(const QModelIndex & , const QModelIndex &  )));


	
	// connecting the server tab buttons
	connect(ui.pb_add,SIGNAL(pressed()),SLOT(slot_addServer()));
	connect(ui.ListView_hosts, SIGNAL(itemClicked ( QTreeWidgetItem * , int )),SLOT(slot_clickedListView(QTreeWidgetItem * ,  int)));
	connect(ui.pb_new,SIGNAL(pressed()),SLOT(slot_new()));	
	connect(ui.comboBox_server,SIGNAL(currentIndexChanged ( const QString &)),
		SLOT(slot_serverChanged( const QString &)));
	connect( ui.setQuietMode, SIGNAL( clicked(bool) ), this, SLOT( slot_cbquiet() ) );
	connect( ui.setOpenMode, SIGNAL( clicked(bool) ), this, SLOT( slot_cbopen() ) );
	connect( ui.setLookingMode, SIGNAL( clicked(bool) ), this, SLOT( slot_cblooking() ) );
	connect( ui.whoBox1,  SIGNAL(  currentIndexChanged ( int )), this, SLOT(slot_setRankSpread()));
	connect( ui.whoBox2,  SIGNAL(  currentIndexChanged ( int )), this, SLOT(slot_setRankSpread()));

	
	// connects the parser signals to the main window (lists, rooms, ...)
	connect(parser, SIGNAL(signal_svname(GSName&)), SLOT(slot_svname(GSName&)));
	connect(parser, SIGNAL(signal_accname(QString&)), SLOT(slot_accname(QString&)));
	connect(parser, SIGNAL(signal_addSeekCondition(const QString&,const QString&, const QString&, const QString&, const QString&)),this,
			SLOT(slot_addSeekCondition(const QString&, const QString&, const QString&, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_seekList(const QString&, const QString&)),this,SLOT(slot_seekList(const QString&, const QString&)));
	connect(parser, SIGNAL(signal_room(const QString&, bool)),SLOT(slot_room(const QString&, bool)));
	connect(parser, SIGNAL(signal_game(Game*)), SLOT(slot_game(Game*)));
//	connect(parser, SIGNAL(signal_gameFinished(Game*)), SLOT(slot_gameFinished(Game*)));
	connect(parser, SIGNAL(signal_player(Player*, bool)), SLOT(slot_player(Player*, bool)));
	connect(parser, SIGNAL(signal_message(QString)), SLOT(slot_message(QString)));
	connect(parser, SIGNAL(signal_statsPlayer(Player*)), SLOT(slot_statsPlayer(Player*)));
	connect(parser, SIGNAL(signal_talk(const QString&, const QString&, bool)), SLOT(slot_talk(const QString&, const QString&, bool)));
	connect(parser, SIGNAL(signal_checkbox(int, bool)), SLOT(slot_checkbox(int, bool)));
	connect(parser, SIGNAL(signal_connexionClosed()), SLOT(slot_connexionClosed()));
	connect(parser, SIGNAL(signal_playerConnected(Player*)), SLOT (slot_playerConnected(Player*)));
	connect(parser, SIGNAL(signal_matchRequest(const QString&, bool)), this, SLOT(slot_matchRequest(const QString&, bool)));
	connect(parser, SIGNAL(signal_matchCanceled(const QString&)), this, SLOT(slot_matchCanceled(const QString&)));
	connect(parser, SIGNAL(signal_refresh(int)),this, SLOT(slot_refresh(int)));
	connect(parser, SIGNAL(signal_clearSeekCondition()),this,SLOT(slot_clearSeekCondition()));
	connect(parser, SIGNAL(signal_cancelSeek()),this,SLOT(slot_cancelSeek()));
	connect(parser, SIGNAL(signal_msgBox(const QString&)),this,SLOT(slot_msgBox(const QString&)));

	// connects the parser signals to the interface (game moves, all board and games infos, ...)
	connect(parser, SIGNAL(signal_move(GameInfo*)), qgoif, SLOT(slot_move(GameInfo*)));
	connect(parser, SIGNAL(signal_gameInfo(Game*)), qgoif, SLOT(slot_gameInfo(Game*)));
	connect(parser, SIGNAL(signal_observedGameClosed(int)), qgoif, SLOT(slot_boardClosed(int)));
	connect(parser, SIGNAL(signal_score(const QString&, const QString&, bool, const QString&)), 
		qgoif, SLOT(slot_score(const QString&, const QString&, bool, const QString&)));
	connect(parser, SIGNAL(signal_result(Game*)), qgoif, SLOT(slot_result(Game*)));
	connect(parser, SIGNAL(signal_kibitz(int, const QString&, const QString&)), qgoif, SLOT(slot_kibitz(int, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_observers(int, const QString&, const QString&)), qgoif, SLOT(slot_observers(int, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_clearObservers(int)), qgoif, SLOT(slot_observers(int)));
	connect(parser, SIGNAL(signal_enterScoreMode()), qgoif, SLOT(slot_enterScoreMode()));
	connect(parser, SIGNAL(signal_removeStones( const QString&, const QString&)), qgoif, SLOT(slot_removeStones( const QString&, const QString&)));
	connect(parser, SIGNAL(signal_restoreScore()), qgoif, SLOT(slot_restoreScore()));
	connect(parser, SIGNAL(signal_requestDialog(const QString&, const QString&, const QString&, const QString&)), 
		qgoif, SLOT(slot_requestDialog(const QString&, const QString&, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_undo(const QString&, const QString&, const QString&)), qgoif, SLOT(slot_undo(const QString&, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_gameReview(Game*)), qgoif, SLOT(slot_gameReview(Game*)));


	//Connects the interface signals
	connect(qgoif,SIGNAL(signal_sendCommandFromInterface(const QString&, bool)), SLOT(slot_sendCommand(const QString &, bool)));
	

	currentCommand = new sendBuf("",0);

	// Creates the SGF parser for displaying the file infos
	MW_SGFparser = new SGFParser(NULL);

	//sound
	connectSound = 	SoundFactory::newSound( "/usr/share/qgo2/sounds/static.wav" );
	gameSound = 	SoundFactory::newSound( "/usr/share/qgo2/sounds/blip.wav" );
}

MainWindow::~MainWindow()
{
}


void MainWindow::closeEvent(QCloseEvent *)
{
	saveSettings();
}



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



/* 
 * Loads the file header data from the item selected in the directory display
 */
void MainWindow::slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/ )
{

	ui.displayBoard->clearData();

	ui.File_WhitePlayer->setText("");
	ui.File_BlackPlayer->setText("");
	ui.File_Date->setText("");
	ui.File_Handicap->setText("");
	ui.File_Result->setText("");
	ui.File_Komi->setText("");
	ui.File_Size->setText("");

	QVariant v = topLeft.data(QDirModel::FilePathRole);
	//qDebug( "Selected item : %s" ,v.toString().toLatin1().constData());

	if (model->isDir(topLeft))
	{
		ui.button_loadGame->setDisabled(true);
		return ;
	}
	//qDebug( "Selected file : %s \n" ,model->filePath(topLeft).toLatin1().constData());

	fileLoaded = model->filePath(topLeft);
	SGFloaded = MW_SGFparser->loadFile(fileLoaded);
	
	if (SGFloaded == NULL)
	{
		ui.button_loadGame->setDisabled(true);
		return ;
	}

	ui.button_loadGame->setEnabled(true);
	
	GameLoaded = MW_SGFparser-> initGame(SGFloaded, fileLoaded);
	
	if (GameLoaded)
	{
		QString komi, hcp, sz;
		komi.setNum(GameLoaded->komi);	
		hcp.setNum(GameLoaded->handicap);
		sz.setNum(GameLoaded->size);

		ui.File_WhitePlayer->setText(GameLoaded->playerWhite);
		ui.File_BlackPlayer->setText(GameLoaded->playerBlack);
		ui.File_Date->setText(GameLoaded->date);
		ui.File_Handicap->setText(hcp);
		ui.File_Result->setText(GameLoaded->result);
		ui.File_Komi->setText(komi);
		ui.File_Size->setText(sz);

		displayGame();
	}	

}


/*
 *
 */
void MainWindow::displayGame()
{
	
	if (ui.displayBoard->getSize() != GameLoaded->size)
		ui.displayBoard->init(GameLoaded->size);
		
	ui.displayBoard->displayHandicap(GameLoaded->handicap);

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
		ui.displayBoard->updateStone(stoneBlack,x,y);
		cursor = s.indexOf(";B[",cursor +1);

	}

	cursor = s.indexOf(";W[");
	nb_displayed = 20;

	while ( (cursor >0) &&  (cursor < end_main) && (nb_displayed--) )
	{
		x = s.at(cursor+3).unicode() - a_offset;
		y = s.at(cursor+4).unicode() - a_offset;
		ui.displayBoard->updateStone(stoneWhite,x,y);
		cursor = s.indexOf(";W[",cursor +1);

	}


}

/* 
 * Loads file from the item selected in the directory display
 */
void MainWindow::slot_loadComputerFile(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/ )
{
	QVariant v = topLeft.data(QDirModel::FilePathRole);

	if (model->isDir(topLeft))
	{
		ui.button_loadComputerGame->setDisabled(true);
		return ;
	}

	fileLoaded2 = model->filePath(topLeft).toLatin1().constData();
	SGFloaded2 = MW_SGFparser->loadFile(fileLoaded2);
	
	if (SGFloaded2 == NULL)
	{
		ui.button_loadComputerGame->setDisabled(true);
		return ;
	}

	ui.button_loadComputerGame->setEnabled(true);
	
	GameLoaded2 = MW_SGFparser-> initGame(SGFloaded2, fileLoaded2);
}



/*
 * The 'New Game' button in 'sgf editor' tab has been pressed.
 */
void MainWindow::slot_fileNewBoard()
{
	
	GameData *gd = new GameData();

	gd->size = ui.newFile_Size->text().toInt();
	gd->handicap = ui.newFile_Handicap->text().toInt();
	gd->playerBlack = ui.newFile_BlackPlayer->text();
	gd->playerWhite = ui.newFile_WhitePlayer->text();
	gd->komi = ui.newFile_Komi->text().toFloat();

//	createGame(modeNormal, gd , TRUE, TRUE );
	qgoif->createGame(modeNormal, gd , TRUE, TRUE );
}

void MainWindow::slot_fileOpenBoard()
{

	qgoif->createGame(modeNormal, GameLoaded , TRUE, TRUE);// , SGFloaded );

}

/*
 * The 'New Game' button in 'Go Engine' tab has been pressed.
 */
void MainWindow::slot_computerNewBoard()
{
	
	GameData *gd = new GameData();

	gd->size = ui.newComputer_Size->text().toInt();
	gd->handicap = ui.newComputer_Handicap->text().toInt();

	gd->komi = ui.newComputer_Komi->text().toFloat();

	bool imBlack = (ui.computerPlaysWhite->isChecked());//cb_ComputerBlackPlayer->currentIndex() != 0);
	bool imWhite = (ui.computerPlaysBlack->isChecked());//cb_ComputerWhitePlayer->currentIndex() != 0);
	gd->playerBlack = (imBlack ? "Human" : "Computer"); //ui.newComputer_BlackPlayer->text();
	gd->playerWhite = (imWhite ? "Human" : "Computer"); //ui.newComputer_WhitePlayer->text();

	if (imBlack == imWhite)
	{
		QMessageBox::warning(this, PACKAGE, tr("*** Both players are the same ! ***"));
		return ;
	}

	qgoif->createGame(modeComputer, gd , imBlack, imWhite );
}

/*
 * The 'New Game' button in 'Go Engine' tab has been pressed.
 */
void MainWindow::slot_computerOpenBoard()
{
//	createGame(modeComputer, GameLoaded2 , TRUE, TRUE , SGFloaded2 );
	qgoif->createGame(modeComputer, GameLoaded2 , TRUE, TRUE );
}

/*
 * A column header of the game list has been clicked
 */
void MainWindow::slot_sortGames (int i)
{
	static Qt::SortOrder sortOrder;

	int j = ui.ListView_games->sortColumn();

	//sorts by the key stored in the hidden columns
	if (i==2)
		i = 12;
	if (i==4)
		i = 13;

	//If we sort the same column, reverse the sorting order
	if (i==j)
		sortOrder = (sortOrder == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
	else 
		sortOrder = Qt::AscendingOrder;

	ui.ListView_games->sortItems(i,sortOrder);
}


/*
 * A column header of the players list has been clicked
 */
void MainWindow::slot_sortPlayers (int i)
{
	static Qt::SortOrder sortOrder;

	int j = ui.ListView_players->sortColumn();

	//sorts by the key stored in the hidden columns
	if (i==2)
		i = 12;

	//If we sort the same column, reverse the sorting order
	if (i==j)
		sortOrder = (sortOrder == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
	else 
		sortOrder = Qt::AscendingOrder;

	ui.ListView_players->sortItems(i,sortOrder);
}

/*
 * games list has been double clicked
 */
void MainWindow::slot_gamesDoubleClicked(QTreeWidgetItem* lv)
{
	sendcommand ("observe " + lv->text(0));
}

/*
 * players list has been double clicked
 */
void MainWindow::slot_playersDoubleClicked(QTreeWidgetItem* lv)
{
	sendcommand ("stats " + lv->text(1));
}

/*
 * rank list has been changed.
 */
void MainWindow::slot_setRankSpread()
{
	QString r1,r2;

	if ((ui.whoBox1->currentIndex() == 0) && (ui.whoBox2->currentIndex() == 0))
	{
		rkMin = "NR";
		rkMax = "9p";
//		return;
	}

	else if ( 	((ui.whoBox1->currentIndex() == 0) && (ui.whoBox2->currentIndex() == 1)) ||
		((ui.whoBox1->currentIndex() == 1) && (ui.whoBox2->currentIndex() == 0))  ||
		((ui.whoBox1->currentIndex() == 1) && (ui.whoBox2->currentIndex() ==1)) )
	{
		rkMin = "1p";
		rkMax = "9p";
//		return;
	}	

	else if ((ui.whoBox1->currentIndex() == 0) && (ui.whoBox2->currentIndex() > 1))
	{
		rkMin = ui.whoBox2->currentText();
		rkMax = ui.whoBox2->currentText();
//		return;
	}	


	else if ((ui.whoBox1->currentIndex() > 1) && (ui.whoBox2->currentIndex() == 0))
	{
		rkMin = ui.whoBox1->currentText();
		rkMax = ui.whoBox1->currentText();
//		return;
	}	

	else if ((ui.whoBox1->currentIndex() > 1) && (ui.whoBox2->currentIndex() == 1))
	{
		rkMin = ui.whoBox1->currentText();
		rkMax = "9p";
//		return;
	}	

	else if ((ui.whoBox1->currentIndex() == 1) && (ui.whoBox2->currentIndex() > 1))
	{
		rkMin = ui.whoBox2->currentText();
		rkMax = "9p";
//		return;
	}


	else if ((ui.whoBox2->currentIndex() >= ui.whoBox1->currentIndex() ))
	{
		rkMin = ui.whoBox2->currentText();
		rkMax = ui.whoBox1->currentText();
	} 
	else 
	{
		rkMin = ui.whoBox1->currentText();
		rkMax = ui.whoBox2->currentText();
	} 


	qDebug( "rank spread : %s - %s" , rkMin.toLatin1().constData() , rkMax.toLatin1().constData());
}
