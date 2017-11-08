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
        delete root;
    lastMoveInMainBranch = current = root = new Move(boardSize);
	// node index used for IGS review
	root->setNodeIndex(1);
	
	if(checkPositionTags)
		delete checkPositionTags;
    checkPositionTags = new Matrix(boardSize);
}

Tree::~Tree()
{
	delete checkPositionTags;
    if (root)
        delete root;
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
        if(current->isInMainBranch())
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

bool Tree::hasSon(Move *m)
{
    if (root == NULL || m == NULL || current == NULL)
		return false;
    return (current->hasSon(m->getColor(), m->getX(), m->getY()) != NULL);
}

/*
 * Find a move starting from the given in the argument in the main branch, or at marked move.
 */
Move* Tree::findMove(Move *start, int x, int y, bool checkmarker)
{
    Move *t = start;
    while (t && (t->getX() != x || t->getY() != y))
    {
        t = (checkmarker && t->marker ? t->marker : t->son);
    }
    return t;
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

void Tree::setCurrent(Move *m)
{
    current = m;
    emit currentMoveChanged(current);
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
    if (x == PASS_XY && y == PASS_XY)
        return;

    if ((x < 1) || (x > boardSize) || (y < 1) || (y > boardSize))
        return;

    if (current == root)
    {
        current->setHandicapMove(true);
		current->setMoveNumber(0); //TODO : make sure this is the proper way
        current->setX(-1);
        current->setY(-1);
        current->setColor(stoneBlack); // So that it is white's turn
	}

    if(current->getGamePhase() == phaseEdit)
        current->getMatrix()->insertStone(x, y, c, true);
    else
        current->getMatrix()->insertStone(x, y, c);
}

/*
 * this deletes the current node an all its sons
 */
void Tree::deleteNode()
{
    if (current == root)
    {
        init();
        return;
    }

    Move *remember = current->parent;
    delete current;
    setCurrent(remember);
    if(remember->isInMainBranch())
        lastMoveInMainBranch = remember;
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
    if (root)
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
    if (current && current->brother)
        setCurrent(current->brother);
}

void Tree::slotNavPrevVar()
{
    if (current == NULL)
        return;
    Move *m = current->getPrevBrother();
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
        if (m->getNumSons() > 1 && old != m->son)
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
    current->getMatrix()->markTerritory();
    current->setTerritoryMarked(true);
    current->setScored(true);
    emit currentMoveChanged(current); // Toggles window refresh

    countMarked(current);
}

void Tree::countMarked(Move * mv)
{
    deadWhite = 0;
    deadBlack = 0;
    terrWhite = 0;
    terrBlack = 0;
    capturesBlack = mv->getCapturesBlack();
    capturesWhite = mv->getCapturesWhite();

    mv->getMatrix()->count(terrBlack, terrWhite, deadBlack, deadWhite);
    qDebug() << "Tree::countMarked():" << terrBlack << capturesBlack << deadWhite << terrWhite << capturesWhite << deadBlack;
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
