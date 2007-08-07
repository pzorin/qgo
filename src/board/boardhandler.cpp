/***************************************************************************
*
* This class is used to navigate into the 'tree' class
* It is the only class that gives orders to the 'board'and
* the 'interface handler', depending on what is found in 
* the tree
* On the other hand, this class deos not know about the game mode
*
 ***************************************************************************/

#include "boardhandler.h"
#include "boardwindow.h"
#include "board.h"
#include "sgfparser.h"
#include "move.h"
#include "matrix.h"
#include "tree.h"

#include <iostream>

BoardHandler::BoardHandler(BoardWindow *bw, Tree *t, int size)
	:QObject(bw)
{
	boardSize = size;	

	boardwindow =bw;

	board = bw->getBoard();
	Q_CHECK_PTR(board);
	
	// Create a variation tree
	if(t)
		tree = t;
	else 
		tree = new Tree(boardSize);
	Q_CHECK_PTR(tree);
	
//	currentMove = 0;
	lastValidMove = NULL;

	capturesBlack = capturesWhite = 0;
	markedDead = false;
	
	// initialises the timer
	wheelTime = QTime::currentTime();

	// Set game data to default
//	gameData = new GameData();
	
//	clipboardNode = NULL;
//	nodeResults = NULL;

  	// Assume we display events (incoming move when observing)
  	//display_incoming_move = true ;

	//local_stone_sound = setting->readBoolEntry("SOUND_STONE");
}

BoardHandler::~BoardHandler()
{

}


void BoardHandler::clearData()
{
	tree->init(boardSize);
	lastValidMove = NULL;
//	currentMove = 0;
//	gameMode = modeNormal;
//	markType = markNone;

	board->clearData();
	capturesBlack = capturesWhite = 0;
	markedDead = false;
//	if (nodeResults != NULL)
//		nodeResults->clear();
}


void BoardHandler::slotNavForward()
{
//	if (gameMode == modeScore)
//		return false;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->nextMove();
	if (m != NULL)
		updateMove(m);
	
}


void BoardHandler::slotNavBackward()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->previousMove();
	if (m != NULL)
		updateMove(m);
}

void BoardHandler::slotNavFirst()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	// We need to set the markers. So go the tree upwards
//	Move *m = tree->getCurrent();
//	CHECK_PTR(m);
	
	// Ascent tree until root reached
//	while (m->parent != NULL)
//		m = tree->previousMove();
	
	tree->setToFirstMove();  // Set move to root
	Move *m = tree->getCurrent();
	if (m != NULL)
		updateMove(m);

}

void BoardHandler::slotNavLast()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->getCurrent();
	Q_CHECK_PTR(m);
	
	// Descent tree to last son of main variation
	while (m->son != NULL)
		m = tree->nextMove();
	
	if (m != NULL)
		updateMove(m);
}

void BoardHandler::slotNavPrevComment()
{
//	 if (gameMode == modeScore)
//		return;

	Q_CHECK_PTR(tree);

	Move  *m = tree->getCurrent()->parent ;

	Q_CHECK_PTR(m);

	while (m != NULL)
	{
		if (m->getComment() != "")
			break;
		if (m->parent == NULL)
			break;
		m = m->parent;
	}

	if (m != NULL)
	{
		tree->setCurrent(m);
		updateMove(m);
	}
}

void BoardHandler::slotNavNextComment()
{
//	if (gameMode == modeScore)
//		return;


	Q_CHECK_PTR(tree);

	Move  *m = tree->getCurrent()->son ;

	Q_CHECK_PTR(m);

	while (m != NULL)
	{
		if (m->getComment() != "" )
			break;
		if (m->son == NULL)
			break;
		m = m->son;
	}

	if (m != NULL)
	{
		tree->setCurrent(m);
		updateMove(m);
	}
}

/*
 * Called when the 'nav to' button is pressed
 */
