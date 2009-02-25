#include "boarddispatch.h"
#include "boardwindow.h"
#include "messages.h"
#include "listviews.h"
#include "../game_interfaces/resultdialog.h"
#include "../game_interfaces/countdialog.h"
#include "gamedata.h"
#include "playergamelistings.h"
#include "networkconnection.h"

/* It would be difficult to create a board without a connection,
 * but we need to either be certain and not test at all, or assign
 * it at creation.  I'm just worried about the client code knowing
 * too much about the connection.*/
/* Also the !boardwindow things?  I mean its pretty damn hard
 * to get these messages without them, we just just need to be sure */

BoardDispatch::BoardDispatch(NetworkConnection * conn, GameListing * l)
{
	qDebug("Creating new Board Dispatch\n");
	
	connection = conn;
	boardwindow = 0;
	resultdialog = 0;
	countdialog = 0;
	/* New rules.  The boarddispatch creates the game data or
	 * whatever loads the game data before passing it to the
	 * boardwindow creates it.  The boardwindow deletes it.
	 * there's only one gameData per board */
	gameData = new GameData();
	//maybe we could set number here?  if we have it?
	
	/* Not sure we can do this. GameListings have to have a
	 * key.  But to go without a gameListing in the BoardDispatch?*/
	if(l)
		gameListing = new GameListing(*l);
	//else
	//	gameListing = new GameListing();
}

/* Somewhere we need to send the console commands to quit the board
 * to the server, I'm just not sure where.  The type is different
 * dependent on the board/game opened so that would imply here,
 * but its dependent on the connection as well... I guess we just
 * need to support each type of game.
 * Anyway, code is taken from qGoIF::slot_boardClosed*/
BoardDispatch::~BoardDispatch()
{
	qDebug("Destroying board dispatch\n");
	/* Clear dispatch so boardwindow doesn't try to close it */
	if(boardwindow)
	{
		clearObservers();
		if(boardwindow->getGamePhase() != phaseEnded)
		{
			boardwindow->qgoboard->stopTime();
			boardwindow->qgoboard->kibitzReceived("Disconnected");
		}
		boardwindow->setBoardDispatch(0);
	}
	if(resultdialog)
		delete resultdialog;
	if(countdialog)
		delete countdialog;
	
	delete gameListing;
}

/* FIXME, we need to make better use of this */
void BoardDispatch::closeBoard(void)
{
	qDebug("bd::closeBoard");
	if(connection)
	{
		switch(boardwindow->getGameMode())
		{
			case modeObserve:
				if(boardwindow->getGamePhase() != phaseEnded)
					connection->stopObserving(*gameListing); //FIXME	
				break;
			case modeReview:
				connection->stopReviewing(*gameListing);	
				break;
			case modeMatch:
				/* FIXME if game is over, we don't need to adjourn
				 * nor perhaps with the above modes either
				 * so we should check this out... maybe this
				 * function shouldn't be being called */
				if(boardwindow->getGamePhase() != phaseEnded)
					connection->adjournGame(*gameListing);	 //FIXME
				break;
			default:
				qDebug("Unknown game Mode, board dispatch does nothing\n");
				break;
		}
		if(gameData)
		{
			qDebug("Closing board dispatch\n");
			int number = gameData->number;
			gameData = 0;
			connection->closeBoardDispatch(number);
		}
	}
	//lets have closeBoardDispatch do this
	//delete this;
}

void BoardDispatch::recvMove(MoveRecord * m)
{
	if(!boardwindow)
	{
		qDebug("Board dispatch has no board window\n");
		return;
	}
	boardwindow->qgoboard->set_move(m);
}

