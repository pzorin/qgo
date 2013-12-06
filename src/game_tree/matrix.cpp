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

Matrix::Matrix(const Matrix &m)
    : size (m.size)
{
	Q_ASSERT(size > 0 && size <= 36);
	
    matrix = new unsigned short[size*size];
	Q_CHECK_PTR(matrix);
	
    for (int i=0; i<size*size; i++)
        matrix[i] = (m.matrix[i] & ~MX_STONEDIRTY);
}

Matrix::Matrix(const Matrix &m, bool cleanup)
    : size(m.size)
{
    Q_ASSERT(size > 0 && size <= 36);
	
    matrix = new unsigned short[size*size];
	Q_CHECK_PTR(matrix);

    for (int i=0; i<size*size; ++i)
    {
        if(cleanup)
            matrix[i] = (m.matrix[i] & ~MX_STONEDIRTY) & (0x000f | MX_STONEDEAD);	//clearAllMarks
        else
            matrix[i] = (m.matrix[i] & ~MX_STONEDIRTY) & 0x2fff;			//absMatrix
        matrix[i] |= MX_STONEDIRTY;
    }
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
    matrix[key] |= MX_STONEDIRTY;
	if(fEdit)
        matrix[key] |= MX_STONEEDIT;
}

void Matrix::insertStone(int x, int y, StoneColor c, bool fEdit)
{
    Q_ASSERT(x > 0 && x <= size &&
        y > 0 && y <= size);

    insertStone(coordsToKey(x,y),c,fEdit);
}

unsigned short Matrix::at(int key) const
{
    return (matrix[key] & ~MX_STONEDIRTY);
}

unsigned short Matrix::at(int x, int y) const
{
    //qDebug("assert line %d: %d %d < %d", __LINE__, x, y, size);
    Q_ASSERT(x >= 0 && x < size &&
        y >= 0 && y < size);

    return at(internalCoordsToKey(x,y));
}

StoneColor Matrix::getStoneAt(int key) const
{
    return  (StoneColor) (matrix[key] & 0x000f);
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

bool Matrix::isStoneDirty(int x, int y)
{
    Q_ASSERT(x > 0 && x <= size &&
            y > 0 && y <= size);
    return (matrix[coordsToKey(x,y)] & MX_STONEDIRTY);
}

void Matrix::stoneUpdated(int x, int y)
{
    Q_ASSERT(x > 0 && x <= size &&
            y > 0 && y <= size);
    matrix[coordsToKey(x,y)] &= ~MX_STONEDIRTY;
}

void Matrix::invalidateStone(int x, int y)
{
    Q_ASSERT(x > 0 && x <= size &&
            y > 0 && y <= size);
    matrix[coordsToKey(x,y)] |= MX_STONEDIRTY;
}

void Matrix::markChangesDirty(Matrix & m)
{
    for (int i=0; i<size*size; ++i)
    {
        if((matrix[i] & 0x00ff) != (m.matrix[i] & 0x00ff))
            matrix[i] |= MX_STONEDIRTY;
    }
}

MarkType Matrix::getMarkAt(int key) const
{
    return  (MarkType) ((matrix[key] >> 4) & 0x000f);
}

MarkType Matrix::getMarkAt(int x, int y)
{
    Q_ASSERT(x > 0 && x <= size &&
        y > 0 && y <= size);
    return  getMarkAt(coordsToKey(x,y));
}

void Matrix::set(int key, int n)
{
    matrix[key] = n;
    matrix[key] |= MX_STONEDIRTY;
}

void Matrix::set(int x, int y, int n)
{
    Q_ASSERT(x >= 0 && x < size &&
        y >= 0 && y < size);

    set(internalCoordsToKey(x,y), n);
}

void Matrix::insertMark(int x, int y, MarkType t)
{
	Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);

    matrix[coordsToKey(x,y)] |= (t << 4);
    matrix[coordsToKey(x,y)] |= MX_STONEDIRTY;
}

void Matrix::removeMark(int x, int y)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
    matrix[coordsToKey(x,y)] &= 0xff0f;
    matrix[coordsToKey(x,y)] |= MX_STONEDIRTY;

    markTexts.remove(coordsToKey(x,y));
}

void Matrix::clearAllMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
    for (int i=0; i<size*size; ++i)
    {
        matrix[i] &= (0x000f | MX_STONEDEAD);
        matrix[i] |= MX_STONEDIRTY;
    }
    markTexts.clear();
}

