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


#include "matrix.h"
#include "group.h"

Matrix::Matrix(int s)
: size(s)
{
	Q_ASSERT(size > 0 && size <= 36);
	
    matrix = new unsigned short[size*size];
	Q_CHECK_PTR(matrix);
	
    for (int i=0; i<size*size; i++)
        matrix[i] = stoneNone;
}

Matrix::Matrix(const Matrix &m, bool cleanup)
    : size(m.size)
{
    Q_ASSERT(size > 0 && size <= 36);
	
    matrix = new unsigned short[size*size];
	Q_CHECK_PTR(matrix);

    unsigned short mask = (cleanup ? 0x000f | MX_STONEDEAD : 0x0fff);
    for (int i=0; i<size*size; ++i)
        matrix[i] = m.matrix[i] & mask;
}

Matrix::~Matrix()
{
    delete[] matrix;
}

void Matrix::clear()
{
    for (int i=0; i<size*size; ++i)
        matrix[i] = stoneNone;
}

void Matrix::insertStone(int key, StoneColor c, bool fEdit)
{
    matrix[key] = (matrix[key] & 0xfff0) | c;
    if(fEdit)
        matrix[key] |= MX_STONEEDIT;
}

void Matrix::insertStone(int x, int y, StoneColor c, bool fEdit)
{
    Q_ASSERT(x > 0 && x <= size &&
        y > 0 && y <= size);

    insertStone(coordsToKey(x,y),c,fEdit);
}

StoneColor Matrix::getStoneAt(int key) const
{
    return  (StoneColor) (matrix[key] & 0x3);
}

StoneColor Matrix::getStoneAt(int x, int y)
{	
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
    return  getStoneAt(coordsToKey(x,y));
}

bool Matrix::isStoneDead(int x, int y)
{	
	//qDebug("xy: %d %d", x, y);
	Q_ASSERT(x > 0 && x <= size &&
			y > 0 && y <= size);
	
    return (matrix[coordsToKey(x,y)] & MX_STONEDEAD);
}

MarkType Matrix::getMarkAt(int key) const
{
    return  (MarkType) (matrix[key] & markAll);
}

MarkType Matrix::getMarkAt(int x, int y)
{
    Q_ASSERT(x > 0 && x <= size &&
        y > 0 && y <= size);
    return  getMarkAt(coordsToKey(x,y));
}

void Matrix::insertMark(int x, int y, MarkType t)
{
	Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);

    matrix[coordsToKey(x,y)] &= (~markAll);
    matrix[coordsToKey(x,y)] |= t;
}

void Matrix::removeMark(int x, int y)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
    matrix[coordsToKey(x,y)] &= (~markAll);
    markTexts.remove(coordsToKey(x,y));
}

void Matrix::clearAllMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
    for (int i=0; i<size*size; ++i)
    {
        matrix[i] &= (~markAll);
    }
    markTexts.clear();
}

void Matrix::clearTerritoryMarks()
{
    for (int i=0; i<size*size; ++i)
        matrix[i] &= (~markTerrDame);
}

// Called when leaving score mode
void Matrix::absMatrix()
{
	Q_ASSERT(size > 0 && size <= 36);
	
    for (int i=0; i<size*size; ++i)
    {
        matrix[i] &= 0x2fff;		//remove dead and edit?
        if (getStoneAt(i) == stoneErase)
            insertStone(i, stoneNone);
	}
}

void Matrix::setMarkText(int x, int y, const QString &txt)
{
    Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);
    markTexts.insert(coordsToKey(x,y),txt);
}

const QString Matrix::getMarkText(int x, int y)
{
    Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);
    return markTexts.value(coordsToKey(x,y));
}

/*
 * returns a string representing the marks in the matrix, in SGF format
 */
