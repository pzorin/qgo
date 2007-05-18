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

#include <QtGui>



/*
 * This initialises everything on the board : background, gatter, cursor, etc ....
 */
Board::Board(QWidget *parent, QGraphicsScene *c)
: QGraphicsView(c,parent)
{
}

void Board::init(int size)
{
//	setRenderHints(QPainter::SmoothPixmapTransform);
//	gamePhase = phaseInit;

	viewport()->setMouseTracking(TRUE);

	lockResize =  false ;
	board_size = size;//DEFAULT_BOARD_SIZE;
	showCoords = true;//TODO setting->readBoolEntry("BOARD_COORDS");
	showSGFCoords = false;//TODO setting->readBoolEntry("SGF_BOARD_COORDS");
/*	antiClicko = setting->readBoolEntry("ANTICLICKO");
	
*/	
	// Create an ImageHandler instance.
	imageHandler = new ImageHandler();
	Q_CHECK_PTR(imageHandler);
	
	// Init the canvas
	canvas = new QGraphicsScene(0,0,BOARD_X, BOARD_Y,this);
	Q_CHECK_PTR(canvas);

	setScene(canvas);
	gatter = new Gatter(canvas, board_size);
	

		
	// Init data storage for marks and ghosts
	marks = new QList<Mark*>;
//	marks->setAutoDelete(TRUE);
	lastMoveMark = NULL;
	
	ghosts = new QList<Stone*>;
	
	// Init the gatter size and the imagehandler pixmaps
	calculateSize();

	imageHandler->init(square_size);
/*	
	// Initialize some class variables
	nodeResultsDlg = NULL;
	fastLoad = false;
	isModified = false;
	mouseState = NoButton;
	for (int i=0; i<400; i++)
	{
		if (i < 52)
			letterPool[i] = false;
		numberPool[i] = false;
	}
	//coordsTip = new Tip(this);
#ifdef Q_WS_WIN
	resizeDelayFlag = false;
#endif
	curX = curY = -1;
	showCursor = setting->readBoolEntry("CURSOR");
*/	
//	isLocalGame = true;
	
	// Init the ghost cursor stone
	curStone = new Stone(imageHandler->getGhostPixmaps(), canvas, stoneBlack, 0, 0);
	curStone->setZValue(4);
	curStone->hide();
	showCursor = FALSE;
/*
	lockResize = false;
	navIntersectionStatus = false;

*/

	//Init the stones
	stones = new QHash<int,Stone *>::QHash();
}

Board::~Board()
{
	qDeleteAll(*stones);
	qDeleteAll(*ghosts);
	qDeleteAll(*marks);
}

/*
 * cleans up the board
 */
