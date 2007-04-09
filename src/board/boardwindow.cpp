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
#include "move.h"

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

	ui.actionWhatsThis = QWhatsThis::createAction ();


	// Initialises the buttons and else
	editButtons = new QButtonGroup(this);
	editButtons->addButton(ui.stoneButton, 0);
	editButtons->addButton(ui.squareButton, 1);
	editButtons->addButton(ui.circleButton, 2);
	editButtons->addButton(ui.triangleButton, 3);
	editButtons->addButton(ui.crossButton, 4);
	editButtons->addButton(ui.labelLetterButton, 5);
	editButtons->addButton(ui.labelNumberButton, 6);
	editButtons->addButton(ui.colorButton, 7);

	QMenu *menu = new QMenu();
	menu->insertAction(0,ui.fileExportASCII);
	menu->insertAction(0,ui.fileExportSgfClipB);
	menu->insertAction(0,ui.fileExportPic);
	menu->insertAction(0,ui.fileExportPicClipB);

	ui.actionExport->setMenu(menu);

	QToolButton *exportButton = new QToolButton();
	exportButton->setDefaultAction(ui.actionExport);
	
	exportButton->setPopupMode( QToolButton::InstantPopup);
	ui.toolBar->insertWidget ( ui.actionImport, exportButton );

	//Creates the game tree
	tree = new Tree(boardSize);

	//creates the interface handler
	interfaceHandler = new InterfaceHandler( this);
	interfaceHandler->toggleMode(gameMode);

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


	connect(ui.actionCoordinates, SIGNAL(toggled(bool)), this, SLOT(slotViewCoords(bool)));

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

	//Connects the board to the interface and boardhandler
	connect(ui.board, SIGNAL(signalClicked(bool , int, int, Qt::MouseButton )) , 
		qgoboard , SLOT( slotBoardClicked(bool, int, int , Qt::MouseButton )));

	connect(ui.board, SIGNAL(signalWheelEvent(QWheelEvent*)),
		boardHandler, SLOT(slotWheelEvent(QWheelEvent*)));


	//Connects the game buttons to the slots
	connect(ui.passButton,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
	connect(ui.passButton_2,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
	connect(ui.scoreButton,SIGNAL(toggled(bool)), qgoboard, SLOT(slotScoreToggled(bool)));
//connect(ui.scoreButton,SIGNAL(pressed()), qgoboard, SLOT(slotPassPressed()));
	//Connects the 'edit' buttons to the slots
	connect(editButtons, SIGNAL(buttonPressed ( int )), 
		this, SLOT(slotEditButtonPressed( int )));

	//Loads the sgf file if any
	if (! gameData->fileName.isEmpty())
		loadSGF(gameData->fileName);

	
	qgoboard->init();		
	gamePhase = phaseOngoing;

	// This is only needed with a computer game, when the computer has to make the first move
	qgoboard->startGame();
}

BoardWindow::~BoardWindow()
{
}

void BoardWindow::closeEvent(QCloseEvent *)
{
}


/*
 * Loads the SGF string. returns true if the file was sucessfully parsed
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

/*
 * One of the buttons in the 'Edit Tools' frame has been pressed
 */
void BoardWindow::slotEditButtonPressed( int m )
{
	MarkType t;
	QString txt;
	
	switch(m)
	{
	case 0:
		t = markNone;
		break;
		
	case 1:
		t = markSquare;
		break;
		
	case 2:
		t = markCircle;
		break;
		
	case 3:
		t = markTriangle;
		break;
		
	case 4:
		t = markCross;
		break;
		
	case 5:
		t = markText;
		break;
		
	case 6:
		t = markNumber;
		break;

	case 7:
	{
		Move *current = tree->getCurrent();
		// set next move's color
		if (qgoboard->getBlackTurn())
		{
			current->setPLinfo(stoneWhite);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_WHITE));
//#else
			ui.colorButton->setIcon(QIcon(":/new/prefix1/ressources/pics/stone_white.png"));
//#endif
		}
		else
		{
			current->setPLinfo(stoneBlack);
//#ifndef USE_XPM
//			mainWidget->colorButton->setPixmap(QPixmap(ICON_NODE_BLACK));
//#else
			ui.colorButton->setIcon(QIcon(":/new/prefix1/ressources/pics/stone_black.png"));
//#endif
		}

		// check if set color is natural color:
		if (current->getMoveNumber() == 0 && current->getPLnextMove() == stoneBlack ||
			current->getMoveNumber() > 0 && current->getColor() != current->getPLnextMove())
			current->clearPLinfo();

//		board->setCurStoneColor();
		return;
	}
		
	default:
		return;
	}
	
//	statusMark->setText(getStatusMarkText(t));
//	board->setMarkType(t);
}

void BoardWindow::slotViewCoords(bool toggle)
{
	if (toggle)
		ui.board->setShowCoords(false);
	else
		ui.board->setShowCoords(true);
	
//	statusBar()->message(tr("Ready."));
}

