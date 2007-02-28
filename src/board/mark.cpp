/*
* mark.cpp
*/

#include "mark.h"
#include "imagehandler.h"


/*
* Mark
*/

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

MarkSquare::MarkSquare(int x, int y, int size, QGraphicsScene *canvas, QColor col)
: QGraphicsRectItem(0,canvas),
Mark(x, y)
{
	double size_d = (double)size;

//	if (setting->readBoolEntry("SMALL_MARKS"))
//		size_d *= 0.85;

//	if (setting->readBoolEntry("BOLD_MARKS"))
//		setPen(QPen(col, 2));
//	else
		setPen(QPen(col, 1));
	setSize(size_d, size_d);
	setZValue(10);
}

/*
* MarkCircle
*/

MarkCircle::MarkCircle(int x, int y, int size,QGraphicsScene *canvas, QColor col, bool s)
: QGraphicsEllipseItem(0,canvas),
Mark(x, y),
small(s)
{
//	if (setting->readBoolEntry("BOLD_MARKS"))
//		setPen(QPen(col, 2));
//	else
		setPen(QPen(col, 1));
	setSize((double)size, (double)size);
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
 
	QGraphicsEllipseItem::setRect(myX, myY, x, y);
}

/*
* MarkTriangle
*/
MarkTriangle::MarkTriangle(int x, int y, int s,QGraphicsScene *canvas, QColor col)
: QGraphicsPolygonItem(0,canvas),
Mark(x, y)
{
//	if (setting->readBoolEntry("BOLD_MARKS"))
//		setPen(QPen(col, 2));
//	else

	pa = QPolygon::QPolygon(3);

		setPen(QPen(col, 1));
	setSize((double)s, (double)s);
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
		size = (int)(w*0.55);
	
//	QPolygon pa(3);
//	pa[0] = QPoint(0, 0);
//	pa[1] = QPoint(size/2, -size);
//	pa[2] = QPoint(size, 0);
	pa.setPoint(0,size/2, 0);
	pa.setPoint(1,0, size);
	pa.setPoint(2,size, size);
	setPolygon(pa);
}

/*
* MarkCross
*/

MarkCross::MarkCross(int x, int y, int s, QGraphicsScene *canvas, QColor col, bool plus)
: QGraphicsLineItem(0,canvas),
Mark(x, y), size(s)
{
	plussign = plus;
	ol = NULL;

	int penWidth;
//	if (setting->readBoolEntry("BOLD_MARKS"))
//		penWidth = 2;
//	else
		penWidth = 1;

	setPen(QPen(col, penWidth));
	setSize((double)s, (double)s);
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
	delete ol;
	hide();
}

void MarkCross::setSize(double w, double)
{
//	if (setting->readBoolEntry("SMALL_MARKS"))
//		size = (int)(w*0.45);
//	else
		size = (int)(w*0.55);
	
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

MarkText::MarkText( int x, int y, int size, const QString &txt,
			 QGraphicsScene *canvas, QColor col, short c, bool bold, bool  /*overlay*/)
			 : QGraphicsSimpleTextItem(txt,0, canvas),
			 Mark(x, y), curSize(size), counter(c)
{
//	rect = NULL;
	useBold = bold;
//	maxLength = 1;

	if (text().length() > maxLength)
		maxLength = text().length();
	
	setSize(size, size);
	setColor(col);
/*	
	QPixmap * pix = new QPixmap(width, height);
	pix->fill(Qt::white);
	//bitBlt((QPaintDevice *)(pix),0,0,(QPaintDevice *)(ih->getBoardPixmap(setting->readEntry("SKIN"))),x,y,width,height,Qt::CopyROP);
	//bitBlt((QPaintDevice *)(pix),0,0,(QPaintDevice *)(ih->getTablePixmap(setting->readEntry("SKIN_TABLE"))),x,y,5,5,Qt::CopyROP,false);
	copyBlt(pix,0,0,ih->getTablePixmap(setting->readEntry("SKIN_TABLE")),size * (x-1) - width/2,size * (y-1) - height/2,8,8);
	//pix.fill((QWidget *)(canvas), x,y);

	if (overlay)
	{
	
	// we use a rectangle under the letter so as not to have the underlying grid
	// lines interfere with the letter
	// The only case where it is not set to 'true' is when a pass move is indicated,
	// bottom right off the board. This is an 'over'kill
	
		CHECK_PTR(canvas);
		rect = new QCanvasRectangle(canvas);
		CHECK_PTR(rect);
		rect->setPen(NoPen);
		CHECK_PTR(ih);
		rect->setSize(width, height);
		//rect->setX(size * (x-1) - width/2);
		//rect->setX(size * (y-1) - height/2);
		//rect->setBrush(QBrush(white, *(ih->getBoardPixmap(static_cast<skinType>(setting->readIntEntry("SKIN"))))));
		rect->setBrush(QBrush(white, *(ih->getBoardPixmap(setting->readEntry("SKIN")))));
		
		rect->setZ(1);
//		rect->show();
	}

*/

	setZValue(10);
}

MarkText::~MarkText()
{
//	if (rect != NULL)
//		rect->hide();
//	delete rect;
//	hide();
}

void MarkText::setSize(double x, double)
{
	curSize = x;
	
//	if (setting->readBoolEntry("SMALL_MARKS"))
//		x *= 0.5;
//	else
		x *= 0.6;
	
	// adjust font size, so all labels have the same size.
//	if (setting->readBoolEntry("ADJ_FONT") && maxLength > 1)
		x /= (double)(maxLength) * 0.6;

//	QFont f = setting->fontMarks;
//	if (setting->readBoolEntry("VAR_FONT"))
//		f.setPointSize((int)x);
//	setFont(f);

	width = (int)boundingRect().width();
	height = (int)boundingRect().height();

//	if (rect != NULL)
//		rect->setRect(rect->x(), rect->y(), width, height);
}
