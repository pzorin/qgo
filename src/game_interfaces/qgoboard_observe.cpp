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


qGoBoardObserveInterface::qGoBoardObserveInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoard(bw,  t, gd) //, QObject(bw)
{
	game_Id = QString::number(gd->gameNumber);
}



/*
 * This functions initialises the board for observing a game
 */
bool qGoBoardObserveInterface::init()
{
	boardwindow->getUi().board->clearData();

	emit signal_sendCommandFromBoard("games " + game_Id, FALSE);
	emit signal_sendCommandFromBoard("moves " +  game_Id, FALSE);
	emit signal_sendCommandFromBoard("all " +  game_Id, FALSE);

	return TRUE;

}



