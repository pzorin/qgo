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


void InterfaceHandler::setMoveData(int n, bool black, int brothers, int sons, bool /*hasParent*/, bool /*hasPrev*/, bool /*hasNext*/, int lastX, int lastY)
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
/*	
	if (board->getGameMode() == modeNormal || board->getGameMode() == modeEdit)
	{
		// Update the toolbar buttons
		navPrevVar->setEnabled(hasPrev);
		navNextVar->setEnabled(hasNext);
		navBackward->setEnabled(hasParent);
		navForward->setEnabled(sons);
		navFirst->setEnabled(hasParent);
		navStartVar->setEnabled(hasParent);
		navMainBranch->setEnabled(hasParent);
		navLast->setEnabled(sons);
		navNextBranch->setEnabled(sons);
		navSwapVariations->setEnabled(hasPrev);
		navPrevComment->setEnabled(hasParent);
		navNextComment->setEnabled(sons);
    		navIntersection->setEnabled(true);
		
		slider->setEnabled(true);
	}
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
	
	// Update slider
	mainWidget->toggleSliderSignal(false);

	int mv = boardwindow->slider->maxValue();
	int v = boardwindow->slider->value();

	if (boardwindow->slider->maxValue() < n)
		  setSliderMax(n);

	// we need to be carefull with the slider :
	// normal case, slider is moved
	if (board->getGameMode() != modeObserve ||
	// observing, but browsing (no incoming move)
	(board->getGameMode() == modeObserve && mv >= n) ||
	// observing, but at the last move, and an incoming move occurs 
	(board->getGameMode() == modeObserve && mv < n && v==n-1))
		slider->setValue(n);

	mainWidget->toggleSliderSignal(true);
*/
}


