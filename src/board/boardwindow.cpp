/***************************************************************************
*
* This class is the board window. It's the main purpose of qGo
* The boards get close messages,etc. from parent mainwindow on application exit
*
 ***************************************************************************/

#include "stdio.h"
#include "boardwindow.h"
#include "boardhandler.h"
#include "board.h"
#include "interfacehandler.h"
#include "qgoboard.h"
#include "move.h"
#include "../network/boarddispatch.h"
#include "listviews.h"
#include "ui_gameinfo.h"

#include <QtGui>

class BoardHandler;

BoardWindow::BoardWindow(GameData *gd, bool iAmBlack , bool iAmWhite, class BoardDispatch * _dispatch)
	: QMainWindow((QWidget*)mainwindow, 0)
{
	if(!gd)
	{
		gameData = new GameData();
		gameData->gameMode = modeNormal;
	}
	
	gameData = gd;
	dispatch = _dispatch;
	
	if(gameData->nigiriToBeSettled)
	{
		myColorIsBlack = false;
		myColorIsWhite = false;
	}
	else
	{
		myColorIsBlack = iAmBlack;
		myColorIsWhite = iAmWhite;
	}
	
	gamePhase = phaseInit;
	qDebug("PHASE is init\n");
	boardSize = gd->board_size;
	qDebug("Boardsize: %d handicap %d", boardSize, gd->handicap);
	//Creates the game tree
	tree = new Tree(boardSize);

	observerListModel = 0;
	setupUI();

	//Loads the sgf file if any
	if (! gameData->fileName.isEmpty())
		loadSGF(gameData->fileName);

	//creates the board interface (or proxy) that will handle the moves an command requests
	switch (gameData->gameMode)
	{
		case modeNormal :
			qgoboard = new 	qGoBoardNormalInterface(this, tree,gameData);
			break;
		case modeComputer :
			try
			{
				qgoboard = new 	qGoBoardComputerInterface(this, tree,gameData);
			}
			catch(QString err)
			{
				QMessageBox msg(QObject::tr("Error"),
						err,
						QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
				//msg.setActiveWindow();
				msg.raise();
				msg.exec();
				return;
			}
			break;	
		case modeObserve :
			qgoboard = new 	qGoBoardObserveInterface(this, tree,gameData);
			break;	
		case modeMatch :
			try
			{
				qgoboard = new 	qGoBoardMatchInterface(this, tree,gameData);
			}
			catch(QString err)
			{
				QMessageBox msg(QObject::tr("Error"),
						err,
						QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
				//msg.setActiveWindow();
				msg.raise();
				msg.exec();
				return;
			}
			break;	

		case modeReview :
			qgoboard = new 	qGoBoardReviewInterface(this, tree,gameData);
			break;	

		default:
			break;
	}
	setupBoardUI();

//	ui.board->init(boardSize);
	/*if(!qgoboard->init())
	{
		qDebug("qgoboard init failed\n");
	}*/

	/* FIXME since we're now having setGamePhase actually do something
	 * we might want to use it here to set up the initial button
	 * settings */
	gamePhase = phaseOngoing;
	show();
	setFocus();


	// Computer may have to make the first move
	if(gameData->gameMode == modeComputer)
		qgoboard->startGame();
	//ui.board->resize(407, 606);
		//ui.board->setBaseSize(407, 606);
	//ui.board->setGeometry(0, 0, 407, 606);
	
	//update();
	//gridLayout->update();
	
}

BoardWindow::~BoardWindow()
{
	qDebug("Deleting BoardWindow\n");
	QSettings settings;
	
	settings.setValue("BOARD_WINDOW_SIZE_X", width());
	settings.setValue("BOARD_WINDOW_SIZE_Y", height());
	settings.setValue("BOARD_SIZES", ui.boardSplitter->saveState());
	
	delete tree;	//okay?
	
	if(observerListModel)
		delete observerListModel;
	
	delete gameData;
	/* FIXME I'm not totally certain we want to delete the dispatch
	 * here.  If the boardwindow closes, then that sends its own
	 * closed signal through the network if it exits.  If the
	 * dispatch closes, then it doesn't close the board anyway */
	//if(dispatch)
	//	delete dispatch;
}

void BoardWindow::closeEvent(QCloseEvent *e)
{
	/* We need to prompt user on close as well as
	* set up code to send adjourn/resign signal, etc.
	* Otherwise other client can actually get stuck */
	if (checkModified()==1)		//checkModified needs to be checked out FIXME
	{
		e->accept();
		if(dispatch)
		{
			dispatch->closeBoard();
		}
		//FIXME delete, make sure there's no canvas destruction
		//crashes
		deleteLater();
	}
	else
		e->ignore();
}

void BoardWindow::setupUI(void)
{
	QSettings settings;
	ui.setupUi(this);
	ui.actionWhatsThis = QWhatsThis::createAction ();

	// Initialises the buttons and else
	editButtons = new QButtonGroup(this);
	editButtons->addButton(ui.stoneButton, 0);
	editButtons->addButton(ui.squareButton, 1);
	editButtons->addButton(ui.circleButton, 2);
	editButtons->addButton(ui.triangleButton, 3);
	editButtons->addButton(ui.crossButton, 4);
	editButtons->addButton(ui.labelLetterButton, 5);
	editButtons->addButton(ui.labelNumberButton, 6);
	editButtons->addButton(ui.colorButton, 7);
	editButtons->setExclusive(FALSE);


	QMenu *menu = new QMenu();
//	menu->insertAction(0,ui.fileExportASCII);
	menu->insertAction(0,ui.actionExportSgfClipB);
	menu->insertAction(0,ui.actionExportPic);
	menu->insertAction(0,ui.actionExportPicClipB);

	ui.actionExport->setMenu(menu);

	QToolButton *exportButton = new QToolButton();
	exportButton->setDefaultAction(ui.actionExport);
	
	exportButton->setPopupMode( QToolButton::InstantPopup);
	ui.toolBar->insertWidget ( ui.actionImport, exportButton );

	if(!gameData)
		qDebug("bw: no game data? okay?, line: %d\n", __LINE__);
	if(!gameData)
		clockDisplay = new ClockDisplay(this, canadian, 6000, 25, 60);	//what is this, FIXME
	else
		clockDisplay = new ClockDisplay(this, gameData->timeSystem, gameData->maintime, gameData->stones_periods, gameData->periodtime);
	ui.board->init(boardSize);

	interfaceHandler = new InterfaceHandler( this);
	interfaceHandler->toggleMode(gameData->gameMode);

	if (gameData)
		interfaceHandler->updateCaption(gameData);

	// creates the board handler for navigating in the tree
	boardHandler = new BoardHandler(this, tree, boardSize);

	int window_x, window_y;
	window_x = settings.value("BOARD_WINDOW_SIZE_X").toInt();
	window_y = settings.value("BOARD_WINDOW_SIZE_Y").toInt();
	resize(window_x, window_y);
	ui.boardSplitter->restoreState(settings.value("BOARD_SIZES").toByteArray());

	// Connects the nav buttons to the slots
	connect(ui.navForward,SIGNAL(pressed()), boardHandler, SLOT(slotNavForward()));
	connect(ui.navBackward,SIGNAL(pressed()), boardHandler, SLOT(slotNavBackward()));
	connect(ui.navNextVar, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextVar()));
	connect(ui.navPrevVar, SIGNAL(pressed()), boardHandler, SLOT(slotNavPrevVar()));
	connect(ui.navFirst, SIGNAL(pressed()), boardHandler, SLOT(slotNavFirst()));
	connect(ui.navLast, SIGNAL(pressed()), boardHandler, SLOT(slotNavLast()));
	connect(ui.navMainBranch, SIGNAL(pressed()), boardHandler, SLOT(slotNavMainBranch()));
	connect(ui.navStartVar, SIGNAL(pressed()), boardHandler, SLOT(slotNavStartVar()));
	connect(ui.navNextBranch, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextBranch()));
	connect(ui.navIntersection, SIGNAL(pressed()), boardHandler, SLOT(slotNavIntersection()));
	connect(ui.navPrevComment, SIGNAL(pressed()), boardHandler, SLOT(slotNavPrevComment()));
	connect(ui.navNextComment, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextComment()));
	connect(ui.navNextComment, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextComment()));
	connect(ui.slider, SIGNAL(sliderMoved ( int)), boardHandler , SLOT(slotNthMove(int)));


	connect(ui.board, SIGNAL(signalWheelEvent(QWheelEvent*)),
		boardHandler, SLOT(slotWheelEvent(QWheelEvent*)));


	//Connects the 'edit' buttons to the slots
	connect(editButtons, SIGNAL(buttonPressed ( int )), 
		this, SLOT(slotEditButtonPressed( int )));
	connect(ui.deleteButton,SIGNAL(pressed()), this, SLOT(slotEditDelete()));


	connect(ui.actionCoordinates, SIGNAL(toggled(bool)), SLOT(slotViewCoords(bool)));
	connect(ui.actionFileSave, SIGNAL(triggered(bool)), SLOT(slotFileSave()));
	connect(ui.actionFileSaveAs, SIGNAL(triggered(bool)), SLOT(slotFileSaveAs()));
	connect(ui.actionSound, SIGNAL(toggled(bool)), SLOT(slotSound(bool)));
	connect(ui.actionExportSgfClipB, SIGNAL(triggered(bool)), SLOT(slotExportSGFtoClipB()));
	connect(ui.actionExportPicClipB, SIGNAL(triggered(bool)), SLOT(slotExportPicClipB()));
	connect(ui.actionExportPic, SIGNAL(triggered(bool)), SLOT(slotExportPic()));
	connect(ui.actionDuplicate, SIGNAL(triggered(bool)), SLOT(slotDuplicate()));

	if(gameData->gameMode == modeObserve || gameData->gameMode == modeMatch || gameData->gameMode == modeReview)
	{
		observerListModel = new ObserverListModel();
		ui.observerView->setModel(observerListModel);
	}
	/* Set column widths ?? */
}


