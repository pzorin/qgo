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


#include "tree.h"
#include "move.h"
#include "matrix.h"
#include "group.h"

#include <iostream>
#include <vector>

#include <QtCore>

Tree::Tree(int * board_size)
{
	root = NULL;
	checkPositionTags = NULL;
	groupMatrixView = NULL;
	groupMatrixCurrent = NULL;
	init(board_size);
	lastValidMoveChecked = NULL;
	loadingSGF = false;
	koStoneX = 0;
	koStoneY = 0;
	lastCaptures = 0;
	insertStoneFlag = false;
	
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
	lastMoveInMainBranch = current = root;
	
	if(checkPositionTags)
		delete checkPositionTags;
	checkPositionTags = new Matrix(*boardSize);

	deleteGroupMatrices();		//just in case

	int i, j;
	groupMatrixView = new Group** [*boardSize];
	for(i = 0; i < *boardSize; i++)
	{
		groupMatrixView[i] = new Group* [*boardSize];
		for(j = 0; j < *boardSize; j++)
			groupMatrixView[i][j] = NULL;
	}
	groupMatrixCurrent = new Group** [*boardSize];
	for(i = 0; i < *boardSize; i++)
	{
		groupMatrixCurrent[i] = new Group* [*boardSize];
		for(j = 0; j < *boardSize; j++)
			groupMatrixCurrent[i][j] = NULL;
	}
}

Tree::~Tree()
{
	
	delete checkPositionTags;
	deleteGroupMatrices();
	
	qDeleteAll(*stones);
	stones->clear();

	clear();
}

void Tree::deleteGroupMatrices(void)
{
	if(!groupMatrixView)
		return;
	for(int i = 0; i < *boardSize; i++)
	{
		delete[] groupMatrixView[i];
		delete[] groupMatrixCurrent[i];
	}
	delete[] groupMatrixView;
	delete[] groupMatrixCurrent;
}

/*
 *  Adds a brother at the end of the brother chain of current()
 */
// may be kind of silly if we move it to the Move class FIXME
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
		
		current->addBrother(node);
	}
	
	current = node;
	
	return true;
}

/*
 * Returns: true  - A son found, added as brother
 *          false - No son found, added as first son
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
			if(current == node)
			{
				qDebug("Attempting to add move as its own son!");
				return false;
			}
			current->son = node;
			
			node->parent = current;
			node->setTimeinfo(false);
			assignCurrent(current, node);
			if(isInMainBranch(current->parent))
				lastMoveInMainBranch = current;
#ifdef NOTWORKING
			// This is from an undo, but its awkward here
			// presumably son would not have been null with a marker
			// if this wasn't from undos or online review or the like
			// we'll understand this better if we look into online
			// review code FIXME
			if(current->parent->marker)
			{	
				if(!addBrother(current->parent->marker))
				{
					qFatal("Failed to add undo brother.");
					return false;
				}
				else
				{
					Move * m = current->parent->marker;
					current->parent->marker = NULL;
					//check for multiple undos in a row
					while(m->marker && m->son == NULL)	//is son check right?
					{
						m->son = m->marker;
						m->marker = NULL;
						m = m->son;
					}
				}
			}
#endif //NOTWORKING
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
			if(!loadingSGF)		//since this would put every branch on the last brother
				current->parent->marker = node;
			
			assignCurrent(current, node);
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
		assignCurrent(current, current->son);
	else
		assignCurrent(current, current->marker);  // Marker set, use this to go the remembered path in the tree
	
	current->parent->marker = current;  // Parents remembers this move we went to
	//current->getMatrix()->invalidateAdjacentGroups(, groupMatrixView);
	//invalidateAdjacentCheckPositionGroups(MatrixStone(current->getX(), current->getY(), current->getColor()));
	return current;
}

Move* Tree::previousMove()
{
	if (root == NULL || current == NULL || current->parent == NULL)
		return NULL;
	
	current->parent->marker = current;  // Remember the son we came from
	assignCurrent(current, current->parent);
	//current->getMatrix()->invalidateAdjacentGroups(MatrixStone(current->getX(), current->getY(), current->getColor()), groupMatrixView);
	//current->getMatrix()->invalidateAllGroups(groupMatrixView);
	//invalidateAdjacentCheckPositionGroups(MatrixStone(current->getX(), current->getY(), current->getColor()));
	return current;
}

Move* Tree::nextVariation()
{
	if (root == NULL || current == NULL || current->brother == NULL)
		return NULL;
	
	assignCurrent(current, current->brother);
	//current->getMatrix()->invalidateAdjacentGroups(MatrixStone(current->getX(), current->getY(), current->getColor()), groupMatrixView);
	//current->getMatrix()->invalidateAllGroups(groupMatrixView);
	//invalidateAdjacentCheckPositionGroups(MatrixStone(current->getX(), current->getY(), current->getColor()));
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
		{
			//current->getMatrix()->invalidateAdjacentGroups(MatrixStone(current->getX(), current->getY(), current->getColor()), groupMatrixView);
			//current->getMatrix()->invalidateAllGroups(groupMatrixView);
			//invalidateAdjacentCheckPositionGroups(MatrixStone(current->getX(), current->getY(), current->getColor()));
			return assignCurrent(current, old);
		}
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
			assignCurrent(current, tmp);
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
 * Traverse the tree and deletes all moves after the given move (including the move)
 */
