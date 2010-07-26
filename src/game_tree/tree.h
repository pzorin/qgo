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
#include "matrix.h"

#include <QtCore>

class Move;

class Tree
{
public:
	Tree(int * board_size);
	~Tree();
	void init(int * board_size);
	bool addBrother(Move *node);
	bool addSon(Move *node);
	int getNumBrothers();
	int getNumSons(Move *m=0);
	int getBranchLength(Move *node=0);
	Move* nextMove();
	Move* previousMove();
	Move* nextVariation();
	Move* previousVariation();
	bool hasSon(Move *m);
	bool hasPrevBrother(Move *m=0);
	bool hasNextBrother();
	void clear();
	void traverseClear(Move *m);
	int count();
	Move* getCurrent() const { return current; }
	void setCurrent(Move *m);
	void setToFirstMove();
	Move* getRoot() const { return root; }
	void setRoot(Move *m) { root = m; }
	int mainBranchSize();
	Move* findMoveInMainBranch(int x, int y) { return findMove(root, x, y, false); }
	Move * findMoveInCurrentBranch(int x, int y) { return findMove(root, x, y, true); }
	Move* findMoveInBranch(int x, int y) { return findMove(current, x, y, true); }
	Move* findLastMoveInMainBranch();
	Move* findNode(Move *m, int node);
	void traverseFind(Move *m, int x, int y, QStack<Move*> &result);

/*
 * Former Boardhandler functions called by SGF parser
 */

	void addEmptyMove(bool brother =false);
	void doPass(bool sgf, bool fastLoad = false);
	
	void editMove(StoneColor c, int x, int y);
	//int addStoneSGF(StoneColor c, int x, int y, bool new_node, bool dontplayyet = false);
	void addStoneToCurrentMove(StoneColor c, int x, int y);
	void deleteNode();

/*
 * Former Stonehandler functions called by addStoneSGF
 * Those functions are used when adding a stone, and check all Go issues : libertes, death, ...
 */
	bool checkMoveIsValid(StoneColor c, int x, int y);
	void addStone(StoneColor c, int x, int y);
	void addMove(StoneColor c, int x, int y);
	void addStoneOrLastValidMove(StoneColor c = stoneNone, int x = -1, int y = -1);
	void undoMove(void);

	int checkPosition(MatrixStone *s, Matrix *m);
//	Group* assembleGroup(MatrixStone *stone, Matrix *m);
//	Group* checkNeighbour(int x, int y, StoneColor color, Group *group, Matrix *m);// TODO remove
	bool removeStone(int x, int y, bool hide=false);
	int hasMatrixStone(int x, int y);
//	int countLiberties(Group *group, Matrix *m); // TODO remove
//	void checkNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties, Matrix *m);// TODO remove
	//bool updateAll(Matrix *m, bool toDraw=true);

	bool insertStone(Move *node);
	void setInsertStone (bool val) { insertStoneFlag = val; };
	bool getInsertStone () { return insertStoneFlag; };

protected:
	Move* findMove(Move *start, int x, int y, bool checkmarker);
	
private:
	void checkAddKoMark(StoneColor c, int x, int y, Move * m = NULL);
	int getLastCaptures(Move * m);
	Move * assignCurrent(Move * & o, Move * & n);
	void invalidateAdjacentCheckPositionGroups(MatrixStone m);
	void invalidateCheckPositionGroups(void);
	bool isInMainBranch(Move * m) const;
	void updateCurrentMatrix(StoneColor c, int x, int y);
	void deleteGroupMatrices(void);
	Move *root, *current, *lastMoveInMainBranch;
	int * boardSize;
	//QList<Group *> *groups;
	QHash<int,MatrixStone *> *stones;
	Matrix * checkPositionTags;
	Group *** groupMatrixView;
	Group *** groupMatrixCurrent;

	int koStoneX;
	int koStoneY;
	int lastCaptures;
	Move * lastValidMoveChecked;
 // true - insert stone directly to position, false - insert stone as new variation (default)
	bool insertStoneFlag;
};

#endif
