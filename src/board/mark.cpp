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


#include "mark.h"
#include "imagehandler.h"

Mark::Mark(int x, int y)
: myX(x), myY(y)
{
}

Mark::~Mark()
{
}

/*
* MarkSquare
*/

MarkSquare::MarkSquare(int x, int y, double size, QGraphicsScene *canvas, QColor col)
: QGraphicsRectItem(0),
Mark(x, y)
{
    canvas->addItem(this);
//	if (setting->readBoolEntry("SMALL_MARKS"))
//		size_d *= 0.85;

//	if (setting->readBoolEntry("BOLD_MARKS"))
		setPen(QPen(col, 2));
//	else
//		setPen(QPen(col, 1));
	setSize(size, size);
	setZValue(10);
}

/*
* MarkCircle
*/

MarkCircle::MarkCircle(int x, int y, double size,QGraphicsScene *canvas, QColor col, bool s)
: QGraphicsEllipseItem(0),
Mark(x, y),
small(s)
{
    canvas->addItem(this);
//	if (setting->readBoolEntry("BOLD_MARKS"))
		setPen(QPen(col, 2));
//	else
//		setPen(QPen(col, 1));
	setSize(size, size);
	setZValue(10);
}
/*
void MarkCircle::drawShape(QPainter & p)
{
	p.setPen(pen());
	p.drawEllipse((int)(x()-rect().width()/2.0+0.5),(int)( y()-rect().height()/2.0+0.5), (int)rect().width(), (int)rect().height());
}
*/
void MarkCircle::setSize(double x, double y)
{
	double m;
//	if (setting->readBoolEntry("SMALL_MARKS"))
//		m = 0.4;
//	else
		m = 0.5;

	if (!small)
	{
		x *= 1.25*m;
		y *= 1.25*m;
	}
	else
	{
		x *= m;
		y *= m;
	}
 
	QGraphicsEllipseItem::setRect(rect().x()/*myX*/, rect().y()/*myY*/, x, y);


}

/*
* MarkTriangle
*/
MarkTriangle::MarkTriangle(int x, int y, double s, QGraphicsScene *canvas, QColor col)
: QGraphicsPolygonItem(0),
Mark(x, y)
{
    canvas->addItem(this);
//	if (setting->readBoolEntry("BOLD_MARKS"))
		setPen(QPen(col, 2));
//	else

	pa = QPolygon(3);

//		setPen(QPen(col, 1));
	setSize(s, s);
//	setPolygon(pa);
	setZValue(10);
}
/*
void MarkTriangle::drawShape(QPainter &p)
{
	p.setPen(pen());
	p.drawPolygon(pa);//poly);
}
*/
void MarkTriangle::setSize(double w, double)
{
//	if (setting->readBoolEntry("SMALL_MARKS"))
//		size = (int)(w*0.45);
//	else
		size = (w*0.55);
	
//	QPolygon pa(3);
//	pa[0] = QPoint(0, 0);
//	pa[1] = QPoint(size/2, -size);
//	pa[2] = QPoint(size, 0);
	pa.setPoint(0,(int)(size/2), 0);
	pa.setPoint(1,0, (int)size);
	pa.setPoint(2,(int)size, (int)size);
	setPolygon(pa);
}

/*
* MarkCross
*/

MarkCross::MarkCross(int x, int y, double s, QGraphicsScene *canvas, QColor col, bool plus)
: QGraphicsLineItem(0),
Mark(x, y), size(s)
{
    canvas->addItem(this);

	plussign = plus;
	ol = NULL;

	int penWidth;
//	if (setting->readBoolEntry("BOLD_MARKS"))
		penWidth = 2;
//	else
//		penWidth = 1;

	setPen(QPen(col, penWidth));
	setSize(s, s);
	setZValue(10);
	
	ol = new MarkOtherLine(canvas);
	if (plussign)
		ol->setLine(0, size/2, size, size/2);
	else
		ol->setLine(0, size, size, 0);
	ol->setPen(QPen(col, penWidth));
	ol->setZValue(10);
	ol->show();
}

