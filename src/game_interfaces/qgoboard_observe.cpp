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
#include "qgtp.h"
#include "tree.h"
#include "move.h"
#include "network/boarddispatch.h"
#include "network/messages.h"
#include "boardwindow.h"


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
