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



BoardHandler::BoardHandler(BoardWindow *bw, Tree *t, int size)
	:QObject(bw)
{
	boardSize = size;	

	boardwindow =bw;

	board = bw->getBoard();
	Q_CHECK_PTR(board);
	
	// Create a SGFParser instance
	//sgfParser = new SGFParser(this);
	//CHECK_PTR(sgfParser);
	
	// Create a variation tree
	if(t)
		tree = t;
	else 
		tree = new Tree(boardSize);
	Q_CHECK_PTR(tree);
	
//	currentMove = 0;
	lastValidMove = NULL;
//	gameMode = modeNormal;
//	markType = markNone;
	capturesBlack = capturesWhite = 0;
	markedDead = false;
	
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

/*
 bool BoardHandler::loadSGF(const QString &fileName, const QString SGFLoaded, bool  fastLoad )
{

	SGFParser *sgfParser = new SGFParser(tree);
	
	// Load the sgf file
//	QString SGFloaded = sgfParser->loadFile(fileName);

	if (!sgfParser->doParse(SGFLoaded))
		return false ;	
	
	board->clearData();
	tree->setToFirstMove();	

	return true;
}
*/

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


//bool BoardHandler::nextMove(bool /*autoplay*/)
/*
{
//	if (gameMode == modeScore)
//		return false;
	
	Q_CHECK_PTR(tree);
	
	Move *m = tree->nextMove();
	if (m == NULL)
		return false;
	
//	if (autoplay)
//		setting->qgo->playAutoPlayClick();
	
	updateMove(m);
	return true;
}
*/

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
	Move *m = tree->findMoveInBranch(x, y);
	
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
			(m->getColor() == stoneWhite), 
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
	// Update comment
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
	CursorType cur = updateCursor(m->getColor());
	board->setCursorType(cur);
	

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
	// qDebug("StoneHandler::updateAll(Matrix *m) - toDraw = %d", toDraw);
	
	Q_CHECK_PTR(m);
	
	// m->debug();
	
//	Stone *stone;
	bool modified = false;//, fake = false;
	short data;

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
			data = abs(m->at(x-1, y-1) % 10);
			
			board->updateStone((StoneColor)data,x,y);

			
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
CursorType BoardHandler::updateCursor(StoneColor currentMoveColor)
{
	switch (boardwindow->getGameMode())
	{
	case modeNormal :
	case modeTeach :
	case modeReview :
		return (currentMoveColor == stoneBlack ? cursorGhostWhite : cursorGhostBlack);

	case modeObserve :
		return cursorIdle;

	case modeMatch :
		if  (currentMoveColor == stoneBlack )
			return ( boardwindow->getMyColorIsWhite() ? cursorGhostWhite : cursorIdle );
		else
			return ( boardwindow->getMyColorIsBlack() ? cursorGhostBlack : cursorIdle );

	case modeComputer :
		if  (currentMoveColor == stoneBlack )
			return ( boardwindow->getMyColorIsWhite() ? cursorGhostWhite : cursorWait );
		else
			return ( boardwindow->getMyColorIsBlack() ? cursorGhostBlack : cursorWait );

	}

	return cursorIdle;
}


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
