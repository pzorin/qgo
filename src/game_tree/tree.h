/*
* tree.h
*/

#ifndef TREE_H
#define TREE_H

#include "defines.h"
#include "matrix.h"
#include "group.h"

#include <QtCore>

class Move;

//template<class type> class QStack;

class Tree
{
public:
	Tree(int board_size);
	~Tree();
	void init(int board_size);
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
	static void traverseClear(Move *m);
	int count();
	Move* getCurrent() const { return current; }
	void setCurrent(Move *m) { current = m; }
	void setToFirstMove();
	Move* getRoot() const { return root; }
	void setRoot(Move *m) { root = m; }
	int mainBranchSize();
	Move* findMoveInMainBranch(int x, int y) { return findMove(root, x, y, false); }
	Move* findMoveInBranch(int x, int y) { return findMove(current, x, y, true); }
	Move* findLastMoveInMainBranch();
	void traverseFind(Move *m, int x, int y, QStack<Move*> &result);

/*
 * Former Boardhandler functions called by SGF parser
 */

	void createMoveSGF(bool brother =false, bool fastload = false);
	void removeStoneSGF(int x, int y, bool hide=false, bool new_node=true);
	void updateCurrentMatrix(StoneColor c, int x, int y);
 	void addMove(StoneColor c, int x, int y, bool clearMarks = true);
	void doPass(bool sgf, bool fastLoad = false);
	void editMove(StoneColor c, int x, int y);
	int addStoneSGF(StoneColor c, int x, int y, bool new_node);
	void deleteNode();

/*
 * Former Stonehandler functions called by addStoneSGF
 * Those functions are used when adding a stone, and check all Go issues : libertes, death, ...
 */

	int checkPosition(MatrixStone *s,Matrix *m, bool koStone = false);
	Group* assembleGroup(MatrixStone *stone, Matrix *m);
	Group* checkNeighbour(int x, int y, StoneColor color, Group *group, Matrix *m);
	bool removeStone(int x, int y, bool hide=false);
	int hasMatrixStone(int x, int y);
	int countLiberties(Group *group, Matrix *m);
	void checkNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties, Matrix *m);
	bool updateAll(Matrix *m, bool toDraw=true);

protected:
	Move* findMove(Move *start, int x, int y, bool checkmarker);
	
private:
	Move *root, *current;
	int boardSize;
	QList<Group *> *groups;
	QHash<int,MatrixStone *> *stones;
};

#endif
