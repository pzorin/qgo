/***************************************************************************
 *   Copyright (C) 2006 by EB   *
 *      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "qgoboard.h"
#include "tree.h"
#include "move.h"
#include "network/boarddispatch.h"
#include "network/messages.h"

qGoBoardMatchInterface::qGoBoardMatchInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoardNetworkInterface(bw,  t, gd) //, QObject(bw)
{	
//	warningSound = settings.value("BYO_SOUND_WARNING").toBool();
//	warningSecs = settings.value("BYO_SEC_WARNING").toInt();
	
	if(bw->getBoardDispatch()->startTimerOnOpen() && 
		  (bw->getBoardDispatch()->clientCountsTime() || bw->getBoardDispatch()->clientSendsTime()))
		boardTimerId = startTimer(1000);
	
	boardwindow->getBoardHandler()->slotNavLast();
}

/* This timer stuff is awkward looking. FIXME */
void qGoBoardMatchInterface::startGame(void)
{
	if(!boardwindow->getBoardDispatch()->startTimerOnOpen() && 
		   (boardwindow->getBoardDispatch()->clientCountsTime() || boardwindow->getBoardDispatch()->clientSendsTime()))
		boardTimerId = startTimer(1000);
}

void qGoBoardMatchInterface::onFirstMove(void)
{
	//we can now start the timer
	if(!boardwindow->getBoardDispatch()->startTimerOnOpen() && 
		   (boardwindow->getBoardDispatch()->clientCountsTime() || boardwindow->getBoardDispatch()->clientSendsTime()))
		boardTimerId = startTimer(1000);				
}

/*
 * We subclass this function, because the server will send the move back
 * so there is no need to have it displayed
 */
void qGoBoardMatchInterface::localMoveRequest(StoneColor c, int x, int y)
{
	sendMoveToInterface(c,x,y);
}

/*
 * This functions gets the request (from a board click)
 * to mark a stone as dead/undead (score mode). this sends the click coords to server
 */
void qGoBoardMatchInterface::localMarkDeadRequest(int x,  int y)
{
	sendMoveToInterface(stoneBlack,x,y);
}


void qGoBoardMatchInterface::timerEvent(QTimerEvent*)
{
	if (boardwindow->getGamePhase() != phaseOngoing)
		return;
	BoardDispatch * boarddispatch = boardwindow->getBoardDispatch();
	if(!boarddispatch)
	{
		qDebug("Match timer event but no board dispatch");
		return;
	}
	if(boarddispatch->clientCountsTime())
		boardwindow->getClockDisplay()->setTimeStep(getBlackTurn());
	
	
	if ((getBlackTurn() && boardwindow->getMyColorIsBlack()) ||
	   ((!getBlackTurn()) && boardwindow->getMyColorIsWhite()))
	{
		if(!boarddispatch->clientCountsTime() && boarddispatch->clientSendsTime())
			boardwindow->getClockDisplay()->setTimeStep(getBlackTurn());
		
		/* FIXME, probably don't want to send time and time loss... even if
		 * network protocols are exclusive with these... */
		if(!boardwindow->getClockDisplay()->warning(getBlackTurn()))
			boarddispatch->sendTimeLoss();
		
		if(boarddispatch->clientSendsTime())
			boarddispatch->sendTime();
	}		
}


/*
 *  time info has been send by parser
 *  TODO : make sure we won't need this in qgoboard (code duplicate)
 */
