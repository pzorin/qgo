/*
* matrix.h
*/

#ifndef MATRIX_H
#define MATRIX_H

#include "globals.h"
#include "group.h"

class Matrix
{
public:
	Matrix(int s=DEFAULT_BOARD_SIZE);
	Matrix(const Matrix &m);
	~Matrix();
	int getSize() const { return size; }
	void clear();
	void insertStone(int x, int y, StoneColor c, GamePhase phase = phaseOngoing);
	void removeStone(int x, int y);
	void eraseStone(int x, int y);
	StoneColor getStoneAt(int x, int y);
	MarkType getMarkAt(int x, int y);
	QString getFirstTextAvailable(MarkType t);

	void insertMark(int x, int y, MarkType t);
	void removeMark(int x, int y);
	void setMarkText(int x, int y, const QString &txt);
	const QString getMarkText(int x, int y);
	short at(int x, int y) const;
	void set(int x, int y, int n);
	void clearAllMarks();
	void clearTerritoryMarks();
	void absMatrix();
	const QString saveMarks();
	const QString saveEditedMoves(Matrix *parent=0);
	const QString printMe(ASCII_Import *charset);

	void checkNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties);
	Group* checkNeighbour(int x, int y, StoneColor color, Group *group);
	int countLiberties(Group *group);
	int countScoredLiberties(Group *group);
	void traverseTerritory( int x, int y, StoneColor &col);
	bool checkNeighbourTerritory( const int &x, const int &y, StoneColor &col);
	void checkScoredNeighbourLiberty(int x, int y, QList<int> &libCounted, int &liberties);
	Group* assembleGroup(MatrixStone *stone);
	bool checkFalseEye( int x, int y, StoneColor col);
	void toggleGroupAt( int x, int y );

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
	short **matrix;
	int size;
	QStringList *markTexts;
};

#endif