void Tree::traverseClear(Move *m)
{
	Q_CHECK_PTR(m);
	QStack<Move*> stack;
	QStack<Move*> trash;

	Move *t = NULL;

	if(isInMainBranch(m))
		lastMoveInMainBranch = m->parent;
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

/* This is slower than it could be, but traverseClear I think is seldom called
 * we also use this in addSon though... */
bool Tree::isInMainBranch(Move * m) const
{
	while(m->parent && m->parent->son == m)
		m = m->parent;
	if(m == root)
		return true;
	else
		return false;
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

void Tree::setCurrent(Move *m)
{
	if(m == lastMoveInMainBranch)
		current = m;
	else
	{
		assignCurrent(current, m);
		invalidateCheckPositionGroups();
	}
}

void Tree::setToFirstMove()
{
	if (root == NULL)
		qFatal("Error: No root!");
	assignCurrent(current, root);
	invalidateCheckPositionGroups();
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
#ifdef OLD
	Move *m = root;
	Q_CHECK_PTR(m);

	if (m==NULL)
	return NULL;
  
	// Descend tree until root reached
	while (m->son != NULL)
		m = m->son;

	return m;
#endif //OLD
	return lastMoveInMainBranch;
}

Move *Tree::findLastMoveInCurrentBranch()
{
	Move *m = getCurrent();
	Q_CHECK_PTR(m);

	if (m==NULL)
	return NULL;
  
	while (m->son != NULL)
	{
		if(m->marker)
			m = m->marker;
		else
			m = m->son;
	}
	return m;
}

void Tree::addEmptyMove( bool /*brother*/)
{
	// qDebug("BoardHandler::createMoveSGF() - %d", mode);
	
	Move *m;
	
	Matrix *mat = current->getMatrix();
	m = new Move(stoneBlack, -1, -1, current->getMoveNumber()+1, phaseOngoing, *mat, true);
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
	
	/* Below removed since brother never used */
	//if (!brother)
		addSon(m);
	//else
	//	addBrother(m);

}

/* Really conflicting with setCurrent name, this is like a joke FIXME */
Move * Tree::assignCurrent(Move * & o, Move * & n) 
{
	/*if(n->getX() == -1 || n->getY() == -1)		//root at least is -1 -1 so who cares? but this is probably from empty nodes FIXME cleanup somehow
		qDebug("new move -1 -1");
	if(o->getX() == -1 || o->getY() == -1)
		qDebug("old move -1 -1");*/
	if(n != root)  //not an issue anymore
	{
		if(n == o->son)
		{
			/* This may or may not be silly.  We call it from addSon which is ridiculous because we just ran
			 * checkPosition.  So either we shouldn't call assignCurrent from addSon but then we have an issue
			 * with the other two, or we need to have some kind of flag or check or maybe see if this new stone
			 * has sons, etc. FIXME */
			/* Also note that the way its currently done, lastMoveInMainBranch isn't set to n until AFTER
			 * its checked for here FIXME */
			if(n == findLastMoveInMainBranch())
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(n->getX(), n->getY(), n->getColor()), groupMatrixCurrent);
			else
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(n->getX(), n->getY(), n->getColor()), groupMatrixView);
		}
		else if(o == n->son)
		{
			if(n == findLastMoveInMainBranch())
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(o->getX(), o->getY(), o->getColor()), groupMatrixCurrent);
			else
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(o->getX(), o->getY(), o->getColor()), groupMatrixView);
			lastCaptures = getLastCaptures(n);
		}
		else if(o->parent == n->parent)
		{
			if(n == findLastMoveInMainBranch())
			{
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(o->getX(), o->getY(), o->getColor()), groupMatrixCurrent);
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(n->getX(), n->getY(), n->getColor()), groupMatrixCurrent);
			}
			else
			{
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(o->getX(), o->getY(), o->getColor()), groupMatrixView);
				n->getMatrix()->invalidateAdjacentGroups(MatrixStone(n->getX(), n->getY(), n->getColor()), groupMatrixView);
			}
		}
		else
		{
			if(n == findLastMoveInMainBranch())
				n->getMatrix()->invalidateChangedGroups(*o->getMatrix(), groupMatrixCurrent);
			else
				n->getMatrix()->invalidateChangedGroups(*o->getMatrix(), groupMatrixView);
		}
	}
	
	n->getMatrix()->markChangesDirty(*o->getMatrix());
	o = n;
	return o;
}

