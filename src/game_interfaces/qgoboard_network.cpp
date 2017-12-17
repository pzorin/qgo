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


#include "qgoboard_net.h"
#include "tree.h"
#include "move.h"
#include "../network/boarddispatch.h"
#include "../network/messages.h"
#include "undoprompt.h"
#include "clockdisplay.h"
#include "gamedata.h"
#include "boardwindow.h"
#include "matrix.h"
#include <QMessageBox>
#include "networkconnection.h"
#include "audio.h"

qGoBoardNetworkInterface::qGoBoardNetworkInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoard(bw, t, gd)
{
	game_Id = QString::number(gd->number);
    bw->clearData();
    dispatch = boardwindow->getBoardDispatch();
    connection = dispatch->connection;

    QSettings settings;
    playSound = (settings.value("SOUND_MYGAMES").toBool() || settings.value("SOUND_OTHERGAMES").toBool());
	
	if (gd->handicap)
	{
        setHandicap(gd->handicap);
	}
	dontsend = false;
	boardTimerId = 0;
	controlling_player = QString();
	reviewCurrent = 0;
	// what about review games?  games without timers ??
}

/*
 * This functions adds a move to a game. returns the new Move* if move was valid, 0 if not)
 */
Move *qGoBoardNetworkInterface::doMove(StoneColor c, int x, int y)
{
    bool validMove = (dontCheckValidity || tree->getCurrent()->checkMoveIsValid(c, x, y));
    dontCheckValidity = false;
    if (!validMove)
        return NULL;

    Move *result = tree->getCurrent()->makeMove(c,x,y,true);
    tree->setCurrent(result);
    tree->lastMoveInMainBranch = result;
    setModified(true);


    /* Not a great place for this, but maybe okay: */
    TimeRecord t = boardwindow->getClockDisplay()->getTimeRecord(!getBlackTurn());
    if(t.time != 0 || t.stones_periods != -1)
    {
        result->setTimeinfo(true);
        result->setTimeLeft(t.time);
        result->setOpenMoves(t.stones_periods);
    }

    /* Non trivial here.  We don't want to play a sound as we get all
     * the moves from an observed game.  But there's no clean way
     * to tell when the board has stopped loading, particularly for IGS.
     * so we only play a sound every 250 msecs...
     * Also, maybe it should play even if we aren't looking at last move, yeah not sure on that FIXME */
    if(boardwindow->getGamePhase() == phaseOngoing && playSound)
    {
        clickSound->play();
    }

    return result;
}


void qGoBoardNetworkInterface::sendMoveToInterface(StoneColor c, int x, int y)
{
    if(controlling_player != QString() && controlling_player != dispatch->getUsername())
		return;
	if(boardwindow->getGameData()->nigiriToBeSettled)
	{
		qDebug("Nigiri unsettled");
		//return;
	}
    if(boardwindow->getGamePhase() == phaseScore && !dispatch->canMarkStonesDeadinScore())
			return;
	if(dontsend)
		return;
	// to prevent double clicking and upsetting servers...
	dontsend = true;
	if(boardwindow->getGamePhase() == phaseScore)
	{
		/* A little awkward... FIXME, why is there no getMyColor()
		 * function ?!?! */
		//also "c" refers most likely to player color, not the stone clicked on
		if(((tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneBlack && boardwindow->getMyColorIsWhite()) ||
		   (tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneWhite && boardwindow->getMyColorIsBlack()))
           && connection->cantMarkOppStonesDead())
		{
			dontsend = false;	//ready to send again
			return;
		}
        MoveRecord * m = new MoveRecord();
        if(tree->getCurrent()->getMatrix()->isStoneDead(x, y))
        {
            if(connection->unmarkUnmarksAllDeadStones())
            {
                if (QMessageBox::question(boardwindow,tr("Unmark All?"),
                                          QString(tr("Unmark all your dead stones?\n")))
                        == QMessageBox::No)
                {
                    dontsend = false;	//ready to send again
                    delete m;
                    return;
                }
            }
            m->flags = MoveRecord::UNREMOVE;
		}
		else
			m->flags = MoveRecord::REMOVE;
		m->x = x;
		m->y = y;
		//move number shouldn't matter
		m->color = c;
        dispatch->sendMove(m);
		delete m;
		return;
	}
	else
	{
		/* Check validity of move before sending */
        if(tree->getCurrent()->checkMoveIsValid(c, x, y))
            dontCheckValidity = true;
        else
        {
			QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("Move %1 %2 is invalid").arg(QString::number(x), QString::number(y)));
			dontsend = false;	//okay to send again
			return;
		}
		/* Rerack time before sending our move */
		boardwindow->getClockDisplay()->makeMove(getBlackTurn());
        dispatch->sendMove(new MoveRecord(
			tree->getCurrent()->getMoveNumber(), x, y, c));
	}
}

