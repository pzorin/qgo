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


#include "qgoboard.h"
#include "tree.h"
#include "move.h"
#include "network/boarddispatch.h"
#include "network/messages.h"
#include "clockdisplay.h"
#include "boardwindow.h"
#include <QMessageBox>

qGoBoardMatchInterface::qGoBoardMatchInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoardNetworkInterface(bw,  t, gd) //, QObject(bw)
{	
//	warningSound = settings.value("BYO_SOUND_WARNING").toBool();
//	warningSecs = settings.value("BYO_SEC_WARNING").toInt();
	
	if(bw->getBoardDispatch()->startTimerOnOpen() && 
		  (bw->getBoardDispatch()->clientCountsTime() || bw->getBoardDispatch()->clientSendsTime()))
		boardTimerId = startTimer(1000);
	
    tree->slotNavLast();
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
	//color is ignored as far as I know, still iffy FIXME
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
	/* I added this clock stopping stuff but it seems unnecessary FIXME
	 * we could leave it in just in case, actually
	 * might still somehow be necessary for clientSendsTime()*/
	if(boarddispatch->isClockStopped())
		return;
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
/* FIXME Looks like this function was used for more than it is now and the
 * stuff with the interface handler, which I assume goes to the clockdisplay...
 * its all a little awkward. */
void qGoBoardMatchInterface::setTimerInfo(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	//int bt_i = btime.toInt();
	//int wt_i = wtime.toInt();
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
	//QTime t0 = QTime::QTime(0,0);
//	t0.addSecs(bt_i).toString("m:ss");
	//QTime t1 = t0;

	// set initial timer until game is initialized
//	if (!have_gameData)
//		win->getInterfaceHandler()->setTimes(bt, bstones, wt, wstones);

	//if (boardwindow->getGamePhase() != phaseInit)
	//	boardwindow->getInterfaceHandler()->setTimes(t0.addSecs(bt_i).toString("m:ss"), bstones, t0.addSecs(wt_i).toString("m:ss"), wstones);
	if (boardwindow->getGamePhase() != phaseInit)
		boardwindow->getClockDisplay()->setTimeInfo(btime.toInt(), bstones.toInt(), wtime.toInt(), wstones.toInt());
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
#ifdef OLD
	emit signal_sendCommandFromBoard("review create_prevgame", false); 
#endif //OLD
}

void qGoBoardMatchInterface::slotDrawPressed()
{
	QMessageBox mb(tr("Request Draw?"),
			QString(tr("Ask %1 to end game in draw?\n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
			QMessageBox::Question,
	  		QMessageBox::Yes | QMessageBox::Default,
   			QMessageBox::No | QMessageBox::Escape,
   			QMessageBox::NoButton);
	mb.raise();
//	qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
		boardwindow->getBoardDispatch()->sendRequestDraw();
}

void qGoBoardMatchInterface::slotCountPressed()
{
	QMessageBox mb(tr("Request Count?"),
			QString(tr("Ask %1 to end game?\n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
			QMessageBox::Question,
	  		QMessageBox::Yes | QMessageBox::Default,
   			QMessageBox::No | QMessageBox::Escape,
   			QMessageBox::NoButton);
	mb.raise();
//	qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
		boardwindow->getBoardDispatch()->sendRequestCount();
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
	QMessageBox::information(boardwindow, tr("Adjourn Declined"), tr("%1 has declined to adjourn the game.").arg(boardwindow->getBoardDispatch()->getOpponentName()));
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

void qGoBoardMatchInterface::requestCountDialog(void)
{
	QMessageBox mb(tr("End game?"),
			QString(tr("%1 requests count\n\nDo you accept ? \n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
			QMessageBox::Question,
   			QMessageBox::Yes | QMessageBox::Default,
   			QMessageBox::No | QMessageBox::Escape,
   			QMessageBox::NoButton);
	mb.raise();
//	qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
	{
		boardwindow->getBoardDispatch()->sendAcceptCountRequest();
		enterScoreMode();
	}
	else
	{
		boardwindow->getBoardDispatch()->sendRefuseCountRequest();
	}
}

void qGoBoardMatchInterface::recvRefuseCount(void)
{
	QMessageBox::information(boardwindow, tr("Count Declined"), tr("%1 has declined to count and end the game.").arg(boardwindow->getBoardDispatch()->getOpponentName()));
}

void qGoBoardMatchInterface::requestMatchModeDialog(void)
{
	QMessageBox mb(tr("Return to game?"),
			QString(tr("%1 requests return to match mode\n\nDo you accept ? \n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
			QMessageBox::Question,
   			QMessageBox::Yes | QMessageBox::Default,
   			QMessageBox::No | QMessageBox::Escape,
   			QMessageBox::NoButton);
	mb.raise();
//	qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
	{
		boardwindow->getBoardDispatch()->sendAcceptMatchModeRequest();
		leaveScoreMode();
	}
	else
	{
		boardwindow->getBoardDispatch()->sendRefuseMatchModeRequest();
	}
}

void qGoBoardMatchInterface::recvRefuseMatchMode(void)
{
	QMessageBox::information(boardwindow, tr("Match mode declined"), tr("%1 has declined to return to the game.").arg(boardwindow->getBoardDispatch()->getOpponentName()));
}

void qGoBoardMatchInterface::requestDrawDialog(void)
{
	QMessageBox mb(tr("End game?"),
		 	QString(tr("%1 requests draw\n\nDo you accept ? \n")).arg(boardwindow->getBoardDispatch()->getOpponentName()),
			QMessageBox::Question,
	  		QMessageBox::Yes | QMessageBox::Default,
   			QMessageBox::No | QMessageBox::Escape,
   			QMessageBox::NoButton);
	mb.raise();
//	qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
	{
		boardwindow->getBoardDispatch()->sendAcceptDrawRequest();
		//FIXME
	}
	else
	{
		boardwindow->getBoardDispatch()->sendRefuseDrawRequest();
	}
}

void qGoBoardMatchInterface::recvRefuseDraw(void)
{
	QMessageBox::information(boardwindow, tr("Draw Declined"), tr("%1 has declined to draw the game.").arg(boardwindow->getBoardDispatch()->getOpponentName()));
}
