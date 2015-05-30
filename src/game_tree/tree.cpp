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
#include "messages.h"
#include "sgfparser.h"
#include "gamedata.h"

#include <vector>

#include <QtCore>

Tree::Tree(int board_size, float komi)
    : boardSize(board_size), root(NULL), komi(komi)
{
    checkPositionTags = NULL;
    init();
    loadingSGF = false;
}

void Tree::init()
{
	if(root)
		clear();
    root = new Move(boardSize);
	// node index used for IGS review
	root->setNodeIndex(1);
	lastMoveInMainBranch = current = root;
	
	if(checkPositionTags)
		delete checkPositionTags;
    checkPositionTags = new Matrix(boardSize);
}

Tree::~Tree()
{
	delete checkPositionTags;
    clear();
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
        current = node;
        if(isInMainBranch(current->parent))
            lastMoveInMainBranch = current;
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

        current = node;
        return true;
    }

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

Move* Tree::nextVariation()
{
	if (root == NULL || current == NULL || current->brother == NULL)
		return NULL;
	
    setCurrent(current->brother);
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
            current = old;
            return current;
		}
		old = tmp;
	}
	
	return NULL;
}

bool Tree::hasSon(Move *m)
{
    if (root == NULL || m == NULL || current == NULL)
		return false;
    return (current->hasSon(m->getColor(), m->getX(), m->getY()) != NULL);
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
    current = m;
    emit currentMoveChanged(current);
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

/* These two functions are a little awkward here, just done to minimize
 * issues with qgoboard edit FIXME */
void Tree::updateCurrentMatrix(StoneColor c, int x, int y)
{
	// Passing?
    if (x == PASS_XY && y == PASS_XY)
		return;
	
    if ((x < 1) || (x > boardSize) || (y < 1) || (y > boardSize))
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
/////////////////FIXME We may have a problem at first move with handicap games ...
	StoneColor c = (current->getColor() == stoneWhite) ? stoneBlack : stoneWhite;
    Move *result = current->makeMove(c, PASS_XY, PASS_XY);
    if (result)
        setCurrent(result);
}

void Tree::addStoneToCurrentMove(StoneColor c, int x, int y)
{
    if (current == root)
    {
        current->setHandicapMove(true);
		current->setMoveNumber(0); //TODO : make sure this is the proper way
        current->setX(-1);
        current->setY(-1);
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
            current = node;
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
            current = node;
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
        init();
		return;
	}
	if(isInMainBranch(m))
		lastMoveInMainBranch = remember;
	if (m->son != NULL)
		traverseClear(m->son);  // Traverse the tree after our move (to avoid brothers)
	delete m;                         // Delete our move
    remember->son = remSon;           // Reset son pointer, NULL
	remember->marker = NULL;          // Forget marker
    setCurrent(remember);       // Set current move to previous move
}


/*
 * Former BoardHandler slots
 */
void Tree::slotNavForward()
{
    if (root == NULL || current == NULL || current->son == NULL)
        return;

    if (current->marker == NULL)  // No marker, simply take the main son
        setCurrent(current->son);
    else
        setCurrent(current->marker);  // Marker set, use this to go the remembered path in the tree

    current->parent->marker = current;  // Parents remembers this move we went to
}

void Tree::slotNavBackward()
{
    if (root == NULL || current == NULL || current->parent == NULL)
        return;

    current->parent->marker = current;  // Remember the son we came from
    setCurrent(current->parent);
}

void Tree::slotNavFirst()
{
    if (root == NULL)
        qFatal("Error: No root!");
    setCurrent(root);
}

void Tree::slotNavLast()
{
    setCurrent(findLastMoveInCurrentBranch());
}

void Tree::slotNavPrevComment()
{
    Move  *m = current->parent;

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
        setCurrent(m);
    }
}

void Tree::slotNavNextComment()
{
    Move  *m = current->son;

    while (m != NULL)
    {
        if (m->getComment() != "")
            break;
        if (m->son == NULL)
            break;
        m = m->son;
    }

    if (m != NULL)
    {
        setCurrent(m);
    }
}

void Tree::slotNthMove(int n)
{
    if (n < 0)
        return;

    Move *m = current,
        *old = m;

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
        setCurrent(m);
}

void Tree::slotNavNextVar()
{
    nextVariation();
}

void Tree::slotNavPrevVar()
{
    Move *m = previousVariation();
    if (m != NULL)
        setCurrent(m);
}

void Tree::slotNavStartVar()
{
    Move *m = current->parent;
    while ((m != NULL) && (m->getNumSons() <= 1))
        m = m->parent;
    setCurrent(m ? m : root);
}

