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


/*
 * board.cpp
 * This 'board' class handles graphics (wooden and table background, 
 * gatter, coordinates, 
 * specific cursor, stones and variaton marks.
 * It does not know about the game tree or the matrix,
 * and takes its orders only from boardhandler
 */
#include "board.h"
#include "stone.h"
#include "gatter.h"
#include "mark.h"
#include "imagehandler.h"
#include "move.h"		//for updateLastMove, cleaner and yet not FIXME
#include "graphicsitemstypes.h"
#include "matrix.h"

#include <QMouseEvent>
#include <QApplication>
#include <QMessageBox>
#include <QClipboard>

class ImageHandler;


/*
 * This initialises everything on the board : background, gatter, cursor, etc ....
 */
Board::Board(QWidget *parent, QGraphicsScene *c)
: QGraphicsView(c,parent), oneColorGo(false)
{
    QSettings settings;

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    isDisplayBoard = false;
    lockResize =  false;
    showCoords = true;//TODO setting->readBoolEntry("BOARD_COORDS");
    showSGFCoords = false;//TODO setting->readBoolEntry("SGF_BOARD_COORDS");
    //antiClicko = setting->readBoolEntry("ANTICLICKO");

    curX = curY = -1;
    downX = downY = -1;

	coordType = uninit;
    marks = new QList<Mark*>;
    stones = new QHash<int,Stone *>();
    ghosts = new QList<Stone*>;
    lastMoveMark = NULL;
    gatter = NULL;

    vCoords1 = new QList<QGraphicsSimpleTextItem*>;
    hCoords1 = new QList<QGraphicsSimpleTextItem*>;
    vCoords2 = new QList<QGraphicsSimpleTextItem*>;
    hCoords2 = new QList<QGraphicsSimpleTextItem*>;

    imageHandler = new ImageHandler();
    Q_CHECK_PTR(imageHandler);

    // Init the canvas
    canvas = new QGraphicsScene(0,0, BOARD_X, BOARD_Y,this);
    Q_CHECK_PTR(canvas);
    // Set background texture
    canvas->setBackgroundBrush(QBrush(*(ImageHandler::getTablePixmap(  settings.value("SKIN_TABLE").toString()))));
    table = canvas->addRect(QRectF(),
                QPen(Qt::NoPen),
                QBrush(* (ImageHandler::getBoardPixmap(settings.value("SKIN").toString()))));

    //setRenderHints(QPainter::SmoothPixmapTransform);
    setScene(canvas);
    viewport()->setMouseTracking(true);

    init(DEFAULT_BOARD_SIZE);

    // Init the ghost cursor stone
    cursor = cursorIdle;
    curStone = new Stone(imageHandler->getGhostPixmaps(), canvas, stoneBlack, 0, 0);
    curStone->setZValue(4);
    curStone->hide();
    showCursor = false;

    setUpdatesEnabled(true);
}

Board::~Board()
{
    setUpdatesEnabled(false);

    delete stones;
    delete ghosts;
    delete marks;

    delete hCoords1;
    delete hCoords2;
    delete vCoords1;
    delete vCoords2;
    delete gatter;
    delete imageHandler;

    delete canvas; // Also deletes all remaining QGraphicsItem's
}

void Board::init(int size)
{
    board_size = size;
    if (gatter != NULL)
        delete gatter;
    gatter = new Gatter(canvas, board_size);
    setupCoords();
    resizeBoard();
}

void Board::setupCoords(void)
{
    qDeleteAll(*hCoords1);
    qDeleteAll(*hCoords2);
    qDeleteAll(*vCoords1);
    qDeleteAll(*vCoords2);
    hCoords1->clear();
    hCoords2->clear();
    vCoords1->clear();
    vCoords2->clear();
	QString hTxt,vTxt;

	// Init the coordinates
    QGraphicsSimpleTextItem *tmp;
	for (int i=0; i<board_size; i++)
	{
		switch(coordType)
		{
			case sgf:
				vTxt = QString(QChar(static_cast<const char>('a' + i)));
				hTxt = QString(QChar(static_cast<const char>('a' + i)));
				break;
			case numberbottomi:
				vTxt = QString::number(i + 1);
				hTxt = QString(QChar(static_cast<const char>('A' + i)));
				break;
			case numbertopnoi:
			default:
				vTxt = QString::number(board_size - i);
				hTxt = QString(QChar(static_cast<const char>('A' + (i<8?i:i+1))));
				break;
		}

        tmp = new QGraphicsSimpleTextItem(vTxt, 0);
        vCoords1->append(tmp);
        canvas->addItem(tmp);
        tmp = new QGraphicsSimpleTextItem(hTxt, 0);
        hCoords1->append(tmp);
        canvas->addItem(tmp);
        tmp = new QGraphicsSimpleTextItem(vTxt, 0);
        vCoords2->append(tmp);
        canvas->addItem(tmp);
        tmp = new QGraphicsSimpleTextItem(hTxt, 0);
        hCoords2->append(tmp);
        canvas->addItem(tmp);
	}
}

