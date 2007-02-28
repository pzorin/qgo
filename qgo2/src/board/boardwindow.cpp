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


BoardWindow::BoardWindow(QWidget * parent, Qt::WindowFlags flags, int size)
	: QMainWindow( parent,  flags )
{
	boardSize = size;

	ui.setupUi(this);
	ui.board->init(size);

	tree = new Tree(size);

	interfacehandler = new InterfaceHandler( this);
	boardhandler = new BoardHandler(this, tree, size);
	
	/*
	 * Connects the nav buttons to the slots
	 */
	connect(ui.navForward,SIGNAL(pressed()),SLOT(slotNavForward()));
	connect(ui.navBackward,SIGNAL(pressed()),SLOT(slotNavBackward()));
	connect(ui.navNextVar, SIGNAL(pressed()), this, SLOT(slotNavNextVar()));
	connect(ui.navPrevVar, SIGNAL(pressed()), this, SLOT(slotNavPrevVar()));
	connect(ui.navFirst, SIGNAL(pressed()), this, SLOT(slotNavFirst()));
	connect(ui.navLast, SIGNAL(pressed()), this, SLOT(slotNavLast()));
	connect(ui.navMainBranch, SIGNAL(pressed()), this, SLOT(slotNavMainBranch()));
	connect(ui.navStartVar, SIGNAL(pressed()), this, SLOT(slotNavStartVar()));
	connect(ui.navNextBranch, SIGNAL(pressed()), this, SLOT(slotNavNextBranch()));
	connect(ui.navIntersection, SIGNAL(pressed()), this, SLOT(slotNavIntersection()));
	connect(ui.navPrevComment, SIGNAL(pressed()), this, SLOT(slotNavPrevComment()));
	connect(ui.navNextComment, SIGNAL(pressed()), this, SLOT(slotNavNextComment()));
}

BoardWindow::~BoardWindow()
{
}

void BoardWindow::closeEvent(QCloseEvent *)
{
}

/*
 * Loads the SGF file into its 'tree'
 * We may need to put this in the Board Handler class
 */
bool BoardWindow::loadSGF(const QString &fileName, const QString & /* filter*/, bool /* fastLoad */)
{

	boardhandler->loadSGF(fileName);
/*	
	SGFParser *sgfParser = new SGFParser(tree);
	
	// Load the sgf file
	QString SGFloaded = sgfParser->loadFile(fileName);

	if (!sgfParser->doParse(SGFloaded))
		return false ;	
	
	//prepareBoard();
	ui.board->clearData();
	tree->setToFirstMove();	
*/

	return true;
}

void BoardWindow::slotNavBackward()
{
	boardhandler->previousMove();
}

void BoardWindow::slotNavForward()
{
	boardhandler->nextMove();
}

void BoardWindow::slotNavFirst()
{
	boardhandler->gotoFirstMove();
}

void BoardWindow::slotNavLast()
{
	boardhandler->gotoLastMove();
}

void BoardWindow::slotNavNextComment()
{
	boardhandler->nextComment();
}

void BoardWindow::slotNavPrevComment()
{
	boardhandler->previousComment();
}

void BoardWindow::slotNavPrevVar()
{
	boardhandler->previousVariation();
}

void BoardWindow::slotNavNextVar()
{
	boardhandler->nextVariation();
}


void BoardWindow::slotNavStartVar()
{
	boardhandler->gotoVarStart();
}

void BoardWindow::slotNavMainBranch()
{
	boardhandler->gotoMainBranch();
}

void BoardWindow::slotNavNextBranch()
{
	boardhandler->gotoNextBranch();
}

void BoardWindow::slotNavIntersection() 
{
	boardhandler->navIntersection();
}
