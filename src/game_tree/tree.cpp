/*
* tree.cpp
*/

#include "tree.h"
#include "move.h"
#include "matrix.h"

#include <iostream>


#include <QtCore>

//Class Move;

Tree::Tree(int board_size)
{
	root = new Move(board_size);
	current = root;
	boardSize = board_size;

/* 
 * This initialisation is from stonehandler
 * It might not be needed
 */
	groups = new QList<Group*>();//::QList();
	stones = new QHash<int,MatrixStone *>();//::QHash();
}

void Tree::init(int board_size)
{
	clear();
	root = new Move(board_size);
	current = root;
	boardSize = board_size;
}

Tree::~Tree()
{
	qDeleteAll(*stones);
	stones->clear();
	
	qDeleteAll( *groups);
	groups->clear();

	clear();
}

/*
 *  Adds a brother at the end of the brother chain of current()
 */
bool Tree::addBrother(Move *node)
{
	if (root == NULL)
	{
		qFatal("Error: No root!");
	}
	else
	{
		if (current == NULL)
		{
			qFatal("Error: No current node!");
			return false;
		}
		
		Move *tmp = current;
		
		// Find brother farest right
		while (tmp->brother != NULL)
			tmp = tmp->brother;
		
		tmp->brother = node;
		node->parent = current->parent;
		node->setTimeinfo(false);
	}
	
	current = node;
	
	return true;
}

/*
 * Returns: True  - A son found, added as brother
 *          False - No son found, added as first son
 */
bool Tree::addSon(Move *node)
{
	if (root == NULL)
	{
		qFatal("Error: No root!");
		return false;
	}
	else
	{
		if (current == NULL)
		{
			qFatal("Error: No current node!");
			return false;
		}
		
		// current node has no son?
		if (current->son == NULL)
		{
			current->son = node;
			node->parent = current;
			node->setTimeinfo(false);
			current = node;
			return false;
		}
		// A son found. Add the new node as farest right brother of that son
		else
		{
			current = current->son;
			if (!addBrother(node))
			{
				qFatal("Failed to add a brother.");
				return false;
			}
			current = node;
			return true;
		}
	}
}

int Tree::getNumBrothers()
{
	if (current == NULL)// || current->parent == NULL)
		return 0;
	else
		return current->getNumBrothers();
//	Move *tmp = current->parent->son;
//	int counter = 0;
//	
//	while ((tmp = tmp->brother) != NULL)
//		counter ++;
//	
//	return counter;
}

bool Tree::hasNextBrother()
{
	if (current == NULL || current->brother == NULL)
		return false;
	
	return current->brother != NULL;
}

bool Tree::hasPrevBrother(Move *m)
{
	if (m == NULL)
		m = current;
	
	if (root == NULL || m == NULL)
		return false;
	
	Move *tmp;
	
	if (m->parent == NULL)
	{
		if (m == root)
			return false;
		else
			tmp = root;
	}
	else
		tmp = m->parent->son;
	
	return tmp != m;
}

int Tree::getNumSons(Move *m)
{
	if (m == NULL)
	{
		if (current == NULL)
			return 0;
		
		m = current;
	}
	
	return m->getNumSons();
//	Move *tmp = m->son;
	
//	if (tmp == NULL)
//		return 0;
	
//	int counter = 1;
//	while ((tmp = tmp->brother) != NULL)
//		counter ++;
	
//	return counter;
}

int Tree::getBranchLength(Move *node)
{
	Move *tmp;
	
	if (node == NULL)
	{
		if (current == NULL)
			return -1;
		tmp = current;
	}
	else
		tmp = node;
	
	Q_CHECK_PTR(tmp);
	
	int counter=0;
	// Go down the current branch, use the marker if possible to remember a previously used path
	while ((tmp = (tmp->marker != NULL ? tmp->marker : tmp->son)) != NULL)
		counter ++;
	
	return counter;
}

Move* Tree::nextMove()
{
	if (root == NULL || current == NULL || current->son == NULL)
		return NULL;
	
	if (current->marker == NULL)  // No marker, simply take the main son
		current = current->son;
	else
		current = current->marker;  // Marker set, use this to go the remembered path in the tree
	
	current->parent->marker = current;  // Parents remembers this move we went to
	return current;
}

