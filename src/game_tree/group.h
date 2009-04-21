/*
* group.h
*/

#ifndef GROUP_H
#define GROUP_H

#include "defines.h"

class Group : public QList<MatrixStone *>
{
public:
	Group();
	~Group();
	
	/*virtual*/ void append(MatrixStone * m) { QList<MatrixStone *>::append(m); };
	/*virtual*/ void append(MatrixStone * m, Group *** groupMatrix) { QList<MatrixStone *>::append(m); groupMatrix[m->x - 1][m->y - 1] = this; };
	/*virtual*/ int compareItems(MatrixStone *d1, MatrixStone *d2);
	void remove(MatrixStone * m);
	bool isAttachedTo(MatrixStone *s);
	int liberties;

#ifndef NO_DEBUG
	void debug();
#endif
};

#endif
