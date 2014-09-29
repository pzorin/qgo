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


#include "move.h"
#include "matrix.h"
#include "group.h"

Move::Move(int board_size)
{
	brother = NULL;
	son = NULL;
	parent = NULL;
	marker = NULL;
	stoneColor = stoneNone;
	x = y = -1;
	gamePhase = phaseOngoing;
	moveNum = 0;
	terrMarked = false;
	capturesBlack = capturesWhite = 0;
	checked = true;
	//fastLoadMarkDict = NULL;
	scored = false;
	scoreWhite = scoreBlack = 0;
	PLinfo = false;
	timeinfo = false;
	handicapMove = false;
	matrix = new Matrix(board_size);
}

Move::Move(StoneColor c, int mx, int my, int n, GamePhase phase, const Matrix &mat, bool clearAllMarks, const QString &s)
: stoneColor(c), x(mx), y(my), moveNum(n), gamePhase(phase), comment(s)
{
	brother = NULL;
	son = NULL;
	parent = NULL;
	marker = NULL;
	capturesBlack = capturesWhite = 0;
	terrMarked = false;
    checked = true;
	scored = false;
	scoreWhite = scoreBlack = 0;
	PLinfo = false;
	timeinfo = false;
	handicapMove = false;
    matrix = new Matrix(mat, clearAllMarks);
}

Move::Move(StoneColor c, int mx, int my, int n, GamePhase phase, const QString &s)
: stoneColor(c), x(mx), y(my), moveNum(n), gamePhase(phase), comment(s)
{
	brother = NULL;
	son = NULL;
	parent = NULL;
	marker = NULL;
	capturesBlack = capturesWhite = 0;
	terrMarked = false;
	checked = false;
    matrix = NULL;
	scored = false;
	scoreWhite = scoreBlack = 0;
	PLinfo = false;
	timeinfo = false;
    handicapMove = false;
}

Move::Move(Move *_parent, StoneColor _c, int _x, int _y)
    : stoneColor(_c), x(_x), y(_y)
{
    parent = _parent;
    moveNum = parent->moveNum+1;
    gamePhase = parent->gamePhase;
    son = brother = marker = NULL;
    capturesBlack = parent->capturesBlack;
    capturesWhite = parent->capturesWhite;
    matrix = new Matrix(*(parent->matrix), true);
    terrMarked = false;
    checked = false;
    scored = false;
    scoreWhite = scoreBlack = 0;
    PLinfo = false;
    timeinfo = false;
    handicapMove = false;
    // current node has no son?
    if (parent->son == NULL)
    {
        parent->son = this;
    }
    // A son found. Add the new node as farest right brother of that son
    else
    {
        parent->son->addBrother(this);
    }

    // Execute move
    if (x != PASS_XY || y != PASS_XY)
    {
        int lastCaptures = matrix->makeMove(x, y, stoneColor);

        if (stoneColor == stoneBlack)
            capturesBlack += lastCaptures;
        else if (stoneColor == stoneWhite)
            capturesWhite += lastCaptures;
    }
}

Move::~Move()
{
    delete matrix;
}

/*
 * returns a string representing the move in SGF format
 */
const QString Move::saveMove(bool isRoot)
{
	QString str;
	
	if (!isRoot && !handicapMove )
		str += ";"; //"\n;";
	
	if (x != -1 && y != -1)
	{
		if(gamePhase == phaseEdit && matrix->getStoneAt(x,y) == stoneErase) {}
		else
		{
			// Write something like 'B[aa]'
			str += stoneColor == stoneBlack ? "B" : "W";
			str += "[" + Matrix::coordsToString(x-1, y-1) + "]";
		}
	}
	
	// Save edited moves (including handicap)
	str += matrix->saveEditedMoves(parent != NULL ? parent->getMatrix() : 0);
	
	// Save marks
	str += matrix->saveMarks();
	
	// Add nodename, if we have one
	if (!nodeName.isNull() && !nodeName.isEmpty())
	{
		// simpletext
		str += "N[";
		str += nodeName;
		str += "]";
	}

	// Add next move's color
	if (PLinfo)
	{
		if (PLnextMove == stoneBlack)
			str += "PL[B]";
		else
			str += "PL[W]";
	}

	// Add comment, if we have one
	if (!comment.isNull() && !comment.isEmpty())
	{
		// text
		QString tmp = comment;
		int pos = 0;
		while ((  (pos = tmp.indexOf("]", pos)) != -1) && (static_cast<signed int>(pos) < tmp.length()))
		{
			tmp.replace(pos, 1, "\\]");
			pos += 2;
		}
		str += "C[";
		str += tmp;
		str += "]";
	}

	// time info
	if (timeinfo && !isRoot && (int) timeLeft)
	{
		if (stoneColor == stoneBlack)
			str += "BL[";
		else
			str += "WL[";
		str += QString::number(timeLeft);
		str += "]";

		// open moves info
		if (openMoves > 0)
		{
			if (stoneColor == stoneBlack)
				str += "OB[";
			else
				str += "OW[";
			str += QString::number(openMoves);
			str += "]";
		}
	}

	// Add unknown properties, if we have some
	if (!unknownProperty.isNull() && !unknownProperty.isEmpty())
	{
		// complete property
		str += unknownProperty;
	}
	
	return str;
}

