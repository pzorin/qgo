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


#include "../defines.h"
#include "gatter.h"

#include <QList>
#include <QtWidgets>


/*************************************************************
 *
 * Initialises the gatter intersections and hoshis points
 *
 *************************************************************/
Gatter::Gatter(QGraphicsScene *canvas, int size)
{
	board_size = size;

    VGatter.reserve(board_size*board_size);
    HGatter.reserve(board_size*board_size);
    QGraphicsLineItem *tmp1;
    for (int i=0; i<board_size*board_size; i++)
    {
        tmp1 = new QGraphicsLineItem(0);
        canvas->addItem(tmp1);
        VGatter[i] = tmp1;
        tmp1 = new QGraphicsLineItem(0);
        canvas->addItem(tmp1);
        HGatter[i] = tmp1;
    }
	
    QGraphicsEllipseItem *tmp2;
    int edge_dist = (board_size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (board_size + 1) / 2;
	int high = board_size + 1 - edge_dist;
	if (board_size % 2 && board_size > 9)
	{
        tmp2 = new QGraphicsEllipseItem(0);
        hoshisList.insert(middle*board_size + low , tmp2);
        canvas->addItem(tmp2);
        tmp2 = new QGraphicsEllipseItem(0);
        hoshisList.insert(middle*board_size + middle , tmp2);
        canvas->addItem(tmp2);
        tmp2 = new QGraphicsEllipseItem(0);
        hoshisList.insert(middle*board_size + high , tmp2);
        canvas->addItem(tmp2);
        tmp2 = new QGraphicsEllipseItem(0);
        hoshisList.insert(low*board_size + middle , tmp2);
        canvas->addItem(tmp2);
        tmp2 = new QGraphicsEllipseItem(0);
        hoshisList.insert(high*board_size + middle , tmp2);
        canvas->addItem(tmp2);
    }
    tmp2 = new QGraphicsEllipseItem(0);
    hoshisList.insert(low*board_size + low , tmp2);
    canvas->addItem(tmp2);
    tmp2 = new QGraphicsEllipseItem(0);
    hoshisList.insert(high*board_size + low , tmp2);
    canvas->addItem(tmp2);
    tmp2 = new QGraphicsEllipseItem(0);
    hoshisList.insert(high*board_size + high , tmp2);
    canvas->addItem(tmp2);
    tmp2 = new QGraphicsEllipseItem(0);
    hoshisList.insert(low*board_size + high , tmp2);
    canvas->addItem(tmp2);


	QMapIterator<int,QGraphicsEllipseItem*> it( hoshisList );

	for ( ; it.hasNext() ; )
	{
		it.next();
		it.value()->setBrush(Qt::SolidPattern);
		it.value()->setPen(Qt::NoPen);
	}

	showAll();
}

/*
 * Destroys the gatter
 */
Gatter::~Gatter()
{
    qDeleteAll(HGatter);
    qDeleteAll(VGatter);
    qDeleteAll(hoshisList);
}


/*
 * Calculates the gatter intersections and hoshis position
 */
void Gatter::resize(int offsetX, int offsetY, int square_size)
{
	int size = square_size / 5;

	// Round size top be odd (hoshis)
	if (size % 2 == 0)
		size--;
	if ((size < 7) && (size>2))
		size = 7;
	else if (size <= 2)
		size = 3;
	

    // indexOf accepts coordinated between 1 and board_size, FIXME iterator
    for (int i=0; i<board_size; i++)
        for (int j=0; j<board_size; j++)
		{
            HGatter[indexOf(i+1,j+1)]->setLine(int(offsetX + square_size * ( i - 0.5*(i!=0))),
						offsetY + square_size * j,
                        int(offsetX + square_size * ( i + 0.5 * (i+1 != board_size))),
						offsetY + square_size * j );
			
            VGatter[indexOf(i+1,j+1)]->setLine(offsetX + square_size *  i,
						int(offsetY + square_size * ( j - 0.5*(j!=0))),
                        offsetX + square_size *  i,
                        int(offsetY + square_size * ( j + 0.5 * (j+1 != board_size))));
			
            if (hoshisList.contains(indexOf(i+1,j+1)))
                    hoshisList.value(indexOf(i+1,j+1))->setRect(offsetX + square_size * i - size/2,
                                                                offsetY + square_size * j- size/2,
                                                                size ,
                                                                size );
		}

}

/*
* Resets all intersections and hoshis to be shown
*/
void Gatter::showAll()
{
    for (int i=0; i<board_size*board_size; i++)
    {
        VGatter[i]->show();
        HGatter[i]->show();
    }

	QMapIterator<int,QGraphicsEllipseItem*> it( hoshisList );

	for ( ; it.hasNext() ; )
	{
		it.next();
		it.value()->show();
	}

}

/*
 * Hides an intersection (when placing a letter mark)
 */
void Gatter::hide(int x, int y)
{
    if (( x<1) || (x > board_size) || ( y<1) || (y > board_size))
		return;

    VGatter[indexOf(x,y)]->hide();
    HGatter[indexOf(x,y)]->hide();

    if (hoshisList.contains(indexOf(x,y)))
        hoshisList.value(indexOf(x,y))->hide();
}

/*
 * shows an intersection (when removing a letter mark)
 */
void Gatter::show(int x, int y)
{
    if (( x<1) || (x > board_size) || ( y<1) || (y > board_size))
		return;

    VGatter[indexOf(x,y)]->show();
    HGatter[indexOf(x,y)]->show();

    if (hoshisList.contains(indexOf(x,y)))
        hoshisList.value(indexOf(x,y))->show();
}