void BoardHandler::slotNavIntersection()
{
	//should not happen
	if (boardwindow->getGameMode() != modeNormal)
		return;

	boardwindow->setGamePhase ( phaseNavTo );
	board->setCursorType(cursorNavTo);
}

/*
 * Called after the preceding (slot nav Intersection)
 * When the intersection 'x/y' has been clicked on
 */
void BoardHandler::findMoveByPos(int x, int y)
{
	Move *m = tree->findMoveInMainBranch(x, y);
	
	//if (boardwindow->getGamePhase() == phaseNavTo)
	boardwindow->setGamePhase ( phaseOngoing );

	if (m != NULL)
	{
		tree->setCurrent(m);
		updateMove(m);
	}
	else
		QApplication::beep();

}

void BoardHandler::slotNavNextVar()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->nextVariation();
	if (m == NULL)
		return;
	
	updateMove(m);
}

void BoardHandler::slotNavPrevVar()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->previousVariation();
	if (m == NULL)
		return;
	
	updateMove(m);
}


void BoardHandler::slotNavStartVar()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	if (tree->getCurrent()->parent == NULL)
		return;
	
	Move *tmp = tree->previousMove(),
		*m = NULL;
	
	if (tmp == NULL)
		return;
	
	// Go up tree until we find a node that has > 1 sons
	while ((m = tree->previousMove()) != NULL && tree->getNumSons() <= 1)
	// Remember move+1, as we set current to the
	// first move after the start of the variation 
		tmp = m;

	
	if (m == NULL)  // No such node found, so we reached root.
	{
		tmp = tree->getRoot();
		// For convinience, if we have Move 1, go there. Looks better.
		if (tmp->son != NULL)
			tmp = tree->nextMove();
	}
	
	// If found, set current to the first move inside the variation
	Q_CHECK_PTR(tmp);
	tree->setCurrent(tmp);
	updateMove(tmp);
}

void BoardHandler::slotNavNextBranch()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->getCurrent(),
		*remember = m;  // Remember current in case we dont find a branch
	Q_CHECK_PTR(m);
	
	// We are already on a node with 2 or more sons?
	if (tree->getNumSons() > 1)
	{
		m = tree->nextMove();
		updateMove(m);
		return;
	}
	
	// Descent tree to last son of main variation
	while (m->son != NULL && tree->getNumSons() <= 1)
		m = tree->nextMove();
	
	if (m != NULL && m != remember)
	{
		if (m->son != NULL)
			m = tree->nextMove();
		updateMove(m);
	}
	else
		tree->setCurrent(remember);
}

/*
 * This function resumes back to the first move in the main branch
 */
void BoardHandler::slotNavMainBranch()
{
//	if (gameMode == modeScore)
//		return;
	
	Q_CHECK_PTR(tree);
	
	if (tree->getCurrent()->parent == NULL)
		return;
	
	Move *m = tree->getCurrent(),
		*old = m,
		*lastOddNode = NULL;
	
	if (m == NULL)
		return;
	
	while ((m = m->parent) != NULL)
	{
		if (tree->getNumSons(m) > 1 && old != m->son)  
			// Remember a node when we came from a branch
			lastOddNode = m;

		m->marker = old;
		old = m;
	}
	
	if (lastOddNode == NULL)
		return;
	
	Q_CHECK_PTR(lastOddNode);

	// Clear the marker, so we can proceed in the main branch
	lastOddNode->marker = NULL;  

	tree->setCurrent(lastOddNode);
	updateMove(lastOddNode);
}


void BoardHandler::slotNthMove(int n)
{
//	if (gameMode == modeScore)
//		return;
	
	Q_ASSERT(n >= 0);
	Q_CHECK_PTR(tree);
	
	Move *m = tree->getCurrent(),
		*old = m;
	Q_CHECK_PTR(m);
	
	int currentMove = m->getMoveNumber();

	while (m != NULL)
	{
		if (m->getMoveNumber() == n)
			break;
		if ((n >= currentMove && m->son == NULL && m->marker == NULL) ||
			(n < currentMove && m->parent == NULL))
			break;
		if (n > currentMove)
		{
			if (m->marker == NULL)
				m = m->son;
			else
				m = m->marker;
			m->parent->marker = m;
		}	    
		else
		{
			m->parent->marker = m;
			m = m->parent;
		}
	}
	
	if (m != NULL && m != old)
		gotoMove(m);

}


