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
#include "audio.h"

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
    gtp = new QGtp();
    currentEngine = NULL;

    connect (gtp, SIGNAL(signal_computerPlayed(int, int)), SLOT(slot_playComputer(int, int)));
    connect (gtp, SIGNAL(computerResigned()), SLOT(slot_resignComputer()));
    connect (gtp, SIGNAL(computerPassed()), SLOT(slot_passComputer()));


    settings.beginReadArray("ENGINES");
    settings.setArrayIndex(settings.value("DEFAULT_ENGINE").toInt());
    QString path = settings.value("path").toString();
    QString args = settings.value("args").toString();
    settings.endArray();
    if (gtp->openGtpSession(path,args,
                gameData->board_size,
                gameData->komi,
                gameData->handicap)==FAIL)
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

    insertStoneFlag = false;
    feedPositionThroughGtp();
}

qGoBoardLocalInterface::~qGoBoardLocalInterface()
{
    qDebug("Deconstructing computer interface");
    delete gtp;
}

void qGoBoardLocalInterface::localMoveRequest(StoneColor c, int x, int y)
{
    if (insertStoneFlag)
    {
        tree->getCurrent()->getMatrix()->insertStone(x,y,c,true);
        return;
    }
    bool engineUpToDate = (tree->getCurrent() == currentEngine);
    Move *result = tree->getCurrent()->hasSon(c,x,y);
    if (!result)
        result = tree->getCurrent()->makeMove(c,x,y);
    if (!result)
        return;

    tree->setCurrent(result);
    setModified(true);
    boardwindow->updateMove(tree->getCurrent());

    /* Non trivial here.  We don't want to play a sound as we get all
     * the moves from an observed game.  But there's no clean way
     * to tell when the board has stopped loading, particularly for IGS.
     * so we only play a sound every 250 msecs...
     * Also, maybe it should play even if we aren't looking at last move, yeah not sure on that FIXME */
    if(boardwindow->getGamePhase() == phaseOngoing && QTime::currentTime() > lastSound)
    {
        if (playSound)
            clickSound->play();
        lastSound = QTime::currentTime();
        lastSound = lastSound.addMSecs(250);
    }

    if (engineUpToDate)
    {
        currentEngine = result;
        sendMoveToInterface(c,x,y);
    }
    else
        feedPositionThroughGtp();

    return;
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
 * This slot is triggered by the signal emitted by 'gtp' when getting a move
 */
void qGoBoardLocalInterface::slot_playComputer(int x, int y)
{
    bool atCurrent = (tree->getCurrent() == currentEngine);
    Move *result = currentEngine->makeMove(currentEngine->whoIsOnTurn(), x, y);
    if (!result)
    {
        QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move (%1, %2) seems to be invalid").arg(x).arg(y));
        return;
    }
    currentEngine = result;

    if (atCurrent)
        tree->setCurrent(result);
    checkComputersTurn();
}

void qGoBoardLocalInterface::slot_resignComputer()
{
    // This hack should be removed by making doMove() and related functions relative to a move
    Move *remember = tree->getCurrent();
    bool atCurrent = (remember == currentEngine);
    tree->setCurrent(currentEngine);

    bool b = getBlackTurn();
    GameResult g((!b ? stoneBlack : stoneWhite), GameResult::RESIGN);
    setResult(g);
    currentEngine = NULL;

    if (!atCurrent)
        tree->setCurrent(remember);
}

void qGoBoardLocalInterface::slot_passComputer()
{
    bool atCurrent = (tree->getCurrent() == currentEngine);
    Move *result = currentEngine->makePass();
    if (!result)
        return;
    currentEngine = result;

    if (result->parent->isPassMove())
    {
        tree->setCurrent(result);
        enterScoreMode();
    } else {
        if (atCurrent)
            tree->setCurrent(result);
        checkComputersTurn();
    }
}

/*
 * sends a "pass" move to GTP
 */
void qGoBoardLocalInterface::sendPassToInterface(StoneColor c)
{
    doPass(c);

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
    currentEngine = m;
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

void qGoBoardLocalInterface::checkComputersTurn(bool force)
{
    if ((!gtp) || (gtp->isBusy()) || (!currentEngine))
        return;

    StoneColor toPlay = currentEngine->whoIsOnTurn();
    int result = OK;
    if ((toPlay == stoneBlack) && (force || !(boardwindow->getMyColorIsBlack())))
        result= gtp->genmoveBlack();
    else if ((toPlay == stoneWhite) && (force || !(boardwindow->getMyColorIsWhite())))
        result = gtp->genmoveWhite();

    if (result == FAIL)
        QMessageBox::warning(boardwindow, PACKAGE, tr("Move request from engine failed\n") + gtp->getLastMessage());
}

void qGoBoardLocalInterface::slotToggleInsertStones(bool val)
{
    insertStoneFlag = val;
}
