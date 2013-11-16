/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
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
#include "boardwindow.h"
#include "boardhandler.h"
#include "board.h"
#include "clockdisplay.h"
#include "interfacehandler.h"
#include "qgoboard.h"
#include "move.h"
#include "tree.h"
#include "sgf/sgfparser.h"
#include "../network/boarddispatch.h"
#include "gameinfo.h"

class BoardHandler;

BoardWindow::BoardWindow(GameData *gd, bool iAmBlack , bool iAmWhite, class BoardDispatch * _dispatch)
	: QMainWindow(0, 0), addtime_menu(0)
{
    if(gd != NULL)
    {
        gameData = gd;
    } else {
		gameData = new GameData();
		gameData->gameMode = modeNormal;
	}
	
	dispatch = _dispatch;
	
	if(gameData->nigiriToBeSettled)
	{
		myColorIsBlack = false;
		myColorIsWhite = false;
	}
	else if(gameData->gameMode == modeComputer)
	{
			if(gameData->white_name == "Human")		//is this reliable?
			{
				myColorIsWhite = true;
				myColorIsBlack = false;
			}
			else
			{
				myColorIsWhite = false;
				myColorIsBlack = true;
			}
	}
	else
	{
		myColorIsBlack = iAmBlack;
		myColorIsWhite = iAmWhite;
	}
	
	gamePhase = phaseInit;
	boardSize = gd->board_size;
	
	//Creates the game tree
	tree = new Tree(&boardSize);

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

	setGamePhase(phaseOngoing);
	show();
	checkHideToolbar(height());
	setFocus();


	// Computer may have to make the first move
	if(gameData->gameMode == modeComputer)
		qgoboard->startGame();
	
	//update();
	//gridLayout->update();

	if(gameData->record_sgf != QString())
		loadSGF(0, gameData->record_sgf);
}

BoardWindow::~BoardWindow()
{
	qDebug("Deleting BoardWindow\n");
	QSettings settings;
	
	settings.setValue("BOARD_WINDOW_SIZE_X", width());
	settings.setValue("BOARD_WINDOW_SIZE_Y", height());
//	settings.setValue("BOARD_SIZES", ui.boardSplitter->saveState());
	
	delete tree;	//okay?
	
	delete gameData;
	delete addtime_menu;
}

bool BoardWindow::okayToQuit(void)
{
	if (!qgoboard->getModified())
		return true;

	QSettings settings;
#ifdef UNNECESSARY
	if(getGameMode() == modeComputer && !settings.value("WARNONCLOSEENGINE").toBool())
		return true;
	if(getGameMode() == modeNormal && !settings.value("WARNONCLOSEEDITED").toBool())
		return true;
#endif //UNNECESSARY
	switch (QMessageBox::warning(this, PACKAGE,
		tr("You modified the game.\nDo you want to save your changes?"),
		tr("Yes"), tr("No"), tr("Cancel"),
		0, 2))
	{
		case 0:
			return slotFileSave() && !qgoboard->getModified();
			
		case 1:
			return true;
			
		case 2:
			return false;
			
		default:
			qWarning("Unknown messagebox input.");
			return false;
	}
	return true;
}

