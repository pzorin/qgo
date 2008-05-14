/***************************************************************************
*
* This class is the local game proxy
* It deals only with modifying a local game (like an sgf file)
* 
***************************************************************************/

#include "qgoboard.h"
#include "boardwindow.h"
#include "tree.h"
#include "move.h"
#include "globals.h"
#include "audio.h"

qGoBoard::qGoBoard(BoardWindow *bw, Tree * t, GameData *gd) : QObject(bw)
{
	isModified = FALSE;	
	tree = t;
	boardwindow = bw;
	
	//data used for observing games, when getting the already played moves
	stated_mv_count = -1;	

	if (gd)
		gameData = new GameData (gd);//FIXME This is not safe to duplicate the gameData. Should stay single in boardHandler
	else
		gameData = NULL;

//TODO get the sound from correct path
	clickSound = SoundFactory::newSound( "/usr/share/qgo2/sounds/stone.wav" );
//	clickSound = SoundFactory::newSound( "/home/eb/Packages/qgo.new/src/sounds/enter.wav" );
}


void qGoBoard::setHandicap(int handicap)
{
	GamePhase store = boardwindow->getGamePhase();
	//qDebug("set Handicap " + QString::number(handicap) + ", stored mode == " + QString::number(store));
	boardwindow->setGamePhase(phaseEdit);

	int size = gameData->size;
	int edge_dist = (size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (size + 1) / 2;
	int high = size + 1 - edge_dist;
	
	if (size > 25 || size < 7)
	{
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
//		setMode(store);
		return;
	}
	
	// change: handicap is first move
//	if (handicap > 1)
//	{
		tree->createMoveSGF();
		//createNode(*tree->getCurrent()->getMatrix(), false, false);
		/* Move should already be set to 0 by board handler and the placement
		 * of handicap stones won't change that */
		//currentMove++;
		tree->getCurrent()->setMoveNumber(0);
		tree->getCurrent()->setHandicapMove(TRUE);
//	}

	// extra:
	if (size == 19 && handicap > 9)
		switch (handicap)
		{
		case 13:  // Hehe, this is nuts... :)
			tree->addStoneSGF(stoneBlack, 17, 17,FALSE);
		case 12:
			tree->addStoneSGF(stoneBlack, 3, 3,FALSE);
		case 11:
			tree->addStoneSGF(stoneBlack, 3, 17,FALSE);
		case 10:
			tree->addStoneSGF(stoneBlack, 17, 3,FALSE);
			
		default:
			handicap = 9;
			break;
		}
	
	switch (size % 2)
	{
	// odd board size
	case 1:
		switch (handicap)
		{
		case 9:
			tree->addStoneSGF(stoneBlack, middle, middle,FALSE);
		case 8:
		case 7:
			if (handicap >= 8)
			{
				tree->addStoneSGF(stoneBlack, middle, low,FALSE);
				tree->addStoneSGF(stoneBlack, middle, high,FALSE);
			}
			else
				tree->addStoneSGF(stoneBlack, middle, middle,FALSE);
		case 6:
		case 5:
			if (handicap >= 6)
			{
				tree->addStoneSGF(stoneBlack, low, middle,FALSE);
				tree->addStoneSGF(stoneBlack, high, middle,FALSE);
			}
			else
				tree->addStoneSGF(stoneBlack, middle, middle,FALSE);
		case 4:
			tree->addStoneSGF(stoneBlack, high, high,FALSE);
		case 3:
			tree->addStoneSGF(stoneBlack, low, low,FALSE);
		case 2:
			tree->addStoneSGF(stoneBlack, high, low,FALSE);
			tree->addStoneSGF(stoneBlack, low, high,FALSE);
		case 1:
//			if (store != modeObserve && store != modeMatch &&  store != modeTeach)
//				gameData->komi = 0.5;
			break;
			
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;
		
	// even board size
	case 0:
		switch (handicap)
		{
		case 4:
			tree->addStoneSGF(stoneBlack, high, high,FALSE);
		case 3:
			tree->addStoneSGF(stoneBlack, low, low,FALSE);
		case 2:
			tree->addStoneSGF(stoneBlack, high, low,FALSE);
			tree->addStoneSGF(stoneBlack, low, high,FALSE);
		case 1:
//			if (store != modeObserve && store != modeMatch &&  store != modeTeach)
//				gameData->komi = 0.5;
			break;
			
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;
		
	default:
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
				
	}
	
	// Change cursor stone color
	//board->setCurStoneColor();
	//gameData->handicap = handicap;
	boardwindow->setGamePhase(store);
	//board->getInterfaceHandler()->disableToolbarButtons();
}

/*
 * adds a mark to the current move's matrix
 * This is used in edit phase to add mark on the board
 */
void qGoBoard::addMark( int x, int y, MarkType t )
{
	if (tree->getCurrent()->getMatrix()->getMarkAt(x,y) != markNone)
		tree->getCurrent()->getMatrix()->removeMark(x,y);

	if (t == markText || t == markNumber)
	{
		QString txt = tree->getCurrent()->getMatrix()->getFirstTextAvailable(t);
		tree->getCurrent()->getMatrix()->setMarkText(x,y,txt);
	}

	tree->getCurrent()->getMatrix()->insertMark(x,y,t);

	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}

/*
 * removes a mark to the current move's matrix
 * This is used in edit phase to remove a mark on the board
 */
void qGoBoard::removeMark( int x, int y)
{
	if (tree->getCurrent()->getMatrix()->getMarkAt(x,y) != markNone)
		tree->getCurrent()->getMatrix()->removeMark(x,y);
		
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}

/*
 * adds a stone to the current move's matrix
 * This is used in edit phase to add stones (and not moves) on the board
 */
void qGoBoard::addStone(StoneColor c, int x, int y)
{
	tree->addStoneSGF(c,x,y,FALSE);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}


/*
 * removes a stone to the current move's matrix
 * This is used in edit phase to remove stones (and not moves) on the board
 */
void qGoBoard::removeStone( int x, int y)
{
	//TODO make sure this is the correct way
	tree->updateCurrentMatrix(stoneNone,  x,  y);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}


/*
 * 'Pass' button pressed
 */
void qGoBoard::slotPassPressed()
//was slot_doPass()
{
	localPassRequest();
}

/*
 * 'Score' button toggled
 */
void qGoBoard::slotScoreToggled(bool pressed)
//was slot_doPass()
{
	if (pressed)
		enterScoreMode();
	else 
		leaveScoreMode();
}

/*
 * This handles the main envent with qGo : something has been clicked on the board
 */
void qGoBoard::slotBoardClicked(bool , int x, int y , Qt::MouseButton mouseState)
{
	bool blackToPlay = getBlackTurn();

	switch (boardwindow->getGamePhase())
	{
		case phaseInit:
			//should not happen
			return ;
		
		case phaseNavTo:
		{
			boardwindow->getBoardHandler()->findMoveByPos(x, y); 
			return;
		}

		case phaseEdit:
		{
			switch (boardwindow->getEditMark())
			{
				//adds a stone (or remove if present)
				case markNone:
				{
					if(tree->getCurrent()->getMatrix()->getStoneAt(x,y) == stoneNone)
						addStone(mouseState == Qt::LeftButton ? stoneBlack : stoneWhite, x, y);
					else
						removeStone(x,y);
					return;
				}
				default:
				{
					if (mouseState == Qt::LeftButton)
						addMark(x,y, boardwindow->getEditMark());
					else
						removeMark(x,y);
					return;
				}
					
			}
			return;
		}

		case phaseEnded:
			return;

		case phaseScore:
		{
//			markDeadStone(x,y);
			localMarkDeadRequest(x,y);
			return;
		}

		case phaseOngoing:
			if (blackToPlay && boardwindow->getMyColorIsBlack())
				localMoveRequest(stoneBlack,x,y);

			if (!blackToPlay && boardwindow->getMyColorIsWhite())
				localMoveRequest(stoneWhite,x,y);
	}
}


/*
 * This processes a 'Pass' button pressed
 */
void qGoBoard::localPassRequest()
//was slot_doPass()
{
	StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );

	doPass();
	sendPassToInterface(c);
}


/*
 * This functions gets the move request (from a board click)
 * and displays the resulting stone (if valid)
 */
void qGoBoard::localMoveRequest(StoneColor c, int x, int y)
{
	if (doMove(c,x,y))
	{
		boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
		sendMoveToInterface(c,x,y);
	}
	
}


/*
 * This functions gets the request (from a board click)
 * to mark a stone as dead/undead (score mode). All local but 'match'
 */
void qGoBoard::localMarkDeadRequest(int x, int y)
{
	markDeadStone(x,y);
}


/*
 * This function adds a pass move to a game. there is no need to return anything
 */
void qGoBoard::doPass()

{
//	StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );

	tree->doPass(FALSE);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

}




/*
 * This functions adds a move to a game. returns 1 if move was valid, 0 if not)
 */
bool qGoBoard::doMove(StoneColor c, int x, int y)
{
	bool validMove = TRUE;

	// does the matrix have already a stone there ?
	if (tree->getCurrent()->getMatrix()->getStoneAt(x,y) != stoneNone)
	{
		qDebug ("QGoboard:doMove - We seem to have already a stone at this place : %d %d",x,y);
		return FALSE;
	}

	//The move is added to the tree. if it exists already, it becomes the current move
	tree->addMove(c,  x, y, TRUE);

	// Is the move valid ?
	if ( tree->addStoneSGF(c,x,y,TRUE) < 0)
	{
		qDebug ("QGoboard:doMove - This move does not seem to be valid : %d %d",x,y);
		tree->deleteNode(); 
		validMove = FALSE;
	}
	else
		if (tree->getCurrent()->getMoveNumber() > stated_mv_count)
//		{
//			qDebug("playing sound");
			if (playSound)
				clickSound->play();
//		}
//	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
	
	return validMove;
}



/*
 * Returns true wether it's black to play
 */
bool qGoBoard::getBlackTurn()
{
	if (tree->getCurrent()->getPLinfo())
		// color of next stone is same as current
		return tree->getCurrent()->getPLnextMove() == stoneBlack;

	if (tree->getCurrent() == tree->getRoot())
		return TRUE;

	// the first handicap move bears number 0 as well
	if (tree->getCurrent()->getMoveNumber() == 0)
	{
		// Handicap, so white starts
		if (gameData->handicap >= 2)
			return FALSE;
			
		return TRUE;
	}
	
	// Normal mode
/*	if (tree->getCurrent()->getGameMode() == modeNormal ||
		tree->getCurrent()->getGameMode() == modeObserve ||
		tree->getCurrent()->getGameMode() == modeMatch ||
		tree->getCurrent()->getGameMode() == modeTeach ||
    		tree->getCurrent()->getGameMode() == modeComputer)
*/
	if (boardwindow->getGamePhase() != phaseEdit)
	{
		// change color
		return (tree->getCurrent()->getColor() == stoneWhite);
	}
	// Edit mode. Return color of parent move.
	else if (tree->getCurrent()->parent != NULL)
		return tree->getCurrent()->parent->getColor() == stoneWhite;

	// Crap happened. 50% chance this is correct .)
	qWarning("Oops, crap happened in BoardHandler::getBlackTurn() !");
	return TRUE;
}

 

/*
 * This functions initialises the scoring mode
 */
void qGoBoard::enterScoreMode()
{
	boardwindow->setGamePhase ( phaseScore );
	boardwindow->getUi().tabDisplay->setCurrentIndex(1);
	boardwindow->getBoardHandler()->updateCursor();
	boardwindow->getBoardHandler()->countScore();
}


/*
 * This functions is called in scoring phase when clicking on a dead group
 */
void qGoBoard::markDeadStone(int x, int y)
{
	// is the click on a stone ?
	if ( tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneNone )
		return ;

	tree->getCurrent()->getMatrix()->toggleGroupAt(x, y);
	boardwindow->getBoardHandler()->countScore();
}

/*
 * The text in the comment zone has been changed
 */
void qGoBoard::slotUpdateComment()
{
	tree->getCurrent()->setComment(boardwindow->getUi().commentEdit->toPlainText());
}


/*
 * kibitz was received. Tranfer it to move and comment window
 */
void qGoBoard::kibitzReceived(const QString& text)
{

	QString k = text;
	k.prepend( "(" + QString::number(tree->getCurrent()->getMoveNumber()) + ") ");
	
	QString txt = tree->getCurrent()->getComment();

	if (!txt.isEmpty())
		txt.append('\n');

	txt.append(k);
	tree->getCurrent()->setComment(txt);

//	if (!boardwindow->getUi().commentEdit->toPlainText().isEmpty())
//		boardwindow->getUi().commentEdit->append("\n");

	
	boardwindow->getUi().commentEdit->append(k);

}

// send regular time Info
void qGoBoard::timerEvent(QTimerEvent*)
{
	// wait until first move
//	if (mv_counter < 0 || id < 0 || game_paused)
	if (boardwindow->getGamePhase() != phaseOngoing)
		return;
/*
	if (getBlackTurn())
	{
		// B's turn
//		bt_i--;

		win->getInterfaceHandler()->setTimes(secToTime(bt_i), b_stones, wt, w_stones);

	}
	else	
	{
		// W's turn
//		wt_i--;

		win->getInterfaceHandler()->setTimes(bt, b_stones, secToTime(wt_i), w_stones);

	}
*/
	boardwindow->getClockDisplay()->setTimeStep(getBlackTurn());
/*
	// warn if I am within the last 10 seconds
	if (gameMode == modeMatch)
	{
		if (myColorIsBlack && bt_i <= BY_timer && bt_i > -1 && mv_counter % 2) // ||
		{
			// each second we alternate button background color
			if (bt_i %2)
				win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(Qt::red);
			else 
				win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;//setPaletteBackgroundColor(Qt::PaletteBase);
			qgo->playTimeSound();
		}		
	
		else if ( !myColorIsBlack && wt_i <= BY_timer && wt_i > -1 && (mv_counter % 2) == 0)
		{
			if (wt_i %2)
				win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(Qt::red);
			else 
				win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;//setPaletteBackgroundColor(Qt::PaletteBase);
			qgo->playTimeSound();
		}
		
		//we have to reset the color when not anymore in warning period)
		else if (win->getInterfaceHandler()->normalTools->pb_timeBlack->paletteBackgroundColor() == Qt::red)
			win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;

		
		else if (win->getInterfaceHandler()->normalTools->pb_timeWhite->paletteBackgroundColor() == Qt::red)
			win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;

		

	}
*/
}

/*
 * Deletes the current move, and all the following
 */
void qGoBoard::deleteNode()
{
//	CHECK_PTR(tree);
	
	Move 	*m = tree->getCurrent(),
		*remember = NULL,
		*remSon = NULL;
	Q_CHECK_PTR(m);
	
	if (m->parent != NULL)
	{
		remember = m->parent;
		
		// Remember son of parent if its not the move to be deleted.
		// Then check for the brothers and fix the pointer connections, if we
		// delete a node with brothers. (It gets ugly now...)
		// YUCK! I hope this works.
		if (remember->son == m)                  // This son is our move to be deleted?
		{
			if (remember->son->brother != NULL)  // This son has a brother?
				remSon = remember->son->brother; // Reset pointer
		}
		else                                     // No, the son is not our move
		{
			remSon = remember->son;
			Move *tmp = remSon, *oldTmp = tmp;
			
			do {   // Loop through all brothers until we find our move
				if (tmp == m)
				{
					if (m->brother != NULL)            // Our move has a brother?
						oldTmp->brother = m->brother;  // Then set the previous move brother
					else                               // to brother of our move
						oldTmp->brother = NULL;        // No brother found.
					break;
				}
				oldTmp = tmp;
			} while ((tmp = tmp->brother) != NULL);
		}
	}
	else if (tree->hasPrevBrother())
	{
		remember = tree->previousVariation();
		if (m->brother != NULL)
			remember->brother = m->brother;
		else
			remember->brother = NULL;
	}
	else if (tree->hasNextBrother())
	{
		remember = tree->nextVariation();
		// Urgs, remember is now root.
		tree->setRoot(remember);
	}
	else
	{
		// Oops, first and only move. We delete everything
		tree->init(boardwindow->getBoardSize());
//		board->hideAllStones();
//		board->hideAllMarks();
//		board->updateCanvas();
		boardwindow->getBoard()->clearData();

//		lastValidMove = NULL;
//		stoneHandler->clearData();
		boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
		return;
	}
	
	if (m->son != NULL)
		Tree::traverseClear(m->son);  // Traverse the tree after our move (to avoid brothers)
	delete m;                         // Delete our move
	tree->setCurrent(remember);       // Set current move to previous move
	remember->son = remSon;           // Reset son pointer
	remember->marker = NULL;          // Forget marker
	
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
	
//	board->setModified();
}



/***************************************************************************
 *
 * Normal Interface
 *
 ****************************************************************************/



qGoBoardNormalInterface::qGoBoardNormalInterface(BoardWindow *bw, Tree * t, GameData *gd) 
	: qGoBoard(bw, t, gd)
{
	boardwindow->getUi().board->clearData();

	// If we have handicap, but not from a loaded file, we have to set the handicap move
	if (gameData->handicap && gameData->fileName.isEmpty())
	{
		setHandicap(gameData->handicap);
		boardwindow->getBoardHandler()->slotNavLast();
	}
	else 
//	tree->setToFirstMove();	
		boardwindow->getBoardHandler()->slotNavFirst();

	QSettings settings;
	// value 1 = no sound, 0 all games, 2 my games
	playSound = (settings.value("SOUND") != 1);
}


/*
 * This functions leaves the scoring mode
 */
void qGoBoardNormalInterface::leaveScoreMode()
{
	boardwindow->getUi().tabDisplay->setCurrentIndex(0);
	boardwindow->setGamePhase ( phaseOngoing );
	boardwindow->getBoardHandler()->exitScore();
}