void BoardHandler::gotoMove(Move *m)
{
	Q_CHECK_PTR(m);
	tree->setCurrent(m);
	updateMove(m);
}


/*
 * This deals with updating the 'board' and the 'interface handler' with the data
 * stored in the tree at the move place.
 * This involves calling and update for the board stones, and 
 * an update of the interface
 */
void BoardHandler::updateMove(Move *m, bool /*ignore_update*/)
{
	if (m == NULL)
	{
		m = tree->getCurrent();
	}
	//qDebug("BoardHandler::updateMove(Move *m)");
/*	
	// Fastloading. Create matrix for current move and insert marks
	if (!m->checked)
	{
		qDebug("NOT CHECKED");
		if (m->parent != NULL)
		{
#ifndef NO_DEBUG
			if (tree->getCurrent()->getMatrix() != NULL)
			{
				qFatal("MOVE HAS A MATRIX BUT SHOULD NOT!");
			}
#endif
			Matrix *dad = m->parent->getMatrix();
			Matrix *neu = new Matrix(*dad);
			m->setMatrix(neu);
		}
		addStoneSGF(m->getColor(), m->getX(), m->getY());
		
		QIntDict<FastLoadMark> *d = m->fastLoadMarkDict;
		if (d != NULL && !d->isEmpty())
		{
			QIntDictIterator<FastLoadMark> it(*d);
			while (it.current())
			{
				m->getMatrix()->insertMark(it.current()->x,
					it.current()->y,
					it.current()->t);
				if (it.current()->t == markText && !(it.current()->txt).isNull())
					m->getMatrix()->setMarkText(it.current()->x,
					it.current()->y,
					it.current()->txt);
				++it;
			}
			delete d;
			m->fastLoadMarkDict = NULL;
		}
		
		m->checked = true;
	}
*/	
	Q_CHECK_PTR(m);
//	int currentMove = m->getMoveNumber();
//	int brothers = getNumBrothers();

	// Update slider branch length
	boardwindow->getInterfaceHandler()->setSliderMax(m->getMoveNumber() + tree->getBranchLength());	

	// Display move data and comment in the GUI
//	if (m->getGameMode() == modeNormal)
		boardwindow->getInterfaceHandler()->setMoveData(
			m->getMoveNumber(), 
			(m->getColor() != stoneBlack), 
			m->getNumBrothers(), 
			m->getNumSons(),
			m->hasParent(), 
			m->hasPrevBrother(), 
			m->hasNextBrother(),
			m->getX(), 
			m->getY());
//	else
//		board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), brothers, getNumSons(),
//		hasParent(), hasPrevBrother(), hasNextBrother());
//	if (board->get_isLocalGame())
	// Update comment if normal game (if observe or match, the comment zone is kept as is)
	if (boardwindow->getGameMode() == modeNormal)
		boardwindow->getInterfaceHandler()->displayComment(m->getComment());


  
	// Get rid of the varation ghosts
//	if (setting->readIntEntry("VAR_GHOSTS"))//TODO
		board->removeGhosts();
	
	// Get rid of all marks except the last-move-mark
//	board->hideAllMarks();
	
	// Remove territory marks
//	if (tree->getCurrent()->isTerritoryMarked())
//	{
//		tree->getCurrent()->getMatrix()->clearTerritoryMarks();
//		tree->getCurrent()->setTerritoryMarked(false);
//	}
	
	// Unshade dead stones
//	if (markedDead)
//	{
		board->removeDeadMarks();
//		markedDead = false;
//	}
	
//	if (m->getGameMode() == modeNormal || m->getGameMode() == modeObserve )  //SL add eb 8
		// If the node is in normal mode, show the circle to mark the last move
		    board->updateLastMove(m->getColor(), m->getX(), m->getY());
//	else
		// ... if node is in edit mode, just delete that circle
//	{
//		board->removeLastMoveMark();
//		board->setCurStoneColor();
//	}
	updateCursor(m->getColor());
//	board->setCursorType(cur);
	

	// Update the ghosts indicating variations
	if (m->getNumBrothers())// && setting->readIntEntry("VAR_GHOSTS")) TODO
		updateVariationGhosts(m);
	
	// Oops, something serious went wrong
	if (m->getMatrix() == NULL)
		qFatal("   *** Move returns NULL pointer for matrix! ***");
	
	// Synchronize the board with the current nodes matrix, provided we want to 
//  if (!ignore_update)                //SL added eb 9 - this if we are browsing an observing game and an undo incomes
 	updateAll(m->getMatrix()); //FIXME this should probably be above in the code
	
	// Display captures or score in the GUI
//	if (m->isScored())  // This move has been scored
//		board->getInterfaceHandler()->setCaptures(m->getScoreBlack(), m->getScoreWhite(), true);
//	else
		boardwindow->getInterfaceHandler()->setCaptures(m->getCapturesBlack(), m->getCapturesWhite());

	// Display times
//	if (currentMove == 0)
//	{
//		if (gameData->timelimit == 0)
//			board->getInterfaceHandler()->setTimes("00:00", "-1", "00:00", "-1");
//		else
//		{
			// set Black's and White's time to timelimit
//			board->getInterfaceHandler()->setTimes(true, gameData->timelimit, -1);
//			board->getInterfaceHandler()->setTimes(false, gameData->timelimit, -1);
//		}
//	}
//	else if (m->getTimeinfo())
//		board->getInterfaceHandler()->setTimes(!getBlackTurn(), m->getTimeLeft(), m->getOpenMoves() == 0 ? -1 : m->getOpenMoves());

//	board->updateCanvas();
}