void BoardWindow::setupBoardUI(void)
{
	//make sure to set the sound button to the proper state before anything
	if (qgoboard->getPlaySound())
	{
		ui.actionSound->setChecked(FALSE);
		ui.actionSound->setIcon(QIcon(":/new/prefix1/ressources/pics/sound_on.png"));
	}
	else
	{
		ui.actionSound->setChecked(TRUE);
		ui.actionSound->setIcon(QIcon(":/new/prefix1/ressources/pics/sound_off.png"));
	}

	//Connects the board to the interface and boardhandler
	connect(ui.board, SIGNAL(signalClicked(bool , int, int, Qt::MouseButton )) , 
		qgoboard , SLOT( slotBoardClicked(bool, int, int , Qt::MouseButton )));

	//Connects the game buttons to the slots
	connect(ui.passButton,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
	connect(ui.passButton_2,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
	connect(ui.scoreButton,SIGNAL(toggled(bool)), qgoboard, SLOT(slotScoreToggled(bool)));
	
	
	connect(ui.doneButton,SIGNAL(pressed()), qgoboard, SLOT(slotDonePressed()));
	connect(ui.reviewButton,SIGNAL(pressed()), qgoboard, SLOT(slotReviewPressed()));	
	connect(ui.undoButton,SIGNAL(pressed()), qgoboard, SLOT(slotUndoPressed()));
	if (gameData->gameMode == modeMatch || gameData->gameMode == modeComputer)
	connect(ui.resignButton,SIGNAL(pressed()), qgoboard, SLOT(slotResignPressed()));
	if(gameData->gameMode == modeMatch)
	connect(ui.adjournButton,SIGNAL(pressed()), qgoboard, SLOT(slotAdjournPressed()));
	/* eb added this but I've had it in the setupUI function since
	 * its more part of the UI than the board.  But maybe its better
	 * or different here.  FIXME */
	/*
	connect(ui.actionCoordinates, SIGNAL(toggled(bool)), SLOT(slotViewCoords(bool)));
	connect(ui.actionFileSave, SIGNAL(triggered(bool)), SLOT(slotFileSave()));
	connect(ui.actionFileSaveAs, SIGNAL(triggered(bool)), SLOT(slotFileSaveAs()));
	connect(ui.actionSound, SIGNAL(toggled(bool)), SLOT(slotSound(bool)));
	connect(ui.actionExportSgfClipB, SIGNAL(triggered(bool)), SLOT(slotExportSGFtoClipB()));
	connect(ui.actionExportPicClipB, SIGNAL(triggered(bool)), SLOT(slotExportPicClipB()));
	connect(ui.actionExportPic, SIGNAL(triggered(bool)), SLOT(slotExportPic()));
	connect(ui.actionDuplicate, SIGNAL(triggered(bool)), SLOT(slotDuplicate()));
	connect(ui.actionGameInfo, SIGNAL(triggered(bool)), SLOT(slotGameInfo(bool)));
	*/

	// Needs Adjourn button ????

	//connects the comments and edit line to the slots
	connect(ui.commentEdit, SIGNAL(textChanged()), qgoboard, SLOT(slotUpdateComment()));
	if (gameData->gameMode != modeNormal && gameData->gameMode != modeComputer )
	connect(ui.commentEdit2, SIGNAL(returnPressed()), qgoboard, SLOT(slotSendComment()));
//connect(ui.scoreButton,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));

}

void BoardWindow::resizeEvent(QResizeEvent *)
{
	qDebug("boardwindow resize event\n");
}

int BoardWindow::checkModified(bool /*interactive*/)
{	
	if (!qgoboard->getModified())
		return 1;
	
//	if (!interactive)
//		return 0;
	
	switch (QMessageBox::warning(this, PACKAGE,
		tr("You modified the game.\nDo you want to save your changes?"),
		tr("Yes"), tr("No"), tr("Cancel"),
		0, 2))
	{
		case 0:
			return slotFileSave() && !qgoboard->getModified();
			
		case 1:
			return 1;
			
		case 2:
			return 2;
			
		default:
			qWarning("Unknown messagebox input.");
			return 0;
	}
		
	return 1;
}

void BoardWindow::gameDataChanged(void)
{
	interfaceHandler->updateCaption(gameData);
}

void BoardWindow::setGameData(GameData *gd)
{
	qDebug("BoardWindow::setGameData deprecated!!!!");
	gameData = new GameData(gd); 
	interfaceHandler->updateCaption(gd);
}

void BoardWindow::swapColors(bool noswap)
{
	if(!noswap)
	{
		QString rank, name;
		
		name = gameData->black_name;
		rank = gameData->black_rank;
		gameData->black_name = gameData->white_name;
		gameData->black_rank = gameData->white_rank;
		gameData->white_name = name;
		gameData->white_rank = rank;
	}
	gameData->nigiriToBeSettled = false;
	if(gameData->black_name == dispatch->getUsername())
	{
		myColorIsBlack = true;
		myColorIsWhite = false;
	}
	else
	{
		myColorIsBlack = false;
		myColorIsWhite = true;
	}
	interfaceHandler->updateCaption(gameData);
	getBoardHandler()->updateCursor();	//appropriate?
	//also need to start any timers if necessary
	//also network timers in addition to game timers
}
/*
 * Loads the SGF string. returns true if the file was sucessfully parsed
 */
bool BoardWindow::loadSGF(const QString fileName, const QString SGF)
{

	SGFParser *sgfParser = new SGFParser(tree);
	
	QString SGFLoaded;
	if (SGF.isEmpty())
		// Load the sgf file
		SGFLoaded = sgfParser->loadFile(fileName);
	else 
		SGFLoaded = SGF;

	if (!sgfParser->doParse(SGFLoaded))
		return false ;	
	
//	board->clearData();
//	tree->setToFirstMove();	
//	boardHandler->slotNavFirst();

	return true;
}

/*
 * One of the buttons in the 'Edit Tools' frame has been pressed
 */
void BoardWindow::slotEditButtonPressed( int m )
{
	QString txt;
	
	//we have to emulate exclusive buttons, while raise the button if it was down
	if (m >= 0 && m < 7 )
	{
		if ( m== editButtons->checkedId())
		{
//			editButtons->button (m)->setChecked( FALSE );
			gamePhase = phaseOngoing;
			boardHandler->updateCursor(qgoboard->getBlackTurn() ? stoneWhite : stoneBlack);
			editButtons->button (7)->setDisabled( FALSE );
		}
		else
		{
			gamePhase = phaseEdit;
			boardHandler->updateCursor( );
			editButtons->button (7)->setDisabled( TRUE );

			for (int i= 0;i<7;i++)
				if (i!=m)
					editButtons->button (i)->setChecked( FALSE );

		}
	}
	switch(m)
	{
	case 0:
		editMark = markNone;
		break;
		
	case 1:
		editMark = markSquare;
		break;
		
	case 2:
		editMark = markCircle;
		break;
		
	case 3:
		editMark = markTriangle;
		break;
		
	case 4:
		editMark = markCross;
		break;
		
	case 5:
		editMark = markText;
		break;
		
	case 6:
		editMark = markNumber;
		break;

	case 7:
	{
		Move *current = tree->getCurrent();
		// set next move's color
		if (qgoboard->getBlackTurn())
		{
			current->setPLinfo(stoneWhite);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_WHITE));
//#else
			ui.colorButton->setIcon(QIcon(":/new/prefix1/ressources/pics/stone_white.png"));
			boardHandler->updateCursor(stoneBlack);
//#endif
		}
		else
		{
			current->setPLinfo(stoneBlack);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_BLACK));
//#else
			ui.colorButton->setIcon(QIcon(":/new/prefix1/ressources/pics/stone_black.png"));
			boardHandler->updateCursor(stoneWhite);
//#endif
		}
		// check if set color is natural color:
		if ((current->getMoveNumber() == 0 && current->getPLnextMove() == stoneBlack) ||
			(current->getMoveNumber() > 0 && current->getColor() != current->getPLnextMove()))
			current->clearPLinfo();
//		board->setCurStoneColor();
		return;
	}
		
	default:
		return;
	}
	