/* FIXME make sure closeEvent doesn't get called on delete */
void BoardWindow::closeEvent(QCloseEvent *e)
{
	if(dispatch && !dispatch->canClose())
	{
		e->ignore();
	}
	else if (okayToQuit())	
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


    moveNumLabel = new QLabel(ui.statusbar);
    komiLabel = new QLabel(ui.statusbar);
    buyoyomiLabel = new QLabel(ui.statusbar);
    handicapLabel = new QLabel(ui.statusbar);
    freeratedLabel = new QLabel(ui.statusbar);
    ui.statusbar->addPermanentWidget(moveNumLabel);
    ui.statusbar->addPermanentWidget(handicapLabel);
    ui.statusbar->addPermanentWidget(komiLabel);
    ui.statusbar->addPermanentWidget(buyoyomiLabel);
    ui.statusbar->addPermanentWidget(freeratedLabel);

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
	editButtons->setExclusive(false);


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

	clockDisplay = new ClockDisplay(this, gameData->timeSystem, gameData->maintime, gameData->stones_periods, gameData->periodtime);
	
	if(dispatch && dispatch->flipCoords())
		ui.board->setCoordType(numberbottomi);
	else
		ui.board->setCoordType(numbertopnoi);
	ui.board->init(boardSize);
	
	interfaceHandler = new InterfaceHandler( this);
    interfaceHandler->toggleMode(gameData->gameMode);
    interfaceHandler->updateCaption(gameData);

	// creates the board handler for navigating in the tree
	boardHandler = new BoardHandler(this, tree, &boardSize);

	int window_x, window_y;
	QVariant board_window_size_x = settings.value("BOARD_WINDOW_SIZE_X");
	if(board_window_size_x != QVariant())
	{	
		window_x = board_window_size_x.toInt();
		window_y = settings.value("BOARD_WINDOW_SIZE_Y").toInt();
		resize(window_x, window_y);
	}
	QVariant board_sizes_for_splitter = settings.value("BOARD_SIZES");
//	if(board_sizes_for_splitter != QVariant())
//		ui.boardSplitter->restoreState(board_sizes_for_splitter.toByteArray());
	
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
	connect(ui.slider, SIGNAL(sliderMoved ( int)), boardHandler , SLOT(slotNthMove(int)));


	connect(ui.board, SIGNAL(signalWheelEvent(QWheelEvent*)),
		boardHandler, SLOT(slotWheelEvent(QWheelEvent*)));


	//Connects the 'edit' buttons to the slots
	connect(editButtons, SIGNAL(buttonPressed ( int )), 
		this, SLOT(slotEditButtonPressed( int )));
	connect(ui.insertMoveButton, SIGNAL(toggled(bool)), SLOT(slotToggleInsertStones(bool)));
	connect(ui.deleteButton,SIGNAL(pressed()), this, SLOT(slotEditDelete()));


    connect(ui.actionCoordinates, SIGNAL(toggled(bool)), SLOT(slotShowCoords(bool)));
	connect(ui.actionFileSave, SIGNAL(triggered(bool)), SLOT(slotFileSave()));
	connect(ui.actionFileSaveAs, SIGNAL(triggered(bool)), SLOT(slotFileSaveAs()));
	connect(ui.actionSound, SIGNAL(toggled(bool)), SLOT(slotSound(bool)));
	connect(ui.actionExportSgfClipB, SIGNAL(triggered(bool)), SLOT(slotExportSGFtoClipB()));
	connect(ui.actionExportPicClipB, SIGNAL(triggered(bool)), SLOT(slotExportPicClipB()));
	connect(ui.actionExportPic, SIGNAL(triggered(bool)), SLOT(slotExportPic()));
	connect(ui.actionDuplicate, SIGNAL(triggered(bool)), SLOT(slotDuplicate()));
	connect(ui.actionGameInfo, SIGNAL(triggered(bool)), SLOT(slotGameInfo(bool)));

	/* Set column widths ?? */
}