Move* Tree::previousMove()
{
	if (root == NULL || current == NULL || current->parent == NULL)
		return NULL;
	
	current->parent->marker = current;  // Remember the son we came from
	current = current->parent;          // Move up in the tree
	return current;
}

Move* Tree::nextVariation()
{
	if (root == NULL || current == NULL || current->brother == NULL)
		return NULL;
	
	current = current->brother;
	return current;
}

Move* Tree::previousVariation()
{
	if (root == NULL || current == NULL)
		return NULL;
	
	Move *tmp, *old;
	
	if (current->parent == NULL)
	{
		if (current == root)
			return NULL;
		else
			tmp = root;
	}
	else
		tmp = current->parent->son;
	
	old = tmp;
	
	while ((tmp = tmp->brother) != NULL)
	{
		if (tmp == current)
			return current = old;
		old = tmp;
	}
	
	return NULL;
}

bool Tree::hasSon(Move *m)
{
	if (root == NULL || m == NULL || current == NULL || current->son == NULL)
		return false;
	
	Move *tmp = current->son;
	
	do {
		if (m->equals(tmp))
		{
			current = tmp;
			return true;
		}
	} while ((tmp = tmp->brother) != NULL);
	
	return false;
}

void Tree::clear()
{
#ifndef NO_DEBUG
	qDebug("Tree had %d nodes.", count());
#endif
	
	if (root == NULL)
		return;
	
	traverseClear(root);
	
	root = NULL;
	current = NULL;
}

void Tree::traverseClear(Move *m)
{
	Q_CHECK_PTR(m);
	QStack<Move*> stack;
	QStack<Move*> trash;

	Move *t = NULL;
	
	// Traverse the tree and drop every node into stack trash
	stack.push(m);
	
	while (!stack.isEmpty())
	{
		t = stack.pop();
		if (t != NULL)
		{
			trash.push(t);
			stack.push(t->brother);
			stack.push(t->son);
		}
	}
	
	// Clearing this stack deletes all moves. Smart, eh?
	//trash.clear();
	// QT doc advises this code instead of the above
	while (!trash.isEmpty())
		delete trash.pop();
}

/*
 * Find a move starting from the given in the argument in the 
 * all following branches.
 * Results are saved into the reference variable result.
 */
void Tree::traverseFind(Move *m, int x, int y, QStack<Move*> &result)
{
	Q_CHECK_PTR(m);
	QStack<Move*> stack;
	Move *t = NULL;
	
	// Traverse the tree and drop every node into stack result
	stack.push(m);
	
	while (!stack.isEmpty())
	{
		t = stack.pop();
		if (t != NULL)
		{
			if (t->getX() == x && t->getY() == y)
				result.push(t);
			stack.push(t->brother);
			stack.push(t->son);
		}
	}
}

Move* Tree::findMove(Move *start, int x, int y, bool checkmarker)
{
	if (start == NULL)
		return NULL;
	
	Move *t = start;
	
	do {
		if (t->getX() == x && t->getY() == y)
			return t;
		if (checkmarker && t->marker != NULL)
			t = t->marker;
		else
			t = t->son;
	} while (t != NULL);
	
	return NULL;
}

int Tree::count()
{
	if (root == NULL)
		return 0;
	
	QStack<Move*> stack;
	int counter = 0;
	Move *t = NULL;
	
	// Traverse the tree and count all moves
	stack.push(root);
	
	while (!stack.isEmpty())
	{
		t = stack.pop();
		if (t != NULL)
		{
			counter ++;
			stack.push(t->brother);
			stack.push(t->son);
		}
	}
	
	return counter;
}

void Tree::setToFirstMove()
{
	if (root == NULL)
		qFatal("Error: No root!");
	
	current = root;
}

int Tree::mainBranchSize()
{
	if (root == NULL || current == NULL )
		return 0;
	
	Move *tmp = root;
	
	int counter = 1;
	while ((tmp = tmp->son) != NULL)
		counter ++;
	
	return counter;    
}


