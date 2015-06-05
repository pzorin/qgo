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


#ifndef MATRIX_H
#define MATRIX_H

#include "defines.h"

/*
* Marks used in editing a game
*/
#define MARK_TERRITORY_VISITED    0x1000

//first nibble is for stone color/erase
#define MX_STONEDEAD	0x8000
#define MX_STONEEDIT	0x4000

#define MX_VISITED	0x2000

class Group;

struct ASCII_Import
{
    char blackStone, whiteStone, starPoint, emptyPoint, hBorder, vBorder;
};

class Matrix
{
public:
    Matrix(int s=DEFAULT_BOARD_SIZE);
    Matrix(const Matrix &m, bool cleanup = false);
    ~Matrix();
	int getSize() const { return size; }
    void insertStone(int x, int y, StoneColor c, bool fEdit = false);
	StoneColor getStoneAt(int x, int y);
    bool isStoneDead(int x, int y);
    MarkType getMarkAt(int x, int y);
	QString getFirstTextAvailable(MarkType t);

	void insertMark(int x, int y, MarkType t);
	void removeMark(int x, int y);
	void setMarkText(int x, int y, const QString &txt);
	const QString getMarkText(int x, int y);
    void clearAllMarks();
	void clearTerritoryMarks();
	void absMatrix();
	const QString saveMarks();
	const QString saveEditedMoves(Matrix *parent=0);
	const QString printMe(ASCII_Import *charset);

    int checkStoneCaptures(StoneColor ourColor, int x, int y, std::vector<Group *> &visited) const;
    void removeGroup(Group * g);

    void toggleGroupAt( int x, int y );
	void toggleStoneAt(int x, int y);
	void markStoneDead(int x, int y);
	void markGroupDead(int x, int y);
	void markGroupAlive(int x, int y);
	void toggleAreaAt( int x, int y );
	void markAreaDead(int x, int y);
    void markAreaAlive(int x, int y);
    void markTerritory();
    void count(int & terrBlack, int & terrWhite, int & deadBlack, int & deadWhite);

    static const QString coordsToString(int x, int y)
    { return QString(QChar(static_cast<const char>('a' + x))).append(QChar(static_cast<const char>('a' + y))); }

    int makeMove(int x, int y, StoneColor c);
    bool addHandicapStones(int handicap);

private:
    int internalCoordsToKey(int i, int j) const { return i*size + j; }
    int coordsToKey(int x, int y) const { return internalCoordsToKey(x-1,y-1); }
    void keyToCoords(int key, int &x, int &y) const
    { x = key / size; y = key - (x++)*size + 1; }

    StoneColor getStoneAt(int key) const;
    MarkType getMarkAt(int key) const;
    void insertStone(int key, StoneColor c, bool fEdit = false);
    Group* assembleGroup(int key, StoneColor c) const;
    void toggleStoneAt(int key);

    void checkNeighbour(int key, StoneColor color, Group *group, std::vector<int> * libertyList = NULL) const;
    int countLiberties(Group *group, unsigned short mask = stoneBlack|stoneWhite);
    Group* assembleAreaGroups(int key, StoneColor c);
    // This function returns a list of keys of points adjacent to the point "key".
    std::vector<int> getNeighbors(int key) const;
    void floodTerritory(unsigned short mark, int key);

    unsigned short * matrix;
    const int size;
    QHash<int,QString> markTexts;
};

#endif
