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


#ifndef BOARD_H
#define BOARD_H

#include "defines.h"

#include <QGraphicsView>

enum CursorType { cursorIdle, cursorGhostBlack , cursorGhostWhite , cursorWait , cursorNavTo };
enum CoordType { uninit, sgf, numbertopnoi, numberbottomi };

class ImageHandler;
class Move;
class Mark;
class Stone;
class Gatter;

class Board : public QGraphicsView
{
	Q_OBJECT

public:
	Board(QWidget *parent=0, QGraphicsScene *c=0 );
	~Board();
	void clearData();
    void init(int size);
    void setCoordType(CoordType c) { coordType = c; }
	void setShowCoords(bool b);
    void resizeBoard();

	void removeGhosts();
	void setVarGhost(StoneColor c, int x, int y);
    void setMark(int x, int y, MarkType t, bool update=true, QString txt=QString::null, bool overlay=true);
	Mark* hasMark(int x, int y);
    void removeMark(int x, int y, bool update = false);
    void updateLastMove(Move * move);

//	Stone* addStoneSprite(StoneColor c, int x, int y, bool shown=true);//TODO shown ?
    void updateStone(StoneColor c, int x, int y, bool dead = false);
	void setCursorType(CursorType cur);
//	void updateDeadMarks(int &black, int &white);
	unsigned int getSize()	{return board_size;}
	void exportPicture(const QString &fileName,  QString *filter, bool toClipboard);
    bool updateAll(Move *move);
    void updateVariationGhosts(Move *m);
		
signals:
	void signalClicked(bool , int, int, Qt::MouseButton );
	void signalWheelEvent(QWheelEvent *);


protected:
	void calculateSize();
    void drawBack(); // old drawBackground
	void initGatter();
	void drawGatter();
	void setupCoords(void);
	void drawCoordinates();
//	void drawStarPoint(int x, int y);
	int convertCoordsToPoint(int c, int o);
	void resizeEvent(QResizeEvent*);
	QHash<int,Stone *> *stones;
	QList<Mark*> *marks;
	QList<Stone*> *ghosts;
	QList<QGraphicsSimpleTextItem*> *hCoords1, *hCoords2 , *vCoords1,*vCoords2 ;
    // Accessed by DisplayBoard:
    bool isDisplayBoard;
    ImageHandler *imageHandler;
    bool showCoords;

private:

	QGraphicsScene *canvas;
	QGraphicsRectItem *table;
	Gatter *gatter;
//	GamePhase gamePhase;

	int board_size, offset, offsetX, offsetY, square_size, board_pixel_size;
	bool showSGFCoords;
	bool showCursor;
	bool antiClicko;
	bool lockResize ;
	bool localSound;
    bool oneColorGo;

	Mark *lastMoveMark;
	bool numberPool[400];
	bool letterPool[52];
	Stone *curStone;
	CursorType cursor;
	CoordType coordType;
	short curX, curY;
	short downX, downY;	//FIXME can we combine safely somehow with curX, curY?

	static long coordsToKey(int x, int y)	
		{ return x * 100 + y; }
	static void keyToCoords(long key, int &x, int &y)	
		{ x = key / 100; y = key - x*100; }

	int hasStone(int x, int y);
	Stone* getStoneAt(int x, int y) { return stones->find(coordsToKey(x, y)).value(); }

	bool hasVarGhost(StoneColor c, int x, int y);

	void removeLastMoveMark();
	void setMarkText(int x, int y, const QString &txt);

	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent ( QMouseEvent * e ) ;
	void wheelEvent(QWheelEvent *e);
	void leaveEvent(QEvent*);

	QTime clickTime;
	Qt::MouseButton mouseState;
};

#endif