void Tree::slotNavNextBranch()
{
    Move *m = current;
    if (m == NULL)
        return;

    // Descend to the next branching point
    while (m->getNumSons() == 1)
        m = m->son;

    if (m->son != NULL)
        setCurrent(m->son);
}

/*
 * This function resumes back to the first move in the main branch
 */
void Tree::slotNavMainBranch()
{
    if (current->parent == NULL)
        return;

    Move *m = current,
        *old = m,
        *lastOddNode = NULL;

    if (m == NULL)
        return;

    while ((m = m->parent) != NULL)
    {
        if (getNumSons(m) > 1 && old != m->son)
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

    setCurrent(lastOddNode);
}

/*
 * Called after the preceding (slot nav Intersection)
 * When the intersection 'x/y' has been clicked on
 */
void Tree::findMoveByPos(int x, int y)
{
    Move *m = findMoveInMainBranch(x, y);
    if(!m)
        findMoveInCurrentBranch(x, y);

    if (m != NULL)
    {
        setCurrent(m);
    }
}

/*
 * Called by qgoboard when score button is pressed up, leaving score mode
 */
void Tree::exitScore()
{
    // Remove territory marks
    if (current->isTerritoryMarked())
    {
        current->getMatrix()->clearTerritoryMarks();
        current->setTerritoryMarked(false);
        current->setScored(false);
    }

    current->getMatrix()->absMatrix();
    emit currentMoveChanged(current);
}

bool Tree::importSGFFile(QString filename)
{
    SGFParser *sgfParser = new SGFParser(this);
    QString SGFLoaded = sgfParser->loadFile(filename);
    return sgfParser->doParse(SGFLoaded);
}

bool Tree::importSGFString(QString SGF)
{
    SGFParser *sgfParser = new SGFParser(this);
    return sgfParser->doParse(SGF);
}

QString Tree::exportSGFString(GameData *gameData)
{
    QString sgf = "";
    SGFParser *p = new SGFParser(this);
    p->exportSGFtoClipB(&sgf, this, gameData);
    delete p;
    return sgf;
}

/*
 * Performs all operations on the matrix of current move to display score marks
 * and score informaton on the uI
 */
void Tree::countScore(void)
{
    Matrix * current_matrix = current->getMatrix();

    capturesBlack = current->getCapturesBlack();
    capturesWhite = current->getCapturesWhite();
    current_matrix->markTerritory();
    current_matrix->count(terrBlack,terrWhite,deadBlack,deadWhite);
    current->setTerritoryMarked(true);
    current->setScored(true);

    emit currentMoveChanged(current); // Toggles window refresh
    emit scoreChanged(terrBlack, capturesBlack, deadWhite,
                      terrWhite, capturesWhite, deadBlack);
}

void Tree::countMarked(void)
{
    Matrix * current_matrix = current->getMatrix();

    deadWhite = 0;
    deadBlack = 0;
    terrWhite = 0;
    terrBlack = 0;
    capturesBlack = current->getCapturesBlack();
    capturesWhite = current->getCapturesWhite();

    for (int i=1; i<=boardSize; i++)
        for (int j=1; j<=boardSize; j++)
        {

            /* When called from network code, we're just using
             * the board as server has reported it.  No stones
             * are marked as dead, but apparently ones marked as
             * territory get ghosted out */
            if(current_matrix->getMarkAt(i, j) == markTerrBlack)
            {
                terrBlack++;
                if (current_matrix->getStoneAt(i, j) == stoneWhite)
                    deadWhite++;
            }
            else if(current_matrix->getMarkAt(i, j) == markTerrWhite)
            {
                terrWhite++;
                if (current_matrix->getStoneAt(i, j) == stoneBlack)
                    deadBlack++;
            }
        }
    emit scoreChanged(terrBlack, capturesBlack, deadWhite,
                      terrWhite, capturesWhite, deadBlack);
}

/* Not totally confident that this belongs here, but
 * the score is counted here */
GameResult Tree::retrieveScore(void)
{
    GameResult g;
    g.result = GameResult::SCORE;
    /* What about different scoring types? (chinese versus japanese)
     * FIXME This basically confirms for me that this does not
     * belong here */

    float blackScore = terrBlack + capturesBlack + deadWhite;
    float whiteScore = terrWhite + capturesWhite + deadBlack + komi;
    if(whiteScore > blackScore)
    {
        g.winner_color = stoneWhite;
        g.winner_score = whiteScore;
        g.loser_score = blackScore;
    }
    else
    {
        g.winner_color = stoneBlack;
        g.winner_score = blackScore;
        g.loser_score = whiteScore;
    }
    return g;
}