Move *Tree::findLastMoveInMainBranch()
{
	Move *m = root;
	Q_CHECK_PTR(m);

	if (m==NULL)
	return NULL;
  
	// Descend tree until root reached
	while (m->son != NULL)
		m = m->son;

	return m;
}

/*
 * This creates an empty move in the tree
 */
void Tree::createMoveSGF( bool brother, bool fastLoad)
{
	// qDebug("BoardHandler::createMoveSGF() - %d", mode);
	
	Move *m;
	
	if (! fastLoad)
	{
		Matrix *mat = current->getMatrix();
		m = new Move(stoneBlack, -1, -1, current->getMoveNumber()+1, phaseOngoing, *mat);
	}
	else
	{
		m = new Move(stoneBlack, -1, -1, current->getMoveNumber()+1, phaseOngoing);
	}
	
	if (!brother && hasSon(m))
	{
		qDebug("*** HAVE THIS SON ALREADY! ***");
		delete m;
		return;
	}
	
	if (!fastLoad ) // mode is ALWAYS normal && mode == modeNormal)
		m->getMatrix()->clearAllMarks();
	
	if (!brother)
		addSon(m);
	else
		addBrother(m);

}

/*
 * This is the code from boardhandler (as opposed to stonehandler::removestone)
 * It has been renamed to 'removeStoneSGF' because both boardhandler and stonehandler are here
 */
void Tree::removeStoneSGF(int x, int y, bool hide, bool new_node)
{
	
	bool res = removeStone(x, y, hide);
	
	if (res)
	{
		if ((current->getGamePhase() == phaseOngoing) && (current->getMoveNumber() > 0))//	currentMove > 0)
		{
			if (new_node)  // false, when reading sgf
				addMove(stoneNone, x, y);
			updateCurrentMatrix(stoneErase, x, y);
		}
		else
			updateCurrentMatrix(stoneErase, x, y);
		
//		board->checkLastMoveMark(x, y);
	}
	
//	board->setModified();
	
//	return res;
}


void Tree::updateCurrentMatrix(StoneColor c, int x, int y)
{
	// Passing?
	if (x == 20 && y == 20)
		return;
	
	if ((x < 1) || (x > boardSize) || (y < 1) || (y > boardSize))
	{
		qWarning("   *** Tree::updateCurrentMatrix() - Invalid move given: %d/%d at move %d ***",
			x, y, current->getMoveNumber());
		return;
	}
	
	Q_CHECK_PTR(current->getMatrix());
	
	if (c == stoneNone)
		current->getMatrix()->removeStone(x, y);
	else if (c == stoneErase)
		current->getMatrix()->eraseStone(x, y);
	else
		current->getMatrix()->insertStone(x, y, c);//, gameMode);
}


/*
 * This function is called by the SGF parser for adding pass moves or a 'StoneNone'
 */
void Tree::addMove(StoneColor c, int x, int y, bool clearMarks)
{
	// qDebug("BoardHandler::addMove - clearMarks = %d", clearMarks);
	
 	Matrix *mat = current->getMatrix();
	Q_CHECK_PTR(mat);
	 
	Move *m = new Move(c, x, y, current->getMoveNumber() +1 , phaseOngoing, *mat);
	Q_CHECK_PTR(m);
	
	if (hasSon(m))
	{
		// qDebug("*** HAVE THIS SON ALREADY! ***");
		delete m;
		return;
	}
	
	// Remove all marks from this new move. We dont do that when creating
	// a new variation in edit mode.
	if (clearMarks)
	{
		m->getMatrix()->clearAllMarks();
//		board->hideAllMarks();
	}
	
	addSon(m);
//	if (tree->addSon(m) && setting->readIntEntry("VAR_GHOSTS") && getNumBrothers())
//		updateVariationGhosts();
//	lastValidMove = m;
}

