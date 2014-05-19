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

#include "boarddispatch.h"
#include "boardwindow.h"
#include "messages.h"
#include "listviews.h"
#include "../game_interfaces/resultdialog.h"
#include "../game_interfaces/countdialog.h"
#include "gamedata.h"
#include "playergamelistings.h"
#include "networkconnection.h"
#include "qgoboard.h"
#include "clockdisplay.h"
#include "tree.h"

/* It would be difficult to create a board without a connection,
 * but we need to either be certain and not test at all, or assign
 * it at creation.  I'm just worried about the client code knowing
 * too much about the connection.*/
/* Also the !boardwindow things?  I mean its pretty damn hard
 * to get these messages without them, we just just need to be sure */

BoardDispatch::BoardDispatch(NetworkConnection * conn, GameListing * l)
{
	connection = conn;
	boardwindow = 0;
	resultdialog = 0;
	countdialog = 0;
	observerListModel = 0;
	
	clockStopped = false;
	reviewInVariation = false;
	/* New rules.  The boarddispatch creates the game data or
	 * whatever loads the game data before passing it to the
	 * boardwindow creates it.  The boardwindow deletes it.
	 * there's only one gameData per board */
	if(l && l->gameData)
		gameData = new GameData(l->gameData);		//ORO uses this
	else
		gameData = new GameData();
	gameData->place = connection->getPlaceString();
	//maybe we could set number here?  if we have it?
	if(strlen(connection->getCodecString()) != 0)
		gameData->codec = "UTF-8";		//all files in unicode where necessary

	/* Not sure we can do this. GameListings have to have a
	 * key.  But to go without a gameListing in the BoardDispatch?*/
	if(l)
        gameListing = l;
	else
        qDebug("BoardDispatch::BoardDispatch() : NULL GameListing * passed!");
}

BoardDispatch::~BoardDispatch()
{
	qDebug("Destroying board dispatch\n");
	/* Clear dispatch so boardwindow doesn't try to close it */
	if(boardwindow)
	{
		//clearObservers();
        boardwindow->setObserverModel(NULL);		//crashhere maybe
		boardwindow->setGamePhase(phaseEnded);		//disables all buttons
		if(observerListModel)
			delete observerListModel;
		if(boardwindow->getGamePhase() != phaseEnded)
		{
			//got an oro crash here after return to match/rematch
			//confusion, qgoboard was 0 FIXME
			//and then after closing the window, apparently
			//board dispatch was NOT cleared and got called after
			//here on disconnect
			boardwindow->qgoboard->stopTime();
			boardwindow->qgoboard->kibitzReceived("Disconnected");
		}
		boardwindow->setBoardDispatch(0);
	}
	if(resultdialog)
		delete resultdialog;
	if(countdialog)
		delete countdialog;
}

bool BoardDispatch::canClose(void)
{
	if(!gameData)
		return true;
	if(gameData->gameMode == modeMatch && gameData->fullresult == 0)
	{
		boardwindow->qgoboard->slotResignPressed();
		/* fullresult won't get set until we receive the server acknowledgement, so
		 * for now, we'll just stop the process each time.  Its likely player is only
		 * playing one game so this is more of a warning */
		return false;
	}
	return true;
}

/* FIXME, we need to make better use of this,
 * or not at all */
void BoardDispatch::closeBoard(void)
{
	if(connection)
	{
		switch(boardwindow->getGameMode())
		{
			case modeObserve:
				if(boardwindow->getGamePhase() != phaseEnded)
                    connection->stopObserving(gameListing); //FIXME
				break;
			case modeReview:
                connection->stopReviewing(gameListing);
				break;
			case modeMatch:
				/* FIXME if game is over, we don't need to adjourn
				 * nor perhaps with the above modes either
				 * so we should check this out... maybe this
				 * function shouldn't be being called,
				 * at least not from here.  Basically,
				 * responsibilities of board dispatch and
				 * qgoboard or boardwindow or whatever are
				 * overlapping here.  Something needs to go */
				/* This shouldn't be here anymore since we do the canClose which
				 * prompts for resign.  Or its an IGS only feature FIXME */
				if(boardwindow->getGamePhase() != phaseEnded)
                    connection->adjournGame(gameListing);	 //FIXME
				if(resultdialog)
				{
					delete resultdialog;
					resultdialog = 0;
				}
				break;
			default:
				qDebug("Unknown game Mode, board dispatch does nothing\n");
				break;
		}
		if(gameData)
		{
			qDebug("Closing board dispatch\n");
			connection->closeBoardDispatch(gameData->number);
			gameData = 0;
		}
	}
	//lets have closeBoardDispatch do this
	//delete this;
}

