/*
* group.h
*/

#ifndef GROUP_H
#define GROUP_H

#include "defines.h"

//class Stone;

class Group : public QList<MatrixStone *>
{
public:
	Group();
	virtual ~Group();// {}
	
	int getLiberties() const { return liberties; }
	void setLiberties(int l) { liberties = l; }
	virtual int compareItems(MatrixStone *d1, MatrixStone *d2);
	bool isAttachedTo(MatrixStone *s);
#ifndef NO_DEBUG
	void debug();
#endif
	
private:
	int liberties;
};

#endif
