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
#include "qgtp.h"
#include "tree.h"
#include "move.h"
#include "network/boarddispatch.h"
#include "network/messages.h"


qGoBoardObserveInterface::qGoBoardObserveInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoardNetworkInterface(bw,  t, gd) //, QObject(bw)
{
	if(bw->getBoardDispatch()->startTimerOnOpen() && bw->getBoardDispatch()->clientCountsTime())
		boardTimerId = startTimer(1000);
	boardwindow->getBoardDispatch()->requestGameInfo();
}

void qGoBoardObserveInterface::onFirstMove(void)
{
	//we can now start the timer
	if(!boardwindow->getBoardDispatch()->startTimerOnOpen() && boardwindow->getBoardDispatch()->clientCountsTime())
		boardTimerId = startTimer(1000);
}

/*
 *  time info has been send by parser
 *  TODO : make sure we won't need this in qgoboard (code duplicate)
 */
void qGoBoardObserveInterface::setTimerInfo(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	qDebug("Does anyone use qGoBoardObserveInterface::setTimerInfo??\n");
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