/*
 * cleans up the board
 */
void Board::clearData()
{
	qDeleteAll(*stones);
	stones->clear();
	qDeleteAll(*ghosts);
	ghosts->clear();
    qDeleteAll(*marks);
    marks->clear();

    update();
}

/*
* Calculates the size of the goban
*/
void Board::calculateSize()
{
	// Calculate the size values

	// distance from table edge to wooden board edge
	const int 	margin = 1,              
		w = (int)canvas->width() - margin * 2,  
		h = (int)canvas->height() - margin * 2;
		
	int table_size = (w < h ? w : h );

	// distance from edge of wooden board to playing area (grids + space for stones on 1st & last line)
	offset = table_size * 2/100 ;  


    QGraphicsSimpleTextItem *coordV = new QGraphicsSimpleTextItem(QString::number(board_size),0);
    QGraphicsSimpleTextItem *coordH = new QGraphicsSimpleTextItem("A",0);
    canvas->addItem(coordV);
    canvas->addItem(coordH);
	int coord_width = (int)coordV->boundingRect().width();
	int coord_height = (int)coordH->boundingRect().height();

	// space for coodinates if shown
	int coord_offset =  (coord_width < coord_height ? coord_height : coord_width);

	if (showCoords)
		offset = coord_offset + 2 ;

	//we need 1 more virtual 'square' for the stones on 1st and last line getting off the grid
	square_size = (table_size - 2*offset) / (board_size);  

	// Should not happen, but safe is safe.
	if (square_size == 0)
		  square_size = 1;

	// grid size
	board_pixel_size = square_size * (board_size-1) + 1 ;    
	offset =  (table_size - board_pixel_size)/2;   

	// Center the board in canvas

	offsetX = margin + (w - board_pixel_size) / 2;
	offsetY = margin + (h - board_pixel_size) / 2;

	//deletes the samples
	delete coordV;
	delete coordH;
}

/*
* actually draws the gatter lines
*/
void Board::drawGatter()
{
	gatter->resize(offsetX,offsetY,square_size);	
}

/*
* draws the goban wood texture
*/
void Board::drawBack()
{
	table->setRect(offsetX - offset,
		offsetY - offset,
		board_pixel_size + offset*2,
		board_pixel_size + offset*2);
}

/*
 * Draws the coordinates around the goban grid
 */
void Board::drawCoordinates()
{
	QGraphicsSimpleTextItem *coord;
	
	// centres the coordinates text within the remaining space at table edge
	const int coord_centre = (offset - square_size/2 )/2; 

    for (int i=0; i<board_size; i++)
	{
		// Left side
		coord = vCoords1->at(i);
		coord->setPos(offsetX - offset + coord_centre - coord->boundingRect().width()/2 , offsetY + square_size * i - coord->boundingRect().height()/2);
        coord->setVisible(showCoords);

		// Right side
		coord = vCoords2->at(i);
        coord->setPos(offsetX + board_pixel_size + offset - coord_centre - coord->boundingRect().width()/2 , offsetY + square_size * i - coord->boundingRect().height()/2);
        coord->setVisible(showCoords);

		// Top
		coord = hCoords1->at(i);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2 , offsetY - offset + coord_centre - coord->boundingRect().height()/2 );
        coord->setVisible(showCoords);
		
		// Bottom
		coord = hCoords2->at(i);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2 ,offsetY + offset + board_pixel_size - coord_centre - coord->boundingRect().height()/2  );
        coord->setVisible(showCoords);
	}
}

/*
* called by 'boardwindow' when the toolbar button is toggled
*/
void Board::setShowCoords(bool b)
{
    if (b != showCoords)
    {
        showCoords = b;
        resizeBoard();  // Redraw the board if the value changed.
    }
}