void Matrix::clearTerritoryMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	int data;
	
    for (int i=0; i<size*size; ++i)
            if ((data = getMarkAt(i)) == markTerrBlack ||
				data == markTerrWhite)
			{
                matrix[i] &= (0x000f | MX_STONEDEAD);
                matrix[i] |= MX_STONEDIRTY;
			}
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
        matrix[i] |= MX_STONEDIRTY;
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
Group* Matrix::checkNeighbour(int key, StoneColor color, Group *group, Group **gm, std::vector<int> *libertyList) const
{
    StoneColor c = getStoneAt(key);
    if(gm && c == stoneNone)
    {
        if((libertyList) && (std::find(libertyList->begin(), libertyList->end(), key) == libertyList->end()))
		{
            libertyList->push_back(key);
			group->liberties++;
		}
		return group;
	}
	else if(c != color)
		return group;

    int pos = group->indexOf(key);
    if ((pos == -1) || (color != group->c))
	{
        group->append(key);
        if(gm)
            gm[key] = group;
	}
	return group;
}

/*
 * Counts and returns the number of liberties of a group of adjacetn stones
 * Since we replaced checkPosition, this might only be called by checkfalseEye
 * if anything.  */
int Matrix::countLiberties(Group *group) 
{
	int liberties = 0;
	QList<int> libCounted;
	
	// Walk through the horizontal and vertial directions, counting the
	// liberties of this group.
	for (int i=0; i<group->count(); i++)
    {
        std::vector<int> neighbors = getNeighbors(group->at(i));
        std::vector<int>::iterator it_neighbor;
        for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
        {
            if ( (getStoneAt(*it_neighbor) == stoneNone ) &&
                 !libCounted.contains(*it_neighbor))
            {
                  libCounted.append(*it_neighbor);
                  liberties ++;
            }

        }
    }
	return liberties;
}

/*
 * Same as above, but used for the scoring phase
 */
int Matrix::countScoredLiberties(Group *group)
{
	int liberties = 0;
	QList<int> libCounted;
	
	// Walk through the horizontal and vertial directions, counting the
	// liberties of this group.
	for ( int i=0; i<group->count(); i++)
    {
        std::vector<int> neighbors = getNeighbors(group->at(i));
        std::vector<int>::iterator it_neighbor;
        for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
        {
            if ( ((at(*it_neighbor) & MARK_TERRITORY_DONE_BLACK) ||
                (at(*it_neighbor) & MARK_TERRITORY_DONE_WHITE)) &&
                 !libCounted.contains(*it_neighbor))
            {
                libCounted.append(*it_neighbor);
                liberties ++;
            }
        }
    }
	return liberties;
}

/*
 * Explores a territorry for marking all of its enclosure
 */
void Matrix::traverseTerritory( int x, int y, StoneColor &col)
{
    Q_ASSERT(x >= 0 && x < size && y >= 0 && y < size);
    traverseTerritory(internalCoordsToKey(x,y),col);
}

void Matrix::traverseTerritory( int key, StoneColor &col)
{
    // Mark visited
    set(key, MARK_TERRITORY_VISITED);

    std::vector<int> neighbors = getNeighbors(key);
    std::vector<int>::iterator it_neighbor;
    bool flag;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        // Default: a stone, correct color, or already visited, or seki. Dont continue
        flag = false;
        // No stone ? Continue
        if (at(*it_neighbor) == 0 || (at(*it_neighbor) & MX_STONEDEAD))
            flag = true;
        // A stone, but no color found yet? Then set this color and dont continue
        // The stone must not be marked as alive in seki.
        else if (col == stoneNone && at(*it_neighbor) < MARK_SEKI)
        {
            col = getStoneAt(*it_neighbor);
            flag = false;
        }
        // A stone, but wrong color? Set abort flag but continue to mark the rest of the dame points
        else
        {
            if ( ((col == stoneBlack) && (at(*it_neighbor) == stoneWhite)) ||
                 ((col == stoneWhite) && (at(*it_neighbor) == stoneBlack)) )
            {
                col = stoneErase;
                flag = false;
            }
        }
        if (flag)
            traverseTerritory(*it_neighbor,col);
    }
}

/*
 * Returns the group (QList) of matrix stones adjacent to the stone "key"
 */
Group* Matrix::assembleGroup(int key, StoneColor c, Group ** gm) const
{
    Group *group = new Group(c);
    Q_CHECK_PTR(group);
    group->append(key);
    if(gm)
        gm[key] = group;
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
                checkNeighbour(*it_neighbor,c,group,gm, &libertyList);
        }
        mark ++;
    }

	return group;
}

int Matrix::sharedLibertyWithGroup(int x, int y, Group * g, Group ** gm)
{
    Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);
    std::vector<int> neighbors = getNeighbors(coordsToKey(x,y));
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        if (gm[*it_neighbor] == g)
            return 1;
    }
	return 0;
}

