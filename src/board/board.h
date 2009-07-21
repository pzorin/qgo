/*
 * board.h
 */

#ifndef BOARD_H
#define BOARD_H

#include "defines.h"

#include <QtGui>

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
	void changeSize();
	void init(int size);
	void setShowCoords(bool b);
	void resizeBoard(int w, int h);

	void removeGhosts();
	void setVarGhost(StoneColor c, int x, int y);
	void setMark(int x, int y, MarkType t, bool update=true, QString txt=0, bool overlay=true);
	Mark* hasMark(int x, int y);
	void removeMark(int x, int y, bool update = false);
	void removeDeadMarks();
	void updateLastMove(Move * move);

//	Stone* addStoneSprite(StoneColor c, int x, int y, bool shown=true);//TODO shown ?
	bool updateStone(StoneColor c, int x, int y, bool dead = false);
	void setCursorType(CursorType cur);
//	void updateDeadMarks(int &black, int &white);
	unsigned int getSize()	{return board_size;}
	void exportPicture(const QString &fileName,  QString *filter, bool toClipboard);

signals:
	void signalClicked(bool , int, int, Qt::MouseButton );
	void signalWheelEvent(QWheelEvent *);


protected:
	bool isDisplayBoard;
	void calculateSize();
	void drawBackground();
	void initGatter();
	void drawGatter();
	void drawCoordinates();
//	void drawStarPoint(int x, int y);
	int convertCoordsToPoint(int c, int o);
	void resizeEvent(QResizeEvent*);
	QHash<int,Stone *> *stones;
	QList<Mark*> *marks;
	QList<Stone*> *ghosts;
	QList<QGraphicsSimpleTextItem*> *hCoords1, *hCoords2 , *vCoords1,*vCoords2 ;

private:

	QGraphicsScene *canvas;
	Gatter *gatter;
	ImageHandler *imageHandler;
//	GamePhase gamePhase;

	int board_size, offset, offsetX, offsetY, square_size, board_pixel_size;
	bool showCoords;
	bool showSGFCoords;
	bool showCursor;
	bool antiClicko;
	bool lockResize ;
	bool localSound;

	Mark *lastMoveMark;
	bool numberPool[400];
	bool letterPool[52];
	Stone *curStone;
	CursorType cursor;
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