//	statusMark->setText(getStatusMarkText(t));
//	board->setMarkType(t);
}


/*
 * button menu 'export to clipboard' activated
 */
void BoardWindow::slotExportSGFtoClipB()
{
	QString str = "";

	SGFParser *p = new SGFParser( tree);

	if (!p->exportSGFtoClipB(&str, tree, gameData))
	{
		QMessageBox::warning(this, tr("Export"), tr("Could not export  the game to clipboard"));
//		qDebug ("QGoboard:setMove - move %d %d done",i,j);
		return ;
	}

	QApplication::clipboard()->setText(str);

}


/*
 * button menu 'export picture' activated
 */
void BoardWindow::slotExportPic()
{
	QString *filter = new QString();
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Export image as"),
		QString(""),
		QString("JPEG (*.jpeg);;PNG (*.png);;BMP (*.bmp);;XPM (*.xpm);;XBM (*.xbm);;PNM (*.pnm);;GIF (*.gif);;MNG (*.mng)"),
		filter);


	if (fileName.isEmpty())
		return;


		// Confirm overwriting file.
//	if ( QFile::exists( fileName ) )
//		if (QMessageBox::information(this, PACKAGE,
//			tr("This file already exists. Do you want to overwrite it?"),
//			tr("Yes"), tr("No"), 0, 0, 1) == 1)
//			return;

	QString ext = 	"." + filter->section(" ",0,0).toLower();
	if (!fileName.endsWith(ext))
		fileName.append(ext);

	ui.board->exportPicture(fileName, new QString(filter->section(" ",0,0)), FALSE);//->left(3));
}