void BoardWindow::setupBoardUI(void)
{
	//make sure to set the sound button to the proper state before anything
    ui.actionSound->setChecked(qgoboard->getPlaySound());
	
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
	{
		connect(ui.adjournButton,SIGNAL(pressed()), qgoboard, SLOT(slotAdjournPressed()));
		connect(ui.countButton,SIGNAL(pressed()), qgoboard, SLOT(slotCountPressed()));
		connect(ui.drawButton,SIGNAL(pressed()), qgoboard, SLOT(slotDrawPressed()));
	}
	if(gameData->gameMode != modeMatch)
	{
		ui.adjournButton->setVisible(false);
		ui.countButton->setVisible(false);
		ui.drawButton->setVisible(false);
	}
	if(dispatch && !dispatch->supportsRequestAdjourn())
		ui.adjournButton->setVisible(false);
	if(dispatch && !dispatch->supportsRequestCount())
		ui.countButton->setVisible(false);
	if(dispatch && !dispatch->supportsRequestDraw())
		ui.drawButton->setVisible(false);

	ui.insertMoveButton->setChecked(false);
	if(gameData->gameMode == modeNormal)
		ui.insertMoveButton->setEnabled(true);
	else
		ui.insertMoveButton->setEnabled(false);
	if(gameData->gameMode == modeMatch /* or teaching? */ && dispatch && dispatch->supportsAddTime())
	{
		addtime_menu = new QMenu();
		QAction * act = addtime_menu->addAction(tr("Add 1 min"));
		act->setData(1);
		act = addtime_menu->addAction(tr("Add 5 min"));
		act->setData(5);
		act = addtime_menu->addAction(tr("Add 10 min"));
		act->setData(10);
		act = addtime_menu->addAction(tr("Add 60 min"));
		act->setData(60);
		act = addtime_menu->addAction(tr("Cancel"));
		act->setData(-1);
	
		/* Right now, IGS is the only server with addtime, but if nigiri is done in another order
		 * this will have to be moved elsewhere: */
		if(!gameData->nigiriToBeSettled)
		{
			if(myColorIsBlack)
			{
				ui.pb_timeWhite->setMenu(addtime_menu);
			}
			else if(myColorIsWhite)
			{
				ui.pb_timeBlack->setMenu(addtime_menu);
			}
			else
				qDebug("Warning: Nigiri settled in match mode but player has no color");
			connect(addtime_menu, SIGNAL(triggered(QAction*)), SLOT(slot_addtime_menu(QAction*)));	
		}
	}
	
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
}

void BoardWindow::resizeEvent(QResizeEvent * e)
{
	checkHideToolbar(e->size().height());	
}

void BoardWindow::checkHideToolbar(int h)
{
    ui.varLabel->setVisible(h > 700);
}

void BoardWindow::gameDataChanged(void)
{
	interfaceHandler->updateCaption(gameData);
	clockDisplay->setTimeSettings(gameData->timeSystem, gameData->maintime, gameData->periodtime, gameData->stones_periods);
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
	//getBoardHandler()->updateCursor();	//appropriate?
	boardHandler->updateCursor(qgoboard->getBlackTurn() ? stoneWhite : stoneBlack);
	//also need to start any timers if necessary
	//also network timers in addition to game timers
}

void BoardWindow::saveRecordToGameData(void)
{
	QString sgf = "";
	SGFParser *p = new SGFParser( tree);

	/* FIXME potentially there's never a reason to not save the record to the gameData or the like
	 * etc., we'll see how this works out, but we might want to change some of this */
	if (!p->exportSGFtoClipB(&sgf, tree, gameData))
	{
		QMessageBox::warning(this, tr("Export"), tr("Could not duplicate the game"));
		return ;
	}
	gameData->record_sgf = sgf;
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

    return sgfParser->doParse(SGFLoaded);
}

/*
 * One of the buttons in the 'Edit Tools' frame has been pressed
 */
void BoardWindow::slotEditButtonPressed( int m )
{
	QString txt;
	Move *current = tree->getCurrent();
	
	//we have to emulate exclusive buttons, while raise the button if it was down
	if (m >= 0 && m < 7 )
	{
		if ( m== editButtons->checkedId())
		{
//			editButtons->button (m)->setChecked( false);
			gamePhase = phaseOngoing;
			boardHandler->updateCursor(qgoboard->getBlackTurn() ? stoneWhite : stoneBlack);
			editButtons->button (7)->setDisabled( false);
		}
		else
		{
			gamePhase = phaseEdit;
			boardHandler->updateCursor( );		//this still has turn change FIXME
			editButtons->button (7)->setDisabled( true);

			for (int i= 0;i<7;i++)
				if (i!=m)
					editButtons->button (i)->setChecked( false);

		}
	}
	switch(m)
	{
	case 0:
		editMark = markNone;
		if(gamePhase == phaseEdit)
			current->setGamePhase(phaseEdit);	//permanent on this move, right?
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
		// set next move's color
		if (qgoboard->getBlackTurn())
		{
			current->setPLinfo(stoneWhite);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_WHITE));
//#else
			ui.colorButton->setIcon(QIcon(":/boardicons/resources/pics/stone_white.png"));
			boardHandler->updateCursor(stoneBlack);
//#endif
		}
		else
		{
			current->setPLinfo(stoneBlack);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_BLACK));
