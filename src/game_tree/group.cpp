/*
 * group.cpp
 */

#include "group.h"
#include "../defines.h"

Group::Group()
{
    liberties = 0;
}

Group::~Group()
{
	while (!isEmpty())
		delete takeFirst();
}


int Group::compareItems(MatrixStone *s1, MatrixStone *s2)
{
	//MatrixStone *s1 = static_cast<MatrixStone*>(d1);
	//MatrixStone *s2 = static_cast<MatrixStone*>(d2);

	Q_CHECK_PTR(s1);
	Q_CHECK_PTR(s2);

	if (s1->x == s2->x &&
	    s1->y == s2->y &&
	    s1->c == s2->c)
		return 0;

	return 1;
}

void Group::remove(MatrixStone * m)
{
	for (int i=0; i<count(); i++)
	{
		if(at(i)->x == m->x && at(i)->y == m->y)
		{
			removeAt(i);
			break;
		}
	}
}

bool Group::isAttachedTo(MatrixStone *s)
{
    Q_CHECK_PTR(s);

    int stoneX = s->x,
	stoneY = s->y,
	      x, y;
    StoneColor col = s->c, c;
    MatrixStone *tmp;

    if (isEmpty())
		return false;
    
    for (int i=0; i<count(); i++)
    {
		tmp = at(i);
		x = tmp->x;
		y = tmp->y;
		c = tmp->c;
		if (((stoneX == x && (stoneY == y-1 || stoneY == y+1)) ||
			 (stoneY == y && (stoneX == x-1 || stoneX == x+1))) &&
			 c == col)
			return true;
    }    
    
    return false;
}

#ifndef NO_DEBUG
void Group::debug()
{
	qDebug(QString("Count: %1 - Liberties: %2").arg(count()).arg(liberties).toLatin1().constData());
	MatrixStone *s;

	//QList<MatrixStone*>::Iterator it(this);
	QListIterator<MatrixStone*> it( *this );
	
	//for ( it=*this.begin() ; it !=*this.end(); ++it)
	for ( ; it.hasNext() ; )
	{
		s=it.next();

		qDebug(" (%d, %d) %s", s->x, s->y,
		s->c == stoneBlack ? "B" : "W");
	}

	
}
#endif