/*
 * button menu 'export picture to clipboard' activated
 */
void BoardWindow::slotExportPicClipB()
{
	QString null = "";
	ui.board->exportPicture(0, 0 , TRUE);	
}



/*
 * button 'duplicate board' activated
 */
void BoardWindow::slotDuplicate()
{
	QString sgf = "";
	QString filename;
	
	SGFParser *p = new SGFParser( tree);

	if (!p->exportSGFtoClipB(&sgf, tree, gameData))
	{
		QMessageBox::warning(this, tr("Export"), tr("Could not duplicate the game"));
//		qDebug ("QGoboard:setMove - move %d %d done",i,j);
		return ;
	}
	GameData * gd = new GameData(gameData);
	gd->gameMode = modeNormal;
	gd->fileName = "";
	BoardWindow *b = new BoardWindow(gd, TRUE, TRUE);

	b->loadSGF(0,sgf);
	b->getBoardHandler()->slotNthMove(tree->getCurrent()->getMoveNumber());
}


/*
 * button 'coordinates' has been toggled
 */
void BoardWindow::slotViewCoords(bool toggle)
{
	if (toggle)
		ui.board->setShowCoords(false);
	else
		ui.board->setShowCoords(true);
	
//	statusBar()->message(tr("Ready."));
}

void BoardWindow::slotGameInfo(bool /*toggle*/)
{
	QDialog *dlg = new QDialog;
	Ui::GameinfoDialog ui;
	ui.setupUi( dlg );
	ui.whiteName->setText( gameData->white_name );
	ui.blackName->setText( gameData->black_name );
	ui.whiteRank->setText( gameData->white_rank );
	ui.blackRank->setText( gameData->black_rank );
	ui.komi->setText( QString::number(gameData->komi ));
	ui.handicap->setText( QString::number(gameData->handicap ));
	ui.result->setText( gameData->result );
	ui.gameName->setText( gameData->gameName );
	ui.date->setText( gameData->date );
	ui.playedAt->setText( gameData->place );
	ui.copyright->setText( gameData->copyright );
	ui.whiteName->setReadOnly( 1 );
	ui.blackName->setReadOnly( 1 );
	ui.whiteRank->setReadOnly( 1 );
	ui.blackRank->setReadOnly( 1 );
	ui.komi->setReadOnly(1);
	ui.handicap->setReadOnly(1);
	ui.result->setReadOnly(1);
	ui.gameName->setReadOnly(1);
	ui.date->setReadOnly(1);
	ui.playedAt->setReadOnly(1);
	ui.copyright->setReadOnly(1);
	dlg->show();
	
//	statusBar()->message(tr("Ready."));
}