//#else
			ui.colorButton->setIcon(QIcon(":/boardicons/resources/pics/stone_black.png"));
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
        filter, QFileDialog::DontUseNativeDialog);


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

	ui.board->exportPicture(fileName, new QString(filter->section(" ",0,0)), false);//->left(3));
}

/*
 * button menu 'export picture to clipboard' activated
 */
void BoardWindow::slotExportPicClipB()
{
	QString null = "";
	ui.board->exportPicture(0, 0 , true);	
}

/*
 * button 'duplicate board' activated
 */
void BoardWindow::slotDuplicate()
{
	saveRecordToGameData();
	GameData * gd = new GameData(gameData);
	gd->gameMode = modeNormal;
	gd->fileName = "";
	BoardWindow *b = new BoardWindow(gd, true, true);
	
	//doublecheck FIXME
	/* Note also that this does not duplicate any ui.board->marks
	 * that are on the original board.  I think score marks qualify
	 * but come up some other way as well */
	/* Note also that this thing below is broken for duplicating edit
	 * boards which should be the default!! */
	/* Also duplicating edit boards removes score marks, so I'm wondering
	 * what the network code does to keep the score marks on the duplicated
	 * board and lose the rest */
	int mn = tree->getCurrent()->getMoveNumber();
	/* Removing, may not be broken at all */
	//if(mn > 0)
	//	mn--;
	b->getBoardHandler()->slotNthMove(mn);

	/* Doesn't seem to matter where the below is */
	/*b->setGamePhase(this->getGamePhase());				//maybe?
	if(this->getGamePhase() == phaseScore)
	{
		b->qgoboard->enterScoreMode();
		b->ui.scoreButton->setDown(true);
	}*/
}

/*
 * button 'coordinates' has been toggled
 */
void BoardWindow::slotShowCoords(bool toggle)
{
    ui.board->setShowCoords(toggle);
}

void BoardWindow::slotGameInfo(bool /*toggle*/)
{
	new GameInfo(this);
}

/*
 * button 'sound' has been toggled
 */