void Tree::doPass(bool sgf, bool fastLoad)
{
//	if (lastValidMove == NULL)
//	{
//		stoneHandler->checkAllPositions();
//	}

/////////////////FIXME We may have a problem at first move with handicap games ...
	StoneColor c = (current->getColor() == stoneWhite) ? stoneBlack : stoneWhite;
	
//	currentMove++;
	
	if (!sgf)
		addMove(c, 20, 20);
	else  // Sgf reading
	{
		if (current->parent != NULL)
			c = current->parent->getColor() == stoneBlack ? stoneWhite : stoneBlack;
		else
			c = stoneBlack;
		if (!fastLoad)
			editMove(c, 20, 20);
	}
	
	if (current->parent != NULL)
		current->setCaptures(current->parent->getCapturesBlack(),
		current->parent->getCapturesWhite());
/*	
	if (!sgf)
	{
		board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), getNumBrothers(), getNumSons(),
			hasParent(), hasPrevBrother(), hasNextBrother(), 20, 20);

		if (board->get_isLocalGame())
			board->getInterfaceHandler()->clearComment();
		
		board->updateLastMove(c, 20, 20);
		board->updateCanvas();
	}
	
	board->setModified();
*/
}

void Tree::editMove(StoneColor c, int x, int y)
{
	// qDebug("BoardHandler::editMove");
	
	if ((x < 1 || x > boardSize || y < 1 || y > boardSize) && x != 20 && y != 20)
		return;
	
	//Move *m = tree->getCurrent();
	//Q_CHECK_PTR(m);
	
	current->setX(x);
	current->setY(y);
	current->setColor(c);
	
	if (current->getMatrix() == NULL)
		qFatal("   *** Tree::editMove() - Current matrix is NULL! ***");

}

/*
 * This function updates the matrix with a stones at coords x,y and color c
 * Since the addMove reproduces the matrix of the previous move, it is updated here
 * It returns the same result as checkPosition (-1 for invalid move, else the number of stones taken)
 */
int Tree::addStoneSGF(StoneColor c, int x, int y, bool new_node)
{
//	bool shown = false;
	
	/* qDebug("BoardHandler::addStoneSGF(StoneColor c, int x, int y) - %d %d/%d %d",
	c, x, y, gameMode); */
/* FIXME	
	if (hasMatrix ??Stone(x, y) == 1)
	{
		// In edit mode, this overwrites an existing stone with another color.
		// This is different to the normal interface, when reading sgf files.
		if (gameMode == modeEdit &&
			tree->getCurrent() != NULL &&
			tree->getCurrent()->getMatrix()->at(x-1, y-1) != c)
		{
			if (!stoneHandler->removeStone(x, y, true))
				qWarning("   *** BoardHandler::addStoneSGF() Failed to remove stone! *** ");
			// updateCurrentMatrix(stoneNone, x, y);
		}
	}
*/
/*	
 *  : FIXME Make sure we don't need this

	if ((tree->getCurrent()->parent != NULL && lastValidMove != NULL &&
		gameMode == modeNormal &&
		tree->getCurrent()->parent != lastValidMove) ||
		(gameMode == modeNormal &&
		tree->getCurrent()->parent->getGameMode() == modeEdit) ||
		(gameData->handicap > 0 && currentMove == 1) ||
		lastValidMove == NULL)
		stoneHandler->checkAllPositions();
*/	
	if ((x < 1 || x > boardSize || y < 1 || y > boardSize) && x != 20 && y != 20)
		qWarning("BoardHandler::addStoneSGF() - Invalid position: %d/%d", x, y);
//	Stone *s = board->addStoneSprite(c, x, y, shown);
//	if (s == NULL)
//		return;
	
	// qDebug("Game Mode = %s", gameMode == modeNormal ? "NORMAL" : "EDIT");
	
	// Remember captures from move before adding the stone

	int capturesBlack, capturesWhite;
	if (current->parent != NULL)
	{
		capturesBlack = current->parent->getCapturesBlack();
		capturesWhite = current->parent->getCapturesWhite();
	}
	else
		capturesBlack = capturesWhite = 0;

	if (current->parent == root && ! new_node)
	{
		current->setMoveNumber(0); //TODO : make sure this is the proper way
		current->setX(-1);
		current->setY(-1);
	}
	else if (new_node)
		editMove(c, x, y);

	bool koStone;
	if (current->getMoveNumber() > 1)
		koStone = (current->parent->parent->getMatrix()->at(x-1, y-1) == c);


/*	
 * This code is replaced by the above line. 

	if (gameMode == modeNormal || gameMode == modeObserve || gameMode == modeTeach)
	{
		currentMove++;
		// This is a hack to force the first  move to be #1. For example, sgf2misc starts with move 0.
		if (currentMove == 1)
			tree->getCurrent()->setMoveNumber(currentMove);
	}
	else if(gameMode == modeEdit)
		tree->getCurrent()->setMoveNumber(0);

	

	if (new_node)
	{
		// Set move data
		editMove(c, x, y);
		
		// Update move game mode
//		if (gameMode != tree->getCurrent()->getGameMode())
//			tree->getCurrent()->setGameMode(gameMode);
	}
	
	// If we are in edit mode, dont check for captures (sgf defines)
//	stoneHandler->toggleWorking(true);
//	stoneHandler->addStone(s, !shown, gameMode == modeNormal, NULL);
*/
	/*
	* the code in the line above uses only the following
	*/
	MatrixStone *s = new MatrixStone ;
	s->x = x;
	s->y = y;
	s->c = c;
	/*
	* The update on the matrix has been moved up because int qGo code, 
	* the check Position is against the stones, white here its against the matrix
	* therefore, we needed to update the matrix before the check position
	*/	
	updateCurrentMatrix(c, x, y);
	int captures = checkPosition(s, current->getMatrix(), koStone);

	if (captures < 0)
		return -1;

	if (c == stoneBlack)
		capturesBlack += captures;
	else if (c == stoneWhite)
		capturesWhite += captures;
//	stoneHandler->toggleWorking(false);
//	updateCurrentMatrix(c, x, y);
	// Update captures
	current->setCaptures(capturesBlack, capturesWhite);
//	lastValidMove = tree->getCurrent();
	return captures;
}