bool Move::isPassMove()
{
    if ((x==PASS_XY) && (y==PASS_XY))
		return true;

	return false;
}

int Move::getNumBrothers()
{
	if (parent ==NULL)
		return 0;

	Move *tmp = parent->son;
	int counter = 0;
	
	while ((tmp = tmp->brother) != NULL)
		counter ++;
	
	return counter;
}

int Move::getNumSons()
{
	Move *tmp = son;
	
	if (tmp == NULL)
		return 0;
	
	int counter = 1;
	while ((tmp = tmp->brother) != NULL)
		counter ++;
	
	return counter;
}

bool Move::hasParent()
{
	return (parent != NULL);
}

bool Move::hasPrevBrother()
{
	if (parent == NULL)
		return false;
	else
		return (this != parent->son);
}

bool Move::hasNextBrother()
{
	return (brother != NULL);
}

void Move::addBrother(Move * b)
{
	Move *tmp = this;
		
	// Find brother farest right
	while (tmp->brother != NULL)
		tmp = tmp->brother;
		
	tmp->brother = b;
	b->parent = tmp->parent;
	b->setTimeinfo(false);			//whats this for FIXME?
}

bool Move::checkMoveIsValid(StoneColor c, int x, int y)
{
    if (x == PASS_XY && y == PASS_XY)
        return true;

    if (x < 1 || x > matrix->getSize() || y < 1 || y > matrix->getSize())
    {
        qWarning("Invalid position: %d/%d", x, y);
        return false;
    }

    /* special case, we're erasing a stone */
    if(c == stoneErase)
    {
        return (matrix->getStoneAt(x,y) != stoneNone);
    }

    // If there is already a stone at (x,y) do nothing
    if ((matrix->getStoneAt(x, y) == stoneBlack) ||
        (matrix->getStoneAt(x, y) == stoneWhite))
        return false;

    // Check for ko
    if (matrix->getMarkAt(x,y) & markKoMarker)
        return false;

    std::vector<Group *> visited;
    std::vector<Group *>::iterator it_visited;
    int lastCaptures = matrix->checkStoneCaptures(c,x,y,visited);
    // FIXME should not have to delete these groups here
    for (it_visited = visited.begin(); it_visited < visited.end(); ++it_visited)
    {
        delete (*it_visited);
    }

    return (lastCaptures >= 0);
}

Move *Move::hasSon(StoneColor c, int x, int y)
{
    Move *tmp = son;
    while (tmp != NULL) {
        if ((tmp->getColor() == c) && (tmp->getX() == x) && (tmp->getY() == y))
            break;
        tmp = tmp->brother;
    };
    return tmp;
}

Move *Move::makeMove(StoneColor c, int x, int y, bool force)
{
    if (force || checkMoveIsValid(c, x, y))
        return new Move(this, c, x, y);
    else
        return NULL;
}

Move *Move::makePass()
{
    return makeMove(stoneColor == stoneBlack ? stoneWhite : stoneBlack, PASS_XY, PASS_XY, true);
}

StoneColor Move::whoIsOnTurn()
{
    if (getPLinfo())
        // color of next stone is same as current
        return getPLnextMove();

    return (stoneColor == stoneBlack ? stoneWhite : stoneBlack);
}