void Board::clearData()
{

//    hideAllMarks();
//    removeLastMoveMark();
//    boardHandler->clearData();
//    if (curStone != NULL)
//		curStone->setColor(stoneBlack);
//    canvas->update();
//	isModified = false;
/*    if (nodeResultsDlg != NULL)
    {
		nodeResultsDlg->hide();
		delete nodeResultsDlg;
		nodeResultsDlg = NULL;
    }
*/
	qDeleteAll(*stones);
	stones->clear();

	qDeleteAll(*ghosts);
	ghosts->clear();

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


	QGraphicsSimpleTextItem *coordV = new QGraphicsSimpleTextItem(QString::number(board_size),0, canvas);
	QGraphicsSimpleTextItem *coordH = new QGraphicsSimpleTextItem("A",0, canvas);
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
	board_pixel_size = square_size * (board_size-1);    
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
* draws the table under the goban, and the goban wood texture
*/
void Board::drawBackground()
{
	int 	w = (int)canvas->width(),
		h = (int)canvas->height();
	
	// Create pixmap of appropriate size
	QPixmap all(w, h);

	// Paint table and board on the pixmap
	QPainter painter;

	painter.begin(&all);
	painter.setPen(Qt::NoPen);


	painter.drawTiledPixmap (0, 0, w, h,QPixmap::QPixmap(":/new/prefix1/ressources/pics/table.png"));
	// /home/eb/Packages/qgo2/src/board/ressources/pics/table.png"));
	//TODO (ImageHandler::getTablePixmap(  setting->readEntry("SKIN_TABLE"))));

	painter.drawTiledPixmap (
		offsetX - offset,
		offsetY - offset,
		board_pixel_size + offset*2,
		board_pixel_size + offset*2,
		//TODO (ImageHandler::getBoardPixmap(setting->readEntry("SKIN"))));
		QPixmap::QPixmap(":/new/prefix1/ressources/pics/wood.png"));
	painter.end();

	QImage image = all.toImage();
	int lighter=20;
	int darker=60;
	int width = 3; 

	int x,y;
	for(x=0;x<width;x++)
		for (y= offsetY - offset +x ; y<offsetY + board_pixel_size + offset-x ;y++)
		{
			image.setPixel(
				offsetX - offset+x , 
				y, 
				QColor(image.pixel(offsetX - offset+x,y)).dark(int(100 + darker*(width-x)*(width-x)/width/width)).rgb());

			image.setPixel(
				offsetX + board_pixel_size + offset-x -1, 
				y,
				QColor(image.pixel(offsetX + board_pixel_size + offset-x-1,y)).light(100+ int(lighter*(width-x)*(width-x)/width/width)).rgb());
		}

	for(y=0;y<width;y++)
		for (x= offsetX - offset +y ; x<offsetX + board_pixel_size + offset-y ;x++)
		{
			image.setPixel(
				x,
				offsetY - offset+y , 
				QColor(image.pixel(x,offsetY - offset+y)).light(int(100 + lighter*(width-y)*(width-y)/width/width)).rgb());

			image.setPixel(
				x,
				offsetY + board_pixel_size + offset-y -1, 
				QColor(image.pixel(x,offsetY + board_pixel_size + offset-y-1)).dark(100+ int(darker*(width-y)*(width-y)/width/width)).rgb());
		}


	width = 10;
	darker=50;

	for(x=0;(x<=width)&&(offsetX - offset-x >0) ;x++)
		for (y= offsetY - offset+x ; (y<offsetY + board_pixel_size + offset+x)&&(y<h) ;y++)
		{
			image.setPixel(
				offsetX - offset-1-x  , 
				y, 
				QColor(image.pixel(offsetX - offset-1-x,y)).dark(int(100 + darker*(width-x)/width)).rgb());
		}

	for(y=0;(y<=width)&&(offsetY + board_pixel_size + offset+y+1<h);y++)
		for (x= (offsetX - offset - y > 0 ? offsetX - offset - y:0) ; x<offsetX + board_pixel_size + offset-y ;x++)
		{
			image.setPixel(
				x,
				offsetY + board_pixel_size + offset+y +1, 
				QColor(image.pixel(x,offsetY + board_pixel_size + offset+y+1)).dark(100+ int(darker*(width-y)/width)).rgb());
		}

	//redraws the image on a brush to set the background
	canvas->setBackgroundBrush ( QBrush::QBrush(image));

}

/*
 * Draws the coordinates around the goban grid
 */
void Board::drawCoordinates()
{
	QGraphicsSimpleTextItem *coord;
	int i;
	
	// centres the coordinates text within the remaining space at table edge
	const int coord_centre = (offset - square_size/2 )/2; 
	QString txt;

	// Draw vertical coordinates. Numbers
	for (i=0; i<board_size; i++)
	{
		// Left side
		if(showSGFCoords)
			txt = QString(QChar(static_cast<const char>('a' + i)));
		else
			txt = QString::number(board_size - i);

		coord = new QGraphicsSimpleTextItem(txt, 0, canvas);
		coord->setPos(offsetX - offset + coord_centre - coord->boundingRect().width()/2 , offsetY + square_size * i - coord->boundingRect().height()/2);

		coord->show();

		// Right side
		coord = new QGraphicsSimpleTextItem(txt, 0,canvas);
    		coord->setPos(offsetX + board_pixel_size + offset - coord_centre - coord->boundingRect().width()/2 , offsetY + square_size * i - coord->boundingRect().height()/2);

		coord->show();
	}
	
	// Draw horizontal coordinates. Letters (Note: Skip 'i')
	for (i=0; i<board_size; i++)
	{
		if(showSGFCoords)
			txt = QString(QChar(static_cast<const char>('a' + i)));
		else
			txt = QString(QChar(static_cast<const char>('A' + (i<8?i:i+1))));
		// Top
		coord = new QGraphicsSimpleTextItem(txt, 0, canvas);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2 , offsetY - offset + coord_centre - coord->boundingRect().height()/2 );
		coord->show();
		// Bottom
		coord = new QGraphicsSimpleTextItem(txt,0, canvas);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2 ,offsetY + offset + board_pixel_size - coord_centre - coord->boundingRect().height()/2  );
		coord->show();
	}

}