/*
* the viewport is resized
*/
void Board::resizeEvent(QResizeEvent*)
{
#ifdef _WS_WIN_x
    if (!resizeDelayFlag)
    {
		resizeDelayFlag = true;
		// not necessary?
        QTimer::singleShot(50, this, SLOT(resizeBoard()));
    }
#else
	if (!lockResize)
        resizeBoard();
#endif
}

/*
* does the resizing work
*/
void Board::resizeBoard()
{
    if (width() < 30 || height() < 30)
		return;

	// Resize canvas
    canvas->setSceneRect(0,0,width(),height());

	// Recalculate the size values
	calculateSize();

	// Rescale the pixmaps in the ImageHandler
    imageHandler->setSquareSize(square_size);

	// Delete gatter lines and update stones positions
	QList<QGraphicsItem *> list = canvas->items();
	QGraphicsItem *item;

	QListIterator<QGraphicsItem *> it( list );


	for (; it.hasNext();)
	{
		item = it.next();
		/*
		 * Coordinates : type = 9
		 */
//		if (item->type() == 9)// || item->type() == 3)// || item->rtti() == 7)
//		{
//			item->hide();
//			delete item;
//		}
		/*else*/ if (item->type() == RTTI_STONE)
		{
			Stone *s = (Stone*)item;
			s->setColor(s->getColor());
			s->setPos(offsetX + square_size * (s->posX() - 1) - s->pixmap().width()/2, 
				offsetY + square_size * (s->posY() - 1) - s->pixmap().height()/2 );

			//TODO introduce a ghost list in the stone class so that this becomes redundant code
			if (s->isDead())
				s->togglePixmap(imageHandler->getGhostPixmaps(), false);


			
		}
		else if (item->type() >= RTTI_MARK_SQUARE &&
			item->type() <= RTTI_MARK_TERR)
		{
			Mark *m;
			switch(item->type())
			{
			case RTTI_MARK_SQUARE: m = (MarkSquare*)item; break;
			case RTTI_MARK_CIRCLE: m = (MarkCircle*)item;/* m->setSmall(setting->readBoolEntry("SMALL_MARKS")); */break;
			case RTTI_MARK_TRIANGLE: m = (MarkTriangle*)item; break;
			case RTTI_MARK_CROSS: m = (MarkCross*)item; break;
			case RTTI_MARK_TEXT: m = (MarkText*)item; break;
			case RTTI_MARK_NUMBER: m = (MarkNumber*)item; break;
			case RTTI_MARK_TERR: m = (MarkTerr*)item; break;
			default: continue;
			}
			if(item->type() == RTTI_MARK_TERR && preferences.terr_stone_mark)
			{
				m = (MarkSmallStoneTerr *)item;
				static_cast<MarkSmallStoneTerr *>(item)->setSize(square_size / SMALL_STONE_TERR_DIVISOR, square_size / SMALL_STONE_TERR_DIVISOR);
				static_cast<MarkSmallStoneTerr *>(item)->setPixmap(imageHandler->getSmallStonePixmaps());
			}
			else
				m->setSize(square_size, square_size);
			m->setPos(offsetX + square_size * (m->posX() - 1) - m->getSizeX()/2.0,
				offsetY + square_size * (m->posY() - 1) - m->getSizeY()/2.0);
		 }
	}

//	boardHandler->gotoMove(m_save);

	/* FIXME sometimes this draws the lines after/on top of the marks.
	 * moving it earlier doesn't fix anything */
	
	// Redraw the board
    drawBack();
	drawGatter();
//	if (showCoords && !isDisplayBoard)
    drawCoordinates();

  // Redraw the mark on the last played stone                             
//  updateLastMove(m_save->getColor(), m_save->getX(), m_save->getY()); 
}

/*
 * sets the cursor shape over the goban,
 * this is called by boardhandler and qgoboard proxy depending on the game mode and phase
 */
