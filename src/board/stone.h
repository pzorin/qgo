/*
* stone.h
*/

#ifndef STONE_H
#define STONE_H

#include "../defines.h"

#include <QtGui>

class Stone : public QGraphicsPixmapItem
{
public:
	Stone(QList<QPixmap> *a, QGraphicsScene *canvas, StoneColor c, int x, int y, bool has_shadow=false);
	~Stone() ;
	
	StoneColor getColor() const { return color; }
	void setColor(StoneColor c=stoneBlack);
	void toggleOneColorGo(bool oneColor) ;
	int type() const { return RTTI_STONE; }
	int posX() const { return myX; }
	int posY() const { return myY; }
	QGraphicsSimpleTextItem * getNum() {return moveNum;}
	void setCoord(int x, int y) { myX = x; myY = y; }
	void togglePixmap(QList<QPixmap> *a, bool showShadow = TRUE);

	bool isDead() const { return dead; }
	void setDead(bool b=true) { dead = b; seki = false;}
	bool isSeki() const { return seki; }
	void setSeki(bool b=true) { seki = b; dead = false; }
	
	void setPos(double x, double y);
	void show();
	void hide();
	
	bool checked;
	

	
private:
	StoneColor color;
	int myX, myY;
	bool dead, seki;
	QList<QPixmap> *pixmapList;
	QGraphicsSimpleTextItem *moveNum;
	QGraphicsPixmapItem *shadow;
};

#endif
