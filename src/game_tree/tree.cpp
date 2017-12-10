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
}

Tree::~Tree()
{
    if (root)
        delete root;
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
    if (!m || (current == m))
        return;

    current = m;
    emit currentMoveChanged(current);
}

Move *Tree::findLastMoveInMainBranch()
{
	return lastMoveInMainBranch;
}

Move *Tree::findLastMoveInCurrentBranch()
{
    Move *m = getCurrent();
    if (m)
        return m->getLastMove(true);
    else
        return NULL;
}

void Tree::addEmptyMove()
{
    Move * node = new Move(current,stoneBlack, -1, -1);
    node->setTimeinfo(false);
    current = node;
    if(current->isInMainBranch())
        lastMoveInMainBranch = current;
}

/* FIXME double check and remove editMove, unnecessary */
void Tree::doPass(bool /*sgf*/, bool /*fastLoad*/)
{
/////////////////FIXME We may have a problem at first move with handicap games ...
	StoneColor c = (current->getColor() == stoneWhite) ? stoneBlack : stoneWhite;
    Move *result = current->makeMove(c, PASS_XY, PASS_XY);
    setCurrent(result);
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

    while (m)
    {
        if (!(m->getComment().isEmpty()))
            break;
        m = m->parent;
    }
    setCurrent(m);
}

void Tree::slotNavNextComment()
{
    Move  *m = current->son;

    while (m)
    {
        if (!(m->getComment().isEmpty()))
            break;
        m = m->son;
    }
    setCurrent(m);
}

void Tree::slotNthMove(int n)
{
    if (n < 0)
        return;

    Move *m = current,
        *old = m;

    int currentMove = m->getMoveNumber();

    while (m && ((currentMove = m->getMoveNumber()) != n))
    {
        m->parent->marker = m;
        if (n > currentMove)
        {
            m = (m->marker ? m->marker : m->son);
        }
        else
        {
            m = m->parent;
        }
    }

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
    setCurrent(current->getPrevBrother());
}

void Tree::slotNavStartVar()
{
    Move *m = current->parent;
    while (m && !(m->brother))
        m = m->parent;
    setCurrent(m ? m : root);
}

void Tree::slotNavNextBranch()
{
    Move *m = current;
    if (m == NULL)
        return;

    // Descend to the next branching point
    while (m->son && !(m->brother))
        m = m->son;

    setCurrent(m);
}

/*
 * This function resumes back to the first move in the main branch
 */
void Tree::slotNavMainBranch()
{
    Move *m = current;
    Move *n = current;
    while (m->parent)
    {
        if (m->parent->son != m)
            n = m->parent;
        m = m->parent;
    }

    if (n == m)
        return;

    n->marker = NULL;
    setCurrent(n);
}

/*
 * Called after the preceding (slot nav Intersection)
 * When the intersection 'x/y' has been clicked on
 */
void Tree::findMoveByPos(int x, int y)
{
    Move *m = findMoveInMainBranch(x, y);
    if(!m)
        m=findMoveInCurrentBranch(x, y);

    if (m)
        setCurrent(m);
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
