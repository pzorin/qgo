/***************************************************************************
 *   Copyright (C) 2009-2014 by The qGo Project                            *
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

#ifndef QGOBOARDLOCALINTERFACE_H
#define QGOBOARDLOCALINTERFACE_H

#include "defines.h"
#include "qgoboard.h"

class BoardWindow;
class Tree;
class GameData;
class Move;
class QGtp;
class Sound;

class qGoBoardLocalInterface : public qGoBoard
{
    Q_OBJECT
public:
    qGoBoardLocalInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
    ~qGoBoardLocalInterface();

    void checkComputersTurn(bool force = false);
    void feedPositionThroughGtp();

signals:

public slots:
    virtual void localMoveRequest(StoneColor c, int x, int y);
    void slot_playComputer(int x, int y);
    void slot_resignComputer();
    void slot_passComputer();
    virtual void slotDonePressed();
    virtual void slotUndoPressed();
    void slotToggleInsertStones(bool val);

private:
    virtual void sendMoveToInterface(StoneColor c,int x, int y);
    virtual void sendPassToInterface(StoneColor c);

    void startGame() {}
//	void enterScoreMode() {}
    void leaveScoreMode() {}

    QGtp *gtp;
    Move *currentEngine;
    bool insertStoneFlag;
};

#endif // QGOBOARDLOCALINTERFACE_H