const QString Matrix::saveMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	QString txt, sSQ = "", sCR = "", sTR = "", sMA = "", sLB = "", sTB = "", sTW = "";
	int i, j, colw = 0, colb = 0;
	
	for (i=0; i<size; i++)
	{
		for (j=0; j<size; j++)
		{
			switch (getMarkAt(i + 1, j + 1))
			{
			case markSquare:
				if (sSQ.isEmpty())
					sSQ += "SQ";
				sSQ += "[" + coordsToString(i, j) + "]";
				break;
			case markCircle:
				if (sCR.isEmpty())
					sCR += "CR";
				sCR += "[" + coordsToString(i, j) + "]";
				break;
			case markTriangle:
				if (sTR.isEmpty())
					sTR += "TR";
				sTR += "[" + coordsToString(i, j) + "]";
				break;
			case markCross:
				if (sMA.isEmpty())
					sMA += "MA";
				sMA += "[" + coordsToString(i, j) + "]";
				break;
			case markText: 
			case markNumber:
				if (sLB.isEmpty())
					sLB += "LB";
				sLB += "[" + coordsToString(i, j);
				sLB += ":";
				txt = getMarkText(i+1, j+1);
				if (txt.isNull() || txt.isEmpty())
					sLB += "?";  // Whoops
				else
					sLB += txt;
				sLB += "]";
				break;
			case markTerrBlack:
				if (sTB.isEmpty())
				{
					sTB += "\nTB";
					colb = 0;
				}
				sTB += "[" + coordsToString(i, j) + "]";
				if (++colb % 15 == 0)
					sTB += "\n  ";
				break;
			case markTerrWhite:
				if (sTW.isEmpty())
				{
					sTW += "\nTW";
					colw = 0;
				}
				sTW += "[" + coordsToString(i, j) + "]";
				if (++colw % 15 == 0)
					sTW += "\n  ";
				break;
			default: continue;
			}
		}
	}
	
	return sSQ + sCR + sTR + sMA + sLB + sTB + sTW;
}

/*
 * returns a string representing the edited stones in the matrix, in SGF format
 */
const QString Matrix::saveEditedMoves(Matrix *parent)
{
	Q_ASSERT(size > 0 && size <= 36);
	
	QString sAB="", sAW="", sAE="";
	int i, j;
	int z;
	for (i=0; i<size; i++)
	{
		for (j=0; j<size; j++)
		{
			z = getStoneAt(i + 1, j + 1);
			if (z != 0)
				z= 0;
            if(!(matrix[internalCoordsToKey(i,j)] & MX_STONEEDIT))
				continue;
			switch (getStoneAt(i + 1, j + 1))
			{
			case stoneBlack:
				if (parent != NULL &&
					parent->getStoneAt(i + 1, j + 1) == stoneBlack)
					break;
				if (sAB.isEmpty())
					sAB += "AB";
				sAB += "[" + coordsToString(i, j) + "]";
				break;
				
			case stoneWhite:
				if (parent != NULL &&
					parent->getStoneAt(i + 1, j + 1) == stoneWhite)
					break;
				if (sAW.isEmpty())
					sAW += "AW";
				sAW += "[" + coordsToString(i, j) + "]";
				break;
				
			case stoneErase:
				if (parent != NULL &&
					(parent->getStoneAt(i + 1, j + 1) == stoneNone ||
					parent->getStoneAt(i + 1, j + 1) == stoneErase))
					break;
				if (sAE.isEmpty())
					sAE += "AE";
				sAE += "[" + coordsToString(i, j) + "]";
				break;
			default:
			case stoneNone:
				break;
			}
		}
	}
	
	return sAB + sAW + sAE;
}