void BoardDispatch::setConnection(NetworkConnection *conn)
{
    connection = conn;
    /* This function is currently called if connection
     *is closed when a board using that connection is open.
     *Probably more has to be done here. */
}

void BoardDispatch::moveControl(QString & player)
{
	boardwindow->qgoboard->moveControl(player);
}

void BoardDispatch::recvMove(MoveRecord * m)
{
	if(!boardwindow)
	{
		qDebug("Board dispatch has no board window\n");
		return;
	}
	if(m->flags == MoveRecord::SETMOVE ||
		m->flags == MoveRecord::BACKWARD ||
		m->flags == MoveRecord::FORWARD ||
		m->flags == MoveRecord::RESETBRANCH ||
		m->flags == MoveRecord::RESETGAME)
	{
		gameData->gameMode = modeReview;
	}
	boardwindow->qgoboard->handleMove(m);
}

//FIXME should be another way to set the game moves
//related directly to the board... then again, I don't
//really want to access the tree from here
void BoardDispatch::sendMove(MoveRecord * m)
{
	if(m->number && m->flags == MoveRecord::NONE)	//i.e., a move
		gameData->moves = m->number;
	//qDebug("setting game moves: %d", gameData->moves);
	if(connection)
		connection->sendMove(gameData->number, m);
}

/* FIXME
 * note that after a request count is sent in tygem,
 * NO other button can be hit until the response occurs,
 * you can't even resign.
 * Its also likely that you can only request one count
 * per turn.
 * But so this suggests maybe a lock buttons button.
 * I don't like the placement of the request count
 * button right now either.  Also, done button means
 * nothing.  If it closes the window, that's one thing
 * but otherwise, why have two buttons.  done is for
 * score, should be disabled afterwards.
 * Also, closing windows is generally not allowed.  It
 * should always force a resign or prompt or maybe ask
 * for an adjourn */
void BoardDispatch::sendRequestCount(void)
{
	if(connection)
	{
		stopTime();	//protocol specific or not?		FIXME, huge issue if protocol does this to!!! see cyberoro killActiveMatchTimers, figure out who should do this when
        boardwindow->setCountEnabled(false);
		connection->sendRequestCount(gameData->number);
	}
}

void BoardDispatch::sendRequestDraw(void)
{
	if(connection)
	{
		stopTime();	//protocol specific or not?
        boardwindow->setDrawEnabled(false);
		connection->sendRequestDraw(gameData->number);
	}
}

void BoardDispatch::sendRequestMatchMode(void)
{
	if(connection)
	{
		stopTime();		//maybe??
        boardwindow->setUndoEnabled(false);
		connection->sendRequestMatchMode(gameData->number);
	}
}

void BoardDispatch::sendTimeLoss(void)
{
	static bool already_sent = false;
	if(connection && !already_sent)
		connection->sendTimeLoss(gameData->number);
	already_sent = true;
}

void BoardDispatch::gameDataChanged(void)
{
	if(!boardwindow)	//this really shouldn't happen
		return;
	boardwindow->gameDataChanged();
}

void BoardDispatch::openBoard(void)
{
    if(boardwindow)
    {
        qDebug("BoardDispatch::openBoard() : window already open\n");
        return;
    }
    if(!connection)
    {
        qDebug("BoardDispatch::openBoard() : connection not set\n");
        return;
    }
		mergeListingIntoRecord(gameData, gameListing);
		
		bool imWhite = false;
		bool imBlack = false;
		QString myName = connection->getUsername();
		if(gameData->gameMode == modeUndefined)
		{
			
			imWhite = (gameData->white_name == myName);
			imBlack = (gameData->black_name == myName);		
			if ( imWhite && imBlack )
				gameData->gameMode = modeTeach;
			else if ( imWhite || imBlack)
				gameData->gameMode = modeMatch;
			else
				gameData->gameMode = modeObserve;
		}
		//else, something else has set it ahead of time
		
		boardwindow = new BoardWindow(gameData, imBlack, imWhite, this);
		if(!observerListModel)
		{
			observerListModel = new ObserverListModel();
			observerListModel->setAccountName(myName);
		}
        boardwindow->setObserverModel(observerListModel);
		// do we need the below?
		//boardwindow->qgoboard->set_statedMoveCount(gameData->moves);
		boardwindow->gameDataChanged();	//necessary at least for cursor
		if(gameData->record_sgf != QString())	//for ORO
            boardwindow->getTree()->slotNavLast();
}

