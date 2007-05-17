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
	clickSound = SoundFactory::newSound( "/usr/share/qGo/sounds/stone.wav" );
}


void qGoBoard::setHandicap(int handicap)
{
//	GameMode store = boardwindow->getGameMode();
	//qDebug("set Handicap " + QString::number(handicap) + ", stored mode == " + QString::number(store));
//	setMode(modeEdit);

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
	//setMode(store);
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
void qGoBoard::slotBoardClicked(bool delay , int x, int y , Qt::MouseButton mouseState)
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
			markDeadStone(x,y);
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
		// FIXME : this should be made in a better way : wait for the interface to acknowledge before adding the move to the tree
		sendMoveToInterface(c,x,y);
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
			clickSound->play();

	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
	
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
 * A move string is incoming from the interface (computer or server)
 */
void qGoBoard::set_move(StoneColor sc, QString pt, QString mv_nr)
{
	int mv_nr_int;
	int mv_counter = tree->getCurrent()->getMoveNumber();
	bool hcp_move = tree->getCurrent()->isHandicapMove();
	// IGS: case undo with 'mark': no following command
	// -> from qgoIF::slot_undo(): set_move(stoneNone, 0, 0)
	if (mv_nr.isEmpty())
		// undo one move
		mv_nr_int = mv_counter - 1;
	else
		mv_nr_int = mv_nr.toInt();

	// We are observing a game, and we take the game in the middle
	if (mv_nr_int > mv_counter)
	{
		if (mv_nr_int != mv_counter + 1 && mv_counter != 0)
			// case: move has done but "moves" cmd not executed yet
			qWarning("**** LOST MOVE !!!! ****");
		// if the move number given by the server is over current, we skip 
		// (means the 'move' command moves have not yet come in)
		// exception : the fist move, if a handicap, has number 0 and next incoming IGS move has number 1 (instead of 0)
		else if (mv_counter == 0 && mv_nr_int != 0 && !hcp_move)
		{
			qDebug("move skipped");
			// skip moves until "moves" cmd executed
			return;
		}
		else
			mv_counter++;
	}
/*	else if (mv_nr_int + 1 == mv_counter)
	{
		// scoring mode? (NNGS)
		if (gameMode == modeScore)
		{
			// back to matchMode
			win->doRealScore(false);
			win->getBoard()->setMode(modeMatch);
		}

		// special case: undo handicap
		if (mv_counter <= 0 && gd.handicap)
		{
			gd.handicap = 0;
			win->getBoard()->getBoardHandler()->setHandicap(0);
			qDebug("set Handicap to 0");
		}

		if (!pt)
		{
			// case: undo
			qDebug("set_move(): UNDO in game " + QString::number(id));
			qDebug("...mv_nr = " + mv_nr);

			                                                                       //added eb 9
			Move *m=win->getBoard()->getBoardHandler()->getTree()->getCurrent();
			Move *last=win->getBoard()->getBoardHandler()->lastValidMove ; //win->getBoard()->getBoardHandler()->getTree()->findLastMoveInMainBranch();

			if (m!=last)                          // equivalent to display_incoming_move = false
				win->getBoard()->getBoardHandler()->getTree()->setCurrent(last) ;
			else
				m = m->parent ;                   //we are going to delete the node. We will bactrack to m

			win->getBoard()->deleteNode();
			win->getBoard()->getBoardHandler()->lastValidMove = win->getBoard()->getBoardHandler()->getTree()->getCurrent();
			win->getBoard()->getBoardHandler()->getTree()->getCurrent()->marker = NULL ; //(just in case)
			win->getBoard()->getBoardHandler()->getStoneHandler()->checkAllPositions();
                             
			win->getBoard()->getBoardHandler()->getTree()->setCurrent(m) ;   //we return where we were observing the game
			// this is not very nice, but it ensures things stay clean
			win->getBoard()->getBoardHandler()->updateMove(m);                      // end add eb 9

			mv_counter--;

			// ok for sound - no moves cmd in execution
			sound = true;
		}

		return;
	}
	else
		// move already done...
		return;
*/
	if (pt.contains("Handicap"))
	{
		QString handi = pt.simplified();
		int h = handi.section(" ",-1).toInt();//element(handi, 1, " ").toInt();
		
		setHandicap(h);
		// check if handicap is set with initGame() - game data from server do not
		// contain correct handicap in early stage, because handicap is first move!
		if ( boardwindow->getGameData()->handicap != h)
		{
			boardwindow->getGameData()->handicap = h;
			qDebug("corrected Handicap");
		}
	}

	else if (pt.contains("Pass",Qt::CaseInsensitive))
	{
//		win->getBoard()->doSinglePass();
//		if (win->getBoard()->getBoardHandler()->local_stone_sound)
//			qgo->playPassSound();
		doPass();
	}
	else
	{
/*		if ((gameMode == modeMatch) && (mv_counter < 2) && !(myColorIsBlack))
		{
			// if black has not already done - maybe too late here???
			if (requests_set)
			{
				qDebug(QString("qGoBoard::set_move() : check_requests at move %1").arg(mv_counter));
				check_requests();
			}
		}
*/
		int i = pt[0].unicode() - QChar::fromAscii('A').unicode() + 1;
		// skip j
		if (i > 8)
			i--;

		int j;

		if (pt[2] >= '0' && pt[2] <= '9')
			j = boardwindow->getGameData()->size + 1 - pt.mid(1,2).toInt();
		else
			j = boardwindow->getGameData()->size + 1 - pt[1].digitValue();

		// avoid sound problem during execution of "moves" cmd
/*		if (stated_mv_count > mv_counter)
			sound = false;

		else if (gameMode == modeComputer)
			sound = true;
*/
//		win->getBoard()->addStone(sc, i, j, sound);

		if (!doMove(sc, i, j))
			QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move %1 seems to be invalid").arg(pt.toLatin1().constData()));
//		qDebug ("QGoboard:setMove - move %d %d done",i,j);
/*
		Move *m = tree->getCurrent();
		if (stated_mv_count > mv_counter ||
			//gameMode == modeTeach ||
			//ExtendedTeachingGame ||
			wt_i < 0 || bt_i < 0)
		{
			m->setTimeinfo(false);
		}
		else
		{
			m->setTimeLeft(sc == stoneBlack ? bt_i : wt_i);
			int stones = -1;
			if (sc == stoneBlack)
				stones = b_stones.toInt();
			else
				stones = w_stones.toInt();
			m->setOpenMoves(stones);
			m->setTimeinfo(true);

			// check for a common error -> times and open moves identical
			if (m->parent &&
				m->parent->parent &&
				m->parent->parent->getTimeinfo() &&
				m->parent->parent->getTimeLeft() == m->getTimeLeft() &&
				m->parent->parent->getOpenMoves() == m->getOpenMoves())
			{
				m->parent->parent->setTimeinfo(false);
			}
		}
*/	
	}

}


/*
 * This functions initialises the scoring mode
 */
void qGoBoard::enterScoreMode()
{
	boardwindow->setGamePhase ( phaseScore );
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
	k.prepend( "(" + QString(tree->getCurrent()->getMoveNumber()) + ") ");
	
	QString txt = tree->getCurrent()->getComment();

	if (!txt.isEmpty())
		txt.append('\n');

	txt.append(k);
	tree->getCurrent()->setComment(txt);

	if (!boardwindow->getUi().commentEdit->toPlainText().isEmpty())
		boardwindow->getUi().commentEdit->append("\n");

	
	boardwindow->getUi().commentEdit->append(k);

}

/***************************************************************************
 *
 * Normal Interface
 *
 ****************************************************************************/



qGoBoardNormalInterface::qGoBoardNormalInterface(BoardWindow *bw, Tree * t, GameData *gd) 
	: qGoBoard(bw, t, gd)
{

}

/*
 * This functions initialises the interface for local game
 * It sets the handicap stones into the tree
 */
bool qGoBoardNormalInterface::init()
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

	return TRUE;
}


/*
 * This functions leaves the scoring mode
 */
void qGoBoardNormalInterface::leaveScoreMode()
{
	boardwindow->setGamePhase ( phaseOngoing );
	boardwindow->getBoardHandler()->exitScore();
}