bool BoardHandler::updateAll(Matrix *m, bool /* toDraw*/)
{
	
	Q_CHECK_PTR(m);
	
	tree->updateAll(m);	
	
//	m->debug();

//	Stone *stone;
	bool modified = false;//, fake = false;
	short data;
	bool dead;
	/*
	* Synchronize the matrix with the stonehandler data and
	* update the canvas.
	* This is usually called when navigating through the tree.
	*/
	
	for (int y=1; y<=boardSize; y++)
	{
		for (int x=1; x<=boardSize; x++)
		{
			// Extract the data for the stone from the matrix
			data = (m->at(x-1, y-1) % 10);
			dead = (data < 0);
			data = abs(data);
			
			board->updateStone((StoneColor)data,x,y, dead);

			
			// Skip mark drawing when reading sgf
//			if (!toDraw)
//				continue;
			
			// Extract the mark data from the matrix
			data = abs(m->at(x-1, y-1) / 10);
			switch (data)
			{
			case markSquare:
				modified = true;
				board->setMark(x, y, markSquare, false);
				break;
				
			case markCircle:
				modified = true;
				board->setMark(x, y, markCircle, false);
				break;
				
			case markTriangle:
				modified = true;
				board->setMark(x, y, markTriangle, false);
				break;
				
			case markCross:
				modified = true;
				board->setMark(x, y, markCross, false);
				break;
				
			case markText:
				modified = true;
				board->setMark(x, y, markText, false, m->getMarkText(x, y));
				break;
				
			case markNumber:
				modified = true;
				board->setMark(x, y, markNumber, false, m->getMarkText(x, y));
				break;
				
			case markTerrBlack:
				modified = true;
				board->setMark(x, y, markTerrBlack, false);
				break;
				
			case markTerrWhite:
				modified = true;
				board->setMark(x, y, markTerrWhite, false);
				break;
				
			case markNone:
				if (board->hasMark(x, y))
				{
					modified = true;
					board->removeMark(x, y, false);
				}
			}


		}
	}

	return modified;
}