void BoardDispatch::recvTime(const TimeRecord & wt, const TimeRecord & bt)
{
	/* I don't think I care if its not ongoing.  That's a minor issue
	 * if we recvTime, we should set it */
	if(!boardwindow /*|| boardwindow->getGamePhase() != phaseOngoing*/)
		return;

	boardwindow->getClockDisplay()->setTimeInfo(bt.time,
						bt.stones_periods,
						wt.time,
						wt.stones_periods);
}

void BoardDispatch::recvAddTime(int minutes, QString player_name)
{
	if(gameData->white_name == player_name)
	{
		TimeRecord t = boardwindow->getClockDisplay()->getTimeRecord(false);
		boardwindow->getClockDisplay()->setTimeInfo(0, -1,
						t.time + (60 * minutes),
						t.stones_periods);
	}
	else if(gameData->black_name == player_name)
	{
		TimeRecord t = boardwindow->getClockDisplay()->getTimeRecord(true);
			boardwindow->getClockDisplay()->setTimeInfo(t.time + (60 * minutes),
						t.stones_periods, 0, -1);
	}
	else
		qDebug("Received add time for unknown player name %s", player_name.toLatin1().constData());
}

void BoardDispatch::recvResult(GameResult * r)
{
	/* FIXME there's still some issues here with this being called from certain services
	 * at the wrong time, etc. */
	if(!boardwindow)
		return;
	if(!r)
	{
        GameResult res = boardwindow->getTree()->retrieveScore();
		r = &res;
	}
	// also set on GameData of boardwindow ???
	if(!gameData)
	{
		qDebug("No game record to check with result");
		return;	
	}
	/* IGS passes sparse entries... bit ugly but: */
	if(r->winner_name == QString() && r->winner_color == stoneNone && r->result != GameResult::NOGAME)
	{
		if(r->loser_name == gameData->white_name)
		{
			r->winner_name = gameData->black_name;
			r->winner_color = stoneBlack;
		}
		else
		{
			r->winner_name = gameData->white_name;
			r->winner_color = stoneWhite;
		}
	}
	boardwindow->qgoboard->setResult(*r);
	
	//saveRecordToGameData();
	//testing
	//connection->sendRematchRequest(gameData->number);
}

/* Why doesn't this appear to save comments that we've seen in ORO games when rejoined?!? FIXME */
void BoardDispatch::saveRecordToGameData(void)
{
	if(gameData->record_sgf == QString())
	{
		boardwindow->saveRecordToGameData();		//for ORO
		connection->saveIfDoesntSave(gameData);		//for ORO
	}
}

void BoardDispatch::sendResult(GameResult * r)
{
	connection->sendResult(gameData, r);	
}

void BoardDispatch::recvObserver(PlayerListing * p, bool present)
{
	if(!boardwindow)
		return;
	if(!gameData)
	{
		qDebug("received observer but no game data!!");
		return;
	}
	std::vector<unsigned short>::iterator i = std::find(p->room_list.begin(), p->room_list.end(), (unsigned short)gameData->number);
	if(present)
	{
		if(p->room_list.end() == i)
			p->room_list.push_back(gameData->number);
		observerListModel->insertListing(p);
	}
	else if(i != p->room_list.end())
	{
		p->room_list.erase(i);
		observerListModel->removeListing(p);
	}
}

void BoardDispatch::clearObservers(void)
{
	if(!boardwindow)
		return;
	observerListModel->clearList();
}

void BoardDispatch::recvKibitz(QString name, QString text)
{
	if(!boardwindow)
		return;
	if(name == QString())
		boardwindow->qgoboard->kibitzReceived(text);
	else
		boardwindow->qgoboard->kibitzReceived(name + ":" + text);
}

void BoardDispatch::sendKibitz(QString text)
{
	if(connection)
		connection->sendMsg(gameData->number, text);	
}

/* FIXME: is this legitimate?  doesn't qgoboard_network handle
 * entering score mode within handleMove?  why would we do it here?
 * find out who calls this and why.  Could be for special cases
 * but... Okay... looks like everyone calls it... not sure
 * what to say about that... might be redundant with set_move
 * or perhaps we've been letting server tell us when to enter
 * score mode and maybe set_move code should go*/
void BoardDispatch::recvEnterScoreMode(void)
{
	if(!boardwindow)
		return;
	boardwindow->qgoboard->enterScoreMode();
}

