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


#include <string.h>
#include <QDebug>
#include "igsconnection.h"
#include "consoledispatch.h"
#include "room.h" // Should not depend on room FIXME
#include "boarddispatch.h"
#include "gamedialog.h"
#include "talk.h"
#include "gamedialogflags.h"
#include "playergamelistings.h"
#include "matchnegotiationstate.h"

#define PLAYERSLISTREFRESH_SECONDS		300
#define GAMESLISTREFRESH_SECONDS		180

IGSConnection::IGSConnection(const ConnectionCredentials credentials)
    : NetworkConnection(credentials)
{
    if(openConnection(hostname,port))
	{
        setState(LOGIN);
	}
	else
		qDebug("Can't open Connection\n");	//throw error?

	init();
}

void IGSConnection::init(void)
{
    keepAliveTimer = 0;
	playersListRefreshTimer = 0;
	gamesListRefreshTimer = 0;
	textCodec = QTextCodec::codecForLocale();
	btime = new TimeRecord();
	wtime = new TimeRecord();
	protocol_save_int = -1;
	game_were_playing = 0;
	guestAccount = false;
	needToSendClientToggle = true;
}

IGSConnection::~IGSConnection()
{
	qDebug("Destroying IGS connection");
	/* Maybe closeConnection shouldn't be here?
	 * at any rate, if there's been an error it
	 * really shouldn't be here */
	sendDisconnect();
	closeConnection();
	delete wtime;
	delete btime;
}

QString IGSConnection::getPlaceString(void)
{
	return "IGS";
}

void IGSConnection::sendText(QString text)
{
	// FIXME We're getting some nonsense on the end of this sometimes for
	// some reason
	//text += "\r\n";
	qDebug("sendText: %s", text.toLatin1().constData());
	QByteArray raw = textCodec->fromUnicode(text);
	if(write(raw.data(), raw.size()) < 0)
		qWarning("*** failed sending to host: %s", raw.data());
}

/* This is kind of ugly, but we can't add sending return-newline to
 * superclass, which means either reallocing text here to send both
 * or this little flag trick to sneak in a second write when we
 * got the first through.  Note that the real solution would be
 * to change every sendText command in this file to have a "\r\n"
 * on it.  That is sort of the better way. FIXME 
 * Note also that network connection does readLine in writeFromBuffer
 * and also write an additional newline...*/
void IGSConnection::sendText(const char * text)
{
    sendText(QString(text));
}

void IGSConnection::sendDisconnect(void)
{
	sendText("exit\r\n");
}

/* What about a room_id?? */
void IGSConnection::sendMsg(unsigned int game_id, QString text)
{
	BoardDispatch * bd = getIfBoardDispatch(game_id);
	if(!bd)
	{
		qDebug("No board dispatch to send message from");
		return;
	}
	GameData * g = bd->getGameData();
	switch(g->gameMode)
	{
		case modeReview:
		case modeMatch:
			sendText("say " + text + "\r\n");
			break;
		case modeObserve:
			sendText("kibitz " + QString::number(game_id) + " " + text + "\r\n");
			break;
		default:
			qDebug("no game type");
			break;
	}
}

void IGSConnection::sendMsg(PlayerListing * player, QString text)
{
    sendText("tell " + player->name + " " + text + "\r\n");
}

void IGSConnection::sendToggle(const QString & param, bool val)
{
	QString value;
	if (val)
		value = " true";
	else
		value = " false";
	sendText("toggle " + param + " " + value + "\r\n");
}

/* If we can observe something besides games,
 * like rooms or people, then this name is a bit
 * iffy too... */
void IGSConnection::sendObserve(const GameListing *game)
{
    unsigned int game_id = game->number;
	if(getIfBoardDispatch(game_id))		//don't observe twice, it unobserves
		return;
	protocol_save_int = game_id;	//since this isn't always reported back
	match_negotiation_state->sendJoinRoom(game_id);
	sendText("observe " + QString::number(game_id) + "\r\n");
}

void IGSConnection::stopObserving(const GameListing *game)
{
    sendText("unobserve " + QString::number(game->number) + "\r\n");
}

void IGSConnection::stopReviewing(const GameListing * game)
{
    sendText("review quit " + QString::number(game->number) + "\r\n");
}

void IGSConnection::sendStatsRequest(const PlayerListing * opponent)
{
    sendText("stats " + opponent->name + "\r\n");
}

void IGSConnection::sendPlayersRequest(void)
{
	sendText("userlist\r\n");
}

void IGSConnection::sendGamesRequest(void)
{
	sendText("games\r\n");
}

void IGSConnection::sendRoomListRequest(void)
{
	sendText("room\r\n");
}

void IGSConnection::sendJoinRoom(const RoomListing & room, const char * /*password*/)
{
	sendText("join " + QString::number(room.number) + "\r\n");
	qDebug("Joining %d", room.number);
	setCurrentRoom(room);	//FIXME unless we get a message after joining?
							//but then the other send requests should
							//be there
}

void IGSConnection::sendJoinChannel(const ChannelListing & room)
{
	if(room.number == 0)
		sendText("yell \\-1\r\n");
	else
		sendText("yell \\" + QString::number(room.number) + "\r\n");
	//set the current room somehow?
}

void IGSConnection::sendMatchInvite(const PlayerListing * player)
{
	MatchRequest * m = 0;
	/* No match Invites, just popup the dialog */
    GameDialog * gd = getGameDialog(player);
    if(player->nmatch)
	{
        const PlayerListing * us = getOurListing();
		m = new MatchRequest();
        m->opponent = player->name;
        m->opponent_id = player->id;
        m->their_rank = player->rank;
        m->our_name = us->name;
        m->our_rank = us->rank;
		
        m->timeSystem = player->nmatch_timeSystem;
        m->maintime = player->nmatch_timeMin;
        m->periodtime = player->nmatch_BYMin;
		m->stones_periods = 25;
        m->nmatch_timeMax = player->nmatch_timeMax;
        m->nmatch_BYMax = player->nmatch_BYMax;
        if(!player->nmatch_handicapMin)
			m->handicap = 1;
		else
            m->handicap = player->nmatch_handicapMin;
        m->nmatch_handicapMax = player->nmatch_handicapMax;
		m->opponent_is_challenger = false;
		m->first_offer = true;		//because we're sending it!!
		//mr->first_offer = false;	//because we don't want to toggle to match from nmatch
		//mr->rated = false;
	}
	gd->recvRequest(m, getGameDialogFlags());
}

void IGSConnection::sendAddTime(int minutes)
{
	sendText("addtime " + QString::number(minutes) + "\r\n");
}

/* I'm thinking you can only play one game at a time on IGS,
 * but the game_ids are there in case it changes its mind */
void IGSConnection::adjournGame(const GameListing */*game_id*/)
{
	/* double check this one */
	/* There are three of these adjournGame, sendAdjournGame, and sendAdjournRequest FIXME */
	sendText("adjourn\r\n");
}

void IGSConnection::sendMove(unsigned int game_id, MoveRecord * move)
{
	switch(move->flags)
	{
		case MoveRecord::PASS:
			sendText("pass\r\n");
			break;
		case MoveRecord::REQUESTUNDO:
			sendText("undoplease\r\n");	//so polite
			break;
		case MoveRecord::UNDO:
			sendText("undo\r\n");
			break;
		case MoveRecord::REFUSEUNDO:
			sendText("noundo\r\n");
			break;
		case MoveRecord::RESIGN:
			sendText("resign\r\n");
			break;
		case MoveRecord::DONE_SCORING:
			sendText("done\r\n");
			break;
		case MoveRecord::NONE:
		case MoveRecord::REMOVE:	//handled simply
		case MoveRecord::UNREMOVE:
			{
			char c1 = move->x - 1 + 'A';
			if(move->x > 8)		// no I in IGS
				c1++;
			BoardDispatch * bd = getIfBoardDispatch(game_id);
			if(!bd)
			{
				qDebug("No board dispatch to send move from");
				return;
			}
			GameData * g = bd->getGameData();
			int c2 = g->board_size + 1 - move->y; 
			/* Why do we send the id here but
			 * not with the others?  Can we play
			 * multiple games?? */
			sendText(QString(c1) + QString::number(c2) + " " + QString::number(game_id) + "\r\n");	
			}
			break;
		default:
			qDebug("IGSConnection: unhandled  type\n");
			break;
	}
}

void IGSConnection::sendMatchRequest(MatchRequest * mr)
{
	QString color;
	switch(mr->color_request)
	{
		case MatchRequest::BLACK:
			color = " B ";
			break;
		case MatchRequest::WHITE:
			color = " W ";
			break;
		case MatchRequest::NIGIRI:
			color = " N ";
			break;		
	}
	/* FIXME nmatch almost shouldn't be a setting on the match
	 * request.  We should check the settings here and if they're
	 * fairly simple, change it to match in order to be compatible
	 * with more other clients... we need to check if this is
	 * our first offer as well we don't want to change to match
	 * mid negotiation.
	 * nmatch does have to be a setting though so we can't change
	 * it through negotiation on game dialog*/
	if(mr->first_offer)
	{
		mr->first_offer = false;
		if(mr->timeSystem == canadian && color != " N ")
			mr->nmatch = false;
		if(mr->handicap)
			mr->nmatch = true;
	}
	if(mr->nmatch)
	{
		if(mr->timeSystem == byoyomi)
			sendText("nmatch " + mr->opponent +
				color +
				QString::number(mr->handicap) + " " +
				QString::number(mr->board_size) + " " +
				QString::number(mr->maintime) + " " +
				QString::number(mr->periodtime) + " " +
				QString::number(mr->stones_periods) + " " +
				"0 0 0\r\n");
		else if(mr->timeSystem == canadian)
			sendText("nmatch " + mr->opponent +
					color +
					QString::number(mr->handicap) + " " +
					QString::number(mr->board_size) + " " +
					QString::number(mr->maintime) + " " +
					QString::number(mr->periodtime) + " " +
					QString::number(mr->stones_periods) + " " +
					"0 0 0\r\n");
		
	}
	else
	{
		sendText("match " + mr->opponent +
				color +
				QString::number(mr->board_size) + " " +
				QString::number(mr->maintime / 60) + " " +
				QString::number(mr->periodtime / 60) + "\r\n");
	}
	match_playerName = mr->opponent;
}

unsigned long IGSConnection::getGameDialogFlags(void)
{
	//GDF_STONES25_FIXED is only set in "match" not "nmatch"
	//there's also pmatch and tdmatch now I think
	return (GDF_CANADIAN | /*GDF_BYOYOMI |*/ GDF_CANADIAN300 | GDF_BY_CAN_MAIN_MIN |
		 GDF_NIGIRI_EVEN | GDF_KOMI_FIXED6
			/*| GDF_STONES25_FIXED*/);
}

void IGSConnection::declineMatchOffer(const PlayerListing * opponent)
{
	// also possibly a "withdraw" message before opponent has responded FIXME
    sendText("decline " + opponent->name + "\r\n");
}

void IGSConnection::acceptMatchOffer(const PlayerListing * /*opponent*/, MatchRequest * mr)
{
	/* For IGS, we just send the same match request, assuming it hasn't changed.
	* It would be bad to have the connection code hold the match request, so
	* we'll have the gamedialogdispatch pass it along and we will trust
	* that its the same.*/
	sendMatchRequest(mr);
}

QTime IGSConnection::gd_checkMainTime(TimeSystem s, const QTime & t)
{
	// match settings are between 1 and 530 with seemingly
	// infinite byo yomi time
	// nmatch is 0 to 3600 for byotime
	// byomoves 0 to 25
	// koryotimecount 0 - 100
	// koryosec 0 - 600
	// preboyoyomi 0 - 10
	// no handicap nigiri
	// if byomoves is not 1 koryo count must be 0
	// if koryosec is not zero koryo count must not be 0
	// time mod 60, but they're all like that
	// note that we currently have match settings preventing
	// pre byoyomi as well as koryo
	QTime c = NetworkConnection::gd_checkMainTime(s, t);
	int seconds = (c.minute() * 60) + c.second();
	int minutes = c.minute();
	switch(s)
	{
		case canadian:
			if(minutes < 1)
				return QTime(0, 1, 0);
			else if(minutes > 530)
				return QTime(8, 50, 0);
			break;
		case byoyomi:
			//FIXME isn't only periodtime supposed to be less than 5?
			if(seconds >= 300)
				return QTime(0, 4, 0);
			else
				return QTime(0, c.minute(), 0);
			break;
		default:
			break;
	}
	return c;
}

QTime IGSConnection::gd_checkPeriodTime(TimeSystem s, const QTime & t)
{
	int seconds = (t.minute() * 60) + t.second();
	switch(s)
	{
		case canadian:
			if(seconds < 300)
				return QTime(0, 5, 0);
			break;
		case byoyomi:
			if(seconds > 299)
				return QTime(0, 4, 59);
			else if(seconds < 5)	//actually should depend on maintime but...
				return QTime(0, 0, 5);
			break;
		default:
			qDebug("unsupported IGS time type p");
			break;
	}
	return t;
}

unsigned int IGSConnection::gd_checkPeriods(TimeSystem s, unsigned int p)
{
	switch(s)
	{
		case canadian:
			if(p > 25)
				return 25;
			else
				return p;
			break;
		case byoyomi:
			if(p > 100)
				return 100;
			else if(p < 1)
				return 1;
			else
				return p;
			break;
		default:
			qDebug("unsupported IGS time type");
			break;
	}
	return p;
}


/* Perhaps IGS does not allow multiple games, but this here will be trickier
 * possibly, with other services that might since there's no game id here */
void IGSConnection::sendAdjournRequest(void)
{
	sendText("adjourn\r\n");
}

void IGSConnection::sendAdjourn(void)
{
	sendText("adjourn\r\n");
}

void IGSConnection::sendRefuseAdjourn(void)
{
	sendText("decline adjourn\r\n");
}

/* There's honestly not much of a reason why we moved this out
 * of the mainwindow_server except that it doesn't really belong
 * there and we don't want the mainwindow ui code to be coupled
 * to the IGS code.  That said, since we've yet to add
 * another seek facility to a different protocol, its completely
 * tailored to IGS, but I guess its a start */
void IGSConnection::sendSeek(SeekCondition * s)
{
	//seek entry 1 19 5 3 0
	QString send_seek = "seek entry " + 
			QString::number(s->number) + 
			" 19 " + s->strength_wished + "\r\n";
	sendText(send_seek);
}

void IGSConnection::sendSeekCancel(void)
{
	sendText("seek entry_cancel\r\n");
}

/*void IGSConnection::setAccountAttrib(AccountAttib * aa)
{
		
}*/

/* So server sends type codes */
void IGSConnection::sendToggleClientOn(void)
{
	sendText("toggle client on\r\n");
}

void IGSConnection::sendListChannels(void)
{
	//could also clear channel list but unnecessary
	ChannelListing * c = new ChannelListing();
	c->number = 0;
	c->name = "No Channel";
	recvChannelListing(c);
	sendText("channels\r\n");
}

void IGSConnection::handlePendingData()
{
	int bytes;

	switch(connectionState)
    {
    case LOGIN:
        bytes = qsocket->bytesAvailable();
        if(bytes)
        {
            QByteArray data = qsocket->readAll();
            handleLogin(QString(data));
        }
        break;
    case PASSWORD:
        bytes = qsocket->bytesAvailable();
        if(bytes)
        {
            QByteArray data = qsocket->readAll();
            handlePassword(QString(data));
        }
        break;
    case PASSWORD_SENT:
    case CONNECTED:
        while(qsocket->canReadLine())
        {
            QByteArray data = qsocket->readLine();
            handleMessage(QString(data));
        }
        break;
    case AUTH_FAILED:
        qDebug("Auth failed\n");
        break;
    case PASS_FAILED:
        qDebug("Pass failed\n");
        break;
    case PROTOCOL_ERROR:
        //wait for someone to destroy the connection
        //can't imagine why we're even here
        break;
    default:
        qDebug("Connection State not related to IGS protocol!!!");
        break;
    }
}

void IGSConnection::handleLogin(QString msg)
{
	if(msg.contains("Login:") > 0)
	{
		qDebug("Login found\n");
        QString u = username + "\r\n";
		sendText(u.toLatin1().constData());	
        setState(PASSWORD);
	}
	else if(msg.contains("sorry") > 0)
	{
        setState(AUTH_FAILED);
		if(console_dispatch)
			console_dispatch->recvText("Sorry");
	}
	else if(console_dispatch)
		console_dispatch->recvText(msg.toLatin1().constData());
}

/* FIXME onAuthenticationNegotiated may need to
 * be called earlier */
void IGSConnection::handlePassword(QString msg)
{
	qDebug(":%d %s\n", msg.size(), msg.toLatin1().constData());
	if(msg.contains("Password:") > 0 || msg.contains("1 1") > 0)
	{
		qDebug("Password prompt or 1 1 found");
        if(password == QString())
		{
			onAuthenticationNegotiated();
		}
		else
		{
			QString p = password + "\r\n";
			sendText(p.toLatin1().constData());
		}	
        setState(PASSWORD_SENT);
	}
	else if(msg.contains("guest"))
	{
        qDebug("Guest account");
        setState(PASSWORD_SENT);
		guestAccount = true;
	}
}

void IGSConnection::onAuthenticationNegotiated(void)
{
	needToSendClientToggle = false;
	sendToggleClientOn();
    NetworkConnection::onAuthenticationNegotiated();
	NetworkConnection::onReady();
}

void IGSConnection::onReady(void)
{
	if(firstonReadyCall)
	{
		/* Check here because I've forgotten exactly which kinds of "1"
		 * msgs might call onReady() and apparently it can happen
		 * after our password has been refused.  I'll add this to
		 * WING and LGS as well*/
		if(connectionState != PASSWORD_SENT)
			return;
		firstonReadyCall = 0;
        setState(CONNECTED);
		setKeepAlive(600);
		/* This gets called too much, we need a better
		 * way to call it */
		/* also needs to be earlier */
		/* Below is kind of round-about.  We need the account name in order to make our
		* name in the players list blue.  We could also use something similar to 
		* affect colors of observed or played games, although I'm not sure why we'd
		* do any of this.  But it means setting the account name on the listview model
		* from the room.  The room has no other real use for the account name, so we
		* pull it from the connection just for that.  Its a lot of misdirection just
		* preserve the encapsulation but I suppose its okay for now.  We might
		* fix it along the lines of the above at some point. */
		/* Specifically, when we figure out how we're going to deal with rooms in the
		* future, then we'll find a better place to do this and set the connection
		* etc.. the idea of a "DefaultRoom" will sort of drop away. FIXME */
		/* There's a bug here where a person who's name is "" gets painted blue FIXME */
		//getDefaultRoom()->setAccountName(username);
	
		QString v = "id qGov" + QString(VERSION) + "\r\n";
		sendText(v.toLatin1().constData());
		
		//onAuthenticationNegotiated();
		
		if(!guestAccount)
		{
			sendText("toggle newundo on\r\n");		//undoplease undo requests
			//sendText("toggle client on\r\n");		//adds type codes, done earlier
			sendText("toggle nmatch on\r\n");		//allows nmatch
			sendText("toggle seek on\r\n");
			sendText("toggle newrating\r\n");		//?s and +s
		
			//sendText("toggle quiet on\r\n");		//FIXME do we want this?
			//sendText("toggle quiet off\r\n");
			//sendText("toggle review on\r\n");
			
            //sendPlayersRequest();
			
			sendNmatchParameters();
			sendText("seek config_list\r\n");
		}
	
        //sendGamesRequest();
		recvRoomListing(new RoomListing(0, "Lobby"));
		sendRoomListRequest();
		sendListChannels();

		qDebug("Ready!\n");
    }
}