/*
* called by 'boardwindow' when the toolbar button is toggled
*/
void Board::setShowCoords(bool b)
{
	bool old = showCoords;
	showCoords = b;
	if (old != showCoords)
		changeSize();  // Redraw the board if the value changed.
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
		QTimer::singleShot(50, this, SLOT(changeSize()));
    }
#else
	if (!lockResize)
		changeSize();
#endif
}



/*
* called by the resize event
*/
void Board::changeSize()
{
#ifdef Q_WS_WIN
	resizeDelayFlag = false;
#endif

//	QSize s = QSize::QSize(width()-5, height()-5);
//	resizeBoard(s.width(), s.height());

	resizeBoard(width()-5, height()-5);
}


/*
* does the resizing work
*/
void Board::resizeBoard(int w, int h)
{
	if (w < 30 || h < 30)
		return;

	// Resize canvas
	canvas->setSceneRect(0,0,w,h);

	// Recalculate the size values
	calculateSize();

	// Rescale the pixmaps in the ImageHandler
	imageHandler->rescale(square_size);
	

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
		if (item->type() == 9)// || item->type() == 3)// || item->rtti() == 7)
		{
//			item->hide();
			delete item;
		}
		else if (item->type() == RTTI_STONE)
		{
			Stone *s = (Stone*)item;
			s->setColor(s->getColor());
			s->setPos(offsetX + square_size * (s->posX() - 1) - s->pixmap().width()/2, 
				offsetY + square_size * (s->posY() - 1) - s->pixmap().height()/2 );
			
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
			m->setSize(square_size, square_size);
			m->setPos(offsetX + square_size * (m->posX() - 1) - m->getSizeX()/2.0,
				offsetY + square_size * (m->posY() - 1) - m->getSizeY()/2.0);
		 }
	}

//	boardHandler->gotoMove(m_save);

	// Redraw the board
	drawBackground();
	drawGatter();
	if (showCoords)
		drawCoordinates();

  // Redraw the mark on the last played stone                             
//  updateLastMove(m_save->getColor(), m_save->getX(), m_save->getY()); 
  
	canvas->update();
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
			
			showCursor = FALSE;
			curStone->hide();
			break;
		}

		case cursorGhostBlack :
		{
			if (cursor == cursorNavTo || cursor == cursorWait)
				setCursor(Qt::ArrowCursor);

			curStone->setColor(stoneBlack);
			showCursor = TRUE;
			break;
		}

		case cursorGhostWhite :
		{
			if (cursor == cursorNavTo || cursor == cursorWait)
				setCursor(Qt::ArrowCursor);

			curStone->setColor(stoneWhite);
			showCursor = TRUE;
			break;
		}	
	
		case cursorNavTo :
		{
			setCursor(Qt::PointingHandCursor);		
			showCursor = FALSE;
			curStone->hide();
			break;
		}

		case cursorWait :
		{
			setCursor(Qt::WaitCursor);
			showCursor = FALSE;
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
	Stone *s;
	
	if (!stones->contains(coordsToKey(x, y)))
		return 0;

	s = stones->find(coordsToKey(x, y)).value();
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
			return TRUE;
	}
		
	return FALSE;

}


