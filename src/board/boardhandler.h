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


#ifndef BOARDHANDLER_H
#define BOARDHANDLER_H

#include "../defines.h"

class BoardWindow;
class Tree;
class Board;
class Move;
class QWheelEvent;

class BoardHandler : public QObject
{
	Q_OBJECT
public:
	BoardHandler(BoardWindow *bw, Tree *tree, int * board_size);
	~BoardHandler();

	void clearData();
	bool markedDead;
	void countScore();
	void countMarked(void);
	class GameResult retrieveScore(void);
	void exitScore();
	void updateMove(Move *m=0, bool ignore_update = false);
	bool updateAll(Move *move, bool toDraw=true);
	void updateCursor(StoneColor c=stoneNone);
	void updateButtons(StoneColor c=stoneNone);
	void gotoMove(Move *m);
//	bool loadSGF(const QString fileName, const QString SGFLoaded=0, bool fastLoad=false);

	void findMoveByPos(int x,int  y);

public slots:
	void slotNavBackward();
	void slotNavForward();
	void slotNavFirst();
	void slotNavLast();
	void slotNavPrevComment();
	void slotNavNextComment();
	void slotNavPrevVar();
	void slotNavNextVar();	
	void slotNavStartVar();
	void slotNavMainBranch();
	void slotNavNextBranch();
	void slotNavIntersection();
	void slotNthMove(int n);

	void slotWheelEvent(QWheelEvent *e);

private:
	Board *board;
	Tree *tree;
	BoardWindow * boardwindow;

	bool updateAll_updateAll;
	int capturesBlack, capturesWhite, caps_black, caps_white;
	int terrBlack, terrWhite;
	Move *lastValidMove;
//	GameMode gameMode;
	int * boardSize;
	
	void updateVariationGhosts(Move *m);
	bool navIntersectionStatus;

	QTime wheelTime;

};

#endif