void BoardWindow::slotSound(bool toggle)
{
    qgoboard->setPlaySound(toggle);
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
        fileName = QFileDialog::getSaveFileName(this,tr("Save File"), fileName, tr("SGF Files (*.sgf);;All Files (*)"), new QString(""), QFileDialog::DontUseNativeDialog );
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
	/* FIXME We should set and clear ALL buttons and the like here */
	switch(gp)
	{
		case phaseInit:
			//more defaults FIXME, and doublecheck
			//also consider vanishing buttons instead of making
			//them disabled, especially with like computer go, etc.
			ui.adjournButton->setEnabled(false);
			ui.drawButton->setEnabled(false);
			break;
		case phaseEnded:
			if(ui.undoButton)
				ui.undoButton->setDisabled(true);
			if(ui.resignButton)
				ui.resignButton->setDisabled(true);
			if(ui.adjournButton)
				ui.adjournButton->setDisabled(true);
			if(ui.passButton)
				ui.passButton->setDisabled(true);
			if(ui.countButton)
				ui.countButton->setDisabled(true);
			if(ui.drawButton)
				ui.drawButton->setDisabled(true);
			/* Done button only for scoring, otherwise close
			 * window */
			if(ui.doneButton)
				ui.doneButton->setDisabled(true);
			break;
		case phaseOngoing:
			/* among many other things: FIXME, adding as we go, for now,
			 * sloppy */
			if(ui.passButton)
				ui.passButton->setEnabled(true);
			if(gameData->undoAllowed || gameData->gameMode == modeComputer)
				ui.undoButton->setEnabled(true);
			else
				ui.undoButton->setEnabled(false);
			if(gameData->gameMode == modeMatch)
			{
				if(gamePhase == phaseScore)	//was phaseScore
					ui.undoButton->setText(tr("Undo"));
				if(getBoardDispatch() && getBoardDispatch()->supportsRequestCount())
					ui.countButton->setEnabled(true);
				else
					ui.countButton->setEnabled(false);
				if(getBoardDispatch() && getBoardDispatch()->supportsRequestDraw())
					ui.drawButton->setEnabled(true);
				else
					ui.drawButton->setEnabled(false);
				if(getBoardDispatch() && getBoardDispatch()->supportsRequestAdjourn())
					ui.adjournButton->setEnabled(true);
				else
					ui.adjournButton->setEnabled(false);
			}
			break;
		case phaseScore:
			/* Maybe we should take everything out of
			 * qgoboard enterScoreMode and put it here */
			/* We might also disable the pass button... except
			 * really maybe its already disabled from being not
			 * our turn.. also, should the button's being
			 * disabled prevent sending the message instead
			 * of the button checking state?  Probably. FIXME */
			if(ui.countButton)
				ui.countButton->setDisabled(true);
			if(ui.undoButton && gameData->gameMode == modeMatch && getBoardDispatch())
			{
				if(getBoardDispatch()->supportsRequestMatchMode())
				{
					ui.undoButton->setText(tr("Match Mode"));
					ui.undoButton->setEnabled(true);	//even in rated
				}
				else if(getBoardDispatch()->undoResetsScore())
				{
					ui.undoButton->setEnabled(true);	//even in rated
				}
			}
			if(ui.passButton)
				ui.passButton->setDisabled(true);
			//Disable the draw button? 
			/* FIXME doublecheck, what is doneButton connected to
			 * in observing a game ?? */
			if(getBoardDispatch() && getBoardDispatch()->canMarkStonesDeadinScore())	//FIXME or maybe as for tygem, it accepts the score as is
				ui.doneButton->setEnabled(true);
			break;
		default:
			break;
	}
	gamePhase = gp;
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
		return doSave(fileName, false);
	}
	else
		return doSave(fileName, true);
}

/*
 * The 'save as' icon has been pressed
 */
bool BoardWindow::slotFileSaveAs()
{
	return doSave(0, false);
}

/*
 * The 'delete node' button has been pressed
 */
void BoardWindow::slotEditDelete()
{
	tree->deleteNode();
	boardHandler->updateMove(tree->getCurrent());
}

/* FIXME this comes up with unrelated keys, which is okay I guess, little
 * annoying. */
void BoardWindow::keyPressEvent(QKeyEvent *e)
{
    // don't view last moves while playing
    if (gameData->gameMode == modeMatch ||
            gameData->gameMode == modeTeach)
	{
		qDebug("Not local game.\n");
        e->ignore();
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

//this seems like its more appropriately a qgoboard slot but...
void BoardWindow::slot_addtime_menu(QAction * a)
{
	switch(a->data().toInt())
	{
		case -1:
			break;
		case 1:
			dispatch->sendAddTime(1);
			break;
		case 5:
			dispatch->sendAddTime(5);
			break;
		case 10:
			dispatch->sendAddTime(10);
			break;
		case 60:
			dispatch->sendAddTime(60);
			break;
	}
}

/*
 * say to tree to not create variation, insert stones directly
 */
void BoardWindow::slotToggleInsertStones(bool val)
{
	tree->setInsertStone(val);
}
