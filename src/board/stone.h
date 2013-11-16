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


#ifndef STONE_H
#define STONE_H

#include "graphicsitemstypes.h"

#include <QGraphicsItem>
#include <QtCore>

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
	void togglePixmap(QList<QPixmap> *a, bool showShadow = true);

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