/*
 * button 'sound' has been toggled
 */
void BoardWindow::slotSound(bool toggle)
{
	qgoboard->setPlaySound(!toggle);
	if (toggle)
		ui.actionSound->setIcon(QIcon(":/new/prefix1/ressources/pics/sound_off.png"));
	else
		ui.actionSound->setIcon(QIcon(":/new/prefix1/ressources/pics/sound_on.png"));
}




/* 
 * Saves the game tree under file 'filename'
 * Note that qt handles overwrite check. */
bool BoardWindow::doSave(QString fileName, bool force)
{
	if (!force)
  	{
     		if  (fileName == NULL ||
			fileName.isNull() ||
          		fileName.isEmpty() ||
          		QDir(fileName).exists())
            	{
              		QString base = getCandidateFileName();
              		if (fileName == NULL || fileName.isNull() || fileName.isEmpty())
                		fileName = base;
              		else
                		fileName.append(base);

		}
		fileName = QFileDialog::getSaveFileName(this,tr("Save File"), fileName, tr("SGF Files (*.sgf);;All Files (*)") );
	}
	
	if (fileName.isEmpty())
		return false;
	
//	if (getFileExtension(fileName, false).isEmpty())

	if (fileName.right(4).toLower() != ".sgf")
		fileName.append(".sgf");
	
	gameData->fileName = fileName;
		
//	if (setting->readBoolEntry("REM_DIR"))
//		rememberLastDir(fileName);

	SGFParser *p = new SGFParser( tree); //FIXME : we may need to have a class SGFParser
	if (!p->doWrite(fileName, tree, gameData))
	{
		QMessageBox::warning(this, PACKAGE, tr("Cannot save SGF file."));
		return false;
	}
		
//	statusBar()->message(fileName + " " + tr("saved."));
	qgoboard->setModified(false);
	return true;
}
void BoardWindow::setGamePhase(GamePhase gp)
{
	gamePhase = gp;
	/* FIXME We should set and clear buttons and the like here */
	switch(gp)
	{
		case phaseEnded:
			if(ui.undoButton)
				ui.undoButton->setDisabled(true);
			if(ui.resignButton)
				ui.resignButton->setDisabled(true);
			if(ui.adjournButton)
				ui.adjournButton->setDisabled(true);
			if(ui.refreshButton)		//what the hell is this for FIXME
				ui.refreshButton->setDisabled(true);
			if(ui.passButton)
				ui.passButton->setDisabled(true);
			// right? enable done button?
			// FIXME, not getting enabled...
			if(ui.doneButton)
				ui.doneButton->setEnabled(true);
			break;
		case phaseOngoing:
			/* among many other things: FIXME, adding as we go, for now,
			 * sloppy */
			if(ui.passButton)
				ui.passButton->setEnabled(true);
			break;
		case phaseScore:
			/* Maybe we should take everything out of
			 * qgoboard enterScoreMode and put it here */
			/* We might also disable the pass button... except
			 * really maybe its already disabled from being not
			 * our turn.. also, should the button's being
			 * disabled prevent sending the message instead
			 * of the button checking state?  Probably. FIXME */
			if(ui.passButton)
				ui.passButton->setDisabled(true);
			/* FIXME doublecheck, what is doneButton connected to
			 * in observing a game ?? */
			ui.doneButton->setEnabled(true);
			break;
		default:
			break;
	}
}