/*
 * This functions does 2 things :
 * - It calculates the validity of a new stone inserted into a matrix
 * - It recalculates all the groups resulting in the stone being inserted
 * It returns -1 (invalid move) or the number of stones taken if the move is valid
 */
int Tree::checkPosition(MatrixStone *stone, Matrix *m, bool koStone)
{
 
//	if (!stone->visible())
//		return true;
	Q_CHECK_PTR(m);
	Group *active = NULL;
	
	// No groups existing? Create one.
	if (groups->isEmpty())
	{
		Group *g = m->assembleGroup(stone);
		Q_CHECK_PTR(g);
		groups->append(g);
		active = g;
	}

	// We already have one or more groups.
	else
	{
		bool flag = false;
		Group *tmp;
		
		for (int i=0; i<groups->count(); i++)
		{
			tmp = groups->at(i);
			//CHECK_PTR(tmp);
			
			// Check if the added stone is attached to an existing group.
			// If yes, update this group and replace the old one.
			// If the stone is attached to two groups, remove the second group.
			// This happens if the added stone connects two groups.
			if (tmp->isAttachedTo(stone))
			{
				// Group attached to stone
				if (!flag)
				{
					delete groups->takeAt(i);
//					if (!groups->remove(i))
//						qFatal("StoneHandler::checkPosition(Stone *stone):"
//						"Oops, removing an attached group failed.");
					active = m->assembleGroup(stone);
					groups->insert(i, active);
					flag = true;
				}
				// Groups connected, remove one
				else
				{
					if (active != NULL && active == groups->at(i))
						active = tmp;
					delete groups->takeAt(i);
//					if (!groups->remove(i))
//						qFatal("StoneHandler::checkPosition(Stone *stone): "
//						"Oops, removing a connected group failed.");
					i--;
				}
			}
		}
		
		// The added stone isnt attached to an existing group. Create a new group.
		if (!flag)
		{
			Group *g = m->assembleGroup(stone);
			Q_CHECK_PTR(g);
			groups->append(g);
			active = g;
		}
	}
	
	//active->debug();
	//qDebug("Tree :: Check Position for stone %d %d %d",stone->c, stone->x, stone->y);
	//m->debug();
	// Now we have to sort the active group as last in the groups QPtrList,
	// so if this one is out of liberties, we beep and abort the operation.
	// This prevents suicide moves.
	groups->append(groups->takeAt(groups->indexOf(active)));
	int stoneCounter = 0;
	// Check the liberties of every group. If a group has zero liberties, remove it.
	for ( int i=0; i<groups->count(); i++)
	{
		Group *tmp = groups->at(i);
		//CHECK_PTR(tmp);


 		tmp->setLiberties(m->countLiberties(tmp));

    
		//qDebug("Group #%d with %d liberties:", i, tmp->getLiberties());
		//tmp->debug();
		
		// Oops, zero liberties.
 		if (tmp->getLiberties() == 0)
		{
			// Suicide move?
			if (tmp == active)
			{
				if (active->count() == 1)
				{
					delete groups->takeAt(i);
					//groups->remove(i);
					removeStone(stone->x, stone->y, false);
				}
				return -1;
			}
			
			//was it a forbidden ko move ?
			if ((tmp->count() == 1) && koStone && ((m->countLiberties(active) == 0)))
			{
				//active->debug();
				delete groups->takeAt(groups->indexOf(active));
				//groups->remove(groups->indexOf(active));
				removeStone(stone->x, stone->y, false);
				return -1 ;
			}

//			int stoneCounter = 0;
			
			// Erase the stones of this group from the stones table.
//			QListIterator<Stone> it(*tmp);
			QListIterator<MatrixStone*> it( *tmp );
			for (; it.hasNext();)
			{
				MatrixStone *s = it.next();
				Q_CHECK_PTR(s);
//				if (workingOnNewMove) 
					updateCurrentMatrix(stoneNone, s->x, s->y);
//				removeStone(s->x, s->y);
				stoneCounter ++;
			}
			


			// Remove the group from the groups list.
			//qDebug("Oops, a group got killed. Removing killed group #%d", i);
			if (tmp == active)
				active = NULL;
			delete groups->takeAt(i);
//			if (!groups->remove(i))
//				qFatal("StoneHandler::checkPosition(Stone *stone): "
//				"Oops, removing a killed group failed.");
			i--;
			
			// Tell the boardhandler about the captures
//			boardHandler->setCaptures(stone->getColor(), stoneCounter);
		}
	}
	
	return stoneCounter;
}