/* In case we want to add updates every so often */
void IGSConnection::timerEvent(QTimerEvent* e)
{
	if(e->timerId() == keepAliveTimer)
		sendText("ayt\r\n");
}

void IGSConnection::setKeepAlive(int seconds)
{
	if(keepAliveTimer)
		killTimer(keepAliveTimer);
	
	if(seconds > 0)
		keepAliveTimer = startTimer(seconds * 1000);
	else
		keepAliveTimer = 0;
}

/* Because the IGS protocol is garbage, we have to break encapsulation here
 * and in BoardDispatch */
BoardDispatch * IGSConnection::getBoardFromAttrib(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi)
{
	BoardDispatch * board;
    QMap<unsigned int, class BoardDispatch *>::iterator i;
    for(i = boardDispatchMap.begin(); i != boardDispatchMap.end(); i++)
	{
        board = i.value();
		if(board->isAttribBoard(black_player, black_captures, black_komi, white_player, white_captures, white_komi))
			return board;
	}
	return NULL;
}

BoardDispatch * IGSConnection::getBoardFromOurOpponent(QString opponent)
{
	BoardDispatch * board;
    QMap<unsigned int, class BoardDispatch *>::iterator i;
	/* Parser may supply our name from the IGS protocol... this is ugly
	 * but I'm really just trying to reconcile what I think a real
 	 * protocol would be like with the IGS protocol */
	if(opponent == username)
		opponent = "";
    for(i = boardDispatchMap.begin(); i != boardDispatchMap.end(); i++)
	{
        board = i.value();
		if(board->isOpponentBoard(username, opponent))
			return board;
	}
	return NULL;
}

void IGSConnection::requestGameInfo(unsigned int game_id)
{
	char string[20];
	snprintf(string, 20, "moves %d\r\n", game_id);
	sendText(string);
	snprintf(string, 20, "all %d\r\n", game_id);
	sendText(string);
}

void IGSConnection::requestGameStats(unsigned int game_id)
{
	qDebug("requestGameStats");
	char string[20];
	snprintf(string, 20, "game %d\r\n", game_id);
	sendText(string);
}

int IGSConnection::time_to_seconds(const QString & time)
{
	QRegExp re = QRegExp("([0-9]{1,2}):([0-9]{1,2})");
	int min, sec;
	
	if(re.indexIn(time) >= 0)
	{
		min = re.cap(1).toInt();
		sec = re.cap(2).toInt();	
	}
	else
	{
		qDebug("Bad time string");
		return 0xffff;
	}
	
	return (60 * min) + sec;
}

/* formulas come from old MainWindow::rkToKey for sorting.
 * might be worth fixing them to match some real score
 * system*/
/* This is a kind of utility function but I'm not sure where to
 * put it yet.  FIXME
 * If I had to guess, the best place for it is probably on
 * a rank object or something used in messages... but we don't
 * want to overload those.*/
unsigned int IGSConnection::rankToScore(QString rank)
{
	if(rank == "NR")
		return 0;
	if(rank == "BC")		//according to IGS value of 23k
		return 800;	

	QString buffer = rank;
	//buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
	buffer.replace(QRegExp("[pdk+?]"), "");
	int ordinal = buffer.toInt();
	unsigned int score;

	if(rank.contains("k"))
		score = (31 - ordinal) * 100;
	else if(rank.contains("d"))
		score = 3000 + (ordinal * 100);
	else if(rank.contains("p"))
		score = 3600 + (ordinal * 100);
	else
		return 0;
	if(rank.contains("?"))
		score--;
	return score;
}

/*
* on IGS, sends the 'nmatch'time, BY, handicap ranges
* command syntax : "nmatchrange 	BWN 	0-9 19-19	 60-60 		60-3600 	25-25 		0 0 0-0"
*				(B/W/ nigiri)	Hcp Sz	   Main time (secs)	BY time (secs)	BY stones	Koryo time
*/
void IGSConnection::sendNmatchParameters(void)
{
	//note that official client doesn't reload an nmatchrange cancel
	//until after you reconnect
	//who the hell knows
	return;
	QString c = "nmatchrange ";
	QSettings settings;

	c.append(preferences.nmatch_black ? "B" : "");
	c.append(preferences.nmatch_white ? "W" : "");
	c.append(preferences.nmatch_nigiri ? "N" : "");
	c.append(" 0-");
	c.append(preferences.nmatch_handicap);
	c.append(" ");
	c.append(QString::number(preferences.default_size));
	c.append("-19 ");
	
	c.append(QString::number(settings.value("DEFAULT_TIME").toInt()*60));
	c.append("-");
	c.append(QString::number(settings.value("NMATCH_MAIN_TIME").toInt()*60));
	c.append(" ");
	c.append(QString::number(settings.value("DEFAULT_BY").toInt()*60));
	c.append("-");
	c.append(QString::number(settings.value("NMATCH_BYO_TIME").toInt()*60));
	c.append(" 25-25 0 0 0-0\r\n");		//FIXME we need to get and send KORYO time as well, or do we
	/* If a line is used like below, match requests won't get to client but will be refused
	 * by server with a line like "wants Koryotime count 50 - 50 */
	//c.append(" 5-25 50 30 1-3\r\n");
	qDebug("nmatch string %s: ", c.toLatin1().constData());
	sendText(c);
}

#define IGS_LOGINMSG		0
#define IGS_PROMPT		1
#define IGS_BEEP		2
#define IGS_DOWN		4
#define IGS_ERROR		5
#define IGS_GAMES		7
#define IGS_FILE		8
#define IGS_INFO		9
#define IGS_KIBITZ		11
#define IGS_MESSAGES		14
#define IGS_MOVE		15
#define IGS_SAY			19
#define IGS_SCORE_M		20
#define IGS_SHOUT		21
#define IGS_STATUS		22
#define IGS_STORED		23
#define IGS_TELL		24
#define IGS_THIST		25
#define IGS_WHO			27
#define IGS_UNDO		28
#define IGS_YELL		32
#define IGS_AUTOMATCH		36
#define IGS_SERVERINFO		39
#define IGS_DOT			40
#define IGS_USERLIST		42
#define IGS_REMOVED		49
#define IGS_INGAMESAY		51
#define IGS_ADJOURNDECLINED	53
#define IGS_REVIEW		56
#define IGS_SEEK		63

void IGSConnection::handleMessage(QString msg)
{
	unsigned int type = 0;
	//qDebug(msg.toLatin1().constData());
	
	/*if(sscanf(msg.toLatin1().constData(), "%d", &type) != 1)
	{
		  qDebug("No number");
		  return;
	}*/
	//if(msg[0].toLatin1() == '\n')
		//return;
	if(msg[0].toLatin1() >= '0' && msg[0].toLatin1() <= '9')
	{
		type = (int)msg[0].toLatin1() - '0';
	}
	if(msg[1].toLatin1() >= '0' && msg[1].toLatin1() <= '9')
	{
		type *= 10;
		type += (int)msg[1].toLatin1() - '0';
	}
	
	if(needToSendClientToggle)
		onAuthenticationNegotiated();
	if(msg.contains("You have logged in as a guest"))	//WING, doesn't work, plus ugly
	{
		guestAccount = true;
        setState(PASSWORD_SENT);
	}
	if(!type)
	{	
		
		//line = line.remove(0, 2).trimmed();
		//if(msg.size() > 4)
		//	qDebug("***%02x %02x %02x %02x", msg[msg.size() - 1].toLatin1(), msg[msg.size() - 2].toLatin1(), msg[msg.size() -3].toLatin1(), msg[msg.size() -4].toLatin1());
		
		if(msg[3].toLatin1() == '1')
		{
			msg = msg.remove(0,2).trimmed();
			handle_prompt(msg);
			return;
		}
		if(msg.size() > 1)
		{
			//additional newline unnecessary  //0a0d
			if(msg[msg.size() - 1].toLatin1() == 0x0a)
				msg.remove(msg.size() - 2, msg.size()).trimmed();
		}
		if(console_dispatch && msg.size() > 2)
			console_dispatch->recvText(msg.toLatin1().constData());
		return;
	}
	//qDebug("***Type %d %c %c",type, msg[0].toLatin1(), msg[1].toLatin1());
	switch(type)
	{
		case IGS_LOGINMSG:
			//FIXME we never get here because !type returns
			handle_loginmsg(msg);
			break;
		case IGS_PROMPT:
			handle_prompt(msg);
			break;
		case IGS_BEEP:
			handle_beep(msg);
			break;
		case IGS_DOWN:
			handle_down(msg);
			break;
		case IGS_ERROR:
			handle_error(msg);
			break;
		case IGS_GAMES:
            handle_games(msg);
			break;
		case IGS_FILE:
			handle_file(msg);
			break;
		case IGS_INFO:
			handle_info(msg);
			break;
		case IGS_KIBITZ:
			handle_kibitz(msg);
			break;
		case IGS_MESSAGES:
			handle_messages(msg);
			break;
		case IGS_MOVE:
			handle_move(msg);
			break;
		case IGS_SAY:
			handle_say(msg);
			break;
		case IGS_SCORE_M:
			handle_score_m(msg);
			break;
		case IGS_SHOUT:
			handle_shout(msg);
			break;
		case IGS_STATUS:
			handle_status(msg);
			break;
		case IGS_STORED:
			handle_stored(msg);
			break;
		case IGS_TELL:
			handle_tell(msg);
			break;
		case IGS_THIST:
			handle_thist(msg);
			break;
		case IGS_WHO:
			handle_who(msg);
			break;
		case IGS_UNDO:
			handle_undo(msg);
			break;
		case IGS_YELL:
			handle_yell(msg);
			break;
		case IGS_AUTOMATCH:
			handle_automatch(msg);
			break;
		case IGS_SERVERINFO:
			handle_serverinfo(msg);
			break;
		case IGS_DOT:
			handle_dot(msg);
			break;
		case IGS_USERLIST:
			handle_userlist(msg);
			break;
		case IGS_REMOVED:
			handle_removed(msg);
			break;
		case IGS_INGAMESAY:
			handle_ingamesay(msg);
			break;
		case IGS_ADJOURNDECLINED:
			handle_adjourndeclined(msg);
			break;
		case IGS_REVIEW:
			handle_review(msg);
			break;
		case IGS_SEEK:
			handle_seek(msg);
			break;
	}
}

/* I think it would be neat if the QString was tokenized, just like with a compiler.  I mean basically there's
 * signed ints and signed floating points, and then there's a few strings, like names, and then everything else
 * is specific go related "keywords".  So we could process the msg before we pass it to the handler and then the
 * handlers wouldn't have any ugly text manipulation stuff... I really like this idea.  And then I think it would
 * become very clear the messages that require something from a previous message, like the 9s and 15s that we
 * had some issues with 
 * 
 * Actually, this reminds me a little of "element" in the old parser, except element only did delimited strings
 * The one question is whether to do QVariants or have our own simple variant like object.  I can imagine the
 * code looking like nested case statements.
 * 
 * The thing is, the original parser was kind of ugly, but at least it was consistent.  This, I'm not proud of
 * because its like the original ugly code, chopped up and made to work in a different form.  Its even uglier.
 cd* But I think I can make it nicer, and then the individual handlers just become case statements based on the type
 * of these element objects... we could call it... TBPToken text based protocol token... 
 * 
 * This is a good idea, but since IGS is basically all keywords, the point would just be to take all the string
 * manipulation out of the handlers and put it somewhere else where it could be ugly but consistent.
 * 
 * Even though this might ferret out some bugs, its still cosmetic and a big change so I'm going to hold off on it.
 * 
 * Now I'm thinking, instead of this also being a big list of string conditionals, we could instead have, much like
 * other protocols protocolcodes header files, we could have a list of strings and following values organized in some
 * sensible way.  So this would just go through the list and if it was an int or a float, it would go with that,
 * otherwise it would find the string from the list and go with that, or just create a string object at the end...*/

/* We want two functions, the tokenizer, and then something that's just a list of std::map adds 
 * the tokens can be an abstract class with the type within... I got to stop writing things and just
 * program them like I used to, but I shouldn't be working on this right now*/
 
 /* I'm having second thoughts about this idea.  First, it doesn't make anything faster.  We could try to remove
  * some of the regular expressions, that would make things a bit faster but that's hardly an issue anyway.
  * Second, it does take the text strings out of the function, but we just replace them with "IGS_" defines and a lot
  * of token handling and parsing stuff.  It might look kind of clean to have a list of text strings with their
  * associated defines in another file but... its a significant amount of effort and I'm not sure I can 
  * see how it would improve the code all that much.  I certainly like the idea... I could even return the multiple
  * functions to the original parser's giant case statement, except instead of hard coded numbers it would be defines
  * all over the place.  I think some of the game/move code and updates need to be fixed up but...
  * I guess this is just a really low priority.*/
#ifdef FIXME
returnsomekindoflist IGSConnection::tokenize(QString msg)
{

}
#endif //FIXME

/* IGS Protocol messages */
/* I know WING has these 0 messages... move to WING if specific FIXME*/
void IGSConnection::handle_loginmsg(QString line)
{
	/* These seem to have extra newlines on them */
	line.remove(line.length() - 1, line.length()).trimmed();
	if(line.length() > 1)
	{
		getConsoleDispatch()->recvText(line.toLatin1().constData());
	}
}

void IGSConnection::handle_prompt(QString line)
{
	line = line.remove(0, 2).trimmed();
	/* There's some other ones here, that might be an issue, besides 5 and 8,
	 * we should consider just calling onReady() here */
	if(line.contains("5") || line.contains("6") || line.contains("8"))
	{
	}
		onReady();
/* From NNGS source:
#define STAT_LOGON	0	// Unused
#define STAT_PASSWORD	1	// Unused
#define STAT_PASSWORD_NEW	2	// Unused
#define STAT_PASSWORD_CONFIRM	3	// Unused
#define STAT_REGISTER	4	// Unused
#define STAT_WAITING	5	
#define STAT_PLAYING_GO	6 
#define STAT_SCORING	7
#define STAT_OBSERVING	8	
#define STAT_TEACHING	9	// Unused
#define STAT_COMPLETE	10	// Unused
*/
}
		// BEEP
void IGSConnection::handle_beep(QString line)
{
	if (line.contains("Game saved"))
	{
#ifdef FIXME
		return IT_OTHER;
#endif //FIXME
	}
}

void IGSConnection::handle_down(QString line)
{
	//4 **** Server shutdown started by admin. ****
	if(line.contains("Server shutdown"))
	{
		getConsoleDispatch()->recvText("Server shutdown started by admin");
		qDebug("Closing connection in accordance with server shutdown");
		closeConnection();
	}
}
		// ERROR message
		//	5 Player "xxxx" is not open to match requests.
		//	5 You cannot observe a game that you are playing.
		//	5 You cannot undo in this game
		//	5 Opponent's client does not support undoplease
		//	5 noldo is currently involved in a match against someone else.
