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

#include "qgoboardlocalinterface.h"

#include "qgtp.h"
#include "tree.h"
#include "move.h"
#include "../network/messages.h"
#include "gamedata.h"
#include "boardwindow.h"
#include "qgtp.h"
#include "board.h"
#include "matrix.h"

#include <QMessageBox>

qGoBoardLocalInterface::qGoBoardLocalInterface(BoardWindow *bw, Tree * t, GameData *gd)
    : qGoBoard(bw, t, gd)
{
    this->setObjectName("qGoBoardLocalInterface");
    boardwindow->clearData();

    // If we have handicap, but not from a loaded file, we have to set the handicap move
    if (gameData->handicap && gameData->fileName.isEmpty())
        setHandicap(gameData->handicap);

    QSettings settings;
    // value 1 = no sound, 0 all games, 2 my games
    playSound = (settings.value("SOUND") != 1);

    // Set up computer interface.
    gtp = new QGtp() ;

    connect (gtp, SIGNAL(signal_computerPlayed(int, int)), SLOT(slot_playComputer(int, int)));
    connect (gtp, SIGNAL(computerResigned()), SLOT(slot_resignComputer()));
    connect (gtp, SIGNAL(computerPassed()), SLOT(slot_passComputer()));

    if (gtp->openGtpSession(settings.value("COMPUTER_PATH").toString(),
                gameData->board_size,
                gameData->komi,
                gameData->handicap,
                GNUGO_LEVEL)==FAIL)
    {
        QMessageBox msg(QMessageBox::Warning,
                        QObject::tr("Error"),
                        QObject::tr("Error opening program: %1").arg(gtp->getLastMessage()),
                        QMessageBox::Ok);
        msg.exec();
        delete gtp;
        gtp = NULL;
    }

    tree->setCurrent(tree->findLastMoveInMainBranch());

    feedPositionThroughGtp();
}

qGoBoardLocalInterface::~qGoBoardLocalInterface()
{
    qDebug("Deconstructing computer interface");
    delete gtp;
}


void qGoBoardLocalInterface::slotDonePressed()
{
    /* FIXME  Since there's no server to tell us the result, we have
     * to calculate it or whatever, print out a winner a score, etc..
     * I'm really surprised there's no existing code to do this
     * actually.  Also feels like the kind of thing that could
     * be in qGoBoard, some generic win reporting thing, score
     * reporting, etc.. and then the set_result functions could
     * call that qGoBoard thing with the specifics.*/
    qGoBoard::slotDonePressed();
    qDebug("Done Pressed\n");
}

void qGoBoardLocalInterface::slotUndoPressed()
{
    if(boardwindow->getGamePhase() == phaseScore)
        leaveScoreMode();
    else
    {
        if (gtp)
            gtp->undo(0);
        qGoBoard::slotUndoPressed();
    }
}

/*
 * This sends the move to the computer through GTP
 */
void qGoBoardLocalInterface::sendMoveToInterface(StoneColor c,int x, int y)
{
    if (!gtp)
        return;
    switch (c)
    {
        case stoneWhite :
        if (gtp->playwhite(x ,y))
            {
            QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to play the stone within program \n") + 	gtp->getLastMessage());
            return;
            }
        break;

        case stoneBlack :
        if (gtp->playblack(x ,y))
        {
            QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to play the stone within program \n") + gtp->getLastMessage());
            return;
        }
        break;

        default :
        ;
    }

    // Check whether the computer should reply
    checkComputersTurn();
}

/*
 * This asks the computer to make a move
 */
void qGoBoardLocalInterface::playComputer()
{
    if (!gtp)
        return;
    if (gtp->isBusy())
        return;
    int result;
    if (getBlackTurn())
        result= gtp->genmoveBlack();
    else
        result = gtp->genmoveWhite();
    if (result == FAIL)
        QMessageBox::warning(boardwindow, PACKAGE, tr("Move request from engine failed\n") + gtp->getLastMessage());
}

/*
 * This slot is triggered by the signal emitted by 'gtp' when getting a move
 */
void qGoBoardLocalInterface::slot_playComputer(int x, int y)
{
    bool b = getBlackTurn();
    if (!doMove(b ? stoneBlack : stoneWhite, x, y))
        QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move (%1, %2) seems to be invalid").arg(x).arg(y));

    // We can also let computer play against itself.
    checkComputersTurn();
}

void qGoBoardLocalInterface::slot_resignComputer()
{
    bool b = getBlackTurn();
    GameResult g((!b ? stoneBlack : stoneWhite), GameResult::RESIGN);
    setResult(g);
}

void qGoBoardLocalInterface::slot_passComputer()
{
    doPass();
    if (tree->getCurrent()->parent->isPassMove())
        enterScoreMode();
    else
        checkComputersTurn();
}

/*
 * sends a "pass" move to GTP
 */
void qGoBoardLocalInterface::sendPassToInterface(StoneColor c)
{
    doPass();

    if (gtp)
    {
    // if simple pass, tell computer and move on
    switch (c)
    {
        case stoneWhite :
        if (gtp->playwhitePass())
        {
            QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to pass within program \n") + gtp->getLastMessage());
            return;
        }
        else
            qDebug("comp notified of white pass");
        break;

        case stoneBlack :
        if (gtp->playblackPass())
        {
            QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to pass within program \n") + gtp->getLastMessage());
            return;
        }
        break;

        default :
        ;
    }
    }
    if (tree->getCurrent()->parent->isPassMove())
        enterScoreMode();
    else
        checkComputersTurn();
}

void qGoBoardLocalInterface::feedPositionThroughGtp()
{
    if (!gtp)
        return;
    gtp->clearBoard();

    QStack<Move*> stack;
    Move *m = tree->getCurrent();
    while (m->parent != NULL)
    {
        stack.push(m);
        m = m->parent;
    }
    // Do not push the root move.
    // It only holds the handicap (if any).
    if (tree->getRoot()->isHandicapMove())
    {
        Matrix * m = tree->getRoot()->getMatrix();
        int size = m->getSize();
        QList<Point> handicap_stones;
        for (int x=1; x<=size; x++)
            for (int y=1; y<=size; y++)
            {
                if (m->getStoneAt(x,y) == stoneBlack)
                    handicap_stones.append(Point(x,y));
            }
        gtp->set_free_handicap(handicap_stones);
    }

    while (!stack.isEmpty())
    {
        m = stack.pop();
        if (m->isPassMove())
        {
            if (m->getColor() == stoneBlack)
                gtp->playblackPass();
            else if (m->getColor() == stoneWhite)
                gtp->playwhitePass();
        } else {
            if (m->getColor() == stoneWhite)
                gtp->playwhite(m->getX(), m->getY());
            else if (m->getColor() == stoneBlack)
                gtp->playblack(m->getX(), m->getY());
        }
    }
    // Request a move if the computer is on turn
    checkComputersTurn();
}

void qGoBoardLocalInterface::checkComputersTurn()
{
    bool blackToPlay = getBlackTurn();
    if (blackToPlay && !(boardwindow->getMyColorIsBlack()))
        playComputer();
    else if (!blackToPlay && !(boardwindow->getMyColorIsWhite()))
        playComputer();
}
