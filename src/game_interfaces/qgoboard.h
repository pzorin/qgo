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


/***************************************************************************
*
* This class is the most important
* It  deals with distributing the move (and  other commands) requests 
* It is subclassed in the different proxies that can be used : local game,
* server game, and computer game
* Its the only class (apart from 'sgf parser') to modify the game tree
* 
***************************************************************************/

#ifndef QGOBOARD_H
#define QGOBOARD_H

#include "defines.h"

class BoardWindow;
class Tree;
class GameData;
class Move;
class QGtp;
class Sound;

/* I think its easier to have the computer and normal modes override a few functions to
 * null then have review and match provide the same complicated net functionality */


class qGoBoard : public QObject //, public Misc<QString>
{
	Q_OBJECT

public:
	qGoBoard(BoardWindow *bw, Tree * t, GameData *gd);
//	qGoBoard(qGoBoard &qgoboard );
    virtual ~qGoBoard() {}
	virtual void setHandicap(int handicap);
	virtual void addStone(StoneColor c, int x, int y);

	virtual void addMark( int x, int y, MarkType t );
	virtual void removeMark( int x, int y);

//	virtual void localMoveRequest(int x, int y)=0;
	virtual bool getBlackTurn(bool time = false);
    virtual void startGame() {}
    virtual void stopTime() {}
    virtual void handleMove(class MoveRecord *) {}
    virtual void moveControl(QString &) {}
	int getMoveNumber(void);
	//virtual void set_havegd(bool b) 		{ have_gameData = b; }
	virtual void setModified(bool b= true)		{ isModified = b;}
	virtual bool getModified()			{ return isModified; }
	virtual bool getPlaySound()			{ return playSound;}
	
	virtual void setResult(class GameResult & );
	virtual void kibitzReceived(const QString& txt);
	virtual void setTimerInfo(const QString&, const QString&, const QString&, const QString&) {}
	virtual void timerEvent(QTimerEvent*);
	class TimeRecord getOurTimeRecord(void);		//awkward
	class TimeRecord getTheirTimeRecord(void);
	virtual void enterScoreMode();
	virtual void leaveScoreMode();
	void toggleGroupAt(int x, int y);
	virtual void markDeadStone(int x, int y);
	virtual void markLiveStone(int x, int y);
	virtual void markDeadArea(int x, int y);
	virtual void markLiveArea(int x, int y);

	virtual void setNode(int , StoneColor , int , int ) {}
    virtual void requestAdjournDialog(void) {}
    virtual void requestCountDialog(void) {}
    virtual void requestMatchModeDialog(void) {}
    virtual void requestDrawDialog(void) {}
    virtual void adjournGame(void) {}
    virtual void recvRefuseAdjourn(void) {}
    virtual void recvRefuseCount(void) {}
    virtual void recvRefuseMatchMode(void) {}
    virtual void recvRefuseDraw(void) {}

	// Board
	virtual void slotBoardClicked(bool, int, int , Qt::MouseButton );
    virtual void passRequest();
	virtual void slotDonePressed();
	virtual void slotResignPressed();
    virtual void slotReviewPressed() {}
    virtual void slotDrawPressed() {}
    virtual void slotCountPressed() {}
	virtual void slotUndoPressed();
    virtual void slotScoreToggled(bool);
    virtual void setPlaySound(bool b) 		{ playSound = b; }

protected:
	BoardWindow *boardwindow;
	Tree *tree;
	GameData *gameData;
	Sound *clickSound;
	int boardTimerId;

    bool	isModified;
    int         id;
	int	stated_mv_count;
	Move * lastMoveInGame;
    bool	playSound;

    /*
     * This functions gets the move request (from a board click)
     * and displays the resulting stone (if valid)
     * The base class implementation is empty because network and local games need completely different hadling.
     */
    virtual void localMoveRequest(StoneColor, int, int) {}
	virtual void localMarkDeadRequest(int x, int y);
    virtual void sendMoveToInterface(StoneColor ,int, int) {}
    virtual Move *doMove(StoneColor, int, int) { return NULL; }
    virtual void doPass(StoneColor c = stoneNone); // By default the player who is on turn will pass
protected:
	bool dontCheckValidity;
	QTime lastSound;
};
#endif