//	case 5:
void IGSConnection::handle_error(QString line)
{
	static QString memory_str;
	
	Room * room = getDefaultRoom();
	qDebug("error? %s", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();
	if(line.contains("Invalid password"))
	{
        setState(PASS_FAILED);
		getConsoleDispatch()->recvText(line.toLatin1().constData());
		//closeConnection();
		return;
	}
	else if(line.contains("string you have typed is illegal"))
	{
        setState(PROTOCOL_ERROR);
		getConsoleDispatch()->recvText(line.toLatin1().constData());
		//closeConnection();
		return;
	}
	else if (line.contains("No user named"))
	{
#ifdef FIXME
		QString name = element(line, 1, "\"");
//				//emit signal_talk(name, "@@@", true);
#endif //FIXME
	}
	else if (line.contains("is currently involved in a match") || 
		 line.contains("is playing a game") ||
		 line.contains("is involved in another game"))
	{
		QString opp;
		if(line.contains("is playing a game"))	//WING
			opp = element(line, 1, " ", "\"");
		else
			opp = element(line, 0, " ");
        PlayerListing * p = getPlayerListingNeverFail(opp);
        GameDialog * gameDialog = getGameDialog(p);
		gameDialog->recvRefuseMatch(GD_REFUSE_INGAME);
	}
	else if (line.contains("is not open to match requests"))
	{
		QString opp = element(line, 0, "\"", "\"");
        PlayerListing * p = getPlayerListingNeverFail(opp);
        GameDialog * gameDialog = getGameDialog(p);
		gameDialog->recvRefuseMatch(GD_REFUSE_NOTOPEN);
	}
	else if(line.contains("does not accept direct match"))
	{
		QString opp = element(line, 0, " ");//, " ");
        PlayerListing * p = getPlayerListingNeverFail(opp);
        GameDialog * gameDialog = getGameDialog(p);
		gameDialog->recvRefuseMatch(GD_REFUSE_NODIRECT);
	}
	else if (line.contains("player is currently not accepting matches"))
	{
				// IGS: 5 That player is currently not accepting matches.
				//GameDialog * gameDialog = getGameDialog(opp);
				//gameDialog->recvMatchRequest(0, 0);
		qDebug("player not currently accepting matches");
		/* There's no opponent from this message.  A 
		* message window called from the room makes sense.
		* Maybe even like a recvServerAlert function FIXME 
		* I'll just do this for now*/
		if(match_playerName.size())
		{
            PlayerListing * p = getPlayerListingNeverFail(match_playerName);
            GameDialog * gameDialog = getGameDialog(p);
			gameDialog->recvRefuseMatch(GD_REFUSE_NOTOPEN);
		}
	}
	else if(line.contains("Invalid parameters"))
	{
		if(match_playerName.size())
		{
            PlayerListing * p = getPlayerListingNeverFail(match_playerName);
            GameDialog * gameDialog = getGameDialog(p);
			gameDialog->recvRefuseMatch(GD_INVALID_PARAMETERS);
		}
	}
	//5 Opponent's client does not support nmatch.
	else if(line.contains("does not support nmatch"))
	{
		if(match_playerName.size())
		{
			PlayerListing * p = getPlayerListingNeverFail(match_playerName);
            GameDialog * gameDialog = getGameDialog(p);
			gameDialog->recvRefuseMatch(GD_OPP_NO_NMATCH);
		}
	}
	else if (line.contains("You cannot undo") || line.contains("client does not support undoplease")) 
	{
		BoardDispatch * boarddispatch = getIfBoardDispatch(game_were_playing);
		if(boarddispatch)
		{
			boarddispatch->recvKibitz(0, line);
			boarddispatch->getGameData()->undoAllowed = false;
		}
		//maybe should be a message box?  FIXME
	}
	else if(line.contains("There is no such game") || line.contains("Invalid game number"))
	{
		/* This comes from both observe and unobserve messages,
		 * meaning that we can't quite use it to remove gamelistings
		 * unless we fix up the protocol_save_int stuff a little
		 * which needs it anyhow. FIXME */
		//connection->protocol_save_int
	}
	else if (line.contains("Setting you open for matches"))
		room->recvToggle(0, true);

		// 5 There is a dispute regarding your nmatch:
		// 5 yfh2test request: B 3 19 420 900 25 0 0 0
		// 5 yfh22 request: W 3 19 600 900 25 0 0 0
	//
		// 5 There is a dispute regarding your match:
		// 5   yfh2test wants White on a 19x19 in 10 mins (10 byoyomi).
		// 5   eb5 wants Black on a 19x19 in 10 mins (12 byoyomi).
	
	else if(line.contains("wants Time"))
	{
		/* FIXME note that we should really look up their match conditions
		 * before even creating game dialog !!! */
		//5 lemon wants Time 60 - 60.
		//5 seinosuke wants Time 300 - 300
		QString opponent = element(line, 0, " ");
		PlayerListing * pl = getPlayerListingNeverFail(opponent);
		//QString timetochange = element(line, 2, " ");
        GameDialog * gameDialog = getGameDialog(pl);
		MatchRequest * m = gameDialog->getMatchRequest();
		MatchRequest * aMatch = new MatchRequest(*m);
		aMatch->maintime = element(line, 3, " ").toInt();
		aMatch->periodtime = element(line, 4, " ", ".").toInt();
		if(aMatch->periodtime > 299)
			aMatch->timeSystem = canadian;
		else
			aMatch->timeSystem = byoyomi;
		gameDialog->recvRequest(aMatch);
		gameDialog->recvRefuseMatch(GD_RESET);
	}
	else if(line.contains("wants Byomoves"))
	{
		//5 x wants Byomoves 1 - 1.
		QString opponent = element(line, 0, " ");
		PlayerListing * pl = getPlayerListingNeverFail(opponent);
		//QString timetochange = element(line, 2, " ");
        GameDialog * gameDialog = getGameDialog(pl);
		MatchRequest * m = gameDialog->getMatchRequest();
		MatchRequest * aMatch = new MatchRequest(*m);
		aMatch->stones_periods = element(line, 3, " ").toInt();
		gameDialog->recvRequest(aMatch);
		gameDialog->recvRefuseMatch(GD_RESET);
	}
	else if(line.contains("wants Byotime"))
	{
		//5 x wants Byomoves 1 - 1.
		QString opponent = element(line, 0, " ");
		PlayerListing * pl = getPlayerListingNeverFail(opponent);
		//QString timetochange = element(line, 2, " ");
        GameDialog * gameDialog = getGameDialog(pl);
		MatchRequest * m = gameDialog->getMatchRequest();
		MatchRequest * aMatch = new MatchRequest(*m);
		aMatch->periodtime = element(line, 3, " ").toInt();
		gameDialog->recvRequest(aMatch);
		gameDialog->recvRefuseMatch(GD_RESET);
	}
	else if(line.contains("wants Handicap"))
	{
		//5 x wants Handicap 0 - 0.
		QString opponent = element(line, 0, " ");
		PlayerListing * pl = getPlayerListingNeverFail(opponent);
        GameDialog * gameDialog = getGameDialog(pl);
		MatchRequest * m = gameDialog->getMatchRequest();
		MatchRequest * aMatch = new MatchRequest(*m);
		aMatch->handicap = element(line, 3, " ").toInt();
		gameDialog->recvRequest(aMatch);
		gameDialog->recvRefuseMatch(GD_RESET);
	}
	else if (line.contains("request:") || line.contains("wants"))
	{
		QString p = element(line, 0, " ");
		if (p == getUsername())
		{
			memory_str = line;
			return;
		}
				
		if (memory_str.contains(getUsername() + " request"))
		{
			memory_str = "";
			return;
		}
		MatchRequest * aMatch = 0;
		GameDialog * gameDialog = 0;
		PlayerListing * pl = 0;
		if(line.contains("wants"))	//match
		{
			if(line.contains("turn"))
			{
				pl = getPlayerListingNeverFail(p);
                gameDialog = getGameDialog(pl);
				MatchRequest * m = gameDialog->getMatchRequest();
				aMatch = new MatchRequest(*m);
				if(element(line, 3, " ") == "[B]")
				{
					aMatch->color_request = MatchRequest::BLACK;
				}	
				else if(element(line, 3, " ") == "[W]")
				{
					aMatch->color_request = MatchRequest::WHITE;
				}
				else
				{
					qDebug("Unknown: %s", line.toLatin1().constData());
					delete aMatch;
					return;
				}
			}
			else
			{
				aMatch = new MatchRequest();
				aMatch->opponent = p;
				aMatch->nmatch = false;
						
				if(element(line, 2, " ") == "Black")
					aMatch->color_request = MatchRequest::BLACK;
				else if(element(line, 2, " ") == "Nigiri")//IGS handles this??
					aMatch->color_request = MatchRequest::NIGIRI;
				else
					aMatch->color_request = MatchRequest::WHITE;
				QString s = element(line, 5, " ");
				
				aMatch->board_size = element(s, 0, "x").toInt();
				aMatch->maintime = element(line, 7, " ").toInt() * 60;
				s = element(line, 9, " ");
				aMatch->periodtime = element(s, 1, "(").toInt();
				aMatch->stones_periods = 25;	// I assume IGS assumes this is always 25
				if(getCurrentRoom()->name.contains("No rated"))
					aMatch->free_rated = FREE;
				else
					aMatch->free_rated = RATED;
			}
		}
		else		//nmatch
		{
			aMatch = new MatchRequest();
			aMatch->opponent = p;
			aMatch->nmatch = true;
			if(element(line, 2, " ") == "B")
				aMatch->color_request = MatchRequest::BLACK;
			else if(element(line, 2, " ") == "N")//IGS handles this??
				aMatch->color_request = MatchRequest::NIGIRI;
			else
				aMatch->color_request = MatchRequest::WHITE;
			//N 0 9 120 30 1 3 1 0

			aMatch->handicap = element(line, 3, " ").toInt();
			aMatch->board_size = element(line, 4, " ").toInt();
			aMatch->maintime = element(line, 5, " ").toInt();
			aMatch->periodtime = element(line, 6, " ").toInt();
			aMatch->stones_periods = element(line, 7, " ").toInt();
			if(aMatch->periodtime >= 300)
				aMatch->timeSystem = canadian;
			else
				aMatch->timeSystem = byoyomi;
			if(getCurrentRoom()->name.contains("No rated"))
				aMatch->free_rated = FREE;
			else
				aMatch->free_rated = RATED;
		}				

		if(!pl)		
			pl = getPlayerListingNeverFail(aMatch->opponent);
        const PlayerListing * us = getOurListing();
		if(us)
		{
			aMatch->our_name = us->name;
			aMatch->our_rank = us->rank;
		}
		aMatch->their_rank = pl->rank;
		
		if(!gameDialog)
            gameDialog = getGameDialog(pl);
		gameDialog->recvRequest(aMatch);
		delete aMatch;
	}
	else if(line.contains("is a private game"))
	{
		/* FIXME we need a dialog box saying that its a private game */
		int number = element(line, 2, " ").toInt();
		qDebug("%d is a private game", number);
	}
	else if(line.contains("Could not add you to the channel"))
	{
		//i.e., trying to join pro channel
		//actually should change back to the channel we were in probably
		//but to do that FIXME, we'd have to set that when we actually
		//entered a channel and I haven't seen a msg for that
		changeChannel("No Channel");
	}
	getConsoleDispatch()->recvText(line.toLatin1().constData());
}			
		// games
		// 7 [##] white name [ rk ] black name [ rk ] (Move size H Komi BY FR) (###)
		// 7 [41]      xxxx10 [ 1k*] vs.      xxxx12 [ 1k*] (295   19  0  5.5 12  I) (  1)
		// 7 [118]      larske [ 7k*] vs.      T08811 [ 7k*] (208   19  0  5.5 10  I) (  0)
		// 7 [255]          YK [ 7d*] vs.         SOJ [ 7d*] ( 56   19  0  5.5  4  I) ( 18)
		// 7 [42]    TetsuyaK [ 1k*] vs.       ezawa [ 1k*] ( 46   19  0  5.5  8  I) (  0)
		// 7 [237]       s2884 [ 3d*] vs.         csc [ 2d*] (123   19  0  0.5  6  I) (  0)
		// 7 [67]    atsukann [14k*] vs.    mitsuo45 [15k*] ( 99   19  0  0.5 10  I) (  0)
		// 7 [261]      lbeach [ 3k*] vs.    yurisuke [ 3k*] (194   19  0  5.5  3  I) (  0)
		// 7 [29]      ppmmuu [ 1k*] vs.       Natta [ 2k*] (141   19  0  0.5  2  I) (  0)
		// 7 [105]      Clarky [ 2k*] vs.       gaosg [ 2k*] ( 65   19  0  5.5 10  I) (  1)
	//case 7:
void IGSConnection::handle_games(QString line)
{
	if (line.contains("##"))
				// skip first line
        return;
    line.remove(QRegularExpression("[\\[\\]\\(\\)]"));
    QStringList words = line.split(QChar(' '),QString::SkipEmptyParts);
    bool ok;
    int number = words.at(1).toInt(&ok);
    if (!ok)
        return;
    GameListing * aGame = getDefaultRoom()->getGameListing(number);

    aGame->_white_name = words.at(2);
    aGame->_white_rank = words.at(3);
    fixRankString(&(aGame->_white_rank));
    // skip "vs."
    aGame->_black_name = words.at(5);
    aGame->_black_rank = words.at(6);
    fixRankString(&(aGame->_black_rank));
    aGame->moves = words.at(7).toInt();
    aGame->board_size = words.at(8).toInt();
    aGame->handicap = words.at(9).toInt();
    aGame->komi = words.at(10).toFloat();
    aGame->By = words.at(11);
    aGame->FR = words.at(12);
    aGame->observers = words.at(13).toInt();

    aGame->white = getPlayerListingNeverFail(aGame->_white_name);
    aGame->black = getPlayerListingNeverFail(aGame->_black_name);

	aGame->running = true;
#ifdef FIXME
	aGame->oneColorGo = false ;
#endif //FIXME

    emit gameListingReceived(aGame);
	if(protocol_save_string == "restoring")
	{
		BoardDispatch * boarddispatch = getBoardDispatch(number);
		/* This is ugly, we can get bad info here so we have
		 * to create a half record to send. FIXME*/
		GameData * aGameData = boarddispatch->getGameData();
		aGameData->number = aGame->number;
		aGameData->white_name = aGame->white_name();
		aGameData->black_name = aGame->black_name();
		aGameData->white_rank = aGame->white_rank();
		aGameData->black_rank = aGame->black_rank();
		aGameData->board_size = aGame->board_size;
		aGameData->handicap = aGame->handicap;
		aGameData->komi = aGame->komi;
		boarddispatch->openBoard();
		
		requestGameInfo(number);
		protocol_save_string = QString();
    }
}
		// "8 File"
//	case 8:
void IGSConnection::handle_file(QString line)
{
	qDebug("%s", line.toLatin1().constData());
#ifdef FIXME
	if (!memory_str.isEmpty() && memory_str.contains("File"))
	{
				// toggle
		memory_str = QString();
		memory = 0;
	}
	else if (memory != 0 && !memory_str.isEmpty() && memory_str == "CHANNEL")
	{
		//emit signal_channelinfo(memory, line);
		memory_str = QString();
	}

	else if (line.contains("File"))
	{
				// the following lines are help messages
		memory_str = line;
				// check if NNGS message cmd is active -> see '9 Messages:'
		if (memory != 14)
			memory = 8;
	}
#endif //FIXME
}
		// INFO: stats, channelinfo
		// NNGS, LGS: (NNGS new: 2nd line w/o number!)
		//	9 Channel 20 Topic: [xxx] don't pay your NIC bill and only get two players connected
		//	9  xxxx1 xxxx2 xxxx3 xxxx4 frosla
		//	9 Channel 49 Topic: Der deutsche Kanal (German)
		//	9  frosla
		//
		//	-->  channel 49
		//	9 Channel 49 turned on.
		//	9 Channel 49 Title:
		//
		//	9 guest has left channel 49.
		//
		//	9 Komi set to -3.5 in match 10
		// - in my game:
		// -   opponent:
		//      9 Komi is now set to -3.5
		// -   me:
		//      9 Set the komi to -3.5
		// NNGS, LGS:
		//	9 I suggest that ditto play White against made:
		//	9 For 19x19:  Play an even game and set komi to 1.5.
		//	9 For 13x13:  Play an even game and set komi to 6.5.
		//	9 For   9x9:  Play an even game and set komi to 4.5.
		// or:
		//	9 I suggest that pem play Black against eek:
		//	For 19x19:  Take 3 handicap stones and set komi to -2.5.
		//	For 13x13:  Take 1 handicap stones and set komi to -6.5.
		//	For   9x9:  Take 1 handicap stones and set komi to 0.5.
		// or:
		//	   I suggest that you play White against Cut:
		//
		//	9 Match [19x19] in 1 minutes requested with xxxx as White.
		//	9 Use <match xxxx B 19 1 10> or <decline xxxx> to respond.
		//
		//	9 Match [5] with guest17 in 1 accepted.
		//	9 Creating match [5] with guest17.
		//
		//	9 Requesting match in 10 min with frosla as Black.
		//	9 guest17 declines your request for a match.
		//	9 frosla withdraws the match offer.
		//
		//	9 You can check your score with the score command
		//	9 You can check your score with the score command, type 'done' when finished.
		//	9 Removing @ K8
		//
		//	9 Use adjourn to adjourn the game.
		//
		// NNGS: cmd 'user' NOT PARSED!
		//	9  Info     Name       Rank  19  9  Idle Rank Info
		//	9  -------- ---------- ---- --- --- ---- ---------------------------------
		//	9  Q  --  5 hhhsss      2k*   0   0  1s  NR                                    
		//	9     --  5 saturn      2k    0   0 18s  2k                                    
		//	9     --  6 change1     1k*   0   0  7s  NR                                    
		//	9 S   -- -- mikke       1d    0  18 30s  shodan in sweden                      
		//	9   X -- -- ksonney     NR    0   0 56s  NR                                    
		//	9     -- -- kou         6k*   0   0 23s  NR                                    
		//	9 SQ! -- -- GnuGo      11k*   0   0  5m  Estimation based on NNGS rating early 
		//	9   X -- -- Maurice     3k*   0   0 24s  2d at Hamilton Go Club, Canada; 3d in 
void IGSConnection::handle_info(QString line)
{
	static PlayerListing * statsPlayer;
	BoardDispatch * boarddispatch;
	Room * room = getDefaultRoom();
	static QString memory_str;
	static int memory = 0;
	//qDebug("9: %s", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();
			// status messages
	if (line.contains("Set open to be"))
	{
		bool val = (line.indexOf("False") == -1);
		room->recvToggle(0, val);
	}
	else if (line.contains("Setting you open for matches"))
		room->recvToggle(0, true);
	else if (line.contains("Set looking to be"))
	{
		bool val = (line.indexOf("False") == -1);
		room->recvToggle(1, val);
	}
			// 9 Set quiet to be False.
	else if (line.contains("Set quiet to be"))
	{
		bool val = (line.indexOf("False") == -1);
		room->recvToggle(2, val);
	}
	else if(line.contains("Welcome to the game room"))
	{
		QString buffer = element(line, 6, " ");
		int number = buffer.remove(0,1).toInt();
		qDebug("Joined room number %d", number);
		//FIXME, we need to reload the player
		//and games lists here
		//potentially something else does this? doublecheck
	}
	else if (line.indexOf("Channel") == 0) 
	{
#ifdef FIXME
				// channel messages
		QString e1 = element(line, 1, " ");
		if (e1.at(e1.length()-1) == ':')
			e1.truncate(e1.length()-1);
		int nr = e1.toInt();

		if (line.contains("turned on."))
		{
					// turn on channel
			emit signal_channelinfo(nr, QString("*on*"));
		}
		else if (line.contains("turned off."))
		{
					// turn off channel
			emit signal_channelinfo(nr, QString("*off*"));
		}
		else if (!line.contains("Title:") || gsName == GS_UNKNOWN)
		{
					// keep in memory to parse next line correct
			memory = nr;
			emit signal_channelinfo(memory, line);
			memory_str = "CHANNEL";
		}
#endif //FIXME
//				return IT_OTHER;
	}
#ifdef FIXME
	else if (memory != 0 && !memory_str.isEmpty() && memory_str == "CHANNEL")
	{
		//emit signal_channelinfo(memory, line);

				// reset memory
		memory = 0;
		memory_str = QString();
//				return IT_OTHER;
	}
#endif //FIXME
			// IGS: channelinfo
			// 9 #42 Title: Untitled -- Open
			// 9 #42    broesel    zero815     Granit
	else if (line.contains("#"))
	{
		QString msg = element(line, 0, " ", "EOL");
		
		if(msg.contains("Title:"))
		{
			channel = new ChannelListing();
			channel->number = element(line, 0, "#", " ").toInt();
			channel->name = element(line, 2, " ");
		}
		else if(channel)
		{
			//parse member list FIXME
			channel = 0;
		}
		if(channel)
			recvChannelListing(channel);
	}
			// NNGS: channels
	else if (line.contains("has left channel") || line.contains("has joined channel"))
	{
#ifdef FIXME
		QString e1 = element(line, 3, " ", ".");
		int nr = e1.toInt();

				// turn on channel to get full info
		emit signal_channelinfo(nr, QString("*on*"));
#endif //FIXME
	}
	else if (line.contains("Game is titled:"))
	{
#ifdef FIXME
		QString t = element(line, 0, ":", "EOL");
		emit signal_title(t);
#endif //FIXME
		return;
	}
	else if (line.contains("offers a new komi "))
	{
#ifdef FIXME
				// NNGS: 9 physician offers a new komi of 1.5.
		QString komi = element(line, 6, " ");
		if (komi.at(komi.length()-1) == '.')
			komi.truncate(komi.length() - 1);
		QString opponent = element(line, 0, " ");

				// true: request
		emit signal_komi(opponent, komi, true);
#endif //FIXME
	}
	else if (line.contains("Komi set to"))
	{
#ifdef FIXME
				// NNGS: 9 Komi set to -3.5 in match 10
		QString komi = element(line, 3, " ");
		QString game_id = element(line, 6, " ");

				// false: no request
		emit signal_komi(game_id, komi, false);
#endif //FIXME
	}
	else if (line.contains("wants the komi to be"))
	{
#ifdef FIXME
				// IGS: 9 qGoDev wants the komi to be  1.5
		QString komi = element(line, 6, " ");
		QString opponent = element(line, 0, " ");

				// true: request
		emit signal_komi(opponent, komi, true);
#endif //FIXME
	}
	else if (line.contains("Komi is now set to"))
	{
#ifdef FIXME
				// 9 Komi is now set to -3.5. -> oppenent set for our game
		QString komi = element(line, 5, " ");
				// error? "9 Komi is now set to -3.5.9 Komi is now set to -3.5"
		if (komi.contains(".9"))
			komi = komi.left(komi.length() - 2);

				// false: no request
		emit signal_komi(QString(), komi, false);
#endif //FIXME
	}
	else if (line.contains("Set the komi to"))
	{
#ifdef FIXME
				// NNGS: 9 Set the komi to -3.5 - I set for own game
		QString komi = element(line, 4, " ");

				// false: no request
		emit signal_komi(QString(), komi, false);
#endif //FIXME
	}
	else if (line.contains("game will count"))
	{
#ifdef FIXME
				// IGS: 9 Game will not count towards ratings.
				//      9 Game will count towards ratings.
		emit signal_freegame(false);
#endif //FIXME
	}
	else if (line.contains("game will not count", Qt::CaseInsensitive))
	{
#ifdef FIXME
				// IGS: 9 Game will not count towards ratings.
				//      9 Game will count towards ratings.
		emit signal_freegame(true);
#endif //FIXME
	}
	else if ((line.contains("[") || line.contains("yes")) && line.length() < 6)
	{
				// 9 [20] ... channelinfo
				// 9 yes  ... ayt
		return;
	}
	// WING has "as" instead of "has"
	else if (line.contains("as restarted your game") ||
			line.contains("has restored your old game"))
	{
		// FIXME, we probably need to set something up here, though
		// for instance with WING, its a 21 message that really
		// heralds things/info
		if (line.contains("restarted"))
					// memory_str -> see case 15 for continuation
			protocol_save_string = element(line, 0, " ");
	}
	else if (line.contains("I suggest that"))
	{
		memory_str = line;
		return;
	}
	else if (line.contains("and set komi to"))
	{
#ifdef FIXME
		//this might be NNGS, LGS only
				// suggest message ...
		if (!memory_str.isEmpty())
					// something went wrong...
			return;

		line = line.simplified();

		QString p1 = element(memory_str, 3, " ");
		QString p2 = element(memory_str, 6, " ", ":");
		bool p1_play_white = memory_str.contains("play White");

		QString h, k;
		if (line.contains("even game"))
			h = "0";
		else
			h = element(line, 3, " ");

		k = element(line, 9, " ", ".");

		int size = 19;
		if (line.contains("13x13"))
			size = 13;
		else if (line.contains("9x 9"))
		{
			size = 9;
			memory_str = QString();
		}
		if (p1_play_white)
			emit signal_suggest(p1, p2, h, k, size);
		else
			emit signal_suggest(p2, p1, h, k, size);
#endif //FIXME
		return;
	}
			// 9 Match [19x19] in 1 minutes requested with xxxx as White.
			// 9 Use <match xxxx B 19 1 10> or <decline xxxx> to respond.
			// 9 NMatch requested with yfh2test(B 3 19 60 600 25 0 0 0).
			// 9 Use <nmatch yfh2test B 3 19 60 600 25 0 0 0> or <decline yfh2test> to respond.
	else if (line.contains("<decline") && line.contains("match"))
	{
                // false -> not my request: used in mainwin.cpp
		line = element(line, 0, "<", ">");
		MatchRequest * aMatch = new MatchRequest();
		unsigned long flags = 0;
		/* All games are rated except for ones in free room on IGS */
		if(getCurrentRoom()->name.contains("No rated"))
			aMatch->free_rated = FREE;
		else
			aMatch->free_rated = RATED;
		aMatch->opponent = line.section(" ", 1, 1);
		if(line.contains("as White"))
			aMatch->color_request = MatchRequest::BLACK;
		else if(line.contains("as Black"))
			aMatch->color_request = MatchRequest::WHITE;
		else if(line.section(" ",2,2) == "B")
			aMatch->color_request = MatchRequest::BLACK;
		else if(line.section(" ",2,2) == "N")
			aMatch->color_request = MatchRequest::NIGIRI;
		else
			aMatch->color_request = MatchRequest::WHITE;
		if(line.contains("nmatch"))
		{
			aMatch->handicap = line.section(" ",3,3).toInt();
			aMatch->board_size = line.section(" ",4,4).toInt();
					//what kind of time? var name?
			//aMatch->maintime = line.section(" ",5,5).toInt();
			//aMatch->byoperiodtime = line.section(" ",6,6).toInt();
			//aMatch->byoperiods = line.section(" " ,7,7).toInt();
			flags = getGameDialogFlags();
			aMatch->maintime = line.section(" ",5,5).toInt();
			aMatch->periodtime = line.section(" ",6,6).toInt();
			aMatch->stones_periods = line.section(" " ,7,7).toInt();
			if(aMatch->periodtime < 300)
				aMatch->timeSystem = byoyomi;
			else
				aMatch->timeSystem = canadian;
			aMatch->nmatch = true;
		}
		else
		{
			aMatch->timeSystem = canadian;
			aMatch->board_size = line.section(" ",3,3).toInt();
			aMatch->maintime = line.section(" ",4,4).toInt() * 60;
			aMatch->periodtime = line.section(" ",5,5).toInt() * 60;
			aMatch->stones_periods = 25;
			flags |= GDF_CANADIAN;
			flags |= GDF_STONES25_FIXED;
			aMatch->nmatch = false;
		}
		PlayerListing * p = getPlayerListingNeverFail(aMatch->opponent);
        const PlayerListing * us = getOurListing();
		if(us)
		{	
			aMatch->our_name = us->name;
			aMatch->our_rank = us->rank;
		}
		aMatch->their_rank = p->rank;
		
        GameDialog * gameDialog = getGameDialog(p);
		gameDialog->recvRequest(aMatch, flags);
		delete aMatch;
	}
			// 9 Match [5] with guest17 in 1 accepted.
			// 9 Creating match [5] with guest17.
	else if (line.contains("Creating match"))
	{
		QString nr = element(line, 0, "[", "]");
				// maybe there's a blank within the brackets: ...[ 5]...
		QString dummy = element(line, 0, "]", ".").trimmed();
		QString opp = element(dummy, 1, " ");

				// We let the 15 game record message create the board
				//GameDialog * gameDialog = connection->getGameDialog(opp);
				//gameDialog->closeAndCreate();
				//GameDialog * gameDialog = 
				//		connection->getGameDialog(opp);
				//MatchRequest * mr = gameDialog->getMatchRequest();
				//created_match_request = new MatchRequest(*mr);
				//connection->closeGameDialog(opp);
				////emit signal_matchCreate(nr, opp);
        			// automatic opening of a dialog tab for further conversation
        			////emit signal_talk(opp, "", true);
	}
	else if (line.contains("Match") && line.contains("accepted"))
	{
		QString nr = element(line, 0, "[", "]");
		QString opp = element(line, 3, " ");
				////emit signal_matchCreate(nr, opp);
		/* FIXME, this is probably where we should be doing something? 
		 * assuming its reliable */
				
	}
			// 9 frosla withdraws the match offer.
			// 9 guest17 declines your request for a match.	
	else if (line.contains("declines your request for a match") ||
			line.contains("withdraws the match offer"))
	{
		QString opp = element(line, 0, " ");
		PlayerListing * p = getPlayerListingNeverFail(opp);
        GameDialog * gameDialog = getGameDialog(p);
		gameDialog->recvRefuseMatch(1);
	}
			//9 yfh2test declines undo
	else if (line.contains("declines undo"))
	{
		qDebug("%s", line.toLatin1().constData());	//FIXME
				// not the cleanest way : we should send this to a message box
		//emit signal_kibitz(0, element(line, 0, " "), line);
		return;
	}
		
			//9 yfh2test left this room
			//9 yfh2test entered this room
	else if (line.contains("this room"))
	{}	//emit signal_refresh(10);				

			//9 Requesting match in 10 min with frosla as Black.
	else if (line.contains("Requesting match in"))
	{
		QString opp = element(line, 6, " ");
		//emit signal_opponentopen(opp);
	}
			// NNGS: 9 Removing @ K8
			// IGS:	9 Removing @ B5
			//     49 Game 265 qGoDev is removing @ B5
	else if (line.contains("emoving @"))
	{
		/* FIXME DOUBLE CHECK!!! */
				/*if (gsName != IGS)
		{
		QString pt = element(line, 2, " ");
		//emit signal_reStones(pt, 0);
	}*/
		/* 49 handles this for IGS... but what about observed games? */
#ifdef FIXME
		
		if(protocol_save_int < 0)
		{
			qDebug("Received stone removal message without game in scoring mode");
			return;
		}
		else
		{
			QString pt = element(line, 2, " ");
			handleRemovingAt(protocol_save_int, pt);
			// FIXME Set protocol_save_int to -1???
		}
#endif //FIXME

	}
			// 9 You can check your score with the score command, type 'done' when finished.
	else if (line.contains("check your score with the score command"))
	{
//				if (gsName == IGS)
					// IGS: store and wait until game number is known
//					memory_str = QString("rmv@"); // -> continuation see 15
//				else
					////emit signal_restones(0, 0);
					////emit signal_enterScoreMode();
		qDebug("Getting boarddispatch from memory: %d", protocol_save_int);
		boarddispatch = getIfBoardDispatch(protocol_save_int);
		if(boarddispatch)
		{
			boarddispatch->recvEnterScoreMode();
			//connection->protocol_save_string = QString("rmv@");	
		}	
	}
			// IGS: 9 Board is restored to what it was when you started scoring
	else if (line.contains("what it was when you"))
	{
		//emit signal_restoreScore();
		qDebug("Getting boarddispatch from memory: %d", protocol_save_int);
		boarddispatch = getIfBoardDispatch(protocol_save_int);
		if(boarddispatch)
		{
			MoveRecord * move = new MoveRecord();
			move->flags = MoveRecord::UNDO_TERRITORY;
			boarddispatch->recvMove(move);
			delete move;
			boarddispatch->recvKibitz("", line);
		}
	}
	else if(line.contains("has typed done"))
	{
		qDebug("Getting boarddispatch from memory: %d", protocol_save_int);
		boarddispatch = getIfBoardDispatch(protocol_save_int);
		if(boarddispatch)
			boarddispatch->recvKibitz("", line);
	}
			// WING: 9 Use <adjourn> to adjourn, or <decline adjourn> to decline.
	else if (line.contains("Use adjourn to") || line.contains("Use <adjourn> to"))
	{
		////emit signal_requestDialog("adjourn", "decline adjourn", 0, 0);
		boarddispatch = getIfBoardDispatch(match_negotiation_state->getGameId());
		if(boarddispatch)
			boarddispatch->recvRequestAdjourn();
	}
			// 9 frosla requests to pause the game.
	else if (line.contains("requests to pause"))
	{
		//emit signal_requestDialog("pause", 0, 0, 0);
	}
	else if (line.contains("been adjourned"))
	{
				// re game from list - special case: own game
#ifdef FIXME
		aGame->nr = "@";
		aGame->running = false;

		//emit signal_game(aGame);
#endif //FIXME
		/* There's several after the fact server INFO messages about
		* games adjourning so we only need to close for one of them
		* The 21 message might actually be better FIXME*/
		boarddispatch = getIfBoardDispatch(protocol_save_int);
		if(boarddispatch)
		{
			qDebug("adjourning game!!");
			boarddispatch->adjournGame();
			closeBoardDispatch(protocol_save_int);
		}	
	}
			// 9 Game 22: frosla vs frosla has adjourned.
	else if (line.contains("has adjourned"))
	{
        int number = element(line, 0, " ", ":").toInt();
        GameListing * aGame = room->getGameListing(number);
		aGame->running = false;

				// for information
				//aGame->Sz = "has adjourned.";

#ifdef OLD
		//emit signal_game(aGame);
//				//emit signal_(aGame);
#endif //OLD
				// No need to get existing listing because
				// this is just to falsify the listing
		room->recvGameListing(aGame);
		/* Also notify board if we're watching */
		boarddispatch = getIfBoardDispatch(aGame->number);
		if(boarddispatch)
		{
			boarddispatch->adjournGame();
			closeBoardDispatch(aGame->number);
        }
	}
			// 9 Removing game 30 from observation list.
	else if (line.contains("from observation list"))
	{
				// is done from qGoIF
				// //emit signal_addToObservationList(-1);
//				aGame->nr = element(line, 2, " ").toInt();
//				aGame->Sz = "-";
//				aGame->running = false;
		//emit signal_observedGameClosed(element(line, 2, " ").toInt());
		return;
	}
			// 9 Adding game to observation list.
	else if (line.contains("to observation list"))
	{
		/* Unfortunately, LGS and IGS have no number here, so
		 * we have to either guess or get it from the observe send
		 * which is easiest */
		getBoardDispatch(protocol_save_int);
		protocol_save_int = -1;
		return;
	}
			// 9 Games currently being observed:  31, 36, 45.
	else if (line.contains("Games currently being observed"))
	{
		if (line.contains("None"))
		{
			//emit signal_addToObservationList(0);
		}
		else
		{
#ifdef FIXME
					// don't work correct at IGS!!!
			int i = line.count(',');

			qDebug(QString("observing %1 games").arg(i+1).toLatin1());
#endif //FIXME
//					//emit signal_addToObservationList(i+1);
		}

//				return IT_OTHER;
	}
			// 9 1 minutes were added to your opponents clock
	else if (line.contains("minutes were added"))
	{
		//boarddispatch = getIfBoardDispatch(protocol_save_int);
		//need to see this in action FIXME addTime
		//also check LGS/WING code
		int t = element(line, 0, " ").toInt();
		boarddispatch = getIfBoardDispatch(match_negotiation_state->getGameId());
		if(boarddispatch)
			boarddispatch->recvAddTime(t, boarddispatch->getOpponentName());

	}
			// 9 Your opponent has added 1 minutes to your clock.
	else if (line.contains("opponent has added"))
	{
		int t = element(line, 4, " ").toInt();
		boarddispatch = getIfBoardDispatch(match_negotiation_state->getGameId());
		if(boarddispatch)
			boarddispatch->recvAddTime(t, getUsername());
	}
			// NNGS: 9 Game clock paused. Use "unpause" to resume.
	else if (line.contains("Game clock paused"))
	{
#ifdef FIXME
		emit signal_timeAdded(-1, true);
#endif //FIXME
	}
			// NNGS: 9 Game clock resumed.
	else if (line.contains("Game clock resumed"))
	{
#ifdef FIXME
		emit signal_timeAdded(-1, false);
#endif //FIXME
	}
	// 9 Increase frosla's time by 1 minute
	else if (line.contains("s time by"))
	{
		//not sure how this works if we're observing the game FIXME
		int t = element(line, 4, " ").toInt();
		boarddispatch = getIfBoardDispatch(match_negotiation_state->getGameId());
		if(boarddispatch)
		{
			if (line.contains(getUsername()))
				boarddispatch->recvAddTime(t, getUsername());
			else
				boarddispatch->recvAddTime(t, boarddispatch->getOpponentName());
		}
	}
	// 9 Setting your . to Banana  [text] (idle: 0 minutes)
	else if (line.contains("Setting your . to"))
	{
		QString busy = element(line, 0, "[", "]");
		if (!busy.isEmpty())
		{
			QString player = element(line, 4, " ");
					// true = player
					////emit signal_talk(player, "[" + busy + "]", true);
		}
	}
	/* I'm pretty sure, as with LGS and WING, we want to
	 * update the game lists here but not use 9 messages
	 * for the board 
	 * 9 message maybe only score with IGS and if we can't
	 * reliably use 22... because of internal difficulties...*/
	else if (line.contains("resigns.")		||
			line.contains("adjourned.")	||
			//don't intefere with status UNLESS IGS (real solution is to
			// interpret status message and ignore here FIXME FIXME)
			line.contains(" : W ", Qt::CaseSensitive)	||
		   	line.contains(" : B ", Qt::CaseSensitive)	||
			line.contains("forfeits on")	||
			line.contains("lost by"))
	{
		GameResult * aGameResult;
        int number = element(line, 0, " ", ":").toInt();
        GameListing * aGame = room->getGameListing(number);
        aGame->running = false;
					// for information
		aGame->result = element(line, 4, " ", "}");

		boarddispatch = getIfBoardDispatch(aGame->number);
		/* FIXME: This shouldn't create a new board if
		* we're not watching it.
		* Also WING sometimes sends 9 and sometimes sends 21 perhaps
		* depending on whether its observed or resign versus score/result
		* we don't want to send double messages.*/
		if(boarddispatch)
		{
			aGameResult = new GameResult();
						
			if(line.contains(" : W ", Qt::CaseSensitive) ||
						line.contains(" : B ", Qt::CaseSensitive))
			{
				aGameResult->result = GameResult::SCORE;

				int posw, posb;
				posw = aGame->result.indexOf("W ");
				posb = aGame->result.indexOf("B ");
				bool wfirst = posw < posb;
				float sc1, sc2;
				sc1 = aGame->result.mid(posw+1, posb-posw-2).toFloat();
					//sc2 = aGame->result.right(aGame->result.length()-posb-1).toFloat();
				/* FIXME This may be right for IGS and wrong for WING */
					//sc2 = aGame->result.mid(posb +1, posb + 6).toFloat();
				sc2 = element(aGame->result, 4, " ").toFloat();
				qDebug("sc1: %f sc2: %f\n", sc1, sc2);
				if(sc1 > sc2)
				{
					aGameResult->winner_score = sc1;
					aGameResult->loser_score = sc2;
				}
				else
				{
					aGameResult->winner_score = sc2;
					aGameResult->loser_score = sc1;

				}

				if (!wfirst)
				{
					int h = posw;
					posw = posb;
					posb = h;
					if(sc2 > sc1)

						aGameResult->winner_color = stoneWhite;
					else
						aGameResult->winner_color = stoneBlack;
				}
				else
				{
					if(sc1 > sc2)
						aGameResult->winner_color = stoneWhite;
					else
						aGameResult->winner_color = stoneBlack;
				}


			}
			else if(line.contains("forfeits on time"))
			{
				aGameResult->result = GameResult::TIME;
				if(line.contains("Black"))
				{
					aGameResult->winner_color = stoneWhite;
				}
				else
				{
					aGameResult->winner_color = stoneBlack;
				}
			}
			else if(line.contains("resign", Qt::CaseInsensitive))
			{
				aGameResult->result = GameResult::RESIGN;
				if(line.contains("Black"))
				{
					aGameResult->winner_color = stoneWhite;
				}
				else
				{
					aGameResult->winner_color = stoneBlack;
				}
			}
			else if(line.contains("lost by"))
			{
				aGameResult->result = GameResult::SCORE;
				if(line.contains("Black"))
				{
					aGameResult->winner_color = stoneWhite;
				}
				else
				{
					aGameResult->winner_color = stoneBlack;

				}

			}
		}


		if (aGame->result.isEmpty())
			aGame->result = "-";
		else if (aGame->result.indexOf(":") != -1)
			aGame->result.remove(0,2);

		if(boarddispatch)
		{
			qDebug("Receiving result!!!");
			boarddispatch->recvResult(aGameResult);
			/* is kibitzing this here what we want?*/
			/* FIXME... should be a shortened list, not the full
			* result msg kibitz */
			/* More, looks like there is possible
			 * crash here if we already received a result
			 * and there's no good way to turn this off.
			 * FIXME All of the recvResult stuff needs
			 * to be cleaned up really */
			//boarddispatch->recvKibitz("", line);
		}
        room->recvGameListing(aGame);
		return;
	}
			// NNGS: 9 Messages: (after 'message' cmd)
			//       8 File
			//       <msg>
			//       8 File
			//       9 Please type "erase" to erase your messages after reading
	else if (line.contains("Messages:"))
	{
				// parse like IGS cmd nr 14: Messages
		memory = 14;
	}
			// IGS:  9 You have a message. To read your messages, type:  message
			// NNGS: 9 You have 1 messages.  Type "messages" to display them
	else if (line.contains("You have") && line.contains("messages"))
	{
				// re cmd nr
		line = line.trimmed();
		line = line.remove(0, 2);
		getConsoleDispatch()->recvText(line.toLatin1().constData());
		return;
	}
			// 9 Observing game  2 (chmeng vs. myao) :
			// 9        shanghai  9k*           henry 15k  
			// 9 Found 2 observers.
	else if (line.contains("Observing game ", Qt::CaseSensitive))
	{
				// right now: only need for observers of teaching game
				// game number
		bool ok;
		memory = element(line, 2, " ").toInt(&ok);
		if (ok)
		{
			memory_str = "observe";
					// FIXME
			//emit signal_clearObservers(memory); 
			BoardDispatch * boarddispatch = getIfBoardDispatch(memory);
			if(boarddispatch)
			{
				GameData * g = boarddispatch->getGameData();
				g->white_name = element(line, 0, "(", " ");
				g->black_name = element(line, 4, " ", ")");
				boarddispatch->gameDataChanged();
			}
			return;
		}
	}
	else if (!memory_str.isEmpty() && memory_str == "observe" && line.contains("."))
	{
//				QString cnt = element(line, 1, " ");
//				//emit signal_kibitz(memory, "00", "");

		memory = 0;
		memory_str = QString();

		return;
	}
	else if (!memory_str.isEmpty() && memory_str == "observe")
	{
		QString name =  element(line, 0, " ");
		QString rank;
		boarddispatch = getBoardDispatch(memory);
		if(!boarddispatch)
		{
			qDebug("No boarddispatch for observer list\n");
			return;
		}
				
		for (int i = 1; ! name.isEmpty(); i++)
		{
			rank = element(line, i, " ");
			fixRankString(&rank);
					// send as kibitz from "0"
			PlayerListing * p = getPlayerListingNeverFail(name);
			boarddispatch->recvObserver(p, true);
			name =  element(line, ++i , " ");
		}

		return;
	}
	else if(line.contains("Found") && line.contains("observers"))
	{
		memory = 0;
		memory_str = QString();
	}
	else if (line.contains("****") && line.contains("Players"))
	{
        return;
	}
			// 9 qGoDev has resigned the game.
	else if (line.contains("has resigned the game"))
	{
		if(!protocol_save_int)
		{
			qDebug("no memory for resign game message");
			return;
		}
		/* If this is our game, there's a 21 message maybe later that
		 * should be handling it FIXME */
		boarddispatch = getIfBoardDispatch(protocol_save_int);
		if(!boarddispatch)
		{
			qDebug("No board dispatch for \"resigned the game\"\n");
			return;
		}
#ifdef FIXME
		aGame->running = false;
#endif //FIXME
			
		GameResult * aGameResult = new GameResult();
		aGameResult->loser_name = element(line, 0, " ");
		aGameResult->result = GameResult::RESIGN;
		/* We need to set unknown/none color here */
		boarddispatch->recvResult(aGameResult);
		return;

	}
	else if	(line.contains("has run out of time"))
	{
		boarddispatch = getBoardDispatch(protocol_save_int);
		if(!boarddispatch)
		{
			qDebug("No board dispatch for \"has run out of time\"\n");
			return;
		}
#ifdef FIXME
		aGame->running = false;
#endif //FIXME
			
		GameResult * aGameResult = new GameResult();
		aGameResult->loser_name = element(line, 0, " ");
		aGameResult->result = GameResult::TIME;
		/* We need to set unknown/none color here */
		boarddispatch->recvResult(aGameResult);
		return;

	}



		//9 Player:      yfh2
		//9 Game:        go (1)
		//9 Language:    default
		//9 Rating:      6k*  23
		//9 Rated Games:     21
		//9 Rank:  8k  21
		//9 Wins:        13
		//9 Losses:      16
		//9 Idle Time:  (On server) 0s
		//9 Address:  yfh2@tiscali.fr
		//9 Country:  France
		//9 Reg date: Tue Nov 18 04:01:05 2003
		//9 Info:  yfh2
		//9 Defaults (help defs):  time 0, size 0, byo-yomi time 0, byo-yomi stones 0
		//9 Verbose  Bell  Quiet  Shout  Automail  Open  Looking  Client  Kibitz  Chatter
		//9     Off    On     On     On        On   Off      Off      On      On   On

	else if (line.contains("Player:"))
	{
		//statsPlayer = new PlayerListing();
		QString name = element(line, 1, " ");
		statsPlayer = getPlayerListingNeverFail(name);
#ifdef FIXME
				/* So this would have cleared the structure, but
		* we're just creating a new empty object later.
		* One HUGE FIXME question is what we might be
		* overriding on the other side.  Maybe we need
		* a bit vector specifying what has updated when
		* the message is passed.  So the notion before
		* was that the structure was held until
				* completely filled and then passed up*/
				
		statsPlayer->extInfo = "";
		statsPlayer->won = "";
		statsPlayer->lost = "";
		statsPlayer->country = "";
		statsPlayer->nmatch_settings = "";
		statsPlayer->rank = "";
		statsPlayer->info = "";
		statsPlayer->address = "";
		statsPlayer->play_str = "";
		statsPlayer->obs_str = "";
		statsPlayer->idle = "";
		statsPlayer->rated = "";
				// not sure it is the best way : above code seem to make use of "signal"
				// but we don't need this apparently for handling stats
#endif //FIXME
		protocol_save_string = "STATS";
		return;
	}
			
	else if (line.contains("Address:"))
	{
		statsPlayer->email_address = element(line, 1, " ");
		return;
	}
			
	else if (line.contains("Last Access"))
	{
		statsPlayer->idletime = element(line, 4, " ")+ " " + element(line, 5, " ")+" " + element(line, 6, " ");
		statsPlayer->seconds_idle = idleTimeToSeconds(statsPlayer->idletime);
		return;
	}
				
	else if (line.contains("Rating:"))
	{
		statsPlayer->rank = element(line, 1, " ");
		fixRankString(&(statsPlayer->rank));
		statsPlayer->rank_score = rankToScore(statsPlayer->rank);
		return;
	}
			
	else if (line.contains("Wins:"))
	{
		statsPlayer->wins = element(line, 1, " ").toInt();
		return;         
	}
				
	else if (line.contains("Losses:"))
	{
		statsPlayer->losses = element(line, 1, " ").toInt();
		return;  
	}
			
	else if ((line.contains("Country:"))||(line.contains("From:")))   //IGS || LGS
	{
		statsPlayer->country = element(line, 0, " ","EOL");
		return; 
	}
			
	else if (line.contains("Defaults"))    //IGS
	{
		statsPlayer->extInfo = element(line, 2, " ","EOL");
        Talk * talk = getDefaultRoom()->getTalk(statsPlayer);
		if(talk)
			talk->updatePlayerListing();
		statsPlayer = 0;
		return; 
	}
	else if (line.contains("User Level:")) //LGS
	{
		statsPlayer->extInfo = line;
		return;
	}
			
				
	else if ((line.contains("Info:"))&& !(line.contains("Rank Info:")))
	{
#ifdef FIXME
		if (! statsPlayer->info.isEmpty())
			statsPlayer->info.append("\n");
		statsPlayer->info.append(line);
		//emit signal_statsPlayer(statsPlayer);
#endif //FIXME
		return;          
	}
			
	else if (line.contains("Playing in game:"))       //IGS and LGS
	{
		statsPlayer->playing = element(line, 3, " ").toInt();

		return;
	}				
	else if (line.contains("Rated Games:"))
	{
		statsPlayer->rated_games = element(line, 2, " ").toInt();
		return;
	}
	else if (line.contains("Idle Time:"))
	{
		statsPlayer->idletime = element(line, 4, " ");
		statsPlayer->seconds_idle = idleTimeToSeconds(statsPlayer->idletime);
		return;
	}
			
	else if (line.contains("Last Access"))
	{
		statsPlayer->idletime = "not on";
		return;
	}
				
	else if (line.left(5) == "19x19")     //LGS syntax
	{
		statsPlayer->rank = element(line, 4, " ");
		fixRankString(&(statsPlayer->rank));
		statsPlayer->rank_score = rankToScore(statsPlayer->rank);
		statsPlayer->rated_games = element(line, 7, " ").toInt();
		return;
	}
			
	else if (line.contains("Wins/Losses"))     //LGS syntax
	{
		statsPlayer->wins = element(line, 2, " ").toInt();
		statsPlayer->losses = element(line, 4, " ").toInt();
		return;
	}
	else if(line.contains("Off") || line.contains("On"))
	{
		/* Scratch this, we're claiming "Defaults" is the last line */
				/*Talk * talk = connection->getTalk(statsPlayer->name);
		if(talk)
		talk->recvPlayerListing(statsPlayer);
					//talk->recvTalk(e2);

		delete statsPlayer;
		statsPlayer = 0;*/
	}
				
		//9 ROOM 01: FREE GAME ROOM;PSMNYACF;自由対局室;19;1;10;1,19,60,600,30,25,10,60,0
		//9 ROOM 10: SLOW GAME ROOM;PSMNYACF;ｽﾛｰ対局室;19;1;20
		//9 ROOM 92: PANDA OPEN ROOM;X;月例大会;19;10;15
	else if (! line.left(5).compare("ROOM "))
	{
		QString room = element(line, 0, " ",";");
		RoomListing * r = new RoomListing(room.section(":", 0, 0).toInt(),
						  room.section(": ", 1, 1));
		r->locked = (element(line, 1,";")=="X") || (element(line, 1,";")=="P");
		recvRoomListing(r);
		//dont delete r
		return;
	}
	else if(line.contains("File"))
		return;
	if (protocol_save_string != "STATS")	//FIXME
		getConsoleDispatch()->recvText(line.toLatin1().constData());
}

// 11 Kibitz Achim [ 3d*]: Game TELRUZU vs Anacci [379]
// 11    will B resign?
//case 11:
void IGSConnection::handle_kibitz(QString line)
{
	static QString memory_str;
	static int memory = 0;
	line = line.remove(0, 2).trimmed();
	BoardDispatch * boarddispatch;
	if (line.contains("Kibitz"))
	{
				// who is kibitzer
		memory_str = element(line, 0, " ", ":");
				// game number
		memory = element(line, 1, "[", "]").toInt();
	}
	else
	{
		if (memory_str.isEmpty())
					// something went wrong...
			return;

		//emit signal_kibitz(memory, memory_str, line);
		boarddispatch = getBoardDispatch(memory);
		if(boarddispatch)
			boarddispatch->recvKibitz(memory_str, line);
		memory = 0;
		memory_str.clear();
	}
}

	// messages
		// 14 File
		// frosla 11/15/02 14:00: Hallo
		// 14 File
void IGSConnection::handle_messages(QString line)
{
	qDebug("%s", line.toLatin1().constData());
#ifdef FIXME	
		//case 14:
	if (!memory_str.isEmpty() && memory_str.contains("File"))
	{
				// toggle
		memory_str = QString();
		memory = 0;
	}
	else if (line.contains("File"))
	{
				// the following lines are help messages
		memory_str = line;
		memory = 14;
	}
#endif //FIXME
};

		// MOVE
		// 15 Game 43 I: xxxx (4 223 16) vs yyyy (5 59 13)
		// 15 TIME:21:lowlow(W): 1 0/60 479/600 14/25 0/0 0/0 0/0
		// 15 TIME:442:MIYASAN(W): 1 0/60 18/30 0/1 10/10 0/60 0/0
		// 15 144(B): B12
		// IGS: teaching game:
		// 15 Game 167 I: qGoDev (0 0 -1) vs qGoDev (0 0 -1)
void IGSConnection::handle_move(QString line)
{
	BoardDispatch * boarddispatch;
		//case 15:
    qDebug("%s", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();	
	static bool need_time = false;	
	int number;
	QString white, black;
	//console->recvText(line.toLatin1().constData());
	/* I think we'll need game_number for scores, so I'm going to
	 * have it on the connection->protocol_saved_int since
	 * hopefully anything that uses it will use it immediately afterward
	 * with no other server msg inbetween */
	//static int game_number = -1;
			//qDebug("Game_number: %d\n", game_number);
	if (line.contains("Game"))
	{		
		number = element(line, 1, " ").toInt();
		white = element(line, 3, " ");
		black = element(line, 8, " ");
		/* Check if we're reloading the game */
		/* Other problem, with IGS, this might be all we get
		* for a restarted game Is this okay here?  Maybe doesn't
		* always get called?  It could have a listing already.*/
		boarddispatch = getIfBoardDispatch(number);
		if(!boarddispatch)
		{
			
			if(white == getUsername() || black == getUsername())
			{
				if(protocol_save_string == white || protocol_save_string == black)
				{
					qDebug("starting to restore");
					requestGameStats(number);
					/* Stats will pick up the info and create the board, otherwise
					* boardsize can get lost, etc. */
					protocol_save_string = "restoring";
					protocol_save_int = number;
					game_were_playing = number;
				}
				else
				{
					/* accept? */
					protocol_save_string = QString();
					/* This code should be unified from that from the other case, msg 9
					* that does this same thing, maybe the 9 should be removed.. FIXME */
					QString opp = (white == getUsername() ? black : white);
					if((white == getUsername() || black == getUsername()) && !getBoardFromOurOpponent(opp))
					{
						PlayerListing * p = getPlayerListingNeverFail(opp);
                        MatchRequest * mr = getAndCloseGameDialog(p);
						if(mr)
						{
							boarddispatch = getBoardDispatch(number);
							GameData * aGameData = boarddispatch->getGameData();
							
							aGameData->number = number;
							aGameData->white_name = white;
							aGameData->black_name = black;
							aGameData->board_size = mr->board_size;
							aGameData->komi = mr->komi;
							aGameData->handicap = mr->handicap;
							aGameData->maintime = mr->maintime;
							aGameData->periodtime = mr->periodtime;
							aGameData->stones_periods = mr->stones_periods;
							aGameData->undoAllowed = true;
							if(aGameData->white_name == getUsername())
							{
								aGameData->white_rank = mr->our_rank;
								aGameData->black_rank = mr->their_rank;
							}
							else
							{
								aGameData->white_rank = mr->their_rank;
								aGameData->black_rank = mr->our_rank;
							}
							boarddispatch->openBoard();
						}
						else
						{
							PlayerListing * blackPlayer = getPlayerListingNeverFail(black);
							PlayerListing * whitePlayer = getPlayerListingNeverFail(white);
							boarddispatch = getBoardDispatch(number);
							GameData * aGameData = boarddispatch->getGameData();
							aGameData->number = number;
							aGameData->white_name = white;
							aGameData->black_name = black;
							aGameData->black_rank = blackPlayer->rank;
							aGameData->white_rank = whitePlayer->rank;
							aGameData->undoAllowed = true;
							// Could be from seek opponent !!!
							
							boarddispatch->openBoard();
							need_time = true;
						}
						protocol_save_int = number;
						game_were_playing = number;
					}
					/* We probably just accepted a match */
						
				}
				match_negotiation_state->setGameId(number);
                    //requestGameInfo(game_number);
				return;
			}
			else
			{
				/* In order to allow observing by console
				 * This all needs to be cleaned up,
				 * FIXME, like the above should be moved below, I think
				 * we only added it haphazardly because we were half working
				 * with the old code and half making allowances for things
				 * like restores.*/
				/* One FIXME major issue though, probably why
				 * we added that thing to sendObserve in the first
				 * place is because if we do this here, then closing
				 * a board isn't enough, if it hits at the wrong
				 * time, a move can pop the board back open again
				 * and then nothing more. */
				if(match_negotiation_state->sentJoinRoom())		//so no observing by console
					boarddispatch = getBoardDispatch(number);
			}
			//return;
		}
		
		GameData * aGameData = boarddispatch->getGameData();
		aGameData->number = number;

		protocol_save_int = number;
				//aGameData->type = element(line, 1, " ", ":");
		
		aGameData->white_prisoners = element(line, 0, "(", " ").toInt();
		wtime->time = element(line, 5, " ").toInt();
		wtime->stones_periods = element(line, 5, " ", ")").toInt();
		
		aGameData->black_prisoners = element(line, 1, "(", " ").toInt();
		btime->time = element(line, 10, " ").toInt();
		btime->stones_periods = element(line, 10, " ", ")").toInt();
		/* FIXME Doublecheck in WING */
				/* Is this a new game? 
		* This is this ugly, convoluted way of checking, but I guess
		* it gets around the IGS protocol, maybe I'll think of another way later.
		* We can use the game listing for the s later, but here, there may
		* not be a game listing if its a new game.  This may be the listing, in
				* a sense. */
		/* Another issue here is that we do not want to call this when
		 * restoring a game since there really isn't a gamedialog... FIXME */
		//openBoard obviously has to be done first
		boarddispatch->openBoard();
		boarddispatch->recvTime(*wtime, *btime);
		boarddispatch->gameDataChanged();
	}
	else if (line.contains("TIME"))
	{	
		number = element(line, 0, ":",":").toInt();
		// FIXME Does WING have these messages???
		// Might not need game record here!!!
		
		boarddispatch = getIfBoardDispatch(number);
		if(!boarddispatch)
		{
			qDebug("TIME message received but no board");
			return;
		}
		protocol_save_int = number;
		
		GameData * aGameData = boarddispatch->getGameData();
#ifdef FIXME
		aGameData->mv_col = "T";
#endif //FIXME
		aGameData->number = number;
		QString time1 = element(line, 1, " ","/");
		QString time2 = element(line, 2, " ","/");
		QString stones = element(line, 3, " ","/");				

		if(need_time)
		{
			need_time = false;
			/* This is if we get here from a seek game FIXME
			 * it would obviously make sense to standardize this
			 * somehow but the protocol isn't too consistent
			 * not to mention the official client having
			 * occasional bugs so... */
			if(1)	//FIXME when you see a byoyomi game here
			{
				aGameData->timeSystem = canadian;
				aGameData->maintime = time1.toInt();
				aGameData->periodtime = time2.toInt();
				aGameData->stones_periods = stones.toInt();
			}
			else
			{
				aGameData->timeSystem = byoyomi;
			}
		}
		if (line.contains("(W)"))
		{
			aGameData->white_name = element(line, 1, ":","(");
			wtime->time = (time1.toInt()==0 ? time2 : time1).toInt();
			wtime->stones_periods = (time1.toInt()==0 ?stones: "-1").toInt();
		}
		else if (line.contains("(B)"))					
		{
			aGameData->black_name = element(line, 1, ":","(");
			btime->time = (time1.toInt()==0 ? time2 : time1).toInt();
			btime->stones_periods = (time1.toInt()==0 ? stones:"-1").toInt();
			/* IGS seems to do W and then B, each time */
			boarddispatch->recvTime(*wtime, *btime);
		}
		else //never know with IGS ...
			return;
		
		//boarddispatch->recvTime(*wtime, *btime);
		//gameDataChanged necessary if above white_name, black_name is issue
		//FIXME
		//boarddispatch->gameDataChanged();
	}
	else if (line.contains("GAMERPROPS"))
	{
		GameData * gd;
		int game_number = element(line, 0, ":",":").toInt();	
		
		BoardDispatch * boarddispatch = getIfBoardDispatch(game_number);
		if(boarddispatch)
		{
			gd = boarddispatch->getGameData();
			/* We need this for komi in our own games sometimes it seems 
			 * or do we... */
			//GAMERPROPS:86: 9 0 6.50
			gd->board_size = element(line, 1, " ").toInt();
			gd->handicap = element(line, 2, " ").toInt();
			gd->komi = element(line, 3, " ").toFloat();
		}
		return;
	}
	else if(protocol_save_int < 0)
	{
		qDebug("Ignoring boardless move");
		return;
	}
	else
	{
		boarddispatch = getIfBoardDispatch(protocol_save_int);
		if(!boarddispatch)
		{
			qDebug("Possible move messages received but no board dispatch");
			return;
		}
		/* If we're getting moves that we can use, then
		* they have some existing board, which means a boardrecord
		* which should be more reliable than the listing */
		GameData * r  = boarddispatch->getGameData();
		//GameListing * l = room->getGameListing(protocol_save_int);
		if(!r)
		{
			qDebug("Move for unlisted game");
			return;
		}
		
		/* Not sure what undos look like anymore FIXME */
		
		/* If there's multiple moves on a line, the ones
		 * after the first are captures */
		MoveRecord * aMove = new MoveRecord();
		aMove->flags = MoveRecord::NONE;
		aMove->number = element(line, 0, "(").toInt();
		QString point = element(line, 0, " ", "EOL");
		if(aMove->number == 0)
			r->move_list_received = true;
		else if(!r->move_list_received)
		{
			delete aMove;
			return;
		}
		if(point.contains("Handicap", Qt::CaseInsensitive))
		{
			/* As long as handicap is
			* set in game data... actually
			* this is useful since sending
			*  0 sets tree properly*/
			aMove->flags = MoveRecord::HANDICAP;
			aMove->x = element(line, 1, " ", "EOL").toInt();
			qDebug("handicap %d", aMove->x);
			/*if(aMove->x != 0)
			{
				GameData * r  = boarddispatch->getGameData();
				r->handicap = aMove->x;
				boarddispatch->gameDataChanged();
			}*/
			/* Don't want to set this here because it will
			 * prevent the handicap from being set here.
			 * something in boardwindow, etc., code has to
			 * prevent double setting the handicap so its
			 * !handicap*/
		}
		else if(point.contains("Pass", Qt::CaseInsensitive))
		{
			aMove->flags = MoveRecord::PASS;
		}
		else
		{
			if(!r->handicap)
				aMove->number++;
			//qDebug("board size from record: %d", r->board_size);
			aMove->x = (int)(point.toLatin1().at(0));
			aMove->x -= 'A';
			point.remove(0,1);
			//qDebug("move number: %d\n", aMove->number);
			aMove->y = element(point, 0, " ").toInt();
					
			if(aMove->x < 9)	//no I on IGS
				aMove->x++;
			//if(l->board_size > 9)
			//{	
				aMove->y = r->board_size + 1 - aMove->y;
			//}
			//qDebug("%d %d\n", aMove->x, aMove->y);
			if(element(line, 0, "(", ")") == "W")
				aMove->color = stoneWhite; 
			else
				aMove->color = stoneBlack; 
		}
		/* Any other color options ??? */
		boarddispatch->recvMove(aMove);
		delete aMove;
		return;
	}
	//ugly, redundant but IGS doesn't give us a good way to know when
	//to do this otherwise
	//if(boarddispatch)
	//	boarddispatch->openBoard();
}
		// SAY
		// 19 *xxxx*: whish you a nice game  :)
		// NNGS - new:
		//  19 --> frosla hallo
void IGSConnection::handle_say(QString line)
{
	BoardDispatch * boarddispatch;
	line = line.remove(0, 2).trimmed();
		//case 19:
//			if (line.contains("-->"))
//				//emit signal_kibitz(0, 0, element(line, 1, " ", "EOL"));
//			else
	boarddispatch = getBoardFromOurOpponent(element(line, 0, "*", "*"));	
	if(boarddispatch)
		boarddispatch->recvKibitz(element(line, 0, "*", "*"), element(line, 0, ":", "EOL"));
	else
		qDebug("Received kibitz without board for player");
			
}	

/* This is tricky but since 21 reports game status anyway,
 * for observed and played, we don't really need 20. */
void IGSConnection::handle_score_m(QString line)
{
		//20 yfh2 (W:O): 4.5 to NaiWei (B:#): 4.0
		//case 20:
	//qDebug("Ignoring IGS_score_m message");
	//return;
	/* we seem to need this message, especially if its our game */
	GameResult * aGameResult = new GameResult();
	BoardDispatch * boarddispatch;
	bool firstname;
//			aGame->nr = "@";
//			aGame->running = false;
	QString res;
	line = line.remove(0, 2).trimmed();
	QString player = element(line, 0, " ");
	if (player == getUsername())
	{
		player = element(line, 4, " ");
		firstname = 1;
	}
	else
		firstname = 0;
	aGameResult->winner_name = player;
	aGameResult->result = GameResult::SCORE;
	boarddispatch = getBoardFromOurOpponent(player);
	if(!boarddispatch)
	{
		qDebug("Can't find board for result message!\n");
		return;
	}

	aGameResult->winner_score = element(line, 2, " ").toFloat();
	aGameResult->loser_score = element(line, 6, " ").toFloat();
	if(aGameResult->winner_score > aGameResult->loser_score)
	{
		aGameResult->winner_color = stoneWhite;
		if(firstname)
			aGameResult->winner_name = getUsername();
	}
	else
	{
		float temp = aGameResult->winner_score;
		aGameResult->winner_score = aGameResult->loser_score;
		aGameResult->loser_score = temp;
		aGameResult->winner_color = stoneBlack;
		if(!firstname)
			aGameResult->winner_name = getUsername();
	}
	boarddispatch->recvResult(aGameResult);
	game_were_playing = 0;
	delete aGameResult;
}	

/* Note that we get shouts on games and players in other rooms,
 * either that, or only the lobby.  Find out which.  */	
// SHOUT - a info from the server
//case 21:
void IGSConnection::handle_shout(QString line)
{
    Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	line = line.remove(0, 2).trimmed();
	
			// case sensitive
	if (line.contains(" connected.}"))
	{
				// {guest1381 [NR ] has connected.}
				//line.replace(QRegExp(" "), "");
		QString name = element(line, 0, "{", " ");
        PlayerListing * aPlayer = room->getPlayerListing(name);
		aPlayer->rank = element(line, 0, "[", "]", true);
		fixRankString(&(aPlayer->rank));
		aPlayer->rank_score = rankToScore(aPlayer->rank);
		aPlayer->info = "??";
		aPlayer->playing = 0;
		aPlayer->observing = 0;
		aPlayer->idletime = "-";
		aPlayer->online = true;
        emit playerListingReceived(aPlayer);
        return;
	}
	else if (line.contains("has disconnected"))
	{
				// {xxxx has disconnected}
        PlayerListing * aPlayer = room->getPlayerListing(element(line, 0, "{", " "));
        aPlayer->online = false;
        emit playerListingReceived(aPlayer);
        return;
	}
	else if (line.contains("{Game"))
	{
				// {Game 198: xxxx1 vs xxxx2 @ Move 33}
		if (line.contains("@"))
		{
			/* FIXME: This may mean that an old game was just restarted
			 * and so we might want to set it up here */
			/* We also get this message at weird times that makes
			 * boards we're not watching pop up!! FIXME */
			qDebug("Opening up this game... restoring?");
			
			int game_number = element(line, 0, " ", ":").toInt();
			QString white = element(line, 2, " ");
			QString black = element(line, 4, " ");

			if(white == getUsername() || black == getUsername())
			{
				if(protocol_save_string == white || protocol_save_string == black)
				{
					/* Then this is a restarted game, create
					 * board dispatch */
					//getBoardDispatch(game_number);
				
				//boarddispatch = getBoardDispatch(aGame->nr);
				//boarddispatch->recvGameListing(aGame);
				requestGameStats(game_number);
				/* Stats will pick up the info and create the board, otherwise
				 * boardsize can get lost, etc. */
				protocol_save_string = "restoring";
				}
				else
					protocol_save_string = QString();
                //requestGameInfo(game_number);
			}
			return;
		}

				// {Game 155: xxxx vs yyyy has adjourned.}
				// {Game 76: xxxx vs yyyy : W 62.5 B 93.0}
				// {Game 173: xxxx vs yyyy : White forfeits on time.}
				// {Game 116: xxxx17 vs yyyy49 : Black resigns.}
				// IGS:
				// 21 {Game 124: Redmond* vs NaiWei* : Black lost by Resign}
				// 21 {Game 184: Redmond* vs NaiWei* : White lost by 1.0}
                /* This results in double kibitz on LGS, maybe we drop it
		 * altogether? */
		if (line.contains("resigns.")		||
				  line.contains("adjourned.")	||
				  line.contains(" : W ", Qt::CaseSensitive)	||
				  line.contains(" : B ", Qt::CaseSensitive)	||
				  line.contains("forfeits on")	||
				  line.contains("lost by"))
		{
			GameResult * aGameResult;
            int number = element(line, 0, " ", ":").toInt();
			GameListing * aGame = room->getGameListing(number);
            aGame->running = false;
					// for information
			aGame->result = element(line, 4, " ", "}");

			boarddispatch = getIfBoardDispatch(aGame->number);
			if(boarddispatch)
			{
				aGameResult = new GameResult();
						
				if(line.contains(" : W ", Qt::CaseSensitive) ||
							       line.contains(" : B ", Qt::CaseSensitive))
				{
					aGameResult->result = GameResult::SCORE;

					int posw, posb;
					posw = aGame->result.indexOf("W ");
					posb = aGame->result.indexOf("B ");
					bool wfirst = posw < posb;
					float sc1, sc2;
					sc1 = aGame->result.mid(posw+1, posb-posw-2).toFloat();
					//sc2 = aGame->result.right(aGame->result.length()-posb-1).toFloat();
					/* FIXME This may be right for IGS and wrong for WING */
					//sc2 = aGame->result.mid(posb +1, posb + 6).toFloat();
					sc2 = element(aGame->result, 4, " ").toFloat();
					qDebug("sc1: %f sc2: %f\n", sc1, sc2);
					if(sc1 > sc2)
					{
						aGameResult->winner_score = sc1;
						aGameResult->loser_score = sc2;
					}
					else
					{
						aGameResult->winner_score = sc2;
						aGameResult->loser_score = sc1;

					}

					if (!wfirst)
					{
						int h = posw;
						posw = posb;
						posb = h;
						if(sc2 > sc1)

							aGameResult->winner_color = stoneWhite;
						else
							aGameResult->winner_color = stoneBlack;
					}
					else
					{
						if(sc1 > sc2)
							aGameResult->winner_color = stoneWhite;
						else
							aGameResult->winner_color = stoneBlack;
					}
				}
				else if(line.contains("forfeits on time"))
				{
					aGameResult->result = GameResult::TIME;
					if(line.contains("Black"))
					{
						aGameResult->winner_color = stoneWhite;
					}
					else
					{
						aGameResult->winner_color = stoneBlack;
					}
				}
				else if(line.contains("resign", Qt::CaseInsensitive))
				{
					aGameResult->result = GameResult::RESIGN;
					if(line.contains("Black"))
					{
						aGameResult->winner_color = stoneWhite;
					}
					else
					{
						aGameResult->winner_color = stoneBlack;
					}
				}
				else if(line.contains("lost by"))
				{
					aGameResult->result = GameResult::SCORE;
					if(line.contains("Black"))
					{
						aGameResult->winner_color = stoneWhite;
					}
					else
					{
						aGameResult->winner_color = stoneBlack;

					}

				}
				else if(line.contains("adjourned"))
				{
					/* A little ugly to have this and return here
					 * but if it works with protocol... */
					boarddispatch->adjournGame();
					closeBoardDispatch(aGame->number);
					delete aGameResult;
					return;
				}
			}


			if (aGame->result.isEmpty())
				aGame->result = "-";
			else if (aGame->result.indexOf(":") != -1)
				aGame->result.remove(0,2);

			if(boarddispatch)
			{
				qDebug("Receiving 21 result!!!");
				/* This can be redundant with 21 */
				boarddispatch->recvResult(aGameResult);
				delete aGameResult; //conditional on boarddispatch!
				/* is kibitzing this here what we want?*/
				boarddispatch->recvKibitz("", line);
			}
			room->recvGameListing(aGame);
			return;
		}
	}
	else if (line.contains("{Match"))
	{
				// {Match 116: xxxx [19k*] vs. yyyy1 [18k*] }
				// {116:xxxx[19k*]yyyy1[18k*]}
				// WING: {Match 41: o4641 [10k*] vs. Urashima [11k*] H:2 Komi:3.5}
        qDebug() << "IGSConnection::handle_shout() " << line;
		line.replace(QRegExp("vs. "), "");
		line.replace(QRegExp("Match "), "");
		line.replace(QRegExp(" "), "");
		int number = element(line, 0, "{", ":").toInt();
		
        GameListing * aGame = room->getGameListing(number);
        /* No reason to wait for player listing since info is here */
		QString name = element(line, 0, ":", "[");
		aGame->white = getPlayerListingNeverFail(name);
		aGame->_white_name = element(line, 0, ":", "[");
		aGame->_white_rank = element(line, 0, "[", "]");
		fixRankString(&(aGame->_white_rank));
		aGame->_white_rank_score = rankToScore(aGame->_white_rank);
		name = element(line, 0, "]", "[");
		aGame->black = getPlayerListingNeverFail(name);
		aGame->_black_name = element(line, 0, "]", "[");
		aGame->_black_rank = element(line, 1, "[", "]");
		fixRankString(&(aGame->_black_rank));
		aGame->_black_rank_score = rankToScore(aGame->_black_rank);
				
		aGame->moves = 0;	//right???
				//aGame->s = "-";
				//aGame->Sz = "-";
				//aGame->handicap = QString();
		aGame->running = true;
				
#ifdef OLD
		if (gsName == WING && aGame->wname == aGame->bname)
					// WING doesn't send 'create match' msg in case of teaching game
			//emit signal_matchCreate(aGame->nr, aGame->bname);

		//emit signal_game(aGame);
#endif //OLD
        room->recvGameListing(aGame);
		return;
	}
	else if(line.contains("Match"))
	{
		//Match 220 gogal gogal
		//what is this?  a request?
	}
			// !xxxx!: a game anyone ?
	else if (line.contains("!: "))
	{
		QString player = element(line, 0, "!");
		//emit signal_shout(player, line);
		return;
	}
	getConsoleDispatch()->recvText(line.toLatin1().constData());
}

void IGSConnection::handle_status(QString line)
{
	qDebug("%s", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();
		// CURRENT GAME STATUS
		// 22 Pinkie  3d* 21 218 9 T 5.5 0
		// 22 aura  3d* 24 276 16 T 5.5 0
		// 22  0: 4441000555033055001
		// 22  1: 1441100000011000011
		// 22  2: 4141410105013010144
		// 22  3: 1411411100113011114
		// 22  4: 1131111111100001444
		// 22  5: 0100010001005011111
		// 22  6: 0055000101105000010
		// 22  7: 0050011110055555000
		// 22  8: 1005000141005055010
		// 22  9: 1100113114105500011
		// 22 10: 1111411414105010141
		// 22 11: 4144101101100111114
		// 22 12: 1411300103100000144
		// 22 13: 1100005000101001144
		// 22 14: 1050505550101011144
		// 22 15: 0505505001111001444
		// 22 16: 0050050111441011444
		// 22 17: 5550550001410001144
		// 22 18: 5555000111411300144
		//case 22:
			/* Status messages are a bit screwy:
	* the columns are actually the y axis on the board
	* and the rows the x axis, so for instance on our
	* board p19 would appear on their board as 0,14
	* In addition, some things are marked wrong like
	* false eyes that are surrounded by stones are
	* marked as territory when they shouldn't be, but
	* that's not our problem.
	* 0 is a black stone, 1 is a white stone, 3 is
	* dame (no territory (did I get term right?)) 4 is
			* white territory, 5 is black territory */
	static QString player = "";
	static int cap;
	static float komi;
	static BoardDispatch * statusDispatch;
	if (!line.contains(":"))
	{
		if(player == "")
		{
			player = element(line, 0, " ");
			cap = element(line, 2, " ").toInt();
			komi = element(line, 6, " ").toFloat();
		}
#ifdef FIXME
		else if(protocol_save_int > -1)
		{
			MoveRecord * aMove = new MoveRecord();
					/* This could be a huge problem,
			* but we're going to assume that
			* if we get one of these messages
			* and we're scoring a game, it means
					* scoring is done */
			/* FIXME might not be necessary if below attrib handles IF */
			statusDispatch = getIfBoardDispatch(protocol_save_int);
			if(statusDispatch)
			{
				aMove->flags = MoveRecord::DONE_SCORING;
				statusDispatch->recvMove(aMove);
			}
			player = "";
			delete aMove;
		}
#endif //FIXME
		else
		{
			statusDispatch = getBoardFromAttrib(element(line, 0, " "), element(line, 2, " ").toInt(), element(line, 6, " ").toFloat(), player, cap, komi);
			if(!statusDispatch)
			{
				// FIXME this happens an awful lot.  I think
				// every time we play a game, though perhaps
				// observing games is okay... anyway, either
				// we need to change something so that we don't
				// even check here, or this doesn't warrant
				// a debug message
				qDebug("No status board found!!!\n");
				player = "";
				return;
			}
			statusDispatch->recvEnterScoreMode();
			player = "";
		}
	}
	else
	{
		if(!statusDispatch)
			return;
		int row = element(line, 0, ":").toInt();
		QString results = element(line, 1, " ");
		/* This might be slower than it needs to
		 * be but...  and hardcoding board,
		 * I'd like to fix this, but it would require... 
		 * like a MoveRecord->next kind of pointer so that
		 * we could send whole lists at once.  But is that
		 * worth it?  Seems like an ultimately useless
		 * functionality. */
		/* X and Y are swapped below */
		MoveRecord * aMove = new MoveRecord();
		aMove->x = row + 1;		
		aMove->flags = MoveRecord::TERRITORY;
        for(int column = 1; column <= results.length(); column++)
		{
			aMove->y = column;
			if(results.at(column - 1) == '4')
				aMove->color = stoneWhite;
			else if(results.at(column - 1) == '5')
				aMove->color = stoneBlack;
			else
				continue;
			statusDispatch->recvMove(aMove);
		}
		
		/* FIXME If there's no more moves, and there's no result
		 * on the game (like an earlier resign at the same time? etc.,
		 * before it?  Then we need to recvResult here */
        if(row + 1 == results.length())
		{
			/* Since its a square board */
			aMove->flags = MoveRecord::DONE_SCORING;
			statusDispatch->recvMove(aMove);
		}
		delete aMove;
	}

//			//emit signal_message(line);
}

void IGSConnection::handle_stored(QString line)
{
	qDebug("%s", line.toLatin1().constData());
		// STORED
		// 9 Stored games for frosla:
		// 23           frosla-physician
//TODO		case 23:
//TODO			break;
}

void IGSConnection::handle_tell(QString line)
{
	line = line.remove(0, 2).trimmed();
		//  24 *xxxx*: CLIENT: <cgoban 1.9.12> match xxxx wants handicap 0, komi 5.5, free
		//  24 *jirong*: CLIENT: <cgoban 1.9.12> match jirong wants handicap 0, komi 0.5
    		//  24 *SYSTEM*: shoei canceled the nmatch request.
		//  24 *SYSTEM*: xxx requests undo.

		// NNGS - new version:
		//  24 --> frosla CLIENT: <qGo 0.0.15b7> match frosla wants handicap 0, komi 0.5, free
		//  24 --> frosla  Hallo
		//case 24:
		//{
	qDebug("24: %s", line.toLatin1().constData());
	int pos;
	BoardDispatch * boarddispatch;
#ifdef FIXME
	if (((((pos = line.indexOf("*")) != -1) && (pos < 3)) || 
		      (((pos = line.indexOf("-->")) != -1) && (pos < 3) &&
		      line.contains("CLIENT:"))))
	{

		line = line.simplified();
		QString opp = element(line, 1, "*");
		int offset = 0;
		if (opp.isEmpty())
		{
			offset++;
			opp = element(line, 1, " ");
		}

		QString h = element(line, 7+offset, " ", ",");
		QString k = element(line, 10+offset, " ");

		if (k.at(k.length()-1) == ',')
			k.truncate(k.length() - 1);
		int komi = (int) (k.toFloat() * 10);

		bool free;
		if (line.contains("free"))
			free = true;
		else
			free = false;

		emit signal_komirequest(opp, h.toInt(), komi, free);
		getConsoleDispatch()->recvText(line.toLatin1().constData());

		// it's tell, but don't open a window for that kind of message...
		return;
	}
#endif //FIXME
      
      			//check for cancelled game offer
	if (line.contains("*SYSTEM*"))
	{
		QString opp = element(line, 1, " ");
        PlayerListing * p = getPlayerListingNeverFail(opp);
		line = line.remove(0,10);
		getConsoleDispatch()->recvText(line.toLatin1().constData());

		if  (line.contains("canceled") &&  line.contains("match request"))
		{	
					/* FIXME We need to close any open gamedialog boxes
			* pertaining to this opponent.  We'll assume
					* one is created I guess */	
			
            GameDialog * gameDialog = getGameDialog(p);
			gameDialog->recvRefuseMatch(2);
					////emit signal_matchCanceled(opp);
		}
		if  (line.contains("requests undo"))
		{
			boarddispatch = getBoardDispatch(protocol_save_int);
			if(!boarddispatch)
			{
				printf("No boarddispatch for undo message");
				return;
			}
			MoveRecord * aMove = new MoveRecord();
			aMove->flags = MoveRecord::REQUESTUNDO;
			boarddispatch->recvMove(aMove);
			delete aMove;
					////emit signal_requestDialog("undo","noundo",0,opp);
		}
		return;
	}
      
			// check for NNGS type of msg
	QString e1,e2;
	if ((pos = line.indexOf("-->")) != -1 && pos < 3)
	{
		e1 = element(line, 1, " ");
				//e2 = "> " + element(line, 1, " ", "EOL").trimmed();
		e2 = element(line, 1, " ", "EOL").trimmed();
	}
	else
	{
		e1 = element(line, 0, "*", "*");
				//e2 = "> " + element(line, 0, ":", "EOL").trimmed();
		e2 = element(line, 0, ":", "EOL").trimmed();
	}

	/* FIXME, what's weird here is that this is usually where you get "Thanks for game"
	 * not in the board window and I'm not sure that's appropriate. */
			// //emit player + message + true (=player)
			////emit signal_talk(e1, e2, true);
	PlayerListing * p = getPlayerListingNeverFail(e1);
    Talk * talk = getDefaultRoom()->getIfTalk(p);
	if(!talk)
	{
        sendStatsRequest(p);
        talk = getDefaultRoom()->getTalk(p);
	}
	if(talk)
		talk->recvTalk(e2);
}


		// results
		//25 File
		//curio      [ 5d*](W) : lllgolll   [ 4d*](B) H 0 K  0.5 19x19 W+Resign 22-04-47 R
		//curio      [ 5d*](W) : was        [ 4d*](B) H 0 K  0.5 19x19 W+Time 22-05-06 R
		//25 File
		//case 25:
void IGSConnection::handle_thist(QString line)
{
	//FIXME if this is something that happens then
	// we should be picking it up
	qDebug("%s", line.toLatin1().constData());
}
		// who
		// 27  Info Name Idle Rank | Info Name Idle Rank
		// 27  SX --   -- xxxx03      1m     NR  |  Q! --   -- xxxx101    33s     NR  
		// 0   4 6    11  15         26     33   38 41
		// 27     --  221 DAISUKEY   33s     8k  |   X172   -- hiyocco     2m    19k*
		// 27  Q  --  216 Saiden      7s     1k* |     --   53 toshiao    11s    10k 
		// 27     48   -- kyouji      3m    11k* |     --   95 bengi       5s     4d*
		// IGS:
		//        --   -- kimisa      1m     2k* |  Q  --  206 takabo     45s     1k*
		//      X 39   -- Marin       5m     2k* |     --   53 KT713      18s     2d*
		//        --   34 mat21       2m    14k* |     --    9 entropy    28s     4d 
		// NNGS:
		// 27   X --   -- arndt      21s     5k  |   X --   -- biogeek    58s    20k   
		// 27   X --   -- buffel     21s     4k  |  S  --    5 frosla     12s     7k   
		// 27  S  --   -- GoBot       1m     NR  |     --    5 guest17     0s     NR   
		// 27     --    3 hama        3s    15k  |     --   -- niki        5m     NR   
		// 27   ! --   -- viking4    55s    19k* |  S  --    3 GnuGo       6s    14k*  
		// 27   X --   -- kossa      21m     5k  |   X --   -- leif        5m     3k*  
		// 27     --    6 ppp        18s     2k* |     --    6 chmeng     20s     1d*  
		// 27                 ********  14 Players 3 Total Games ********

		// WING:
		// 0        9    14                      38      46   51
		// 27   X 13   -- takeo6      1m     2k  |   ! 26   -- ooide       6m     2k*  
		// 27   ! --   -- hide1234    9m     2k* |  QX --   -- yochi       0s     2k*  
		// 27   g --   86 karasu     45s     2k* |  Sg --   23 denakechi  16s     1k*  
		// 27   g 23   43 kH03       11s     1k* |   g --   43 kazusige   24s     1k*  
		// 27   g --   50 jan        24s     1k* |  QX --   -- maigo      32s     1d   
		// 27   g --   105 kume1       5s     1d* |   g --   30 yasumitu   24s     1d   
		// 27  Qf --   13 motono      2m     1d* |   X 13   -- tak7       57s     1d*  
		// 27   ! 50   -- okiek       8m     1d* |   X --   -- DrO        11s     1d*  
		// 27   f --   103 hiratake   35s     8k  |   g --   33 kushinn    21s     8k*
		// 27   g --   102 teacup      1m     1d* |   g --   102 Tadao      32s     1d*  
		//case 27:
void IGSConnection::handle_who(QString line)
{
    PlayerListing * aPlayer = NULL;
	Room * room = getDefaultRoom();
	int pos;
    if (line.contains("****") && line.contains("Players"))
		return;
    if (line.contains("Info") && line.contains("Idle"))
        return;

		if (line[15] != ' ')
		{
//27   X --   -- truetest    7m     1d* |   X --   -- aajjoo      6s      9k
					// parse line
			QString info = line.mid(4,2);
			int observing = line.mid(6,3).toInt();
			int playing = line.mid(11,3).trimmed().toInt();
			QString name = line.mid(15,11).trimmed();
            aPlayer = room->getPlayerListing(name);
			aPlayer->online = true;
			aPlayer->info = info;
			aPlayer->observing = observing;
			aPlayer->playing = playing;
			aPlayer->idletime = line.mid(26,3);
			aPlayer->seconds_idle = idleTimeToSeconds(aPlayer->idletime);
					//aPlayer->nmatch = false;
			if (line[33] == ' ')
			{
				if (line[36] == ' ')
					aPlayer->rank = line.mid(34,2);
				else
					aPlayer->rank = line.mid(34,3);
				fixRankString(&(aPlayer->rank));
				aPlayer->rank_score = rankToScore(aPlayer->rank);
			}
			else
			{
				if (line[36] == ' ')
					aPlayer->rank = line.mid(33,3);
				else
					aPlayer->rank = line.mid(33,4);
				fixRankString(&(aPlayer->rank));
				aPlayer->rank_score = rankToScore(aPlayer->rank);
			}
					
					// check if line ok, true -> cmd "players" preceded
            emit playerListingReceived(aPlayer);
		}
#ifdef FIXME
		else
			qDebug("player27 dropped (1): %s" + line.toLatin1());
#endif //FIXME

				// position of delimiter between two players
		pos = line.indexOf('|');
				// check if 2nd player in a line && player (name) exists
		if (pos != -1 && line[52] != ' ')
		{
					// parse line
			QString info = line.mid(41,2);
			int observing = line.mid(43,3).toInt();
			int playing = line.mid(48,3).trimmed().toInt();
			QString name = line.mid(52,11).trimmed();
            aPlayer = room->getPlayerListing(name);
			aPlayer->online = true;
			aPlayer->info = info;
			aPlayer->observing = observing;
			aPlayer->playing = playing;
			aPlayer->idletime = line.mid(63,3);
			aPlayer->seconds_idle = idleTimeToSeconds(aPlayer->idletime);
					//aPlayer->nmatch = false;
			if (line[70] == ' ')
			{
				if (line[73] == ' ')
					aPlayer->rank = line.mid(71,2);
				else
					aPlayer->rank = line.mid(71,3);
				fixRankString(&(aPlayer->rank));
				aPlayer->rank_score = rankToScore(aPlayer->rank);
			}
			else
			{
				if (line[73] == ' ')
					aPlayer->rank = line.mid(70,3);
				else
					aPlayer->rank = line.mid(70,4);
				fixRankString(&(aPlayer->rank));
				aPlayer->rank_score = rankToScore(aPlayer->rank);
			}

					// true -> cmd "players" preceded
            emit playerListingReceived(aPlayer);
        }
#ifdef FIXME
		else
			qDebug("player27 dropped (2): %s" + line.toLatin1());
#endif //FIXME
}

void IGSConnection::handle_undo(QString line)
		// 28 guest17 undid the last  (J16).
		// 15 Game 7 I: frosla (0 5363 -1) vs guest17 (0 5393 -1)
		// 15   2(B): F17
		// IGS:
		// 28 Undo in game 64: MyMaster vs MyMaster:  N15 
		//case 28:
{
	BoardDispatch * boarddispatch;
	qDebug("*** %s", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();
	if (line.contains("undid the last "))
	{
				// now: just look in qgo_interface if _nr has decreased...
				// but send undo-signal anyway: in case of undo while scoring it's necessary
		MoveRecord * aMove = new MoveRecord();
		aMove->flags = MoveRecord::UNDO;
		QString player = element(line, 0, " ");
		QString point = element(line, 0, "(", ")");

		aMove->x = (int)(point.toLatin1().at(0));
		aMove->x -= 'A';
		if(aMove->x < 9)	//no I on IGS
			aMove->x++;
		point.remove(0,1);
		aMove->y = element(point, 0, " ").toInt();
		/* Do we need the board size here???*/
		aMove->y = 20 - aMove->y;

		boarddispatch = getBoardFromOurOpponent(player);
		boarddispatch->recvMove(aMove);
		delete aMove;
	}
	else if (line.contains("Undo in game"))
	{
		MoveRecord * aMove = new MoveRecord();
		aMove->flags = MoveRecord::UNDO;
		QString nr = element(line, 3, " ");
		nr.truncate(nr.length() - 1);
		QString point = element(line, 7, " ");
		aMove->x = (int)(point.toLatin1().at(0));
		aMove->x -= 'A';
		if(aMove->x < 9)	//no I on IGS
			aMove->x++;
		point.remove(0,1);
		aMove->y = element(point, 0, " ").toInt();
		/* Do we need the board size here???*/
		aMove->y = 20 - aMove->y;
		boarddispatch = getBoardDispatch(nr.toInt());		
		boarddispatch->recvMove(aMove);
		delete aMove;
	}

			// message anyway
	getConsoleDispatch()->recvText(line.toLatin1().constData());
}

void IGSConnection::handle_yell(QString line)
		// different from shout??!?!?
		// IGS
		// -->  ; \20
		// 32 Changing into channel 20.
		// 32 Welcome to cyberspace.
		//
		// 32 49:qgodev: Title is now: (qgodev) qGo development
		// 32 20:qGoDev: Person joining channel
		// 32 20:frosla: hi
		// 32 20:qGoDev: Person leaving channel

		//
		// 1 5
		// -->  channels
		// 9 #1 Title: Professional Channel -- Open
		// 
		// 9 #20 Title: Untitled -- Open
		// 9 #20     frosla
		// #> 23 till: hello all  <-- this is what the members in channel 23 see:
		//			(channel number and who sent the message)
		//
		// NNGS:
		// channel talk: "49:xxxx: hi"
		//case 32:
{
	/* As far as I can tell, the channel info stuff was
	 * never hooked up to anything.  The talk should be
	 * hooked up to something, but this is a FIXME*/
	QString e1,e2;
	qDebug("%s\n", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();
	if (line.contains("Changing into channel"))
	{
#ifdef FIXME
		e1 = element(line, 2, " ",".");
		int nr = e1.toInt();//element(line, 3, " ").toInt();
		emit signal_channelinfo(nr, QString("*on*"));
//				//emit signal_talk(e1, "", false);
				////emit signal_message(line);
#endif //FIXME
		return;
	}
	else if (line.contains("Welcome to cyberspace"))
	{
				////emit signal_message(line);
		getConsoleDispatch()->recvText(line.toLatin1().constData());
		return;
	}
	else if (line.contains("Person joining channel"))
	{
#ifdef FIXME
		int nr = element(line, 0, ":").toInt();
		emit signal_channelinfo(nr, QString("*on*"));
#endif //FIXME
		return;
	}
	else if (line.contains("Person leaving channel"))
	{
#ifdef FIXME
		int nr = element(line, 0, ":").toInt();
		emit signal_channelinfo(nr, QString("*on*"));
#endif //FIXME
		return;
	}

			// //emit (channel number, player + message , false =channel )
	/*switch (gsName)
	{
		case IGS:*/
			e1=element(line, 0, ":");
			e2="> " + element(line, 0, ":", "EOL").trimmed();
			e1 += e2;
			getConsoleDispatch()->recvText(e1.toLatin1().constData());
	/*		break;

		default:
			e1=element(line, 0, ":");
			e2="> " + element(line, 0, ":", "EOL").trimmed();
			e1 += e2;
			break;
	} FIXME */

//			//emit signal_talk(e1, e2, false);
}

/* 36 
36 terra1 wants a match with you:
36 terra1 wants 19x19 in 1 minutes with 10 byo-yomi and 25 byo-stones
36 To accept match type 'automatch terra1'
*/	
void IGSConnection::handle_automatch(QString line)
{
	line = line.remove(0, 2).trimmed();
	if(line.contains("minutes"))
	{
		Room * room = getDefaultRoom(); 
		MatchRequest * aMatch = new MatchRequest();
		aMatch->opponent = element(line, 0, " ");
		
		qDebug("Receieved automatch request from %s", aMatch->opponent.toLatin1().constData());
		QString bs = element(line, 2, " ");
		if(bs == "19x19")
			aMatch->board_size = 19;
		else if(bs == "13x13")
			aMatch->board_size = 13;
		else if(bs == "9x9")
			aMatch->board_size = 9;
		else
		{
			qDebug("Match offered board size: %s", bs.toLatin1().constData());
			delete aMatch;
			return;
		}
		aMatch->maintime = element(line, 4, " ").toInt();
		aMatch->periodtime = element(line, 7, " ").toInt();
		aMatch->stones_periods = element(line, 10, " ").toInt();
		PlayerListing * p = getPlayerListingNeverFail(aMatch->opponent);
		PlayerListing * us = room->getPlayerListing(getUsername());
        aMatch->our_name = us->name;
        aMatch->our_rank = us->rank;

        aMatch->their_rank = p->rank;
		/* Maybe we have to do automatch, rather than accept/decline,
		 * FIXME */
		//protocol_save_string = "automatch";
        GameDialog * gameDialog = getGameDialog(p);
		gameDialog->recvRequest(aMatch);
		
		delete aMatch;
	}
}

void IGSConnection::handle_serverinfo(QString)
{}

		// Setting your . to xxxx
		//case 40:
void IGSConnection::handle_dot(QString)
{}
		// long user report equal to 7
		// 7 [255]          YK [ 7d*] vs.         SOJ [ 7d*] ( 56   19  0  5.5  4  I) ( 18)
		// 42 [88]          AQ [ 2d*] vs.        lang [ 2d*] (177   19  0  5.5  6  I) (  1)

		// IGS: cmd 'user'
		// 42 Name        Info            Country  Rank Won/Lost Obs  Pl Idle Flags Language
		// 0  3           15              31       40   45 48    54   59 62   67    72
		// 42    guest15                  --        NR    0/   0  -   -   10s    Q- default 
		// 42    guest13                  --        NR    0/   0  -   -   24s    Q- default 
		// 42         zz  You may be rec  France    NR    0/   0  -   -    0s    -X default 
		// 42     zz0002  <none>          Japan     NR    0/   0  -   -    1m    QX default 
		// 42     zz0003  <none> ()       Japan     NR    0/   0  -   -    1m    SX default 
		// 42       anko                  Taiwan    2k* 168/  97  -   -    1s    -- default 
		// 42     MUKO12  1/12 〜 1/15    Japan     4k* 666/ 372  17  -    1m    -X default 
		// 42        mof  [Igc2000 1.1]   Sweden    2k* 509/ 463 124  -    0s    -X default 
		// 42     obiyan  <None>          Japan     3k* 1018/ 850  -   50  11s    -- default 
		// 42    uzumaki                  COM       1k    0/   0  -   -    1s    -X default 
		// 42 HansWerner  [Igc2000 1.2]   Germany   1k   11/   3 111  -    1m    QX default 
		// 42    genesis                  Germany   1k* 592/ 409  -  136  14s    -- default 
		// 42    hamburg                  Germany   3k* 279/ 259  -  334  13s    -- default 
		// 42        Sie                  Germany   1k*   6/   7  -   68  39s    -- default 
		// 42     borkum                  Germany   4k* 163/ 228  -  100   7s    -- default 
		// 42     casumy  45-3            Germany   1d    0/   0  -  133   2s    Q- default 
		// 42      stoni                  Germany   2k* 482/ 524  -  166   7s    -- default 
		// 42   Beholder  <None>          Germany   NR    0/   0 263  -    5s    QX default 
		// 42     xiejun  1/10-15         Germany   3d* 485/ 414 179  -   49s    -X default
		// 9                 ******** 1120 Players 247 Total Games ********

		// IGS: new cmd 'userlist' for nmatch information
		//          10        20        30        40        50        60        70        80        80        90       100       110       120
		// 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
		// 42 Name        Info            Country  Rank Won/Lost Obs  Pl Idle Flags Language
		// 42      -----  <none>          Japan    15k+ 2108/2455  -   -    1m    Q- default  T BWN 0-9 19-19 600-600 1200-1200 25-25 0-0 0-0 0-0
		// 42      -----           Japan     3d   11/  13  -  222  46s    Q- default  T BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
		// 42       Neil  <None>          USA      12k  136/  86  -   -    0s    -X default  T BWN 0-9 19-19 60-60 60-3600 25-25 0-0 0-0 0-0
		//case 42:
		
		
//42    koumas*                  -        11k  0000/0000  -    4  11m    9a E?E----

//		42    koumas*                  -        11k  0000/0000  -    4  11m    9a E?E----
//		No match



//		42    spaman*                  -        11k* 0000/0000  -    4  11m    9a E?E----

//		42    spaman*                  -        11k* 0000/0000  -    4  11m    9a E?E----
//		No match

void IGSConnection::handle_userlist(QString line)
{
	//line = line.remove(0, 2).trimmed();
    PlayerListing * aPlayer = NULL;
	Room * room = getDefaultRoom();
	if (line[40] == 'R')	//this was line ??? 
	{
				// skip
		return;
	}
			//                        1                   
			//                        Name
	QRegExp re1 = QRegExp("42 ([A-Za-z0-9 ]{,10}) ");
			//			2
			//			Info
	QRegExp re2 = QRegExp("(.{1,14})  ");
			//                    3
			//                    Country
	QRegExp re3 = QRegExp("([a-zA-Z][a-zA-Z. /&#92;']{,6}|--     )  "
			//                    4
			//                    Rank
			"([0-9 ][0-9][kdp].?| +BC| +NR) +"
			//                    5               6
			//                    Won             Lost
			"([0-9 ]+[0-9]+)/([0-9 ]+[0-9]+) +"
			//                    7           8
			//                    Obs         Pl
			"([0-9]+|-) +([0-9]+|-) +"
			//                    9               10                   11    12
			//                    Idle            Flags
			"([A-Za-z0-9]+) +([^ ]{,2})");// +default");
	QRegExp re4 = QRegExp(" ([TF]) (.*)");
	if(re1.indexIn(line) < 0 || re3.indexIn(line) < 0)
	{
		qDebug("\n%s", line.toUtf8().data());
		qDebug("No match\n");
		return;
	}
			
			// 42       Neil  <None>          USA      12k  136/  86  -   -    0s    -X default  T BWN 0-9 19-19 60-60 60-3600 25-25 0-0 0-0 0-0

	QString name = re1.cap(1).trimmed();
    aPlayer = room->getPlayerListing(name);

	if(re2.indexIn(line) >= 0)
	{
		aPlayer->extInfo = re2.cap(1).trimmed();
	}
	else
		aPlayer->extInfo = "<None>";
	if(aPlayer->extInfo == "")
		aPlayer->extInfo = "<None>";

	aPlayer->country = re3.cap(1).trimmed();
	if(aPlayer->country == "--")
		aPlayer->country = "";
	aPlayer->rank = re3.cap(2).trimmed();
	fixRankString(&(aPlayer->rank));
	aPlayer->rank_score = rankToScore(aPlayer->rank);
	aPlayer->wins = re3.cap(3).trimmed().toInt();
	aPlayer->losses = re3.cap(4).trimmed().toInt();
	aPlayer->observing = re3.cap(5).trimmed().toInt();
	aPlayer->playing = re3.cap(6).trimmed().toInt();
	aPlayer->idletime = re3.cap(7).trimmed();
	aPlayer->seconds_idle = idleTimeToSeconds(aPlayer->idletime);
	aPlayer->info = re3.cap(8).trimmed();

	if(re4.indexIn(line) < 0)
		aPlayer->nmatch = 0;
	else
		aPlayer->nmatch = re4.cap(1) == "T";
	aPlayer->nmatch_settings = "";
	QString nmatchString = "";
			// we want to format the nmatch settings to a readable string
	if (aPlayer->nmatch)
	{	
				// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
		nmatchString = re4.cap(2).trimmed();
		if ( ! nmatchString.isEmpty())
		{
					
			aPlayer->nmatch_black = (nmatchString.contains("B"));
			aPlayer->nmatch_white = (nmatchString.contains("W"));
			aPlayer->nmatch_nigiri = (nmatchString.contains("N"));					
	
					
			aPlayer->nmatch_timeMin = element(nmatchString, 2, " ", "-").toInt();
			aPlayer->nmatch_timeMax = element(nmatchString, 2, "-", " ").toInt();
			QString t1min = QString::number(aPlayer->nmatch_timeMin / 60);
			QString t1max = QString::number(aPlayer->nmatch_timeMax / 60);
			if (t1min != t1max)
				t1min.append(" to ").append(t1max) ;
					
			QString t2min = element(nmatchString, 3, " ", "-");
			QString t2max = element(nmatchString, 3, "-", " ");
			aPlayer->nmatch_BYMin = t2min.toInt();
			aPlayer->nmatch_BYMax = t2max.toInt();

			QString t3min = QString::number(t2min.toInt() / 60);
			QString t3max = QString::number(t2max.toInt() / 60);

			if (t2min != t2max)
				t2min.append(" to ").append(t2max) ;

			if (t3min != t3max)
				t3min.append(" to ").append(t3max) ;

			QString s1 = element(nmatchString, 4, " ", "-");
			QString s2 = element(nmatchString, 4, "-", " ");
			aPlayer->nmatch_stonesMin = s1.toInt();
			aPlayer->nmatch_stonesMax = s2.toInt();
					
			if (s1 != s2)
				s1.append("-").append(s2) ;
					
			if (s1 == "1")
			{
				aPlayer->nmatch_timeSystem = byoyomi;
				aPlayer->nmatch_settings = "Jap. BY : " +
						t1min + " min. + " +
						t2min + " secs./st. " ;
			}
			else
			{
				aPlayer->nmatch_timeSystem = canadian;
				aPlayer->nmatch_settings = "Can. BY : " +
						t1min + " min. + " +
						t3min + " min. /" +
						s1 + " st. ";
			}

			QString h1 = element(nmatchString, 0, " ", "-");
			QString h2 = element(nmatchString, 0, "-", " ");
			aPlayer->nmatch_handicapMin = h1.toInt();
			aPlayer->nmatch_handicapMax = h2.toInt();
			if (h1 != h2)
				h1.append("-").append(h2) ;
					
			aPlayer->nmatch_settings.append("(h " + h1 + ")") ;
		}
		else 
			aPlayer->nmatch_settings = "No match conditions";
	}

			// indicate player to be online
	aPlayer->online = true;
    emit playerListingReceived(aPlayer);
}

// IGS: 49 Game 42 qGoDev is removing @ C5
void IGSConnection::handle_removed(QString line)
{
	line = line.remove(0, 2).trimmed();
	if (line.contains("is removing @"))
	{
		int game = element(line, 1, " ").toInt();
		QString pt = element(line, 6, " ");
		handleRemovingAt(game, pt);
	}
}

void IGSConnection::handleRemovingAt(unsigned int game, QString pt)
{
	BoardDispatch * boarddispatch = getIfBoardDispatch(game);
	if(!boarddispatch)
	{
		qDebug("removing for game we don't have");
		return;
	}
	GameData * r = boarddispatch->getGameData();
	if(!r)
	{
		qDebug("Can't get game record");
		return;
	}
		
	MoveRecord * aMove = new MoveRecord();
	aMove->flags = MoveRecord::REMOVE;
	aMove->x = (int)(pt.toLatin1().at(0));
	aMove->x -= 'A';
	if(aMove->x < 9)	//no I on IGS
		aMove->x++;
	pt.remove(0,1);
	aMove->y = element(pt, 0, " ").toInt();
	/* Do we need the board size here???*/
	aMove->y = r->board_size + 1 - aMove->y;
	boarddispatch->recvMove(aMove);
	delete aMove;
}

//FIXME	
//51 Say in game 432
void IGSConnection::handle_ingamesay(QString s)
{
	qDebug("Ingamesay: %s", s.toLatin1().constData());
}

// 53 Game 75 adjournment is declined
void IGSConnection::handle_adjourndeclined(QString line)
{
	line = line.remove(0, 2).trimmed();
	BoardDispatch * boarddispatch;
	if (line.contains("adjournment is declined"))
	{
		unsigned int game_nr = element(line, 1, " ").toInt();
		boarddispatch = getBoardDispatch(game_nr);
		if(boarddispatch)
			boarddispatch->recvRefuseAdjourn();
		else
		{
			qDebug("Adjourn decline recv %d", game_nr);
		}
	}
}

// IGS : seek syntax, answer to 'seek config_list' command
// 63 CONFIG_LIST_START 4
// 63 CONFIG_LIST 0 60 600 25 0 0
// 63 CONFIG_LIST 1 60 480 25 0 0
// 63 CONFIG_LIST 2 60 300 25 0 0
// 63 CONFIG_LIST 3 60 900 25 0 0
// 63 CONFIG_LIST_END

// 63 OPPONENT_FOUND

// 63 ENTRY_LIST_START 5
// 63 ENTRY_LIST HKL 60 480 25 0 0 19 1 1 0
// 63 ENTRY_LIST tgor 60 480 25 0 0 19 1 1 0
// 63 ENTRY_LIST sun756 60 600 25 0 0 19 1 1 0
// 63 ENTRY_LIST horse 60 600 25 0 0 19 2 2 0
// 63 ENTRY_LIST masaeaki 60 300 25 0 0 19 1 1 0
// 63 ENTRY_LIST_END
void IGSConnection::handle_seek(QString line)
{
	line = line.remove(0, 2).trimmed();
	if (line.contains("OPPONENT_FOUND"))
	{
		QString oppname = element(line, 1, " ");
		ConsoleDispatch * console = getConsoleDispatch();
		if(console)
			console->recvText("Opponent found: " + oppname);
		recvSeekCancel();
	}
	else if (line.contains("CONFIG_LIST "))
	{
		SeekCondition * seekCondition = new SeekCondition();
		seekCondition->number = element(line, 1, " ").toInt();
		seekCondition->maintime = element(line, 2, " ").toInt();
		seekCondition->periodtime = element(line, 3, " ").toInt();
		seekCondition->periods = element(line, 4, " ").toInt();
		seekCondition->bline = element(line, 1, " ", "EOL");
		recvSeekCondition(seekCondition);
	}
	else if (line.contains("CONFIG_LIST_START"))
	{	
		recvSeekCondition(0);
	}
	else if (line.contains("ENTRY_CANCEL"))
	{	
		recvSeekCancel();
	}
	else if ((line.contains("ERROR")) )
	{	
		recvSeekCancel();
		ConsoleDispatch * console = getConsoleDispatch();
		if(console)
			console->recvText(line.toLatin1().constData());
	}
	else if ((line.contains("ENTRY_LIST_START")) )
	{	
		//emit signal_clearSeekList();
		recvSeekPlayer("", "");
	}
	else if ((line.contains("ENTRY_LIST ")) )
	{	
		//FIXME what does this do??? what does it mean?  who am I?
		QString player = element(line, 1, " ");
		QString condition = 
				element(line, 7, " ")+"x"+element(line, 7, " ")+" - " +
				QString::number(int(element(line, 2, " ").toInt()/60))+
				" +" +
				QString::number(int(element(line, 3, " ").toInt()/60)).rightJustified(3) +
				"' (" +
				element(line, 4, " ")+
				") H -"+
				element(line, 8, " ") +
				" +" +
				element(line, 9, " ");						
			
		recvSeekPlayer(player, condition);
	}
}

// IGS review protocol
// 56 CREATE 107
// 56 DATA 107
// 56 OWNER 107 eb5 3k
// 56 BOARDSIZE 107 19
// 56 OPEN 107 1
// 56 KIBITZ 107 1
// 56 KOMI 107 0.50
// 56 TITLE 107 yfh22-eb5(B) IGS
// 56 SGFNAME 107 yfh22-eb5-09-03-24
// 56 WHITENAME 107 yfh22
// 56 WHITERANK 107 1d?
// 56 BLACKNAME 107 eb5
// 56 BLACKRANK 107 3k
// 56 GAMERESULT 107 B+Resign
// 56 NODE 107 1 0 0 0
// 56 NODE 107 2 1 16 4
// 56 NODE 107 3 2 16 16
// 56 NODE 107 4 1 4 4
// 56 NODE 107 5 2 4 16
// 56 NODE 107 1 0 0 0
// 56 CONTROL 107 eb5
// 56 DATAEND 107
// 56 ERROR That user's client does not support review.
// 56 INVITED_PLAY 58 yfh2test
void IGSConnection::handle_review(QString line)
{
	line = line.remove(0, 2).trimmed();
#ifdef FIXME	
	if (line.contains("CREATE"))
		aGame->number = element(line, 1, " ").toInt();
	if (line.contains("OWNER"))
		aGame->player = element(line, 2, " ");
	if (line.contains("BOARDSIZE"))
	{
		aGame->board_size = element(line, 2, " ").toInt();			
		memory = aGame->board_size;
	}
	if (line.contains("KOMI"))
		aGame->K = element(line, 2, " ");
	if (line.contains("WHITENAME"))
		aGame->wname = element(line, 2, " ");
	if (line.contains("WHITERANK"))
	{
		aGame->white_rank = element(line, 2, " ");
		fixRankString(&(aGame->white_rank));
		aGame->white_rank_score = rankToScore(aGame->white_rank);
	}
	if (line.contains("BLACKNAME"))
		aGame->black_name = element(line, 2, " ");
	if (line.contains("BLACKRANK"))
	{
		aGame->black_rank = element(line, 2, " ");
		fixRankString(&(aGame->black_rank));
		aGame->black_rank_score = rankToScore(aGame->black_rank);
	}
	if (line.contains("GAMERESULT"))
	{
		aGame->res = element(line, 2, " ");
		emit signal_gameReview(aGame);
		break;
	}
	
	if (line.contains("INVITED_PLAY"))
	{
		emit signal_reviewInvite(element(line, 1, " "), element(line, 2, " "));
		break ;
	}
	
	if (line.contains("NODE"))
	{
					
		StoneColor sc = stoneNone;
		int c = element(line, 3, " ").toInt();
					
		if (c==1)
			sc = stoneBlack;
		else if(c==2)
			sc=stoneWhite;
	
	
		emit signal_reviewNode(element(line, 1, " ").toInt(), element(line, 2, " ").toInt(), sc, element(line, 4, " ", "EOL").toInt() ,element(line, 3, " ").toInt() );
		break ;
	}
#endif //FIXME
}

QString IGSConnection::element(const QString &line, int index, const QString &del1, const QString &del2, bool killblanks)
{
	int len = line.length();
	int idx = index;
	int i;
	QString sub;

	// kill trailing white spaces
    while (/*(int)*/ len > 0)
    {
        if (line[len-1] < 33)
            len--;
        else
            break;
    }

	// right delimiter given?
	if (del2.isEmpty())
	{
		// ... no right delimiter
		// element("a b c", 0, " ") -> "a"
		// element("  a b c", 0, " ") -> "a"  spaces at the beginning are skipped
		// element("a b c", 1, " ") -> "b"
		// element(" a  b  c", 1, " ") -> "b", though two spaces between "a" and "b"

		// skip (delimiters) spaces at the beginning
		i = 0;
//		while (i < len && line[i] == del1)
		while (i < len && line[i] == ' ')
			i++;

		for (; idx != -1 && i < len; i++)
		{
			// skip multiple (delimiters) spaces before wanted sequence starts
//			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1)
			while (idx > 0 && line[i] == ' ' && i < len-1 && line[i+1] == ' ')
				i++;

			// look for delimiters, maybe more in series
			if (line.mid(i,del1.length()) == del1)
				idx--;
			else if (idx == 0)
				sub += line[i];
		}
	}
	else
	{
		// ... with right delimiter
		// element("a b c", 0, " ", " ") -> "b"
		// element("(a) (b c)", 0, "(", ")") -> "a"
		// element("(a) (b c)", 1, "(", ")") -> "b c"
		// element("(a) (b c)", 0, " ", ")") -> "(b c"
		// element("(a) (b c)", 1, " ", ")") -> "c"
		// element("(a) (b c)", 1, "(", "EOL") -> "b c)"

		// skip spaces at the beginning
		i = 0;
		while (i < len && line[i] == ' ')
			i++;

		// seek left delimiter
		idx++;
	
		for (; idx != -1 && i < len; i++)
		{
			// skip multiple (delimiters) spaces before wanted sequence starts
//			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1)
			while (idx > 0 && line[i] == ' ' && i < len-1 && line[i+1] == ' ')
				i++;

			if ((idx != 0 && line.mid(i,del1.length()) == del1) ||
						  (idx == 0 && line.mid(i,del2.length()) == del2))
			{
				idx--;
			}
			else if (idx == 0)
			{
				// EOL (End Of Line)?
				if (del2 == QString("EOL"))
				{
					// copy until end of line
					for (int j = i; j < len; j++)
						if (!killblanks || line[j] != ' ')
							sub += line[j];

					// leave loop
					idx--;
				}
				else if (!killblanks || line[i] != ' ')
					sub += line[i];
			}
		}
	}
	
	return sub;
}

unsigned int IGSConnection::idleTimeToSeconds(QString time)
{
	QString i = time;
	/* I guess its either minutes or seconds here, not both */
	if(time.contains("m"))
	{
		i.replace(QRegExp("[m*]"), "");
		return (60 * i.toInt());
	}
	else
	{
		i.replace(QRegExp("s"), "");
		return i.toInt();
	}
	/*m1 = time1; m2 = time2;
	s1 = time1; s2 = time2;
	m1.replace(QRegExp("[m*]"), "");
	m2.replace(QRegExp("[m*]"), "");
	s1.replace(QRegExp("[*m]"), "");
	s1.replace(QRegExp("s"), "");
	s2.replace(QRegExp("[*m]"), "");
	s2.replace(QRegExp("s"), "");
	int seconds1 = (m1.toInt() * 60) + s1.toInt();
	int seconds2 = (m2.toInt() * 60) + s2.toInt();*/
}

void IGSConnection::fixRankString(QString * rank)
{
	if(rank->at(rank->length() - 1) == '*')
		rank->truncate(rank->length() - 1);
	else if(*rank == "NR") {}
#ifdef IGS_OLDRATING		//doublecheck WING/LGS FIXME
	else
			*rank += "?";
#endif //IGS_OLDRATING
}

PlayerListing * IGSConnection::getPlayerListingNeverFail(QString & name)
{
    return getDefaultRoom()->getPlayerListing(name);
}