/* This puts groups that the stone touches into "visited" and counts the
 * number of captured stones (positive if enemy stones are captured,
 * negative for suicide). This number can be used to check if the
 * move is legal before playing it.
 * This function does not alter the matrix */
int Matrix::checkStoneCaptures(StoneColor ourColor, int x, int y, Group ** gm, std::vector<Group *> &visited) const
{
    StoneColor theirColor = (ourColor == stoneWhite ? stoneBlack : stoneWhite);
    StoneColor c;
    Group * g = NULL;
    bool emptyNeighbor = false;

    // Find adjacent groups and put them into "visited"
    std::vector<int> neighbors = getNeighbors(coordsToKey(x,y));
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        c = getStoneAt(*it_neighbor);
        if ((c == stoneBlack) || (c == stoneWhite))
        {
            g = gm[*it_neighbor];
            if(!g)
                g = assembleGroup(*it_neighbor, c, gm);
            if (std::find(visited.begin(), visited.end(), g) == visited.end())
            {
                visited.push_back(g);
            }
        } else
            emptyNeighbor = true;
    }

    // Count the number of captured stones
    std::vector<Group *>::iterator it_visited;
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
            captures -= (*it_visited)->size();
        if ((*it_visited)->liberties > 1)
            emptyNeighbor = true;
    }
    // If some group has >1 liberty, this is not a suicide
    if (emptyNeighbor)
        return 0;
    // Else this is a suicide, return the (negative) number of dying stones
    return captures-1;
}

/* This function executes a move unless it is a suicide.
 * This function does not check for ko, assumes that (x,y) is free,
 * and assumes that (c == stoneBlack) || (c == stoneWhite).
 * This function returns the number of captured stones.
 * If the return value is negative, this means that the requested move
 * would lead to suicide, so it was not executed. */
int Matrix::makeMoveIfNotSuicide(int x, int y, StoneColor c, Group ** gm)
{
    std::vector<Group *> visited;
    int capturedStones = checkStoneCaptures(c, x, y, gm, visited);
    if(capturedStones < 0)
        return capturedStones;	//suicide

    // If not suicide, proceed with inserting stone and updating group matrix
    insertStone(x, y, c);
    Group * newgroup = assembleGroup(coordsToKey(x,y),c, gm);
    std::vector<Group *>::iterator it_visited;
    for (it_visited = visited.begin(); it_visited < visited.end(); ++it_visited)
    {
        if ((*it_visited)->c == c)
            delete (*it_visited); // This group is now part of "newgroup"
        else if ((*it_visited)->liberties == 1) // Enemy group with one liberty
            removeGroup((*it_visited), gm, newgroup);
        else // Enemy group with >1 liberty
            (*it_visited)->liberties--;
    }
    return capturedStones;
}

/* Note that checkStoneWithGroups would be called before this meaning that
 * their could be no adjacent stones not in groups */
void Matrix::removeGroup(Group * g, Group ** gm, Group *)
{
	Group * enemyGroup;
    std::vector<Group *> visited;

    /* For every stone in g we go through the neighboring groups.
     * If the neighboring group is distinct from g, then it has different color.
     * Every such group gets its number of liberties increased by 1.
     * We take care not to count such groups multiple times. */
    for(int n = 0; n < g->count(); ++n)
    {
        visited.clear();
        std::vector<int> neighbors = getNeighbors(g->at(n));
        std::vector<int>::iterator it_neighbor;
        for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
        {
            enemyGroup = gm[*it_neighbor];
            if(enemyGroup && enemyGroup != g &&
                    (std::find(visited.begin(), visited.end(), enemyGroup) == visited.end()))
            { // Not visited this group yet
                enemyGroup->liberties++;
                visited.push_back(enemyGroup);
            }
        }

        gm[g->at(n)] = NULL;
        insertStone(g->at(n), stoneNone);
	}
	delete g;
}

void Matrix::removeStoneFromGroups(int x, int y, StoneColor ourColor, Group ** gm)
{
	StoneColor c;
	Group * g = NULL;
    std::vector<Group *> visited;

    int key = coordsToKey(x,y);
    insertStone(key, stoneErase);
    std::vector<int> neighbors = getNeighbors(key);
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        c = getStoneAt(*it_neighbor);
        if ((c == stoneBlack) || (c == stoneWhite))
        {
            g = gm[*it_neighbor];
            if(g)
            {
                if(std::find(visited.begin(), visited.end(), g) == visited.end())
                { // Not visited this group yet
                    g->liberties++;
                    if (g->c == ourColor)
                        g->remove(*it_neighbor);
                    visited.push_back(g);
                }
            } else {
                visited.push_back(assembleGroup(*it_neighbor, c, gm));
            }
        }
    }
}