/*
 * Adds a stone on the board at coords x,y and color c
 */
Stone* Board::addStoneSprite(StoneColor c, int x, int y, bool /*shown*/)
{
	if ((x < 1) || x > board_size || y < 1 || y > board_size)
	{
		qWarning("Board::addStoneSprite() - Invalid stone: %d %d", x, y);
		return NULL;
	}
	
	Stone *s=NULL;

	switch (hasStone(x, y))
	{
	case 1:  // Stone exists and is visible
		// qDebug("*** Already a stone at %d, %d.", x, y);
//		if (boardHandler->display_incoming_move)
//			return NULL;
//		else
//		{
			s = stones->find(coordsToKey(x, y)).value();
			Q_CHECK_PTR(s);
			s->setColor(c);
//			s->setPos(x, y);
//			return s;
//		}

		break;

	case 0:  // No stone existing. Create a new one
		// qDebug("*** Did not find any stone at %d, %d.", x, y);
			
 		s = new Stone(imageHandler->getStonePixmaps(), canvas, c, x, y,true);
			
//		if (boardHandler->getGameData()->oneColorGo)
//			s->toggleOneColorGo(true);

		Q_CHECK_PTR(s);
			
		s->setPos(offsetX + square_size * (x-1.5) ,offsetY + square_size/2 * (y-1.5));
			
		// Change color of a mark on this spot to white, if we have a black stone
//		if (c == stoneBlack)
//			updateMarkColor(stoneBlack, x, y);
	
//		return s;
	
		break;
		
	case -1:  // Stone exists, but is hidden. Show it and check correct color

		s = stones->find(coordsToKey(x, y)).value();
		Q_CHECK_PTR(s);
		
		// qDebug("*** Found a hidden stone at %d, %d (%s).", x, y,
		
		// Check if the color is correct
		if (s->getColor() != c)
			s->setColor(c);
//		s->setPos(x, y);
		s->show();
//		shown = true;
		
		// Change color of a mark on this spot to white, if we have a black stone
//		if (c == stoneBlack)
//			updateMarkColor(stoneBlack, x, y);
		
//		return s;
	break;

	}

	return s;

}



/*
 * Synchronize the board with the given stone color and coordinates
 * This is usually called by boardhandler when scanning a matrix.
 */