void Board::setCursorType(CursorType cur)
{
	if (cursor == cur)
		return;

	switch (cur)
	{
		case cursorIdle :
		{
			if (cursor == cursorNavTo || cursor == cursorWait)
				setCursor(Qt::ArrowCursor);
			
			showCursor = false;
			curStone->hide();
			break;
		}

		case cursorGhostBlack :
		{
			if (cursor == cursorNavTo || cursor == cursorWait)
				setCursor(Qt::ArrowCursor);

			curStone->setColor(stoneBlack);
			showCursor = true;
			break;
		}

		case cursorGhostWhite :
		{
			if (cursor == cursorNavTo || cursor == cursorWait)
				setCursor(Qt::ArrowCursor);

			curStone->setColor(stoneWhite);
			showCursor = true;
			break;
		}	
	
		case cursorNavTo :
		{
			setCursor(Qt::PointingHandCursor);		
			showCursor = false;
			curStone->hide();
			break;
		}

		case cursorWait :
		{
			setCursor(Qt::WaitCursor);
			showCursor = false;
			curStone->hide();
			break;
		}			
	}
	
	cursor = cur;
}

 /*
  *  0 : no stone
  * -1 : hidden
  *  1 : shown
  */
int Board::hasStone(int x, int y)
{
	if (!stones->contains(coordsToKey(x, y)))
		return 0;

	if (stones->find(coordsToKey(x, y)).value()->isVisible())
		return 1;
	
	return -1;
}

 /* 
  * Used for setting the mark color in case a mark is set over the var mark
  */
bool Board::hasVarGhost(StoneColor c, int x, int y)
{
	Stone *s;

	for (int i=0; i<ghosts->count(); i++)
	{
		s=ghosts->at(i);
		if (s->posX() == x && s->posY() == y && s->getColor() == c)
			return true;
	}
		
	return false;
}

/*
 * Synchronize the board with the given stone color and coordinates
 * This is usually called by boardhandler when scanning a matrix.
 */
void Board::updateStone(StoneColor c, int x, int y, bool dead)
{
    Stone *stone = stones->value(coordsToKey(x, y),NULL);
    //if ((stone == NULL) & ((c == stoneBlack) || (c == stoneWhite)))

	switch (c)
	{
	case stoneBlack:
	case stoneWhite:
		if (stone == NULL)
		{
            stone = new Stone(imageHandler->getStonePixmaps(), canvas, c, x, y,true);
            stone->setPos(offsetX + square_size * (x - 1) - stone->pixmap().width()/2,
                offsetY + square_size * (y - 1) - stone->pixmap().height()/2 );
            stones->insert(coordsToKey(x,y) , stone);
            break;
        }

        if (stone->getColor() != c)
            stone->setColor(c);
		
		// We need to check wether the stones have been toggled dead or seki before (scoring mode)
		if ((stone->isDead() || stone->isSeki()) && !dead)
		{
			stone->togglePixmap(imageHandler->getStonePixmaps(), true);
			stone->setDead(false);
		}
		
		if ((!stone->isDead()) && dead)
		{
			stone->togglePixmap(imageHandler->getGhostPixmaps(), false);
			stone->setDead();
		}

        if (!stone->isVisible())
            stone->show();

		break;
		
		
	case stoneNone:
	case stoneErase:
		if (stone && stone->isVisible())
		{
            stone->hide();
		}
		break;
		
	default:
		qWarning("Bad data <%d> at %d/%d in board::updateStone !",
			c, x, y);
	}
}

/*
 * Sets a variation mark of color 'c' at positon 'x,y'
 */
void Board::setVarGhost(StoneColor c, int x, int y)
{
	Stone *s = NULL;
	
//	if (setting->readIntEntry("VAR_GHOSTS") == vardisplayGhost) TODO
//		s = new Stone(imageHandler->getGhostPixmaps(), canvas, c, x, y);
//	else if (setting->readIntEntry("VAR_GHOSTS") == vardisplaySmallStone)
		s = new Stone(imageHandler->getAlternateGhostPixmaps(), canvas, c, x, y, false);
//	else
//		return;
		
	s->setZValue(3);//FIXME : this should not be here, but in the 'stone' code
	/* FIXME Variation ghosts on handicap stones for some weird reason */
	ghosts->append(s);
    
    if (x == PASS_XY && y == PASS_XY)  // Pass
	{
		s->setPos(	offsetX + square_size * (board_size+1),
			offsetY + square_size * board_size);
//		setMark(board_size+2, board_size+1, markText, false, tr("Pass"), false);
	}
	else
	{
		s->setPos(offsetX + square_size * (x-1) - s->boundingRect().width()/2,
			offsetY + square_size * (y-1) - s->boundingRect().height()/2);
	}
}

