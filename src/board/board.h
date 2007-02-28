/*
 * board.h
 */

#ifndef BOARD_H
#define BOARD_H

#include "stone.h"
#include "imagehandler.h"
#include "gatter.h"
#include "mark.h"

#include <vector>

#include <QtGui>

class Board : public QGraphicsView
{
	Q_OBJECT

public:
	Board(QWidget *parent=0, QGraphicsScene *c=0 );
	~Board();
	void clearData();
	void changeSize();
	void init(int size);

	void removeGhosts();
	void setVarGhost(StoneColor c, int x, int y);
	void setMark(int x, int y, MarkType t, bool update=true, QString txt=0, bool overlay=true);
	Mark* hasMark(int x, int y);
	void removeMark(int x, int y, bool update = false);

	Stone* addStoneSprite(StoneColor c, int x, int y, bool shown=true);//TODO shown ?
	bool updateStone(StoneColor c, int x, int y);


protected:
	void calculateSize();
	void drawBackground();
	void initGatter();
	void drawGatter();
	void drawCoordinates();
	void drawStarPoint(int x, int y);
	void resizeBoard(int w, int h);
	int convertCoordsToPoint(int c, int o);
	void resizeEvent(QResizeEvent*);

private:
	QGraphicsScene *canvas;
	Gatter *gatter;
	ImageHandler *imageHandler;

	int board_size, offset, offsetX, offsetY, square_size, board_pixel_size;
	bool showCoords;
	bool showSGFCoords;
	bool showCursor;
	bool antiClicko;
	bool lockResize ;

	QHash<int,Stone *> *stones;
	QList<Mark*> *marks;
	QList<Stone*> *ghosts;
	Mark *lastMoveMark;
	bool numberPool[400];
	bool letterPool[52];

	static long coordsToKey(int x, int y)	
		{ return x * 100 + y; }
	static void keyToCoords(long key, int &x, int &y)	
		{ x = key / 100; y = key - x*100; }

	int hasStone(int x, int y);
	Stone* getStoneAt(int x, int y) { return stones->find(coordsToKey(x, y)).value(); }

	void removeLastMoveMark();
	void setMarkText(int x, int y, const QString &txt);
};

#endif