void qGoBoardNetworkInterface::handleMove(MoveRecord * m)
{
    Move * const remember = tree->getCurrent();
    Move * last = tree->findLastMoveInMainBranch();
    // This handles territory marks provided by the server.
    /* Separate handling is needed because we want to
     * avoid GUI updates until the full information is available.
     * The remaining part of the function should in principle also be rewritten in such a way that
     * GUI updates are only made when needed. */
    if (m->flags == MoveRecord::TERRITORY)
    {
        last->getMatrix()->insertMark(m->x,m->y,m->color == stoneBlack ? markTerrBlack : markTerrWhite);
        return;
    } else if (m->flags == MoveRecord::DONE_SCORING)
    {
        tree->countMarked(last);
        boardwindow->updateMove(tree->getCurrent()); // Updates GUI
        return;
    }

    int move_number, move_counter;
	Move * goto_move;
	//static bool offset_1 = false;
	int i;
	
	dontsend = false;		//clear the dontsend bit
	
	/* In case we join in score phase */
	if(m->flags == MoveRecord::NONE && boardwindow->getGamePhase() == phaseScore)
		m->flags = MoveRecord::REMOVE;
	
	if(boardwindow->getGameData()->gameMode == modeReview)
	{
		if(!reviewCurrent)
            reviewCurrent = last;
    }
	tree->setCurrent(last);

	move_number = m->number;
	//bool hcp_move = tree->getCurrent()->isHandicapMove();
	move_counter = tree->getCurrent()->getMoveNumber();
	if(move_number == NOMOVENUMBER)	//not all services number
		move_number = move_counter + 1;		//since we add one to move counter later
	
	//qDebug("MN: %d MC: %d", move_number, move_counter);
	
    move_counter++;
		
	switch(m->flags)
    {
		case MoveRecord::UNDO_TERRITORY:
		{
            int boardsize = last->getMatrix()->getSize();
            for(int i = 1; i <= boardsize; i++)
			{
                for(int j = 1; j <= boardsize; j++)
				{
                    if(last->getMatrix()->isStoneDead(i, j))
                        markLiveStone(i, j);
				}
			}
			break;
		}
		case MoveRecord::REQUESTUNDO:
		{
			qDebug("Got undo message in network interface!!\n");
            //move_number = mv_counter - 1;
			QString opp = dispatch->getOpponentName();
			UndoPrompt * up = new UndoPrompt(&opp,
                             connection->supportsMultipleUndo(),
							 m->number);
			
			int up_return = up->exec();
			if (up_return != -1)
			{
				dispatch->sendMove(new MoveRecord(m->number, MoveRecord::UNDO));
			}
			else
			{
				dispatch->sendMove(new MoveRecord(m->number, MoveRecord::REFUSEUNDO));
			}
		}
			break;
		case MoveRecord::UNDO:
			{
			/* Are we supposed to make a brother node or something ??? FIXME 
			* qgoboard.cpp also does this.*/
            if(connection->supportsMultipleUndo())
			{
				while(move_counter > move_number + 1)	//move_numbe r+ 1 if our turn?
				{
                    tree->deleteNode();
                    move_counter--;
				}
			}
			else
			{
                tree->deleteNode();
				if ((getBlackTurn() && m->color == stoneBlack) ||
					((!getBlackTurn()) && m->color == stoneWhite))
                {
                    tree->deleteNode();
				}
			}
			/* I've turned off multiple undo for tygem, just for now... 
			 * since NOMOVENUMBER FIXME, actually I'm not sure if tygem
			 * has a normal multiple undo, though it might for review games */
			qDebug("Undoing move %d = %d - 1", move_number, move_counter);
			/* FIXME This can get screwy especially around the scoreMode
			 * stuff.... apparently we can only undo our own passes
			 * unlike other WING moves and it only takes 2 undos, not
			 * 3... as if the first undo just gets you out of the 
			 * score place. */
			if(boardwindow->getGamePhase() == phaseScore)
			{
				/* Not sure if this is always true, but it appears
				 * that the last pass doesn't count as a move or
				 * something, meaning that we should delete twice
				 * for it. This is going to be a problem later
				 * FIXME FIXME FIXME*/
				/* IGS does not leave score mode, ever !!!
				 * nor should anything else do to what scoremode
				 * does. Simply clears some of the dead marks*/
				/* Actually, I'm not sure, its possible that the
				 * glGo client we've been using to test this
				 * doesn't handle undo from score very well.  I've
				 * seen some bugs in it before. */
                tree->deleteNode();
				leaveScoreMode();
			}
			}
			break;
		case MoveRecord::PASS:
			/* FIXME what about a kibitz here saying opponent name
			 * has passed.  I think IGS needs it, not sure about
			 * other protocols which may have it, but if they do,
			 * it should be moved here.  Otherwise easy to miss. */
			/* If there's three passes, we should stop the clock,
			 * although some servers might have two pass models
			 * and we need a flag for that */
			//if(!boardwindow->getMyColorIsBlack())	//if we're white
            if(!connection->netWillEnterScoreMode())	//ugly for oro FIXME
			{
                if(connection->twoPassesEndsGame())
				{
                    if(last->isPassMove())
						enterScoreMode();
				}
				else
				{
                    if(last->parent &&
                    last->isPassMove() &&
                    last->parent->isPassMove())
						enterScoreMode();
						//boardwindow->setGamePhase ( phaseScore );	//okay?	
				}
			}
			//FIXME potentially white/black here should come from color from network which is unreliable now
            kibitzReceived(QString(getBlackTurn() ? "Black" : "White") + " passes.");
			doPass();
			break;
		case MoveRecord::HANDICAP:
            setHandicap(m->x);
            break;
		case MoveRecord::REMOVE:
			//qDebug("md!! toggling life of %d %d", m->x, m->y);
            markDeadStone(m->x, m->y);
			//tree->getCurrent()->getMatrix()->toggleGroupAt(m->x, m->y);
			//boardwindow->qgoboard->kibitzReceived("removing @ " + pt);
			break;
		case MoveRecord::REMOVE_AREA:
			//FIXME
            markDeadArea(m->x, m->y);
			break;
		case MoveRecord::UNREMOVE_AREA:
			//FIXME
            if(connection->unmarkUnmarksAllDeadStones())
			{
				/* Not sure where we get the dead groups from, FIXME 
				 * okay, really, we should have a list of dead groups
				 * for each player that can be checked on here.
				 * The thing is, ORO also has such a list that it tracks
				 * and I don't want duplication of that code but right
				 * now I have other things on my mind and I just want to
				 * get this done, so I'm going to do something really
				 * quick, dirty, and awkward here and I'll fix it in
				 * a later version... ignoring the stitch in time
				 * thing.  So cut this out soon.  Note also that it
				 * might make sense to have an evaluation function
				 * in the board code, to do something to every
				 * stone of a type... maybe not.*/
                int boardsize = last->getMatrix()->getSize();
                for(int i = 1; i <= boardsize; i++)
				{
                    for(int j = 1; j <= boardsize; j++)
					{
                        if(last->getMatrix()->isStoneDead(i, j))
						{
                            if(last->getMatrix()->getStoneAt(i, j) == m->color)
							{
                                markLiveArea(i, j);
							}
						}
					}
				}
				
			}
			else
                markLiveArea(m->x, m->y);
			break;
        case MoveRecord::REFUSEUNDO:
			//handled by protocol as a recvKibitz for whatever reason
			break;
		case MoveRecord::FORWARD:
            if(!dispatch->getReviewInVariation() && last->isInMainBranch())
			{
				/* In case it was in a variation previously to remove the marker.
				 * this should be elsewhere like on the setReviewInVariation(); FIXME */
                last->marker = NULL;
			}
			for(i = 0; i < move_number; i++)
			{
                /*if(dispatch->getReviewInVariation() && tree->isInMainBranch(tree->getCurrent()))
				{
					boardwindow->getBoardHandler()->gotoMove(inVariationBranch);
				}
				else*/
                    tree->slotNavForward();
			}
			break;
		case MoveRecord::BACKWARD:
			for(i = 0; i < move_number; i++)
			{
                if(dispatch->getReviewInVariation() && last && last->isInMainBranch())
					break;
                tree->slotNavBackward();
			}
			break;
		case MoveRecord::RESETBRANCH:
		case MoveRecord::DELETEBRANCH:
            goto_move = last;
			if(goto_move->getMoveNumber() > lastMoveInGame->getMoveNumber())
				goto_move = lastMoveInGame;
			else
			{
                while(!(goto_move->isInMainBranch()))
					goto_move = goto_move->parent;
			}
            tree->setCurrent(goto_move);
			if(m->flags == MoveRecord::DELETEBRANCH) {}
				
			break;
		case MoveRecord::RESETGAME:
			goto_move = tree->getRoot();
            tree->setCurrent(goto_move);
			break;
		case MoveRecord::TOEND:
            if(dispatch->getReviewInVariation())
				goto_move = tree->findLastMoveInCurrentBranch();
			else
				goto_move = lastMoveInGame;
            tree->setCurrent(goto_move);
			break;
		case MoveRecord::SETMOVE:
			/* Might later want to record this
			 * also we should have already set the game to the last move and disabled nav*/
			goto_move = tree->findNode(tree->getRoot(), m->number);

			if (goto_move)
                tree->setCurrent(goto_move);
			else
			{
				QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("Cannot set move to move number %1").arg(m->number));
			}
			break;
		default:
		case MoveRecord::NONE:
			
			if(move_number == move_counter)
			{
				/* FIXME Can we guess at the color here ? */
				if(m->color == stoneNone)
					m->color = (getBlackTurn() ? stoneBlack : stoneWhite);
                if (doMove(m->color, m->x, m->y) == NULL)
					QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move %1 %2 seems to be invalid").arg(QString::number(m->x), QString::number(m->y)));
				else if(m->color == stoneWhite && !boardTimerId)  //awkward ?  FIXME for always move 1?
				{
					onFirstMove();
				}
			}
			else if(move_number < move_counter)
			{
				/* FIXME, this prevents the next if statement
				* partly, this whole thing is screwy */
				/* IGS, certain games have this remove a legit first
				 * move */
				qDebug("Repeat move after undo?? %d %d", move_number, move_counter);
			}
			/* This is for resetting the stones for canadian
			 * timesystems, its awkward for it to be here...  but
			 * I guess I'm getting lazy. FIXME */
			/* I don't think this matters for ORO because time
			 * comes in right after move.  It might matter for IGS */
			/* For stones, this should be called before the move is sent or here
			 * but with getBlackTurn() negated.  Otherwise, its like our move
			 * gets decremented possibly when they play.  It looks weird... double
			 * check with other time style though. */
			boardwindow->getClockDisplay()->rerackTime(getBlackTurn());
			break;
	}
	
	if(boardwindow->getGameData()->gameMode == modeReview)	//for now review mode means no navigation, see interfacehandler comment
		reviewCurrent = tree->getCurrent();
	else
	{
		if (remember != last)
		{
            tree->setCurrent(remember);
		}
    }
}