/*
 * Removes all the variation marks (when navigating into the tree)
 */
void Board::removeGhosts()
{
    qDeleteAll(*ghosts);
    ghosts->clear();
}

/*
 * Sets a mark of type 't' at positon 'x,y'. If a text mark : 'txt'
 */
void Board::setMark(int x, int y, MarkType t, bool /*update*/, QString txt, bool overlay)
{
	if (x == -1 || y == -1)
		return;
	
	Mark *m;

	// We already have a mark on this spot? If it is of the same type,
	// do nothing, else overwrite with the new mark.
	if ((m = hasMark(x, y)) != NULL)
    {
        if (m->getType() == t && m->getType() != markText)  // Text labels are overwritten
            return;
		removeMark(x, y);
    }

	if (lastMoveMark != NULL &&
		lastMoveMark->posX() == x &&
		lastMoveMark->posY() == y)
		removeLastMoveMark();
	
	QColor col = Qt::black;
	
	// Black stone or black ghost underlying? Then we need a white mark.
	if (( hasStone(x, y) == 1 && getStoneAt(x, y)->getColor() == stoneBlack) || hasVarGhost(stoneBlack, x, y))//||
//TODO	(settings->readIntEntry("VAR_GHOSTS") && hasVarGhost(stoneBlack, x, y)))
		col = Qt::white;
    
	short n = -1;
	
	switch(t)
	{
	case markSquare:
		m = new MarkSquare(x, y, square_size, canvas, col);
		break;
		
	case markCircle:
		m = new MarkCircle(x, y, square_size, canvas, col, true);//setting->readBoolEntry("SMALL_STONES"));
        	break;
		
	case markTriangle:
		m = new MarkTriangle(x, y, square_size, canvas, col);
		break;
		
	case markCross:
		m = new MarkCross(x, y, square_size, canvas, col);
		break;
		
	case markText:
//		if (txt == NULL)
//		{
//			n = 0;
//			while (letterPool[n] && n < 51)
//				n++;
//			letterPool[n] = true;
			
//			txt = QString(QChar(static_cast<const char>('A' + (n>=26 ? n+6 : n))));
			
			// Update matrix with this letter
//			boardHandler->getTree()->getCurrent()->getMatrix()->setMarkText(x, y, txt);
//		}
		/*else*/ if (txt.length() == 1)
		{
			// Text was given as argument, check if it can converted to a single letter
			n = -1;
			if (txt[0] >= 'A' && txt[0] <= 'Z')
				n = txt[0].toLatin1() - 'A';
			else if (txt[0] >= 'a' && txt[0] <= 'a')
				n = txt[0].toLatin1() - 'a' + 26;
			
			if (n > -1)
				letterPool[n] = true;
		}
		m = new MarkText(x, y, square_size, txt, canvas, col, n, false, overlay);
		gatter->hide(x,y);//setMarkText(x, y, txt);
		break;
		
	case markNumber:
//		if (txt == NULL)
//		{
//			n = 0;
//			while (numberPool[n] && n < 399)
//				n++;
//			
//			txt = QString::number(n+1);
//			
//			// Update matrix with this letter
//			boardHandler->getTree()->getCurrent()->getMatrix()->setMarkText(x, y, txt);	    
//		}
//		else
			n = txt.toInt() - 1;
		numberPool[n] = true;
		m = new MarkNumber(x, y, square_size, n, canvas, col, false);
		//FIXME why do we call "setMarkText" here and not in markText case above??
		//I'm commenting it out, doesn't seem to change anything
		//setMarkText(x, y, txt);
		gatter->hide(x,y);
		break;
		
	case markTerrBlack:
		if(!preferences.terr_stone_mark)
			m = new MarkTerr(x, y, square_size, stoneBlack, canvas);
		else
			m = new MarkSmallStoneTerr(x, y, square_size / SMALL_STONE_TERR_DIVISOR, stoneBlack, imageHandler->getSmallStonePixmaps(), canvas);
		if (hasStone(x, y) == 1)
		{
			//getStoneAt(x, y)->setDead(true);
			//getStoneAt(x, y)->togglePixmap(imageHandler->getGhostPixmaps(), false);
//			getStoneAt(x, y)->shadow->hide();
//			boardHandler->markedDead = true;
			updateStone(stoneWhite, x, y, true);
		}
//		boardHandler->getTree()->getCurrent()->setScored(true);
		break;
		
	case markTerrWhite:
		if(!preferences.terr_stone_mark)
			m = new MarkTerr(x, y, square_size, stoneWhite, canvas);
		else
			m = new MarkSmallStoneTerr(x, y, square_size / SMALL_STONE_TERR_DIVISOR, stoneWhite, imageHandler->getSmallStonePixmaps(), canvas);
		if (hasStone(x, y) == 1)
		{
			//getStoneAt(x, y)->setDead(true);
			//getStoneAt(x, y)->togglePixmap(imageHandler->getGhostPixmaps(), false);
//			boardHandler->getStoneHandler()->getStoneAt(x, y)->shadow->hide();
//			boardHandler->markedDead = true;
			updateStone(stoneBlack, x, y, true);
		}
//		boardHandler->getTree()->getCurrent()->setScored(true);
		break;
		
	default:
		qWarning("   *** Board::setMark() - Bad mark type! ***");
		return;
	}
	
	Q_CHECK_PTR(m);
	m->setPos(offsetX + square_size * (x-1) - m->getSizeX()/2 , offsetY + square_size * (y-1) - m->getSizeY()/2);
	m->show();
	
	marks->append(m);
//	if (update)
//		boardHandler->editMark(x, y, t, txt);
}