void qGoBoardMatchInterface::setTimerInfo(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	int bt_i = btime.toInt();
	int wt_i = wtime.toInt();
//	b_stones = bstones;
//	w_stones = wstones;
/*
#ifdef SHOW_INTERNAL_TIME
	if (chk_b < 0)
	{
		chk_b = bt_i;
		chk_w = wt_i;
	}
#endif
*/
	// set string in any case
//	bt = secToTime(bt_i);
//	wt = secToTime(wt_i);
	QTime t0 = QTime::QTime(0,0);
//	t0.addSecs(bt_i).toString("m:ss");
	QTime t1 = t0;

	// set initial timer until game is initialized
//	if (!have_gameData)
//		win->getInterfaceHandler()->setTimes(bt, bstones, wt, wstones);

	if (boardwindow->getGamePhase() != phaseInit)
		boardwindow->getInterfaceHandler()->setTimes(t0.addSecs(bt_i).toString("m:ss"), bstones, t0.addSecs(wt_i).toString("m:ss"), wstones);

	// if time info available, sound can be played
	// cause no moves cmd in execution
//	sound = true;
}



/*
 * initialises the scoring phase in match mode
 */
void qGoBoardMatchInterface::enterScoreMode()
{
	if(boardwindow->getGamePhase() == phaseScore)
		return;
	stopTime();
	qGoBoard::enterScoreMode();

	//boardwindow->getUi().doneButton->setEnabled(true);

	kibitzReceived(tr("SCORE MODE: click on a stone to mark as dead..."));
}

void qGoBoardMatchInterface::leaveScoreMode()
{
	if(boardwindow->getGamePhase() != phaseScore)
		return;
	qGoBoard::leaveScoreMode();
	/* Make sure this doesn't conflict with game result stuff */
	if(!boardwindow->getBoardDispatch()->startTimerOnOpen() && 
		   (boardwindow->getBoardDispatch()->clientCountsTime() || boardwindow->getBoardDispatch()->clientSendsTime()))
		boardTimerId = startTimer(1000);

	//boardwindow->getUi().doneButton->setEnabled(true);

	kibitzReceived(tr("LEAVING SCORE MODE"));	//awkward text FIXME
}

/*
 * 'resign button pressed
 */
void qGoBoardMatchInterface::slotReviewPressed()
{
	emit signal_sendCommandFromBoard("review create_prevgame", FALSE); 
}



/*
 * 'adjourn button pressed
 */
void qGoBoardMatchInterface::slotAdjournPressed()
{
	qDebug("qGBMI::slotAdjournPressed()");
	/* I'm going to tie this to the destructor of the board instead */
	/* Scratch that... actually, its supposed to be something
	 * that can be denied although that's weird to me... since
	 * they can always close the window.*/
	/* But I have to figure out the IGS command first, at least...*/
	QMessageBox mb(tr("Adjourn?"),
		       QString(tr("Ask %1 to adjourn?\n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
		       QMessageBox::Question,
		       QMessageBox::Yes | QMessageBox::Default,
		       QMessageBox::No | QMessageBox::Escape,
		       QMessageBox::NoButton);
	mb.raise();
//		qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
	{
		boardwindow->getBoardDispatch()->sendAdjournRequest();
	}
}

void qGoBoardMatchInterface::recvRefuseAdjourn(void)
{
	QMessageBox::information(boardwindow, tr("Adjourn Declined"), boardwindow->getBoardDispatch()->getOpponentName() + tr(" has declined to adjourn the game."));
}

void qGoBoardMatchInterface::requestAdjournDialog(void)
{
	QMessageBox mb(tr("Adjourn?"),
	       QString(tr("%1 wants to adjourn\n\nDo you accept ? \n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
	       QMessageBox::Question,
	       QMessageBox::Yes | QMessageBox::Default,
	       QMessageBox::No | QMessageBox::Escape,
	       QMessageBox::NoButton);
		mb.raise();
//		qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
	{
		boardwindow->getBoardDispatch()->sendAdjourn();
		/* FIXME
		 * Once we hit adjourn, the window should close, or at the very least
		 * should change phase and the clocks should stop.  */
		boardwindow->setGamePhase(phaseEnded);
		// this should active Done button and disable other buttons (review okay?)
		// also fix above instance !!!
	}
	else
	{
		boardwindow->getBoardDispatch()->sendRefuseAdjourn();
	}
}
