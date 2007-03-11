/***************************************************************************
 *
 * interfacehandler.cpp
 * This class deals with the tree information taken from the tree when 
 * navigating in it.
 * It is called by the BoardHandler (which does the actual navigation), and 
 * sets the interface widgets accodrding to the moves information
 *
 ***************************************************************************/


//#include "defines.h"
#include "interfacehandler.h"
#include "boardwindow.h"

/*
struct ButtonState
{
    bool navPrevVar, navNextVar, navBackward, navForward, navFirst, navStartVar, navMainBranch,
		navLast, navNextBranch, navPrevComment, navNextComment, navIntersection, editPaste, editPasteBrother; // SL added eb 11
};
*/

InterfaceHandler::InterfaceHandler(BoardWindow *bw)
{
	boardwindow = bw;
//    buttonState = new ButtonState;
	scored_flag = false;
}

InterfaceHandler::~InterfaceHandler()
{
//    delete buttonState;
}


void InterfaceHandler::clearData()
{
    // qDebug("void InterfaceHandler::clearData()");
	
//	clearComment();
//	setMoveData(0, true, 0, 0, false, false, false);
//    modeButton->setOn(false);
//	  mainWidget->setToolsTabWidget(tabNormalScore);
//    mainWidget->editButtonGroup->setButton(0);
//    editTools->hide();
	boardwindow->capturesBlack->setText("0");
	boardwindow->capturesWhite->setText("0");
	
//	if (board->getGameMode() != modeObserve && 
//		board->getGameMode() != modeMatch &&
//		board->getGameMode() != modeTeach)
//	{
		boardwindow->pb_timeBlack->setText("00:00");
		boardwindow->pb_timeWhite->setText("00:00");
//	}
//    normalTools->show();
	boardwindow->scoreButton->setDown(false);
//	editPaste->setEnabled(false);
//	editPasteBrother->setEnabled(false);
	boardwindow->slider->setValue(0);
//	setSliderMax(SLIDER_INIT);
	scored_flag = false;
}

/*
 * displays the informations relative to a game on the board window
 * TODO : decide wether this is here or at boardwindow level
 */
void InterfaceHandler::updateCaption(GameData *gd)
{
    // Print caption
    // example: qGo 0.0.5 - Zotan 8k vs. tgmouse 10k
    // or if game name is given: qGo 0.0.5 - Kogo's Joseki Dictionary
	boardwindow->setWindowTitle( /* QString(isModified ? "* " : "") + */
		(gd->gameNumber != 0 ?
		"(" + QString::number(gd->gameNumber) + ") " : QString()) +
		(gd->gameName.isEmpty() ?
		gd->playerWhite +
		(!gd->rankWhite.isEmpty() ?
		" " + gd->rankWhite : QString())
		+ " " + QObject::tr("vs.") + " "+
		gd->playerBlack +
		(!gd->rankBlack.isEmpty() ?
		" " + gd->rankBlack : QString()) :
		gd->gameName) +
		"   " + QString(PACKAGE   " "  VERSION));


	bool simple = gd->rankWhite.length() == 0 && gd->rankBlack.length() == 0;
	QGroupBox *gb = boardwindow->getUi().whiteFrame;

	QString player = gd->playerWhite;
	if (simple && player == QObject::tr("White"))
		gb->setTitle(QObject::tr("White"));	
	else
	{
		// truncate to 12 characters max
		player.truncate(12);

		if (gd->rankWhite.length() != 0)
			player = QObject::tr("W") + ": " + player + " " + gd->rankWhite;
		else
			player = QObject::tr("W") + ": " + player;
		
		gb->setTitle(player);
	}

	gb = boardwindow->getUi().blackFrame;

	player = gd->playerBlack;
	if (simple && player == QObject::tr("Black"))
		gb->setTitle(QObject::tr("Black"));	
	else
	{
		// truncate to 12 characters max
		player.truncate(12);

		if (gd->rankBlack.length() != 0)
			player = QObject::tr("B") + ": " + player + " " + gd->rankBlack;
		else
			player = QObject::tr("B") + ": " + player;
		
		gb->setTitle(player);
	}
	
}

/*
 * displays the informations relative to a move on the board window
 */