/*
 * Update the cursor for sending an order to the 'board'
 */
void BoardHandler::updateCursor(StoneColor currentMoveColor)
{
	CursorType cur = cursorIdle;
	
	switch (boardwindow->getGameMode())
	{
	case modeNormal :
	case modeTeach :
	case modeReview :
		if (boardwindow->getGamePhase() == phaseScore ||boardwindow->getGamePhase() == phaseEdit )
			cur = cursorIdle;
		else
			cur = (currentMoveColor == stoneBlack ? cursorGhostWhite : cursorGhostBlack);
		break; 

	case modeObserve :
		break;//cur = cursorIdle;

	case modeMatch :
		if  (currentMoveColor == stoneBlack )
			cur =  ( boardwindow->getMyColorIsWhite() ? cursorGhostWhite : cursorIdle );
		else
			cur = ( boardwindow->getMyColorIsBlack() ? cursorGhostBlack : cursorIdle );
		break;

	case modeComputer :
		if  (currentMoveColor == stoneBlack )
			cur = ( boardwindow->getMyColorIsWhite() ? cursorGhostWhite : cursorWait );
		else
			cur = ( boardwindow->getMyColorIsBlack() ? cursorGhostBlack : cursorWait );
		break;
	}

	board->setCursorType(cur);
//	return cursorIdle;
}

/*
 * Update the variation marks on the board if any
 */
void BoardHandler::updateVariationGhosts(Move *move)
{
	// qDebug("BoardHandler::updateVariationGhosts()");
	
	Move *m = move->parent->son;
	Q_CHECK_PTR(m);
	
	do 
	{
	//	if (m == tree->getCurrent())
	//		continue;
		board->setVarGhost(m->getColor(), m->getX(), m->getY());
	} while ((m = m->brother) != NULL);
}


void BoardHandler::slotWheelEvent(QWheelEvent *e)
{
	// leave if not editing
	if (boardwindow->getGameMode() != modeNormal)
		return;

	if (boardwindow->getGamePhase() != phaseOngoing)
		return;

	// Check delay
	if (QTime::currentTime() < wheelTime)
		return;
	
	// Needs an extra check on variable mouseState as state() does not work on Windows.
	if (e->delta() == 120)
	{
		if (e->buttons() == Qt::RightButton || e-> modifiers() ==  Qt::ShiftModifier)
			slotNavNextVar();
		else
			slotNavForward();
	}
	else
	{
		if (e->buttons() == Qt::RightButton || e-> modifiers() ==  Qt::ShiftModifier)//|| mouseState == RightButton)
			slotNavPrevVar();
		else
			slotNavBackward();
	}

	// Delay of 100 msecs to avoid too fast scrolling
	wheelTime = QTime::currentTime();
	wheelTime = wheelTime.addMSecs(50);

	e->accept();
}

/*
 * Performs all operations on the matrix of current move to display score marks
 * and score informaton on the uI
 */
