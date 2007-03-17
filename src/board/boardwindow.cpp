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
#include "qgoboard.h"

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
	gameMode = modeNormal;
	myColorIsBlack = TRUE;
	myColorIsWhite = TRUE;
	init();
}

BoardWindow::BoardWindow( QWidget *parent , Qt::WindowFlags flags , GameData *gd , GameMode gm , bool iAmBlack , bool iAmWhite)
	: QMainWindow( parent,  flags )
{

	gameData = new GameData(gd);

	gameMode = gm;
	myColorIsBlack = iAmBlack;
	myColorIsWhite = iAmWhite;

	boardSize = gd->size;
	init();

}

void BoardWindow::init()
{
	gamePhase = phaseInit;

	ui.setupUi(this);
	ui.board->init(boardSize);

	tree = new Tree(boardSize);

	//creates the interface handler
	interfaceHandler = new InterfaceHandler( this);
	if (gameData)
		interfaceHandler->updateCaption(gameData);

	//creates the board interface (or proxy) that will handle the moves an command requests
	switch (gameMode)
	{
		case modeNormal :
		{
			qgoboard = new 	qGoBoardNormalInterface(this, tree,gameData);
			break;
		}
		case modeComputer :
		{
			qgoboard = new 	qGoBoardComputerInterface(this, tree,gameData);
			break;	
		}
		default:
			break;
	}

	// creates the board handler for noavigating in the tree
	boardHandler = new BoardHandler(this, tree, boardSize);


	// Connects the nav buttons to the slots
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
	connect(ui.slider, SIGNAL(sliderMoved ( int)), boardHandler , SLOT(slotNthMove(int)));

	//Connects the board to the interface
	connect(ui.board, SIGNAL(signalClicked(bool , int, int, Qt::MouseButton )) , 
		qgoboard , SLOT( slotBoardClicked(bool, int, int , Qt::MouseButton )));

	//Loads the sgf file if any
	if (! gameData->fileName.isEmpty())
		loadSGF(gameData->fileName);

	
	qgoboard->init();		
	gamePhase = phaseOngoing;
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
bool BoardWindow::loadSGF(const QString fileName, const QString /*SGFLoaded*/, bool /* fastLoad */)
{

	SGFParser *sgfParser = new SGFParser(tree);
	
	// Load the sgf file
	QString SGFLoaded = sgfParser->loadFile(fileName);

	if (!sgfParser->doParse(SGFLoaded))
		return false ;	
	
//	board->clearData();
//	tree->setToFirstMove();	
//	boardHandler->slotNavFirst();

	return true;
}





