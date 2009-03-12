/*
* tree.cpp
*/
#include "tree.h"
#include "move.h"
#include "matrix.h"

#include <iostream>
#include <vector>

#include <QtCore>

//Class Move;

Tree::Tree(int * board_size)
{
	root = NULL;
	checkPositionTags = NULL;
	init(board_size);
	
/* 
 * This initialisation is from stonehandler
 * It might not be needed
 */
	stones = new QHash<int,MatrixStone *>();//::QHash();
}

void Tree::init(int * board_size)
{
	if(root)
		clear();
	boardSize = board_size;
	root = new Move(*boardSize);
	// node index used for IGS review
	root->setNodeIndex(1);
	current = root;
	
	if(checkPositionTags)
		delete checkPositionTags;
	checkPositionTags = new Matrix(*boardSize);
}

Tree::~Tree()
{
	
	delete checkPositionTags;
	
	qDeleteAll(*stones);
	stones->clear();

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

/*
 * Traverse the tree and deletes all moves after the given move
 */
void Tree::traverseClear(Move *m)
{
	Q_CHECK_PTR(m);
	QStack<Move*> stack;
	QStack<Move*> trash;

	Move *t = NULL;
	
	//drop every node into stack trash	
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

/*
 * Find a move starting from the given in the argument in the main branch, or at marked move.
 */
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




/*
 * Find a move starting from the given in the argument in the 
 * all following branches.
 */
Move* Tree::findNode(Move *m, int node)
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
			if (t->getNodeIndex() == node)
				return t;
			stack.push(t->brother);
			stack.push(t->son);
		}
	}

	// node index not found
	return NULL;
}

