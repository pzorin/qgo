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



	connect(ui.ListView_games->header(),SIGNAL( sectionClicked ( int ) ), SLOT(slot_sortGames (int)));
	connect(ui.ListView_players->header(),SIGNAL( sectionClicked ( int ) ), SLOT(slot_sortPlayers (int)));
	// doubleclick
	connect(ui.ListView_games, SIGNAL(itemDoubleClicked ( QTreeWidgetItem *, int )), SLOT(slot_gamesDoubleClicked(QTreeWidgetItem*)));

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
		
	// create qGo Interface for board handling
	qgoif = new qGoIF(this);

	// filling the file view
	QStringList filters = (QStringList() << "*.sgf" << "*.SGF");
	model = new QDirModel(filters,  QDir::AllEntries | QDir::AllDirs , QDir::Name,0);

	ui.dirView_1->setModel(model);

	ui.dirView_1->hideColumn(1);
	ui.dirView_1->hideColumn(2);
	ui.dirView_1->setColumnWidth(0,250); 
	ui.dirView_1->setCurrentIndex(model->index( QDir::homePath () ));

	ui.dirView_2->setModel(model);

	ui.dirView_2->hideColumn(1);
	ui.dirView_2->hideColumn(2);
	ui.dirView_2->setColumnWidth(0,250); 
	ui.dirView_2->setCurrentIndex(model->index( QDir::homePath () ));


	// connecting the Go server tab buttons and signals
	connect( ui.pb_connect, SIGNAL( toggled(bool) ), SLOT( slot_connect(bool) ) );
	connect (igsConnection, SIGNAL(signal_textReceived (const QString&)), SLOT(slot_textReceived (const QString&)));

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
	
	// connects the parser signals to the main window (lists, rooms, ...)
	connect(parser, SIGNAL(signal_svname(GSName&)), SLOT(slot_svname(GSName&)));
	connect(parser, SIGNAL(signal_accname(QString&)), SLOT(slot_accname(QString&)));
	connect(parser, SIGNAL(signal_addSeekCondition(const QString&,const QString&, const QString&, const QString&, const QString&)),this,
			SLOT(slot_addSeekCondition(const QString&, const QString&, const QString&, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_room(const QString&, bool)),SLOT(slot_room(const QString&, bool)));
	connect(parser, SIGNAL(signal_game(Game*)), SLOT(slot_game(Game*)));
	connect(parser, SIGNAL(signal_player(Player*, bool)), SLOT(slot_player(Player*, bool)));
	connect(parser, SIGNAL(signal_message(QString)), SLOT(slot_message(QString)));


	// connects the parser signals to the interface (game moves, all board and games infos, ...)
	connect(parser, SIGNAL(signal_move(GameInfo*)), qgoif, SLOT(slot_move(GameInfo*)));
	connect(parser, SIGNAL(signal_gameInfo(Game*)), qgoif, SLOT(slot_gameInfo(Game*)));
	connect(parser, SIGNAL(signal_observedGameClosed(int)), qgoif, SLOT(slot_boardClosed(int)));

	//Connects the interface signals
	connect(qgoif,SIGNAL(signal_sendCommandFromInterface(const QString&, bool)), SLOT(slot_sendCommand(const QString &, bool)));
	

	currentCommand = new sendBuf("",0);

	// Creates the SGF parser for displaying the file infos
	MW_SGFparser = new SGFParser(NULL);

}

MainWindow::~MainWindow()
{
}


void MainWindow::closeEvent(QCloseEvent *)
{
	saveSettings();
}


/* 
 * Loads the file header data from the item selected in the directory display
 */
void MainWindow::slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/ )
{

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

	fileLoaded = model->filePath(topLeft).toLatin1().constData();
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
 * Creates a game board with all elements initialised from 
 * Gamedata and an sgf file 
 */
void MainWindow::createGame(GameMode _gameMode, GameData * _gameData, bool _myColorIsBlack , bool _myColorIsWhite , QString /*_fileLoaded*/ )
{
	BoardWindow *b = new BoardWindow(this,0,_gameData , _gameMode , _myColorIsBlack , _myColorIsWhite);
	
/*	
	TODO : we have put this into the boardwindow code. Is this the proper method ? (we did reload the file ...)
	if ( ! _fileLoaded.isEmpty())
		if(!b->loadSGF("", _fileLoaded))
		{
			delete b; 
			return ;
		}
*/
	b->show();
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
	gd->playerBlack = ui.newComputer_BlackPlayer->text();
	gd->playerWhite = ui.newComputer_WhitePlayer->text();
	gd->komi = ui.newComputer_Komi->text().toFloat();

	bool imBlack = (ui.cb_ComputerBlackPlayer->currentIndex() != 0);
	bool imWhite = (ui.cb_ComputerWhitePlayer->currentIndex() != 0);

	if (imBlack && imWhite)
	{
		QMessageBox::warning(this, PACKAGE, tr("** * Both players are Human ! ***"));
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