void BoardDispatch::recvLeaveScoreMode(void)
{
	if(!boardwindow)
		return;
	startTime();		//protocol specific or not?
	boardwindow->qgoboard->leaveScoreMode();
}

void BoardDispatch::recvRequestCount(void)
{
	//this is those count messages before passes
	//FIXME
	//we need to copy the matchinvite file to a
	//RequestCountDialog, etc.
	if(boardwindow)
		boardwindow->qgoboard->requestCountDialog();
}

void BoardDispatch::recvAcceptCountRequest(void)
{
	recvEnterScoreMode();
	//is this all?
}
		
void BoardDispatch::recvRejectCountRequest(void)
{
	//so we can nag them endlessly:
	//no, seriously, FIXME server protocol may have some limit
	//on this
    boardwindow->setCountEnabled(true);
	if(boardwindow)
		boardwindow->qgoboard->recvRefuseCount();
}

void BoardDispatch::recvRequestDraw(void)
{
	if(boardwindow)
		boardwindow->qgoboard->requestDrawDialog();
}

void BoardDispatch::recvAcceptDrawRequest(void)
{
	//FIXME
	//maybe we just draw?
}
		
void BoardDispatch::recvRefuseDrawRequest(void)
{
	//FIXME
	startTime();	//protocol specific or not
	if(boardwindow)
		boardwindow->qgoboard->recvRefuseDraw();
}

void BoardDispatch::recvRequestMatchMode(void)
{
	if(!boardwindow)
		return;
	boardwindow->qgoboard->requestMatchModeDialog();
}

void BoardDispatch::sendAcceptMatchModeRequest(void)
{
	connection->sendAcceptRequestMatchMode(gameData->number);
}

void BoardDispatch::sendRefuseMatchModeRequest(void)
{
	startTime();		//protocol specific or not?
	connection->sendDeclineRequestMatchMode(gameData->number);
}

void BoardDispatch::recvRejectMatchModeRequest(void)
{
    boardwindow->setUndoEnabled(true);
	if(boardwindow)
		boardwindow->qgoboard->recvRefuseMatchMode();
}

void BoardDispatch::createCountDialog(void)
{
	// pretty sure we need to get the game result from the boardwindow
	// without triggering an end game result dialog or something like
	// that FIXME
	countdialog = new CountDialog(boardwindow, this, gameData->number);	
}

void BoardDispatch::recvRejectCount(void)
{
	if(countdialog)
		countdialog->recvRejectCount();	
}

void BoardDispatch::recvAcceptCount(void)
{
	if(countdialog)
		countdialog->recvAcceptCount();	
}

void BoardDispatch::sendRejectCount(void)
{
	connection->sendRejectCount(gameData);
}

void BoardDispatch::sendAcceptCount(void)
{
	connection->sendAcceptCount(gameData);	
}

void BoardDispatch::recvRequestAdjourn(void)
{
	if(boardwindow)
		boardwindow->qgoboard->requestAdjournDialog();
}

void BoardDispatch::sendAdjournRequest(void)
{
	connection->sendAdjournRequest();
}

/* We either don't need this or it might overlap with some other message
 * like an end game message */
void BoardDispatch::sendAdjourn(void)
{
	connection->sendAdjourn();
}

void BoardDispatch::sendRefuseAdjourn(void)
{
	connection->sendRefuseAdjourn();
}

void BoardDispatch::sendAcceptCountRequest(void)
{
	connection->sendAcceptCountRequest(gameData);
}

void BoardDispatch::sendRefuseCountRequest(void)
{
	startTime();		//protocol specific or not?
	connection->sendRefuseCountRequest(gameData);
}

void BoardDispatch::sendAcceptDrawRequest(void)
{
	connection->sendAcceptDrawRequest(gameData);
}

void BoardDispatch::sendRefuseDrawRequest(void)
{
	connection->sendRefuseDrawRequest(gameData);
}

void BoardDispatch::adjournGame(void)
{
	// in case we get this after we've closed window
	if(boardwindow)
		boardwindow->qgoboard->adjournGame();
	//wrong, sometimes there is a dispatch if we want to talk, etc.
	//let the close and the connection delete things
	//delete this;		//if the game is adjourned, there's no dispatch
}

void BoardDispatch::setRematchDialog(ResultDialog * r)
{
	resultdialog = r;
}

void BoardDispatch::sendRematchRequest(void)
{
	connection->sendRematchRequest();
}