//TODO : wipe out , moved to matrix
/*
Group* Tree::assembleGroup(MatrixStone *stone, Matrix *m)
{
//	if (stones->isEmpty())
//		qFatal("StoneHandler::assembleGroup(Stone *stone): No stones on the board!");
	
	Group *group = new Group();
	Q_CHECK_PTR(group);
	

	group->append(stone);
	
	int mark = 0;
	
	// Walk through the horizontal and vertical directions and assemble the
	// attached stones to this group.
	while (mark < group->count())
	{
		stone = group->at(mark);
		// we use preferably the matrix
		if ((m==NULL )|| (m!= NULL || m->at(stone->x - 1, stone->y - 1) != stoneNone ))
		{
			int 	stoneX = stone->x,
				stoneY = stone->y;
			StoneColor col = stone->c;
			
			// North
			//group = checkNeighbour(stoneX, stoneY-1, col, group,m);
			group = m->checkNeighbour(stoneX, stoneY-1, col, group);
			// West
			//group = checkNeighbour(stoneX-1, stoneY, col, group,m);
			group = m->checkNeighbour(stoneX-1, stoneY, col, group);
			// South
			//group = checkNeighbour(stoneX, stoneY+1, col, group,m);
			group = m->checkNeighbour(stoneX, stoneY+1, col, group);
			// East
			//group = checkNeighbour(stoneX+1, stoneY, col, group,m);
			group = m->checkNeighbour(stoneX+1, stoneY, col, group);
		}
		mark ++;
	}
	
	return group;
}

//TODO wipe this out : replaced in Matrix
Group* Tree::checkNeighbour(int x, int y, StoneColor color, Group *group, Matrix *m) 
{
/*
 * qGo original code completely rewritten
 *
	bool visible = false ;
	int size = boardSize;

	if (stones->find(Matrix::coordsToKey(x, y)) != stones->end())
		tmp = stones->find(Matrix::coordsToKey(x, y)).value();


  // Okay, this is dirty and synthetic :
  // Because we use this function where the matrix can be NULL, we need to check this
  // Furthermore, since this has been added after the first code, 
  // we keep the 'stone->visible' test where one should only use the 'matrix' code
	if (m != NULL && x-1 >= 0 && x-1 < size && y-1 >= 0 && y-1 < size) 
   		visible = (m->at(x - 1, y - 1) != stoneNone); 
  // We do this in order not to pass a null matrix to the matrix->at function 
  // (seen in handicap games)

  // again we priviledge matrix over stone visibility (we might be browsing a game)
	if (tmp != NULL && tmp->c == color &&  tmp->visible() (tmp->c != stoneNone) || visible)) 
	{
		if (!group->contains(tmp))
		{
			group->append(tmp);
//			tmp->checked = true;
		}
	}
*/
/*
	if (!m)
		qDebug("Oops : null matrix in Tree::checkNeighbour");

	// Are we into the board, and is the tentative stone present in the matrix ?
	if (x == 0 || x == boardSize + 1  || y == 0 || y == boardSize + 1 ||  (m->at(x - 1, y - 1) != color))
		return group;

	MatrixStone *tmp= new MatrixStone ;
	tmp->x = x;
	tmp->y = y;	
	tmp->c =  color;

	int mark = 0;
	bool found = FALSE;

	while ( mark < group->count() && !found )
	{
		//Do we find the same stone in the group ?
		if (! group->compareItems(tmp, group->at(mark)))
			found = TRUE ;

		mark ++;
	}
	
	// if not, we add it to the group
	if (!found)
		group->append(tmp);

	return group;
}
*/