void qGoBoardNetworkInterface::passRequest()
{
	/* doPass is called when we receive the move from the server, 
	 * as with moves */
	/* FIXME, we need to make sure the move is valid before we send it!!!!!
	 * but without playing it since we get that from server. */
    if((getBlackTurn() && !(boardwindow->getMyColorIsBlack())) ||
       (!getBlackTurn() && !(boardwindow->getMyColorIsWhite())))
    {
        return; // It is not our move
    }

	/* Rerack time before sending our move 
	 * Is this okay here?  Do we need to rerack for passes or what ? FIXME*/
	boardwindow->getClockDisplay()->makeMove(getBlackTurn());
    dispatch->sendMove(new MoveRecord(tree->getCurrent()->getMoveNumber(), MoveRecord::PASS));
}

void qGoBoardNetworkInterface::slotUndoPressed()
{
	if(boardwindow->getGamePhase() == phaseScore)
	{
        if(connection->supportsRequestMatchMode())
        {
            if (QMessageBox::question(0,tr("Return to game?"),
                                      QString(tr("Ask opponent to return to game?\n")))
                    == QMessageBox::Yes)
                dispatch->sendRequestMatchMode();
            return;
		}
        else if(dispatch->undoResetsScore())
		{
            dispatch->sendMove(new MoveRecord(-1, MoveRecord::UNDO));
			return;
		}
	}
	int moves = tree->getCurrent()->getMoveNumber();
	//if its our turn - 2?
	if ((getBlackTurn() && boardwindow->getMyColorIsBlack()) ||
		    ((!getBlackTurn()) && boardwindow->getMyColorIsWhite()))
		moves -= 2;
	else
		moves--;
	// might want to prompt anyway FIXME ?
    if(connection->supportsMultipleUndo())
	{
		UndoPrompt * up = new UndoPrompt(0, true, moves);
		int up_return = up->exec();
		if(up_return == -1)
			return;
		else
			moves = up_return;
	}
	
    dispatch->sendMove(new MoveRecord(moves, MoveRecord::REQUESTUNDO));
}
/* Note that these all crash if we disconnect from server first, but server should handle that anyway FIXME */