void BoardDispatch::recvRematchRequest(void)	//crash here somehow
{
	if(resultdialog)
		resultdialog->recvRematchRequest();
	else if(gameData)	//incase we get this after recreating new dispatch somehow
		resultdialog = new ResultDialog(boardwindow, this, gameData->number, 0);
	else
		qDebug("board dispatch received rematch but no gameData present");
}

void BoardDispatch::sendTime(void)
{
	if(connection)
		connection->sendTime(this);
}

void BoardDispatch::sendAddTime(int minutes)
{
	connection->sendAddTime(minutes);
}

/* So far just to trigger timer */
void BoardDispatch::startGame(void)
{
    // Only used by Tygem connection, so assume that this is qGoBoardNetworkInterface::startGame();
	boardwindow->qgoboard->startGame();	
}

void BoardDispatch::sendRematchAccept(void)
{
	connection->sendRematchAccept();
	resultdialog = 0;
}

void BoardDispatch::recvRefuseAdjourn(void)
{
	// in case we get this after we've closed window
	if(boardwindow)
		boardwindow->qgoboard->recvRefuseAdjourn();
}

/* When we start observing a game, we get a little bit of information,
 * the rest we have to get from a listing that the room dispatch passes
 * us on creation.  This could be trouble later because its specific
 * to IGS, but maybe not */
void BoardDispatch::mergeListingIntoRecord(GameData * r, GameListing * l)
{
	/* IGS doesn't, for instance, serve up the board size very well,
	 * so we need to get that from somewhere else... the listing,
	 * but if its a new game... 
	 * Basically only the game dialog knows the board size, so it 
	 * has to supply it.*/
	/* I'm thinking that we should just have the gamedialog create
	 * the board.  It should know the room, all the info, etc. */
	if(r->white_rank == QString())
	{
		r->white_rank = l->white_rank();
		r->black_rank = l->black_rank();
	}
	if(r->white_name == QString())
	{
		r->white_name = l->white_name();
		r->black_name = l->black_name();
	}
	/* FIXME Oro, second matches tend to lose the ranks.  Add rematch
	 * and decline msgs and then fix this up.  Also nigiri cursor */
	qDebug("bd::mlir %s %s vs %s %s", r->white_name.toLatin1().constData(), r->white_rank.toLatin1().constData(), 
	       				r->black_name.toLatin1().constData(), r->black_rank.toLatin1().constData());
	qDebug("komi: %f\n", r->komi);
	/* FIXME, no komi in ORO listing... and
	 * what is this function for again?  Maybe we shouldn't
	 * always do this?? FIXME FIXME */

	/* FIXME we get here from IGS without white or black names somehow.
	 * also if game list has changed before refresh there's other issues */
	
	/* FIXME if player observes IGS through console, the ranks can come up wrong
	 * possibly because the games list hasn't been refreshed and that thing above
	 * sets the ranks to what they are in the listing.  They're not passed in the
	 * game info from the server.  There's little that can be done about it, its
	 * more of a protocol bug.  And I still want to get rid of this whole function */

	//FIXME, trying not overwriting komi for now
	//FIXME This is not okay, komi is apparently not passed all the time or with
	//observed games on IGS, but we shouldn't have to do this:
	if(connection->getPlaceString() == "IGS")
		r->komi = l->komi;
	//r->board_size = l->board_size;
	r->moves = l->moves;
	//r->handicap = l->handicap;
	r->By = l->By;
	r->FR = l->FR;
	r->observers = l->observers;
	
	//GameData * gd = qgoif->makeGameData(gameData);  //FIXME
	if (gameData->FR.contains("F"))
		gameData->free_rated = FREE;
	else if (gameData->FR.contains("T"))
		gameData->free_rated = TEACHING;
	else
		gameData->free_rated = RATED;
	
	gameData->date = QDate::currentDate().toString("dd MM yyyy") ;
}

/* There's no "black komi" or I think we'd store it as a negative float.
 * IGS lists them both the same though on the status messages which
 * makes even less sense then the status message itself */
bool BoardDispatch::isAttribBoard(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi)
{
	if(!gameData)
		return 0;
	qDebug("%s %s %s %s %d %d %d %d %f %f (b%f)\n",
		black_player.toLatin1().constData(),
		gameData->black_name.toLatin1().constData(),
		white_player.toLatin1().constData(),
		gameData->white_name.toLatin1().constData(),
		black_captures,
		gameData->black_prisoners,
		white_captures,
		gameData->white_prisoners,
		white_komi,
		gameData->komi, black_komi);
	/* Dead stones are included in IGS message captures */
	if(black_player == gameData->black_name &&
	   white_player == gameData->white_name &&
	   /*black_captures == gameData->black_prisoners &&
	   white_captures == gameData->white_prisoners &&*/
	   white_komi == gameData->komi)
	   	return 1;
	else
		return 0;
}