void BoardHandler::countScore()
{
	tree->getCurrent()->getMatrix()->clearTerritoryMarks();

	// capturesBlack -= caps_black;
	// capturesWhite -= caps_white;
	capturesBlack = tree->getCurrent()->getCapturesBlack();
	capturesWhite = tree->getCurrent()->getCapturesWhite();
//	caps_black = 0;
//	caps_white = 0;
	tree->getCurrent()->setScored(true);

	// Copy the current matrix
	Matrix *m = new Matrix(*(tree->getCurrent()->getMatrix()));
	Q_CHECK_PTR(m);

//	m->debug();
	// Do some cleanups, we only need stones
	//m->absMatrix();
	m->clearAllMarks();
	
	// Mark all dead stones in the matrix with negative number
	int i=0, j=0;
/*	for (i=0; i<board->getBoardSize(); i++)
		for (j=0; j<board->getBoardSize(); j++)
			if (stoneHandler->hasStone(i+1, j+1) == 1)
				if (stoneHandler->getStoneAt(i+1, j+1)->isDead())
					m->set(i, j, m->at(i, j) * -1);
				else if (stoneHandler->getStoneAt(i+1, j+1)->isSeki())
					m->set(i, j, m->at(i, j) * MARK_SEKI);
*/				
	int terrWhite = 0, terrBlack = 0;

	while (m != NULL)
	{
		bool found = false;
		
		for (i=0; i< boardSize; i++)
		{
			for (j=0; j< boardSize; j++)
			{
				if (m->at(i, j) <= 0)
				{
					found = true;
					break;
				}
			}
			if (found)
				break;
		}
		
		if (!found)
			break;
		
		// Traverse the enclosed territory. Resulting color is in col afterwards
		StoneColor col = stoneNone;
		m->traverseTerritory( i, j, col);
		
		// Now turn the result into real territory or dame points
		for (i=0; i<boardSize; i++)
		{
			for (j=0; j<boardSize; j++)
			{
				if (m->at(i, j) == MARK_TERRITORY_VISITED)
				{
					// Black territory
					if (col == stoneBlack)
					{
						tree->getCurrent()->getMatrix()->removeMark(i+1, j+1);
						tree->getCurrent()->getMatrix()->insertMark(i+1, j+1, markTerrBlack);
						terrBlack ++;
						m->set(i, j, MARK_TERRITORY_DONE_BLACK);
					}
					// White territory
					else if (col == stoneWhite)
					{
						tree->getCurrent()->getMatrix()->removeMark(i+1, j+1);
						tree->getCurrent()->getMatrix()->insertMark(i+1, j+1, markTerrWhite);
						terrWhite ++;
						m->set(i, j, MARK_TERRITORY_DONE_WHITE);
					}
					// Dame
					else
						m->set(i, j, MARK_TERRITORY_DAME);
				}
			}
		}
	}

	// Finally, remove all false eyes that have been marked as territory. This
	// has to be here, as in the above loop we did not find all dame points yet.
	for (i = 0; i < boardSize; i++) 
	{
		for (j = 0; j < boardSize; j++) 
		{
			if (	m->at(i, j) == MARK_TERRITORY_DONE_BLACK ||
				m->at(i, j) == MARK_TERRITORY_DONE_WHITE) 
			{
				StoneColor col = (m->at(i, j) == MARK_TERRITORY_DONE_BLACK ? stoneBlack : stoneWhite);
				if (m->checkFalseEye(i, j, col)) 
				{
					tree->getCurrent()->getMatrix()->removeMark(i + 1, j + 1);
					if (col == stoneBlack)
						terrBlack--;
					else
						terrWhite--;
				}
			}
		}
	}

	// Mark the move having territory marks
	tree->getCurrent()->setTerritoryMarked(true);
	// Paint the territory on the board
	updateAll(tree->getCurrent()->getMatrix());
//	board->updateCanvas();
	
	// Update Interface
/*	boardwindow->getInterfaceHandler()->setScore(terrBlack, capturesBlack + caps_black,
		terrWhite, capturesWhite+ caps_white,
		boardwindow->getGameData()->komi);
*/	
	delete m;
}

/*
 * Called by qgoboard when score button is pressed up, leaving score mode
 */
void BoardHandler::exitScore()
{
	// Remove territory marks
	if (tree->getCurrent()->isTerritoryMarked())
	{
		tree->getCurrent()->getMatrix()->clearTerritoryMarks();
		tree->getCurrent()->setTerritoryMarked(false);
		tree->getCurrent()->setScored(false);
	}
	
	// Unshade dead stones
//	board->removeDeadMarks();
	markedDead = false;
	
	updateMove(tree->getCurrent());
}