/*
 * Sets a text in 'text' mark at positon 'x,y'.
 */
void Board::setMarkText(int x, int y, const QString &txt)
{
	Mark *m;
    
	// Oops, no mark here, or no text mark
	if (txt.isNull() || txt.isEmpty() ||
		(m = hasMark(x, y)) == NULL || m->getType() != markText)
		return;
	
	m->setText(txt);
	// Adjust the position on the board, if the text size has changed.
	m->setSize((double)square_size, (double)square_size);
	m->setPos(offsetX + square_size * (x-1) - m->getSizeX()/2 , offsetY + square_size * (y-1) - m->getSizeY()/2);
}

/*
 * Queries wether there is a mark at position 'x,y'
 */
Mark* Board::hasMark(int x, int y)
{
	Mark *m = NULL;

//	for (m=marks->first(); m != NULL; m=marks->next())
	for (int i=0; i<marks->count(); i++)
	{
		m=marks->at(i);
		if (m->posX() == x && m->posY() == y)
			return m;
	}

	return NULL;
}

/*
 * Removes the mark at position 'x,y'
 */
void Board::removeMark(int x, int y, bool /*update*/)
{
	Mark *m = NULL;
	
	if (lastMoveMark != NULL &&
		lastMoveMark->posX() == x &&
		lastMoveMark->posY() == y)
		removeLastMoveMark();

//	for (m=marks->first(); m != NULL; m=marks->next())
	for (int i=0; i<marks->count(); i++)
	{
		m=marks->at(i);
		if (m->posX() == x && m->posY() == y)
		{
			if (m->getCounter() != -1)
			{
				if (m->getType() == markText)
					letterPool[m->getCounter()] = false;
				else if (m->getType() == markNumber)
					numberPool[m->getCounter()] = false;
			}
			delete marks->takeAt(i);
			gatter->show(x,y);
//			if (update)
//				boardHandler->editMark(x, y, markNone);
			return;
		}
	}
}

/*
 * Removes the mark on the last stone played
 */
void Board::removeLastMoveMark()
{
	if (lastMoveMark != NULL)
	{
//		lastMoveMark->hide();
		delete lastMoveMark;
		lastMoveMark = NULL;
	}
}

/*
 * Updates the mark on the last stone played
 */
 void Board::updateLastMove(Move * move)
{
	StoneColor c = move->getColor();
	int x = move->getX();
	int y = move->getY();
	
	delete lastMoveMark;
	lastMoveMark = NULL;

    if (x == PASS_XY && y == PASS_XY)  // Passing	(FIXME don't think we use this anymore)
		removeLastMoveMark();
	else if(move->isHandicapMove()) {}	// no last move marks on handicaps
	else if (c != stoneNone && x != -1 && y != -1 && x <= board_size && y <= board_size)
	{
		
		if(preferences.number_current_move)
		{
			//possible FIXME, MarkNumber adds 1 in mark.h to the move number ?!
			lastMoveMark = new MarkNumber(x, y, square_size, move->getMoveNumber() - 1, canvas, 
					c == stoneBlack ? Qt::white : Qt::black, false);
		}
		else
		{
			lastMoveMark = new MarkCross(x, y, square_size, canvas,
					c == stoneBlack ? Qt::white : Qt::black, true);
		}
		Q_ASSERT(lastMoveMark);
//		lastMoveMark = new MarkCircle(x, y, square_size, canvas,
//			c == stoneBlack ? white : black, setting->readBoolEntry("SMALL_STONES"));
		lastMoveMark->setPos(offsetX + square_size * (x-1) - lastMoveMark->getSizeX()/2,
					offsetY + square_size * (y-1) - lastMoveMark->getSizeY()/2);
		lastMoveMark->show();
	}

//	setCurStoneColor();
}

