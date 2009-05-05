/*
* matrix.h
*/

#ifndef MATRIX_H
#define MATRIX_H

#include "defines.h"
#include "group.h"

class Matrix
{
public:
	Matrix(int s=DEFAULT_BOARD_SIZE);
	Matrix(const Matrix &m);
	Matrix(const Matrix &m, bool cleanup);
	Matrix & operator=(const Matrix &m);
	~Matrix();
	int getSize() const { return size; }
	void clear();
	void insertStone(int x, int y, StoneColor c, GamePhase phase);
	void removeStone(int x, int y);
	void eraseStone(int x, int y);
	StoneColor getStoneAt(int x, int y);
	bool isStoneDead(int x, int y);
	bool isStoneDirty(int x, int y);
	void stoneUpdated(int x, int y);
	void invalidateStone(int x, int y);
	void markChangesDirty(Matrix & m);
	MarkType getMarkAt(int x, int y);
	QString getFirstTextAvailable(MarkType t);

	void insertMark(int x, int y, MarkType t);
	void removeMark(int x, int y);
	void setMarkText(int x, int y, const QString &txt);
	const QString getMarkText(int x, int y);
	unsigned short at(int x, int y) const;
	void set(int x, int y, int n);
	void clearAllMarks();
	void clearTerritoryMarks();
	void absMatrix();
	const QString saveMarks();
	const QString saveEditedMoves(Matrix *parent=0);
	const QString printMe(ASCII_Import *charset);

	void checkNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties);
	Group* checkNeighbour(int x, int y, StoneColor color, Group *group, Group *** groupMatrix = NULL);
	int countLiberties(Group *group);
	int countScoredLiberties(Group *group);
	void traverseTerritory( int x, int y, StoneColor &col);
	bool checkNeighbourTerritory( const int &x, const int &y, StoneColor &col);
	void checkScoredNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties);
	Group* assembleGroup(MatrixStone *stone, Group *** groupMatrix = NULL);
	Group* assembleAreaGroups(MatrixStone *stone);
	Group* checkStoneWithGroups(MatrixStone * stone, Group *** groupMatrix, Group * joins[4], Group * enemyGroups[4]);
	void removeGroup(Group * g, Group *** groupMatrix, Group * killer);
	void removeStoneFromGroups(MatrixStone * stone, Group *** groupMatrix);
	void invalidateChangedGroups(Matrix & m, Group *** gm);
	void invalidateAdjacentGroups(MatrixStone m, Group *** gm);
	void invalidateAllGroups(Group *** gm);

	bool checkFalseEye( int x, int y, StoneColor col);
	void toggleGroupAt( int x, int y );
	void toggleStoneAt(int x, int y);
	void markStoneDead(int x, int y);
	void markGroupDead(int x, int y);
	void markGroupAlive(int x, int y);
	void toggleAreaAt( int x, int y );
	void markAreaDead(int x, int y);
	void markAreaAlive(int x, int y);
	void updateDeadMarks(int &black, int &white);

	static long coordsToKey(int x, int y)
	{ return x * 100 + y; }
	static void keyToCoords(long key, int &x, int &y)
	{ x = key / 100; y = key - x*100; }
	static const QString coordsToString(int x, int y)
	{ return (QString(QChar(static_cast<const char>('a' + x))) +
	QString(QChar(static_cast<const char>('a' + y)))); }

	

#ifndef NO_DEBUG
	void debug() const;
#endif
	
	
protected:
	void init();
	void initMarkTexts();
	QStringList::Iterator getMarkTextIterator(int x, int y);
	
private:
	void findInvalidAdjacentGroups(Group * g, Group *** gm, std::vector<Group*> & groupList);
	int sharedLibertyWithGroup(int x, int y, Group * g, Group *** gm);

	unsigned short **matrix;
	int size;
	QStringList *markTexts;
	std::vector<unsigned short>tempLibertyList;
};

#endif