void BoardDispatch::sendMove(MoveRecord * m)
{
	if(m->number && m->flags == MoveRecord::NONE)	//i.e., a move
		gameData->moves = m->number;
	qDebug("setting game moves: %d", gameData->moves);
	if(connection)
		connection->sendMove(gameData->number, m);
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
	if(!boardwindow)
	{
		if(!connection)
		{
			qDebug("Connection not set on board dispatch\n");
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
		boardwindow->observerListModel->setAccountName(myName);
		// do we need the below?
		//boardwindow->qgoboard->set_statedMoveCount(gameData->moves);
	}
	else
	{
		qDebug("Game data update");
		/* Probably need to call some boardwindow text update thing
		 * Actually, recvRecord would just be called to trigger
		 * some kind of update since we would alter the record directly.
		 * But I want just one game data I think... but who creates
		 * and who deletes.  We usually claim network creates, and...
		 * board window deletes?  That seems okay.  What if there's a
		 * failure when game starts up?  But I guess record would already
		 * exist in some sense so... FIXME */
		
		
		/* Mainly to allow accurate IGS status lookups */
		//gameData->black_prisoners = g->black_prisoners;
		//gameData->white_prisoners = g->white_prisoners;
	}
	//boardwindow = bw;
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

void BoardDispatch::recvResult(GameResult * r)
{
	qDebug("here: %p", this);
	qDebug("BW: %p dispatch for: %d", boardwindow, gameData ? gameData->number : -1);
	if(!boardwindow)
		return;
	if(!r)
	{
		GameResult res = boardwindow->getBoardHandler()->retrieveScore();
		r = &res;
	}
	// also set on GameData of boardwindow ???
	if(!gameData)
	{
		qDebug("No game record to check with result");
		return;	
	} 
	/* IGS passes sparse entries... bit ugly but: */
	if(r->winner_name == QString() && r->winner_color == stoneNone)
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
	
	//testing
	//connection->sendRematchRequest(gameData->number);
}

void BoardDispatch::sendResult(GameResult * r)
{
	connection->sendResult(gameData, r);	
}

void BoardDispatch::recvObserver(PlayerListing * p, bool present)
{
	if(!boardwindow)
		return;
	if(present)
		boardwindow->observerListModel->insertListing(p);
	else
		boardwindow->observerListModel->removeListing(p);
}

void BoardDispatch::clearObservers(void)
{
	if(!boardwindow)
		return;
	boardwindow->observerListModel->clearList();
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
 * entering score mode within set_move?  why would we do it here?
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

void BoardDispatch::recvRequestCount(void)
{
	//this is those count messages before passes
	//FIXME
	//we need to copy the matchinvite file to a
	//RequestCountDialog, etc.
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

void BoardDispatch::recvRematchRequest(void)
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

/* So far just to trigger timer */
void BoardDispatch::startGame(void)
{
	boardwindow->qgoboard->startGame();	
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
	r->white_rank = l->white_rank();
	r->black_rank = l->black_rank();
	/* FIXME Oro, second matches tend to lose the ranks.  Add rematch
	 * and decline msgs and then fix this up.  Also nigiri cursor */
	qDebug("bd::mlir %s %s vs %s %s", r->white_name.toLatin1().constData(), r->white_rank.toLatin1().constData(), 
	       				r->black_name.toLatin1().constData(), r->black_rank.toLatin1().constData());
	/* FIXME, no komi in ORO listing... and
	 * what is this function for again?  Maybe we shouldn't
	 * always do this?? FIXME FIXME */

	/* FIXME we get here from IGS without white or black names somehow.
	 * also if game list has changed before refresh there's other issues */
	
	//FIXME, trying not overwriting komi for now
	//r->komi = l->komi;
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
	if(!noswap)
	{
		QString rank, name;
		name = gameData->black_name;
		rank = gameData->black_rank;
		gameData->black_name = gameData->white_name;
		gameData->black_rank = gameData->white_rank;
		gameData->white_name = name;
		gameData->white_rank = rank;
		qDebug("bd:swapColors: %s %s vs %s %s", gameData->black_name.toLatin1().constData(), gameData->black_rank.toLatin1().constData(), gameData->white_name.toLatin1().constData(), gameData->white_rank.toLatin1().constData());
	
	}
	gameData->nigiriToBeSettled = false;
}

void BoardDispatch::requestGameInfo(void)
{
	connection->requestGameInfo(gameData->number);
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