/*
 * Used to know where the mouse is over the goban
 */
int Board::convertCoordsToPoint(int c, int o)
{
	int p = c - o + square_size/2;
	if (p >= 0)
		return p / square_size + 1;
	else
		return -1;
}

/*
 * Called when the mouse pointer leaves the goban
 */
void Board::leaveEvent(QEvent*)
{
	curStone->hide();
}

/*
 * Sets the ghost cursor depending on the arrow position
 */
void Board::mouseMoveEvent ( QMouseEvent * e )
{

	if (!showCursor)
		return;

	int x = convertCoordsToPoint(e->x(), offsetX),
		y = convertCoordsToPoint(e->y(), offsetY);

	/* FIXME, maybe don't draw cursor if x/y changes from downX downY?? */
	if(downX > 0 && (downX != x || downY != y))
	{
        curStone->hide();
		curX = curY = -1;
		return;
	}
	// Outside the valid board?
	if ((x < 1) || x > board_size || y < 1 || y > board_size)
	{
        curStone->hide();
		curX = curY = -1;
		return;
	}
	
	// Nothing changed
	if (curX == (short)x && curY == (short)y)
		return;
	
	// Update the statusbar coords tip
//	emit coordsChanged(x, y, board_size,showSGFCoords);
	
	// Remember if the cursor was hidden meanwhile.
	// If yes, we need to repaint it at the old position.
//	bool flag = curX == -1;
	
	curX = (short)x;
	curY = (short)y;

//	if (// !showCursor || setting->readBoolEntry("CURSOR") ||
//		(gamePhase == phaseEdit /*&& boardHandler->getMarkType() != markNone*/) ||
//		gamePhase == phaseScore ||
//		(curStone->posX() == x && curStone->posY() == y && !flag))
//		return;
    
	curStone->setPos(offsetX + square_size * (x - 1) - curStone->pixmap().width()/2, 
				offsetY + square_size * (y - 1) - curStone->pixmap().height()/2 );
//	curStone->setX(offsetX + square_size * (x-1));
//	curStone->setY(offsetY + square_size * (y-1));
	curStone->setCoord(x, y);
/*
	bool notMyTurn = 	(curStone->getColor() == stoneBlack && !myColorIsBlack ||
			 curStone->getColor() == stoneWhite && myColorIsBlack);
    
	if (navIntersectionStatus ||              
        boardHandler->getGameMode() == modeObserve ||
	( boardHandler->getGameMode() == modeMatch && notMyTurn) ||
	( boardHandler->getGameMode() == modeComputer && notMyTurn))
	
		curStone->hide();
	else
*/
	curStone->show();
}

/*
 * mouse wheel has been turned
 */
void Board::wheelEvent(QWheelEvent *e)
{
	emit signalWheelEvent( e);
}

/*
 * starts the counter for anticlivko moves
 */
void Board::mousePressEvent(QMouseEvent * e)
{
	downX = convertCoordsToPoint(e->x(), offsetX),
	downY = convertCoordsToPoint(e->y(), offsetY);

	// Button gesture outside the board?
	if ((downX < 1) || downX > board_size || downY < 1 || downY > board_size)
	{
		downX = downY = -1;
		//return	//??
	}
	clickTime = QTime::currentTime();
	clickTime = clickTime.addMSecs(250);
}

