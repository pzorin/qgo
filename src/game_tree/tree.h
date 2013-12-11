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


#ifndef TREE_H
#define TREE_H

#include "defines.h"

#include <QtCore>

class Move;
class Group;
class Matrix;

class Tree
{
public:
    Tree(int board_size);
	~Tree();
    void init();
    int getNumSons(Move *m=0);
	int getBranchLength(Move *node=0);
	Move* nextMove();
	Move* previousMove();
	Move* nextVariation();
	Move* previousVariation();
    Move* getCurrent() const { return current; }
	void setCurrent(Move *m);
	void setToFirstMove();
	Move* getRoot() const { return root; }
    Move * findMoveInMainBranch(int x, int y) { return findMove(root, x, y, false); }
	Move * findMoveInCurrentBranch(int x, int y) { return findMove(root, x, y, true); }
    Move * findLastMoveInMainBranch();
	Move * findLastMoveInCurrentBranch();
	Move * findNode(Move *m, int node);
    bool isInMainBranch(Move * m) const;
/*
 * Former Boardhandler functions called by SGF parser
 */

	void addEmptyMove(bool brother =false);
	void doPass(bool sgf, bool fastLoad = false);
	
    void addStoneToCurrentMove(StoneColor c, int x, int y);
	void deleteNode();

/*
 * Former Stonehandler functions called by addStoneSGF
 * Those functions are used when adding a stone, and check all Go issues : libertes, death, ...
 */
	bool checkMoveIsValid(StoneColor c, int x, int y);
    void addMove(StoneColor c, int x, int y);
    void addLastValidMove();
	void undoMove(void);

	bool insertStone(Move *node);
    void setInsertStone (bool val) { insertStoneFlag = val; }
    bool getInsertStone () { return insertStoneFlag; }
    void setLoadingSGF(bool b) { loadingSGF = b; }
	
protected:
	Move* findMove(Move *start, int x, int y, bool checkmarker);
	
private:
    bool addBrother(Move *node);
    bool addSon(Move *node);
    bool hasSon(Move *m);
    bool hasPrevBrother(Move *m=0);
    bool hasNextBrother();
    void clear();
    void traverseClear(Move *m);
    int count();
    void setRoot(Move *m) { root = m; }
    int mainBranchSize();
    void traverseFind(Move *m, int x, int y, QStack<Move*> &result);

    void checkAddKoMark(StoneColor c, int x, int y, Move * m = NULL);
	int getLastCaptures(Move * m);
	Move * assignCurrent(Move * & o, Move * & n);
    void invalidateCheckPositionGroups(void);
	void updateCurrentMatrix(StoneColor c, int x, int y);
	void deleteGroupMatrices(void);

    const int boardSize;

	Move *root, *current, *lastMoveInMainBranch;
	Matrix * checkPositionTags;

    Group ** groupMatrixView;
    Group ** groupMatrixCurrent;

	int koStoneX;
	int koStoneY;
	int lastCaptures;
	Move * lastValidMoveChecked;
 // true - insert stone directly to position, false - insert stone as new variation (default)
	bool insertStoneFlag;
	bool loadingSGF;
};

#endif
