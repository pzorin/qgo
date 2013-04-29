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


#include "displayboard.h"
#include "stone.h"
#include "mark.h"
#include "board.h"
#include "imagehandler.h"

#include <QtWidgets>

DisplayBoard::DisplayBoard(QWidget *parent, QGraphicsScene *c)
: Board(parent,c)
{
    isDisplayBoard = true;
    showCoords = false;
    imageHandler->setDisplay(true);
}

DisplayBoard::~DisplayBoard()
{
}

/* Used by file browse display */
void DisplayBoard::displayHandicap(int handicap)
{
	int size = 19;
	int edge_dist = (size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (size + 1) / 2;
	int high = size + 1 - edge_dist;	


	switch (handicap)
	{
	case 9:
		updateStone(stoneBlack, middle, middle);
	case 8:
	case 7:
		if (handicap >= 8)
		{
			updateStone(stoneBlack, middle, low);
			updateStone(stoneBlack, middle, high);
		}
		else
			updateStone(stoneBlack, middle, middle);
	case 6:
	case 5:
		if (handicap >= 6)
		{
			updateStone(stoneBlack, low, middle);
			updateStone(stoneBlack, high, middle);
		}
		else
			updateStone(stoneBlack, middle, middle);
	case 4:
		updateStone(stoneBlack, high, high);
	case 3:
		updateStone(stoneBlack, low, low);
	case 2:
		updateStone(stoneBlack, high, low);
		updateStone(stoneBlack, low, high);
	default:
		break;
	}


}

