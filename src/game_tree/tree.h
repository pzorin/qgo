/*
* tree.h
*/

#ifndef TREE_H
#define TREE_H

#include "defines.h"
#include "matrix.h"
//#include "group.h"

#include <QtCore>

class Move;

//template<class type> class QStack;

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

	void createEmptyMove(bool brother =false);
	void removeStoneSGF(int x, int y, bool hide=false, bool new_node=true);
	void updateCurrentMatrix(StoneColor c, int x, int y, GamePhase gamePhase = phaseOngoing);
 	//void addMove(StoneColor c, int x, int y, bool clearMarks = true);
	//void addMove(StoneColor c, int x, int y, Matrix * mat, bool clearMarks = true);
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

protected:
	Move* findMove(Move *start, int x, int y, bool checkmarker);
	
private:
	void checkAddKoMark(StoneColor c, int x, int y, Move * m = NULL);
	int getLastCaptures(Move * m);
	Move * assignCurrent(Move * & o, Move * & n);
	void invalidateAdjacentCheckPositionGroups(MatrixStone m);
	void invalidateCheckPositionGroups(void);
	bool isInMainBranch(Move * m) const;
	void updateMatrix(Matrix * m, StoneColor c, int x, int y, GamePhase gamephase = phaseOngoing);
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
};

#endif
