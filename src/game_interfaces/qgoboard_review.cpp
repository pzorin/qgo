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
#include "gamedata.h"
#include "boardwindow.h"
#include "board.h"

#include <QMessageBox>

qGoBoardReviewInterface::qGoBoardReviewInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoardNetworkInterface(bw,  t, gd) //, QObject(bw)
{
	game_Id = QString::number(gd->number);

    boardwindow->clearData();

	QSettings settings;
	// value 1 = no sound, 0 all games, 2 my games
	playSound = (settings.value("SOUND") != 1);
	
    tree->slotNavFirst();
}


/*
 * local move request to be sent to the server
 */
void qGoBoardReviewInterface::localMoveRequest(StoneColor c, int x, int y)
{
//	if (doMove(c,x,y))
		// FIXME : this should be made in a better way : wait for the interface to acknowledge before adding the move to the tree
//	{
//		boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
		sendMoveToInterface(c,x,y);
//	}
	
}

/* This is like a special set_move and should be more clear */
/*
 * A node is incoming from the interface (server)
 */
void qGoBoardReviewInterface::setNode(int node_nr, StoneColor sc, int x, int y)
{
	Move *m = tree->findNode(tree->getRoot(), node_nr);

	if (m)
	//node found, we go there
        tree->setCurrent(m);
	else
	// no node found, we create one from the current move
	{
        if (doMove(sc, x,y))
            tree->getCurrent()->setNodeIndex(node_nr);
        else
			QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move %1,%2 seems to be invalid").arg(x).arg(y));
	}
}



#ifdef OLD
/*
 * A move string is incoming from the interface (server)
 * TODO : code duplicate : make sure we can't send this to qgoboard
 */
void qGoBoardReviewInterface::set_move(StoneColor sc, QString pt, QString mv_nr)
{

}

/*
 * sends a "pass" move to the server
 */
void qGoBoardReviewInterface::sendPassToInterface(StoneColor /*c*/)
{
	/* What really is this doing in the review interface ??? */
	emit signal_sendCommandFromBoard("pass", false);
	boardwindow->getBoardDispatch()->sendMove(new MoveRecord(MoveRecord::PASS));
}
#endif //OLD

/*
 * sends a move to the server
 */
#ifdef OLD
void qGoBoardReviewInterface::sendMoveToInterface(StoneColor c, int x, int y)
{

	if (x > 8)
		x++;

	char c1 = x - 1 + 'A';
	//int c2 = gd.size + 1 - y;
	int c2 = boardwindow->getBoardSize()  + 1 - y;

//	if (ExtendedTeachingGame && IamPupil)
//		emit signal_sendcommand("kibitz " + QString::number(id) + " " + QString(c1) + QString::number(c2), false);
	QString id = "";

//	if (gsName == IGS)
		id = game_Id;
	/* Where and why would we get the number here, is 0 okay for now ??? FIXME */

	boardwindow->getBoardDispatch()->sendMove(new MoveRecord(0, x, y, c)); 

	emit signal_sendCommandFromBoard(QString(c1) + QString::number(c2) + " " + id, false);

	

}
#endif //OLD

/*
 * 'undo' button pressed
 */
void qGoBoardReviewInterface::slotUndoPressed() 
{

}
