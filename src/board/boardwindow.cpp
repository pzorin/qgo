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

#include "boardwindow.h"
#include "board.h"
#include "clockdisplay.h"
#include "qgoboard.h"
#include "qgoboardlocalinterface.h"
#include "move.h"
#include "tree.h"
#include "../network/boarddispatch.h"
#include "gameinfo.h"
#include "matrix.h"
#include "ui_boardwindow.h"
#include <QtWidgets>

class BoardHandler;

BoardWindow::BoardWindow(GameData *gd, bool iAmBlack , bool iAmWhite, class BoardDispatch * _dispatch)
    : QMainWindow(0, 0), ui(new Ui::BoardWindow), addtime_menu(0), boardSize(gd->board_size)
{
    ui->setupUi(this);

    if(gd != NULL)
    {
        gameData = gd;
    } else {
		gameData = new GameData();
        gameData->gameMode = modeEdit;
	}
	
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

	//Creates the game tree
    tree = new Tree(boardSize, gameData->komi);
	
	//Loads the sgf file if any
	if (! gameData->fileName.isEmpty())
        tree->importSGFFile(gameData->fileName);

    QSettings settings;
    ui->actionWhatsThis = QWhatsThis::createAction ();

    moveNumLabel = new QLabel(ui->statusbar);
    komiLabel = new QLabel(ui->statusbar);
    buyoyomiLabel = new QLabel(ui->statusbar);
    handicapLabel = new QLabel(ui->statusbar);
    freeratedLabel = new QLabel(ui->statusbar);
    ui->statusbar->addPermanentWidget(moveNumLabel);
    ui->statusbar->addPermanentWidget(handicapLabel);
    ui->statusbar->addPermanentWidget(komiLabel);
    ui->statusbar->addPermanentWidget(buyoyomiLabel);
    ui->statusbar->addPermanentWidget(freeratedLabel);

    // Initialises the buttons and else
    editButtons = new QButtonGroup(this);
    editButtons->addButton(ui->stoneButton, 0);
    editButtons->addButton(ui->squareButton, 1);
    editButtons->addButton(ui->circleButton, 2);
    editButtons->addButton(ui->triangleButton, 3);
    editButtons->addButton(ui->crossButton, 4);
    editButtons->addButton(ui->labelLetterButton, 5);
    editButtons->addButton(ui->labelNumberButton, 6);
    editButtons->addButton(ui->colorButton, 7);
    editButtons->setExclusive(false);


    QMenu *menu = new QMenu();
//	menu->insertAction(0,ui->fileExportASCII);
    menu->insertAction(0,ui->actionExportSgfClipB);
    menu->insertAction(0,ui->actionExportPic);
    menu->insertAction(0,ui->actionExportPicClipB);

    ui->actionExport->setMenu(menu);

    QToolButton *exportButton = new QToolButton();
    exportButton->setDefaultAction(ui->actionExport);

    exportButton->setPopupMode( QToolButton::InstantPopup);
    ui->toolBar->insertWidget ( ui->actionImport, exportButton );

    clockDisplay = new ClockDisplay(this, gameData->timeSystem, gameData->maintime, gameData->stones_periods, gameData->periodtime);

    if(dispatch && dispatch->flipCoords())
        ui->board->setCoordType(numberbottomi);
    else
        ui->board->setCoordType(numbertopnoi);
    ui->board->init(boardSize);

    setMode(gameData->gameMode);
    updateCaption();

    int window_x, window_y;
    QVariant board_window_size_x = settings.value("BOARD_WINDOW_SIZE_X");
    if(board_window_size_x != QVariant())
    {
        window_x = board_window_size_x.toInt();
        window_y = settings.value("BOARD_WINDOW_SIZE_Y").toInt();
        resize(window_x, window_y);
    }

    // Connects the nav buttons to the slots
    connect(ui->navForward,SIGNAL(pressed()), tree, SLOT(slotNavForward()));
    connect(ui->navBackward,SIGNAL(pressed()), tree, SLOT(slotNavBackward()));
    connect(ui->navNextVar, SIGNAL(pressed()), tree, SLOT(slotNavNextVar()));
    connect(ui->navPrevVar, SIGNAL(pressed()), tree, SLOT(slotNavPrevVar()));
    connect(ui->navFirst, SIGNAL(pressed()), tree, SLOT(slotNavFirst()));
    connect(ui->navLast, SIGNAL(pressed()), tree, SLOT(slotNavLast()));
    connect(ui->navMainBranch, SIGNAL(pressed()), tree, SLOT(slotNavMainBranch()));
    connect(ui->navStartVar, SIGNAL(pressed()), tree, SLOT(slotNavStartVar()));
    connect(ui->navNextBranch, SIGNAL(pressed()), tree, SLOT(slotNavNextBranch()));
    connect(ui->navIntersection, SIGNAL(pressed()), this, SLOT(slotNavIntersection()));
    connect(ui->navPrevComment, SIGNAL(pressed()), tree, SLOT(slotNavPrevComment()));
    connect(ui->navNextComment, SIGNAL(pressed()), tree, SLOT(slotNavNextComment()));
    connect(ui->slider, SIGNAL(sliderMoved ( int)), tree , SLOT(slotNthMove(int)));

    wheelTime = QTime::currentTime();
    connect(ui->board, SIGNAL(signalWheelEvent(QWheelEvent*)), this, SLOT(slotWheelEvent(QWheelEvent*)));


    //Connects the 'edit' buttons to the slots
    connect(editButtons, SIGNAL(buttonPressed ( int )), this, SLOT(slotEditButtonPressed( int )));
    connect(ui->deleteButton,SIGNAL(pressed()), this, SLOT(slotEditDelete()));
    connect(ui->actionCoordinates, SIGNAL(toggled(bool)), SLOT(slotShowCoords(bool)));
    connect(ui->actionFileSave, SIGNAL(triggered(bool)), SLOT(slotFileSave()));
    connect(ui->actionFileSaveAs, SIGNAL(triggered(bool)), SLOT(slotFileSaveAs()));
    connect(ui->actionSound, SIGNAL(toggled(bool)), SLOT(slotSound(bool)));
    connect(ui->actionExportSgfClipB, SIGNAL(triggered(bool)), SLOT(slotExportSGFtoClipB()));
    connect(ui->actionExportPicClipB, SIGNAL(triggered(bool)), SLOT(slotExportPicClipB()));
    connect(ui->actionExportPic, SIGNAL(triggered(bool)), SLOT(slotExportPic()));
    connect(ui->actionDuplicate, SIGNAL(triggered(bool)), SLOT(slotDuplicate()));
    connect(ui->actionGameInfo, SIGNAL(triggered(bool)), SLOT(slotGameInfo(bool)));

    connect(tree, SIGNAL(currentMoveChanged(Move*)), this, SLOT(updateMove(Move*)));
    connect(tree, SIGNAL(scoreChanged(int,int,int,int,int,int)), this, SLOT(slotGetScore(int,int,int,int,int,int)));

    connect(ui->buttonModeEdit,SIGNAL(clicked()),SLOT(switchToEditMode()));
    connect(ui->buttonModeLocal,SIGNAL(clicked()),SLOT(switchToLocalMode()));

    ui->insertMoveButton->setChecked(false);
    ui->insertMoveButton->setEnabled(gameData->gameMode == modeEdit);
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
                ui->pb_timeWhite->setMenu(addtime_menu);
            }
            else if(myColorIsWhite)
            {
                ui->pb_timeBlack->setMenu(addtime_menu);
            }
            else
                qDebug("Warning: Nigiri settled in match mode but player has no color");
            connect(addtime_menu, SIGNAL(triggered(QAction*)), SLOT(slot_addtime_menu(QAction*)));
        }
    }

    //connects the comments and edit line to the slots
    connect(ui->commentEdit, SIGNAL(textChanged()), this, SLOT(slotUpdateComment()));
    connect(ui->commentEdit2, SIGNAL(returnPressed()), this, SLOT(slotSendComment()));

    connect(ui->computerBlack,SIGNAL(toggled(bool)),this,SLOT(setComputerBlack(bool)));
    connect(ui->computerWhite,SIGNAL(toggled(bool)),this,SLOT(setComputerWhite(bool)));
    connect(ui->computerMakeMove,SIGNAL(clicked()),this,SLOT(requestComputerMove()));
    ui->computerControlsWidget->setVisible(gameData->gameMode == modeLocal);

    if ((gameData->gameMode == modeLocal) || (gameData->gameMode == modeEdit))
    {
        connect(ui->blackName,SIGNAL(textChanged(QString)),SLOT(setBlackName(QString)));
        connect(ui->whiteName,SIGNAL(textChanged(QString)),SLOT(setWhiteName(QString)));
        ui->blackName->setReadOnly(false);
        ui->whiteName->setReadOnly(false);
    }

	//creates the board interface (or proxy) that will handle the moves an command requests
	switch (gameData->gameMode)
	{
        case modeEdit :
        case modeLocal :
        qgoboard = new qGoBoardLocalInterface(this, tree,gameData);
        ui->computerBlack->setChecked(!iAmBlack);
        ui->computerWhite->setChecked(!iAmWhite);
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
    if (gameData->gameMode == modeLocal)
        connect(ui->insertMoveButton, SIGNAL(toggled(bool)), static_cast<qGoBoardLocalInterface*>(qgoboard), SLOT(slotToggleInsertStones(bool)));
    //make sure to set the sound button to the proper state before anything
    ui->actionSound->setChecked(qgoboard->getPlaySound());

    //Connects the board to the interface and boardhandler
    connect(ui->board, SIGNAL(signalClicked(bool , int, int, Qt::MouseButton )), qgoboard, SLOT( slotBoardClicked(bool, int, int , Qt::MouseButton )));

    //Connects the game buttons to the slots
    connect(ui->passButton,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
    connect(ui->passButton_2,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
    connect(ui->scoreButton,SIGNAL(toggled(bool)), qgoboard, SLOT(slotScoreToggled(bool)));


    connect(ui->doneButton,SIGNAL(pressed()), qgoboard, SLOT(slotDonePressed()));
    connect(ui->reviewButton,SIGNAL(pressed()), qgoboard, SLOT(slotReviewPressed()));
    connect(ui->undoButton,SIGNAL(pressed()), qgoboard, SLOT(slotUndoPressed()));

    // Why is this conditional? FIXME
    if (gameData->gameMode == modeMatch || gameData->gameMode == modeLocal || gameData->gameMode == modeEdit)
        connect(ui->resignButton,SIGNAL(pressed()), qgoboard, SLOT(slotResignPressed()));

    if(gameData->gameMode == modeMatch)
    {
        connect(ui->adjournButton,SIGNAL(pressed()), qgoboard, SLOT(slotAdjournPressed()));
        connect(ui->countButton,SIGNAL(pressed()), qgoboard, SLOT(slotCountPressed()));
        connect(ui->drawButton,SIGNAL(pressed()), qgoboard, SLOT(slotDrawPressed()));
    } else {
        ui->adjournButton->hide();
        ui->countButton->hide();
        ui->drawButton->hide();
    }
    if(dispatch && !dispatch->supportsRequestAdjourn())
        ui->adjournButton->hide();
    if(dispatch && !dispatch->supportsRequestCount())
        ui->countButton->hide();
    if(dispatch && !dispatch->supportsRequestDraw())
        ui->drawButton->hide();

    setGamePhase(phaseOngoing);
    show();
	checkHideToolbar(height());
	setFocus();
	
	//update();
	//gridLayout->update();

	if(gameData->record_sgf != QString())
        tree->importSGFString(gameData->record_sgf);
}

BoardWindow::~BoardWindow()
{
	qDebug("Deleting BoardWindow\n");
	QSettings settings;
	
	settings.setValue("BOARD_WINDOW_SIZE_X", width());
	settings.setValue("BOARD_WINDOW_SIZE_Y", height());
//	settings.setValue("BOARD_SIZES", ui->boardSplitter->saveState());
	
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

void BoardWindow::resizeEvent(QResizeEvent * e)
{
	checkHideToolbar(e->size().height());	
}

void BoardWindow::checkHideToolbar(int h)
{
    ui->varLabel->setVisible(h > 700);
}

void BoardWindow::gameDataChanged(void)
{
    updateCaption();
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

    myColorIsBlack = (gameData->black_name == dispatch->getUsername());
    myColorIsWhite = !myColorIsBlack;

    updateCaption();
    updateCursor(qgoboard->getBlackTurn() ? stoneWhite : stoneBlack);
	//also need to start any timers if necessary
	//also network timers in addition to game timers
}

void BoardWindow::saveRecordToGameData(void)
{
    gameData->record_sgf = tree->exportSGFString(gameData);
}

/*
 * One of the buttons in the 'Edit Tools' frame has been pressed
 */
void BoardWindow::slotEditButtonPressed( int m )
{
    Move *current = tree->getCurrent();
	
	//we have to emulate exclusive buttons, while raise the button if it was down
	if (m >= 0 && m < 7 )
	{
		if ( m== editButtons->checkedId())
		{
//			editButtons->button (m)->setChecked( false);
			gamePhase = phaseOngoing;
            updateCursor(qgoboard->getBlackTurn() ? stoneWhite : stoneBlack);
			editButtons->button (7)->setDisabled( false);
		}
		else
		{
			gamePhase = phaseEdit;
            updateCursor( );		//this still has turn change FIXME
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
            ui->colorButton->setIcon(QIcon(":/boardicons/resources/pics/stone_white.png"));
            updateCursor(stoneBlack);
//#endif
		}
		else
		{
			current->setPLinfo(stoneBlack);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_BLACK));
//#else
            ui->colorButton->setIcon(QIcon(":/boardicons/resources/pics/stone_black.png"));
            updateCursor(stoneWhite);
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
    QString str = tree->exportSGFString(gameData);
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

    ui->board->exportPicture(fileName, new QString(filter->section(" ",0,0)), false);//->left(3));
}

/*
 * button menu 'export picture to clipboard' activated
 */
void BoardWindow::slotExportPicClipB()
{
	QString null = "";
    ui->board->exportPicture(0, 0 , true);
}

/*
 * button 'duplicate board' activated
 */
void BoardWindow::slotDuplicate()
{
	saveRecordToGameData();
	GameData * gd = new GameData(gameData);
    gd->gameMode = modeEdit;
	gd->fileName = "";
	BoardWindow *b = new BoardWindow(gd, true, true);
	
	//doublecheck FIXME
    /* Note also that this does not duplicate any ui->board->marks
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
    tree->slotNthMove(mn);

	/* Doesn't seem to matter where the below is */
	/*b->setGamePhase(this->getGamePhase());				//maybe?
	if(this->getGamePhase() == phaseScore)
	{
		b->qgoboard->enterScoreMode();
        b->ui->scoreButton->setDown(true);
	}*/
}

/*
 * button 'coordinates' has been toggled
 */
void BoardWindow::slotShowCoords(bool toggle)
{
    ui->board->setShowCoords(toggle);
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

    QString SGF = tree->exportSGFString(gameData);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(0, PACKAGE, QObject::tr("Could not open file:") + " " + fileName);
        return false;
    }
    file.write(SGF.toLatin1());
    file.flush();
    file.close();

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
            ui->adjournButton->setEnabled(false);
            ui->drawButton->setEnabled(false);
			break;
		case phaseEnded:
            if(ui->undoButton)
                ui->undoButton->setDisabled(true);
            if(ui->resignButton)
                ui->resignButton->setDisabled(true);
            if(ui->adjournButton)
                ui->adjournButton->setDisabled(true);
            if(ui->passButton)
                ui->passButton->setDisabled(true);
            if(ui->countButton)
                ui->countButton->setDisabled(true);
            if(ui->drawButton)
                ui->drawButton->setDisabled(true);
			/* Done button only for scoring, otherwise close
			 * window */
            if(ui->doneButton)
                ui->doneButton->setDisabled(true);
			break;
		case phaseOngoing:
			/* among many other things: FIXME, adding as we go, for now,
			 * sloppy */
        ui->tabDisplay->setCurrentIndex(0);
            if(ui->passButton)
                ui->passButton->setEnabled(true);
            if(gameData->undoAllowed || gameData->gameMode == modeLocal)
                ui->undoButton->setEnabled(true);
			else
                ui->undoButton->setEnabled(false);
			if(gameData->gameMode == modeMatch)
			{
				if(gamePhase == phaseScore)	//was phaseScore
                    ui->undoButton->setText(tr("Undo"));
				if(getBoardDispatch() && getBoardDispatch()->supportsRequestCount())
                    ui->countButton->setEnabled(true);
				else
                    ui->countButton->setEnabled(false);
				if(getBoardDispatch() && getBoardDispatch()->supportsRequestDraw())
                    ui->drawButton->setEnabled(true);
				else
                    ui->drawButton->setEnabled(false);
				if(getBoardDispatch() && getBoardDispatch()->supportsRequestAdjourn())
                    ui->adjournButton->setEnabled(true);
				else
                    ui->adjournButton->setEnabled(false);
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
            if(ui->countButton)
                ui->countButton->setDisabled(true);
            if(ui->undoButton && gameData->gameMode == modeMatch && getBoardDispatch())
			{
				if(getBoardDispatch()->supportsRequestMatchMode())
				{
                    ui->undoButton->setText(tr("Match Mode"));
                    ui->undoButton->setEnabled(true);	//even in rated
				}
				else if(getBoardDispatch()->undoResetsScore())
				{
                    ui->undoButton->setEnabled(true);	//even in rated
				}
			}
            if(ui->passButton)
                ui->passButton->setDisabled(true);
			//Disable the draw button? 
			/* FIXME doublecheck, what is doneButton connected to
			 * in observing a game ?? */
			if(getBoardDispatch() && getBoardDispatch()->canMarkStonesDeadinScore())	//FIXME or maybe as for tygem, it accepts the score as is
                ui->doneButton->setEnabled(true);
            ui->tabDisplay->setCurrentIndex(1);

            updateCursor();
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
            tree->slotNavBackward();
			break;

		case Qt::Key_Right:
            tree->slotNavForward();
			break;

		case Qt::Key_Up:
            tree->slotNavPrevVar();
			break;

		case Qt::Key_Down:
            tree->slotNavNextVar();
			break;

		case Qt::Key_Home:
            tree->slotNavFirst();
			break;

		case Qt::Key_End:
            tree->slotNavLast();
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

void BoardWindow::setComputerBlack(bool val)
{
    myColorIsBlack = !val;

    // dynamic_cast downcasts the pointer and returns null if this is not safe
    // This requires Run-Time Type Information to be enabled at compile time
    qGoBoardLocalInterface* qgoboard_temp = dynamic_cast<qGoBoardLocalInterface*>(qgoboard);
    if (qgoboard_temp != NULL)
    {
        qgoboard_temp->checkComputersTurn();
    }
}

void BoardWindow::setComputerWhite(bool val)
{
    myColorIsWhite = !val;

    // dynamic_cast downcasts the pointer and returns null if this is not safe
    // This requires Run-Time Type Information to be enabled at compile time
    qGoBoardLocalInterface* qgoboard_temp = dynamic_cast<qGoBoardLocalInterface*>(qgoboard);
    if (qgoboard_temp != NULL)
    {
        qgoboard_temp->checkComputersTurn();
    }
}

void BoardWindow::requestComputerMove()
{
    // dynamic_cast downcasts the pointer and returns null if this is not safe
    // This requires Run-Time Type Information to be enabled at compile time
    qGoBoardLocalInterface* qgoboard_temp = dynamic_cast<qGoBoardLocalInterface*>(qgoboard);
    if (qgoboard_temp != NULL)
    {
        qgoboard_temp->checkComputersTurn(true);
    }
}

void BoardWindow::setBlackName(QString name)
{
    gameData->black_name = name;
    ui->blackName->blockSignals(true);
    ui->blackName->setText(name);
    updateCaption();
    ui->blackName->blockSignals(false);
}

void BoardWindow::setWhiteName(QString name)
{
    gameData->white_name = name;
    ui->whiteName->blockSignals(true);
    ui->whiteName->setText(name);
    updateCaption();
    ui->whiteName->blockSignals(false);
}

void BoardWindow::switchToEditMode()
{
    gameData->gameMode = modeEdit;
    setMode(modeEdit);
    ui->computerControlsWidget->setVisible(gameData->gameMode == modeLocal);
    myColorIsBlack = true;
    myColorIsWhite = true;
}

void BoardWindow::switchToLocalMode()
{
    gameData->gameMode = modeLocal;
    setMode(modeLocal);
    ui->computerControlsWidget->setVisible(gameData->gameMode == modeLocal);
    myColorIsBlack = !(ui->computerBlack->isChecked());
    myColorIsWhite = !(ui->computerWhite->isChecked());

    // dynamic_cast downcasts the pointer and returns null if this is not safe
    // This requires Run-Time Type Information to be enabled at compile time
    qGoBoardLocalInterface* qgoboard_temp = dynamic_cast<qGoBoardLocalInterface*>(qgoboard);
    if (qgoboard_temp != NULL)
    {
        tree->setCurrent(tree->findLastMoveInCurrentBranch());
        qgoboard_temp->feedPositionThroughGtp();
    }
}

/*
 * The text in the comment zone has been changed
 */
void BoardWindow::slotUpdateComment()
{
    tree->getCurrent()->setComment(ui->commentEdit->toPlainText());
}

void BoardWindow::updateButtons(StoneColor lastMoveColor)
{
    switch (getGameMode())
    {
        case modeMatch:
        case modeLocal:
            if(getGamePhase() == phaseOngoing)
            {
                bool onMove = (lastMoveColor == stoneBlack && getMyColorIsWhite()) ||
                        (lastMoveColor == stoneWhite && getMyColorIsBlack());
                ui->passButton->setEnabled(onMove);
                ui->resignButton->setEnabled(onMove);
            }
            break;
        default:
            break;
    }
}

void BoardWindow::updateCursor(StoneColor currentMoveColor)
{
    CursorType cur = cursorIdle;

    switch (getGameMode())
    {
    case modeEdit :
    case modeTeach :
    case modeUndefined:
        if (getGamePhase() == phaseScore || getGamePhase() == phaseEdit )
            cur = cursorIdle;
        else
            cur = (currentMoveColor == stoneBlack ? cursorGhostWhite : cursorGhostBlack);
        break;
    case modeReview:
        break;
    case modeObserve :
        break;
    case modeMatch :
        if(getGamePhase() == phaseOngoing)
        {
            if  (currentMoveColor == stoneBlack )
                cur =  ( getMyColorIsWhite() ? cursorGhostWhite : cursorIdle );
            else
                cur = ( getMyColorIsBlack() ? cursorGhostBlack : cursorIdle );
        }
        //else	//FIXME
        break;
    case modeLocal :
        if(getGamePhase() == phaseOngoing)
        {
            if  (currentMoveColor == stoneBlack )
                cur = ( getMyColorIsWhite() ? cursorGhostWhite : cursorWait );
            else
                cur = ( getMyColorIsBlack() ? cursorGhostBlack : cursorWait );
        }
        break;
    }

    ui->board->setCursorType(cur);
}

/*
 * This deals with updating the 'board' and the 'interface handler' with the data
 * stored in the tree at the move place.
 * This involves calling and update for the board stones, and
 * an update of the interface
 */
void BoardWindow::updateMove(Move *m)
{
    if (m == NULL)
    {
        m = tree->getCurrent();
    }

    // Update slider branch length
    setSliderMax(m->getMoveNumber() + tree->getBranchLength());
    setMoveData(
            m->getMoveNumber(),
            (m->getColor() != stoneBlack),
            m->getNumBrothers(),
            m->getNumSons(),
            m->hasParent(),
            m->hasPrevBrother(),
            m->hasNextBrother(),
            m->getX(),
            m->getY());

    // Update comment if normal game (if observe or match, the comment zone is kept as is)
    if (getGameMode() == modeEdit)
        ui->commentEdit->setPlainText(m->getComment());

    // Isn't updateAll() sufficient? FIXME
    ui->board->removeGhosts();
    ui->board->updateAll(m);
    ui->board->updateLastMove(m);

    updateButtons(m->getColor());
    updateCursor(m->getColor());

    // Update the ghosts indicating variations
    if (m->getNumBrothers())// && setting->readIntEntry("VAR_GHOSTS")) TODO
        ui->board->updateVariationGhosts(m);

    // Oops, something serious went wrong
    if (m->getMatrix() == NULL)
        qFatal("   *** Move returns NULL pointer for matrix! ***");

    ui->capturesBlack->setText(QString::number(m->getCapturesBlack()));
    ui->capturesWhite->setText(QString::number(m->getCapturesWhite()));

    // Display times
    if(getGameMode() == modeEdit)
    {
        if(m->getMoveNumber() == 0)
        {
            if (gameData->timelimit == 0)
                getClockDisplay()->setTimeInfo(0, -1, 0, -1);
            else
                getClockDisplay()->setTimeInfo(gameData->timelimit, -1, gameData->timelimit, -1);
        }
        else if(m->getTimeinfo())
        {
            int other_time, other_stones_periods;
            if(m->parent && m->parent->getMoveNumber() != 0 && m->parent->getTimeinfo())
            {
                other_time = (int)m->parent->getTimeLeft();
                other_stones_periods = (m->parent->getOpenMoves() == 0 ? -1 : m->parent->getOpenMoves());
            }
            else
            {
                other_time = 0;
                other_stones_periods = -1;
            }
            if(!qgoboard->getBlackTurn())
                getClockDisplay()->setTimeInfo((int)m->getTimeLeft(), m->getOpenMoves() == 0 ? -1 : m->getOpenMoves(), other_time, other_stones_periods);
            else
                getClockDisplay()->setTimeInfo(other_time, other_stones_periods, (int)m->getTimeLeft(), m->getOpenMoves() == 0 ? -1 : m->getOpenMoves());
        }
    }
}

void BoardWindow::slotSendComment()
{
    QString ourcomment = ui->commentEdit2->text();
    ui->commentEdit2->clear();

    if (dispatch)
    {
        dispatch->sendKibitz(ourcomment);
        // User name only prepended for local display
        ourcomment.prepend(dispatch->getUsername() + "[" + dispatch->getOurRank() + "]: ");
    }
    ourcomment.prepend( "(" + QString::number(tree->getCurrent()->getMoveNumber()) + ") ");

    displayComment(ourcomment);
}

/*
 * Called when the 'nav to' button is pressed
 */
void BoardWindow::slotNavIntersection()
{
    //should not happen
    if (getGameMode() != modeEdit &&
        getGameMode() != modeObserve)
        return;
    /* Double check the above, seems like we should be able to use
     * the nav button in any mode except maybe the scorephase of
     * a game.. but I mean why can't we look back over moves whenever?
     * FIXME */

    setGamePhase ( phaseNavTo );
    ui->board->setCursorType(cursorNavTo);
}

void BoardWindow::slotWheelEvent(QWheelEvent *e)
{
    // leave if not editing
    if (getGameMode() != modeEdit &&
        getGameMode() != modeObserve)	//or observing
        return;

    if (getGamePhase() != phaseOngoing)
        return;

    // Check delay
    if (QTime::currentTime() < wheelTime)
        return;

    // Needs an extra check on variable mouseState as state() does not work on Windows.
    if (e->delta() > 0)
    {
        if (e->buttons() == Qt::RightButton || e-> modifiers() ==  Qt::ShiftModifier)
            tree->slotNavNextVar();
        else
            tree->slotNavForward();
    }
    else
    {
        if (e->buttons() == Qt::RightButton || e-> modifiers() ==  Qt::ShiftModifier)//|| mouseState == RightButton)
            tree->slotNavPrevVar();
        else
            tree->slotNavBackward();
    }

    // Delay of 100 msecs to avoid too fast scrolling
    wheelTime = QTime::currentTime();
    wheelTime = wheelTime.addMSecs(50);

    e->accept();
}

void BoardWindow::slotGetScore(int terrBlack, int captBlack, int deadWhite, int terrWhite, int captWhite, int deadBlack)
{
    ui->komiScore->setText(QString::number(gameData->komi));
    ui->terrWhite->setText(QString::number(terrWhite));
    ui->capturesWhiteScore->setText(QString::number(captWhite+deadBlack));
    ui->totalWhite->setText(QString::number((float)(terrWhite+captWhite+deadBlack) + gameData->komi));
    ui->terrBlack->setText(QString::number(terrBlack));
    ui->capturesBlackScore->setText(QString::number(captBlack+deadWhite));
    ui->totalBlack->setText(QString::number(terrBlack+captBlack+deadWhite));

    gameData->black_prisoners = captBlack;
    gameData->white_prisoners = captWhite;
}

/*
 * displays the informations relative to a game on the board window
 */
void BoardWindow::updateCaption()
{
    // Print caption
    // example: qGo 0.0.5 - Zotan 8k vs. tgmouse 10k
    // or if game name is given: qGo 0.0.5 - Kogo's Joseki Dictionary
    setWindowTitle( /* QString(isModified ? "* " : "") + */
        ( (gameData->number != 0 && gameData->number < 10000) ?
            "(" + QString::number(gameData->number) + ") " :
            QString()) + (gameData->gameName.isEmpty() ?
                gameData->white_name
                + (!gameData->white_rank.isEmpty() ?
                    " " + gameData->white_rank :
                    QString())
                + " " + QObject::tr("vs.") + " " + gameData->black_name
                + (!gameData->black_rank.isEmpty() ?
                    " " + gameData->black_rank :
                    QString()) :
                gameData->gameName) +	" - " + QString(PACKAGE));


    bool simple = gameData->white_rank.length() == 0 && gameData->black_rank.length() == 0;
    /* This name stuff is super redundant with below FIXME */
    QString player = gameData->white_name;
    if (simple && player == QObject::tr("White"))
    {
        ui->whiteName->setText(QObject::tr("White"));
        ui->whiteFrame_score_label->setText(QObject::tr("White"));
    }
    else if(!gameData->nigiriToBeSettled)
    {
        // truncate to 13 characters max
        player.truncate(13);

        if (gameData->white_rank.length() != 0)
            player = player + " " + gameData->white_rank;

        ui->whiteName->setText(player);
        ui->whiteFrame_score_label->setText(QObject::tr("W") + ": " + player);
    }

    player = gameData->black_name;
    if (simple && player == QObject::tr("Black"))
    {
        ui->blackName->setText(QObject::tr("Black"));
        ui->blackFrame_score_label->setText(QObject::tr("Black"));
    }
    else if(!gameData->nigiriToBeSettled)
    {
        // truncate to 13 characters max
        player.truncate(13);

        if (gameData->black_rank.length() != 0)
            player = player + " " + gameData->black_rank;

        ui->blackName->setText(player);
        ui->blackFrame_score_label->setText(QObject::tr("B") + ": " + player);
    }

    //TODO set  clock

    if(gameData->free_rated == RATED)
        freeratedLabel->setText("Rated");
    else if(gameData->free_rated == FREE)
        freeratedLabel->setText("Free");
    else if(gameData->free_rated == TEACHING)
        freeratedLabel->setText("Teaching");

    komiLabel->setText(QString("Komi: ").append(QString().setNum(gameData->komi)));
    handicapLabel->setText(QString("H: ").append(QString().setNum(gameData->handicap)));
}

/*
 * This updates the UI with the correct layout depending on the game mode
 */
void BoardWindow::setMode(GameMode mode)
{
    if ((mode == modeEdit) || (mode == modeLocal))
    {
        ui->commentEdit2->setDisabled(true);
    } else {
        ui->buttonModeEdit->hide();
        ui->buttonModeLocal->hide();
    }


    switch (mode)
    {

    case modeEdit:
        ui->actionPlay->setEnabled(true);
        ui->tabTools->setCurrentIndex(0) ;//setVisible(false);
        ui->scoreButton->setEnabled(true);
        ui->passButton_2->setEnabled(true);
        ui->commentEdit->setReadOnly(false);
        ui->commentEdit2->setDisabled(true);
        return;

    case modeObserve:
        ui->actionPlay->setDisabled(true);
        ui->tabTools->setVisible(false) ;
        ui->passButton_2->setDisabled(true);
        ui->commentEdit->setReadOnly(true);
        ui->commentEdit2->setReadOnly(false);
        ui->commentEdit2->setEnabled(true);
        return ;

    case modeMatch :
        ui->actionPlay->setDisabled(true);
        ui->tabTools->setCurrentIndex(1) ;
        ui->scoreButton->setDisabled(true);
        ui->passButton->setEnabled(true);
        ui->undoButton->setEnabled(true);
        ui->resignButton->setEnabled(true);
        ui->reviewButton->setDisabled(true);
        ui->reviewButton->setVisible(true);
        ui->adjournButton->setEnabled(true);
        /* FIXME we could have the refreshButton refresh the observers on
         * IGS.  This requires a "supports" protocol function, etc., to
         * send the refresh.  Or we could just periodically refresh the
         * observers.
         * But note that its currently not "connect"ed to anything. */
        //ui->refreshButton->setEnabled(true);
        ui->refreshButton->setEnabled(false);

        ui->doneButton->setEnabled(false);
        ui->commentEdit->setReadOnly(true);
        ui->commentEdit2->setEnabled(true);
        ui->commentEdit2->setReadOnly(false);
        ui->navButtonsFrame->setEnabled(false);
        return ;

    case   modeLocal :
        ui->actionPlay->setDisabled(true);
        ui->tabTools->setCurrentIndex(1) ;
        ui->scoreButton->setDisabled(true);
        ui->passButton->setEnabled(true);
        ui->undoButton->setEnabled(true);
        ui->resignButton->setEnabled(true);
        ui->adjournButton->setEnabled(false);
        ui->doneButton->setEnabled(false);
        ui->commentEdit->setReadOnly(true);
        ui->navButtonsFrame->setEnabled(false);
        ui->commentEdit2->setDisabled(true);
        ui->reviewButton->setDisabled(true);
        return ;

    case modeTeach:
        ui->actionPlay->setDisabled(true);
        ui->tabTools->setCurrentIndex(1) ;
        ui->scoreButton->setDisabled(true);
        ui->passButton->setEnabled(true);
        ui->undoButton->setEnabled(true);
        ui->resignButton->setEnabled(true);
        ui->adjournButton->setEnabled(true);
        ui->doneButton->setEnabled(false);
        ui->commentEdit->setReadOnly(true);
        ui->navButtonsFrame->setEnabled(false);
        ui->commentEdit2->setReadOnly(false);
        ui->commentEdit2->setEnabled(true);
        return ;

    case modeReview :
        ui->actionPlay->setDisabled(true);
        ui->tabTools->setCurrentIndex(1) ;
        ui->scoreButton->setDisabled(true);
        ui->passButton->setEnabled(true);
        ui->undoButton->setEnabled(true);
        ui->resignButton->setEnabled(true);
        ui->adjournButton->setEnabled(true);
        ui->doneButton->setEnabled(false);
        ui->commentEdit->setReadOnly(true);
        ui->commentEdit2->setReadOnly(false);
        ui->commentEdit2->setEnabled(true);
        return ;
    case modeUndefined:
        return;
    }
}

/*
 * Resets all displays on the board window
 */
void BoardWindow::clearData()
{
    ui->board->clearData();

    ui->commentEdit->clear();

    ui->capturesBlack->setText("0");
    ui->capturesWhite->setText("0");

    ui->pb_timeBlack->setText("00:00");
    ui->pb_timeWhite->setText("00:00");

    ui->scoreButton->setDown(false);

    ui->slider->setValue(0);
    ui->slider->setMaximum(0);
}

/*
 * displays the informations relative to a move on the board window
 */
void BoardWindow::setMoveData(int n, bool black, int brothers, int sons, bool hasParent, bool hasPrev, bool hasNext, int lastX, int lastY)
{
    // move number
    QString s(QObject::tr("Move") + " ");
    s.append(QString::number(n));

    // color and coordinates
    if (lastX >= 1 && lastX <= getBoardSize() && lastY >= 1 && lastY <= getBoardSize())
    {
        s.append(" (");
        s.append(black ? QObject::tr("W")+" " : QObject::tr("B")+" ");
        s.append(QString(QChar(static_cast<const char>('A' + (lastX<9?lastX:lastX+1) - 1))) +
            QString::number(getBoardSize()-lastY+1) + ")");
    }

    //pass move
    else if (lastX == PASS_XY && lastY == PASS_XY)
    {
        s.append(" (");
        s.append(black ? QObject::tr("W")+" " : QObject::tr("B")+" ");
        s.append(" " + QObject::tr("Pass") + ")");
    }

    moveNumLabel->setText(s);

    // set turn information (and color on the edit button)
    s = black ? QObject::tr("Black to play") : QObject::tr("White to play");
    ui->turnLabel->setText(s);

    ui->colorButton->setIcon(black ? QIcon(":/boardicons/resources/pics/stone_black.png") : QIcon(":/boardicons/resources/pics/stone_white.png") );

    // sons and variatons display
    s = "";
    s.append(QString::number(brothers));
    if (brothers == 1)
        s.append(" " + QObject::tr("brother") + "\n");
    else
        s.append(" " + QObject::tr("brothers") + "\n");

    s.append(QString::number(sons));
    if (sons == 1)
        s.append(" " + QObject::tr("son"));
    else
        s.append(" " + QObject::tr("sons"));
    ui->varLabel->setText(s);

    if(getGameMode() == modeReview)
    {
        /* For now, just disable navigation if its in the review mode, they can always duplicate
         * the board and a lot more is necessary for qgo to do reviews. */
        ui->navPrevVar->setEnabled(false);
        ui->navNextVar->setEnabled(false);
        ui->navBackward->setEnabled(false);
        ui->navForward->setEnabled(false);
        ui->navFirst->setEnabled(false);
        ui->navStartVar->setEnabled(false);
        ui->navMainBranch->setEnabled(false);
        ui->navLast->setEnabled(false);
        ui->navNextBranch->setEnabled(false);
        ui->swapVarButton->setEnabled(false);
        ui->navPrevComment->setEnabled(false);
        ui->navNextComment->setEnabled(false);
        ui->navIntersection->setEnabled(false);

        ui->slider->setEnabled(false);
    }
    else if (getGameMode() == modeEdit || getGameMode() == modeObserve )//|| board->getGameMode() == modeEdit)
    {
        // Update the toolbar buttons
        ui->navPrevVar->setEnabled(hasPrev);
        ui->navNextVar->setEnabled(hasNext);
        ui->navBackward->setEnabled(hasParent);
        ui->navForward->setEnabled(sons);
        ui->navFirst->setEnabled(hasParent);
        ui->navStartVar->setEnabled(hasParent);
        ui->navMainBranch->setEnabled(hasParent);
        ui->navLast->setEnabled(sons);
        ui->navNextBranch->setEnabled(sons);
        ui->swapVarButton->setEnabled(hasPrev);
        ui->navPrevComment->setEnabled(hasParent);
        ui->navNextComment->setEnabled(sons);
        ui->navIntersection->setEnabled(true);

        ui->slider->setEnabled(true);
    }

    // Update slider
    ui->slider->blockSignals(true);
    if (ui->slider->maximum() < n)
          setSliderMax(n);
    ui->slider->setValue(n);
    ui->slider->blockSignals(false);
}

void BoardWindow::setUndoEnabled(bool state)
{
    ui->undoButton->setEnabled(state);
}

void BoardWindow::setDrawEnabled(bool state)
{
    ui->drawButton->setEnabled(state);
}

void BoardWindow::setCountEnabled(bool state)
{
    ui->countButton->setEnabled(state);
}

void BoardWindow::setDoneEnabled(bool state)
{
    ui->doneButton->setEnabled(state);
}

void BoardWindow::setAdjournEnabled(bool state)
{
    ui->adjournButton->setEnabled(state);
}

void BoardWindow::setResignEnabled(bool state)
{
    ui->resignButton->setEnabled(state);
}

void BoardWindow::setObserverModel(QAbstractItemModel *model)
{
    ui->observerView->setModel(model);
}

void BoardWindow::setTimeBlack(QString time)
{
    ui->pb_timeBlack->setText(time);
}

void BoardWindow::setTimeWhite(QString time)
{
    ui->pb_timeWhite->setText(time);
}

void BoardWindow::warnTimeBlack(TimeWarnState state)
{
    switch (state)
    {
    case TimeOK:
        if(ui->pb_timeBlack->palette().color(QPalette::Background) != Qt::black)
        {
            ui->pb_timeBlack->setPalette(QPalette(Qt::black));
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    /* Otherwise windows XP/Mac style makes time buttons ugly white on white.
     * This could interfere with blinking warning. */
    ui->pb_timeBlack->setStyleSheet("background-color: black; color: white");
#endif //Q_OS_WIN
        }
        return;
    case TimeLow:
        if(ui->pb_timeBlack->palette().color(QPalette::Background) == Qt::black)
            ui->pb_timeBlack->setPalette(QPalette(Qt::yellow));
        else
            ui->pb_timeBlack->setPalette(QPalette(Qt::black));
        return;
    case TimeExpired:
        ui->pb_timeBlack->setPalette(QPalette(Qt::red));
    }
}

void BoardWindow::warnTimeWhite(TimeWarnState state)
{
    switch (state)
    {
    case TimeOK:
        if(ui->pb_timeWhite->palette().color(QPalette::Background) != Qt::black)
        {
            ui->pb_timeWhite->setPalette(QPalette(Qt::black));
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    /* Otherwise windows XP/Mac style makes time buttons ugly white on white.
     * This could interfere with blinking warning. */
    ui->pb_timeWhite->setStyleSheet("background-color: black; color: white");
#endif //Q_OS_WIN
        }
        return;
    case TimeLow:
        if(ui->pb_timeWhite->palette().color(QPalette::Background) == Qt::black)
            ui->pb_timeWhite->setPalette(QPalette(Qt::yellow));
        else
            ui->pb_timeWhite->setPalette(QPalette(Qt::black));
        return;
    case TimeExpired:
        ui->pb_timeWhite->setPalette(QPalette(Qt::red));
    }
}

void BoardWindow::displayComment(QString comment)
{
    // add comments to SGF/tree FIXME
    QString txt = tree->getCurrent()->getComment();
    if(getGamePhase() != phaseEnded)
        txt.append(comment);
    tree->getCurrent()->setComment(txt);

    ui->commentEdit->append(comment);
    /* Should probably update the history in a central location
     * and update display automatically,
     * but this is not implemented yet */
}

/*
 * modifies the maximum value of the slider (used when a move is added)
 */
void BoardWindow::setSliderMax(int n)
{
    if (n < 0)
        n = 0;

    if (n == ui->slider->maximum())
        return;

    ui->slider->blockSignals (true);
    ui->slider->setMaximum(n);
    ui->slider->blockSignals (false);
    ui->sliderRightLabel->setText(QString::number(n));
}