void InterfaceHandler::setMoveData(int n, bool black, int brothers, int sons, bool hasParent, bool hasPrev, bool hasNext, int lastX, int lastY)
{
	QString s(QObject::tr("Move") + " ");
	s.append(QString::number(n));

	if (lastX >= 1 && lastX <= boardwindow->getBoardSize() && lastY >= 1 && lastY <= boardwindow->getBoardSize())
	{
		s.append(" (");
		s.append(black ? QObject::tr("W")+" " : QObject::tr("B")+" ");
		s.append(QString(QChar(static_cast<const char>('A' + (lastX<9?lastX:lastX+1) - 1))) +
			QString::number(boardwindow->getBoardSize()-lastY+1) + ")");
	}

	else if (lastX == 20 && lastY == 20)  // Pass
	{
		s.append(" (");
		s.append(black ? QObject::tr("W")+" " : QObject::tr("B")+" ");
		s.append(" " + QObject::tr("Pass") + ")");
	}

	boardwindow->getUi().moveNumLabel->setText(s);
//	statusTurn->setText(" " + s.right(s.length() - 5) + " ");  // Without 'Move '
	
//	statusNav->setText(" " + QString::number(brothers) + "/" + QString::number(sons));
	s = black ? QObject::tr("Black to play") : QObject::tr("White to play");
	boardwindow->getUi().turnLabel->setText(s);
	
	s = "";
	s.append(QString::number(brothers));
	if (brothers == 1)
		s.append(" " + QObject::tr("brother") + "\n");
	else
		s.append(" " + QObject::tr("brothers") + "\n");

	s.append(QString::number(sons));
	if (sons == 1)
		s.append(" " + QObject::tr("son"));
	else
		s.append(" " + QObject::tr("sons"));
	boardwindow->getUi().varLabel->setText(s);
	
//	if (board->getGameMode() == modeNormal || board->getGameMode() == modeEdit)
//	{
		// Update the toolbar buttons
		boardwindow->getUi().navPrevVar->setEnabled(hasPrev);
		boardwindow->getUi().navNextVar->setEnabled(hasNext);
		boardwindow->getUi().navBackward->setEnabled(hasParent);
		boardwindow->getUi().navForward->setEnabled(sons);
		boardwindow->getUi().navFirst->setEnabled(hasParent);
		boardwindow->getUi().navStartVar->setEnabled(hasParent);
		boardwindow->getUi().navMainBranch->setEnabled(hasParent);
		boardwindow->getUi().navLast->setEnabled(sons);
		boardwindow->getUi().navNextBranch->setEnabled(sons);
		boardwindow->getUi().swapVarButton->setEnabled(hasPrev);
		boardwindow->getUi().navPrevComment->setEnabled(hasParent);
		boardwindow->getUi().navNextComment->setEnabled(sons);
    		boardwindow->getUi().navIntersection->setEnabled(true);
		
		boardwindow->getUi().slider->setEnabled(true);
/*	}
	else  if (board->getGameMode() == modeObserve)
	{
		// Update the toolbar buttons
		navBackward->setEnabled(hasParent);
		navForward->setEnabled(sons);
		navFirst->setEnabled(hasParent);
		navLast->setEnabled(sons);
		navPrevComment->setEnabled(hasParent);
		navNextComment->setEnabled(sons);
		navIntersection->setEnabled(true);

		slider->setEnabled(true);

//		board->getBoardHandler()->display_incoming_move = !bool(sons);

	}
	else
		slider->setDisabled(true);
*/	
	// Update slider
	boardwindow->getUi().slider->blockSignals (TRUE);

//	int mv = boardwindow->getUi().slider->maximum();
//	int v = boardwindow->getUi().slider->value();

	if (boardwindow->getUi().slider->maximum() < n)
		  setSliderMax(n);

	// we need to be carefull with the slider :
	// normal case, slider is moved
//	if (board->getGameMode() != modeObserve ||
	// observing, but browsing (no incoming move)
//	(board->getGameMode() == modeObserve && mv >= n) ||
	// observing, but at the last move, and an incoming move occurs 
//	(board->getGameMode() == modeObserve && mv < n && v==n-1))
		boardwindow->getUi().slider->setValue(n);

	boardwindow->getUi().slider->blockSignals (TRUE);
}


/*
 * display text in the comment area
 */
void InterfaceHandler::displayComment(const QString &c)
{
//	if (board->get_isLocalGame())
//	{
		if (c.isEmpty())
			boardwindow->getUi().commentEdit->clear();
		else
			boardwindow->getUi().commentEdit->setText(c);
//	}
//	else if (!c.isEmpty())
//			commentEdit->append(c);
}

/*
 * modifies the maximum value of the slider (used when a move is added)
 */
void InterfaceHandler::setSliderMax(int n)
{
	if (n < 0)
		n = 0;

	boardwindow->getUi().slider->setMaximum(n);
    	boardwindow->getUi().sliderRightLabel->setText(QString::number(n));
}

/*
 * Sets the number of prisonners on the UI
 */
void InterfaceHandler::setCaptures(float black, float white)
{

	boardwindow->getUi().capturesBlack->setText(QString::number(black));
	boardwindow->getUi().capturesWhite->setText(QString::number(white));
}