MarkCross::~MarkCross()
{
	if (ol != NULL)
		ol->hide();
	/* There's something funny here.  A double free with
	 * something later. FIXME.  Here's a start on it. */
	//delete ol;
	//ol = NULL;
	hide();
}

void MarkCross::setSize(double w, double)
{
//	if (setting->readBoolEntry("SMALL_MARKS"))
		size = (w*0.45);
//	else
//		size = (int)(w*0.55);
	
	if (plussign)
	{
		setLine(size/2, 0, size/2, size);
		if (ol != NULL)
			ol->setLine(0, size/2, size, size/2);
	}
	else
	{
		setLine(0, 0, size, size);
		if (ol != NULL)
			ol->setLine(0, size, size, 0);
	}
}

void MarkCross::setColor(const QColor &col)
{
	QGraphicsLineItem::setPen(col);
	if (ol != NULL)
		ol->setPen(col);
}

/*
* MarkText
*/

bool MarkText::useBold = false;
int MarkText::maxLength = 1;

MarkText::MarkText( int x, int y, double size, const QString &txt,
			 QGraphicsScene *canvas, QColor col, short c, bool bold, bool  /*overlay*/)
             : QGraphicsSimpleTextItem(txt,0),
			 Mark(x, y), curSize(size), counter(c)
{
    canvas->addItem(this);

//	rect = NULL;
	useBold = bold;
//	maxLength = 1;

	if (text().length() > maxLength)
		maxLength = text().length();
	
	setSize(size, size);
	setColor(col);

	setZValue(10);
}

MarkText::~MarkText()
{
//	if (rect != NULL)
//		rect->hide();
//	delete rect;
//	hide();
}

/* FIXME This changes the pen whenever any mark is "setSize"d for all
 * marks.  It looks really bad for high numbers since they alter the
 * low numbers and the low numbers keep that small size, even when
 * one starts a new board.  Which is actually weird and likely because
 * the old marks were not deleted and are still being drawn !!!!
 * But I see the logic in having a common font size in general which
 * means either changing it for text versus numbers or having it adjust.
 * The deletion of marks though is a serious issue !!! */
void MarkText::setSize(double x, double)
{
	curSize = x;
	
//	if (setting->readBoolEntry("SMALL_MARKS"))
		x *= 0.5;
//	else
//		x *= 0.6;
	
	// adjust font size, so all labels have the same size.
//	if (setting->readBoolEntry("ADJ_FONT") && maxLength > 1)
		x /= (double)(maxLength);// * 0.6;

//	QFont f = setting->fontMarks;
	QFont f("",(int) x);// = QFont::QFont();
//	if (setting->readBoolEntry("VAR_FONT"))
	
//	font().setPointSize((int)x);

//	f.setPointSize((int)x);
	f.setBold(false);
	f.setStyleStrategy(QFont::NoAntialias);
//	f.setWeight(10);
	setFont(f);

	width = (int)boundingRect().width();
	height = (int)boundingRect().height();


}

MarkSmallStoneTerr::MarkSmallStoneTerr(int x, int y, double s, StoneColor c, QList<QPixmap> * p, QGraphicsScene *canvas)
    : Mark(x, y), QGraphicsPixmapItem(0), _x(x), _y(y), col(c), size(s)
{
    canvas->addItem(this);

	//FIXME the ZValue keeps the gatter and hoshi marks from messing with
	//the territory marks, but I get the feeling that ZValues are inconsistently
	//used and that the gatter drawing could be significantly sped up
	setZValue(1);
	setPixmap(p);
}

void MarkSmallStoneTerr::setPixmap(QList <QPixmap> * p)
{
	if (p->count() <= 2)
		QGraphicsPixmapItem::setPixmap(p->at( col == stoneBlack ? 0 : 1));
	else	
		QGraphicsPixmapItem::setPixmap(p->at( col == stoneBlack ? 0 : (rand() % (p->count() -2) ) + 1));
}		