bool Tree::removeStone(int x, int y, bool hide)
{
 	if (!hasMatrixStone(x, y))
		return false;
	
	if (!hide)
	{
    // We delete the killed stones only if we want to update the board (i.e. : not if we are browsing the game)
//	if (boardHandler->getDisplay_incoming_move())
		if (!stones->remove(Matrix::coordsToKey(x, y)))
		{
			   qWarning("   ***   Key for stone %d, %d not found!   ***", x, y);
			   return false;
		}

	}
	else
	{
		MatrixStone *s = NULL;
		if (stones->find(Matrix::coordsToKey(x, y)) == stones->end()) 
		{
			qWarning("   ***   Key for stone %d, %d not found!   ***", x, y);
			return false;
		}
		s = stones->find(Matrix::coordsToKey(x, y)).value();
		s->c = stoneNone;
	}
	
	return true;
}


/*
 *  0 : no stone
 * -1 : hidden
 *  1 : shown
 */
int Tree::hasMatrixStone(int x, int y)
{
	MatrixStone *s;
	
	if (stones->find(Matrix::coordsToKey(x, y)) == stones->end())
		return 0;
	
	s = stones->find(Matrix::coordsToKey(x, y)).value();
	if (s->c != stoneNone)
		return 1;
	
	return -1;
}