const QString Matrix::printMe(ASCII_Import *charset)
{
	Q_ASSERT(size > 0 && size <= 36);
	
#if 0
	qDebug("BLACK STONE CHAR %c\n"
		"WHITE STONE CHAR %c\n"
		"STAR POINT  CHAR %c\n"
		"EMPTY POINT CHAR %c\n",
		charset->blackStone,
		charset->whiteStone,
		charset->starPoint,
		charset->emptyPoint);
#endif
	
	int i, j;
	QString str;
	
	str = "\n    ";
	for (i=0; i<size; i++)
		str += QString(QChar(static_cast<const char>('A' + (i<8?i:i+1)))) + " ";
	str += "\n   +";
	for (i=0; i<size-1; i++)
		str += "--";
	str += "-+\n";
	
	for (i=0; i<size; i++)
	{
		if (size-i < 10) str += " ";
		str += QString::number(size-i) + " |";
		for (j=0; j<size; j++)
		{
			switch (getStoneAt(j + 1, i + 1))
			{
			case stoneBlack: str += QChar(charset->blackStone); str += " "; break;
			case stoneWhite: str += QChar(charset->whiteStone); str += " "; break;
			default:
				// Check for starpoints
				if (size > 9)  // 10x10 or larger
				{
					if ((i == 3 && j == 3) ||
						(i == size-4 && j == 3) ||
						(i == 3 && j == size-4) ||
						(i == size-4 && j == size-4) ||
						(i == (size+1)/2 - 1 && j == 3) ||
						(i == (size+1)/2 - 1 && j == size-4) ||
						(i == 3 && j == (size+1)/2 - 1) ||
						(i == size-4 && j == (size+1)/2 - 1) ||
						(i == (size+1)/2 - 1 && j == (size+1)/2 - 1))
					{
						str += QChar(charset->starPoint);
						str += " ";
						break;
					}
				}
				else  // 9x9 or smaller
				{
					if ((i == 2 && j == 2) ||
						(i == 2 && j == size-3) ||
						(i == size-3 && j == 2) ||
						(i == size-3 && j == size-3))
					{
						str += QChar(charset->starPoint);
						str += " ";
						break;
					}
				}
				
				str += QChar(charset->emptyPoint);
				str += " ";
				break;
			}
		}
		str = str.left(str.length()-1);
		str += "| ";
		if (size-i < 10) str += " ";
		str += QString::number(size-i) + "\n";
	}
	
	str += "   +";
	for (i=0; i<size-1; i++)
		str += "--";
	str += "-+\n    ";
	for (i=0; i<size; i++)
		str += QString(QChar(static_cast<const char>('A' + (i<8?i:i+1)))) + " ";
	str += "\n";
	
	return str;
}

/*
 * checks wether the positions x,y holds a stone 'color'.
 * if yes, appends the stone and returns the group
 */
void Matrix::checkNeighbour(int key, StoneColor color, Group *group, std::vector<int> *libertyList) const
{
    StoneColor c = getStoneAt(key);
    if(c != color)
    {
        if((libertyList) && (c == stoneNone)
                && (std::find(libertyList->begin(), libertyList->end(), key) == libertyList->end()))
		{
            libertyList->push_back(key);
			group->liberties++;
		}
        return;
    }

    if (!(group->contains(key)))
        group->append(key);
}

/* Counts and returns the number of liberties of a group.
 * What does not count as a liberty is determined by "mask".
 * By default "mask" is set to black and white stones,
 * so any unoocupied point counts as a liberty.
 * One can put opponent's territory into mask to exclude dame points
 * from liberties. This is useful for detecting false eyes. */
int Matrix::countLiberties(Group *group, unsigned short mask)
{
    QVector<int> libCounted;
	
	// Walk through the horizontal and vertial directions, counting the
	// liberties of this group.
	for (int i=0; i<group->count(); i++)
    {
        std::vector<int> neighbors = getNeighbors(group->at(i));
        std::vector<int>::iterator it_neighbor;
        for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
        {
            if ( !(matrix[*it_neighbor] & mask) &&
                 !libCounted.contains(*it_neighbor))
            {
                  libCounted.append(*it_neighbor);
            }

        }
    }
    return libCounted.size();
}

/*
 * Explores a territorry for marking all of its enclosure
 */
void Matrix::traverseTerritory( int key, StoneColor &col)
{
    // Mark visited
    matrix[key] |= MARK_TERRITORY_VISITED;

    std::vector<int> neighbors = getNeighbors(key);
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        // Disregard visited points
        if (matrix[*it_neighbor] & MARK_TERRITORY_VISITED)
            continue;

        // If we find a stone that is not dead, we put its color into "col"
        // TODO: handle seki properly
        if ((getStoneAt(*it_neighbor) != stoneNone) && !(matrix[*it_neighbor] & MX_STONEDEAD))
        {
            col = (StoneColor)(col | getStoneAt(*it_neighbor));
            continue;
        }

        // The remaining possibilities are dead stones or emty territory: go there
        traverseTerritory(*it_neighbor,col);
    }
}