/*
 * Generate a candidate for the filename for this game
 */
QString BoardWindow::getCandidateFileName()
{
	
	QString base = QDate::currentDate().toString("yyyy-MM-dd") + "-" + gameData->white_name + "-" + gameData->black_name    ;
	QString result = base ;
	QString dir= "" ;

//	if (setting->readBoolEntry("REM_DIR"))
//			dir = setting->readEntry("LAST_DIR");
	int i = 1;
	while (QFile(dir + result+".sgf").exists())
	{
		//number = Q.number(i++);
		result = base + "-"+ QString::number(i++);
		//fileName = fileName + ".sgf";
	} 
	return dir + result + ".sgf";
}

/*
 * The 'save' icon has been pressed
 */
bool BoardWindow::slotFileSave()
{
	QString fileName= gameData->fileName;
	if (fileName.isEmpty())
	{
//		if (setting->readBoolEntry("REM_DIR"))
//			fileName = setting->readEntry("LAST_DIR");
//		else
//			fileName = QString::null;
		return doSave(fileName, FALSE);
	}
	else
		return doSave(fileName, TRUE);
}


/*
 * The 'save as' icon has been pressed
 */
bool BoardWindow::slotFileSaveAs()
{
	return doSave(0, FALSE);
}

/*
 * The 'delete node' button has been pressed
 */
