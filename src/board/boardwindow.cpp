/***************************************************************************
*
* This class is the board window. It's the main purpose of qGo
* The main interface handles all the open boards as a list of this class
*
 ***************************************************************************/

#include "stdio.h"
#include "boardwindow.h"
#include "boardhandler.h"
#include "board.h"
#include "interfacehandler.h"

//#include "sgfparser.h"
//#include "matrix.h"
//#include "move.h"

#include <QtGui>

class BoardHandler;

BoardWindow::BoardWindow(QWidget * parent, Qt::WindowFlags flags, int size)
	: QMainWindow( parent,  flags )
{
	boardSize = size;
	gameData = NULL;
	init();
}

BoardWindow::BoardWindow( QWidget *parent , Qt::WindowFlags flags , GameData *gd , GameMode gm , bool iAmBlack , bool iAmWhite)
	: QMainWindow( parent,  flags )
{
	if (gd)
		gameData = new GameData (gd);
	else
		gameData = NULL;

	gameMode = gm;
	myColorIsBlack = iAmBlack;
	myColorIsWhite = iAmWhite;

	boardSize = gd->size;
	init();

}

void BoardWindow::init()
{
	ui.setupUi(this);
	ui.board->init(boardSize);

	interfaceHandler = new InterfaceHandler( this);

	tree = new Tree(boardSize);
	boardHandler = new BoardHandler(this, tree, boardSize);

	interfaceHandler = new InterfaceHandler( this);
	if (gameData)
		interfaceHandler->updateCaption(gameData);

	
	/*
	 * Connects the nav buttons to the slots
	 */
	connect(ui.navForward,SIGNAL(pressed()), boardHandler, SLOT(slotNavForward()));
	connect(ui.navBackward,SIGNAL(pressed()), boardHandler, SLOT(slotNavBackward()));
	connect(ui.navNextVar, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextVar()));
	connect(ui.navPrevVar, SIGNAL(pressed()), boardHandler, SLOT(slotNavPrevVar()));
	connect(ui.navFirst, SIGNAL(pressed()), boardHandler, SLOT(slotNavFirst()));
	connect(ui.navLast, SIGNAL(pressed()), boardHandler, SLOT(slotNavLast()));
	connect(ui.navMainBranch, SIGNAL(pressed()), boardHandler, SLOT(slotNavMainBranch()));
	connect(ui.navStartVar, SIGNAL(pressed()), boardHandler, SLOT(slotNavStartVar()));
	connect(ui.navNextBranch, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextBranch()));
	connect(ui.navIntersection, SIGNAL(pressed()), boardHandler, SLOT(slotNavIntersection()));
	connect(ui.navPrevComment, SIGNAL(pressed()), boardHandler, SLOT(slotNavPrevComment()));
	connect(ui.navNextComment, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextComment()));
	connect(ui.navNextComment, SIGNAL(pressed()), boardHandler, SLOT(slotNavNextComment()));
	if  ( connect(ui.slider, SIGNAL(sliderReleased ()), this , SLOT(slotNthMove())))
		qDebug("could connect slider");
}

BoardWindow::~BoardWindow()
{
}

void BoardWindow::closeEvent(QCloseEvent *)
{
}

/*
 * Loads the SGF string
 */
bool BoardWindow::loadSGF(const QString /*&fileName*/, const QString SGFLoaded, bool /* fastLoad */)
{

	if (! boardHandler->loadSGF("", QString(SGFLoaded) ))
		return FALSE ;

	boardHandler->slotNavFirst(); //gotoFirstMove(); // TODO check wether this belongs here ... (copy board)

	return TRUE ;

}
/*
void BoardWindow::slotNavBackward()
{
	boardHandler->previousMove();
}

void BoardWindow::slotNavForward()
{
//	boardHandler->nextMove();
}

void BoardWindow::slotNavFirst()
{
	boardHandler->gotoFirstMove();
}

void BoardWindow::slotNavLast()
{
	boardHandler->gotoLastMove();
}

void BoardWindow::slotNavNextComment()
{
	boardHandler->nextComment();
}

void BoardWindow::slotNavPrevComment()
{
	boardHandler->previousComment();
}

void BoardWindow::slotNavPrevVar()
{
	boardHandler->previousVariation();
}

void BoardWindow::slotNavNextVar()
{
	boardHandler->nextVariation();
}


void BoardWindow::slotNavStartVar()
{
	boardHandler->gotoVarStart();
}

void BoardWindow::slotNavMainBranch()
{
	boardHandler->gotoMainBranch();
}

void BoardWindow::slotNavNextBranch()
{
	boardHandler->gotoNextBranch();
}

void BoardWindow::slotNavIntersection() 
{
	boardHandler->navIntersection();
}


*/
void BoardWindow::slotNthMove() 
{
	int n = 5;
	qDebug("slider moved to pos %d", n);
	boardHandler->slotNthMove(n);
}