/*
//TODO wipe this function out : replaced in Matrix code
int Tree::countLiberties(Group *group, Matrix *m) 
{
//	CHECK_PTR(group);
//	CHECK_PTR(m); 
  
	int liberties = 0;
	QList<int> libCounted;
	
	// Walk through the horizontal and vertial directions, counting the
	// liberties of this group.
	for (int i=0; i<group->count(); i++)
	{
		MatrixStone *tmp = group->at(i);
		//CHECK_PTR(tmp);
		
		int 	x = tmp->x,
			y = tmp->y;
		
		// North
//		checkNeighbourLiberty(x, y-1, libCounted, liberties,m);
		m->checkNeighbourLiberty(x, y-1, libCounted, liberties);
		// West
		//checkNeighbourLiberty(x-1, y, libCounted, liberties,m);
		m->checkNeighbourLiberty(x-1, y, libCounted, liberties);
		// South
		//checkNeighbourLiberty(x, y+1, libCounted, liberties,m);
		m->checkNeighbourLiberty(x, y+1, libCounted, liberties);
		// East
		//checkNeighbourLiberty(x+1, y, libCounted, liberties,m);
		m->checkNeighbourLiberty(x+1, y, libCounted, liberties);
	}
	return liberties;
}

//TODO wipe this function out : replaced in Matrix code
void Tree::checkNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties, Matrix *m)
{
	if (!x || !y)
		return;
	
//	MatrixStone *s;
	Q_CHECK_PTR(m);
/*
 * We	will assume that in the qGo2 structure, passing a null matrix does not happen
 *
  if (m==NULL) //added eb 8 -> we don't have a matrix passed here, so we check on the board
  {
	  if (x <= boardHandler->board->getBoardSize() && y <= boardHandler->board->getBoardSize() && x >= 0 && y >= 0 &&
		  !libCounted.contains(100*x + y) &&
  		((s = stones->find(Matrix::coordsToKey(x, y))) == NULL ||
	  	!s->visible()))
	  {
		  libCounted.append(100*x + y);
		  liberties ++;
	  }
  }  
  else                                      
  {
*/    
/*
	if (	x <= boardSize && 
		y <= boardSize && x >= 0 && y >= 0 &&
	    	!libCounted.contains(100*x + y) &&
	    	(m->at(x - 1, y - 1) == stoneNone ))         // ?? check stoneErase ?
	{
		  libCounted.append(100*x + y);
		  liberties ++;
	}
//  }
}
*/

/*
 * This is used by the sgfparser when reverting to the previous node after the end of a branch
 * It is only used to recalculates all the groups according to the matrix
 */
bool Tree::updateAll(Matrix *m, bool /*toDraw*/)
{
	// qDebug("StoneHandler::updateAll(Matrix *m) - toDraw = %d", toDraw);
	
	Q_CHECK_PTR(m);
	
//	m->debug();
	
//	Stone *stone;
	bool modified = false;
//	bool fake = false;
//	short data;

	short color;	

	/*
	* Recalculates all the groups according to the matrix
	*/

	// First we remove verything from the group list
	// This also delete ALL the MatrixStones
	while (! groups->isEmpty())
		delete groups->takeFirst();
	//Might not be needed
	groups->clear();
	

	for (int y=1; y<=boardSize; y++)
	{
		for (int x=1; x<=boardSize; x++)
		{
			// Extract the data for the stone from the matrix
//			data = abs(m->at(x-1, y-1) % 10);
			color=m->at(x-1, y-1);

			if (color == stoneBlack || color ==stoneWhite)
			{
				MatrixStone *s = new MatrixStone;
				s->x=x;
				s->y=y;
				s->c= (color == stoneBlack ? stoneBlack : stoneWhite) ;

				checkPosition(s,m);
			}
		}
	}

	return modified;
}

/*
 * this deletes the current node an all its sons
 */
void Tree::deleteNode()
{
	Move *m = getCurrent(),
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
	else if (hasPrevBrother())
	{
		remember = previousVariation();
		if (m->brother != NULL)
			remember->brother = m->brother;
		else
			remember->brother = NULL;
	}
	else if (hasNextBrother())
	{
		remember = nextVariation();
		// Urgs, remember is now root.
		setRoot(remember);
	}
	else
	{
		// Oops, first and only move. We delete everything
		init(boardSize);
//		board->hideAllStones();
//		board->hideAllMarks();
//		board->updateCanvas();
//		lastValidMove = NULL;
//		stoneHandler->clearData();
//		updateMove(tree->getCurrent());
		return;
	}
	
	if (m->son != NULL)
		traverseClear(m->son);  // Traverse the tree after our move (to avoid brothers)
	delete m;                         // Delete our move
	setCurrent(remember);       // Set current move to previous move
	remember->son = remSon;           // Reset son pointer
	remember->marker = NULL;          // Forget marker
	
//	updateMove(tree->getCurrent(), !display_incoming_move);
	
//	board->setModified();
}


