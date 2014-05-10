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
#include "boardhandler.h"
#include "gamedata.h"
#include "boardwindow.h"
#include "qgtp.h"

#include <QMessageBox>

qGoBoardLocalInterface::qGoBoardLocalInterface(BoardWindow *bw, Tree * t, GameData *gd)
    : qGoBoard(bw, t, gd)
{
    this->setObjectName("qGoBoardLocalInterface");
    boardwindow->getUi()->board->clearData();

    // If we have handicap, but not from a loaded file, we have to set the handicap move
    if (gameData->handicap && gameData->fileName.isEmpty())
        setHandicap(gameData->handicap);

    QSettings settings;
    // value 1 = no sound, 0 all games, 2 my games
    playSound = (settings.value("SOUND") != 1);

    // Set up computer interface.
    gtp = new QGtp() ;

    connect (gtp, SIGNAL(signal_computerPlayed(bool, int, int)), SLOT(slot_playComputer(bool, int, int)));
    connect (gtp, SIGNAL(computerResigned()), SLOT(slot_resignComputer()));
    connect (gtp, SIGNAL(computerPassed()), SLOT(slot_passComputer()));

    if (gtp->openGtpSession(settings.value("COMPUTER_PATH").toString(),
                gameData->board_size,
                gameData->komi,
                gameData->handicap,
                GNUGO_LEVEL)==FAIL)
    {
        throw QString(QObject::tr("Error opening program: %1")).arg(gtp->getLastMessage());
    }

    tree->setCurrent(tree->findLastMoveInMainBranch());
    boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

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
        gtp->undo(0);
        qGoBoard::slotUndoPressed();
    }
}

/*
 * This sends the move to the computer through GTP
 */
void qGoBoardLocalInterface::sendMoveToInterface(StoneColor c,int x, int y)
{
    boardwindow->getUi()->resignButton->setEnabled(false);

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
void qGoBoardLocalInterface::slot_playComputer(bool ok, int x, int y)
{
    if (!ok)
    {
        QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to have the program play its stone\n") + gtp->getLastMessage());
        return;
    }

    bool b = getBlackTurn();
    if (!doMove(b ? stoneBlack : stoneWhite, x, y))
        QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move (%1, %2) seems to be invalid").arg(x).arg(y));
    boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

    // We can also let computer play against itself.
    checkComputersTurn();

    /* We shouldn't be able to resign during computers turn
     * but this is a little awkward to put it in the comp
     * interface here.  But otherwise there's no real
     * way to check who the current player is since the computer
     * moves are done with a function call rather than based on
     * some check of who's turn it is */
    boardwindow->getUi()->resignButton->setEnabled(true);
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
    boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
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
    boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

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
    //	if (win->blackPlayerType==COMPUTER)
    //		playComputer(c);

        break;

        case stoneBlack :
        if (gtp->playblackPass())
        {
            QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to pass within program \n") + gtp->getLastMessage());
            return;
        }

//		if (win->whitePlayerType==COMPUTER)
//			playComputer(c);
        break;

        default :
        ;
    }
    if (tree->getCurrent()->parent->isPassMove())
        enterScoreMode();
    else
        checkComputersTurn();
}

void qGoBoardLocalInterface::feedPositionThroughGtp()
{
    QStack<Move*> stack;
    Move *m = tree->getCurrent();
    while (m->parent != NULL)
    {
        stack.push(m);
        m = m->parent;
    }
    // Do not push the root move.
    // It only holds the handicap (if any).

    gtp->clearBoard();
    gtp->fixedHandicap(gameData->handicap);
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