void BoardWindow::slotEditDelete()
{
	tree->deleteNode();
}

/* FIXME this comes up with unrelated keys, which is okay I guess, little
 * annoying. */
void BoardWindow::keyPressEvent(QKeyEvent *e)
{
	qDebug("boardwindow.cpp: key pressed");

#if 0
	// check for window resize command = number button
	if (e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9)
	{
		QString strKey = QString::number(e->key() - Qt::Key_0);
		
		if (e->state() & AltButton)
		{
			// true -> store
			reStoreWindowSize(strKey, true);
			return;
		}
		else if (e->state() & ControlButton)
		{
			// false -> restore
			if (!reStoreWindowSize(strKey, false))
			{
				// sizes not found -> leave
				e->ignore();
				return;
			}
			
			return;
		}
	}
#endif
	
//	bool localGame = true;
	// don't view last moves while observing or playing
	if (//gameData->gameMode == modeObserve ||
		   gameData->gameMode == modeMatch ||
		   gameData->gameMode == modeTeach)
	{
		qDebug("Not local game.\n");
//		localGame = false;
//		e->accept();
//		e->ignore();
		return;
	}
	
	switch (e->key())
	{
		/*
		// TODO: DEBUG
#ifndef NO_DEBUG
case Key_W:
board->debug();
break;

case Key_L:
board->openSGF("foo.sgf");
break;

case Key_S:
board->saveBoard("foo.sgf");
break;

case Key_X:
board->openSGF("foo.xml", "XML");
break;
		// /DEBUG
#endif
*/

		case Qt::Key_Left:
			boardHandler->slotNavBackward();
			break;

		case Qt::Key_Right:
			boardHandler->slotNavForward();
			break;

		case Qt::Key_Up:
			boardHandler->slotNavPrevVar();
			break;

		case Qt::Key_Down:
			boardHandler->slotNavNextVar();
			break;

		case Qt::Key_Home:
			boardHandler->slotNavFirst();
			break;

		case Qt::Key_End:
			boardHandler->slotNavLast();
			break;

		default:
			e->ignore();
	}

	e->accept();
}

void BoardWindow::setBoardDispatch(BoardDispatch * d)
{
	dispatch = d;
}