void Tree::invalidateAdjacentCheckPositionGroups(MatrixStone m)
{
	if(current == root)
		return;
	if(current == findLastMoveInMainBranch())
		current->getMatrix()->invalidateAdjacentGroups(m, groupMatrixCurrent);
	else
		current->getMatrix()->invalidateAdjacentGroups(m, groupMatrixView);
}

void Tree::invalidateCheckPositionGroups(void)
{
	if(current == root)
		return;
	if(current == findLastMoveInMainBranch())
		current->getMatrix()->invalidateAllGroups(groupMatrixCurrent);
	else
		current->getMatrix()->invalidateAllGroups(groupMatrixView);
}

/* These two functions are a little awkward here, just done to minimize
 * issues with qgoboard edit FIXME */
void Tree::updateCurrentMatrix(StoneColor c, int x, int y)
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
	if(current->getGamePhase() == phaseEdit)
		current->getMatrix()->insertStone(x, y, c, true);
	else
		current->getMatrix()->insertStone(x, y, c);
}

/* FIXME double check and remove editMove, unnecessary */
void Tree::doPass(bool /*sgf*/, bool /*fastLoad*/)
{
//	if (lastValidMove == NULL)
//	{
//		stoneHandler->checkAllPositions();
//	}

/////////////////FIXME We may have a problem at first move with handicap games ...
	StoneColor c = (current->getColor() == stoneWhite) ? stoneBlack : stoneWhite;
	
//	currentMove++;
	addMove(c, 20, 20);
#ifdef OLD
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
#endif //OLD
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

bool Tree::checkMoveIsValid(StoneColor c, int x, int y)
{
	MatrixStone *s = new MatrixStone(x, y, c);
	lastCaptures = 0;
	lastValidMoveChecked = NULL;

	if ((x < 1 || x > *boardSize || y < 1 || y > *boardSize) && x != 20 && y != 20)		//because 20,20 is pass, but ugly/unnecessary here FIXME
	{
		qWarning("Invalid position: %d/%d", x, y);
		return false;
	}

	/* special case, we're erasing a stone */
	if(c == stoneErase)
	{
		if(current->getMatrix()->getStoneAt(x,y) == stoneNone)
		{
			qDebug("Trying to erase a stone that doesn't exist: %d %d", x, y);
			return false;
		}
		/* FIXME I don't think this is right.  First of all, it needs to be fixed in deleteNode() as well which is called as an undo
		 * as well as as a tree edit.  But here, I don't think we want a new move added as a son to erase a move, I think we just
		 * want to erase it... or maybe not because it makes sense to add it as a tree when its an undo in a game.
		 * another point is that its not like a move in order, its like a separate tree, so addMove(stoneErase seems singularly
		 * useless since we have to do something special with the tree anyway, i.e., no addSon, but maybe we go backwards and
		 * delete some marker */
		lastValidMoveChecked = new Move(c, x, y, current->getMoveNumber() + 1, phaseOngoing, *(current->getMatrix()), true);	//clearMarks = true
		checkPosition(s, lastValidMoveChecked->getMatrix());
		return true;
	}

	/* I'm not sure what "stoneErase" is for.  If we find nothing uses it, we should remove it FIXME */
	if (current->getMatrix()->getStoneAt(x,y) != stoneNone &&
		current->getMatrix()->getStoneAt(x,y) != stoneErase)
	{
		if(current->getMatrix()->getStoneAt(x, y) == stoneBlack)
			qDebug("black stone");
		else if(current->getMatrix()->getStoneAt(x, y) == stoneWhite)
			qDebug("white stone");
		else
			qDebug("question");
		qDebug ("We seem to have already a stone at this place : %d %d", x, y);
		return false;
	}

	bool koStone = 0;
	if (current->getMoveNumber() > 1)
		koStone = (current->parent->getMatrix()->at(x-1, y-1) == c);

	//check for ko
	if(koStone && x == koStoneX && y == koStoneY)
		return false;

	lastValidMoveChecked = new Move(c, x, y, current->getMoveNumber() + 1, phaseOngoing, *(current->getMatrix()), true);	//clearMarks = true
	if((lastCaptures = checkPosition(s, lastValidMoveChecked->getMatrix())) < 0)
	{
		delete lastValidMoveChecked;
		lastValidMoveChecked = NULL;
		return false;
	}

	return true;
}

void Tree::addMove(StoneColor c, int x, int y)
{
	/* special case: pass */
	if(x == 20 && y == 20)
	{
		Matrix *mat = current->getMatrix();
		Q_CHECK_PTR(mat);
	 
		Move *m = new Move(c, x, y, current->getMoveNumber() +1 , phaseOngoing, *mat, true);
		Q_CHECK_PTR(m);
	
		if (hasSon(m))
		{
			// qDebug("*** HAVE THIS SON ALREADY! ***");
			delete m;
			return;
		}
		addSon(m);
		return;
	}

	if(checkMoveIsValid(c, x, y))
		addStoneOrLastValidMove();
}

void Tree::addStone(StoneColor c, int x, int y)
{
	addStoneOrLastValidMove(c, x, y);
}

void Tree::addStoneOrLastValidMove(StoneColor c, int x, int y)
{
	if (insertStoneFlag)
	{
		c = lastValidMoveChecked->getColor();
		x = lastValidMoveChecked->getX();
		y = lastValidMoveChecked->getY();

		Matrix *mat = current->getMatrix();
		Q_CHECK_PTR(mat);
		//do insert if game mode is OK
		Move *m = new Move (c, x, y, current->getMoveNumber() +1 , phaseOngoing, *mat, true);
		Q_CHECK_PTR(m);
		insertStone(m);
		lastValidMoveChecked = NULL;
		return;
	}

	koStoneX = 0;
	koStoneY = 0;
	if(lastValidMoveChecked)
	{
		c = lastValidMoveChecked->getColor();
		x = lastValidMoveChecked->getX();
		y = lastValidMoveChecked->getY();
	
		if (hasSon(lastValidMoveChecked))
		{
			/* This happens a lot if we add a move that's already there
			 * when really we're just playing along with the tree. 
			 * This would be the place to add any kind of tesuji testing
			 * code */
			delete lastValidMoveChecked;
		}
		else
		{
			addSon(lastValidMoveChecked);
			checkAddKoMark(current->getColor(), current->getX(), current->getY(), current);
		}
	}
	else if(c != stoneNone)
	{
		Matrix *mat = current->getMatrix();
		Q_CHECK_PTR(mat);
	 
		Move *m = new Move(c, x, y, current->getMoveNumber() +1 , phaseOngoing, *mat, true);
		Q_CHECK_PTR(m);
		if (hasSon(m))
		{
			// qDebug("*** HAVE THIS SON ALREADY! ***");
			delete m;
		}
		else
		{
			addSon(m);
			checkAddKoMark(current->getColor(), current->getX(), current->getY(), current);
			/* FIXME updateMatrix calls should be concealed within new Move.  updateCurrentMatrix should also not be called from
			 * qgoboard. Except, now we call "insertStone" in the middle of checkStoneWithGroups. I think right now I'll just
 			 * try to simplify the tree code and then later worry about making it cleaner and clearer. */
			updateCurrentMatrix(c, x, y);
		}
	}
	else
		return;

	lastValidMoveChecked = NULL;

	int capturesBlack, capturesWhite;
	if (current->parent != NULL)
	{
		capturesBlack = current->parent->getCapturesBlack();
		capturesWhite = current->parent->getCapturesWhite();
	}
	else
		capturesBlack = capturesWhite = 0;	

	if (c == stoneBlack)
		capturesBlack += lastCaptures;
	else if (c == stoneWhite)
		capturesWhite += lastCaptures;
	lastCaptures = 0;
	current->setCaptures(capturesBlack, capturesWhite);
}

void Tree::addStoneToCurrentMove(StoneColor c, int x, int y)
{
	if (current->parent == root || current == root)
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
	updateCurrentMatrix(c, x, y);
	//editMove(c, x, y);
}

/*
 *	Returns: true  - son of current move was found
 *           false - 
 */
bool Tree::insertStone(Move *node)
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
			if(current == node)
			{
				qDebug("Attempting to add move as its own son!");
				return false;
			}

			current->son = node;
			node->parent = current;
			node->setTimeinfo(false);
			assignCurrent(current, node);
			node->getMatrix()->insertStone(node->getX(), node->getY(), node->getColor());
			
			return false;
		}
		// A son found
		else
		{
			
			//we insert node between current and current->son
			node->parent = current;
			node->son = current->son;
			current->son->parent = node;
			current->son = node;

			//update all brothers to enable switching between them
			Move *t = node->son->brother;
			while (t != NULL) 
			{
				t->parent = node;
				t = t->brother;
			}
			current->parent->marker = current;
			node->marker = NULL;

			node->setTimeinfo(false);
			assignCurrent(current, node);
			node->getMatrix()->insertStone(node->getX(), node->getY(), node->getColor());

			//update son - it is exclude from traverse search because we cannot update brothers of node->son
			node->son->setMoveNumber(node->son->getMoveNumber()+1);
			node->son->getMatrix()->insertStone(node->getX(), node->getY(), node->getColor());

			if (node->son->son != NULL)
			{
				// Traverse the tree and update every node (matrix and moveNum)
				QStack<Move*> stack;
				Move *t = NULL;
				stack.push(node->son->son);

				while (!stack.isEmpty())
				{
					t = stack.pop();
					if (t != NULL)
					{
						if (t->brother != NULL)
							stack.push(t->brother);
						if (t->son != NULL)
							stack.push(t->son);
						t->setMoveNumber(t->getMoveNumber()+1);
						t->getMatrix()->insertStone(node->getX(), node->getY(), node->getColor());
					}
				}
			}
			return true;
		}
	}
}