/*
 * Returns the group (QList) of matrix stones connected with the stone "key"
 */
Group* Matrix::assembleGroup(int key, StoneColor c) const
{
    Group *group = new Group(c);
    Q_CHECK_PTR(group);
    group->append(key);
    std::vector<int> libertyList;
    int mark = 0;
	
	// Walk through the horizontal and vertical directions and assemble the
	// attached stones to this group.
    while (mark < group->count())
    {
        if (getStoneAt(group->at(mark)) != stoneNone )		//FIXME how can this be stoneNone?
        {
            std::vector<int> neighbors = getNeighbors(group->at(mark));
            std::vector<int>::iterator it_neighbor;
            for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
                checkNeighbour(*it_neighbor,c,group, &libertyList);
        }
        mark ++;
    }

	return group;
}

/* This puts groups that the stone touches into "visited" and counts the
 * number of captured stones (positive if enemy stones are captured,
 * negative for suicide). This number can be used to check if the
 * move is legal before playing it.
 * This function does not alter the matrix */
int Matrix::checkStoneCaptures(StoneColor ourColor, int x, int y, std::vector<Group *> &visited) const
{
    StoneColor theirColor = (ourColor == stoneWhite ? stoneBlack : stoneWhite);
    StoneColor c;
    Group * g = NULL;
    bool emptyNeighbor = false;

    // Find adjacent groups and put them into "visited"
    std::vector<int> neighbors = getNeighbors(coordsToKey(x,y));
    std::vector<int>::iterator it_neighbor;
    std::vector<Group *>::iterator it_visited;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        c = getStoneAt(*it_neighbor);
        if ((c == stoneBlack) || (c == stoneWhite))
        {
            g = NULL;
            for (it_visited = visited.begin(); it_visited < visited.end(); ++it_visited)
            {
                if ((*it_visited)->contains(*it_neighbor))
                {
                    g = *it_visited;
                    break;
                }
            }
            if (g == NULL)
            {
                g = assembleGroup(*it_neighbor, c);
                visited.push_back(g);
            }
        } else
            emptyNeighbor = true;
    }

    // Count the number of captured stones
    int captures = 0;
    // Check if some enemy groups are captured
    for (it_visited = visited.begin(); it_visited < visited.end(); ++it_visited)
    {
        if (((*it_visited)->c == theirColor) && ((*it_visited)->liberties == 1))
            captures += (*it_visited)->size();
    }
    // If enemy captured, return number of captures
    if (captures > 0)
        return captures;
    // If no enemies captured, proceed
    // If some neighboring square is empty, this is not a suicide
    if (emptyNeighbor)
        return 0;
    // Count the number of stones in our adjacent groups
    emptyNeighbor = false;
    for (it_visited = visited.begin(); it_visited < visited.end(); ++it_visited)
    {
        if ((*it_visited)->c == ourColor)
        {
            captures -= (*it_visited)->size();
            if ((*it_visited)->liberties > 1)
                emptyNeighbor = true;
        }
    }
    // If some group has >1 liberty, this is not a suicide
    if (emptyNeighbor)
        return 0;
    // Else this is a suicide, return the (negative) number of dying stones
    return captures-1;
}

/* This function executes a move without checking its validity.
 * This function returns the number of captured stones.
 * If the return value is negative, this means that the requested move
 * was a suicide. */