/*
 * count all moves in the tree.
 */
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
void Tree::createMoveSGF( bool /*brother*/)
{
	// qDebug("BoardHandler::createMoveSGF() - %d", mode);
	
	Move *m;
	
	Matrix *mat = current->getMatrix();
	m = new Move(stoneBlack, -1, -1, current->getMoveNumber()+1, phaseOngoing, *mat);
	/*else	//fastload
	{
		m = new Move(stoneBlack, -1, -1, current->getMoveNumber()+1, phaseOngoing);
	}*/
#ifdef FIXME
	if (!brother && hasSon(m))
	{
		/* FIXME in loading that Kogo's joseki dictionary we get
		 * about 20 of these, probably sources to a sgfparser issue */
		/* Okay, as far as I know, this function is never called with
		 * "brother", so it just always does addSon.
		 * Then addSon will add a brother if it should be a brother.
		 * Obviously this needs clarification */
		qDebug("*** HAVE THIS SON ALREADY! ***");
		delete m;
		return;
	}
#endif //FIXME	
	//if (!fastLoad ) // mode is ALWAYS normal && mode == modeNormal)
	m->getMatrix()->clearAllMarks();
	
	/* Below removed since brother never used */
	//if (!brother)
		addSon(m);
	//else
	//	addBrother(m);

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


void Tree::updateCurrentMatrix(StoneColor c, int x, int y, GamePhase gamePhase)
{
	// Passing?
	if (x == 20 && y == 20)
		return;
	
	if ((x < 1) || (x > *boardSize) || (y < 1) || (y > *boardSize))
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
		current->getMatrix()->insertStone(x, y, c, gamePhase);
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
}

void Tree::editMove(StoneColor c, int x, int y)
{
	// qDebug("BoardHandler::editMove");
	
	if ((x < 1 || x > *boardSize || y < 1 || y > *boardSize) && x != 20 && y != 20)
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
 /* FIXME It looks like this function was originally just for SGF files? and now
  * its used for everything?  Anyway, I added dontplayyet so that moves could be
  * tested before being sent to server, but all the commented out code below
  * should probably just be cleaned out and the name should be changed.  If it
  * really has something still to do with the SGF stuff then that functionality
  * should be put somewhere else in a wrapper around this */
int Tree::addStoneSGF(StoneColor c, int x, int y, bool new_node, bool dontplayyet)
{
	static int koStoneX = 0;
	static int koStoneY = 0;
#ifdef DIFFERING_ADJACENT_PLAY_FIX
	static int dpyX = 0, dpyY = 0;
#endif //DIFFERING_ADJACENT_PLAY_FIX
	static Matrix * last_move_matrix = 0;
	static int last_captures;
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
	if ((x < 1 || x > *boardSize || y < 1 || y > *boardSize) && x != 20 && y != 20)
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
	
	/* FIXME This is weird to me.  The root is move 0, right?  So
	 * what is this parent == root nonsense??? And that "TODO"
	 * makes me think this is not the proper way. */
	if ((current->parent == root || current == root)&& ! new_node)
	{
		qDebug("setting that first weird move line %d", __LINE__);
		if(current->parent == root)
			qDebug("current parent root no new node %d %d", x, y);
		current->setMoveNumber(0); //TODO : make sure this is the proper way
		current->setX(-1);//-1
		current->setY(-1);//-1
		/* SGF submits handicap edit moves as root, they need to be
		 * given a color so that that color can flip over the cursor
		 * this is ugly, the cursor should be set by who's turn it
		 * is, by whatever means that's found, NOT by the color
		 * of the last move, to unify it.  Regardless, here's our
		 * solution.  Took me a while to find it because I assumed
		 * that the cursor and the move color we're linked when they're
		 * not.  Note also that this color is different from the
		 * individual colors of the matrix positions for this move */
		current->setColor(stoneBlack);
	}
	else if (new_node)
		editMove(c, x, y);

	bool koStone = 0;
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
	if(!new_node)	//for handicap stones and others?
		updateCurrentMatrix(c, x, y, phaseEdit);
	else if(!dontplayyet)
		updateCurrentMatrix(c, x, y);
	//qDebug("addStoneSGF: %d %d black: %d", x, y, c == stoneBlack);
	int captures;
	
	
	Matrix * checkPosition_matrix;
	
	if(dontplayyet)
	{
		Matrix *m = current->getMatrix();
		checkPosition_matrix = new Matrix(*m);
		checkPosition_matrix->insertStone(x, y, c, phaseOngoing);
#ifdef DIFFERING_ADJACENT_PLAY_FIX
		dpyX = x;
		dpyY = y;
#endif //DIFFERING_ADJACENT_PLAY_FIX
	}
	else if(!last_move_matrix)
		checkPosition_matrix = current->getMatrix();
#ifdef DIFFERING_ADJACENT_PLAY_FIX
	else if(last_move_matrix && x != dpyX || y ! = dpyY)
	{
		delete last_move_matrix;
		last_move_matrix = 0;
		checkPosition_matrix = current->getMatrix();
	}
#endif //DIFFERING_ADJACENT_PLAY_FIX
	/* Potential issue.  We don't want to call checkPosition twice
	 * since its intensive, so we check the legality of a move and
	 * then when it comes back from the server, we play it.  A
	 * possible issue is if a move was evaluated as legal and
	 * then the move to follow was somehow different.  Not sure
	 * when this would happen, but if it did, I don't think we
	 * would handle it properly.  
	 * This actually happens if we call setHandicap between
	 * the checking and playing of a stone.  The solution
	 * is really to make sure that setHandicap is only
	 * called once when it should be.  Unfortunately, its
	 * possible that with certain protocols, we'll need
	 * GAMERPROPS msg to get the handicap so we're just
	 * going to test here with dpyX and dpyY.  This is
	 * totally unnecessary overhead if setHandicap was
	 * set up properly so we should FIXME at some point.*/
	if(!last_move_matrix)
	{
		//check for ko
		if(koStone && x == koStoneX && y == koStoneY)
			captures = -1;
		else
			captures = checkPosition(s, checkPosition_matrix);
	}
	else
	{
		captures = last_captures;
		checkPosition_matrix = current->getMatrix();
		*checkPosition_matrix = *last_move_matrix;
		delete last_move_matrix;
		last_move_matrix = 0;
	}
	
	if (captures < 0)
	{
		if(dontplayyet)
		{
			delete checkPosition_matrix;
		}
		return -1;
	}
	if(dontplayyet)
	{
		/* We've ran check position and the move is okay, but
		 * we haven't gotten it back from the server yet, so we
		 * don't want to actually make the move. */
		last_captures = captures;
		last_move_matrix = checkPosition_matrix;
		return captures;
	}
	
	/* Add ko mark */
	if(captures == 1)
	{
		StoneColor opp = (c == stoneBlack ? stoneWhite : stoneBlack);
		Matrix * trix = current->getMatrix();
		StoneColor testcolor;
		int sides = 3;
		if(x < trix->getSize())
		{
			testcolor = trix->getStoneAt(x + 1, y);
			if(testcolor == opp)
				sides--;
			else if(testcolor == stoneNone)
			{
				koStoneX = x + 1;
				koStoneY = y;
			}
		}
		else
			sides--;
		
		if(x > 1)
		{
			testcolor = trix->getStoneAt(x - 1, y);
			if(testcolor == opp)
				sides--;
			else if(testcolor == stoneNone)
			{
				koStoneX = x - 1;
				koStoneY = y;
			}
		}
		else
			sides--;
		if(y < trix->getSize())
		{
			testcolor = trix->getStoneAt(x, y + 1);
			if(testcolor == opp)
				sides--;
			else if(testcolor == stoneNone)
			{
				koStoneX = x;
				koStoneY = y + 1;
			}
		}
		else
			sides--;
		
		if(y > 1)
		{
			testcolor = trix->getStoneAt(x, y - 1);
			if(testcolor == opp)
				sides--;
			else if(testcolor == stoneNone)
			{
				koStoneX = x;
				koStoneY = y - 1;
			}
		}
		else
			sides--;
		
		if(sides != 0)
		{
			koStoneX = 0;
			koStoneY = 0;
		}
		else
		{
			/* Don't save to file */
			if(preferences.draw_ko_marker)
				trix->insertMark(koStoneX, koStoneY, markKoMarker);
		}
	}
	
	

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


int Tree::checkPosition(MatrixStone * stone, Matrix * m)
{
	StoneColor c;
	StoneColor move_color = stone->c;
	StoneColor oppcolor = (move_color == stoneBlack ? stoneWhite : stoneBlack);
	int i, j;
	int board_size = m->getSize();
	
	//qDebug("Checking position of move: %d %d", stone->x, stone->y);
	i = stone->x;
	j = stone->y;

	/* If the adjXs aren't "zeroed" out, then they can coincidentally
	 * prevent the wrong vector from being marked as completed and
	 * cause a segmentation fault.  But then here this could still
	 * happen with move 255, 255, somehow... */
	unsigned short adjN = 0xffff, adjW = 0xffff, adjS = 0xffff, adjE = 0xffff;
	unsigned short stone_pos, move_pos;
	unsigned int init_adj = 0;
	int captures = 0;
	int list_size = 0;
	std::vector <unsigned short> suicidePrevention;
	/* NWSE could be wrong in the sense that the display may
	* be inverted from ordinals, but that's not
	* really a problem.*/
	std::vector <unsigned short> capN, capW, capS, capE;
	std::vector <unsigned short> * addlist;
	std::vector <unsigned short> checklist;
	int current_color;
	
	/* Clear the tag matrix */
	/* There's no particular reason to use a "Matrix" nor for "phaseOngoing",
	 * but the facility was there, any matrix would have sufficed though */
	/* Another random annoyance.  The "Matrix" code was created with
	 * inconsistent 0 basing.  Which means that when we use set and at with
	 * our custom matrix, we need to subtract 1 from x and y so that we
	 * can use the same variables with getStoneAt on the m matrix */
	checkPositionTags->clear();
#define NONE		0
#define SOUTH		1
#define EAST		2
#define WEST		3
#define NORTH		4
#define OURCOLOR	5
		
	checkPositionTags->set(i - 1, j - 1, OURCOLOR);
	
	if(i + 1 <= board_size)	//EAST
	{
		adjE = ((i + 1) << 8) + j;
		init_adj++;
		checklist.push_back(adjE);
	}
	else
		capE.push_back(0xffff);
	if(i - 1 > 0)		//WEST
	{
		adjW = ((i - 1) << 8) + j;
		init_adj++;
		checklist.push_back(adjW);
	}
	else
		capW.push_back(0xffff);
	if(j - 1 > 0)		//SOUTH
	{
		adjS = (i << 8) + j - 1;
		init_adj++;
		checklist.push_back(adjS);
	}
	else
		capS.push_back(0xffff);
	if(j + 1 <= board_size)	//NORTH
	{
		adjN = (i << 8) + j + 1;
		init_adj++;
		checklist.push_back(adjN);
	}
	else
		capN.push_back(0xffff);
	//checklist.push_back(i << 8 + j);
	move_pos = (i << 8) + j;
	suicidePrevention.push_back(move_pos);
	while(!checklist.empty())
	{
		unsigned short adj = checklist.back();
		checklist.pop_back();
		list_size--;
		i = adj >> 8;
		j = adj & 0x00ff;
		c = m->getStoneAt(i, j);
		if(checklist.size() < init_adj)	// just popped
		{
			init_adj--;
			if(c == move_color)
			{
				if(adj == adjE)
					capE.push_back(0xffff);
				else if(adj == adjW)
					capW.push_back(0xffff);
				else if(adj == adjS)
					capS.push_back(0xffff);
				else if(adj == adjN)
					capN.push_back(0xffff);
				if(suicidePrevention.back() == 0xffff)
					continue;
				addlist = &suicidePrevention;
				current_color = OURCOLOR;
			}
			else if(c == stoneNone)
			{
				// SP always has at least move so this is okay
				if(suicidePrevention.back() != 0xffff)
				{
					suicidePrevention.clear();
					suicidePrevention.push_back(0xffff);
				}
				if(adj == adjE)
					capE.push_back(0xffff);
				else if(adj == adjW)
					capW.push_back(0xffff);
				else if(adj == adjS)
					capS.push_back(0xffff);
				else if(adj == adjN)
					capN.push_back(0xffff);
				// no stone, no trace
				continue;
			}
			else if(adj == adjE)
			{
				if(!capE.empty())
					continue;
				addlist = &capE;
				capE.push_back(adj);
				current_color = EAST;
			}
			else if(adj == adjW)
			{
				if(!capW.empty())
					continue;
				addlist = &capW;
				capW.push_back(adj);
				current_color = WEST;
			}
			else if(adj == adjS)
			{
				if(!capS.empty())
					continue;
				addlist = &capS;
				capS.push_back(adj);
				current_color = SOUTH;
			}
			else if(adj == adjN)
			{
				/* Probably don't need a check for
				 * last list since it would take
				 * priority */
				//if(capN.back() == 0xffff)
				//	continue;
				addlist = &capN;
				capN.push_back(adj);
				current_color = NORTH;
			}
			list_size = 0;
			checkPositionTags->set(i - 1, j - 1, current_color);
		}
		else
		{
			stone_pos = (i << 8) + j;
			//if we come at the move from other
			//than the move, then we take over
			//that list
			//this should only clear the list
			//BEFORE we get there by the way
			//which probably means we should
			//verify this
			if(adjE == stone_pos)
			{
				capE.clear();
				capE.push_back(0xffff);
			}
			else if(adjW == stone_pos)
			{
				capW.clear();
				capW.push_back(0xffff);
			}
			/*else if(adjN == stone_pos)
			{
				//can't happen
				capN.clear();
				capN.push_back(0xffff);
			}*/
			else if(adjS == stone_pos)
			{
				capS.clear();
				capS.push_back(0xffff);
			}
		}
		
		if(i + 1 <= board_size)
		{
			stone_pos = ((i + 1) << 8) + j;
			c = m->getStoneAt(i + 1, j);
			if(checkPositionTags->at(i, j - 1) == NONE)
			{
				
				if(c == stoneNone)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
				else if((c == move_color && addlist == &suicidePrevention) ||
					(c == oppcolor && addlist != &suicidePrevention))
				{
					list_size++;
					addlist->push_back(stone_pos);
					checklist.push_back(stone_pos);
					checkPositionTags->set(i, j - 1, current_color);
				}	
			}
			else if(addlist != &suicidePrevention && checkPositionTags->at(i, j - 1) != current_color)
			{
				/* If we enter a colored list that did not
				 * supercede the list we're on now, then the
				 * currentlist is clear */
				if(c == oppcolor)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
			}
		}
		if(i - 1 > 0)
		{
			stone_pos = ((i - 1) << 8) + j;
			c = m->getStoneAt(i - 1, j);
			if(checkPositionTags->at(i - 2, j - 1) == NONE)
			{
				
				if(c == stoneNone)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
				else if((c == move_color && addlist == &suicidePrevention) ||
					(c == oppcolor && addlist != &suicidePrevention))
				{
					list_size++;
					addlist->push_back(stone_pos);
					checklist.push_back(stone_pos);
					checkPositionTags->set(i - 2, j - 1, current_color);
				}
					
			}
			else if(addlist != &suicidePrevention && checkPositionTags->at(i - 2, j - 1) != current_color)
			{
				if(c == oppcolor)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
			}
		}
		if(j - 1 > 0)
		{
			stone_pos = (i << 8) + j - 1;
			c = m->getStoneAt(i, j - 1);
			if(checkPositionTags->at(i - 1, j - 2) == NONE)
			{
				if(c == stoneNone)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
				else if((c == move_color && addlist == &suicidePrevention) ||
					(c == oppcolor && addlist != &suicidePrevention))
				{
					list_size++;
					addlist->push_back(stone_pos);
					checklist.push_back(stone_pos);
					checkPositionTags->set(i - 1, j - 2, current_color);
				}
			}
			else if(addlist != &suicidePrevention && checkPositionTags->at(i - 1, j - 2) != current_color)
			{
				if(c == oppcolor)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
			}	
		}
		if(j + 1 <= board_size)
		{
			stone_pos = (i << 8) + j + 1;
			c = m->getStoneAt(i, j + 1);
			if(checkPositionTags->at(i - 1, j) == NONE)
			{
				
				if(c == stoneNone)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
				else if((c == move_color && addlist == &suicidePrevention) ||
					(c == oppcolor && addlist != &suicidePrevention))
				{
					list_size++;
					addlist->push_back(stone_pos);
					checklist.push_back(stone_pos);
					checkPositionTags->set(i - 1, j, current_color);
				}	
			}
			else if(addlist != &suicidePrevention && checkPositionTags->at(i - 1, j) != current_color)
			{
				if(c == oppcolor)
				{
					for(int k = 0; k < list_size; k++)
						checklist.pop_back();
					addlist->clear();
					addlist->push_back(0xffff);
					continue;
				}
			}
		}
	}
	
	// check lists for captures and suicide and execute
	if((!capW.empty() && capW.back() == 0xffff) &&
		(!capE.empty() && capE.back() == 0xffff) &&
		(!capN.empty() && capN.back() == 0xffff) &&
		(!capS.empty() && capS.back() == 0xffff) &&
		suicidePrevention.back() != 0xffff)
	{
		// suicide
		qDebug("no suicide");
		removeStone(stone->x, stone->y, false);
		return -1;
	}
	else
	{
		/*
		// kos handled in addStoneSGF caller
		if(koStone)
		{
			int sum = 0;
			if(capW.back() != 0xffff)
				sum += capW.size();
			if(capS.back() != 0xffff)
				sum += capS.size();
			if(capN.back() != 0xffff)
				sum += capN.size();
			if(capE.back() != 0xffff)
				sum += capE.size();
			if(sum == 1)
			{
				qDebug("Cannot take back the ko yet");
				removeStone(i, j, false);
				return -1;
			}
		}
		*/
		if(capW.back() != 0xffff)
		{
			while(!capW.empty())
			{
				stone_pos = capW.back();
				capW.pop_back();
				m->removeStone(stone_pos >> 8, stone_pos & 0x00ff);
				captures++;
			}
		}
		if(capE.back() != 0xffff)
		{
			while(!capE.empty())
			{
				stone_pos = capE.back();
				capE.pop_back();
				m->removeStone(stone_pos >> 8, stone_pos & 0x00ff);
				captures++;
			}
		}
		if(capN.back() != 0xffff)
		{
			while(!capN.empty())
			{
				stone_pos = capN.back();
				capN.pop_back();
				m->removeStone(stone_pos >> 8, stone_pos & 0x00ff);
				captures++;
			}
		}
		if(capS.back() != 0xffff)
		{
			while(!capS.empty())
			{
				stone_pos = capS.back();
				capS.pop_back();
				m->removeStone(stone_pos >> 8, stone_pos & 0x00ff);
				captures++;
			}
		}
	}
	//qDebug("%d captures", captures);
	return captures;
}	

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