void Matrix::invalidateChangedGroups(Matrix & m, Group ** gm)
{
	std::vector<Group*> groupList;
	std::vector<Group*>::iterator k;
	Group * g;

    for (int i=0; i<size*size; ++i)
    {
        if((matrix[i] & 0x00ff) != (m.matrix[i] & 0x00ff))
        {
            g = gm[i];
            if(!g)
                continue;
            if(std::find(groupList.begin(), groupList.end(), g) == groupList.end())
            {
                groupList.push_back(g);
                findInvalidAdjacentGroups(g, gm, groupList);
            }
        }
    }
    for(k = groupList.begin(); k != groupList.end(); k++)
	{
        for(int i = 0; i < (*k)->count(); i++)
            gm[(*k)->at(i)] = NULL;
		delete *k;
	}
}

void Matrix::invalidateAdjacentGroups(int x, int y, Group ** gm)
{
    // No assert here because of issues with loading SGF. Valid range 1..size
    std::vector<Group*> groupList;
	Group * g;
    if(x == 20 && y == 20)		//awkward passing check FIXME
		return;
    if(x == -1 || y == -1)		//can happen from sgf, FIXME, probably related to assignCurrent call from setCurrent
		return;
    std::vector<int> neighbors = getNeighbors(coordsToKey(x,y));
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        if((g = gm[*it_neighbor]))
        {
            if(std::find(groupList.begin(), groupList.end(), g) == groupList.end())
            {
                groupList.push_back(g);
                findInvalidAdjacentGroups(g, gm, groupList);
            }
        }
    }

    std::vector<Group*>::iterator k;
	for(k = groupList.begin(); k != groupList.end(); k++)
	{
        for(int i = 0; i < (*k)->count(); i++)
            gm[(*k)->at(i)] = NULL;
		delete *k;
	}
}

void Matrix::findInvalidAdjacentGroups(Group * g, Group ** gm, std::vector<Group*> & groupList)
{
    Group * f;
    for(int n = 0; n < g->count(); ++n)
    {
        std::vector<int> neighbors = getNeighbors(g->at(n));
        std::vector<int>::iterator it_neighbor;
        for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
        {
            if((f = gm[*it_neighbor]) && f != g)
            {
                if(std::find(groupList.begin(), groupList.end(), f) == groupList.end())
                    groupList.push_back(f);
            }
        }
	}
}

/* FIXME A little confusing here is that "markChangesDirty" is called usually with an invalidateAllGroups
 * but really seems like the two could be combined somehow. 
 * On second thought, I feel like markChangesDirty might not be called from setCurrent calls but that the
 * solution is to hide it somewhere, to wrap any change to current... yes...*/
void Matrix::invalidateAllGroups(Group ** gm)
{
    std::vector<Group*> groupList;
    for(int i = 0; i < size*size; i++)
    {
        if(gm[i])
        {
            if(std::find(groupList.begin(), groupList.end(), gm[i]) == groupList.end())
                groupList.push_back(gm[i]);
            gm[i] = NULL;
        }
	}

    std::vector<Group*>::iterator k;
	for(k = groupList.begin(); k != groupList.end(); k++)
		delete *k;
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
                group = checkNeighbour(*it_neighbor,c,group);
                group = checkNeighbour(*it_neighbor,stoneNone,group);
            }
		}
		mark ++;
	}
	
	return group;
}

/*
 * Returns true if the stone at x,y belongs to a groups with only 1 liberty
 * Previously checkfalseEye worked on the scored liberties.  The problem
 * was that it missed certain eyes on disconnected groups that could be
 * connected.  I don't know why it would only look at scored liberties.
 * If group is dead that's another matter, but I don't think that's an
 * issue. 
*/
/* Irritatingly, and unlike other functions, the valid range of arguments is
 * 0<= x,y < size */
bool Matrix::checkfalseEye( int x, int y, StoneColor col)
{
    Q_ASSERT(x >= 0 && x < size && y >= 0 && y < size);
    return checkfalseEye(internalCoordsToKey(x,y),col);
}

bool Matrix::checkfalseEye( int key, StoneColor col)
{
    std::vector<int> neighbors = getNeighbors(key);
    std::vector<int>::iterator it_neighbor;
    for (it_neighbor = neighbors.begin(); it_neighbor < neighbors.end(); ++it_neighbor)
    {
        if (getStoneAt(*it_neighbor) == col)
        {
            if (countLiberties(assembleGroup(*it_neighbor,col)) == 1)
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
    matrix[key] |= MX_STONEDIRTY;
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