bool BoardDispatch::isOpponentBoard(QString us, QString them)
{
	if(!gameData)
		return 0;
	if((gameData->black_name == us && gameData->white_name == them) || (gameData->white_name == us && gameData->black_name == them))
		return 1;
	else if(them == "" && (gameData->black_name == us || gameData->white_name == us))
		return 1;
	else
		return 0;
}

void BoardDispatch::swapColors(bool noswap)
{
	boardwindow->swapColors(noswap);
}

void BoardDispatch::requestGameInfo(void)
{
	connection->requestGameInfo(gameData->number);
}

int BoardDispatch::getMoveNumber(void)
{
	return boardwindow->qgoboard->getMoveNumber();
}

GameData * BoardDispatch::getGameData(void)
{
	return gameData;
}

/* We're getting this associated with sendKeepAlives and
 * possibly rematches.  We shouldn't need to check for
 * boardwindow.  FIXME */
TimeRecord BoardDispatch::getOurTimeRecord(void)
{
	if(boardwindow)
		return boardwindow->qgoboard->getOurTimeRecord();
	else
		return TimeRecord();
}

TimeRecord BoardDispatch::getTheirTimeRecord(void)
{
	if(boardwindow)
		return boardwindow->qgoboard->getTheirTimeRecord();
	else
		return TimeRecord();
}

QString BoardDispatch::getOpponentName(void)
{
	if(!gameData)
		return QString();
	if(gameData->black_name == getUsername())
		return gameData->white_name;
	else if(gameData->white_name == getUsername())
		return gameData->black_name;
	else
		return QString();
}

QString BoardDispatch::getUsername(void) { return connection->getUsername(); }

QString BoardDispatch::getOurRank(void)
{
    return connection->getOurListing()->rank;
}

bool BoardDispatch::getBlackTurn(void)
{
	if(boardwindow && boardwindow->qgoboard)
		return boardwindow->qgoboard->getBlackTurn();
	return false;
}

class ObserverListModel * BoardDispatch::getObserverListModelForRematch(void)
{
	class ObserverListModel * olm = observerListModel;
	observerListModel = 0;
	return olm;
}		

void BoardDispatch::setObserverListModel(class ObserverListModel * olm)
{
	if(observerListModel)
		qDebug("warning: observer list model already set! overwriting");
	observerListModel = olm;
}

bool BoardDispatch::supportsRematch(void)
{ 
	/* A little tricky to have this check here but... */
	if(!gameData || gameData->gameMode != modeMatch)
		return false;
	if(connection) 
		return connection->supportsRematch(); 
	return false;
}

bool BoardDispatch::flipCoords(void) { return connection->flipCoords(); }
bool BoardDispatch::supportsMultipleUndo(void) { return connection->supportsMultipleUndo(); }
bool BoardDispatch::supportsRequestMatchMode(void) { return connection->supportsRequestMatchMode(); };
bool BoardDispatch::supportsRequestCount(void) { return connection->supportsRequestCount(); };
bool BoardDispatch::supportsRequestDraw(void) { return connection->supportsRequestDraw(); };
bool BoardDispatch::supportsRequestAdjourn(void) { return connection->supportsRequestAdjourn(); };
bool BoardDispatch::supportsAddTime(void) { return connection->supportsAddTime(); };
bool BoardDispatch::startTimerOnOpen(void) {return connection->startTimerOnOpen(); }
bool BoardDispatch::clientCountsTime(void) { return connection->clientCountsTime(); }
bool BoardDispatch::clientSendsTime(void) { return connection->clientSendsTime(); }
bool BoardDispatch::twoPassesEndsGame(void) { return connection->twoPassesEndsGame(); }
bool BoardDispatch::netWillEnterScoreMode(void) { return connection->netWillEnterScoreMode(); }
bool BoardDispatch::undoResetsScore(void) { return connection->undoResetsScore(); }
bool BoardDispatch::canMarkStonesDeadinScore(void) { return connection->canMarkStonesDeadinScore(); }
bool BoardDispatch::unmarkUnmarksAllDeadStones(void) { return connection->unmarkUnmarksAllDeadStones(); }
bool BoardDispatch::cantMarkOppStonesDead(void) { return connection->cantMarkOppStonesDead(); }
