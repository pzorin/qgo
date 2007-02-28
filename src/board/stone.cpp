/*
* stone.cpp
* This class handles the graphical appearance of the stones on the board
* Is is used only by the 'board' class
*/

#include "defines.h"
#include "stone.h"
#include "imagehandler.h"

#include <stdlib.h>

/*
 * Creates the stone as a GraphicsScene item
 */
Stone::Stone(QList<QPixmap> *a, QGraphicsScene *canvas, StoneColor c, int x, int y , bool has_shadow)
: QGraphicsPixmapItem(0, canvas), color(c), myX(x), myY(y)
{
	pixmapList = a;

	// We use a trick here with the count of the pixmap list
	// because the alternate ghosts list has only 2 images

			
//	int ii = (rand() % (pixmapList->count() -2) ) + 1;
//	if (has_shadow)
//		qDebug("nb of im : %d - index chosen %d", pixmapList->count(), ii);
	
	if (pixmapList->count() <= 2)
		setPixmap(pixmapList->at( color == stoneBlack ? 0 : 1));
	else	
		setPixmap(pixmapList->at( color == stoneBlack ? 0 : (rand() % (pixmapList->count() -2) ) + 1));

	shadow = NULL;
	
	if (has_shadow) {
		shadow = new QGraphicsPixmapItem(0, canvas);
		shadow->setPixmap(pixmapList->last());
		shadow->setZValue(4);
	}
	
	setZValue(5);
	show();
	
	dead = false;
	seki = false;
	checked = false;
	
}

Stone::~Stone()
{
	delete shadow;
} 

/*
 * Resets the stone image with the color (used when resizing the board)
 */
void Stone::setColor(StoneColor c)
{
	setPixmap(pixmapList->at( c == stoneBlack ? 0 : (rand() % WHITE_STONES_NB) + 1  ));
	color = c;
	
	if (shadow)
		shadow->setPixmap(pixmapList->last());
}

/*
 * self expl.
 */
void Stone::toggleOneColorGo(bool oneColor)
{
	// if we play one color go, the black stones are repainted white, else normal black
	if (color ==  stoneBlack)
		setPixmap(pixmapList->at( oneColor ? (rand() % WHITE_STONES_NB) + 1 : 0 ));
}

/*
 * Moves the stone to position (in pixels)
 */
void Stone::setPos(double x, double y)
{
	
	QGraphicsPixmapItem::setPos((qreal)x,(qreal)y);
	qreal offset;
	if (shadow) {
		 offset = shadow->boundingRect().height();
		 shadow->setPos((qreal)(x-offset / 8 ), (qreal)(y+ offset / 8));
	}
	
}


void Stone::hide()
{
	QGraphicsPixmapItem::hide();
	if (shadow) 	 shadow->hide();
	
}

void Stone::show()
{
	QGraphicsPixmapItem::show();
	if (shadow) 	 shadow->show();
	
}