int Matrix::makeMove(int x, int y, StoneColor c)
{
    std::vector<Group *> visited;
    int capturedStones = checkStoneCaptures(c, x, y, visited);

    if (capturedStones >= 0)
        insertStone(x, y, c);

    std::vector<Group *>::iterator it_visited;
    for (it_visited = visited.begin(); it_visited < visited.end(); ++it_visited)
    {
        if (((*it_visited)->c != c) && ((*it_visited)->liberties == 1)) // Enemy group with one liberty
        {
            removeGroup((*it_visited));
        } else if (((*it_visited)->c == c) && (capturedStones < 0)) // suicide
        {
            removeGroup((*it_visited));
        } else
        {
            delete (*it_visited);
        }
    }
    return capturedStones;
}

/* Note that checkStoneWithGroups would be called before this meaning that
 * their could be no adjacent stones not in groups */
void Matrix::removeGroup(Group * g)
{
    /* For every stone in g we go through the neighboring groups.
     * If the neighboring group is distinct from g, then it has different color.
     * Every such group gets its number of liberties increased by 1.
     * We take care not to count such groups multiple times. */
    for(int n = 0; n < g->count(); ++n)
    {
        insertStone(g->at(n), stoneNone);
	}
	delete g;
}

void Matrix::removeStoneFromGroups(int x, int y)
{
    int key = coordsToKey(x,y);
    insertStone(key, stoneErase);
}

/* This is kind of ugly but I'm trying to use the existing matrix
 * code for something weird 
 * Could there be a potential miscalc if called on empty vertex?
 * Would it be*/
Group* Matrix::assembleAreaGroups(int key, StoneColor c)
{
    Group *group = new Group(c);
	Q_CHECK_PTR(group);
    StoneColor oppColor = (c == stoneWhite ? stoneBlack : stoneWhite);

    group->append(key);
	
	int mark = 0;
	// Walk through the horizontal and vertical directions and assemble the
	// attached stones to this group.
    while (mark < group->count())
	{
        if (getStoneAt(group->at(mark)) != oppColor )
        {
            std::vector<int> neighbors = getNeighbors(group->at(mark));
            std::vector<int>::iterator it_neighbor;
            for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
            {
                checkNeighbour(*it_neighbor,c,group);
                checkNeighbour(*it_neighbor,stoneNone,group);
            }
		}
		mark ++;
	}
	
	return group;
}

bool Matrix::checkfalseEye( int key, StoneColor col)
{
    unsigned short mask = stoneBlack | stoneWhite;
    mask |= (col == stoneBlack ? markTerrWhite : markTerrBlack);
    std::vector<int> neighbors = getNeighbors(key);
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        if (getStoneAt(*it_neighbor) == col)
        {
            if (countLiberties(assembleGroup(*it_neighbor,col), mask) == 1)
                return true;
        }
    }
	return false;
}

/*
 * This function marks all the stones of a group as dead (or alive if they were dead)
 */
void Matrix::toggleGroupAt( int x, int y)
{
    Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);
	StoneColor col = getStoneAt(x, y);

	if ( col != stoneWhite && col != stoneBlack )
		return ;

    Group *g = assembleGroup(coordsToKey(x,y),col);			//FIXME use the view

	for (int i=0; i<g->count(); i++)
        toggleStoneAt(g->at(i));
}

/* These are all kind of redundant, some aren't used, etc..
 * the other thing that bothers me is that the stone.cpp class
 * should probably hold "dirty" and then we've considered before
 * that matrix could be removed for something that takes up
 * less space, but if this works for now, that's fine. FIXME */

void Matrix::toggleStoneAt(int key)
{
    matrix[key] ^= MX_STONEDEAD;
}

void Matrix::toggleStoneAt(int x, int y)
{
    toggleStoneAt(coordsToKey(x,y));
}


void Matrix::markStoneDead(int x, int y)
{
	if(isStoneDead(x, y))
		return;
	toggleStoneAt(x, y);
}

void Matrix::markGroupDead(int x, int y)
{
	if(isStoneDead(x, y))
		return;
	toggleGroupAt(x, y);
}

void Matrix::markGroupAlive(int x, int y)
{
	if(!isStoneDead(x, y))
		return;
	toggleGroupAt(x, y);
}

/* Any stones connected to this xy stone by empty
 * vertices are to be marked dead */