void Board::mouseReleaseEvent(QMouseEvent* e)
{
	mouseState = e->button();

	int 	x = convertCoordsToPoint(e->x(), offsetX),
		y = convertCoordsToPoint(e->y(), offsetY);

	/* FIXME click protection... if we don't like, we should change it */
	if(downX != x || downY != y)
	{
		downX = downY = -1;
		return;
	}
	downX = downY = -1;
	// Button gesture outside the board?
	if ((x < 1) || x > board_size || y < 1 || y > board_size)
/*	{
		if (e->button() == LeftButton &&
			e->state() == RightButton)
			previousMove();
		else if (e->button() == RightButton &&
			e->state() == LeftButton)
			nextMove();
		else if (e->button() == LeftButton &&
			e->state() == MidButton)
			gotoVarStart();
		else if (e->button() == RightButton &&
			e->state() == MidButton)
			gotoNextBranch();
*/		
		return;
//	}
/*
	// Lock accidental gesture over board
	if ((e->button() == Qt::LeftButton && e->state() == Qt::RightButton) ||
		(e->button() == Qt::RightButton && e->state() == Qt::LeftButton) ||
		(e->button() == Qt::LeftButton && e->state() == Qt::MidButton) ||
		(e->button() == Qt::RightButton && e->state() == Qt::MidButton))
		return;
*/
	    // Check delay
    	bool delay = (QTime::currentTime() > clickTime);
	//note that apparently nothing uses delay at the moment
	emit signalClicked(delay, x , y , mouseState);
}

/*
 * called when using a button menu 'export'
 * saves the board image and puts it to clipboard or file
 */
void Board::exportPicture(const QString &fileName,  QString *filter, bool toClipboard)
{

    QPixmap pix = this->grab(table->boundingRect().toAlignedRect());

	if (toClipboard)
	{
		QApplication::clipboard()->setPixmap(pix);
		return;
	}


    if (!pix.save(fileName, filter->toLatin1()))
		QMessageBox::warning(this, PACKAGE, tr("Failed to save image!"));
}

bool Board::updateAll(Move * move)
{
    Matrix * m = move->getMatrix();
    Q_CHECK_PTR(m);

    bool modified = false;//, fake = false;
    StoneColor color;
    bool dead;
    /*
    * Synchronize the matrix with the stonehandler data and
    * update the canvas.
    * This is usually called when navigating through the tree.
    */
    for (int y=1; y<=board_size; y++)
    {
        for (int x=1; x<=board_size; x++)
        {
            /* FIXME apparently matrix uses negative values for
             * both dead and edited stones.  I think the
             * assumption is that we won't be editing during
             * a score phase which is the only time things will
             * be marked dead whereas other ghosts are used
             * for variations.
             * I'm not sure what the consequences on editing
             * will be for this, but for now, I just want to
             * make sure the handicap stones can't be ghosted.
             * We could say that the handicap stones aren't
             * edits, but this is what they've been set up
             * as so that's more tricky. */
            dead = (m->isStoneDead(x, y)) & (move->getMoveNumber() != 0);
            color = m->getStoneAt(x, y);

            if (oneColorGo && color == stoneBlack)
                color = stoneWhite;
            updateStone(color,x,y, dead);

            switch (m->getMarkAt(x, y))
            {
            case markKoMarker:
            case markSquare:
                modified = true;
                setMark(x, y, markSquare, false);
                break;

            case markCircle:
                modified = true;
                setMark(x, y, markCircle, false);
                break;

            case markTriangle:
                modified = true;
                setMark(x, y, markTriangle, false);
                break;

            case markCross:
                modified = true;
                setMark(x, y, markCross, false);
                break;

            case markText:
                modified = true;
                setMark(x, y, markText, false, m->getMarkText(x, y));
                break;

            case markNumber:
                modified = true;
                setMark(x, y, markNumber, false, m->getMarkText(x, y));
                break;

            case markTerrBlack:
                modified = true;
                setMark(x, y, markTerrBlack, false);
                break;

            case markTerrWhite:
                modified = true;
                setMark(x, y, markTerrWhite, false);
                break;

            case markNone:
            case markTerrDame:
                if (hasMark(x, y))
                {
                    modified = true;
                    removeMark(x, y, false);
                }
            default:
                break;
            }
        }
    }
    return modified;
}

/*
 * Update the variation marks on the board if any
 */
void Board::updateVariationGhosts(Move *move)
{
    // qDebug("BoardHandler::updateVariationGhosts()");
    Move *m = move->parent->son;
    Q_CHECK_PTR(m);

    do
    {
        setVarGhost(m->getColor(), m->getX(), m->getY());
    } while ((m = m->brother) != NULL);
}