bool Board::updateStone(StoneColor c, int x, int y)
{

	Stone *stone;
	QHash<int,Stone *>::iterator si;
	bool modified = false;
	

	if ((si = stones->find(coordsToKey(x, y))) == stones->end())
		stone = NULL;
	else 
		stone = si.value();	

	switch (c)
	{
	case stoneBlack:
	case stoneWhite:
		if (stone == NULL)
		{
			stone = new Stone(imageHandler->getStonePixmaps(), canvas, c, x, y,true);
			Q_CHECK_PTR(stone);
			
			stone->setPos(offsetX + square_size * (x-1.5) ,offsetY + square_size * (y-1.5));			

			stones->insert(coordsToKey(x,y) , stone);
			modified = true;
			break;
		}
		else if (!stone->isVisible())
		{
			stone->show();
			modified = true;
		}
		
		if (stone->getColor() != c)
		{
			stone->setColor(c);
			modified = true;
		}
		
		// We need to check wether the stones have been toggled dead or seki before (scoring mode)
		if (stone->isDead() || stone->isSeki())
			stone->togglePixmap(imageHandler->getStonePixmaps(), TRUE);

		break;
		
		
	case stoneNone:
	case stoneErase:
		if (stone && stone->isVisible())
		{
			stone->hide();
			modified = true;
		}
		break;
		
	default:
		qWarning("Bad data <%d> at %d/%d in board::updateStone !",
			c, x, y);
	}

	return modified;
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
		s = new Stone(imageHandler->getAlternateGhostPixmaps(), canvas, c, x, y, FALSE);
//	else
//		return;
	
	s->setZValue(3);//FIXME : this should not be here, but in the 'stone' code

	ghosts->append(s);
    
	if (x == 20 && y == 20)  // Pass
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
	// Remove all variation ghosts
	while (! ghosts->isEmpty())
		delete ghosts->takeFirst();
	//Might not be needed
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
//	{
//		if (m->getType() == t && m->getType() != markText)  // Text labels are overwritten
//			return;
		
		removeMark(x, y);
//	}

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
		setMarkText(x, y, txt);
		gatter->hide(x,y);
		break;
		
	case markTerrBlack:
		m = new MarkTerr(x, y, square_size, stoneBlack, canvas);
		if (hasStone(x, y) == 1)
		{
			getStoneAt(x, y)->setDead(true);
			getStoneAt(x, y)->togglePixmap(imageHandler->getGhostPixmaps(), FALSE);
//			getStoneAt(x, y)->shadow->hide();
//			boardHandler->markedDead = true;
		}
//		boardHandler->getTree()->getCurrent()->setScored(true);
		break;
		
	case markTerrWhite:
		m = new MarkTerr(x, y, square_size, stoneWhite, canvas);
		if (hasStone(x, y) == 1)
		{
			getStoneAt(x, y)->setDead(true);
			getStoneAt(x, y)->togglePixmap(imageHandler->getGhostPixmaps(), FALSE);
//			boardHandler->getStoneHandler()->getStoneAt(x, y)->shadow->hide();
//			boardHandler->markedDead = true;
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
 * restores the stones that were marked dead or seki
 */
void Board::removeDeadMarks()
{
/*	QIntDictIterator<Stone> it(*stones);
	Stone *s;
	
	while (it.current())
	{
		s = it.current();
		CHECK_PTR(s);
		if (s->isDead() || s->isSeki())
		{
			s->setDead(false);
			s->setSeki(false);
			s->setSequence(boardHandler->board->getImageHandler()->getStonePixmaps());
			s->shadow->show();
		}
		++it;
	}
*/
	QHashIterator<int, Stone*> i(*stones);
	while (i.hasNext()) 
	{
		i.next();
		if (i.value()->isDead() || i.value()->isSeki())
		{
			i.value()->setDead(false);
			i.value()->setSeki(false);
			i.value()->togglePixmap(imageHandler->getStonePixmaps(), TRUE);
		}
	}

}


void Board::updateDeadMarks(int &black, int &white)
{
//	QIntDictIterator<Stone> it(*stones);
	Stone *s;
	
//	while (it.current())

	QHashIterator<int, Stone*> it(*stones);
	while (it.hasNext()) 	
	{
		s = it.next().value();
//		CHECK_PTR(s);
		if (s->isDead())
		{
			if (s->getColor() == stoneBlack)
				white ++;
			else
				black ++;
		}
//		++it;
	}
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
void Board::updateLastMove(StoneColor c, int x, int y)
{

	delete lastMoveMark;
	lastMoveMark = NULL;

	if (x == 20 && y == 20)  // Passing
		removeLastMoveMark();

	else if (c != stoneNone && x != -1 && y != -1 && x <= board_size && y <= board_size)
	{
//		lastMoveMark = new MarkText(imageHandler, x, y, square_size, "+", canvas,
		// true = plus sign
		lastMoveMark = new MarkCross(x, y, square_size, canvas,
			c == stoneBlack ? Qt::white : Qt::black, true);
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
	canvas->update();
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

	// Outside the valid board?
	if ((x < 1) || x > board_size || y < 1 || y > board_size)
	{
		curStone->hide();
		canvas->update();
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
    
	canvas->update();
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
void Board::mousePressEvent(QMouseEvent *)
{
	clickTime = QTime::currentTime();
	clickTime = clickTime.addMSecs(250);
}

void Board::mouseReleaseEvent(QMouseEvent* e)
{
	mouseState = e->button();

	int 	x = convertCoordsToPoint(e->x(), offsetX),
		y = convertCoordsToPoint(e->y(), offsetY);

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

	emit signalClicked(delay, x , y , mouseState);
}