void Matrix::toggleAreaAt( int x, int y)
{
	StoneColor col = getStoneAt(x, y);

	if ( col != stoneWhite && col != stoneBlack )
		return ;

    Group *g = assembleAreaGroups(coordsToKey(x,y),col);

	for (int i=0; i<g->count(); i++)
        toggleStoneAt(g->at(i));

    delete g;
}

void Matrix::markAreaDead(int x, int y)
{
	if(isStoneDead(x, y))
		return;
	toggleAreaAt(x, y);
}

void Matrix::markAreaAlive(int x, int y)
{
	if(!isStoneDead(x, y))
		return;
    toggleAreaAt(x, y);
}

int Matrix::countDeadWhite()
{
    int result = 0;
    for (int i=0; i<size*size; ++i)
        if ((matrix[i] & MX_STONEDEAD) && (getStoneAt(i) == stoneWhite))
            ++result;
    return result;
}

int Matrix::countDeadBlack()
{
    int result = 0;
    for (int i=0; i<size*size; ++i)
        if ((matrix[i] & MX_STONEDEAD) && (getStoneAt(i) == stoneBlack))
            ++result;
    return result;
}

void Matrix::markTerritory(int & terrBlack, int & terrWhite)
{
    terrWhite = 0;
    terrBlack = 0;
    int i,j;

    for (i=0; i<size*size; ++i)
        matrix[i] &= (~MX_VISITED); // Mark all points as not visited

    StoneColor col;
    for (i=0; i<size*size; ++i)
    {
        if ((!(matrix[i] & MX_VISITED)) && (getStoneAt(i) == stoneNone))
        {
            // Only count territory starting from empty points
            col = stoneNone;
            traverseTerritory( i, col);
            unsigned short mark = 0;
            if (col & stoneWhite)
                mark |= markTerrWhite;
            if (col & stoneBlack)
                mark |= markTerrBlack;
            int terr = 0;
            for (j=0; j<size*size; ++j)
            {
                if (matrix[j] & MARK_TERRITORY_VISITED)
                {
                    matrix[j] &= (~MARK_TERRITORY_VISITED);
                    matrix[j] |= mark;
                    matrix[j] |= MX_VISITED;
                    ++terr;
                }
            }
            if (col == stoneWhite)
                terrWhite += terr;
            if (col == stoneBlack)
                terrBlack += terr;
        }
    }

    // Finally, remove all false eyes that have been marked as territory. This
    // has to be here, as in the above loop we did not find all dame points yet.
    // FIXME: handle false eyes and seki properly.
    /*for (i=0; i<size*size; ++i)
    {
        MarkType mark = getMarkAt(i);
        if (mark == markTerrBlack)
        {
            if (checkfalseEye(i,stoneBlack))
            {
                matrix[i] &= 0xff0f;
                --terrBlack;
            }
        }
        if (mark == markTerrWhite)
        {
            if (checkfalseEye(i,stoneWhite))
            {
                matrix[i] &= 0xff0f;
                --terrWhite;
            }
        }
    }*/
}

/*
 * This function returns the first letter or number available in the letter marks
 */
QString Matrix::getFirstTextAvailable(MarkType t)
{
    QString mark;

	switch (t)
	{
		case markText :
			for (int n = 0 ; n < 51 ; n++)
			{
				mark = QString(QChar(static_cast<const char>('A' + (n>=26 ? n+6 : n))));
                if (markTexts.key(mark,-1) == -1)
					break;
			}
			break;
		case markNumber :
			for (int n = 1 ; n < 400 ; n++)
			{
                mark = QString::number(n);
                if (markTexts.key(mark,-1) == -1)
					break;
			}
			break;
		//should not happen
		default:
			break;
	}

	return mark;
	
}

std::vector<int> Matrix::getNeighbors(int key) const
{
    std::vector<int> result;
    if (key >= size) // north
        result.push_back(key-size);
    if ((key % size) > 0) // west
        result.push_back(key-1);
    if ((key % size) < size-1) // east
        result.push_back(key+1);
    if ((key)<size*size-size) // south
        result.push_back(key+size);
    return result;
}