void Tree::undoMove(void)
{
#ifdef NOTWORKING
	previousMove();
	
	current->marker = current->son;
	current->son = NULL;
#endif //NOTWORKING
	deleteNode();
}

void Tree::checkAddKoMark(StoneColor c, int x, int y, Move * m)
{
	if(x == 20 && y == 20)	//awkward passing check FIXME
	{
		koStoneX = 0; koStoneY = 0; 	//necessary?? FIXME
		return;
	}
	if(!m)
		m = current;
	if(lastCaptures == 1)
	{
		StoneColor opp = (c == stoneBlack ? stoneWhite : stoneBlack);
		Matrix * trix = m->getMatrix();
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
}

int Tree::getLastCaptures(Move * m)
{
	if(!m->parent)
		return 0;
	if(m->getColor() == stoneWhite)
		return m->getCapturesWhite() - m->parent->getCapturesWhite();
	else if(m->getColor() == stoneBlack)
		return m->getCapturesBlack() - m->parent->getCapturesBlack();
	else
		return 0;
}

int Tree::checkPosition(MatrixStone * stone, Matrix * m)
{
	Group *** gm;
	Group * joins[4];
	Group * enemyGroups[4];
	Group * newgroup;
	int capturedStones;
	int i, j;

	if(current == findLastMoveInMainBranch())
		gm = groupMatrixCurrent;
	else
		gm = groupMatrixView;
#ifdef CHECKPOSITION_DEBUG
	qDebug("Checking position on %p", gm);
#endif //CHECKPOSITION_DEBUG
	if(stone->c == stoneErase)
	{
		m->removeStoneFromGroups(stone, gm);
		return 0;
	}
	newgroup = m->checkStoneWithGroups(stone, gm, joins, enemyGroups);
	if(newgroup->liberties == 0 && 
		(!enemyGroups[0] || enemyGroups[0]->liberties != 1) && 
		(!enemyGroups[1] || enemyGroups[1]->liberties != 1) && 
		(!enemyGroups[2] || enemyGroups[2]->liberties != 1) && 
		(!enemyGroups[3] || enemyGroups[3]->liberties != 1))
	{
		//reset joined groups
		for(i = 0; i < 4; i++)
		{
			if(!joins[i])
				continue;
			for(j = 0; j < joins[i]->count(); j++)
				gm[joins[i]->at(j)->x - 1][joins[i]->at(j)->y - 1] = joins[i];
		}
		gm[stone->x-1][stone->y-1] = NULL;
		m->insertStone(stone->x, stone->y, stoneNone);
		delete newgroup;
		return -1;	//suicide
	}
	else
	{
		for(i = 0; i < 4; i++)
		{
			if(!joins[i])
				continue;
			for(j = 0; j < joins[i]->count(); j++)
				gm[joins[i]->at(j)->x - 1][joins[i]->at(j)->y - 1] = newgroup;
			delete joins[i];
		}
		capturedStones = 0;
		for(i = 0; i < 4; i++)
		{
			if(enemyGroups[i])
			{
				if(enemyGroups[i]->liberties == 1)
				{
					capturedStones += enemyGroups[i]->count();
					for(j = i + 1; j < 4; j++)
					{
						if(enemyGroups[j] == enemyGroups[i])
							enemyGroups[j] = NULL;
					}
					m->removeGroup(enemyGroups[i], gm, newgroup);
				}
				else
				{
					for(j = i + 1; j < 4; j++)
					{
						if(enemyGroups[j] == enemyGroups[i])
							enemyGroups[j] = NULL;
					}
					enemyGroups[i]->liberties--;
				}
			}
		}
		return capturedStones;
	}
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
		MatrixStone * s = NULL;
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

/* Below is only used by removeStone and is similar to other code within removeStone FIXME */
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
/* FIXME this should certainly invalidate something to move us to the previous matrix or the like.
 * and its called straight out of the network code so... basically we need to think through
 * how we want to do the two different views and how to transition between them, almost shouldn't
 * be an issue, like its the current view matrix that changes, and then we switch to the current
 * as we scroll... but I want to write the invalidate that works locally... */
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
	if(isInMainBranch(m))
		lastMoveInMainBranch = remember;
	if (m->son != NULL)
		traverseClear(m->son);  // Traverse the tree after our move (to avoid brothers)
	assignCurrent(current, remember);
	delete m;                         // Delete our move
	//setCurrent(remember);       // Set current move to previous move
	remember->son = remSon;           // Reset son pointer, NULL
	remember->marker = NULL;          // Forget marker
	
	/*if(current == findLastMoveInMainBranch())
		current->getMatrix()->invalidateAllGroups(groupMatrixCurrent);
	else
		current->getMatrix()->invalidateAllGroups(groupMatrixView);*/
//	updateMove(tree->getCurrent(), !display_incoming_move);
	
//	board->setModified();
}