/* Really the ui button disables should be on some gamePhase code FIXME */
void qGoBoardNetworkInterface::slotDonePressed()
{
    dispatch->sendMove(new MoveRecord(MoveRecord::DONE_SCORING));
    boardwindow->setDoneEnabled(false);		//FIXME okay? don't want to send done twice
}

void qGoBoardNetworkInterface::slotResignPressed()
{
    if(dispatch->getOpponentName() == QString())
	{
		boardwindow->getGameData()->fullresult = new GameResult();		//temporary for bugs FIXME
		return;
    }
    if (QMessageBox::question(boardwindow,tr("Resign?"),
                              QString(tr("Resign game with %1\n")).arg(dispatch->getOpponentName()))
            == QMessageBox::Yes)
	{
        dispatch->sendMove(new MoveRecord(tree->getCurrent()->getMoveNumber(), MoveRecord::RESIGN));
        boardwindow->setResignEnabled(false);		//FIXME okay? don't want to send resign twice
	}
}

void qGoBoardNetworkInterface::adjournGame(void)
{
    QString opp_name = dispatch->getOpponentName();
	boardwindow->setGamePhase(phaseEnded);
	qDebug("qgBNI::adjournGame");
	if(opp_name == QString())
	{
        GameData * r = dispatch->getGameData();
		if(!r)
			qDebug("No game record on adjourned game");
		else
			QMessageBox::information(boardwindow , tr("Game Adjourned"), tr("%1 vs. %2 has been adjourned.").arg(r->white_name).arg(r->black_name));

	}
	else
		QMessageBox::information(boardwindow , tr("Game Adjourned"), tr("Game with %1 has been adjourned.").arg(opp_name));
	boardwindow->getGameData()->fullresult = new GameResult(stoneNone, GameResult::ADJOURNED);
    boardwindow->setAdjournEnabled(false);		//FIXME okay? don't want to send adjourn after adjourn
}

/* Might look nicer if we just set the game phase to ended or
 * something, I'll though we might just have lost connection so... */
void qGoBoardNetworkInterface::stopTime(void)
{
	if(boardTimerId)
		killTimer(boardTimerId);
}
