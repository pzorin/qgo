/***************************************************************************
 *   Copyright (C) 2011 by The qGo Project                                 *
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
#include <math.h>
#include "cyberoroconnection.h"
#include "cyberoroprotocol.h"
#include "consoledispatch.h"
#include "room.h"
#include "boarddispatch.h"
#include "gamedialog.h"
#include "talk.h"
#include "serverlistdialog.h"
#include "codecwarndialog.h"
#include "matchinvitedialog.h"
#include "gamedialogflags.h"
#include "orosetphrasechat.h"
#include "setphrasepalette.h"
#include "createroomdialog.h"
#include "gamedata.h"
#include "playergamelistings.h"
#include "quickconnection.h"
#include "matchnegotiationstate.h"
#include <QMessageBox>

#ifdef NEWPROTOCOL
#include "protocol.h"
#endif //NEWPROTOCOL

#define ALLOW_ALL_INVITES		0
#define ALLOW_STRONGER_INVITE		1
#define ALLOW_EVEN_INVITE		2
#define ALLOW_WEAKER_INVITE		3
#define ALLOW_PAIR_INVITE		4
#define IGNORE_ALL_INVITES		5

#define PLAYERLIST_SKIPNUMBER_UNSET	0xffff

//#define RE_DEBUG

const char * KoreanCodec = "eucKR";
const char * ChineseCodec = "GB2312";
const char * JapaneseCodec = "Shift-JIS";
const char * LatinCodec = "ISO-8859-1";

CyberOroConnection::CyberOroConnection(const ConnectionCredentials credentials)
    : NetworkConnection(credentials)
{
    //Note that this is not world.cyberoro.com
	//which right now is 91.91.
	//This is gateway server which is sent login
	//information and authenticates us to other
	//servers that are provided on a list. */
    if(openConnection(hostname, port, NOT_MAIN_CONNECTION))
	{
        setState(LOGIN);
	}
	else
		qDebug("Can't open Connection\n");	//throw error?

	textCodec = QTextCodec::codecForLocale();
	serverCodec = QTextCodec::codecForLocale();
	//FIXME check for validity of codecs ??? 
	ORO_setup_setphrases();
	
	current_server_index = -1;
	challenge_response = 0;
	codetable = 0;
	
	room_were_in = 0;	//lobby?
	connecting_to_game_number = 0;
	
	matchKeepAliveTimerID = 0;
	matchRequestKeepAliveTimerID = 0;
	
	/* We should either create the palette on creation of
	 * the connection, or have a button to activate it that
	 * appears when the room is connected */
	setphrasepalette = new SetPhrasePalette(this);
	setphrasepalette->show();
}

CyberOroConnection::~CyberOroConnection()
{
	if(setphrasepalette)
		delete setphrasepalette;
	qDebug("Destroying CyberORO connection");
	closeConnection();
	std::vector<ServerItem*>::iterator iter;
	for(iter = serverList.begin(); iter != serverList.end(); iter++)
		delete *iter;
	if(challenge_response)
		delete[] challenge_response;
	if(codetable)
		delete[] codetable;
}

void CyberOroConnection::OnConnected()
{
	/* Likely client version info */
	if(connectionState == LOGIN)
	{
		sendLogin();
	}
}

void CyberOroConnection::sendText(QString)
{
	return;
}

void CyberOroConnection::sendDisconnect(void)
{
	/* Yeah, FIXME, we don't want to send this
	 * when we're reconnecting... which means we
	 * need a special connection state ... yada yada */
	if(connectionState == CONNECTED)
		sendDisconnectMsg();
}

void CyberOroConnection::sendText(const char * text)
{
	qDebug("Wanted to send %s", text);
	return;
}

/* What about a room_id?? */
void CyberOroConnection::sendMsg(unsigned int id, QString text)
{
	if(id == 0)
	{
		sendServerChat(text);
		return;
	}
	sendRoomChat(text.toLatin1().constData());
}

void CyberOroConnection::sendMsg(PlayerListing * player, QString text)
{
	sendPersonalChat(player, text.toLatin1().constData());
}

void CyberOroConnection::sendToggle(const QString & param, bool val)
{
	/* FIXME, more to do here probably */
	qDebug("Setting open %d", val);
	if(param == "open")
		sendInvitationSettings(val);
}

/* If we can observe something besides games,
 * like rooms or people, then this name is a bit
 * iffy too... */
void CyberOroConnection::sendObserve(const GameListing *game)
{
    connecting_to_game_number = game->number;
	sendObserveAfterJoining(game);
}

void CyberOroConnection::adjournGame(const GameListing * )
{
}

void CyberOroConnection::closeBoardDispatch(unsigned int game_id)
{
	/* This isn't quite right, we need to send a leave if
	* we close a window, but its not necessarily an adjournGame.
	* At the same time, I think dispatch needs to remain open
	* after game like a room, to talk in */
	GameListing * g = getDefaultRoom()->getGameListing(game_id);
    /* I've been saving boards that have results but I think maybe that's
         * supposed to be for betting matches in general.  I.e., we should check
         * here if we need to save the board */
    if(g->isBetting || g->result != QString())
    {
        BoardDispatch * b = getIfBoardDispatch(game_id);
        b->saveRecordToGameData();
    }
    /* Looks like finish observing is sent for both
     * also possibly finish observing used in matches somehow as well? */
    /* Order here matters apparently, leave is for leaving the room,
            FinishObserving says we're no longer looking at board.
              FIXME doublecheck
            No, that didn't fix it.*/
    if(room_were_in == g->number)
        sendLeave(g);
    sendFinishObserving(g);

	NetworkConnection::closeBoardDispatch(game_id);
}


/* I'd like to just send the match invite, but only if we're not
 * in the same room... actually, maybe its almost like a room invite */
void CyberOroConnection::sendMatchRequest(MatchRequest * mr)
{
	/* Right now, we're working with this from the stand point of
	 * the counter offer.  The gamedialog needs to check if
	 * anything has changed and then do acceptOffer. */
	sendMatchOffer(*mr, true);
}

/* Note that we could ignore the dispute time one and change whatever
 * we wanted to, but its kind of cheap to the other player who can't
 * see what we changed. */
unsigned long CyberOroConnection::getGameDialogFlags(void)
{
	return (GDF_CANADIAN | GDF_BYOYOMI | GDF_TVASIA | GDF_FREE_RATED_CHOICE 
		  | GDF_RATED_SIZE_FIXED | GDF_RATED_HANDICAP_FIXED
	          | GDF_NIGIRI_EVEN | GDF_ONLY_DISPUTE_TIME
		  | GDF_BY_CAN_MAIN_MIN);
}

void CyberOroConnection::declineMatchOffer(const PlayerListing * opponent)
{
	qDebug("Declining match offer");
	/* If this is our offer, we need to sendMatchOfferCancel FIXME */
	sendDeclineMatchOffer(opponent);
	// do we also need to leave the room?
}

void CyberOroConnection::cancelMatchOffer(const PlayerListing * opponent)
{
	qDebug("Canceling match offer");
	sendMatchOfferCancel(opponent);
}

void CyberOroConnection::acceptMatchOffer(const PlayerListing * /*opponent*/, MatchRequest * mr)
{
	/* FIXME, we should really just move the real code in here instead
	 * of the extra layer of indication... when its done. */
	sendMatchOffer(*mr);
}

void CyberOroConnection::handlePendingData()
{
    unsigned char * c;
    unsigned char header[4];
	unsigned int packet_size;
	int bytes;

	/* We need better connection states, but we'll use the old
	 * for now */

    switch(connectionState)
    {
    case LOGIN:
        bytes = qsocket->bytesAvailable();
        if(bytes)
        {
            QByteArray data = qsocket->readAll();
            //0004064d

#ifdef RE_DEBUG
            for(int i = 0; i < bytes; i++)
                printf("%02x", c[i]);
            printf("\n");
#endif //RE_DEBUG
            //1f27080015270000
            if(data.constData()[0] == 0x1f)
            {
                qDebug("CyberOroConnection::handlePendingData() : Bad password or login");
                setState(PASS_FAILED);
                //closeConnection();
                return;
            }
            /* First packet is list of servers and ips */
            else if(data.constData()[0] == 0x24)
                handleServerList((unsigned char *)data.data());
        }
        break;
    case CONNECTED:
        while((bytes = qsocket->bytesAvailable()) > 0)
        {
            qsocket->peek((char *)header, 4);
#ifdef RE_DEBUG
            printf("%02x%02x%02x%02x\n", header[0], header[1], header[2], header[3]);
#endif //RE_DEBUG
            packet_size = header[2] + (header[3] << 8);
            if((unsigned)bytes < packet_size)
                break;
            c = new unsigned char[packet_size];
            qsocket->read((char*)c, packet_size);
            handleMessage(c, packet_size);
            delete[] c;
        }
        break;
    case CANCELED:
    case RECONNECTING:
        break;
    /*case AUTH_FAILED:
        qDebug("Auth failed\n");
        break;*/
    default:
        qDebug("connection state not handled by ORO");
        break;
    }
}

/* FIXME, these need to be const msg refs I think */
/* They also need to be reordered in a more intuitive manner
 * after we get the IGS cruft out */
void CyberOroConnection::handleServerList(unsigned char * msg)
{
    unsigned char * p = msg;
    unsigned char * s;
	int servers, i;
	int ip_string_length, server_name_length;
	ServerItem * si;
	
	if(p[0] != 0x24 || p[1] != 0x27)
	{
		qDebug("Non standard server list msg header");		//unnecessary check FIXME
		printf("%02x %02x", p[0], p[1]);
	}
	p += 4;
	challenge_response = new unsigned char[106];
	for(i = 0; i < 106; i++)
		challenge_response[i] = p[i];
	p += 108;	//106 + 01 00
	servers = p[1];
	/* FIXME, I no longer think that this is the number of
	 * servers.  I think there are always 8 */
	/* Actually, I think it is the number of servers but some of them
	 * are not game servers... */
	servers = 8;
#ifdef RE_DEBUG
	qDebug("Servers: %d", servers);
	printf("\n");
	for(i = 0; i < 20; i++)
		printf("%02x ", p[i]);
	printf("\n");
#endif //RE_DEBUG
	p += 2;
	for(i = 0; i < servers; i++)
	{
		/* If the first byte is 0x0a, right before the ip, that
		 * might indicate full, the byte before that then might
		 * be the language of the previous server entry. */
		si = new ServerItem();
		ip_string_length = 0;
		server_name_length = 0;
		s = p + 16;
		strncpy(si->ipaddress, (const char *)p, 16);
		while(p < s && *p != 0x00)
		{
#ifdef RE_DEBUG
			printf("%c", *p);
#endif //RE_DEBUG
			p++;
			ip_string_length++;
		}
#ifdef RE_DEBUG
		printf(" -> ");
#endif //RE_DEBUG
		// pad out to 15 (max ip length
		p += (16 - ip_string_length);
		s = p + 16;
		//strncpy(si->name, (const char *)p, 18);
		si->name = QString((char *)p);
		while(p < s && *p != 0x00)
		{
#ifdef RE_DEBUG
			printf("%c", *p);
#endif //RE_DEBUG
			p++;
			server_name_length++;
		}
		
		p += (16 - server_name_length);	
		printf("\n");
		serverList.push_back(si);
#ifdef RE_DEBUG
		printf("Language byte: %02x\n", p[0]);
#endif //RE_DEBUG
		switch(p[0])
		{
			case 1:
				si->codec = (char *)KoreanCodec;
				break;
			case 2:
				si->codec = (char *)ChineseCodec;
				break;
			case 3:
				si->codec = (char *)JapaneseCodec;
				break;
			case 6:
				si->codec = (char *)LatinCodec;
				break;
			default:
				break;
		}
		p += 2;
	}
	/* We should really check for overflows here so that we don't run out
	 * of packet.  FIXME FIXME FIXME 1046 bytes every time*/
	
	/* We close here because this first time, its going to close
	 * anyway, and if we don't close here, we'll get an error
	 * and lose the object */
	
	
	//requestAccountInfo();		//this will close this connection	
	closeConnection(false);
    setState(RECONNECTING);
	requestAccountInfo();
	if(reconnectToServer() < 0)
	{
		qDebug("User canceled");
        setState(CANCELED);
		return;
	}
}

void CyberOroConnection::changeServer(void)
{
	qDebug("changeServer called");
	if(connectionState != CONNECTED)
		return;
	if(reconnectToServer() < 0)
	{
		qDebug("User canceled");
		return;
	}
}

void CyberOroConnection::createRoom(void)
{
	CreateRoomDialog * createroomdialog = new CreateRoomDialog(this);
	createroomdialog->exec();	
}

/* FIXME we need error handling here, a return value, something
 * or at least we need to call the right destructors throughout,
 * and safely. */
int CyberOroConnection::reconnectToServer(void)
{
	/* FIXME, we need to add something to specify the server we're
	 * currently on */
	/* FIXME, why does the serverReconnectDialog disappear the ui
	 * elements in the server window ?!?!? */
	ServerListDialog * serverReconnectDialog = new ServerListDialog(serverList, current_server_index);
	int server_i = serverReconnectDialog->exec();
	if(server_i < 0 || server_i == QDialog::Rejected)
		return -1;
	server_i--;
	if(!this)
	{
		/* FIXME, this needs to be much better.  It should be,
		 * to start with, irrelevant. */
		qDebug("Waited too long to connect");
		return -1;
	}
	current_server_index = server_i;
	
	/* FIXME
	 * Might be neat if we listed the server that one was currently on
	 * somewhere, like on the main window. */
	if(serverList.size() <= (unsigned)server_i)
	{
		qDebug("serverList has probably been freed when you weren't looking");
		return -1;
	}
	
	qDebug("Reconnecting to %s: %s...", serverList[server_i]->name.toLatin1().constData(), serverList[server_i]->ipaddress);
	
	if(connectionState == CONNECTED)
		closeConnection(false);
	/* Though connection isn't really negotiated yet,
	 * we can get here without a playerListingIDRegistry as soon
	 * as the connection is opened, so we're going to do this
	 * first.  FIXME for other protocols */
	onAuthenticationNegotiated();
	if(openConnection(serverList[server_i]->ipaddress, 7002))
	{
		qDebug("Reconnected");
        setState(CONNECTED);
        game_code_to_number.clear();
	}
	else
	{
		/* FIXME This can happen, for instance two of the china servers gave connection refused errors
		 * which left the select server drop down menu grayed out!!! Add handling for this! */
		qDebug("Can't open Connection!!");
	}
	/* FIXME, really we should set server init variables in a function
	 * called from here on every reconnect, not on connection object allocation*/
	playerlist_skipnumber = PLAYERLIST_SKIPNUMBER_UNSET;
	playerlist_received = 0;
	roomlist_observers = 0;
	playerlist_roomnumber = 1;
	playerlist_observernumber = 0;

	if(server_i > 6)
		qDebug("Please don't try to connect to the voice, we don't know what that is");

	serverCodec = QTextCodec::codecForName(getCodecString());
	if(!serverCodec)
	{
		new CodecWarnDialog(getCodecString());
		serverCodec = QTextCodec::codecForLocale();
	}
	
	// this is the initial packet
    const unsigned char packet[8] = { 0x0a, 0xfa, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if(write((const char *)packet, 8) < 0)
		qWarning("*** failed sending init packet to reconnected host");
	return 0;
}

/* Check comment in networkconnection code, FIXME */
// Why is this function different for this connection type?
// Because player tracking is by ID, not name, for CyberOro
const PlayerListing * CyberOroConnection::getOurListing(void)
{
    ourListing = getDefaultRoom()->getPlayerListing(our_player_id);
    return ourListing;
}

void CyberOroConnection::sendPersonalChat(const PlayerListing * player, const char * text)
{
	unsigned int length = strlen(text) + 12;
	char *packet = new char[length];
	int i;
		
	packet[0] = 0xe2;
	packet[1] = 0x59;
	packet[2] = length & 0x00ff;
	packet[3] = (length >> 8);
    packet[4] = player->id & 0x00ff;
    packet[5] = (player->id >> 8);
    packet[6] = player->specialbyte;
	packet[7] = 0x00;
	packet[8] = our_player_id & 0x00ff;
	packet[9] = (our_player_id >> 8);
	//their id 93[62]00 (maybe a room), id?, 00 00
	packet[10] = 0x00;
	packet[11] = 0x00;
	for(i = 12; i < (int)length; i++)
		packet[i] = text[i - 12];
#ifdef RE_DEBUG
	printf("Before encode\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, 0xc);
#ifdef RE_DEBUG
	printf("Sending personal chat\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending personal chat");
	delete[] packet;
}

// there's a room chat, looks like
//ec 59 size our_id 00 00 text with encode 8
/* But I don't see the room id so I'm confused */
//FIXME
/* Now I'm thinking it just goes to the room we're in */
void CyberOroConnection::sendRoomChat(const char * text)
{
	unsigned int length = strlen(text) + 8;
	char *packet = new char[length];
	int i;
		
	packet[0] = 0xec;
	packet[1] = 0x59;
	packet[2] = length & 0x00ff;
	packet[3] = (length >> 8);
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0x00;
	packet[7] = 0x00;		//dc? font?
	for(i = 8; i < (int)length; i++)
		packet[i] = text[i - 8];
#ifdef RE_DEBUG
	printf("Before encode\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, 0x8);
#ifdef RE_DEBUG
	printf("Sending room chat\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending room chat");
	delete[] packet;
}

void CyberOroConnection::sendServerChat(QString text)
{
	unsigned int length = text.size() + 8;
	unsigned char *packet = new unsigned char[length];
	int i;
	
	packet[0] = 0x0a;
	packet[1] = 0x5a;
	packet[2] = length & 0x00ff;
	packet[3] = (length >> 8);
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0x00;
	packet[7] = 0x00;			//i've seen 40 here
	for(i = 8; i < (int)length; i++)
		packet[i] = text.toLatin1().constData()[i - 8];

	encode(packet, 0x8);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending server char");
	delete[] packet;
}

void CyberOroConnection::sendSetChatMsg(unsigned short phrase_id)
{
	unsigned int length = 20;
	unsigned char *packet = new unsigned char[length];
			
	packet[0] = 0x55;
	packet[1] = 0xc3;
	packet[2] = length & 0x00ff;
	packet[3] = (length >> 8);
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0xff;	//probably their id if directed?
	packet[7] = 0xff;
	packet[8] = 0x01;
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;
	packet[12] = phrase_id & 0x00ff;
	packet[13] = (phrase_id >> 8);
	packet[14] = 0x00;
	packet[15] = 0x00;
	
	packet[16] = 0x00;
	packet[17] = 0x00;
	packet[18] = 0x00;
	packet[19] = 0x00;
	
	encode(packet, 0x14);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending set chat msg");
	delete[] packet;
}

void CyberOroConnection::sendJoinRoom(const RoomListing & room, const char * password)
{
	unsigned int length = 18;
	unsigned char * packet = new unsigned char[length];
	int i;

	packet[0] = 0xde;
	packet[1] = 0x5d;
	packet[2] = 0x12;
	packet[3] = 0x00;;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = room.number & 0x00ff;
	packet[7] = (room.number >> 8);
	//password padded out to ten bytes with zeroes
	//FIXME, but password only 8 in create room???
	if(password)
	{
		for(i = 0; i < (int)strlen(password); i++)
			packet[8 + i] = password[i];
		for(i = 9; i >= (int)strlen(password); i--)
			packet[8 + i] = 0x00;
	}
	else
	{
		for(i = 8; i < 18; i++)
			packet[i] = 0x00;
	}	
	qDebug("Sending join room");
	encode(packet, 0x12);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending join room");
	delete[] packet;
}

void CyberOroConnection::sendObserveOutside(const GameListing *game)
{
	unsigned int length = 10;
	char *packet = new char[length];

	/* We need to turn this to a join if its a room, not a game
	 * otherwise bad things happen FIXME */
    if(!game->game_code)
	{
        qDebug("No game code for %d", game->number);
		// can we send number instead?  where was the game code?
        delete[] packet;
		return;
	}
	packet[0] = 0x40;
	packet[1] = 0x9c;
	packet[2] = 0x0a;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
    packet[6] = game->game_code & 0x00ff;
    packet[7] = (game->game_code >> 8);
	packet[8] = 0x00;
	packet[9] = 0x00;	//careful could be something else? FIXME
	//our id game id then ED0B, maybe two random bytes? time?
#ifdef RE_DEBUG
	printf("Sending observe outside to %s vs %s: %d ", game.white_name().toLatin1().constData(), game.black_name().toLatin1().constData(), game.game_code);
	for(unsigned int i = 0; i < length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, 0xa);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending observe outside");
	delete[] packet;
	
    connecting_to_game_number = game->number;
}

/* This is a subcase of sendJoinRoom, we should fix that up
 * and add a password prompt FIXME */
void CyberOroConnection::sendObserveAfterJoining(const GameListing *game)
{
	unsigned int length = 18;
	char *packet = new char[length];
	int i;

	/* Observing a room definitely uses game number over game_code.
	 * weird... DOUBLECHECK */
	packet[0] = 0xde;
	packet[1] = 0x5d;
	packet[2] = 0x12;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
    packet[6] = game->number & 0x00ff;
    packet[7] = (game->number >> 8);
	for(i = 8; i < (int)length; i++)
		packet[i] = 0x00;	//password in here? all password?
#ifdef RE_DEBUG
	printf("Sending observe to %s vs %s: %d (%d)\n", game.white_name().toLatin1().constData(), game.black_name().toLatin1().constData(), game.game_code, game.number);	//crash here ?!?!
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, 0x12);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending observe");
	delete[] packet;
    setRoomNumber(game->number);
}

void CyberOroConnection::sendFinishObserving(const GameListing *game)
{
	unsigned int length = 14;
	char *packet = new char[length];
	int i;
	
	packet[0] = 0x45;
	packet[1] = 0x9c;
	packet[2] = 0x0e;
	packet[3] = 0x00;
    packet[4] = game->game_code & 0x00ff;
    packet[5] = (game->game_code >> 8);
	packet[6] = our_player_id & 0x00ff;
	packet[7] = (our_player_id >> 8);
	for(i = 8; i < 14; i++)
		packet[i] = 0x00;
	//and then?  0x54 instead of last 0x00?	//I'm thinking 0e encode
	// overwrites this and then it gets passed back to us in reply
	encode((unsigned char *)packet, 0xe);
#ifdef RE_DEBUG
	qDebug("Sending finish observing");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending finish observing");
	delete[] packet;
	//setRoomNumber(0);
}

void CyberOroConnection::sendCreateRoom(RoomCreate * room)
{
	unsigned int length = 64;
	char *packet = new char[length];
	int i;
	
	if(!room)
	{
		qDebug("CreateRoom called on NULL room!!");
        delete[] packet;
		return;
	}
	if((room->title && strlen((const char *)room->title) > 20) ||
	   (room->password && strlen((const char *)room->password) > 8))
	{
		qDebug("Can't create room, bad title or password");
        delete[] packet;
		return;
	}
	packet[0] = 0xca;
	packet[1] = 0x5d;
	packet[2] = 0x40;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0x00;
	packet[7] = 0x00;
	packet[8] = 0x00;
	packet[9] = 0x00;
	
	
	packet[10] = (room->title ? 0x80 : 0x00) | (room->password ? 0x40 : 0x00);
	//packet[10] = 0xc0; //80 open	//stronger quick 1:1 general password
	
	//set 0 before hand
	packet[11] = 0x00;
	packet[12] = 0x00;
	packet[13] = 0x00;
	packet[14] = 0x00;
	packet[15] = 0x00;
	packet[16] = 0x00;
	switch(room->type)
	{
		/* gomoku appears to use settings from game tab
		* what's the difference?  it automatically starts when
		* someone joins? */
		//gomoku equal player standard time
		//00 09 11
		case RoomCreate::GOMOKU:
		case RoomCreate::GAME:
			if(room->type == RoomCreate::GOMOKU)
				packet[11] = 0x09;
			else
				packet[11] = 0x00;
				packet[12] = ((unsigned char)room->opponentStrength << 4) |
				     	      (unsigned char)room->timeLimit;
			
			break;
		case RoomCreate::CHAT:
			packet[11] = 0xc0 | (unsigned char)room->topic;
			packet[12] = 0x00 | (unsigned char)room->location;
			//chat c1 21 movies -9 china
			//chat c0 c0 baduk all ages korea
			//chat cf 08 etc 70+ etc
			//chat c5 a3 manga 40-49 thai
			//chat c6 c4 humour 50-59 US
			qDebug("Unsupported Room Type");
			return;
			break;
		case RoomCreate::REVIEW:
			packet[11] = 0x80;
			
			packet[14] = 0x02;	//live //00 teaching
			//below 10k live commentary review
			//00 80 00 00 02 10
			//below 11k teaching focused
			//00 80 00 00 00 11 04
			//below 6d live
			//00 80 00 00 02 01
			qDebug("Unsupported Room Type");
			return;
			break;
		case RoomCreate::MULTI:
			//multi must be below or at(doublecheck?) your rank
			//multi 7 20m 20m below 10k
			//00 80 00 07 03 10 02
			//multi 6 15m 30m below 13k
			//00 80 00 06 04 13 01
			qDebug("Unsupported Room Type");
			return;
			break;
		case RoomCreate::VARIATION:
			qDebug("Unsupported Room Type");
			return;
			break;	
	}
	
	for(i = 17; i < 26; i++)
		packet[i] = 0x00;
	//title padded to 20
	if(room->title)
	{
		for(i = 0; i < (int)strlen((const char *)room->title); i++)
			packet[26 + i] = room->title[i];
		for(i = 19; i >= (int)strlen((const char *)room->title); i--)
			packet[26 + i] = 0x00;
	}
	else
	{
		for(i = 26; i < 46; i++)
			packet[i] = 0x00;
	}	
	for(i = 46; i < 56; i++)
		packet[i] = 0x00;
	//password padded to 8
	if(room->password)
	{
		for(i = 0; i < (int)strlen((const char *)room->password); i++)
			packet[56 + i] = room->password[i];
		for(i = 7; i >= (int)strlen((const char *)room->password); i--)
			packet[56 + i] = 0x00;
	}
	else
	{
		for(i = 56; i < 64; i++)
			packet[i] = 0x00;
	}	
	
	encode((unsigned char *)packet, 0x1a);
	qDebug("Sending create room");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending create room");
	delete[] packet;
	//setRoomNumber(0);
	delete room;
}

void CyberOroConnection::sendMatchInvite(const PlayerListing * player)
{
    qDebug("room were in %d room there in %d", room_were_in, player->observing);
	/* We'll need to check that they're in the same room as we
	 * are, in which case, we could just popup the dialog */
    if(room_were_in && player->observing == room_were_in)
	{
		sendMatchOfferPending(player);
	}
	else
		sendMatchInvite(player, false);
}

/* note that this only enters the match offer dialog
 * it doesn't accept the game.*/
void CyberOroConnection::sendMatchInvite(const PlayerListing * player, bool accepting)
{
#ifdef FIXME
	if(!match_negotiation_state->newMatchAllowed())		//really should be handled elsewhere FIXME
		return;
#endif //FIXME
	unsigned int length = 16;
	unsigned char * packet = new unsigned char[length];
	
	if(accepting)
		packet[0] = 0x86;
	else
		packet[0] = 0x22;
	packet[1] = 0x79;
	packet[2] = 0x10;
	packet[3] = 0x00;
	if(accepting)
	{
        packet[4] = player->id & 0x00ff;
        packet[5] = (player->id >> 8);
        packet[6] = player->specialbyte;
	}
	else
	{
		packet[4] = our_player_id & 0x00ff;
		packet[5] = (our_player_id >> 8);
		packet[6] = our_special_byte;
	}
	packet[7] = 0x00;
	if(accepting)
	{
		packet[8] = our_player_id & 0x00ff;
		packet[9] = (our_player_id >> 8);
		packet[10] = our_special_byte;
	}
	else
	{
        packet[8] = player->id & 0x00ff;
        packet[9] = (player->id >> 8);
        packet[10] = player->specialbyte;
	}
	packet[11] = 0x00;
	
	packet[12] = 0x00;
	packet[13] = 0x00;
	packet[14] = 0x00;
	packet[15] = 0x00;	//or 7b? random??? FIXME 
#ifdef RE_DEBUG
	printf("match invite packet: ");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0x10);
#ifdef RE_DEBUG
	qDebug("Sending accept match request or invite");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending accept match request or invite");
	delete[] packet;
	match_negotiation_state->sendMatchInvite((PlayerListing *)&player);
}

void CyberOroConnection::sendDeclineMatchInvite(const PlayerListing * player)
{
	unsigned int length = 12;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0xa4;
	packet[1] = 0x79;
	packet[2] = 0x0c;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
    packet[6] = player->id & 0x00ff;
    packet[7] = (player->id >> 8);
    packet[8] = player->specialbyte;
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;
	encode(packet, 0xc);
	qDebug("Sending decline match request");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending decline match request");
	delete[] packet;
}


/* This is also apparently sent automatically when
 * too much time passes after a request.  But I missed
 * that last byte, I think it was 1b, but it could have
 * been anything. FIXME 
 * There's also a chance I got this wrong and it
 * wasn't a decline message at all but something else. !!!*/
void CyberOroConnection::sendDeclineMatchOffer(const PlayerListing * player)
{
	unsigned int length = 10;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x32;
	packet[1] = 0x7d;
	packet[2] = 0x0a;
	packet[3] = 0x00;
    packet[4] = player->id & 0x00ff;
    packet[5] = (player->id >> 8);
	packet[6] = our_player_id & 0x00ff;
	packet[7] = (our_player_id >> 8);
	packet[8] = 0x00;
	packet[9] = 0x00;	//FIXME
	//32 7d 0a 00 49 04 d3 04 00 xx
	encode(packet, 0xa);
	qDebug("Sending decline request");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending decline request");
	delete[] packet;
}

void CyberOroConnection::sendMatchOfferPending(const PlayerListing * player)
{
	unsigned int length = 10;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x1e;
	packet[1] = 0x7d;
	packet[2] = 0x0a;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
    packet[6] = player->id & 0x00ff;
    packet[7] = (player->id >> 8);
	packet[8] = 0x00;
	packet[9] = 0x00;
	
	encode(packet, 0xa);
	qDebug("Sending match offer pending");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending match offer pending");
	delete[] packet;
	match_negotiation_state->sendMatchOfferPending();
}

void CyberOroConnection::sendMatchOfferCancel(const PlayerListing * )
{
	unsigned int length = 8;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x28;
	packet[1] = 0x7d;
	packet[2] = 0x08;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0x00;
	packet[7] = 0x00;	//FIXME
	
	
	encode(packet, 0x8);
	qDebug("Sending match offer cancel");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending match offer cancel");
	delete[] packet;
}

void CyberOroConnection::sendMatchOffer(const MatchRequest & mr, bool offercounteroffer)
{
	unsigned int length = 0xa0;
	unsigned char * packet = new unsigned char[length];
	Room * room = getDefaultRoom();
	PlayerListing * opponent = room->getPlayerListing(mr.opponent_id);
	if(!opponent)
	{
		qDebug("Can't get opponent listing for match request\n");
        delete[] packet;
		return;
	}
	int i;
	if(offercounteroffer)
		packet[0] = 0xc8;
	else
		packet[0] = 0xcd;
	packet[1] = 0xaf;
	packet[2] = 0xa0;
	packet[3] = 0x00;
	
	/* This is supposed to be their id for some reason... always?
	 * or only on accept? */
	if(offercounteroffer || mr.rematch)
	{
		// Careful, might only be our id when we send
		packet[4] = our_player_id & 0x00ff;
		packet[5] = (our_player_id >> 8);
		//commenting this out fixed the resign that we see to have (W)
		//but it still makes them see it wrong
		//also, it probably affects the "is challenger stuff so I'm not
		//so sure I want to do this
		//but then we still can't play black and the (W) is wrong
		//...
		// FIXME FIXME this all needs to be carefully redone
		//I think I should start really by seeing what the resigns
		//are sent as, i.e., with whos id.  I think its difficult because
		//there's no good way to see what id one is assigned
		//no, I think at the very least when accepting this
		//should not be commented out
		//don't know what mr.rematch is though
	}
	else
	{
		packet[4] = mr.opponent_id & 0x00ff;
		packet[5] = (mr.opponent_id >> 8);
	}
	packet[6] = mr.last_game_code & 0x00ff;
	packet[7] = (mr.last_game_code >> 8);
	packet[8] = 0x00;
	packet[9] = 0x02;	//02 or 00
	/* Looks like rated is forced if there's a (greater than one stone? )
	 * rank difference ? FIXME */
	if(mr.free_rated == RATED)
		packet[10] = 0x00;
	else
		packet[10] = 0x01;	//friendly
	packet[11] = mr.flags;
	if(mr.color_request == MatchRequest::NIGIRI)
		packet[11] |= 0x08;
	switch(mr.board_size)
	{
		case 19:
			packet[12] = 0x00;
			break;
		case 13:
			packet[12] = 0x01;
			break;
		case 9:
			packet[12] = 0x02;
			break;
		case 7:
			packet[12] = 0x03;
			break;
		case 5:
			packet[12] = 0x04;
			break;
		default:
			printf("Bad board size: %d\n", mr.board_size);
			return;
			break;
	}
	//looks like 13 is just whether there is a handicap
	if(mr.handicap > 1)
	{
		packet[13] = 0x01;

	}
	else
		packet[13] = 0x00;
#ifdef RE_DEBUG
	printf("challenger is black, 13: %d %d\n", mr.challenger_is_black, packet[13]);
#endif //RE_DEBUG
	packet[14] = mr.handicap;	//handicap I'd warrant
	/* FIXME
	 * handicap may force komi = 0.5 rather than whatever we've set it at.  Really
	 * gamedialog should set this up properly with some flag.  Also, we don't seem to
	 * check the komi later and we should, actually looks like oro ignores it even if its set */
	//if this is 0x01, then its negative komi, taken from white
	packet[15] = (mr.komi < 0 ? 0x01 : 0x00);		//not sure about this
	packet[16] = (int)(fabs(mr.komi) - 0.5);
#ifdef RE_DEBUG
	printf("komi: %02x %f %f\n", packet[16], mr.komi, fabs(mr.komi) - .5);
#endif //RE_DEBUG
	packet[17] = 0x00;
	//a4 01 a4 01 a4 01
	//6 bytes of time settings
	/* FIXME, in real life, we're probably going to need the old
	 * match request to fill this out properly 
	 * (later) ... or not... I think its just because same
	 * game struct is used for loading new games, a struct
	 * we should USE FIXME*/
	packet[18] = mr.maintime & 0x00ff;
	packet[19] = (mr.maintime >> 8);
	packet[20] = mr.maintime & 0x00ff;
	packet[21] = (mr.maintime >> 8);
	packet[22] = mr.maintime & 0x00ff;
	packet[23] = (mr.maintime >> 8);
	switch(mr.timeSystem)
	{
		case canadian:
			packet[31] = 0x01;
			break;
		case byoyomi:
			packet[31] = 0x00;
			break;
		case tvasia:
			packet[31] = 0x02;
			break;
		default:
			qDebug("Unknown time system!!");
			break;
	}
	packet[24] = mr.stones_periods;
	packet[25] = mr.stones_periods;
	packet[28] = mr.periodtime & 0x00ff;
	packet[29] = (mr.periodtime >> 8);
	packet[30] = mr.stones_periods;
	packet[26] = 0x00;
	packet[27] = 0x00;
	//[32] 01		may indicate white's handicap
	packet[32] = 0x02;
	//packet[32] = 0x01;		//definitely not on send, this throws it back to us
#ifdef RE_DEBUG
	printf("byte 32: %02x\n", packet[32]);
#endif //RE_DEBUG
	packet[33] = 0x00;
	packet[34] = 0x00;
	packet[35] = 0x00;
#ifdef RE_DEBUG
	printf("Room number: %d\n", mr.number);
#endif //RE_DEBUG
	packet[36] = mr.number & 0xff;
	packet[37] = (mr.number >> 8);
	packet[38] = 0x00;		//room number again sometimes?
	packet[39] = 0x00;
	for(i = 40; i < 52; i++)
		packet[i] = 0x00;
	packet[52] = 0x1f;	//??? FIXME
	/*packet[52] = ~0x1f;
	 * this was an extra bad idea, setting this to e0
	 * launches some weird special game type. */
	packet[53] = 0x00;
	for(i = 54; i < 62; i++)
		packet[i] = 0x00;
	//10 zeroes, though first might be part of our 1f friend
	packet[62] = 0x00;
#ifdef RE_DEBUG
	printf("opponent special byte: %02x\n", opponent->specialbyte);
#endif //RE_DEBUG
	packet[63] = 0x00;
	packet[63] = 0x00;
	//challengers name first... 
	// maybe challenger first if nigiri? otherwise black first?
	const QString * first_name, * second_name;
	unsigned short first_id, second_id;
	switch(mr.color_request)
	{
		case MatchRequest::NIGIRI:
			/* Pretty sure if we reverse these it
			 * screws up who sends the nigiri
			 * possibly among other things. */
			if(mr.opponent_is_challenger)
			{
				first_name = &opponent->name;
				first_id = mr.opponent_id;
				second_name = &getUsername();
				second_id = our_player_id;
			}
			else
			{
				first_name = &getUsername();
				first_id = our_player_id;
				second_name = &opponent->name;
				second_id = mr.opponent_id;
			}
		break;
		case MatchRequest::BLACK:
			first_name = &getUsername();
			first_id = our_player_id;
			second_name = &opponent->name;
			second_id = mr.opponent_id;
			break;
		case MatchRequest::WHITE:
			first_name = &opponent->name;
			first_id = mr.opponent_id;
			second_name = &getUsername();
			second_id = our_player_id;
			break;
	}
	
		  
		  
	for(i = 0; i < first_name->size(); i++)
		packet[64 + i] = first_name->at(i).cell();
	for(i = 9; i >= first_name->size(); i--)
		packet[64 + i] = 0x00;
	packet[74] = first_id & 0x00ff;
	packet[75] = (first_id >> 8);
	
	for(i = 0; i < second_name->size(); i++)
		packet[76 + i] = second_name->at(i).cell();
	for(i = 9; i >= second_name->size(); i--)
		packet[76 + i] = 0x00;
	packet[86] = second_id & 0x00ff;
	packet[87] = (second_id >> 8);
	
	for(i = 88; i < 98; i++)
		packet[i] = 0x00;
	packet[98] = 0x10;
	packet[99] = 0x27;
	for(i = 100; i < (int)length; i++)
		packet[i] = 0x00;
	//10 zeroes
	//packet[103] = 0x10;
	//packet[104] = 0x27;
	// 10 zeroes
	//packet[115] = 0x10;
	//packet[116] = 0x27;
	// 3 * 16 zeroes 
	
	match_negotiation_state->offerMatchTerms((MatchRequest *)&mr);
	match_negotiation_state->setGameId(mr.number);
	
#ifdef RE_DEBUG
	printf("match packet we are sending now: ");
	for(i = 0; i < (int)length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	encode(packet, 0x40);
#ifdef RE_DEBUG
	qDebug("Sending accept request");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending accept request");
	delete[] packet;
}

/* I don't have a fucking clue what this is, only that
 * it comes up constantly practically without warning.
 * it could be a "typing message", maybe there's one similar
 * when you check the score, etc.. to notify the other player
 * of what you're doing.  But since we've seen them before...
 * I don't know.  Either way, I'm not certain that its
 * a necessary message */
 /* These are necessary before moves are accepted I think...
  * and if they aren't... well for some reason moves aren't
  * getting played.
  * I tend to think this is how the time stuff works... 
  * and we potentially have to send one of these initially.*/
void CyberOroConnection::sendGameUpdate(unsigned short game_code)
{
	unsigned int length = 0xa;
	unsigned char * packet = new unsigned char[length];
	packet[0] = 0x50;
	packet[1] = 0xc3;
	packet[2] = 0x0a;
	packet[3] = 0x00;
	packet[4] = game_code & 0x00ff;
	packet[5] = (game_code >> 8);
	
	/* Okay looks like packet[6] controls the smiley faces ?
	 * 0b might be 0 shaped mouth, 0a could be yawn with tear? */
	packet[6] = 0x09;	//huh?? 0b
	/* Also seen 0x0b 0x0d 0x12 0x11 0x0b which might be a preplay mode, an estimate, possibly
	 * a clear or a return to preplay mode or something else and then back to match */
	packet[7] = 0x00;
	
	packet[8] = 0x00;
	packet[9] = 0x00;
//50 c3 0a 00 cd 01 0b 00 00 00
//50 c3 0a 00 0b 09 00 fc f6 08		//?
	encode(packet, 0x0a);
	qDebug("Sending game update");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending game update");
	delete[] packet;
}

void CyberOroConnection::sendKeepAlive(const GameListing *game)
{
	unsigned int length = 0x10;
	unsigned char * packet = new unsigned char[length];
    BoardDispatch * boarddispatch = getIfBoardDispatch(game->number);
	if(!boarddispatch)
	{
        qDebug("tried to sendKeepAlive on nonexistent board %d", game->number);
        delete[] packet;
		return;
	}
	TimeRecord tr = boarddispatch->getOurTimeRecord();
	GameData * gr = boarddispatch->getGameData();
	packet[0] = 0x64;
	packet[1] = 0xc3;
	packet[2] = 0x10;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
    packet[6] = game->game_code & 0x00ff;
    packet[7] = (game->game_code >> 8);
	if(gr->white_name == getUsername())
		packet[8] = 0x01;	//color?
	else
		packet[8] = 0x00;
	packet[9] = 0x00;
	
	//time
	packet[10] = tr.time & 0x00ff;
	packet[11] = (tr.time >> 8);
	//periods
	packet[12] = tr.stones_periods & 0x00ff;
	packet[13] = (tr.stones_periods >> 8);
	packet[14] = 0x00;		//probably whether we're in byo yomi?
	packet[15] = 0x00;		//FIXME

	encode(packet, 0x10);
	qDebug("Sending keep alive");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending keep alive");
	delete[] packet;
}

/* Unnecessary but we don't want to raise suspicion */
void CyberOroConnection::sendRequestKeepAlive(const GameListing *game)
{
	unsigned int length = 0x0c;
	unsigned char * packet = new unsigned char[length];
    BoardDispatch * boarddispatch = getIfBoardDispatch(game->number);
	if(!boarddispatch)
	{
        qDebug("tried to sendRequestKeepAlive on nonexistent board %d", game->number);
        delete[] packet;
		return;
	}
	GameData * gr = boarddispatch->getGameData();
	packet[0] = 0x5c;
	packet[1] = 0x4e;
	packet[2] = 0x0c;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
    packet[6] = game->game_code & 0x00ff;
    packet[7] = (game->game_code >> 8);
	packet[8] = gr->moves & 0x00ff;
	packet[9] = (gr->moves >> 8);
	packet[10] = 0x00;		//probably whether we're in byo yomi?
	packet[11] = 0x00;		//FIXME

	qDebug("sending request keepalive on moves: %d", gr->moves);
	encode(packet, 0x0c);
	qDebug("Sending request keep alive");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending keep alive2");
	delete[] packet;
}

void CyberOroConnection::saveIfDoesntSave(GameData * data)
{
	GameListing * listing = getDefaultRoom()->getGameListing(data->number);
	listing->gameData = new GameData(data);
}

void CyberOroConnection::timerEvent(QTimerEvent * event)
{
	if(event->timerId() == matchKeepAliveTimerID ||
		  event->timerId() == matchRequestKeepAliveTimerID)
	{
		Room * room = getDefaultRoom();
		GameListing * listing = room->getGameListing(match_negotiation_state->getGameId());
        if(event->timerId() == matchKeepAliveTimerID)
            sendKeepAlive(listing);
		else
            sendRequestKeepAlive(listing);
	}
}

void CyberOroConnection::sendMove(unsigned int game_id, MoveRecord * move)
{
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_id);
	if(!boarddispatch)
	{
		qDebug("Can't get board for send move: %d", game_id);
	}
	GameData * r = boarddispatch->getGameData();
	switch(move->flags)
	{
		case MoveRecord::RESIGN:
			sendResign(r->game_code);
			killActiveMatchTimers();
			return;
			break;
			
		case MoveRecord::REMOVE:
		case MoveRecord::UNREMOVE:
			sendRemoveStones(r->game_code, move);
			return;	
			break;
		case MoveRecord::DONE_SCORING:
			sendDoneScoring(r->game_code, r->opponent_id);
			return;	
			break;
		case MoveRecord::REQUESTUNDO:
			sendUndo(r->game_code, move);
			return;
			break;
		case MoveRecord::UNDO:
			sendAcceptUndo(r->game_code, move);
			return;
			break;
		case MoveRecord::REFUSEUNDO:
			sendDeclineUndo(r->game_code, move);
			return;
			break;
		default:
			break;
	}
	
	unsigned int length = 0x14;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0xdc;
	packet[1] = 0xaf;
	packet[2] = 0x14;
	packet[3] = 0x00;
	packet[4] = r->game_code & 0x00ff;
	packet[5] = (r->game_code >> 8);
	packet[6] = our_player_id & 0x00ff;
	packet[7] = (our_player_id >> 8);
	/* I've seen black moves sent with this... could be that non
	 * challenger sends 0101?, or maybe challenger does... */
    if(r->white_name == getOurListing()->name)
	{
		packet[8] = 0x01;		//both ones?
		packet[9] = 0x01;
	}
	else
	{
		packet[8] = 0x00;			//this maybe has to be for color?
		packet[9] = 0x00;
	}
	// this may be off, 1 too high in even games
	move->number++;		//wtf FIXME
	packet[10] = (move->number & 0x00ff);
	packet[11] = (move->number >> 8);
	if(move->flags == MoveRecord::PASS)
	{
		packet[12] = 0;
		packet[13] = 0;
	}
	else
	{
		packet[12] = move->x;
		packet[13] = move->y;
	}
	TimeRecord t = getBoardDispatch(game_id)->getOurTimeRecord();
	if(move->number == 1 || move->number == 2)
	{
		/* We apparently don't start the clock until 3rd move
		 * so this should fix that up. */
		packet[14] = r->maintime & 0x00ff;
		packet[15] = (r->maintime >> 8);
	}
	else
	{
		packet[14] = t.time & 0x00ff;
		packet[15] = (t.time >> 8);
	}
	if(t.stones_periods == -1)
	{
		//UGLY FIXME
		t.stones_periods = r->stones_periods;
#ifdef RE_DEBUG
		printf("r->periods here %d\n", r->stones_periods);
#endif //RE_DEBUG
		packet[17] = 0x00;
	}
	else
		packet[17] = 0x01;
	packet[16] = t.stones_periods;
	//packet[17] = (t.stones_periods >> 8);
	packet[18] = 0x00;
	packet[19] = 0x00;

//move
//dc af 14 00 fa 01 ec 05 00 00 0a 00 04 08 6e 04
//03 00 00 00
//pass
//dc af 14 00 6a e2 ec 05 00 00 02 00 00 00 b0 04
//03 00 00 00
	
//we send
//dc af 14 00 49 0d 2b 07 00 00 01 00 07 07 78 00 03 00 00 00 

#ifdef RE_DEBUG
	printf("movepacket: ");
	for(unsigned int i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0x14);
#ifdef RE_DEBUG
	qDebug("Sending move");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending move packet");
	delete[] packet;
	
	if(matchKeepAliveTimerID)
	{
		killTimer(matchKeepAliveTimerID);
		matchKeepAliveTimerID = 0;
		matchRequestKeepAliveTimerID = startTimer(39000);
	}
	
}

void CyberOroConnection::sendUndo(unsigned int game_code, const MoveRecord * move)
{
	unsigned int length = 0x0e;
	unsigned char * packet = new unsigned char[length];
	GameData * aGameData;
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("How can there not be a board dispatch, it just sent us an undo");
        delete[] packet;
		return;
	}
	aGameData = boarddispatch->getGameData();
	packet[0] = 0xe6;
	packet[1] = 0xaf;
	packet[2] = 0x0e;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = game_code & 0x00ff;
	packet[7] = (game_code >> 8);
	if(aGameData->white_name == getUsername())
		packet[8] = 0x01;
	else
		packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = move->number & 0x00ff;	//this is the move to start from (10)
	packet[11] = (move->number >> 8);
	packet[12] = 0x00;
	packet[13] = 0x00;
	
//from sender perspective	encode(e)
//e6af 0e00 b50d 4c02 01 00 0a 00 00 00
#ifdef RE_DEBUG
	printf("undo packet: ");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0xe);
	qDebug("Sending undo");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending undo packet");
	delete[] packet;
}

/* FIXME oro client can't seem to play after an undo, but we can if
 * its our turn.  We need to make sure that this is an ORO bug, and not
 * us screwing up the move number we send back */
void CyberOroConnection::sendDeclineUndo(unsigned int game_code, const MoveRecord * move)
{
	unsigned int length = 14;
	unsigned char * packet = new unsigned char[length];
	GameData * aGameData;
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("How can there not be a board dispatch, it just sent us a timeloss");
        delete[] packet;
		return;
	}
	aGameData = boarddispatch->getGameData();
	packet[0] = 0xf0;
	packet[1] = 0xaf;
	packet[2] = 0x0e;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = game_code & 0x00ff;
	packet[7] = (game_code >> 8);
	if(aGameData->white_name == getUsername())	//opposite color
		packet[8] = 0x00;
	else
		packet[8] = 0x01;
	packet[9] = 0x00;
	packet[10] = move->number & 0x00ff;
	packet[11] = (move->number >> 8);
	packet[12] = 0x00;
	packet[13] = 0x00;
	
	encode(packet, 0x0e);
	qDebug("Sending decline undo");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending decline undo");
	delete[] packet;
}

void CyberOroConnection::sendAcceptUndo(unsigned int game_code, const MoveRecord * move)
{
	unsigned int length = 14;
	unsigned char * packet = new unsigned char[length];
	GameData * aGameData;
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("How can there not be a board dispatch, it just sent us a timeloss");
        delete[] packet;
		return;
	}
	aGameData = boarddispatch->getGameData();
	packet[0] = 0xeb;
	packet[1] = 0xaf;
	packet[2] = 0x0e;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = game_code & 0x00ff;
	packet[7] = (game_code >> 8);
	if(aGameData->white_name == getUsername())
		packet[8] = 0x01;
	else
		packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = move->number & 0x00ff;
	packet[11] = (move->number >> 8);
	packet[12] = 0x00;
	packet[13] = 0x00;
	
	encode(packet, 0x0e);
	qDebug("Sending accept undo");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending accept undo");
	delete[] packet;
}

void CyberOroConnection::sendEndgame12Msg(unsigned int game_id, unsigned short msg_code)
{
	unsigned int length = 0x0c;
	unsigned char * packet = new unsigned char[length];
	GameData * aGameData;
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_id);
	if(!boarddispatch)
	{
		qDebug("How can there not be a board dispatch, it just sent us an endgame msg");
        delete[] packet;
		return;
	}
	aGameData = boarddispatch->getGameData();
	/* FIXME looking at this suggests that we have all the msg codes byte order backwards if they're
	 * to be consistent!! Interesting */
	packet[0] = (msg_code >> 8);
	packet[1] = msg_code & 0x00ff;
	packet[2] = 0x0c;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = aGameData->game_code & 0x00ff;
	packet[7] = (aGameData->game_code >> 8);
	if(aGameData->white_name == getUsername())
		packet[8] = 0x01;
	else
		packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;
	
	encode(packet, 0xc);
	qDebug("Sending %d endgame msg", msg_code);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending request count packet");
	delete[] packet;
}

/* FIXME Most of these messages all seem to be the same and I would love to clean up all of this
 * code with a single case statement or something */
void CyberOroConnection::sendRequestCount(unsigned int game_id)
{
	sendEndgame12Msg(game_id, 0xc4b3);
	killActiveMatchTimers();
}

void CyberOroConnection::sendAcceptCountRequest(GameData * data)
{
	sendEnterScoring(data->game_code);
}

void CyberOroConnection::sendRefuseCountRequest(GameData * data)
{
	sendEndgame12Msg(data->number, 0xceb3);
	BoardDispatch * boarddispatch = getIfBoardDispatch(data->number);
	if(!boarddispatch)
	{
		qDebug("How can there not be a board dispatch, it just sent us a send refuse count request");
		return;
	}
	startMatchTimers(boarddispatch->getBlackTurn() ? (data->black_name == getUsername()) : (data->white_name == getUsername()));
}

void CyberOroConnection::sendRequestMatchMode(unsigned int game_id)
{
	sendEndgame12Msg(game_id, 0x05b4);
	killActiveMatchTimers();		//does killing the match timers like this also stop the clock from ticking doublecheck FIXME?
	match_negotiation_state->sendMatchModeRequest();
}

void CyberOroConnection::sendAcceptRequestMatchMode(unsigned int game_id)
{
	unsigned int length = 0x08;
	unsigned char * packet = new unsigned char[length];
	GameData * aGameData;
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_id);
	if(!boarddispatch)
	{
		qDebug("How can there not be a board dispatch, it just sent us a accept request match mode");
        delete[] packet;
		return;
	}
	aGameData = boarddispatch->getGameData();
	packet[0] = 0x8c;
	packet[1] = 0xc3;
	packet[2] = 0x08;
	packet[3] = 0x00;
	packet[4] = aGameData->game_code & 0x00ff;
	packet[5] = (aGameData->game_code >> 8);
	packet[6] = 0x00;
	packet[7] = 0x00;

	encode(packet, 0x8);
	qDebug("Sending accept request match mode");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending request match mode packet");
	delete[] packet;
	//FIXME this isn't starting thematch timers properly FIXME
	startMatchTimers(boarddispatch->getBlackTurn() ? (aGameData->black_name == getUsername()) : (aGameData->white_name == getUsername()));
}

void CyberOroConnection::sendDeclineRequestMatchMode(unsigned int game_id)
{
	sendEndgame12Msg(game_id, 0x0ab4);
}

void CyberOroConnection::sendTimeLoss(unsigned int game_id)
{
	sendEndgame12Msg(game_id, 0xf6b3);
	killActiveMatchTimers();
}

void CyberOroConnection::sendRemoveStones(unsigned int game_code, const MoveRecord * move)
{
	unsigned int length = 0x10;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0xe2;
	packet[1] = 0xb3;
	packet[2] = 0x10;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = game_code & 0x00ff;
	packet[7] = (game_code >> 8);
	packet[8] = our_player_id & 0x00ff;	//why our id again??
	packet[9] = (our_player_id >> 8);
	//packet[10] = (move.number & 0x00ff);		//what the hell are these?
	//packet[11] = (move.number >> 8);		// FIXME scores???
	if(move->flags == MoveRecord::REMOVE)
		packet[12] = 0x01;		//probably unremove bit
	else	//if(move->flags == MoveRecord::UNREMOVE)
		packet[12] = 0x02;
	packet[13] = 0x00;
	
	packet[14] = move->x;
	packet[15] = move->y;
	
//e2 b3 10 00 ec 05 fa 01 ec 05 01 00 01 00 03 04
//e2 b3 10 00 ec 05 fa 01 ec 05 98 66 01 00 03 10
#ifdef RE_DEBUG
	printf("remove stones packet: ");
	for(unsigned int i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0x0c);
	qDebug("Sending remove stones");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending remove stones packet");
	delete[] packet;
}

/* Looks like we send this, for instance, as white if we pass
 * and then receive a black pass */
void CyberOroConnection::sendEnterScoring(unsigned int game_code)
{
	sendEndgame12Msg(game_code_to_number[game_code], 0xc9b3);
}

void CyberOroConnection::sendDoneScoring(unsigned int game_code, unsigned short opp_id)
{
	unsigned int length = 14 + (deadStonesList.size() * 2) + (deadStonesList.size() ? 2 : 0);
	unsigned char * packet = new unsigned char[length];
	int i;
	bool receivedOppDone = match_negotiation_state->receivedDoneCounting();

	if(receivedOppDone)
	{
		packet[0] = 0xe7;
		packet[1] = 0xb3;
	}
	else
	{
		packet[0] = 0xf1;
		packet[1] = 0xb3;
	}
	packet[2] = length & 0x00ff;
	packet[3] = (length >> 8);
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = game_code & 0x00ff;
	packet[7] = (game_code >> 8);
	/* I think bytes 8 and 9 are opponents id if we haven't
	 * received a done from them yet and ours if we have. FIXME */
	if(receivedOppDone)
	{
		packet[8] = our_player_id & 0x00ff;
		packet[9] = (our_player_id >> 8);
	}
	else
	{
		packet[8] = opp_id & 0x00ff;
		packet[9] = (opp_id >> 8);
	}
	//packet[8] = our_player_id & 0x00ff;	//why our id again??
	//packet[9] = (our_player_id >> 8);
	//packet[10] = (move.number & 0x00ff);		//what the hell are these?
	//packet[11] = (move.number >> 8);		// FIXME scores???
	//packet[10] = 0x00;		//possible scores, random?
	if(receivedOppDone)
	{
		unsigned short done_response = match_negotiation_state->getCountingVerification();
		packet[10] = done_response & 0x00ff;
		packet[11] = (done_response >> 8);
	}
	else
	{
		//FIXME what the hell?
		//packet[10] = 0x82;		//these seems to be kept...
		//packet[11] = 0x45;		//should be random? = 63?
		/* I'm going to assume that these are random, to ensure that scores can't be spoofed.
		 * i.e., we should check their response */
		packet[10] = rand() % 256;
		packet[11] = rand() % 256;
		match_negotiation_state->setCountingVerification(packet[10] + (packet[11] << 8));
	}
	
	packet[12] = 0x01;
	packet[13] = deadStonesList.size();
	if(deadStonesList.size())
	{
		/* We're supposed to send the last one */
		packet[14] = deadStonesList[deadStonesList.size() - 1].x;
		packet[15] = deadStonesList[deadStonesList.size() - 1].y;
		for(i = 0; i < (int)deadStonesList.size(); i++)
		{
			packet[16 + (i * 2)] = deadStonesList[i].x;
			packet[17 + (i * 2)] = deadStonesList[i].y;
		}
	}
	
//f1 b3 18 00 ec 05 fa 01 ec 05 98 82 01 04 03 10
//11 0f 11 05 03 04 03 10
#ifdef RE_DEBUG
	printf("done scoring packet: ");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0x0c);			//doublecheck that scoring works FIXME
	qDebug("Sending done scoring");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending done scoring");
	delete[] packet;
	
	if(receivedOppDone)
	{
		match_negotiation_state->setDoneCounting();
		sendMatchResult(game_code);
	}
	else
		match_negotiation_state->sendDoneCounting();
}

/* I don't think we're allowed to send this in score phase or
 * after opponent has pressed done or something */
void CyberOroConnection::sendResign(unsigned int game_code)
{
	sendEndgame12Msg(game_code_to_number[game_code], 0xb0b3);
}

void CyberOroConnection::sendAdjournRequest(void)
{
	sendEndgame12Msg(match_negotiation_state->getGameId(), 0xb5b3);
}

void CyberOroConnection::sendAdjourn(void)
{
	sendEndgame12Msg(match_negotiation_state->getGameId(), 0xbab3);
}

void CyberOroConnection::sendRefuseAdjourn(void)
{
	sendEndgame12Msg(match_negotiation_state->getGameId(), 0xbfb3);
}
	
	
/* So far, we know this is sent by client that responded
 * to done or by loser.  The margin seems to be ignored
 * by opposing client but maybe its registered by
 * server. */
void CyberOroConnection::sendMatchResult(unsigned short game_code)
{
	unsigned int length = 20;
	unsigned char * packet = new unsigned char[length];
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for match result send", game_code_to_number[game_code]);
        delete[] packet;
		return;
	}
	/* We need to notify boarddispatch of finished game so we can
	 * get the full result */
	boarddispatch->recvResult(0);
	GameData *aGameData = boarddispatch->getGameData();
	packet[0] = 0x80;
	packet[1] = 0xbb;
	packet[2] = 0x14;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = game_code & 0x00ff;
	packet[7] = (game_code >> 8);
	packet[8] = 0x03;	//?
	if(aGameData->white_name == getUsername())	//doublecheck
		packet[9] = 0x01;
	else
		packet[9] = 0x00;
	if(!aGameData->fullresult)
	{
		qDebug("No result on game data to send result");
		return;
	}
	float margin = aGameData->fullresult->winner_score - aGameData->fullresult->loser_score;
	packet[10] = (int)margin & 0x00ff;
	packet[11] = ((int)margin >> 8);
	packet[12] = 0x1a;	//?? 04
	packet[13] = 0x00;
	packet[14] = 0x00;	//?? 01
	packet[15] = 0x00;
	packet[16] = 0x00;	//?? 01
	packet[17] = 0x00;
	packet[18] = 0x00;
	packet[19] = 0x00;
	
	encode(packet, 0x14);
	qDebug("Sending match result");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending match result");
	delete[] packet;
}

/* It seems like this appears like a match invite if opponent closes
 * the window */
void CyberOroConnection::sendRematchRequest(void)
{
    if(!room_were_in)
    {
        qDebug("Trying to send rematch request but not in a room");
        return;
    }
	unsigned int length = 10;
	unsigned char * packet = new unsigned char[length];
	BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for rematch send", room_were_in);
		return;
	}
	GameData * gr = boarddispatch->getGameData();
	packet[0] = 0x69;
	packet[1] = 0xc3;
	packet[2] = 0x0a;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = gr->opponent_id & 0x00ff;
	packet[7] = (gr->opponent_id >> 8);
	packet[8] = 0x12;	//?
	packet[9] = 0x00;	//?
	
	encode(packet, 0xa);
	qDebug("Sending rematch request");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending rematch request");
	delete[] packet;
	
	/* Since end of game resets the match state */
	PlayerListing * player = getDefaultRoom()->getPlayerListing(gr->opponent_id);
	match_negotiation_state->sendRematch(player);
}

void CyberOroConnection::sendRematchAccept(void)
{
    if(!room_were_in)
    {
        qDebug("Trying to send rematch accept but we aren't in room");
        return;
    }
	unsigned int length = 10;
	unsigned char * packet = new unsigned char[length];
	BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for rematch accept send", room_were_in);
		return;
	}
	GameData * gr = boarddispatch->getGameData();
	packet[0] = 0x6e;
	packet[1] = 0xc3;
	packet[2] = 0x0a;
	packet[3] = 0x00;
	packet[4] = gr->opponent_id & 0x00ff;
	packet[5] = (gr->opponent_id >> 8);
	packet[6] = our_player_id & 0x00ff;
	packet[7] = (our_player_id >> 8);
	packet[8] = 0x12;	//?	 is this a room?
	packet[9] = 0x00;	//?
	
	encode(packet, 0xa);
	qDebug("Sending rematch request accept");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending rematch request accept");
	delete[] packet;
	match_negotiation_state->sendRematchAccept();
	//prepare for rematch func?
}

/* I think this double as both our odd/even guess as well
 * as the number we send.  But I think... well I assume
 * that the challenged player is holding the stones out? */
void CyberOroConnection::sendNigiri(unsigned short game_code, bool /*odd*/)
{
    unsigned short room_number = game_code_to_number[game_code];
	BoardDispatch * boarddispatch = getIfBoardDispatch(room_number);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for nigiri send", room_number);
		return;
	}
    unsigned int length = 12;
    unsigned char * packet = new unsigned char[length];
	GameData * gr = boarddispatch->getGameData();
	packet[0] = 0xf5;
	packet[1] = 0xaf;
	packet[2] = 0x0c;
	packet[3] = 0x00;
	packet[4] = game_code & 0x00ff;
	packet[5] = (game_code >> 8);
	packet[6] = room_number & 0x00ff;
	packet[7] = (room_number >> 8);
	//packet[8] = rand() % 255;	//this is both odd and even
	/* If its 0x0_, it looks like its an odd choice
	 * 0x2_ looks like an even choice,
	 * then the second four bits seem to be the number that gets
	 * sent.  But we can still seemingly confuse the oro client,
	 * like we're supposed to send something else... it lets it
	 * play for the right turn, but the colors aren't set.  And
	 * then in addition, we can't seem to play, like we're supposed
	 * to send something else first
	 * But so its like the first 4 bits are our choice of odd 0 or even 2
	 * and then the second 4 bits are the number of stones that our
	 * opponent is holding.  But actually, I've seen 0, 2, and 3 sent!*/
	/* FIXME I think we can say definitively that its simpler than we think
	 * but it definitely doesn't work */
	packet[8] = (rand() % 0xd) + 2;
	packet[8] |= (rand() % 0x19) + 1;		//19? d???  I guess it doesn't matter but...
	we_send_nigiri = true;
	
	/* I've seen the new version of this here work at least for one color */
	//if((((packet[8] & 0xe0) >> 4) % 2) == ((packet[8] & 0x1f) % 2))	 
	//if(((((packet[8] & 0xe0) >> 5) + 1) % 2) == ((packet[8] & 0x1f) % 2)) 
	if((!(packet[8] & 0x20)) == ((packet[8] & 0x1f) % 2)) 
	{
		//0x38	even 24 we play black
#ifdef RE_DEBUG
		qDebug("nigiri successful");
#endif //RE_DEBUG
		boarddispatch->recvKibitz(QString(), "Opponent plays white");
		if(gr->black_name != getUsername())
			boarddispatch->swapColors();
		else
			boarddispatch->swapColors(true);
	}
	else
	{
		packet[8] |= 0x80;	//nigiri failure we play white
		//0xaf even 15 we play white
#ifdef RE_DEBUG
		qDebug("nigiri failure");
#endif //RE_DEBUG
		boarddispatch->recvKibitz(QString(), "Opponent plays black");
		if(gr->white_name != getUsername())
			boarddispatch->swapColors();
		else
			boarddispatch->swapColors(true);
	}
	//QMessageBox::information(0 , tr("sending niri"), tr("sending ") + QString::number(packet[8], 16) + tr(" nigiri."));
	
	packet[9] = 0x00;	//?
	packet[10] = 0x00;
	packet[11] = 0x00;
	
	/* Its possible to send this and confuse the ORO client 
	 * actually, if we send this, when we are challenging,
	 * it thinks that the ORO client sent it.. ?!?! */
#ifdef RE_DEBUG
	printf("nigiri packet: ");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0xc);
	qDebug("Sending nigiri");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending nigiri");
	delete[] packet;
	
	gr = boarddispatch->getGameData();	//get again
	/* FIXME don't think we start until second move */
	startMatchTimers(gr->black_name == getUsername());
	
	// we seem to immediately send keep alive here
	if(gr->black_name == getUsername())
		sendGameUpdate(gr->game_code);
}

/* As in leave a played or in room game?  Maybe rooms as well? 
 * Double check, all those zeroes are suspicious. 
 * Actually, probably same as sendObserve message for joining
 * a game? room?  but with no possible password and no id.
 * Yeah, leave is basically the same thing as joining a room
 * except it joins the 00 room,  I guess the lobby. 
 * Also seems like we can observe after joining other rooms
 * without error, it just moves us. */
void CyberOroConnection::sendLeave(const GameListing *)
{
	/* Find why leaving a joined game sends two leaves... */
	if(room_were_in == 0)
	{
		qDebug("Attempted to sendLeave but in lobby!");
		return;
	}
#ifdef RE_DEBUG
	qDebug("Sending leave");
#endif //RE_DEBUG

	sendJoinRoom(0, NULL);
	
	setRoomNumber(0);
}

void CyberOroConnection::requestAccountInfo(void)
{
	//QuickConnection * c = new QuickConnection(getQSocket(), NULL, this, QuickConnection::sendRequestAccountInfo);
    metaserverQC = new QuickConnection(hostname, port, NULL, this, QuickConnection::sendRequestAccountInfo);
}

char * CyberOroConnection::sendRequestAccountInfo(int * size, void *)
{
	*size = 0x1a;
	char * packet = new char[*size];
	packet[0] = 0x88;
	packet[1] = 0x27;
	packet[2] = 0x1a;
	packet[3] = 0x00;
	
	packet[4] = 0x40;
	packet[5] = 0x00;
	writeZeroPaddedString(&(packet[6]), getUsername(), 10);
	packet[16] = 0x43;
	packet[17] = 0x00;
	packet[18] = 0x12;
	packet[19] = 0x0f;
	packet[20] = 0xdc;
	packet[21] = 0x02;
	for(int i= 22; i < *size; i++)
		packet[i] = 0x00;
#ifdef RE_DEBUG
	printf("requesting account info:\n");
	for(int i = 0; i < (int)*size; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	return packet;
}

void CyberOroConnection::handleAccountInfoMsg(int size, char * msg)
{
	unsigned short message_type = msg[1] + (msg[0] << 8);
	
	msg += 4;
	size -=4;
	switch(message_type)
	{
		case 0x8129:
			handleFriends((unsigned char *)msg, size);
			metaserverQC->deleteLater();
			metaserverQC = 0;
			break;
		case 0x7c29:
			//7c29 1a00 789e 0800 8800 b502 e0b0 887c
			//78de 0800 0100 0000 709e
			
			//7c29 1a00 000b 0900 ffff ffa0 00ff ffff
			//b502 ffff ffe0 ffff ffb0 ffff ff88 7c00
			//4b09 0001 0000 00ff ffff f80a
		default:
#ifdef RE_DEBUG
			msg -= 4;	//FIXME
			printf("Strange account info: %02x%02x: \n", msg[0], msg[1]);
			for(int i = 2; i < (int)size + 4; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
	}
}

void CyberOroConnection::sendLogin(void)
{
	//term code could be version, probably just junk
	unsigned char term_code[2] = { 0x57, 0x04 }; //= { 0x5b, 0x02 };
	int length = 0x1a;	//10 + user and pass padded to 10
	unsigned char * packet = new unsigned char[length];
	unsigned char * p = packet;
	int i;
	
	p[0] = 0x0b; //= 0x6e;
	p[1] = 0xe1; //= 0xe2;
	p[2] = length;
	p[3] = 0x00;
	p += 4;

	writeZeroPaddedString((char *)p, username, 10);
	p += 10;
	
	writeZeroPaddedString((char *)p, password, 10);
	p += 10;
	for(i = 0; i < 2; i++)
		p[i] = term_code[i];
#ifdef RE_DEBUG
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG		
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending login packet to host");
	delete[] packet;
}

void CyberOroConnection::sendInvitationSettings(bool invite)
{
	unsigned int length = 0x0c;
	unsigned char * packet = new unsigned char[length];
	packet[0] = 0x9a;
	packet[1] = 0x65;
	packet[2] = 0x0c;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0x00;
	packet[7] = 0x00;
	packet[8] = (invite ? ALLOW_ALL_INVITES : IGNORE_ALL_INVITES);
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;	//weird byte???

#ifdef RE_DEBUG
	printf("Sending invite settings: ");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, 0xc);
	qDebug("Sending invitation settings\n");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending invitation settings");
	delete[] packet;
}

/* These four require separate connection to meta
 * server! */
void CyberOroConnection::addFriend(PlayerListing * player)
{
	if(metaserverQC)
	{
		qDebug("Already connected to accounts server, please wait.");
		//silent no error might be okay
		return;
	}
    metaserverQC = new QuickConnection(hostname, port, (void *)player, this, QuickConnection::sendAddFriend);
	NetworkConnection::addFriend(player);
}

char * CyberOroConnection::sendFriendsBlocksMsg(int * size, void *p, enum FriendsBlocksMsgType f)
{
	PlayerListing * player = (PlayerListing *)p; 
	*size = 0x1a;
	char * packet = new char[*size];
	if(f == fbm_addFriend || f == fbm_addBlock)
		packet[0] = 0x13;
	else
		packet[0] = 0x77;
	packet[1] = 0x29;
	packet[2] = (*size & 0xff);
	packet[3] = (*size >> 8);
	if(f == fbm_addFriend || f == fbm_removeFriend)
		packet[4] = 0x01;
	else
		packet[4] = 0x02;
	packet[5] = 0x00;
	writeZeroPaddedString((char *)&(packet[6]), getUsername(), 10);
	writeZeroPaddedString((char *)&(packet[16]), player->name, 10);
	return packet;
}

char * CyberOroConnection::sendAddFriend(int * size, void * p)
{
	return sendFriendsBlocksMsg(size, p, fbm_addFriend);
}

#ifdef RE_DEBUG
void CyberOroConnection::recvFriendResponse(int size, char * msg)
#else
void CyberOroConnection::recvFriendResponse(int size, char * /*msg*/)
#endif //RE_DEBUG
{
	if(size != 8)
	{
		qDebug("received friends list response of strange size: %d\n", size);
		//return;
	}
	//1f2708001a270000
#ifdef RE_DEBUG
	int i;
	printf("recvFriendResponse:\n");
	for(i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	metaserverQC->deleteLater();
	metaserverQC = 0;
}

void CyberOroConnection::removeFriend(PlayerListing * player)
{
	if(metaserverQC)
	{
		qDebug("Already connected to accounts server, please wait.");
		//silent no error might be okay
		return;
	}
    metaserverQC = new QuickConnection(hostname, port, (void *)player, this, QuickConnection::sendRemoveFriend);
	NetworkConnection::removeFriend(player);
}

char * CyberOroConnection::sendRemoveFriend(int * size, void * p)
{
	return sendFriendsBlocksMsg(size, p, fbm_removeFriend);
}

void CyberOroConnection::addBlock(PlayerListing * player)
{
	if(metaserverQC)
	{
		qDebug("Already connected to accounts server, please wait.");
		//silent no error might be okay
		return;
	}
    metaserverQC = new QuickConnection(hostname, port, (void *)player, this, QuickConnection::sendAddBlock);
	NetworkConnection::addBlock(player);
}

void CyberOroConnection::removeBlock(PlayerListing * player)
{
	if(metaserverQC)
	{
		qDebug("Already connected to accounts server, please wait.");
		//silent no error might be okay
		return;
	}
    metaserverQC = new QuickConnection(hostname, port, (void *)player, this, QuickConnection::sendRemoveBlock);
	NetworkConnection::removeBlock(player);
}

char * CyberOroConnection::sendAddBlock(int * size, void * p)
{
	return sendFriendsBlocksMsg(size, p, fbm_addBlock);
}

char * CyberOroConnection::sendRemoveBlock(int * size, void * p)
{
	return sendFriendsBlocksMsg(size, p, fbm_removeBlock);
}

void CyberOroConnection::sendDisconnectMsg(void)
{	
	if(!codetable)
		return;	
	unsigned char * packet = new unsigned char[8];
	packet[0] = 0x1c;
	packet[1] = 0x52;
	packet[2] = 0x08;
	packet[3] = 0x00;
	packet[4] = our_player_id & 0x00ff;
	packet[5] = (our_player_id >> 8);
	packet[6] = 0x00;		//zeroes for fun?? FIXME
	packet[7] = 0x00;
	
	encode(packet, 8);
#ifdef RE_DEBUG
	qDebug("Sending disconnect msg");
#endif //RE_DEBUG
	if(write((const char *)packet, 8) < 0)
		qWarning("*** failed sending disconnect packet");
	delete[] packet;
}

/* This is a challenge-response message.  The meta-server validates
 * the name and password and sends a chunk of something which is
 * then passed to whatever servers the client connects to */
 /* This isn't a challenge response probably but just a server
  * connect packet... that's my guess. 
  * It seems like this is sent when we switch servers mid session so
  * yeah...*/
void CyberOroConnection::sendChallengeResponse(void)
{
	int i;
	unsigned char * p;
	unsigned char packet[120];
	p = packet;
	/* The opening four bytes are operated on by some kind of cyclic
	 * xor in a table... we're still working it out.  Also below
	 * may have two options as well. DONE */
	p[0] = 0x07;
	p[1] = 0x52;
	p[2] = 0x78;
	p[3] = 0x00;
	encode(p, 6);
	p += 4;
	for(i = 0; i < 106; i++)
		p[i] = challenge_response[i];
	p += 106;
	/* I think this 05 can change but I'm not certain. */
	p[0] = 0x05;
	p[1] = 0x00;
	p += 2;
	/* These 4 bytes are literally random, we can put
	 * anything here */
	p[0] = 0x01;
	p[1] = 0x02;
	p[2] = 0x03;
	p[3] = 0x04;
	p += 4;
	p[0] = 0x31;
	p[1] = 0x2e;
	p[2] = 0x33;
	p[3] = 0x38;
#ifdef RE_DEBUG
	printf("Sending challenge response\n");
	for(i = 0; i < 120; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, 120) < 0)
		qWarning("*** failed sending challenge response");
}

/* I think the codetable is used so that the session can't
 * be violated or forged.  In other words, since we're the
 * only client with the right codetable and the right IV
 * position in that code table as by all the messages we've
 * sent, no one can, for instance, send a move packet with
 * our name and make us make a move that we don't intend.
 * Seems that there could be simpler ways to do it and I
 * do wonder if its partly for obfuscation still.*/
void CyberOroConnection::encode(unsigned char * h, unsigned int cycle_size)
{
	int i;
	unsigned char a = 0xff;
	int IV3 = cycle_size;	//maybe we're passed this?
	
	IV3 -= 3;
	/* sub edx, eax jl */
	IV3++;
	for(i = 0; i < IV3; i++)
	{
		h[i] = h[i] ^ codetable[codetable_IV];	
		a = h[i] ^ a;
		codetable_IV++;
		codetable_IV %= 1000;	//double check!!! could be /!!
		if(codetable_IV == 0)	//should probably be % 1000 ==, not %=!!!
		{
			codetable_IV2++;
			qDebug("incrementing codetable_IV2: %d", codetable_IV2);
		}
	}
	if(cycle_size >= 8)
	{
		/* 16000f points to the byte before the header for some reason */
		h[cycle_size - 1/* + [codetable +34d3]*/] = a;
	}
	if(codetable_IV2 >= 10)
	{
		if(codetable_IV >= 900)
		{
			/* looks like we zero codetable_IV2 and
			 * then do some crazy repeated shit */
		}
	}
}

void CyberOroConnection::handleMessage(QString)
{
}

/* We may convert everything here to unsigned char *s, delete the old...
 * we're still sort of hung up on all the things we were doing
 * to work with the IGS code. */
void CyberOroConnection::handleMessage(unsigned char * msg, unsigned int size)
{
	unsigned short message_type = msg[1] + (msg[0] << 8);
	//message_type = msg[0];
	//message_type >>= 8;
	//message_type += msg[1];
	/* Third byte is likely the size of the message meaning multiple
	 * messages per packet, etc. */
	/*if((message_type & 0x00ff) == 0x00b3)
	{*/
#ifdef RE_DEBUG
		//seems unnecessary even for debug
		/*printf("****: \n");
		for(unsigned int i = 0; i < size; i++)
			printf("%02x", msg[i]);
		printf("\n");*/
#endif //RE_DEBUG
	/*}*/
	msg += 4;
	size -=4;
	switch(message_type)
	{
		case ORO_CODETABLE:
			handleCodeTable(msg, size);
			break;
		case 0x2652:
			handleConnected(msg, size);
			break;
		case ORO_PLAYERLIST:
			handlePlayerList(msg, size);
			break;
		/*case 0xfa00:
			printf("likely game message\n");
			break;*/
		case 0x2c56:
#ifdef RE_DEBUG
			printf("0x2c56: some kind of logon");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case ORO_BROADCASTLIST:
			//boardcast games list
			handleBroadcastGamesList(msg, size);
			break;
		case ORO_ROOMLIST:
			handleRoomList(msg, size);
			break;
		case 0xb261:
#ifdef RE_DEBUG
			printf("0xb261: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xee61:
#ifdef RE_DEBUG
			printf("0xee61: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xdf61:
#ifdef RE_DEBUG
			printf("0xdf61: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xe461:
			handleMsg2(msg, size);
			break;
		case 0x62f2:	//0x94 bytes, this looks a LIVE GO game listing entry
			/*0000 0600 5046 cc13 b91d 0303 0000 3c03 0200 4873 6965 682e 592e 4d00 0000
			0000 0000 5375 7a75 6b69 2e41 0000 0000 0000 0000
			8ed3 88cb 9ddf 0000 0000 0000 0000 0000 97e9 96d8 95e0 0000
			0000 0000 0000 0000 0303 c4c4 0305 0000 0000 0000 
			91e6 3237 8afa 8f97 97ac 967b 88f6 9656 90ed 91e6 318b
			c700000000000000000000000000000000000000000000000100001f*/
#ifdef RE_DEBUG
			printf("0x62f2: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x68f6:
			//could this mean connected?
#ifdef RE_DEBUG
			printf("0x68f6: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case ORO_GAMELIST:
			handleGamesList(msg, size);
			break;
		case ORO_PLAYERROOMJOIN:
			handlePlayerRoomJoin(msg, size);
			break;
		case 0x0456:
			handlePlayerConnect(msg, size);
			break;
		case 0x5ac3:
			handleGamePhaseUpdate(msg, size);
			break;
		case 0x50c3:
			handleMsg3(msg, size);
			break;
		case 0x38f4:
			/* FIXME, this needs to maybe open a board,
			 * maybe not... but something needs to */
			handleBettingMatchStart(msg, size);
			break;
		case 0x56f4:
			handleBettingMatchResult(msg, size);
			break;
		case ORO_BROADCASTMOVE:	//betting match?
		case ORO_MOVE:
			handleMove(msg, size);
			break;
		case ORO_UNDO:
			handleUndo(msg, size);
			break;
		case ORO_DECLINEUNDO:
			handleDeclineUndo(msg, size);
			break;
		case ORO_ACCEPTUNDO:
			handleAcceptUndo(msg, size);	//generic undo?
			break;
		case ORO_MOVELIST:
			handleMoveList(msg, size);
			break;
		case ORO_BROADCASTMOVELIST:
			handleMoveList2(msg, size);
			break;
		case 0xd2af:
			handleMatchOpened(msg, size);
			break;
		case 0x5fc3:
			handleResumeMatch(msg, size);
			break;
		//case 0x38f4:
		//	handleObserveAfterJoining(msg, size);
		//	break;
		case 0x9a65:
			handleInvitationSettings(msg, size);
			break;
		case 0xf5af:
			handleNigiri(msg, size);
			break;
		case 0x1e7d:	//match invite accept
			handleMatchOfferPending(msg, size);
			break;
		case 0x147d:
			handleAcceptMatchInvite(msg, size);
			break;
		case 0x459c:
#ifdef RE_DEBUG
			// we send this as finish observing
			printf("0x459c: finish observing?");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x2279:
			handleMatchInvite(msg, size);
			break;
		case 0x1a7d:
#ifdef RE_DEBUG
			printf("0x1a7d: game starting? ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x0a7d:
			handleNewRoom(msg, size);
			break;
		case 0x1a81:
			handleGameMsg(msg, size);
			break;
		case ORO_CREATEROOM:	//was WantsMatch
			handleCreateRoom(msg, size);
			break;
		case ORO_RESIGN:
			handleResign(msg, size);
			break;
		case ORO_REMOVESTONES:
			handleRemoveStones(msg, size);
			break;
		case ORO_STONESDONE:
			handleStonesDone(msg, size);
			break;
		case 0xe7b3:		//response to our done
			handleScore(msg, size);
			break;
		case 0x8fbb:
#ifdef RE_DEBUG
			//thinking this is just a match result
			printf("0xtime: 8fbb: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x85bb:
			handleNewGame(msg, size);	//really game result?
			break;
		case 0xecb3:	//something to do with done messages in matches
#ifdef RE_DEBUG
			/* Its possible, see out151, that one of these
			 * bytes is just supposed to be passed back
			 * or even that the ecb3 is like an angry,
			 * response to a bad done message*/
			printf("0xecb3: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		/* One of the below 6 indicates we've joined an adjourned
	         * game that's already in score mode */
		/* Actually I think eede just means we've resumed the game.
	         * we're just going to make the moveList aware of it */
		case 0xeede:
			// this looks like an end observer list msg
			/* Its probably this one, its immediately followed by
			 * a game id*/
#ifdef RE_DEBUG
			printf("0xeede: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xdfde:
			//probably not observer list!!!
			handleObserverList(msg, size);
			break;
		case 0xadde:
#ifdef RE_DEBUG
			printf("0xadde: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xa8de: 
#ifdef RE_DEBUG
			printf("a8de: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xc4fe:
#ifdef RE_DEBUG
			printf("0xc4fe: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xedfd:
#ifdef RE_DEBUG
			printf("0xedfd: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x06fe:
#ifdef RE_DEBUG
			printf("0x06fe: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x01fe:
#ifdef RE_DEBUG
			printf("0x01fe: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x0bfe:
#ifdef RE_DEBUG
			printf("0x0bfe: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		//82c3: 0a00b60ca23a6816
		//***
		case 0x82c3:
#ifdef RE_DEBUG
			printf("0x82c3: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xc4b3:
			handleRequestCount(msg, size);
			break;
		case 0xceb3:
			handleRejectCountRequest(msg, size);
			break;
		case 0x05b4:
			handleRequestMatchMode(msg, size);
			break;
		case 0x0ab4:
			handleRejectMatchModeRequest(msg, size);
			break;
		case 0x20cb:
		{
#ifdef RE_DEBUG
			PlayerListing * p = getDefaultRoom()->getPlayerListing(msg[0] + (msg[1] << 8));
			printf("0x20cb %s: ", (p ? p->name.toLatin1().constData() : "-"));
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
		}
			break;
		case ORO_TIMELOSS:
			handleTimeLoss(msg, size);
			break;
		case 0x69c3:
			handleRematchRequest(msg, size);
			break;
		case 0x6ec3:
			handleRematchAccept(msg, size);
			break;
		case ORO_SETPHRASECHAT:
			handleSetPhraseChatMsg(msg, size);
			break;
		case 0x327d:
			handleMatchDecline(msg, size);
			break;
		case 0xbc61: 
#ifdef RE_DEBUG
			printf("0xbc61: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x162b: 
#ifdef RE_DEBUG
			printf("0x162b: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x60f4: 
#ifdef RE_DEBUG
			printf("0x60f4: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x78c3: 
#ifdef RE_DEBUG
			printf("0x78c3: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0xa479: 
			handleDeclineMatchInvite(msg, size);
			break;
		case 0x285a:	//maybe a special tournament announcement or something
		case ORO_SERVERANNOUNCE:
			handleServerAnnouncement(msg, size);
			break;
		case ORO_SERVERANNOUNCELINK:
			handleServerAnnouncementwithLink(msg, size);
			break;
		case 0x0c2b: 
#ifdef RE_DEBUG
			printf("0x0c2b: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x39cb: 
		{
#ifdef RE_DEBUG
			PlayerListing * p = getDefaultRoom()->getPlayerListing(msg[0] + (msg[1] << 8));
			printf("0x39cb %s: ", (p ? p->name.toLatin1().constData() : "-"));
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
		}
			break;
		case 0x3ff2: 
#ifdef RE_DEBUG
			printf("0x3ff2: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x482b: 	//bad challenge response response?
			/* I think this might indicate a full server */
			QMessageBox::information(0, tr("Server full"), tr("Server full, try another"));
			if(reconnectToServer() < 0)
			{
				qDebug("User canceled");
                setState(CANCELED);
				return;
			}
#ifdef RE_DEBUG
			printf("0x482b: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x022b:
#ifdef RE_DEBUG
			/* This looks like its trying to correct the protocol
			 * or something, like its giving us either the msg type
			 * we should have sent or the last one sent */
			printf("0x022b: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x1356:
#ifdef RE_DEBUG
			printf("0x1356 likely player disconnect: ");
			for(unsigned int i = 0; i < size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case ORO_ENTERSCORING:
			handleEnterScoring(msg, size);
			break;
		case ORO_ADJOURNREQUEST:
			handleAdjournRequest(msg, size);
			break;
		case ORO_ADJOURNDECLINE: 
			handleAdjournDecline(msg ,size);
			break;
		case ORO_ADJOURN:
			handleAdjourn(msg, size);
			break;
		case ORO_LOBBYCHAT:
			handleServerRoomChat(msg, size);
			/* We'll eventually need to translate these into unicode characters
			 * and it looks like they have a player id that comes before */
			break;
		case ORO_ROOMCHAT:
			handleRoomChat(msg, size);
			break;
		case ORO_PERSONALCHAT:
			handlePersonalChat(msg, size);
			break;
		case 0xe1af:		//FIXME maybe not
			handleGameEnded(msg, size);
			break;
		case 0x4a9c:
			handlePlayerDisconnect2(msg, size);
			break;
		case ORO_MATCHOFFER:
			handleMatchOffer(msg, size);
			break;
		default:
#ifdef RE_DEBUG
			msg -= 4;	//FIXME
			printf("Two bytestrange: %02x%02x: \n", msg[0], msg[1]);
//#ifdef RE_DEBUG
			for(unsigned int i = 2; i < size + 4; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
	}
}

/* Last byte is a kind of initialization vector, IV */
void CyberOroConnection::handleCodeTable(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	int i;
	
	if(size != 1002)
		qDebug("code table message of size: %d", size);
	if(codetable)
		delete[] codetable;
	codetable = new unsigned char[1000];
	for(i = 0; i < 1000; i++)
		codetable[i] = p[i];
	codetable_IV = p[1000];
	codetable_IV2 = 0;

	sendChallengeResponse();
}

void CyberOroConnection::handleConnected(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	if(size != 10)
		qDebug("handleConnected size: %d", size);
	our_player_id = p[0] + (p[1] << 8);
#ifdef RE_DEBUG
	qDebug("our id: %02x%02x %02x\n", p[0], p[1], p[2]);
#endif //RE_DEBUG
	our_special_byte = p[2];
	p += 6;
	
	onReady();		//is this okay?
	
	//try this here
	//sendInvitationSettings(true);	//for now
	// FIXME, toggle is doing this now, but is it kosher with initial state?
	// i.e., when we first join what happens? where does that get checked?
}

#ifdef NEWPROTOCOL
/*OROPlayerListPacket::OROPlayerListPacket(char * p)
{
	data = p;
}*/

//void * ZeroPaddedString::operator new[] (size_t s) { size = s; };

void CyberOroConnection::handlePlayerList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char name[11];
	int players;
	unsigned char rankbyte, invitebyte;
	int i;
	unsigned short id;
	OROPlayerListPacket packet((char *)msg);
	Room * room = getDefaultRoom();
	PlayerListing * newPlayer = new PlayerListing();
	PlayerListing * aPlayer;
	PlayerListing * backPlayer;
	newPlayer->online = true;
	newPlayer->info = "??";
	newPlayer->playing = 0;
	newPlayer->observing = 0;
	newPlayer->idletime = "-";
	
	players = p[2];

#ifdef RE_DEBUG
	printf("%02x%02x\n", p[0], p[1]);	
#endif //RE_DEBUG	
	printf("new players: %d\n", packet.h.playerRecords);
	p += 2;
	//676c696c696e6e000000 770a 16a4 02c3 0000 9b72 e607 e407 0000 0056
	while(p < (msg + size))
	{
#ifdef RE_DEBUG
		for(i = 0; i < 28; i++)
			printf("%02x", p[i]);
		printf("\n");
#endif //RE_DEBUG
		strncpy((char *)name, (char *)p, 10); name[10] = 0x00;
#ifdef RE_DEBUG
		printf("%s ", name);
#endif //RE_DEBUG
		p += 10;
		id = p[0] + (p[1] << 8);
		aPlayer = room->getPlayerListing(id);
		if(!aPlayer)
		{
			aPlayer = newPlayer;
			newPlayer->playing = 0;
		}
		aPlayer->id = id;
		if(strncmp((char *)name, "jguest", 6) == 0 || strncmp((char *)name, "jpguest", 7) == 0)
			aPlayer->hidden = true;
		else
			aPlayer->hidden = false;
		//aPlayer->name = QString((char*)name);
		aPlayer->name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		
		p += 2;
		//a byte here seems to be a send msg byte, or something
		aPlayer->specialbyte = p[0];
		//p++;
#ifdef RE_DEBUG
		for(i = 0; i < 8; i++)
			printf("%02x", p[i]);
#endif //RE_DEBUG
		// That's right, its the pro bit 
		rankbyte = p[1];
		if(rankbyte & 0x40)
		{
			aPlayer->pro = true;
		}
		else
			aPlayer->pro = false;
		
		//lost interest, its not the game
		//aPlayer->observing = p[2];	//FIXME, what is this?
		//this fourth byte here is country I think.
		// but its really unreliable, almost like its something
		// that clients from these countries have as a setting
		// or maybe part of a bit field.
		aPlayer->country = getCountryFromCode(p[3]);
		//fifth byte is their open status
		invitebyte = p[4];
		aPlayer->nmatch_settings = QString::number(p[0]) + QString(" ") + QString::number((p[1])) + QString(" ") + QString::number(p[2]) + QString(" ") + QString::number(p[3]);
		p += 6;
		aPlayer->rank_score = p[0] + (p[1] << 8); 
		if(aPlayer->pro)
			aPlayer->rank = QString::number(rankbyte & 0x1f) + QString("dp");
		else
			aPlayer->rank = rating_pointsToRank(aPlayer->rank_score);
		aPlayer->info = getStatusFromCode(invitebyte, aPlayer->rank);
		aPlayer->wins = p[2] + (p[3] << 8);
		aPlayer->losses = p[4] + (p[5] << 8);
		p += 6;
#ifdef RE_DEBUG
		printf(" RP: %d W/L: %d/%d ", aPlayer->rank_score, aPlayer->wins, aPlayer->losses);
		for(i = 0; i < 4; i++)
			printf("%02x", p[i]);
		printf("\n");
#endif //RE_DEBUG
		p += 3;
		p++;
		room->recvPlayerListing(aPlayer);
		
		playerlist_received++;
		if(playerlist_skipnumber != PLAYERLIST_SKIPNUMBER_UNSET
				 && playerlist_skipnumber < playerlist_received)
		{
#ifdef RE_DEBUG
			printf("Playerlist received %d skip number %d\n", playerlist_received, playerlist_skipnumber);
#endif //RE_DEBUG
			/* get next room with observers */
			GameListing * gamelisting = room->getGameListing(playerlist_roomnumber);
			if(!gamelisting)
				goto label_playerlist_nogame;
			if(gamelisting->observers > 0 && playerlist_observernumber == gamelisting->observers)
			{
				playerlist_observernumber = 0;
				gamelisting = room->getGameListing(++playerlist_roomnumber);
				if(!gamelisting)
					goto label_playerlist_nogame;
			}
			else if(gamelisting->observers > 0)
			{
				playerlist_observernumber++;
			}
			else
			{
				while(gamelisting->observers == 0)
				{
					playerlist_roomnumber++;
					gamelisting = room->getGameListing(playerlist_roomnumber);
					if(!gamelisting)
						goto label_playerlist_nogame;
				}
			}
			backPlayer = room->getPlayerListing(aPlayer->id);
			if(backPlayer->playing != playerlist_roomnumber)
			{
				/* add room to player, ORO is one room per player */
				backPlayer->observing = playerlist_roomnumber;
				/* add player to room */
				gamelisting->observer_list.push_back(backPlayer);
#ifdef RE_DEBUG
				printf("Adding player %s %d to game %d\n", backPlayer->name.toLatin1().constData(), backPlayer->id, playerlist_roomnumber);
#endif //RE_DEBUG
			}
		}
label_playerlist_nogame:
		/* If the game can't be found it either means that the player is not in a room
		* or it means we don't have the room yet because that list hasn't come in.*/
		
		backPlayer = room->getPlayerListing(aPlayer->id);
		playerlist_inorder.push_back(backPlayer);
	}
	delete newPlayer;
#ifdef RE_DEBUG
	printf("*** Players %d\n", players);
#endif //RE_DEBUG
}
#endif //NEWPROTOCOL

#ifdef RE_DEBUG
//#define PLAYERLIST_DEBUG
#endif //RE_DEBUG
/* I've finally realized how ORO does this safely.  It waits until its received
 * everything, before it displays anything.  Worries me a bit, the way we do it but...
 * we can make it safe... I just wonder if it could be better, especially if it would
 * be cleaner to have a "loading" dialog block user input until everything is up */
void CyberOroConnection::handlePlayerList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
    unsigned char name[11];
    unsigned char rankbyte, invitebyte;
	unsigned short id;
	Room * room = getDefaultRoom();
    PlayerListing * aPlayer;

    int players = p[2];

	p += 2;
	//676c696c696e6e000000 770a 16a4 02c3 0000 9b72 e607 e407 0000 0056
	while(p < (msg + size))
    {
        strncpy((char *)name, (char *)p, 10);
        name[10] = 0x00;
		p += 10;
		id = p[0] + (p[1] << 8);
		aPlayer = room->getPlayerListing(id);
		if(strncmp((char *)name, "jguest", 6) == 0 || strncmp((char *)name, "jpguest", 7) == 0)
			aPlayer->hidden = true;
		else
			aPlayer->hidden = false;
        aPlayer->name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		
		p += 2;
		//a byte here seems to be a send msg byte, or something
		aPlayer->specialbyte = p[0];
        // That's right, its the pro bit
		rankbyte = p[1];
        aPlayer->pro = bool(rankbyte & 0x40);
		
		//lost interest, its not the game
		//aPlayer->observing = p[2];	//FIXME, what is this?
		//this fourth byte here is country I think.
		// but its really unreliable, almost like its something
		// that clients from these countries have as a setting
		// or maybe part of a bit field.
		aPlayer->country = getCountryFromCode(p[3]);
		//fifth byte is their open status
		invitebyte = p[4];
		aPlayer->nmatch_settings = QString::number(p[0]) + QString(" ") + QString::number((p[1])) + QString(" ") + QString::number(p[2]) + QString(" ") + QString::number(p[3]);
		p += 6;
		aPlayer->rank_score = p[0] + (p[1] << 8); 
		if(aPlayer->pro)
			aPlayer->rank = QString::number(rankbyte & 0x1f) + QString("dp");
		else
			aPlayer->rank = rating_pointsToRank(aPlayer->rank_score);
		aPlayer->info = getStatusFromCode(invitebyte, aPlayer->rank);
		aPlayer->wins = p[2] + (p[3] << 8);
		aPlayer->losses = p[4] + (p[5] << 8);
		p += 6;
		p += 3;
		p++;
        emit playerListingReceived(aPlayer);
    }
}

QString CyberOroConnection::rating_pointsToRank(unsigned int rp)
{
	int rem = rp % 1000;
	rp -= rem;
	rp /= 1000;
	if(rp < 26)
	{
		rp = 26 - rp;
		return QString::number(rp) + QString("k");
	}
	else
	{
		rp -= 25;
		return QString::number(rp) + QString("d");
	}
	return QString("?");
}

unsigned int CyberOroConnection::rankToScore(QString rank)
{
	QString buffer = rank;
	//buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
	buffer.replace(QRegExp("[pdk]"), "");
	int ordinal = buffer.toInt();
	unsigned int rp;

	if(rank.contains("k"))
		rp = (26 - ordinal) * 1000;
	else if(rank.contains("d"))
		rp = 25000 + (ordinal * 1000);
	else if(rank.contains("p"))
		rp = 36000 + (ordinal * 1000);
	else
		return 0;
	
	return rp;
}

/* FIXME What are the other bits for ?!?!?!*/
QString CyberOroConnection::getCountryFromCode(unsigned char code)
{
	code &= 0x0f;
	switch(code)
	{
		case 1:
			return "KOREA";
			break;
		case 2:
			return "CHINA";
			break;
		case 3:
			return "JAPAN";
			break;
		case 4:
			return "INTL";
			break;
		case 5:
			return "THAILAND";
			break;
		case 6:
			//that blue flag...
			return "UN?";
			break;
		case 7:	
			return "USA";
			break;
		case 8:
			return "HONGKONG";
			break;
		case 9:
			return "SINGAPORE";
			break;
		case 0xa:
			return "CANADA";
			break;
		case 0xb:
			return "GERMANY";
			break;
		case 0xc:
			return "FRANCE";
			break;
		case 0xd:
			return "AUSTRALIA";
			break;
		case 0xe:
			return "RUSSIA";
			break;
		case 0xf:
			return "UKRAINE";
			break;
	}
	return QString("-");
}

//c661
void CyberOroConnection::handleRoomList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned int number_of_games;
	unsigned short number;
	unsigned int i;
	PlayerListing * player;
	GameListing * aGameListing;
    Room * room = getDefaultRoom();
	bool be_adding_observers;

	if(playerlist_skipnumber != PLAYERLIST_SKIPNUMBER_UNSET &&
		roomlist_observers < playerlist_received)
		be_adding_observers = true;
	else
		be_adding_observers = false;
	
	number_of_games = p[0] + (p[1] << 8);
	p += 4;
#ifdef RE_DEBUG
	printf("c661 Number of games in this list: %d\n", number_of_games);
	printf("roomlist_observers %d versus %d\n", roomlist_observers, playerlist_received);
#endif //RE_DEBUG
	// 4, 4 for one player, 4 for the second player, another 4
	while(number_of_games--)
	{
		number = p[0] + (p[1] << 8);
		if(number == 0)
		{
			/* There's no 0th record except maybe total number of
			 * games and those 01 tagged records are weird, like
			 * their either empty or they're for particular
			 * players... they might be player stats but they
			 * don't show up in official ORO app so who knows.*/
			/* FIXME, these are probably important and I just
			 * have no idea. */
			unsigned char * q = p;
#ifdef RE_DEBUG
			printf("0record:\n");	//unnecessary even for debug
#endif //RE_DEBUG
			/* I think the third and fourth bytes here are
			 * the number of players not in rooms, or to
			 * skip from player lists before adding players
			 * to games */
			playerlist_skipnumber = p[2] + (p[3] << 8);
			if(playerlist_received >= playerlist_skipnumber)
				be_adding_observers = true;
			for(i = 0; i < 4; i++)
			{
#ifdef RE_DEBUG
				printf("%02x%02x%02x%02x ", q[0], q[1], q[2], q[3]);
#endif //RE_DEBUG
				q += 4;
			}
#ifdef RE_DEBUG
			printf("\n");
#endif //RE_DEBUG
			p += 16;
			continue;
		}
		aGameListing = room->getGameListing(number);
        aGameListing->running = true;
		unsigned char * q = p;
		for(i = 0; i < 4; i++)
		{
#ifdef RE_DEBUG
			printf("%02x%02x%02x%02x ", q[0], q[1], q[2], q[3]);
#endif //RE_DEBUG
			q += 4;
		}
#ifdef RE_DEBUG
		printf("\n");
#endif //RE_DEBUG
		if(p[4] & 0x70)
			aGameListing->isLocked = true;		//I don't think this is always the case
		//p[4] & 0x70 = locked game password 
		/*
		3c review game records 7d replay
		16 waiting for pair match team even + stronger
		2a even pair game ongoing with p[5] == c0
		02 bit could be pair game, but which, even or stronger?
		*/
		/* I think phase is or can be unreliable for broadcast
		 * games, maybe others */
		/*FIXME, setting observers here skews setAttached*/
		aGameListing->observers = p[2];
		int phase = getPhase(p[6]);
		if(phase != -1)
			aGameListing->moves = phase;
		else
			aGameListing->moves = 0;
		aGameListing->owner_id = p[8] + (p[9] << 8);
		if(!aGameListing->white)
		{
            player = room->getPlayerListing(aGameListing->owner_id);
			aGameListing->white = player;
		}
		else
			player = 0;
		/* These are really like room owners */
		if(!aGameListing->black)
		{
			//new sets this to 0
			//FIXME so why even have a conditional here?
			//also does this get unset if its found to
			//be a game later?
			aGameListing->_black_name = getRoomTag(p[4]);
			aGameListing->isRoomOnly = true;
		}
		else
			aGameListing->FR = getRoomTag(p[4]);
		
		room->recvGameListing(aGameListing);
		aGameListing = room->getGameListing(aGameListing->number);
		aGameListing->observer_list.clear();

		if(player)
		{
			//after recv
			//FIXME
			setAttachedGame(player, aGameListing->number);
		}
		p += 16;
#ifdef RE_DEBUG
		printf("Observers: %d\n", aGameListing->observers);
#endif //RE_DEBUG
		/* Because player and room listing messages apparently
		 * come in any order, probably because there's some
		 * underlying MS database that spits all this stuff out
		 * from separate places, even separate servers... : */
		if(be_adding_observers)
		{
			for(i = 0; i < aGameListing->observers; i++)
			{
				if(roomlist_observers + playerlist_skipnumber < playerlist_inorder.size())
				{
					player = playerlist_inorder[roomlist_observers + playerlist_skipnumber];
					if(player->playing != number)
					{
						/* add room to player, ORO is one room per player */
						player->observing = number;
						/* add player to room */
						aGameListing->observer_list.push_back(player);
#ifdef RE_DEBUG
						printf("Adding player %s %d to game %d\n", player->name.toLatin1().constData(), player->id, number);
#endif //RE_DEBUG
					}
					playerlist_observernumber++;
					if(playerlist_observernumber == aGameListing->observers)
					{
						playerlist_observernumber = 0;
						playerlist_roomnumber = number + 1;	//at least
					}
				}
				roomlist_observers++;
			}
		}
		else
			roomlist_observers += aGameListing->observers;
	}
	if(p != msg + size)
		qDebug("handleRoomList: doesn't match size %d", size);
	//FIXME try without
}

/* We might FIXME replace the return on this with a QString
 * when we alter the list view to handle different columns.
 * But for now, moves is pretty accurate for the field */
int CyberOroConnection::getPhase(unsigned char byte)
{
	switch(byte)
	{
		case 0:
			return 0;
					//open
			break;
		case 1:	
			return 30;
					//early
			break;
		case 2:
			return 50;
					//middle
			break;
		case 3:
			return 130;
					//end
			break;
		case 11:
					//looks like wished opponent
		default:
			return -1;
			break;
	}
}

QString CyberOroConnection::getRoomTag(unsigned char byte)
{
	switch(byte)
	{
		case 0x1f:
		case 0x3d:
					//normal and broadcast in progress
			return QString();
			break;
		case 0x04:
			return QString("** chat 04");
			break;
		case 0x1a:
			return QString("** chat 1a");
			break;
		case 0x70:
			//likely chat 0 locked?
			return QString("** chat 70");
			break;
		case 0x3c:
			return QString("** review 3c");
			break;
		case 0x19:
			return QString("** chat 19");
			break;
		case 0x1c:
			return QString("** quit? 19");
			break;
		case 0x0b:
			return QString("** wished 0b");
			break;
		case 0x0f:
			return QString("** one color 0f");
			break;
		default:
			return (QString("** ") + QString::number(byte, 16));
			break;		
	}
}

//f861  These are broadcasted games I think.  They're not players on the service
void CyberOroConnection::handleBroadcastGamesList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * game_record;
	unsigned int number_of_games;
	int number;
	unsigned char name[11];
	name[10] = 0x00;
#ifdef RE_DEBUG
	unsigned int i;
#endif //RE_DEBUG
	
	Room * room = getDefaultRoom();
    GameListing * aGameListing;
	
	number_of_games = p[0] + (p[1] << 8);
	p += 2;
#ifdef RE_DEBUG
	printf("Number of games: %d\n", number_of_games);	
#endif //RE_DEBUG
	while(number_of_games--)
	{
		game_record = p;
		number = p[0] + (p[1] << 8);
		aGameListing = room->getGameListing(number);
        aGameListing->running = true;
		p += 2;
		aGameListing->game_code = game_record[2] + (game_record[3] << 8);
		p += 2;
		strncpy((char *)name, (char *)&(game_record[4]), 10);
		aGameListing->black = 0;
		//aGameListing->_black_name = QString((char *)name);
		aGameListing->_black_name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		p += 10;
		strncpy((char *)name, (char *)&(game_record[14]), 10);
		aGameListing->white = 0;
		//aGameListing->_white_name = QString((char *)name);
		aGameListing->_white_name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		
		p += 10;
#ifdef RE_DEBUG
		printf("br %02x wr %02x\n", p[0], p[1]);
#endif //RE_DEBUG
		if((game_record[24] & 0xC0) == 0xC0)
		{
			aGameListing->_black_rank = QString::number(game_record[24] & 0x0f) + "p";
			aGameListing->_black_rank_score = 34000 + ((game_record[24] & 0x0f) * 1000);
		}
		else if((game_record[24] & 0xA0) == 0xA0)
		{
			aGameListing->_black_rank = QString::number(game_record[24] & 0x0f) + "d";
			aGameListing->_black_rank_score = 25000 + ((game_record[24] & 0x0f) * 1000);
		}
		if((game_record[25] & 0xC0) == 0xC0)
		{
			aGameListing->_white_rank = QString::number(game_record[25] & 0x0f) + "p";
			aGameListing->_white_rank_score = 34000 + ((game_record[25] & 0x0f) * 1000);
		}
		else if((game_record[25] & 0xA0) == 0xA0)
		{
			aGameListing->_white_rank = QString::number(game_record[25] & 0x0f) + "d";
			aGameListing->_white_rank_score = 25000 + ((game_record[25] & 0x0f) * 1000);
		}
		p += 2;
		/* The next two bytes are the country codes for each player,
		 * but I'm not sure where to display them. FIXME */
		/* [K] [C] [J], let's not kid ourself about the pro game countries */
		QString country = getCountryFromCode(game_record[27]);
		aGameListing->_black_name += ("[" + country[0] + "]");
		country = getCountryFromCode(p[1]);
		aGameListing->_white_name += ("[" + country[0] + "]"); 
		p += 2;
		
#ifdef RE_DEBUG
		printf("broadcast %d: ", aGameListing->number);
		for(i = 0; i < 48; i++)
			printf("%02x", (p - 28)[i]);
		printf("\n"); 
#endif //RE_DEBUG
		aGameListing->game_code = aGameListing->number;	//just for these broadcasts
		
		/* Looks like if the last two 4 bytes are 0xf190 ed00, then
		 * its game-over */
		p += 28;
		/* This seems to be wrong */
		if(p[0] == 0xf1 && p[1] == 0x90 && p[2] == 0xed && p[3] == 0x00)
		{
			aGameListing->FR = "Ended";
			//also, use moves = -1 I think FIXME
		}
		//aGameListing->FR = QString::number(p[0], 16) + QString::number(p[1], 16) + QString::number(p[2], 16) + QString::number(p[3], 16);
		p += 4;
		aGameListing->isRoomOnly = false;
		aGameListing->isBroadcast = true;	
		//to prevent broadcast games from closing FIXME
        emit gameListingReceived(aGameListing);
	}
    if(p != (msg + size))
		qDebug("bad p: line: %d", __LINE__);
}

/* Turns out, in this initial, game joining list, its possible to get ids
 * from players we haven't received yet.  So we need to, either postpone
 * posting the list or... something... until we get the full
 * player list.  If this is only a problem initially... and I think
 * that is generally the case... but... I have to think about how
 * to handle this. (maybe if we knew total number of players?) FIXME FIXME*/
 //da61
void CyberOroConnection::handleGamesList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned int number_of_games;
	unsigned int id;
#ifdef RE_DEBUG
	unsigned int i;
#endif //RE_DEBUG
	
	Room * room = getDefaultRoom();
	PlayerListing * white, * black;
	GameListing * aGameListing;
    int number;

	/* Looks like the [] QList index error might be coming from this
	 * and its fatal!!! so watch out... probably number of games is
	 * wrong. We need to differentiate the different game list msg
	 * types
	 * Its like the second time we have to use the temp trick, it
	 * bugs out... maybe the reference doesn't actually change...*/
	number_of_games = p[0] + (p[1] << 8);
	p += 2;
	/*printf("Number of games: %d\n", number_of_games);
	for(i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
	return;*/

	while(number_of_games-- && p < (msg + size - 9))	//check probably unnecessary FIXME
	{
		number = p[0] + (p[1] << 8);
        aGameListing = room->getGameListing(number);
        aGameListing->running = true;
		p += 2;
#ifdef RE_DEBUG
		printf("da61: %d %02x%02x\n", aGameListing->number, p[0], p[1]);
		for(i = 0; i < 8; i++)
			printf(" %02x", p[i]);
		printf("\n");
#endif //RE_DEBUG
		p += 2;
		aGameListing->game_code = p[0] + (p[1] << 8);
		game_code_to_number[aGameListing->game_code] = aGameListing->number;		//weird?
		p += 2;
		
		id = p[0] + (p[1] << 8);
		black = room->getPlayerListing(id);
        aGameListing->black = black;

        p += 2;
		id = p[0] + (p[1] << 8);
		white = room->getPlayerListing(id);
        aGameListing->white = white;
		
        p += 2;
#ifdef RE_DEBUG
		if(white && black)
		printf("Adding listing for %s vs %s\n", 
		       white->name.toLatin1().constData(), 
		       black->name.toLatin1().constData());
#endif //RE_DEBUG
		aGameListing->isRoomOnly = false;
		//aGameListing->observers = 2;	//the players
		room->recvGameListing(aGameListing);
		//after recv
#ifdef FIXME //maybe we need this here?!?
		owner = room->getPlayerListing(aGameListing->owner_id);
		if(owner != black)
			setAttachedGame(black, aGameListing->number);
		if(owner != white)
			setAttachedGame(white, aGameListing->number);
		//detach owner if any, players are attached alread
		if(owner && owner != black && owner != white)
			setAttachedGame(owner, 0);
#endif //FIXME
		setAttachedGame(black, aGameListing->number);
		setAttachedGame(white, aGameListing->number);
	}
    if(p != (msg + size))
        printf("gameslist3 error: %d %ld\n", size, p - msg);
}

//0456
void CyberOroConnection::handlePlayerConnect(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char name[11];
	name[10] = 0x00;
	unsigned char name2[11];
	name2[10] = 0x00;
	unsigned char rankbyte;
	unsigned short id;
	Room * room = getDefaultRoom();
    PlayerListing * aPlayer;

	if(size != 42)
		qDebug("handlePlayerConnect of size: %d\n", size);
	strncpy((char *)name, (char *)p, 10);
#ifdef RE_DEBUG
	printf("0456 %s connected, ", name);
#endif //RE_DEBUG
	
	p += 10;
	strncpy((char *)name2, (char *)p, 10);
#ifdef RE_DEBUG
	printf("or player %s, ", name2);
#endif //RE_DEBUG
	
	
	p += 10;	//name again;
#ifdef RE_DEBUG
	printf("id: %02x%02x\n", p[0], p[1]);
#endif //RE_DEBUG
	id = p[0] + (p[1] << 8);
    aPlayer = room->getPlayerListing(id);
	aPlayer->id = id;
	//aPlayer->name = (char *)name;
	if(strncmp((char *)name, "jguest", 6) == 0 || strncmp((char *)name, "jpguest", 7) == 0)
		aPlayer->hidden = true;
	else
		aPlayer->hidden = false;
	aPlayer->name = serverCodec->toUnicode((const char *)name2, strlen((const char *)name2));
	aPlayer->notnickname = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
	/* It actually looks like this catches more unicode foreign names, then the
	* second name.  But I get the feeling that one is the text username or
	* something... not sure */
	/* Yeah, FIXME, the first name appears to allow asian characters, the second is just
	 * username.  We might want an option to turn them on and off */
	//aPlayer->name = (char *)name2;
	//aPlayer->name = serverCodec->toUnicode((const char *)name2, strlen((const char *)name2));
		
	/* I think we want to take the first name if we take the second
	* here, I think the second is allowed to be unicode or something...
	* done FIXME, still not sure of this, we should
	* really just add unicode support !!!*/
	
	
	// are you my special msg byte??? FIXME
	p += 2;
	aPlayer->specialbyte = p[0];
	p++;
	rankbyte = p[0];
	if(rankbyte & 0x40)
		aPlayer->pro = true;
	p++;
	aPlayer->wins = p[0] + (p[1] << 8);
	aPlayer->losses = p[2] + (p[3] << 8);
	p += 4;
	p += 4;
	aPlayer->rank_score = p[0] + (p[1] << 8);
	if(aPlayer->pro)
		aPlayer->rank = QString::number(rankbyte & 0x1f) + QString("dp");
	else
		aPlayer->rank = rating_pointsToRank(aPlayer->rank_score);
#ifdef RE_DEBUG
	printf(" RP: %d W/L: %d/%d ", aPlayer->rank_score, aPlayer->wins, aPlayer->losses);
#endif //RE_DEBUG
	p += 2;
#ifdef RE_DEBUG
	for(int i = 0; i < 8; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	aPlayer->country = getCountryFromCode(p[2]);
	aPlayer->info = getStatusFromCode(p[3], aPlayer->rank);
	aPlayer->nmatch_settings = QString("-") + QString::number(msg[23]) + " " + QString::number(p[0]) + QString(" ") + QString::number((p[1])) + QString(" ") + QString::number(p[2]) + QString(" ") + QString::number(p[3]) + QString::number(p[4]) + QString(" ") + QString::number((p[5])) + QString(" ") + QString::number(p[6]) + QString(" ") + QString::number(p[7]);
	p += 8;	//other data;
	aPlayer->observing = 0;		//necessary?
	//aPlayer->playing = 0;	//necessary?
    emit playerListingReceived(aPlayer);
}

/* This is necessary as a safety precaution.  Maybe if we were sure
 * about all aspects of protocol, but for now, let's prevent the
 * crashes. */
/* Starting to think that we shouldn't modify observers here.
 * entering should be enough.  That and the listing on connect */
 /* CyberORO does have multi 1:N games which means that there's possibly
  * some issues where there might be a player attached to multiple games */
 /* Also, FIXME, there's no real need for an observing and a playing with ORO,
  * we should just use observing to determine what room they're in.  Its confusing
  * otherwise.  Although I guess "playing" can be used to mean their name is in the
  * listing.  All of these places where we close empty rooms need to be brought
  * together FIXME*/
void CyberOroConnection::setAttachedGame(PlayerListing * const player, unsigned short game_id)
{
	GameListing * game;
	if(player->playing && player->playing != game_id)
	{
        game = getDefaultRoom()->getGameListing(player->playing);
        /* This is silly, unless its a chat room, its still the same game,
             * we shouldn't remove the players */
        if(game->white == player)
        {
            game->_white_name = game->white->name;
            game->_white_rank = game->white->rank;
            game->white = 0;
#ifdef FIXME			//chat only
            if(game->black)
            {
                /* This is so the name is always
                     * in the same column */
                game->white = game->black;
                game->black = 0;
            }
            else
            {
                if(game->observer_list.size())
                {
                    game->white = game->observer_list[0];
                    /* FIXME actually a game should never have no name, so I think when black leaves, black's name
                         * is replaced by white, but in our case, since we have white than black, we'd the opposite
                         * and then we have to pull up observernames, either alphabetical or in the order they joined
                         * or something.  Should also probably change flags to chat or something */
                }
            }
#endif //FIXME	
        }
        else if(game->black == player)
        {
            if(game->black->name == "0")
                qDebug("black name equals 0!!!!");
            game->_black_name = game->black->name;
            game->_black_rank = game->black->rank;
            game->black = 0;
        }
        /*if(!g->observers)
            {
                //broadcast games never go down, so there's FIXME
                //issues here
                qDebug("Closing game %d, no observers", g->number);
                g->running = false;
                getDefaultRoom()->recvGameListing(g);
            }*/
	}
	else if(player->playing == game_id)
		return;			//nothing to do
#ifdef RE_DEBUG
	printf("Moving %s %p from %d to %d\n", player->name.toLatin1().constData(), player, player->playing, game_id);
#endif //RE_DEBUG
	player->playing = game_id;

	/* Doesn't help.  Looks like observer counts do not include the players playing themselves which makes some sense
	 * but we need them in our little list and, well the whole thing is iffy FIXME */
	/* Okay, we're adding observer counts here, we can check it later.  They're not in the data we get, so that's an issue */
	
	if(game_id && player->observing != game_id)
	{
		game = getDefaultRoom()->getGameListing(game_id);
        //game->observers++;
        game->observer_list.push_back(player);
    }

	if(player->observing && player->observing != game_id)
		removeObserverFromGameListing(player);
	else
		player->observing = game_id;
}

//0x55c3
void CyberOroConnection::handleSetPhraseChatMsg(unsigned char * msg, unsigned int size)
{
	if(size != 16)
	{
		qDebug("set phrase chat msg of strange size: %d", size);
	}
	unsigned char * p = msg;
	//2a02ffff 0000 0000 63ea000083cedc9
	unsigned short id = p[0] + (p[1] << 8);
    unsigned short setphrase_code;
	PlayerListing * player = getDefaultRoom()->getPlayerListing(id);
	/* Our chat appears back in all the rooms were in???
	 * why?? 
	 * FIXME room_were_in must not be the whole picture */
	//if(player->name == getUsername())
	//	return;		//to prevent duplicate chats from us
    p += 4;
	bool in_room = p[0];	//doublecheck, could potentially mean its from us
	p += 4;
	
#ifdef RE_DEBUG
	printf("Directed id %d, room_were_in %d\n", directed_id, room_were_in);
#endif //RE_DEBUG
/* They translated a lot more phrases, but these are the major ones
 * it looks like */
//p[1] byte is category? and then p[0] is specific msg	
	setphrase_code = p[0] + (p[1] << 8);
	std::map<unsigned short, QString>::iterator it = ORO_setphrase.find(setphrase_code);
	if(it != ORO_setphrase.end())
	{
		if(room_were_in && in_room)
		{
			BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
			if(!boarddispatch)
			{
				// might just be a room
				if(console_dispatch)
					console_dispatch->recvText(player->name + ": " + it->second);
			}
			else
				boarddispatch->recvKibitz(player->name + "[" + player->rank + "]", it->second);
		}
		else
		{
			if(console_dispatch)
				console_dispatch->recvText(player->name + "[" + player->rank + "]: " + it->second);
		}
	}
	else
	{
		if(room_were_in && in_room)
		{
			BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
			if(!boarddispatch)
			{
				if(console_dispatch)
					console_dispatch->recvText(player->name + ": " + QString::number(setphrase_code, 16));
			}
			else
				boarddispatch->recvKibitz(player->name + "[" + player->rank + "]", QString::number(setphrase_code, 16));
		}
		else
		{
			if(console_dispatch)
				console_dispatch->recvText(player->name + "[" + player->rank + "]: " + QString::number(setphrase_code, 16));
		}
	}
#ifdef RE_DEBUG
	printf("0x55c3: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//0xf659
void CyberOroConnection::handleServerAnnouncement(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	
#ifdef RE_DEBUG
	printf("0xf659: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	p += 2;

	/* FIXME, likely first two bytes are something else */
	QString u;
		
	u = serverCodec->toUnicode((const char *)p, size - 2);
			//u = codec->toUnicode(b, size - 4);
	if(console_dispatch)
		console_dispatch->recvText("*** " + u);
}

//645a
void CyberOroConnection::handleServerAnnouncementwithLink(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	
#ifdef RE_DEBUG
	printf("645a: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	p += 2;

	/* FIXME, likely first two bytes are something else */
	QString u;
	QString link;
	unsigned int len = 0;
	while((msg[8 + len] || msg[9 + len])&& len + 8 < 260)
		len += 2;
	printf("Server anon len %d\n", len);
	u = serverCodec->toUnicode((const char *)&(msg[8]), len);
			//u = codec->toUnicode(b, size - 4);
	len = 0;
	while(msg[264 + len] && len + 264 < size)
		len++;
	printf("Server anon len2 %d\n", len);
	link = QString((const char *)&(msg[264]));	//not unicode
	if(console_dispatch)
		console_dispatch->recvText("*** " + u + ": " + link);
}

void CyberOroConnection::handleServerRoomChat(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * text = new unsigned char[size - 3];
	//unsigned int text_size = (size - 2) / 2;
	//unsigned short * text = new unsigned short[text_size];
	unsigned short player_id = p[0] + (p[1] << 8);
#ifdef RE_DEBUG
	int i;
#endif //RE_DEBUG
	/*printf("** msg size: %d: ", size);
	for(i = 0; i < (int)size; i++)
		printf("%02x", p[i]);
	printf("\n");*/
	
	p += 2;
	//there's two encoding bytes here... 00 86 looks like ascii
	
	p += 2;
	Room * room = getDefaultRoom(); //3
	strncpy((char *)text, (char *)p, size - 4); text[size - 4] = 0x00;
	/*for(i = 0; i < text_size; i++)
	{
		text[i] = p[0] + (p[1] << 8);
		p += 2;
	}*/
	PlayerListing * player = room->getPlayerListing(player_id);
    QString u = serverCodec->toUnicode((const char *)text, size - 4);
    //u = codec->toUnicode(b, size - 4);
    if(console_dispatch)
        console_dispatch->recvText(player->name + "[" + player->rank + "]: " + u);

    delete[] text;
}

//EC 59 1B 00 D3 04 00 7E 68 65 6C 6C 6F 20 63 68  .Y.....~hello ch
//61 74 20 72 6F 6F 6D 20 31 35 30                 at room 150
//0xec59
void CyberOroConnection::handleRoomChat(unsigned char * msg, unsigned int size)
{
	/* Maybe this is game chat only ?!??! Its weird... I guess
	 * its just for whatever room you're in and you can only be in
	 * one room at a time, a game room, a club room, but only one room.*/
	unsigned char * p = msg;
	
#ifdef RE_DEBUG
	int i;
#endif //RE_DEBUG
	/*printf("** msg size: %d: ", size);
	for(i = 0; i < (int)size; i++)
		printf("%02x", p[i]);
	printf("\n");*/
	Room * room = getDefaultRoom();
	const PlayerListing * player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	if(player->name == getUsername())
		return;		//block as echo
		
	//these two bytes are font or something 
	p += 2;
	// the rest is the text
	unsigned char * text = new unsigned char[size - 3];
	strncpy((char *)text, (char *)p, size - 4); text[size - 4] = 0x00;
	
	if(player)
	{
#ifdef RE_DEBUG
		printf("%s says to you ", player->name.toLatin1().constData());
		for(i = 0; i < (int)size - 4; i++)
			printf("%02x", text[i]);
		printf("\n");
#endif //RE_DEBUG
		
		QString u;
		
		u = serverCodec->toUnicode((const char *)text, size - 4);
		
		BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
		if(boarddispatch)
			boarddispatch->recvKibitz(player->name + "[" + player->rank + "]", u);
		else
		{
			qDebug("No boarddispatch for %d", room_were_in);
			if(console_dispatch)
				console_dispatch->recvText(player->name + "(" + QString::number(room_were_in) + "): " + u);
		}
	}
	else
		printf("unknown player says something");
	delete[] text;	
}

void CyberOroConnection::handlePersonalChat(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * text = new unsigned char[size - 7];
#ifdef RE_DEBUG
	unsigned int i;
	printf("** msg size: %d: ", size);
	for(i = 0; i < size; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	Room * room = getDefaultRoom();
	p += 4;
	PlayerListing * player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	p += 2;
	strncpy((char *)text, (char *)p, size - 8); text[size - 8] = 0x00;
	if(player)
	{
#ifdef RE_DEBUG
		printf("%s says to you ", player->name.toLatin1().constData());
		for(i = 0; i < size - 8; i++)
			printf("%02x", text[i]);
		printf("\n");
#endif //RE_DEBUG
        Talk * talk = getDefaultRoom()->getTalk(player);
		if(talk)
		{
			talk->recvTalk(QString((char *)text));
			talk->updatePlayerListing();
		}
		//if(console_dispatch)
		//	console_dispatch->recvText(player->name + "-> " + QString((char *)text));
	}
	else
		printf("unknown player says something");
	delete[] text;
}

//not disconnect, enter lobby? 0e56, really leaning towards enterlobby
// or enter anything
// this is definitely an actual enter room.  We might need to use this
// when we join a room, especially or in particular a broadcast room
// if the betting message doesn't otherwise indicate that we've actually
// joined.  Otherwise we get the move list and there's nothing tohandle it
// FIXME
void CyberOroConnection::handlePlayerRoomJoin(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	unsigned int id;
	unsigned short room_number;
	BoardDispatch * boarddispatch;
	GameListing * game;
	
	if(size != 4)
		qDebug("handlePlayerRoomJoin of size: %d\n", size);
	id = p[0] + (p[1] << 8);
	PlayerListing * aPlayer = room->getPlayerListing(id);
	room_number = p[2] + (p[3] << 8);
#ifdef RE_DEBUG
	printf("%s %02x%02x entering room %d\n", aPlayer->name.toLatin1().constData(), p[0], p[1], room_number);
#endif //RE_DEBUG
	/* We get this message before the boarddispatch is created, which
	 * means that we won't see ourselves entering the game.  If
	 * we wanted to fix this, we could create the boarddispatch
	 * possibly a little earlier, or just add our own name as
	 * a listing when we observe games.  Then there's matches
	 * also. FIXME */
	if(room_were_in)
	{
		if(aPlayer->observing == room_were_in)
		{
			boarddispatch = getIfBoardDispatch(room_were_in);
			if(boarddispatch)
				boarddispatch->recvObserver(aPlayer, false);
		}
		else if(room_number == room_were_in)
		{
			boarddispatch = getIfBoardDispatch(room_were_in);
			if(boarddispatch)
				boarddispatch->recvObserver(aPlayer, true);
		}
	}
	if(aPlayer->observing)
		removeObserverFromGameListing(aPlayer);
	aPlayer->observing = room_number;
	if(room_number)			//0 is lobby
	{
		game = room->getGameListing(room_number);
        game->observers++;
        game->observer_list.push_back(aPlayer);
        if(id == our_player_id)
		{
			if(game->gameData)		//only used on finished games where we won't receive info from server
			{
				boarddispatch = getBoardDispatch(room_number);
				boarddispatch->openBoard();	
			}
			setRoomNumber(room_number);
			if((boarddispatch = getIfBoardDispatch(room_number)))
				addObserverListToBoard(boarddispatch);
		}
	}
	/* We set to 0 here with the idea that they aren't entering
	 * their own game, but its a little weird FIXME */
	setAttachedGame(aPlayer, 0);
    emit playerListingReceived(aPlayer);
}

void CyberOroConnection::removeObserverFromGameListing(const PlayerListing * p)
{
	Room * room = getDefaultRoom();
	GameListing * game = room->getGameListing(p->observing);
#ifdef RE_DEBUG
	//printf("Removing observer from game %d with %d observers\n", game->number, game->observers);
#endif //RE_DEBUG
	if(game->observers == 0)
	{
		/* This can happen if, for instance, someone creates a broadcast game
		 * that hangs around with 0 observers, and then leaves it and joins
		 * another game.  We just don't want negative numbers */
		return;
	}
	game->observers--; 
	for(std::vector<PlayerListing *>::iterator oblistit = game->observer_list.begin();
		   oblistit != game->observer_list.end(); oblistit++)
	{
		if(*oblistit == p)
		{
			game->observer_list.erase(oblistit);
			break;
		}
	}
	if(game->observers == 0 && !game->isBroadcast)
	{
		/* We should probably increment an observer count here, and
		* decrement, and if it gets to 0, remove the room.  I don't
		* think there's another message that does it FIXME (but this
		* requires accurate observer counts. */
		game->running = false;
        emit gameListingReceived(game);
		/* No, oro tells us elsewhere to delete game */
		/* Not necessarily, its possible that if the game is fully over when everyone leaves
		 * its assumed that it gets deleted FIXME */
	}
}

//4a9c likely real disconnect or maybe not!!!
void CyberOroConnection::handlePlayerDisconnect2(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	unsigned int id;
	unsigned short room_id;
	if(size != 4)
		qDebug("handlePlayerDisconnect of size: %d\n", size);
	id = p[0] + (p[1] << 8);
	PlayerListing * aPlayer = room->getPlayerListing(id);
	p += 2;
	/* If this is the room id, its reversed... */
	room_id = p[0] + (p[1] << 8);
#ifdef RE_DEBUG
	printf("%s 4a9c: %02x%02x %d %d\n", aPlayer->name.toLatin1().constData(), p[0], p[1], room_id, aPlayer->observing);
#endif //RE_DEBUG
	
	if(aPlayer->observing)
	{
		room_id = aPlayer->observing;
		removeObserverFromGameListing(aPlayer);
	}
	else
		room_id = 0;

	BoardDispatch * boarddispatch = getIfBoardDispatch(room_id);
	if(boarddispatch)
	{
		// Player left a running game?  maybe observer though
		// but we need to check if stop clock is necessary
		boarddispatch->recvObserver(aPlayer, false);	//doublecheck
		GameData * gd = boarddispatch->getGameData();
		if(gd->gameMode == modeMatch && (aPlayer->name == gd->black_name || aPlayer->name == gd->white_name))
		{
			boarddispatch->recvKibitz(QString(), tr("%1 has left the room.").arg(aPlayer->name)); 
			if(gd->moves < 10)		//doublecheck, not 11?? FIXME
			{
				boarddispatch->recvKibitz(QString(), tr("Adjourned games with less than 10 moves are not counted."));
				GameResult aGameResult;
				aGameResult.result = GameResult::NOGAME;
				boarddispatch->recvResult(&aGameResult);
			}
			else
			{
				match_negotiation_state->opponentDisconnect();
				boarddispatch->stopTime();
				QString q = QString(tr("Opponent Disconnected"));
				boarddispatch->moveControl(q);
			}
		}
	}
	
    aPlayer->online = false;
    emit playerListingReceived(aPlayer);
}

//85bb
// I think these are result messages... I thought they were new game
// from people doing rematches... but there's no game codes in them...
// they could also be player updates I guess, but that's doubtful
void CyberOroConnection::handleNewGame(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned short player_id = p[0] + (p[1] << 8);
	
	if(size != 28)
		qDebug("85bb new game of size %d", size);
#ifdef RE_DEBUG
    Room * room = getDefaultRoom();
    PlayerListing * player;
    player = room->getPlayerListing(player_id);
    if(player)
		printf("%s ", player->name.toLatin1().constData());
	// 4th byte here is 01 for winner and 02 for loser
	// separate msg for each player result
	if(p[3] == 0x10)
		printf(" wins ");
	else if(p[3] == 0x20)
		printf(" loses ");
	else
		printf(" %02x ", p[3]);
	for(unsigned int i = 0; i < 28; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	// we probably don't send these during pass?
	if(player_id == our_player_id)
		killActiveMatchTimers();
	//GameListing * g = room->getGameListing();
	//g->running = false;
	//room->recvGameListing(g);
}

/* Below might not be observer list at all.... 
 * Maybe its for betting matches ?!?!? */
 //FIXME, yeah, not observer list
//0xdfde
void CyberOroConnection::handleObserverList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
    BoardDispatch * boarddispatch;
	
	if(!room_were_in)
	{
		qDebug("observer list but we're not in a room!");
		return;
	}
	boarddispatch = getIfBoardDispatch(room_were_in);
	if(!boarddispatch)
	{
		qDebug("no board dispatch for room were in");
		return;
	}
	
#ifdef RE_DEBUG
	printf("0xdfde: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
    // unsigned short game_code = p[0] + (p[1] << 8);
	p += 2;
    //unsigned short observer_list_index = p[0] + (p[1] << 8);
	p += 2;
	/*if(size != 10)
	{
		qDebug("GameEnded of strange size: %d", size);
		return;
	}*/
	p += 24;  //zeroes
	while(p < (msg + size - 20))
	{
		p += 8;
        // unsigned short id = p[0] + (p[1] << 8);
		/*aPlayer = room->getPlayerListing(id);
		if(aPlayer)
			boarddispatch->recvObserver(aPlayer, true);
		else*/
#ifdef RE_DEBUG
			printf("ol: %02x %02x\n", p[0], p[1]);
#endif //RE_DEBUG
		p += 12;
	}
}

/* What if this wasn't game ended but instead game started and we got it when
 * there were returns to match mode as well as rematches?!?! I'll try sending
 * it as an accept. FIXME*/
//0xe1af:
void CyberOroConnection::handleGameEnded(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned short game_code;
	unsigned int game_number;
	GameListing * game;
	Room * room = getDefaultRoom();
	if(size != 10)
	{
		qDebug("GameEnded of strange size: %d", size);
		return;
	}
	game_code = msg[0] + (msg[1] << 8);
	game_number = msg[2] + (msg[3] << 8);
	p += 4;
#ifdef RE_DEBUG
	//DOUBLECHECK
	printf("e1af for %d %d\n", game_code, game_number);
#endif //RE_DEBUG
	game = room->getGameListing(game_number);
	/* FIXME, what about owner_id???  this could be a room
	 * closed message */
	/* Room doesn't go away after game, one can still join it until
	 * everyone is gone */
#ifdef NOTREALLY
	/*if(game->white)
	{
		game->white->playing = 0;
		game->white = 0;	//just in case
	}
	if(game->black)
	{
		game->black->playing = 0;
		game->black = 0;	//just in case
	}
	game->running = false;
	room->recvGameListing(game);
	game_code_to_number.erase(game_number);	// good?*/
#endif //NOTREALLY
	//the rest are pretty repetitive, non-specific FIXME
#ifdef RE_DEBUG
	printf("0xe1af: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

/* I'm thinking 0a7d may be NewMatch and ca5d is NewRoom FIXME */
//0a7d
void CyberOroConnection::handleNewRoom(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	PlayerListing * black;
	PlayerListing * white;
	unsigned short id;
	unsigned short number;
    GameListing * aGameListing;
	
	if(size != 6 && size != 18)
	{
		qDebug("NewRoom of strange size: FIXME 18 %d", size);
		//FIXME we're having handleResume call this with size 18
		// fix that... 
		//return;
	}
	
	
	number = p[0] + (p[1] << 8);
	if(!number)
		qDebug("0a7d on number 0");
	aGameListing = room->getGameListing(number);
    p += 2;
	/* Why does ORO 0a7d have white then black, but 1a81 has
	 * black then white?  And what about numbers and game_codes?
	 * it seems like ORO protocol is based on two different programs,
	 * like there's legacy code in there.  And I complained about IGS
	 * so much.  I guess nothing is perfect.  And I'm lazy in this all
	 * the time too...
	 * Okay, actually, this seems to reverse, like maybe this just means
	 * they're in the same room or something and they're still deciding
	 * white versus black, or there's some nigiri thing... this white
	 * black DOESN'T matter. */
	/* Okay, since we changed the listing to handle numbers, game codes
	 * on board dispatches... what's changed here ??? FIXME */
	aGameListing->owner_id = p[0] + (p[1] << 8);
	aGameListing->observers = 0;	//okay?
	white = room->getPlayerListing(aGameListing->owner_id);
    /* This is redundant, but we won't receive entering msgs
     * for these players */
    setAttachedGame(white, aGameListing->number);
    white->observing = aGameListing->number;
    if(aGameListing->owner_id == our_player_id)
        setRoomNumber(aGameListing->number);
    aGameListing->observers++;
    aGameListing->observer_list.push_back(white);

	p += 2;
	id = p[0] + (p[1] << 8);
	black = room->getPlayerListing(id);
	if(!black)
	{
		printf("can't get black player: %02x%02x\n", p[0], p[1]);
		//return;		//not an issue
	}
	else
	{
		setAttachedGame(black, aGameListing->number);
		black->observing = aGameListing->number;
		if(id == our_player_id)
			setRoomNumber(aGameListing->number);
		aGameListing->observers++;
		aGameListing->observer_list.push_back(black);
	}
	/* FIXME, no recv, no game yet, maybe there should be. */
	aGameListing->white = white;
	aGameListing->black = black;
	
#ifdef RE_DEBUG
	if(newGameListing)
		printf("0a7d for game with no game code: %d %s size(%d) %s\n", aGameListing->number, white->name.toLatin1().constData(), size, black->name.toLatin1().constData());
#endif //RE_DEBUG
    emit gameListingReceived(aGameListing);
}

//1a81
void CyberOroConnection::handleGameMsg(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	unsigned short game_code;
	unsigned int handicap;
	unsigned char byte;
	PlayerListing * black;
	PlayerListing * white;
	PlayerListing * owner;
	GameListing * aGameListing = 0;
	if(size != 22)
	{
		qDebug("game msg of strange size %d", size);
	}
#ifdef RE_DEBUG
	printf("1a81 message: ");
#endif //RE_DEBUG
	game_code = p[0] + (p[1] << 8);		//used in joining games
#ifdef RE_DEBUG
	printf("%02x%02x {%02x ", p[0], p[1], p[2]);
#endif //RE_DEBUG
	p += 2;
	//0x1f
	p++;
	handicap = p[0];
	p++;
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < size - 4; i++)
		printf("%02x", p[i]);
#endif //RE_DEBUG
	byte = p[2];
	p += 2;
	
	black = room->getPlayerListing(p[0] + (p[1] << 8));
#ifdef RE_DEBUG
	if(black)
		printf("} game of %s %02x%02x", black->name.toLatin1().constData(), p[0], p[1]);
#endif //RE_DEBUG
	p += 2;
	white = room->getPlayerListing(p[0] + (p[1] << 8));
#ifdef RE_DEBUG
	if(white)
		printf(" and %s %02x%02x", white->name.toLatin1().constData(), p[0], p[1]);
#endif //RE_DEBUG
	p += 2;
#ifdef RE_DEBUG
	printf("\n");
#endif //RE_DEBUG
	if(!black || !white)
	{
		printf("%d gamecode game is missing black or white\n", game_code);
		return;
	}
	/* If there's a rematch, we might be able to just use
	 * the "attachedgame" to get the number */
	if(black->observing)
	{
		aGameListing = room->getGameListing(black->observing);
#ifdef RE_DEBUG
		if(aGameListing)
			printf("Found %d for this game\n", aGameListing->number);
#endif //RE_DEBUG	
	}
	if(white->observing && !aGameListing)
	{
		aGameListing = room->getGameListing(white->observing);
#ifdef RE_DEBUG
		if(aGameListing)
			printf("Found %d for this game\n", aGameListing->number);
#endif //RE_DEBUG
	}
	if(black->playing && !aGameListing)
	{
		aGameListing = room->getGameListing(black->playing);
#ifdef RE_DEBUG
		if(aGameListing)
			printf("Found %d for this game\n", aGameListing->number);
#endif //RE_DEBUG
	}
	if(white->playing && !aGameListing)
	{
		aGameListing = room->getGameListing(white->playing);
#ifdef RE_DEBUG
		if(aGameListing)
			printf("Found %d for this game\n", aGameListing->number);
#endif //RE_DEBUG
	}
	if(aGameListing && aGameListing->running == false)
	{
		qDebug("Found non running game in game registry !!!");
		return;
	}
	
	if(!aGameListing)
	{
		printf("No 0a7d for this game\n");
		return;
		/* FIXME, if there's no room, we can't add the game. */
	}

	game_code_to_number[game_code] = aGameListing->number;
#ifdef RE_DEBUG
	printf("Setting game code to game number %02x%02x %d\n", msg[0], msg[1], aGameListing->number);
#endif //RE_DEBUG
	/* Somehow, its possible to get here with a game listing that's been erased and yet somehow comes up
	 * from getGameListing FIXME FIXME*/
	aGameListing->black = black;
	aGameListing->white = white;
	owner = room->getPlayerListing(aGameListing->owner_id);
	/* FIXME, Presumably this is where the crash comes from, we don't get entering rooms for these players
	 * which is fine, but we need to inc the observers to prevent removing the owner from deleting it */
	if(owner != black)
	{
		setAttachedGame(black, aGameListing->number);
		aGameListing->observers++;
	}
	if(owner != white)
	{
		setAttachedGame(white, aGameListing->number);
		aGameListing->observers++;
	}
	//detach owner if any, players are attached alread
	if(owner && owner != black && owner != white)
		setAttachedGame(owner, 0);
	/* Its crashing below FIXME because the players aren't adding to the observers list and then setAttachedGame is deleting
	 * the game */
	/* FIXME we may want to swap the colors? if they're now
	 * legit, pending on nigiri comprehension?? */
	aGameListing->FR = QString::number(byte);		//should probably just be ->flags FIXME
	aGameListing->game_code = game_code;
	aGameListing->handicap = handicap;
	aGameListing->isRoomOnly = false;
}
//1b00 2900 0100 0000 0600 2c01 6900 2500 0404 0000 1400 0400 0200 6b61 7365 697a 696e 0000
//626d 7762 6d77 0000 0000 6b61 7365 697a 696e 0000 8df7 92ac 0000 0000 0000 0303 a8a8 0103
//000200000000000042000000000000000000000000000000000000000000000000000000000000000000000b

//1b00 ffff 0100 0000 0600 8403 8403 8403 0303 0000 1e00 0300 0200 7363 6879 3837 3337 0000
//7968 6c65 6564 6562 0000 bcf8 bcf6 0000 0000 0000 c8ab c0ba b0c5 bbe7 0000 0101 a7a7 0105 
//000200000000000000000000000000000000000000000000000000000000000000000000000000000001000b


//1c00 ffff 0100 0000 0600 2c01 2c01 2c01 0505 0000 1400 0500 0200 7475 7266 6679 0000 0000
//6c69 6831 3230 3600 0000 8b49 9656 0000 0000 0000 3132 3036 0000 0000 0000 0301 a7a7 0105
//000200000000000000000000000000000000000000000000000000000000000000000000000000000001000b

//1c00 ffff 0100 0000 0600 2c01 2c01 2c01 0303 0000 1e00 0300 0200 696e 796f 7000 0000 0000
//6b6b 3036 3239 0000 0000 c7d1 bdb4 c7d1 bdb4 0000 6b6b 3036 3239 0000 0000 0101 a7a7 0101
//00020000000000000000000000000000000000000000000000000000000000000000000000000
//0000001000b


/* I think we get these when we try to observe broadcasted, or betting games
 * as well. */
void CyberOroConnection::handleBettingMatchStart(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	BoardDispatch * boarddispatch;
    GameListing * aGameListing;
	GameData * aGameData, * newGameData = 0;
	unsigned char name[11];
	name[10] = 0x00;
	unsigned short game_number;
	int black_seconds, white_seconds;
	int black_periods, white_periods;
	bool black_in_byoyomi, white_in_byoyomi;
	bool joining = false;
	if(size != 116)
	{
		qDebug("Betting Match msg of strange size %d", size);
	}
#ifdef RE_DEBUG
	printf("New Betting Match: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	
	/* We get this when we join a broadcasted game.  I'm not certain when
	 * it should open a board yet.  Maybe we'll have to set something in
	 * the observe code to do that... but I'm going to try something here */
	/* Oh but wait... because its a total piece of shit, the move list2
	 * comes with game_code, rather than number.  Without making things
	 * stateful, and I'm just not ready for that kind of commitment, we'll
	 * need to create on MoveList2.  Except there's another problem.
	 * the game info is on the listing which we get with the number.
	 * I swear to god.  I was so annoyed with IGS that they didn't
	 * consistently transmit game ids with info.  But ORO is just ridiculous,
	 * with two different identifiers, not used consistently.*/
	/* Okay, final solution.  We store the game number when we start to
	 * observe and the move list picks it up. */
	/* We still can get time settings here and store them on the listing?? */
	game_number = msg[0] + (msg[1] << 8);
	
	p += 2;
	/* The next two bytes might help. or not*/
	if(msg[2] == 0xff && msg[3] == 0xff)
	{
		/* If these are ffff, we can add it to the listing I think */
		joining = false;
#ifdef RE_DEBUG
		printf("not for joining\n");
#endif //RE_DEBUG
	}
	else if(our_player_id == msg[2] + (msg[3] << 8))
	{
		joining = true;
#ifdef RE_DEBUG
		printf("joining\n");
#endif //RE_DEBUG
	}
	
	p += 2;
	/* FIXME Okay, I'm thinking that there's a standard game
	 * data struct that we should just write something to convert
	 * from.  I.e., we pass it the chunk of data and done.
	 * Check out some of the match offers and see if we can
	 * puzzle it out */
	if(joining)
	{
#ifdef RE_DEBUG
	//this keeps coming up, I think just to tell us there's a game but its annoying because it opens
	//a window FIXME
	qDebug("Apparently joining or not %d (%d?)", game_number, connecting_to_game_number);
#endif //RE_DEBUG
		boarddispatch = getBoardDispatch(game_number);
		aGameData = boarddispatch->getGameData();
	}
	else
    {
        aGameData = new GameData();
	}
	aGameData->number = game_number;
	/* Also, we should check for connecting_to_game_number */
	p += 2;
	// our id?
	//0100 0000 0600 2c01 6900 2500 0404 0000 1400 0400 0200 6b61 
	//1b00 ffff 
	//0100 0000 0600 8403 8403 8403 0303 0000 1e00 0300 0200
	aGameData->handicap = msg[6];
	if(msg[7])
		aGameData->komi = -((float)(msg[8])) - 0.5;
	else
		aGameData->komi = ((float)(msg[8])) + 0.5;
	
	p += 6;	//skip first time?
	black_seconds = msg[12] + (msg[13] << 8);
	p += 2;
	white_seconds = msg[14] + (msg[15] << 8);
	p += 2;
	black_periods = msg[16];
	white_periods = msg[17];
	p += 2;
	black_in_byoyomi = msg[18];
	white_in_byoyomi = msg[19];
	if(!black_in_byoyomi)
		black_periods = -1;	//FIXME, awkward flags
	if(!white_in_byoyomi)
		white_periods = -1;
	
	p += 2;
	aGameData->maintime = black_seconds;
	aGameData->periodtime = msg[20] + (msg[21] << 8);
	p += 2;
	aGameData->stones_periods = msg[22];
	switch(msg[23])
	{
		case 0:
			aGameData->timeSystem = byoyomi;
			qDebug("byoyomi %d %d\n", aGameData->stones_periods, aGameData->periodtime);
			break;
		case 1:
			aGameData->timeSystem = canadian;
			qDebug("canadian %d %d\n", aGameData->stones_periods, aGameData->periodtime);
			break;
		case 2:
			aGameData->timeSystem = tvasia;
			qDebug("asia %d %d\n", aGameData->stones_periods, aGameData->periodtime);
			break;
	}
	p += 2;
	//two names
	p += 2;
	//0200
	//second byte could be board size, but usually 19 for Betting
	aGameData->board_size = 19;		//FIXME?!??
	
	if(1)//(add_listing)
	{
		aGameListing = room->getGameListing(game_number);
		aGameListing->running = true;
        aGameListing->isBetting = true;
		aGameListing->handicap = aGameData->handicap;
		aGameListing->game_code = aGameData->number;	//because broadcast?? 
		aGameListing->komi = aGameData->komi;
		strncpy((char *)name, (char *)&(msg[28]), 10);
		p += 10;
		/* First two names often seem to choke? FIXME, try second two */
		/*aGameListing->black = room->getPlayerListing(p[0] + (p[1] << 8));
		if(!aGameListing->black)
		{*/
			//aGameListing->_black_name = QString((char *)name);
			aGameListing->_black_name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		/*}
		p += 2;*/
		strncpy((char *)name, (char *)&(msg[38]), 10);
		p += 10;
		/*aGameListing->white = room->getPlayerListing(p[0] + (p[1] << 8));
		if(!aGameListing->white)
		{*/
			//aGameListing->_white_name = QString((char *)name);
			aGameListing->_white_name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		/*}
		p += 2;*/
		//two more names, unicode??
		strncpy((char *)name, (char *)&(msg[48]), 10);
			//aGameListing->_black_name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		p += 10;
		//printf("Othername: %s\n", name);
		strncpy((char *)name, (char *)&(msg[58]), 10);
			//aGameListing->_white_name = serverCodec->toUnicode((const char *)name, strlen((const char *)name));
		p += 10;
		//printf("Othername2: %s\n", name);
		QString country = getCountryFromCode(msg[68]);
		aGameListing->_black_name += ("[" + country[0] + "]");
		country = getCountryFromCode(p[1]);
		aGameListing->_white_name += ("[" + country[0] + "]"); 
		p += 2;
		if((msg[70] & 0xC0) == 0xC0)
		{
			aGameListing->_black_rank = QString::number(msg[70] & 0x0f) + "p";
			aGameListing->_black_rank_score = 34000 + ((msg[70] & 0x0f) * 1000);
		}
		else if((msg[70] & 0xA0) == 0xA0)
		{
			aGameListing->_black_rank = QString::number(msg[70] & 0x0f) + "d";
			aGameListing->_black_rank_score = 25000 + ((msg[70] & 0x0f) * 1000);
		}
		if((msg[71] & 0xC0) == 0xC0)
		{
			aGameListing->_white_rank = QString::number(msg[71] & 0x0f) + "p";
			aGameListing->_white_rank_score = 34000 + ((msg[71] & 0x0f) * 1000);
		}
		else if((msg[71] & 0xA0) == 0xA0)
		{
			aGameListing->_white_rank = QString::number(msg[71] & 0x0f) + "d";
			aGameListing->_white_rank_score = 25000 + ((msg[71] & 0x0f) * 1000);
		}
#ifdef RE_DEBUG
		printf(" %02x%02x%02x%02x\n", msg[70], msg[71], msg[72], msg[73]);
#endif //RE_DEBUG
		p += 2;
#ifdef RE_DEBUG
		printf("Adding listing for %s vs %s\n",
		       aGameListing->white_name().toLatin1().constData(),
		       aGameListing->black_name().toLatin1().constData());
#endif //RE_DEBUG
		//aGameListing->game_code = aGameData->game_code;
		aGameListing->isRoomOnly = false;
		room->recvGameListing(aGameListing);
	}
	else
		p += 24;
	
	aGameListing = room->getGameListing(game_number);

	//recv game record? but what's to receive?
	aGameData->black_name = aGameListing->black_name();
	aGameData->white_name = aGameListing->white_name();
	aGameData->black_rank = aGameListing->black_rank();
	aGameData->white_rank = aGameListing->white_rank();
	
	if(joining)
	{
	boarddispatch->openBoard();
	boarddispatch->recvTime(TimeRecord(white_seconds, white_periods), TimeRecord(black_seconds, black_periods));
	
	/* We get this upon opening a betting match */
	/* And observers: */
	if(room_were_in == game_number)		//no observing outside
	{
#ifdef RE_DEBUG
		qDebug("adding observer list to betting match board...");
#endif //RE_DEBUG
		addObserverListToBoard(boarddispatch);
	}
	}
	else
		delete newGameData;
}

//ca5d	chat rooms also
//this could potentially also be a join room
void CyberOroConnection::handleCreateRoom(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	PlayerListing * aPlayer;
	unsigned short room_number;
	unsigned char room_type;
	if(size != 60)
	{
		qDebug("create room msg of strange size: %d", size);
	}
	aPlayer = room->getPlayerListing(msg[0] + (msg[1] << 8));
#ifdef RE_DEBUG
	if(aPlayer)
		qDebug("player %s %02x%02x chat room/match\n", aPlayer->name.toLatin1().constData(), msg[0], msg[1]);
	else
		qDebug("can't find player %02x%02x\n", msg[0], msg[1]);
#endif //RE_DEBUG
	p += 2;
	room_number = msg[2] + (msg[3] << 8);
    GameListing * aGameListing = room->getGameListing(room_number);
    aGameListing->running = true;
	aGameListing->owner_id = aPlayer->id;
	
#ifdef RE_DEBUG
	printf("room number on that chat: %d\n", room_number);
#endif //RE_DEBUG
	p += 2;
	p += 3;
	//probably room info
	//0000 00
	//4011
	switch(msg[7])
	{
		case 0xc0:
			break;
		case 0x40:
			break;
		case 0x09:
			break;
		case 0x00:
			aGameListing->_black_name = "** wished " + QString::number(msg[8], 16);
			break;
	}
	room_type = msg[7];
	aGameListing->white = aPlayer;
	
	aGameListing->black = 0;
	aGameListing->_black_name = QString::number(room_type, 16);
	aGameListing->observers = 1;	//the person who created it

	
	aGameListing->isRoomOnly = true;
	//p[1] here is time and strength wished as per create room code
	p += 2;
	p += 8;
	//a room byte info?
	p++;
	//title!!
	
	p += 20;
	// and lots of zeroes
	p += 22;
	//if(p != msg + size)
	//{
#ifdef RE_DEBUG
		//other than 60 ?? FIXME
		printf("ca5d: \n");
		for(unsigned int i = 0; i < size; i++)
			printf("%02x", msg[i]);
		printf("\n");
#endif //RE_DEBUG
	//}
//review game minminmin 7d
//c901de00 0000 00c0 000000027d60005acdabbadc003b00003cb3a5017d60b65a68b3a501d98bcf7700e0fd7f68b3a5015a88cf770000000000000000
//chat room
//89041401 0000 0040 11000000000000000000000000760000000000000000000000000000000000000000000000000000000000000000000000000000
//chat room
//15008500 0000 0000 11000000000000000000000000b90000000000000000000000000000000000000000000000000000000000000000000000000000
//980d6600 0000 0000 11000000000000000000000000bc0000000000000000000000000000000000000000000000000000000000000000000000000000
//wished normal time even 1k
//74099800 0000 0000 11000000000000000000000000e90000000000000000000000000000000000000000000000000000000000000000000000000000
//wished normal time strong
//b8052301 0000 0000 01000000000000000000000000860000000000000000000000000000000000000000000000000000000000000000000000000000
//wished normal time even
//8d00b100 00000000 11000000000000000000000000dd0000000000000000000000000000000000000000000000000000000000000000000000000000
//wished normal time even
//29093201 00000000 110000000000000000000000008e0000000000000000000000000000000000000000000000000000000000000000000000000000
//74099800 00000000 11000000000000000000000000e90000000000000000000000000000000000000000000000000000000000000000000000000000
//quit playing
//5d0da60000000000 110000000000000000000000005b0000000000000000000000000000000000000000000000000000000000000000000000000000
//locked gomoku? equal standard wished?		
//6d0b1201 0000 0009 11000000000000000000000000ed0000000000000000000000000000000000000000000000000000000000000000000000000000	
	
    emit gameListingReceived(aGameListing);
	
	/* Set the attachment right after the game list is recvd */
	if(aPlayer)
		setAttachedGame(aPlayer, aGameListing->number);
}

//56f4
void CyberOroConnection::handleBettingMatchResult(unsigned char * msg, unsigned int size)
{
	unsigned short game_code = msg[0] + (msg[1] << 8);
	unsigned short game_id = game_code_to_number[game_code];
	//FIXME
	if(size != 16)
	{
		qDebug("betting match result of strange size %d", size);
	}
#ifdef RE_DEBUG
	printf("56f4: likely betting match result: %d\n", game_id);
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_id);
	if(!boarddispatch)
		return;
	GameResult aGameResult;
	//3875020100000000000000000000001c B+R
	//35750101332e35000000000000000015 B+3.5 "3.5"	//FIXME should convert and double check with what we get
	//37750202000000000000000000000015 W+R
	if(msg[2] == 0x01)
	{
		aGameResult.result = GameResult::SCORE;
		float margin;
		sscanf((char *)&(msg[4]), "%f", &margin);
		aGameResult.winner_score = margin;
		aGameResult.loser_score = 0.0;
	}
	else if(msg[2] == 0x02)
	{
		aGameResult.result = GameResult::RESIGN;
	}
	if(msg[3] == 0x01)
	{
		aGameResult.winner_color = stoneBlack;
	}
	else if(msg[3] == 0x02)
	{
		aGameResult.winner_color = stoneWhite;
	}
	//4d75 0202 302e 3500 0000 0000 0000 0015
	boarddispatch->recvResult(&aGameResult);
	
	/* Rejoining a betting match that one had already joined after it was over, doesn't pop up the
	 * result again that I can tell FIXME */
	
	/* Sure they never hang around with the game over thing ? */
	//aGameListing->running = false;
	//room->recvGameListing(aGameListing);
	//game_code_to_number.erase(game_id);
}

//5ac3
void CyberOroConnection::handleGamePhaseUpdate(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	GameListing * aGameListing;
	
	if(size != 4)
	{
		qDebug("GamePhaseUpdate of strange size: %d", size);
		return;
	}
#ifdef RE_DEBUG
	printf("5ac3: %d fl %02x ", p[0] + (p[1] << 8), p[2]);
	for(unsigned int i = 0; i < 4; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	// maybe first byte is game number?, and then update?
	/* I'm going to pretend these are game phase status messages */
	aGameListing = room->getGameListing(p[0] + (p[1] << 8));
    aGameListing->moves = getPhase(p[2]);
    emit gameListingReceived(aGameListing);
}

//50c3
#ifdef RE_DEBUG
void CyberOroConnection::handleMsg3(unsigned char * msg, unsigned int size)
#else
void CyberOroConnection::handleMsg3(unsigned char * /*msg*/, unsigned int size)
#endif //RE_DEBUG
{
	//FIXME better name, what was this again?
	// maybe its the timer msg 
	
	if(size != 6)
	{
		qDebug("50c3 of strange size: %d", size);	
	}
#ifdef RE_DEBUG
	unsigned char * p = msg;
	printf("**** 50c3: ");
	for(unsigned int i = 0; i < 4; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
}

/* Maybe this is nigiri but it seems sent by other people's games
 * as well at strange times, I guess just setting colors */
//f5af
void CyberOroConnection::handleNigiri(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char thebyte;
	//unsigned short game_code = msg[0] + (msg[1] << 8);
	unsigned short room_number = msg[2] + (msg[3] << 8);
	QString name, rank, our_rank;
	GameData * gr;
	
	if(size != 8)
	{
		qDebug("Nigiri message of strange size: %d\n", size);
	}
	
	p += 4;
	thebyte = p[0];
	/* Its also possible that we'd need to do this on games that
	 * we joined.  Its even possible that this is sent all
	 * the time for games that just started */
	if(match_negotiation_state->isOurGame(room_number))
	{
		if(we_send_nigiri)	//ignore
			return;
		//QMessageBox::information(0 , tr("receiving niri"), tr("receiving ") + QString::number(thebyte, 16) + tr(" nigiri ") + QString::number(p[3], 16));
		BoardDispatch * boarddispatch = getIfBoardDispatch(room_number);
		if(!boarddispatch)
		{
			qDebug("handlenigiri but no board dispatch");
			return;
		}
		/* "our game being played"  That's got to be the worst
		 * variable name in the history of the universe.
		 * even "our match in progress" would be slightly better.
		 * Or "number of current game", "currently played game number"... */
		
		/* We need to start the timers here I think */
		gr = boarddispatch->getGameData();
		
		/* FIXME its a BIG deal if you send a move before nigiri is
		 * fully negotiated.  The move is ignored, opponent can't play
		 * and we can't send the move again because we're waiting for it.
		 * we need to either not set the colors or the turn or block
		 * the start of the game or something until nigiri is
		 * fully negotiated.
		 * double FIXME I don't think it has anything to do with this,
		 * we're not doing the nigiri right */
		
		/* There's some other kind of problem here related to the swap.
		 * for instance even if they get black we don't display their move
		 * for some reason.  And then sometimes we send resigns
		 * that look different.  I think... maybe the listing is over
		 * writing the colors later or something, I just added something
		 * to maybe fix that. */
		
		/* this stuff lets us play only if opponent is black and we're white
		 * but then it gets the resign messages flipped.
		 * if we're black, then the resign messages are okay but we can't play */
		/* send nigiri fails if they play black in that we don't see their
		 * black move and can't play ourself, names seem correct,
		 * if we resign as that white, we get a message with (B) and opponent
		 * thinks that white has one by resignation. */
		/* This is so critical but I have no idea what's wrong with it.
		 * that (B) has to be set that way, basically by handleResign
		 * which means that the gr->white_name, etc., are wrong, I just
		 * don't know why this wouldn't be reflected in the score board
		 * I consider randomly that maybe I forgot and have two copies
		 * of the gameData running around, still not sure on that but
		 * it looks clean. 
		 * What's really weird is that the sendResign doesn't have a
		 * color byte, and yet the official client interprets it nonsensically
		 * which either means it does have a color byte or we're confusing
		 * the official client, but this is even when we handle the
		 * nigiri !  We have nothing we can do to screw it up... maybe
		 * the accept Match offer... maybe... but that's hard to believe
		 * I guess that's the only other option.*/
		//if((thebyte % 2) == 0)
		/* I'm thinking that we always get the one meant
		 * for us... that the server adds 1 or something
		 * to tailor it for us */
		//if(((((thebyte & 0xe0) >> 5) + 1) % 2) == ((thebyte & 0x1f) % 2))	    
		//if((!(thebyte & 0x20)) == ((thebyte & 0x1f) % 2)) 
		if(!(thebyte & 0x80))
		{
#ifdef RE_DEBUG
			qDebug("opponent nigiri successful");
#endif //RE_DEBUG
			boarddispatch->recvKibitz(QString(), "Opponent plays black");
			if(gr->black_name == getUsername())
				boarddispatch->swapColors();
			else
				boarddispatch->swapColors(true);
		}
		else
		{
#ifdef RE_DEBUG
			qDebug("opponent nigiri failure");
#endif //RE_DEBUG
			boarddispatch->recvKibitz(QString(), "Opponent plays white");
			if(gr->white_name == getUsername())
				boarddispatch->swapColors();
			else
				boarddispatch->swapColors(true);
		}
		gr = boarddispatch->getGameData();	//get again
		/* FIXME Don't think we start until second move */
		startMatchTimers(gr->black_name == getUsername());
	}
	else
	{
		/* We get these a lot on other games and I think they
		 * may be part of starting a match... like we should
		 * check the owner and swap the colors if that's an issue
		 * FIXME */
	}
#ifdef RE_DEBUG
	printf("0xf5af: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

/* This comes before we've gotten the ids */
void CyberOroConnection::handleFriends(unsigned char * msg, unsigned int size)
{
	unsigned short friend_records;
	unsigned char * p = msg;
	unsigned char name[11];
	unsigned int i;
	
	friend_records = p[0] + (p[1] << 8);
	p += 2;
	//check msg size???! FIXME
	qDebug("handleFriends %d %d", friend_records, size);		//4 520 suggests (4 * 12) + 2 + 470
	for(i = 0; i < friend_records; i++)
	{
		//name padded to 10, plus two bytes for flags	
		strncpy((char *)name, (char *)p, 10);
		p += 10;
#ifdef RE_DEBUG
		printf("friend or foe? %s, %02x%02x", name, p[0], p[1]);
#endif //RE_DEBUG
		//first byte is 01, they're probably friend
		//first byte is 02, probably block
		if(p[0] == 0x01)
			friendedList.push_back(new FriendWatchListing(QString((const char *)name), friendwatch_notify_default));
		else if(p[0] == 0x02)
			blockedList.push_back(new FriendWatchListing(QString((const char *)name), friendwatch_notify_default));
		p += 2;
	}
	//the rest is a mystery for right now
#ifdef RE_DEBUG
	printf("0x8129: ");
	for(i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

/* We think this is the last, always the last of the connection lists
 * so we might want to clear the playerlist_inorder vector here,
 * if that's okay?  Otherwise on connect is fine too. */
void CyberOroConnection::handleMsg2(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	int records = p[0] + (p[1] << 8);
	p += 2;
#ifdef RE_DEBUG
	printf("Its msg2: e461\n");
#endif //RE_DEBUG
	playerlist_inorder.clear();	//hopefully this is a good place
	//each record is 18 bytes;
	while(records--)
	{
#ifdef RE_DEBUG
		for(unsigned int i = 0; i < 18; i++)
			printf("%02x", p[i]);
		printf("\n");
#endif //RE_DEBUG
		p += 18;
	}
	if(p != (msg + size))
		printf("Weird msg length: %d at line %d\n", size, __LINE__);
}

//0x69c3:
#ifdef RE_DEBUG
void CyberOroConnection::handleRematchRequest(unsigned char * msg, unsigned int size)
#else
void CyberOroConnection::handleRematchRequest(unsigned char * /*msg*/, unsigned int size)
#endif //RE_DEBUG
{
	/* In ORO win client this actually looks just like a matchinvite */
	if(size != 6)
	{
		qDebug("Rematch request of strange size: %d", size);
		return;
	}
	if(!room_were_in)
	{
		qDebug("Got rematch request, but not in a room");
		return;
	}
	BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
	if(!boarddispatch)
	{
		qDebug("Couldn't get board dispatch on room were in: %d", room_were_in);
		return;
	}
	boarddispatch->recvRematchRequest();

	/* FIXME, we'll need to send the request and get the accept and
	* see what comes next... */
	//their id, our id, two more bytes
#ifdef RE_DEBUG
	printf("rematch request: 69c3: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//0x6ec3:
#ifdef RE_DEBUG
void CyberOroConnection::handleRematchAccept(unsigned char * msg, unsigned int size)
#else
void CyberOroConnection::handleRematchAccept(unsigned char * /*msg*/, unsigned int size)
#endif //RE_DEBUG
{
	bool were_black;
	if(size != 6)
	{
		qDebug("rematch accept packet with strange size: %d", size);
		return;
	}
	/* FIXME, we'll need to send the request and get the accept and
	 * see what comes next... */
	//their id, our id, two more bytes
#ifdef RE_DEBUG
	printf("rematch request accept: 6ec3: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	//610a a804 128f
	//we need to check our_player_id against first two bytes
	if(!room_were_in)
	{
		qDebug("Not in a room for rematch accept");
		return;
	}
	BoardDispatch * boarddispatch = getIfBoardDispatch(room_were_in);
	if(!boarddispatch)
	{
		qDebug("handle rematch accept has no board dispatch");
		return;
	}
	if(!match_negotiation_state->sentRematch())
		return;

	MatchRequest * m = new MatchRequest();
	GameData * gd = boarddispatch->getGameData();
	m->number = room_were_in;
	m->last_game_code = gd->game_code;
	m->rematch = true;
	m->board_size = gd->board_size;
	m->komi = gd->komi;
	m->handicap = gd->handicap;
	m->free_rated = gd->free_rated;
	if(m->handicap == 0 && gd->komi > 0.0)
	{
		//switch colors?
		//yes switch colors, we send the match offer..
		if(gd->black_name == getUsername())
			were_black = false;
		else
			were_black = true;
	}
	else
		were_black = (gd->black_name == getUsername());
	m->opponent_id = gd->opponent_id;
	m->opponent_is_challenger = false;
	if(were_black)
	{	
		m->opponent = gd->white_name;
		m->their_rank = gd->white_rank;
		m->our_name = gd->black_name;
		m->our_rank = gd->black_rank;
		m->color_request = MatchRequest::BLACK;
		m->challenger_is_black = false;	//?
	}
	else
	{
		m->opponent = gd->black_name;
		m->their_rank = gd->black_rank;
		m->our_name = gd->white_name;
		m->our_rank = gd->white_rank;
		m->color_request = MatchRequest::WHITE;
		m->challenger_is_black = false;
	}
	m->timeSystem = gd->timeSystem;
	m->maintime = gd->maintime;
	m->periodtime = gd->periodtime;
	m->stones_periods = gd->stones_periods;
	
	//d2af does this:
	//NetworkConnection::closeBoardDispatch(room_were_in);	//doesn't close window necessarily
	
	
	sendMatchOffer(*m, false);	//not counter offer
	match_negotiation_state->offerMatchTerms(m);			//is this the right state? FIXME
	delete m;
	
	//BoardDispatch * boarddispatch = getBoardDispatch(room_were_in);		//should open new game
	//this needs also to send something else to trigger whatever
}

/* This is also used during game negotiation, not just rematches */
//0x327d
void CyberOroConnection::handleMatchDecline(unsigned char * msg, unsigned int size)
{
	PlayerListing * player;
	unsigned char * p = msg;

	if(size != 6)
	{
		qDebug("match decline message of strange size %d", size);
	}

	if(p[0] + (p[1] << 8) != our_player_id)
	{
		//qDebug("*** Woah, 327d rematch decline not meant for us!");		//this happens FIXME
		return;
	}
	p += 2;
	player = getDefaultRoom()->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	// those last two bytes?
#ifdef RE_DEBUG
	printf("0x327d: unexplained bytes: %02x %02x\n", p[0], p[1]);
#endif //RE_DEBUG
	if(!match_negotiation_state->sentMatchOffer() && !match_negotiation_state->sentRematch())
	{
		match_negotiation_state->reset();
		return;
	}
	if(!match_negotiation_state->verifyPlayer(player))
	{
		qDebug("Bad player from rematch decline");
		match_negotiation_state->reset();
		return;
	}
    GameDialog * gameDialogDispatch = getIfGameDialog(player);
	if(gameDialogDispatch)
	{
		gameDialogDispatch->recvRefuseMatch(GD_REFUSE_DECLINE);
	}
	else
	{
		QMessageBox mb(tr("Rematch declined"), tr("%1 has declined rematch").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
			QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	}
	match_negotiation_state->reset();
#ifdef RE_DEBUG
	printf("0x327d: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//b0b3
void CyberOroConnection::handleResign(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	GameData * gr;
	unsigned char * p = msg;
	PlayerListing * player;
	if(size != 8)
	{
		qDebug("Resign message of strange size: %d", size);
		return;
	}
	player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	unsigned short game_code = msg[2] + (msg[3] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	boarddispatch = getIfBoardDispatch(game_number);
	if(boarddispatch)
	{
		GameResult aGameResult;
		gr = boarddispatch->getGameData();
		if(!gr)
		{
			printf("Can't get game record for resign msg\n");
			return;
		}
		aGameResult.result = GameResult::RESIGN;
		if(gr->black_name == player->name)
		{
			aGameResult.winner_color = stoneWhite;
			aGameResult.winner_name = gr->white_name;
			aGameResult.loser_name = gr->black_name; //necessary FIXME???
		}
		else
		{
			aGameResult.winner_color = stoneBlack;
			aGameResult.winner_name = gr->black_name;
			aGameResult.loser_name = gr->white_name;
		}
		boarddispatch->recvResult(&aGameResult);
		if(match_negotiation_state->isOurGame(game_number))
		{
			killActiveMatchTimers();
			match_negotiation_state->reset();
			//FIXME what about rematch?  what about other resets?!?!?FIXME
		}
	}
}

//I think this double as an accept count request
//c9b30c00 c801 ea0c 000000f1
void CyberOroConnection::handleEnterScoring(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	PlayerListing * player;
	player = room->getPlayerListing(msg[0] + (msg[1] << 8));
	
	if(size != 8)
	{
		qDebug("Enter scoring message of strange size: %d", size);
    }
	unsigned short game_code = msg[2] + (msg[3] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	boarddispatch = getIfBoardDispatch(game_number);
	if(boarddispatch)
	{
#ifdef RE_DEBUG
		qDebug("Entering scoring mode");
#endif //RE_DEBUG
		boarddispatch->recvEnterScoreMode();
		if(match_negotiation_state->isOurGame(game_number))
		{
			match_negotiation_state->enterScoreMode();
			deadStonesList.clear();
			killActiveMatchTimers();
		}
	}
}

//FIXME pretty sure this happens too often, like twice in
//a game or something
void CyberOroConnection::killActiveMatchTimers(void)
{
#ifdef RE_DEBUG
	qDebug("killing timers");
#endif //RE_DEBUG
	if(matchRequestKeepAliveTimerID)
	{
		killTimer(matchRequestKeepAliveTimerID);
		matchRequestKeepAliveTimerID = 0;
	}
	if(matchKeepAliveTimerID)
	{
		killTimer(matchKeepAliveTimerID);
		matchKeepAliveTimerID = 0;
	}
}

void CyberOroConnection::startMatchTimers(bool ourTurn)
{
	if(ourTurn)
	{
#ifdef RE_DEBUG
		qDebug("Starting keep alive timer");
#endif //RE_DEBUG
		matchKeepAliveTimerID = startTimer(39000);
		matchRequestKeepAliveTimerID = 0;
	}
	else
	{
#ifdef RE_DEBUG
		qDebug("Starting request keep alive timer");
#endif //RE_DEBUG
		matchRequestKeepAliveTimerID = startTimer(39000);
		matchKeepAliveTimerID = 0;
	}
}

/* Final f1b31400 b006 640b 2f05 5c22 0102 1106 0804 1106
 * is basically the same thing... plus a couple more bytes...
 * makes me think we should write only one handler or something.
 * or maybe e7b31400 2f05 640b 2f05 5c6d 0102 1106 0804 1106
 * is really the final ?!?!? FIXME  */
//e2b31200 2f05 640b 2f05 2863 0201 1106 0804

void CyberOroConnection::handleRemoveStones(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	bool unremove;
	int number_of_marks;
	PlayerListing * player;
	player = room->getPlayerListing(msg[0] + (msg[1] << 8));
	p += 2;
	unsigned short game_code = msg[2] + (msg[3] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		printf("Can't get board dispatch for remove stones for %02x%02x\n", p[0], p[1]);
		return;
	}
	p += 2;
	//another player id
	p += 2;
#ifdef RE_DEBUG
	printf("e2b3: ");
	for(unsigned int i = 0; i < size - 6; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	//weird two bytes
	p += 2;
	// a 01 or 02
	unremove = (msg[8] == 2);
	p++;
	number_of_marks = msg[9];
	p++;
	MoveRecord aMove;
	aMove.x = msg[10];
	aMove.y = msg[11];
	/* First one is the new one, the rest is the full list */
	/* Also, I think if its an unremove, its removed from the list,
	 * both in this packet and ensuing ones */
	//129d 0102 0f09 1310 0f09

	/* Instead might be an acknowledge of previous sends. */
	//aMove.x = msg[size - 2];	//definitely not right
	//aMove.y = msg[size - 1];
	if(unremove)
	{
		aMove.flags = MoveRecord::UNREMOVE_AREA;
		if(match_negotiation_state->isOurGame(game_number))
		{
			if(!match_negotiation_state->counting())
				return;
			std::vector <MoveRecord>::iterator it;
			for(it = deadStonesList.begin(); it != deadStonesList.end(); it++)
			{
				if(it->x == aMove.x &&
				    it->y == aMove.y)
				{
					qDebug("removing %d %d from dead stones list", aMove.x, aMove.y);
					deadStonesList.erase(it);
					break;
				}
			}
		}
	}
	else
	{
		aMove.flags = MoveRecord::REMOVE_AREA;
		if(match_negotiation_state->isOurGame(game_number))
		{
			if(!match_negotiation_state->counting())
				return;
			qDebug("pushing %d %d onto dead stones list\n", aMove.x, aMove.y);
			deadStonesList.push_back(aMove);
		}
	}
	
	boarddispatch->recvMove(&aMove);
	/* What about mark undos??? FIXME */
	p += 2; 	//skip the rest
	/*
		while(number_of_marks--)
		{
			MoveRecord aMove;
			aMove.flags = MoveRecord::REMOVE;
			aMove.x = p[0];
			aMove.y = p[1];
			p += 2;
			boarddispatch->recvMove(&aMove);
		}
	*/
}

//f1b3
/* This is definitely like an opponent has hit done kind of
 * msg, not that that means anything to us until we hit done */
#ifdef RE_DEBUG
void CyberOroConnection::handleStonesDone(unsigned char * msg, unsigned int size)
#else
void CyberOroConnection::handleStonesDone(unsigned char * msg, unsigned int /*size*/)
#endif //RE_DEBUG
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	int number_of_marks;
	PlayerListing * player;
	player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	unsigned short game_code = p[0] + (p[1] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		printf("Can't get board dispatch for remove stones for %02x%02x\n", p[0], p[1]);
		return;
	}
	p += 2;
	//another player id
	p += 2;
#ifdef RE_DEBUG
	printf("0xf1b3: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	//weird two bytes
	if(match_negotiation_state->isOurGame(game_number))
	{
		unsigned short done_response = p[0] + (p[1] << 8);
		
		if(match_negotiation_state->sentDoneCounting())
		{
			if(!match_negotiation_state->verifyCountDoneMessage(done_response))
			{
				qDebug("Count done fail!!\n");
				//FIXME
			}
			match_negotiation_state->setDoneCounting();
		}
		else
		{
			match_negotiation_state->setCountingVerification(done_response);
			match_negotiation_state->receiveDoneCounting();
		}
	}
	p += 2;
	// a 01 or 02
	p++;
	number_of_marks = p[0];
	p++;
	/* First one is the new one, the rest is the full list */
	MoveRecord aMove;
	aMove.flags = MoveRecord::REMOVE;
	aMove.x = p[0];
	aMove.y = p[1];
	boarddispatch->recvKibitz(QString(), QString("%1 has hit done...").arg(player->name));
	//boarddispatch->recvMove(&aMove);
	/* What about mark undos??? FIXME */
	p += 2; 	//skip the rest
	
	/* FIXME, we might actually want to just clear and readd all the stones
	 * listed.  I mean otherwise we're sort of negating the point of
	 * the done message */
	/*
	while(number_of_marks--)
	{
	MoveRecord aMove;
	aMove.flags = MoveRecord::REMOVE;
	aMove.x = p[0];
	aMove.y = p[1];
	p += 2;
	boarddispatch->recvMove(&aMove);
}
	*/
}

//e7b3
#ifdef RE_DEBUG
void CyberOroConnection::handleScore(unsigned char * msg, unsigned int size)
#else
void CyberOroConnection::handleScore(unsigned char * msg, unsigned int /*size*/)
#endif //RE_DEBUG
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	unsigned short game_id, game_code;
	int number_of_marks;
	PlayerListing * player;
	player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	game_code = p[0] + (p[1] << 8);
	game_id = game_code_to_number[game_code];
	boarddispatch = getIfBoardDispatch(game_id);
	if(!boarddispatch)
	{
		printf("Can't get board dispatch for remove stones for %02x%02x\n", p[0], p[1]);
		return;
	}
	p += 2;
	//another player id
	p += 2;
#ifdef RE_DEBUG
	printf("e7b3: ");
	for(unsigned int i = 0; i < size - 6; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	//weird two bytes
	p += 2;
	// a 01 or 02
	p++;
	number_of_marks = p[0];
	p++;

	if(match_negotiation_state->isOurGame(game_id))
	{
		if(!match_negotiation_state->doneCounting())
		{
			printf("Not done counting and e7b3\n");
			return;
		}
	}

	/* First one is the new one, the rest is the full list */
	MoveRecord aMove;
	aMove.flags = MoveRecord::REMOVE;
	aMove.x = p[0];
	aMove.y = p[1];
	//boarddispatch->recvMove(&aMove);
	/* What about mark undos??? FIXME */
	p += 2; 	//skip the rest
	/*
	while(number_of_marks--)
	{
	MoveRecord aMove;
	aMove.flags = MoveRecord::REMOVE;
	aMove.x = p[0];
	aMove.y = p[1];
	p += 2;
	boarddispatch->recvMove(&aMove);
}
	*/
	boarddispatch->recvResult(0);
#ifdef OLD
	//double check, this appropriate here? FIXME, we need a func for it
	GameListing * aGameListing = room->getGameListing(game_id);
	if(aGameListing)
	{
		//FIXME I doubt it
		//aGameListing->running = false;
		//room->recvGameListing(aGameListing);
		our_game_being_played = 0;
		
	}
#endif //OLD
}

//c4b3
void CyberOroConnection::handleRequestCount(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * player;
	unsigned short game_code, game_number;

	if(size != 8)
	{
		qDebug("Request count msg of strange size: %d", size);	//should be 12 - 4
	}
	
	player = room->getPlayerListing(msg[0] + (msg[1] << 8));
	p += 2;
	game_code = msg[2] + (msg[3] << 8);
	game_number = game_code_to_number[game_code];
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Request count: No board dispatch for %02x%02x %d", msg[2], msg[3], game_number);
		return;
	}
	p += 2;
	
	killActiveMatchTimers();
	boarddispatch->recvRequestCount();
				
#ifdef RE_DEBUG
	printf("0x request count?: c4b3: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//ceb3
void CyberOroConnection::handleRejectCountRequest(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * player;
	unsigned short game_code, game_number;

	if(size != 8)
	{
		qDebug("Request count msg of strange size: %d", size);	//should be 12 - 4
	}
	
	player = room->getPlayerListing(msg[0] + (msg[1] << 8));
	p += 2;
	game_code = msg[2] + (p[3] << 8);
	game_number = game_code_to_number[game_code];
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", msg[2], msg[3]);
		return;
	}
	p += 2;
	
	boarddispatch->recvRejectCountRequest();
				
#ifdef RE_DEBUG
	printf("0x reject request count?: ceb3: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	GameData * gr = boarddispatch->getGameData();
	startMatchTimers(boarddispatch->getBlackTurn() ? (gr->black_name == getUsername()) : (gr->white_name == getUsername()));
}

//05b4
void CyberOroConnection::handleRequestMatchMode(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	PlayerListing * player;
	unsigned short game_code, game_number;
	if(size != 8)
	{
		qDebug("Request match mode msg of strange size: %d", size);
	}

	//4503 d802 0000 007c
	player = room->getPlayerListing(msg[0] + (msg[1] << 8));
	game_code = msg[2] + (msg[3] << 8);
	game_number = game_code_to_number[game_code];
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", msg[2], msg[3]);
		return;
	}

	killActiveMatchTimers();		//okay?  what about cheating issues?? FIXME
	boarddispatch->recvRequestMatchMode();	
				
#ifdef RE_DEBUG
	printf("0x request match mode?: 05b4: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//0ab4
void CyberOroConnection::handleRejectMatchModeRequest(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * player;
	unsigned short game_code, game_number;
	if(size != 8)
	{
		qDebug("Request match mode msg of strange size: %d", size);
	}

	//4503 d802 0000 007c
	player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	game_code = msg[2] + (msg[3] << 8);
	game_number = game_code_to_number[game_code];
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", msg[2], msg[3]);
		return;
	}
	p += 2;
	
	boarddispatch->recvRejectMatchModeRequest();
				
#ifdef RE_DEBUG
	printf("0x decline request match mode?: 0ab4: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//f6b3
void CyberOroConnection::handleTimeLoss(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * player;
	unsigned short game_code, game_number;
	
	qDebug("handleTimeLoss size %d", size);

	player = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
	game_code = p[0] + (p[1] << 8);
	game_number = game_code_to_number[game_code];
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", p[0], p[1]);
		return;
	}
	p += 2;
	GameData * aGameData = boarddispatch->getGameData();
	
	GameResult aGameResult;
	aGameResult.result = GameResult::TIME;
	aGameResult.game_number = game_number;
	aGameResult.loser_name = player->name;
	if(aGameData->white_name == player->name)
	{
		//winner and loser colors??!?!? FIXME FIXME
		aGameResult.winner_name = aGameData->black_name;
		aGameResult.winner_color = stoneBlack;
	}
	else
	{
		aGameResult.winner_name = aGameData->white_name;
		aGameResult.winner_color = stoneWhite;
	}
	boarddispatch->recvResult(&aGameResult);
	/* Maybe the last 4 bytes are the remaining 1 second of time, maybe
	 * its color or something?*/
#ifdef RE_DEBUG
	printf("f6b3: %02x%02x%02x%02x\n", p[0], p[1], p[2], p[3]);
#endif //RE_DEBUG
	/* What about killing the listing or something??*/
}

//dcaf	(42f4 goes here as well)
void CyberOroConnection::handleMove(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	GameData * gr;
	PlayerListing * player;
	int periods;
	bool inPeriodTime;
	unsigned short seconds;
	unsigned short player_id;
	unsigned short game_id = p[0] + (p[1] << 8);
	unsigned short game_number = game_code_to_number[game_id];
	static bool prev_pass = false;
	if(size != 16)
	{
		qDebug("Move of strange size: %d", size);
		return;
	}
	p += 2;
	MoveRecord * aMove = new MoveRecord();
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", p[0], p[1]);
		return;
	}
	gr = boarddispatch->getGameData();
	if(!gr)
	{
		qDebug("Shouldn't be a boarddispatch here\n");
		return;
	}
	player_id = p[0] + (p[1] << 8);
	player = room->getPlayerListing(player_id);
	/* I get the feeling there's a bug around here FIXME */
#ifdef RE_DEBUG
	printf("Move!!!: %02x%02x %02x%02x %02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
#endif //RE_DEBUG
	p += 2;
	p += 2;
	aMove->number = p[0] + (p[1] << 8);
	/* ORO sends move 1 as first move after handicap */
	/* FIXME There's something weird here.  For some reason
	 * games we play have to have the first white move after
	 * handicap (and possibly others) as what ORO sends
	 * naturally, 1, but observing games, we have to
	 * decrement all the move numbers by 1.  This has
	 * to have something to do with the move list because
	 * see we don't see a move list for our own game.
	 * perhaps if we joined a game before the first
	 * move, we'd be just as screwed up.*/
	//aMove->number--;	//because we expected 0 move?
	/* Also print out the moves we send after you fix this.
	aMove->flags = MoveRecord::NONE;*/
	
	/* Player here is NOT reliable for color, let's try without
	 * though I think the first two bytes 0101 vs 0000 might be
	 * a color flag */
	/*if(player->name == gr->white_name)
		aMove->color = stoneWhite;
	else if(player->name == gr->black_name)
		aMove->color = stoneBlack;
	else
		qDebug("no color\n");*/
	aMove->color = stoneNone;
	p += 2;
#ifdef RE_DEBUG
	printf("%d %d \n", p[0], p[1]);
#endif //RE_DEBUG
	aMove->x = p[0];
	aMove->y = p[1];
	p += 2;
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < 6; i++)	//8, 9 or 10?
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	seconds = p[0] + (p[1] << 8);
	//3rd byte is periods remaining, first byte is probably seconds
	periods = p[2];
	inPeriodTime = p[3];
	p += 4;
	
	if(!inPeriodTime)
		periods = -1;		//for clockdisplay
	if(aMove->x == 0 || aMove->y == 0)
		aMove->flags = MoveRecord::PASS;
	boarddispatch->recvMove(aMove);
	/* FIXME color is not set!! */
	if(aMove->color == stoneWhite)
		boarddispatch->recvTime(TimeRecord(seconds, periods), TimeRecord(0, -1));
	else
		boarddispatch->recvTime(TimeRecord(0, -1), TimeRecord(seconds, periods));
	
	
	
	if(match_negotiation_state->isOurGame(game_number))
	{
		matchKeepAliveTimerID = startTimer(39000);		//keep alives
		if(matchRequestKeepAliveTimerID)
		{
			killTimer(matchRequestKeepAliveTimerID);
			matchRequestKeepAliveTimerID = 0;
		}
		// FIXME if this is a pass, and the previous move
		// was a pass, then I think we need to sendEnterScoring()
		// but maybe only if we passed first and this is opp passing
		if(aMove->flags == MoveRecord::PASS)
		{
			if(prev_pass && player_id != our_player_id)
				sendEnterScoring(game_id);
			prev_pass = true;
		}
		else
			prev_pass = false;
	}
	delete aMove;
}

//e6af
void CyberOroConnection::handleUndo(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned short player_id, game_code;
	
	if(size != 10)
	{
		qDebug("undo of strange size: %d", size);
		return;
	}
	
	player_id = p[0] + (p[1] << 8);
	game_code = p[2] + (p[3] << 8);
	
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("no board dispatch for undo");
		return;
	}
	PlayerListing * player = getDefaultRoom()->getPlayerListing(player_id);
	p += 4;
	p += 2;	//color byte
	MoveRecord * move = new MoveRecord();
	move->flags = MoveRecord::REQUESTUNDO;
	move->number = p[0] + (p[1] << 8);
	
	//boarddispatch->recvKibitz(0, player->name + QString(" wants undo to move %1.").arg(opp_requests_undo_move_number));
	
	boarddispatch->recvMove(move);
	delete move;
	//from sender perspective	encode(e)
	//e6af 0e00 b50d 4c02 01 00 0a 00 00 00
#ifdef RE_DEBUG
	printf("0xe6af undo: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
}

//0xf0af
void CyberOroConnection::handleDeclineUndo(unsigned char * msg, unsigned int size)
{
	unsigned short player_id, game_code;
	unsigned char * p = msg;
	
	if(size != 10)
	{
		qDebug("declineundo of strange size: %d", size);
		return;
	}
	player_id = p[0] + (p[1] << 8);
	game_code = p[2] + (p[3] << 8);
	
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("no board dispatch for decline undo");
		return;
	}
	PlayerListing * player = getDefaultRoom()->getPlayerListing(player_id);
	
	boarddispatch->recvKibitz(0, player->name + " refused undo.");
	
#ifdef RE_DEBUG
	printf("0xf0af decline undo: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

void CyberOroConnection::handleAcceptUndo(unsigned char * msg, unsigned int size)
{
	unsigned short player_id, game_code;
	unsigned char * p = msg;
	
	if(size != 10)
	{
		qDebug("accept undo msg of strange size: %d", size);
	}
	
	player_id = p[0] + (p[1] << 8);
	game_code = p[2] + (p[3] << 8);
	
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code_to_number[game_code]);
	if(!boarddispatch)
	{
		qDebug("no board dispatch for decline undo");
		return;
	}
	PlayerListing * player = getDefaultRoom()->getPlayerListing(player_id);
	boarddispatch->recvKibitz(0, player->name + " accepted undo.");
	
	p += 4;
	p += 2;	//color byte
	MoveRecord * move = new MoveRecord();
	move->flags = MoveRecord::UNDO;
	move->number = p[0] + (p[1] << 8);
	
	//boarddispatch->recvKibitz(0, player->name + QString(" wants undo to move %1.").arg(opp_requests_undo_move_number));
	
	boarddispatch->recvMove(move);
	delete move;
	
#ifdef RE_DEBUG
	printf("0xebaf accept undo: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//0x42f4: 4d75 bd00 0101 ea00 0d08 0200 0101 000b
//0x42f4: 4d75 bd00 0101 ec00 0b02 0300 0101 000b
//0x42f4: 4d75 bd00 0000 ed00 0201 0f00 0101 000b

//same as 4cf4, 4cf4 being for in room?
//d7af
void CyberOroConnection::handleMoveList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	BoardDispatch * boarddispatch;
	int number_of_moves;
	int i;
	bool pass = false;
	bool enterScore = false;
	GameData * gr;
	MoveRecord * aMove = new MoveRecord();
	unsigned short game_number;
	p += 2;
	unsigned short game_code = msg[2] + (msg[3] << 8);
	game_number = game_code_to_number[game_code];
	p += 2;
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", p[0], p[1]);
		return;
	}
	gr = boarddispatch->getGameData();
	/* FIXME This is rather unfortunate as well but until we fix
	 * qgo_network code's move_counter nonsense, a hold over
	 * from the old code that I just copied without completely
	 * replacing, we do need to take this into account. FIXME */
	
	number_of_moves = msg[4] + (msg[5] << 8);
	p += 2;
	p += 4;
#ifdef RE_DEBUG
	printf("Move List!!!! %d\n", number_of_moves);
#endif //RE_DEBUG
	//player = room->getPlayerListing(p[0] + (p[1] << 8));
	aMove->flags = MoveRecord::NONE;
	//for(i = (gr->handicap ? 1 : 0) ; i < number_of_moves + (gr->handicap ? 1 : 0); i++)
	/* We're sending these starting with 1 because moves after
	 * we'll be offset in that way. i.e., ORO starts with 1*/
	if(gr->handicap)
		aMove->color = stoneWhite;
	else
		aMove->color = stoneBlack;
	for(i = 1; i < number_of_moves + 1; i++)
	{
		//aMove->color = stoneNone;	//keep resetting this
		aMove->number = i;	//or i + !?
		aMove->x = msg[10 + (2 * (i - 1))];
		aMove->y = msg[11 + (2 * (i - 1))];
#ifdef RE_DEBUG
		printf("Move: %d %d\n", msg[10 + (2 * (i - 1))], msg[11 + (2 * (i - 1))]);
#endif //RE_DEBUG
		if(aMove->x == 0)
		{
			aMove->flags = MoveRecord::PASS;
			if(pass/* && aMove->color == stoneWhite*/)	//white passes last
				enterScore = true;
			pass = true;
		}
		else
			pass = false;
		p += 2;
		boarddispatch->recvMove(aMove);
		if(aMove->x == 0)
			aMove->flags = MoveRecord::NONE;	//for next iteration
		if(aMove->color == stoneWhite)
			aMove->color = stoneBlack;
		else
			aMove->color = stoneWhite;
	}
	delete aMove;
	if(p != (msg + size))
		qDebug("Move list with strange size: %d", size);	//20
	/* We get this if we go back into match mode.  Only question is whether if
	 * we crash during score mode, we get a separate message to enter score mode */
	if(enterScore && !match_negotiation_state->isOurGame(game_number))
	{
		printf("Enter score mode, not our game\n");
		boarddispatch->recvEnterScoreMode();
		match_negotiation_state->enterScoreMode();
	}
}

//4cf4 4 zeroes different FIXME join
/* I think this is like for dead games or records or something.  broadcast at the least
* Broadcast is extremely likely.  May also start with that 38f4 betting message */
void CyberOroConnection::handleMoveList2(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	BoardDispatch * boarddispatch;
	int number_of_moves;
	int i;
	unsigned short game_id;
	MoveRecord * aMove = new MoveRecord();
	
	p += 2;
	game_id = p[0] + (p[1] << 8);
	
	GameListing * listing = room->getGameListing(connecting_to_game_number);
	listing->game_code = game_id;
	game_code_to_number[listing->game_code] = listing->number;
#ifdef RE_DEBUG
	printf("Setting game code to number broadcast %02x%02x %d\n", p[0], p[1], listing->number);
#endif //RE_DEBUG
	boarddispatch = getBoardDispatch(listing->number);
	
    emit gameListingReceived(listing);

	connecting_to_game_number = 0;
	
	p += 2;
	
	
	number_of_moves = p[0] + (p[1] << 8);
	p += 2;
	//p += 4;		//doesn't have this !!!
	p += 2;			//instead
#ifdef RE_DEBUG
	printf("Move List Broadcast/Special %d %d!!!!\n", listing->number, listing->game_code);
#endif //RE_DEBUG
	//player = room->getPlayerListing(p[0] + (p[1] << 8));
	aMove->flags = MoveRecord::NONE;
	//for(i = (aGameData->handicap ? 1 : 0) ; i < number_of_moves + (aGameData->handicap ? 1 : 0); i++)
	for(i = 1; i < number_of_moves + 1; i++)
	{
		
		aMove->color = stoneNone;	//keep resetting this
		aMove->number = i;	//or i + !?
		aMove->x = p[0];
		aMove->y = p[1];
#ifdef RE_DEBUG
		printf("Move: %d %d\n", p[0], p[1]);
#endif //RE_DEBUG
		if(aMove->x == 0)
			aMove->flags = MoveRecord::PASS;
		p += 2;
		boarddispatch->recvMove(aMove);
		if(aMove->x == 0)
			aMove->flags = MoveRecord::NONE;	//for next iteration
	}
	delete aMove;
	if(p != (msg + size))
		qDebug("MoveList2 strange size: %d", size);
}

// note that we receive response on this, so likely we should
// set ourself, check box there FIXME
//9a65 maybe?!?!? not a result... probably invitation allowed!!!
void CyberOroConnection::handleInvitationSettings(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	unsigned char * p = msg;
	PlayerListing * player;

	if(size != 8)
	{
		qDebug("Invitation settings msg of strange size %d", size);
	}
	player = room->getPlayerListing(p[0] + (p[1] << 8));
#ifdef RE_DEBUG
	printf("%s ", player->name.toLatin1().constData());
	for(unsigned int i = 2; i < size; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	p += 2;
	//3rd byte after name is invitation settings
	player->info = getStatusFromCode(p[2], player->rank);
	
}

QString CyberOroConnection::getStatusFromCode(unsigned char code, QString rank)
{
	// FIXME
	// What we need to do here is have a function (also for use in
	// strict game dialog functionality) that checks our
	// rank versus the target and adjusts an open flag, just one
	// based on that.
	// game dialog needed something more specific so
    const PlayerListing * us = getOurListing();
	int ret_val = compareRanks(us->rank, rank);
	switch(code)
	{
		case ALLOW_ALL_INVITES:	
			return "O";
			break;
		case ALLOW_STRONGER_INVITE:
			if(ret_val == 1)
				return "O";
			else
				return "X";
			break;
		case ALLOW_EVEN_INVITE:
			if(ret_val == 0)
				return "O";
			else
				return "X";
			break;
		case ALLOW_WEAKER_INVITE:
			if(ret_val == -1)
				return "O";
			else
				return "X";
			break;
		case ALLOW_PAIR_INVITE:
			break;
		case IGNORE_ALL_INVITES:
			return "X";
			break;
	}
	return "";
}

int CyberOroConnection::compareRanks(QString rankA, QString rankB)
{
	if(rankA.contains("p") && !rankB.contains("p"))
		return 1;
	if(rankB.contains("p") && !rankA.contains("p"))
		return -1;
	if(rankA.contains("d") && !rankB.contains("d"))
		return 1;
	if(rankB.contains("d") && !rankA.contains("d"))
		return -1;
	if(rankA.contains("k"))
	{
		QString buffer = rankA;
		buffer.replace(QRegExp("[pdk]"), "");
		int ordinalA = buffer.toInt();
		buffer = rankB;
		buffer.replace(QRegExp("[pdk]"), "");
		int ordinalB = buffer.toInt();
		if(ordinalA > ordinalB)
			return -1;
		else if(ordinalA == ordinalB)
			return 0;
		else
			return 1;
	}
	else
	{
		QString buffer = rankA;
		buffer.replace(QRegExp("[pdk]"), "");
		int ordinalA = buffer.toInt();
		buffer = rankB;
		buffer.replace(QRegExp("[pdk]"), "");
		int ordinalB = buffer.toInt();
		if(ordinalA > ordinalB)
			return 1;
		else if(ordinalA == ordinalB)
			return 0;
		else
			return -1;
	}
}
 
/* It looks like when there's a rematch we get a 1a81, which we need to
 * make sure we recognize, and then a d2af, which should trigger
 * a rematch accept somehow.  Also likely currently a segmentation fault
 * on this. */
/* I'm fairly certain this also doubles as an acceptMatchModeRequest */
void CyberOroConnection::handleMatchOpened(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * player;
	PlayerListing * playerA, * playerB;
	GameData * aGameData = 0;
	unsigned short player_id;
	unsigned short game_id;
	unsigned short game_number;
	int handicap;
	unsigned short black_seconds, white_seconds;
	int black_periods, white_periods;
	float komi;
	bool white_in_byoyomi, black_in_byoyomi;
	bool we_are_challenger = false;
	int board_size;
	bool nigiri = false;
	bool memo, review, estimate;
	if(size != 156)
	{
		qDebug("Match Opened msg of strange size %d", size);
	}
#ifdef RE_DEBUG
	printf("d2af: \n");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	/* FIXME I'm not sure this is the current players turn,
	 * it might just be the challenger or something */
	player_id = p[0] + (p[1] << 8);
	player = room->getPlayerListing(player_id);
#ifdef RE_DEBUG
	if(player)
		printf("Currently %s's turn...\n", player->name.toLatin1().constData());	//not true for nigiri at least, FIXME
	else
		qDebug("Can't find player for for d2af\n");
#endif //RE_DEBUG
	/* This doesn't work */
	//if(player_id == our_player_id)
	//	we_are_challenger = true;
	//else
	//	we_are_challenger = false;
	p += 2;
	game_id = msg[2] + (msg[3] << 8);
	game_number = game_code_to_number[game_id];
	if(!game_number)
		printf("No game number for %02x%02x\n", msg[2], msg[3]);
	/* FIXME, its apparently possible to get this on a match we're
	 * already observing for some reason... if we haven't set a connecting
	 * to number, this causes us to create an unnecessary board */
	/* FIXME is this reliable for our games ? */

	p += 2;
	//b50d 7203 0002 01f0 0201 0300 0000 3c00 3c003c00
	//0002
	//I think this is the rematch number
	//4 and 5 might be the ordinal of this game in a sequence (maybe not)
	p += 2;

	if(connecting_to_game_number)
	{
		if(connecting_to_game_number != game_number)
		{
			printf("Received unexpected d2af\n");
		}
		else
			connecting_to_game_number = 0;
#ifdef RE_DEBUG
		//printf("d2af Setting game code to number %02x%02x %d\n", msg[2], msg[3], game_number);
#endif //RE_DEBUG
		//game_code_to_number[game_id] = game_number;
	}
	else if(match_negotiation_state->isOurGame(game_number))
	{
		if(match_negotiation_state->opponentDisconnected())
		{
			if(match_negotiation_state->getOpponent() != player->name)
			{
				qDebug("Got match opened, but %s is not our opponent", player->name.toLatin1().constData());
				//return;	//FIXME
			}
			qDebug("opponent reconnected");
			match_negotiation_state->setPlayer(player);
			match_negotiation_state->startMatch();
			boarddispatch = getIfBoardDispatch(game_number);
			if(!boarddispatch)
			{
				qDebug("Opponent reconnected but no board!");
				return;
			}
			aGameData = boarddispatch->getGameData();
			if(aGameData->game_code != game_id)		//probably won't
				aGameData->game_code = game_id;
			boarddispatch->startTime();
			QString q;
			boarddispatch->moveControl(q);	
		}
		else
		{
			if(!match_negotiation_state->verifyPlayer(player) &&
						(player_id != our_player_id || (!match_negotiation_state->sentRematch() && !match_negotiation_state->canEnterRematchAdjourned())))
			{
				//on resuming a game after being disconnected, we hit here
				//it could be anything but its amazing that our game is set at all?!? FIXME
				// actually, obviously their id changes when they resume so "verifyPlayer" will fail !!! FIXME
				qDebug("Would have been a rematch but some issue");  //FIXME FIXME
				//FIXME resetting here could be a problem with someone blocking games by causing us to reset when
				//we shouldn't?  maybe not.  I'm being paranoid, this isn't udp
				//match_negotiation_state->reset();
				//return;
				// so yeah, going to go ahead, assuming its okay though should still FIXME
			}
			boarddispatch = getIfBoardDispatch(game_number);
			if(!boarddispatch)
			{
				if(player)
                    getAndCloseGameDialog(player);
				setRoomNumber(game_number);
			}
			else
			{
				aGameData = boarddispatch->getGameData();
				if(aGameData->game_code != game_id)
				{
					/* We're getting here when there's just a rejoin
					* unknown indicator 01 could be resume, but game code is different 
					* note that we get the list of moves when they drop and rejoin!! */
					GameListing * listing = room->getGameListing(game_number);
					delete listing->gameData;
					listing->gameData = 0;
					printf("our rematch\n");
					NetworkConnection::closeBoardDispatch(game_number);
					aGameData = 0;
				}
				else
				{
				
	#ifdef RE_DEBUG
					printf("Maybe a return to match mode our game\n");
	#endif //RE_DEBUG
					//if so, need to restart timers!!! FIXME, added to boarddispatch but doublecheck
					boarddispatch->recvLeaveScoreMode();
					match_negotiation_state->startMatch();
					return;
				}
			}
		}
	}
	else
	{
		boarddispatch = getIfBoardDispatch(game_number);
		if(!boarddispatch)
		{
			qDebug("Unexpected d2af\n");
			return;
		}
		else
		{
			aGameData = boarddispatch->getGameData();
			if(aGameData->game_code != game_id)
			{
				/* FIXME should be combined with earlier deletion of this under our match.
				 * this is ugly incidentally, that we add this strange thing to store the
				 * gameData because ORO doesn't resend on rejoin, and then we have to go
				 * around it like this. */
				/* FIXME pretty sure this can happen sometimes when there's no rematch
				 * so there must be an additional flag */
				GameListing * listing = room->getGameListing(game_number);
				delete listing->gameData;
				listing->gameData = 0;
				printf("observer rematch\n");
				NetworkConnection::closeBoardDispatch(game_number);
				aGameData = 0;
			}
			else
			{
#ifdef RE_DEBUG
				printf("Maybe a return to match mode observer\n");
#endif //RE_DEBUG
				boarddispatch->recvLeaveScoreMode();
				return;
			}
		}
	}
	/* In case of rematches, we close the existing board
	 * dispatch, but only through the network
	 * connection.  We don't want to send a leave msg*/
	/* FIXME for observed games we need to distinguish here between rematches, return to match mode
	 * and spoofed board open packets also see match mode comment above.*/
	
	
	boarddispatch = getBoardDispatch(game_number);
	
	/* Consider adding our name to observer list here: */
	//FIXME shouldn't some other message do this?  doublecheck
	if(room_were_in == game_number)
	{
		PlayerListing * our_player = room->getPlayerListing(our_player_id);
		boarddispatch->recvObserver(our_player, true);
		//if its our game and we want the 2 != 0 to stop,
		//we need to add ourself and opponent to game or make on 1a81 or whatever
		//to do it FIXME
	}
	
	if(!aGameData)
		aGameData = boarddispatch->getGameData();
	
	
	//FIXME make sure this doesn't enable undo button
	//in observed matches
	aGameData->undoAllowed = p[1] & 0x80;
	memo = p[1] & 0x40;
	review = p[1] & 0x20;
	estimate = p[1] & 0x10;
	if(p[1] & 0x08)
	{
		nigiri = true;
		aGameData->nigiriToBeSettled = true;
	}
	/* Maybe 08 is black, 00 is white? and nigiri? */
	p += 2;
	//0201		//one of these may be the board size flag, 9x9
	switch(p[0])
	{
		case 0:
			board_size = 19;
			break;
		case 1:
			board_size = 13;
			break;
		case 2:
			board_size = 9;
			break;
		case 3:
			board_size = 7;
			break;
		case 4:
			board_size = 5;
			break;
		default:
			printf("Strange boardsize indicator: %02x\n", p[0]);
			board_size = 19;
			break;
	}
#ifdef RE_DEBUG
	printf("Unknown indicator: %02x\n", p[1]);
	/* very likely this means a resume after disconnect
	 * I saw 00 for a legitimate rematch... its possible that the ones
	 * that shouldn't open a new window are 01 but I need to verify,
	 * note that this can be in "observer rematch" as well*/
#endif //RE_DEBUG
	p += 2;
	
	handicap = p[0];
	
	if(handicap)
	{
		//maybe clear komi?
		if(handicap == 1)
			handicap = 0;
	}
	
	p++;
	if(p[0])
		komi = -((float)(p[1])) - 0.5;
	else
		komi = ((float)(p[1])) + 0.5;
	
	p += 5;			//this skips one time duplicate as well
	black_seconds = p[0] + (p[1] << 8);
	p += 2;
	white_seconds = p[0] + (p[1] << 8);
	p += 2;
	aGameData->maintime = black_seconds;	//FIXME they're the same right?
	
	black_periods = p[0];
	white_periods = p[1];
	p += 2;
	black_in_byoyomi = p[0];
	white_in_byoyomi = p[1];
	/* If maintime is set to 0, we need to jump into periods.  Not
	 * sure why this isn't set on incoming message, I guess because match
	 * just opened. */
	if(!black_in_byoyomi && aGameData->maintime)
		black_periods = -1;	//FIXME, awkward flags
	if(!white_in_byoyomi && aGameData->maintime)
		white_periods = -1;
	p += 2;
	
	aGameData->periodtime = p[0] + (p[1] << 8);
	p += 2;
	aGameData->stones_periods = p[0];
	switch(p[1])
	{
		case 0:
			aGameData->timeSystem = byoyomi;
			break;
		case 1:
			aGameData->timeSystem = canadian;
			break;
		case 2:
			aGameData->timeSystem = tvasia;
			break;
	}
	p += 2;
	
	//00 00 A4 01 A4 01 A4 01 05 05 00 00 3C 00 05 00  ............<...
//02 00 00 00 			//these second two 00 00 might be a move number?
	p += 4;
	game_number = p[0] + (p[1] << 8);
#ifdef RE_DEBUG
	printf("Room number : %d\n", game_number);
#endif //RE_DEBUG
//96 00 96 00 
	p += 4;
	p += 23;
	p++;
	p += 10;
    playerA = room->getPlayerListing(p[0] + (p[1] << 8));
	p += 2;
#ifdef RE_DEBUG
	printf("PlayerA is %s %s\n", playerA->name.toLatin1().constData(), playerA->rank.toLatin1().constData());
#endif //RE_DEBUG
	p += 10;
	playerB = room->getPlayerListing(p[0] + (p[1] << 8));
#ifdef RE_DEBUG
	printf("PlayerB is %s %s\n", playerB->name.toLatin1().constData(), playerB->rank.toLatin1().constData());
#endif //RE_DEBUG
	p += 2;
	/* FIXME It looks like its possible to reverse the ranks here and
	 * we need to find out why and how */

	/* Double check for nigiri issues */
	if(match_negotiation_state->isOurGame(game_number))
	{
		if(playerA->id == our_player_id)
		{
			aGameData->opponent_id = playerB->id;
			if(nigiri)
				we_are_challenger = true;
		}
		else
		{
			aGameData->opponent_id = playerA->id;
		}
		match_negotiation_state->startMatch();
	}
	aGameData->black_name = playerA->name;
	aGameData->white_name = playerB->name;
	aGameData->black_rank = playerA->rank;
	aGameData->white_rank = playerB->rank;
	aGameData->board_size = board_size;
	aGameData->handicap = handicap;
	aGameData->number = game_number;
	aGameData->game_code = game_id;
	aGameData->komi = komi;
	boarddispatch->openBoard();
	boarddispatch->recvTime(TimeRecord(white_seconds, white_periods), TimeRecord(black_seconds, black_periods));
	
	if(playerA->id == our_player_id || playerB->id == our_player_id)
	{
		if(!nigiri)
		{
			startMatchTimers((playerA->id == our_player_id && !handicap) ||
					(playerB->id == our_player_id && handicap));
		}
		else if(nigiri && we_are_challenger)
			sendNigiri(aGameData->game_code, true);
		else
		{
			boarddispatch->recvKibitz(QString(), "Waiting for opponent nigiri...");
			we_send_nigiri = false;
		}
	}
	/* seems like color can get reversed */

	/* And observers: */
	if(room_were_in == game_number)		//no observing outside
		addObserverListToBoard(boarddispatch);
}

void CyberOroConnection::addObserverListToBoard(BoardDispatch * boarddispatch)
{
	GameListing * game = getDefaultRoom()->getGameListing(boarddispatch->getGameData()->number);
	//sanity check: for DEBUG ing
	/* FIXME We're getting 2 != 1 and a crash here 
	 * also 2 != 0, has something to do with if its our game*/
#ifdef RE_DEBUG
	if(game->observers != game->observer_list.size())
		qDebug("Observers and observer list size mismatch on game %d!! (%d != %d)", boarddispatch->getGameData()->number, game->observers, game->observer_list.size());
#endif //RE_DEBUG
	for(std::vector<PlayerListing *>::iterator oblistit = game->observer_list.begin();
		oblistit != game->observer_list.end(); oblistit++)
		boarddispatch->recvObserver(*oblistit, true);
}

//38 F4 78 00 01 00 00 08 18 00 00 00 06 00 50 46  8.x...........PF
//2A 0B 3B 00 05 01 00 01 3C 00 05 00 02 00 4B 2E  *.;.....<.....K.
//4B 6F 62 61 79 61 73 68 41 2E 4B 61 74 6F 00 00  KobayashA.Kato..
//00 00 8F AC 97 D1 8C F5 88 EA 00 00 89 C1 93 A1  ................
//8F 5B 8E 75 00 00 03 03 C9 C8 03 05 00 00 00 00  .[.u............
//00 00 00 00 1F 01 96 BC 90 6C 90 ED 8D C5 8F 49  .........l.....I
//97 5C 91 49 00 00 00 00 00 00 00 00 00 00 00 00  .\.I............
//00 00 00 00 01 00 00 1F
/* Double check that this isn't just for joining of dead matches or broadcast deads 
 * Okay, this was a mistake altogether, it was a betting message match that got
 * lodged in there. */
void CyberOroConnection::handleObserveAfterJoining(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	//BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * player;
	/*GameListing * game;
	GameData * aGameData;
	unsigned short game_id;
	int handicap;
	unsigned short black_seconds, white_seconds;
	int black_periods, white_periods;
	unsigned short period_time;
	float komi;
	bool send_handicap_move = false;
	bool white_in_byoyomi, black_in_byoyomi;*/
	
	printf("FIXME handleObserveAfterJoining size: %d\n", size);
	
	player = room->getPlayerListing(p[0] + (p[1] << 8));
#ifdef FIXME
	if(player)
		printf("Currently %s's turn...\n", player->name.toLatin1().constData());
	else
		printf("Can't find player for for d2af\n");
	printf("38f4\n");
	for(int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //FIXME
	//FIXME
}

void CyberOroConnection::handleMatchOffer(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned short periodtime;
	unsigned short room_number;
	int periods;
	bool undo, memo, review, estimate;
	bool nigiri = false;
	MatchRequest mr;
	Room * room = getDefaultRoom();
	unsigned short id = p[0] + (p[1] << 8);
	if(size != 156)
	{
		qDebug("match offer message of strange size %d", size);
	}
	if(id == our_player_id)
		mr.opponent_is_challenger = false;
	else
	{
		mr.opponent_id = id;
		mr.opponent_is_challenger = true;
	}
	/* FIXME I think its okay if we get this as long as we're in the same room with the opponent */

	PlayerListing * player = room->getPlayerListing(mr.opponent_id);
	PlayerListing * ourplayer = room->getPlayerListing(our_player_id);
	
	mr.opponent = player->name;
	mr.their_rank = player->rank;
	mr.our_name = ourplayer->name;
	mr.our_rank = ourplayer->rank;
	
	p += 2;
#ifdef RE_DEBUG
	printf("last game id?: %02x %02x %d\n", p[0], p[1], p[0] + (p[1] << 8));
#endif //RE_DEBUG
	mr.last_game_code = p[0] + (p[1] << 8);
	p += 2;
#ifdef RE_DEBUG
	printf("bytes we need: %02x %02x\n", p[0], p[1]);
#endif //RE_DEBUG
	p += 2;
	//01f0
	// first byte 01 = friendly, 00 = promotional/rated?
	if(p[0])
		mr.free_rated = FREE;
	else
		mr.free_rated = RATED;
	//second byte contains partly bit field for settings
	// double check
	undo = p[1] & 0x80;
	memo = p[1] & 0x40;
	review = p[1] & 0x20;
	estimate = p[1] & 0x10;
	mr.flags = p[1] & 0xf0;
#ifdef RE_DEBUG
	printf("Possible color indicator: %02x\n", p[1] & 0x0f);
#endif //RE_DEBUG
	/*switch(p[1] & 0x0f)
	{
		case 0:
			mr.color_request = MatchRequest::WHITE;
			break;
		case 8:
			mr.color_request = MatchRequest::BLACK;
			break;
		default:
			break;
	}*/
	if(p[1] & 0x08)
	{
		mr.color_request = MatchRequest::NIGIRI;
		nigiri = true;
	}
	/* Maybe 08 is black, 00 is white? and nigiri? */
	p += 2;
	//0201		//one of these may be the board size flag, 9x9
	switch(p[0])
	{
		case 0:
			mr.board_size = 19;
			break;
		case 1:
			mr.board_size = 13;
			break;
		case 2:
			mr.board_size = 9;
			break;
		case 3:
			mr.board_size = 7;
			break;
		case 4:
			mr.board_size = 5;
			break;
		default:
			printf("Strange boardsize indicator: %02x\n", p[0]);
			break;
	}
#ifdef RE_DEBUG
	printf("Unknown indicator: %02x\n", p[1]);
#endif //RE_DEBUG
	/* This doesn't necessarily mean challenger is black,
	 * I don't know what this means, I've seen it as 0,
	 * for challenger as all colors */
	mr.challenger_is_black = p[1];
	p += 2;
	//0300		//probably handicap
	mr.handicap = p[0];
	p++;
	if(p[0])
		mr.komi = -((float)(p[1])) - 0.5;
	else
		mr.komi = ((float)(p[1])) + 0.5;
	//0400;		//probably komi
	p += 3;
	//5802		//here main time is repeated there times !?!
	mr.maintime = p[0] + (p[1] << 8);
	p += 6;
	//0505		//probably time periods, but I think we get this later as well
	p += 2;
	//0000
	p += 2;
	periodtime = p[0] + (p[1] << 8);
	p += 2;
	//0500
	periods = p[0];
	// that second byte MIGHT be the game style
	// 0 for byo yomi, 1 for canada, 2, for "tv asia"
	// pretty certain this is FIXME correct time style !!!!
	switch(p[1])
	{
		case 0:
			/* Really this is korean, but hopefully its the same thing */
			mr.timeSystem = byoyomi;
			break;
		case 1:
			mr.timeSystem = canadian;
			break;
		case 2:
			mr.timeSystem = tvasia;
			break;
		default:
			printf("Unknown time style indicator %02x\n", p[1]);
			break;
	}
	mr.stones_periods = periods;
	mr.periodtime = periodtime;
	p += 2;
	//0200
#ifdef RE_DEBUG
	printf("First weird byte: %02x %02x\n", p[0], p[1]);
#endif //RE_DEBUG
	p += 2;
	//0000 3900
	p += 2;
	room_number = p[0] + (p[1] << 8);
#ifdef RE_DEBUG
	printf("mr number: %d\n", room_number);
#endif //RE_DEBUG
	mr.number = room_number;
	p += 2;
	p += 14;		//zeroes
	//1f00
	p += 2;
	p += 8;			//zeroes
	//0075
#ifdef RE_DEBUG
	/* FIXME, not really sure what this byte is,
	 * fairly certain its unrelated to color though*/
	if(p[1] & 0x10)
		printf("0x10 setting on\n");
	else
		printf("0x10 setting off\n");
	if(p[1] & 0x20)
		printf("0x20 setting on\n");
	else
		printf("0x20 setting off\n");
	if(p[1] & 0x40)
		printf("0x40 setting on\n");
	else
		printf("0x40 setting off\n");
	if(p[1] & 0x80)
		printf("0x80 setting on\n");
	else
		printf("0x80 setting off\n");
	if(p[1] & 0x01)
		printf("0x01 setting on\n");
	else
		printf("0x01 setting off\n");
	if(p[1] & 0x02)
		printf("0x02 setting on\n");
	else
		printf("0x02 setting off\n");
	if(p[1] & 0x04)
		printf("0x04 setting on\n");
	else
		printf("0x04 setting off\n");
	if(p[1] & 0x08)
		printf("0x08 setting on\n");
	else
		printf("0x08 setting off\n");
#endif //RE_DEBUG
		
	
	p += 2;
	p += 10;		//name and id
	unsigned short black_challenger_id = p[0] + (p[1] << 8);
	if(black_challenger_id == our_player_id && !nigiri)
		mr.color_request = MatchRequest::BLACK;
	else if(!nigiri)
	{
		mr.color_request = MatchRequest::WHITE;
		if(black_challenger_id != our_player_id)
			mr.opponent_id = black_challenger_id;
	}
	p += 2;
	p += 10;		//name and id
	unsigned short white_non_challenger_id = p[0] + (p[1] << 8);
	if(white_non_challenger_id != our_player_id)
		mr.opponent_id = white_non_challenger_id;
	p += 2;
	/* Either challenger is second, or white is second or there's another
	 * flag to indicate which is which */
	p += 22;	//zeroes
	//1027
	p += 2;
	p += (3 * 16); 	//zeroes
	
#ifdef RE_DEBUG
	printf("0xc8af: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
    GameDialog * gameDialogDispatch = getGameDialog(player);
	gameDialogDispatch->recvRequest(&mr, getGameDialogFlags());
}

//5fc3
void CyberOroConnection::handleResumeMatch(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned short room_number = p[0] + (p[1] << 8);
	unsigned short our_id, their_id;
	PlayerListing * opponent;
	p += 2;
	our_id = msg[2] + (msg[3] << 8);
	if(our_id != our_player_id)
	{
#ifdef RE_DEBUG
		qDebug("resume game not meant for us! %02x%02x", msg[2], msg[3]);	//happens often
#endif //RE_DEBUG
		return;
	}
	their_id = msg[4] + (msg[5] << 8);
	
	opponent = getDefaultRoom()->getPlayerListing(their_id);
    // ffs? lots of ffs?
#ifdef RE_DEBUG
	printf("resume adjourned? 5fc3?: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	match_negotiation_state->setupRematchAdjourned(room_number, opponent->name);
	/* It only get set as our game if its got our name in it
	 * and even that may not be enough but... treat like
	 * 0a7d from here.
	
	 * Actually, this may be a bad idea because it can screw up
	 * the player names !!! ours is always first regardless of color
	 * we'd have to make sure that 0a7d is not relied on for color
	 * and more than that that the d2af does set the color up properly.*/
	/* More than that, it causes NewRoom to complain about size 18 */
	handleNewRoom(msg, size);
#ifdef RE_DEBUG
	qDebug("Resuming\n");
#endif //RE_DEBUG
}
/* FIXME, combine these very common handler messages into one function */

//b5b3
void CyberOroConnection::handleAdjournRequest(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	
	if(size != 8)
	{
		qDebug("Adjourn request of strange size: %d", size);
	}
	//unsigned short opponent_id = p[0] + (p[1] << 8);
	unsigned short game_code = p[2] + (p[3] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Received adjourn decline on nonexistent board");
		return;
	}
	boarddispatch->recvRequestAdjourn();

	p += 2;
	//their id game id, maybe color?
#ifdef RE_DEBUG
	qDebug("b5b3 size: %d", size);
#endif //RE_DEBUG
}

//bfb3
void CyberOroConnection::handleAdjournDecline(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	if(size != 8)
	{
		qDebug("Adjourn decline of strange size: %d", size);
	}
	//unsigned short opponent_id = p[0] + (p[1] << 8);
	unsigned short game_code = p[2] + (p[3] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Received adjourn decline on nonexistent board");
		return;
	}
	boarddispatch->recvRefuseAdjourn();

	p += 2;
	//their id game id, maybe color?
//5304 430d 0100 0071
#ifdef RE_DEBUG
	printf("0xbfb3 likely adjourn decline: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");

	qDebug("bfb3 size: %d", size);
#endif //RE_DEBUG
}

//bab3
void CyberOroConnection::handleAdjourn(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	if(size != 8)
	{
		qDebug("Adjourn of strange size: %d", size);
	}
	//unsigned short opponent_id = p[0] + (p[1] << 8);
	unsigned short game_code = p[2] + (p[3] << 8);
	unsigned short game_number = game_code_to_number[game_code];
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Received adjourn  on nonexistent board");
		return;
	}
	/* FIXME, why do we need to close the board dispatch, shouldn't
	* the adjournGame do that?  or should it, maybe we can talk after? */
	boarddispatch->adjournGame();
	//connection->closeBoardDispatch(memory);
#ifdef RE_DEBUG
	qDebug("bab3 size: %d", size);
#endif //RE_DEBUG
}

/* This pops up in a short, maybe 10 - 30 second dialog that then automatically
 * declines.  If you hit accept, then you go into the match offer dialog.
 * I'm not sure if we want to auto accept (if we're looking/open) and then
 * let the player decline from the settings if they want to?  Or maybe that
 * would make the client appear rude and we should instead show a popup
 * for this message... which is a different sort of behavior to emulate... */
 //2279
void CyberOroConnection::handleMatchInvite(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	if(size != 12)
	{
		qDebug("MatchInvite of size %d\n", size);
		// probably should print it out and exit FIXME
	}
	Room * room = getDefaultRoom();
	PlayerListing * player = room->getPlayerListing(p[0] + (p[1] << 8));
	//22 79 10 00 FC 08 27 00 00 08 09 00 00 00 00 7B
	p += 2;
	// the 00 08 is our id, 9 could be special byte but the 27 changes
	
#ifdef RE_DEBUG
	printf("%s requests match %02x %02x %02x\n", player->name.toLatin1().constData(), p[0], p[1], p[4]);
#endif //RE_DEBUG
	/* FIXME, I really don't think we want to auto accept.  It puts one in
	 * a special room And then if you go to observe after joining in
	 * another game in progress, you leave the match room.  */
	/* FIXME blocks here right now because match_negotiation_state isn't properly reset */
	if(!match_negotiation_state->newMatchAllowed())
		return;
	
	MatchInviteDialog * mid = new MatchInviteDialog(player->name, player->rank);
	int mid_return = mid->exec();
	
	if(mid_return == 1)
        sendMatchInvite(player, true);
	else if(mid_return == -1)
        sendDeclineMatchInvite(player);
}

//0x147d
void CyberOroConnection::handleAcceptMatchInvite(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	if(size != 4)
	{
		qDebug("MatchInviteAccept of size %d\n", size);
		// probably should print it out and exit FIXME
	}
	PlayerListing * player = getDefaultRoom()->getPlayerListing(p[0] + (p[1] << 8));
    if(!match_negotiation_state->sentMatchInvite())
		return;
	if(!match_negotiation_state->verifyPlayer(player))
		return;
    sendMatchOfferPending(player);
	
#ifdef RE_DEBUG
	printf("0x147d: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//FIXME I suspect that the a479 is the match invite decline
//but 0c2b is also a candidate
//0xa479: 3e0d1c02ab0000ef
//0x0c2b: 3e0d1c02ab0000ef

//a479
void CyberOroConnection::handleDeclineMatchInvite(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	if(size != 8)
	{
		qDebug("MatchInviteDecline of size %d\n", size);
		// probably should print it out and exit FIXME
	}
	PlayerListing * player = getDefaultRoom()->getPlayerListing(p[0] + (p[1] << 8));
    if(!match_negotiation_state->sentMatchInvite())
		return;
	if(!match_negotiation_state->verifyPlayer(player))
		return;
	match_negotiation_state->reset();
	QMessageBox mb(tr("Invite declined"), tr("%1 has declined invitation").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
			QMessageBox::NoButton, QMessageBox::NoButton);
	mb.exec();
	
#ifdef RE_DEBUG
	printf("0xa479: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

//0x1e7d
void CyberOroConnection::handleMatchOfferPending(unsigned char * msg, unsigned int size)
{
	/* What's the difference between this and the accept match invite?  Any ?? */
	/* This is the copyback of our own sendMatchOfferPending */
#ifdef RE_DEBUG
	printf("0x1e7d: ");
	for(int i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	//590c 2d07 00ae
	unsigned char * p = msg;
	if(size != 6)
	{
		qDebug("MatchRoomOpen of size %d\n", size);
		// probably should print it out and exit FIXME
	}
	if(msg[0] + (msg[1] << 8) != our_player_id)
	{
		/* Actually, could this be the game id? FIXME
		 * But we ignore it if... no, see because below, we're using it
		 * to open the game dialog offer... */
#ifdef RE_DEBUG
		/* Potentially this is regarding our opponent and we should also
		 * have a standard way of printing out IDS FIXME */
		qDebug("Received MatchRoomOpen for someone besides us %02x%02x", p[1], p[0]);
#endif //RE_DEBUG
		return;
	}
	p += 2;
	PlayerListing * player = getDefaultRoom()->getPlayerListing(msg[2] + (msg[3] << 8));
    p += 2;
	// no idea what this is
	//setRoomNumber(p[0] + (p[1] << 8));

	if(!match_negotiation_state->sentMatchOfferPending())
		return;
	if(!match_negotiation_state->verifyPlayer(player))
		return;
	
    GameDialog * gd = getGameDialog(player);
	gd->recvRequest(0, getGameDialogFlags());
}

/* The point of this is that we should clear out the observer lists
 * on rooms we leave, etc. */
void CyberOroConnection::setRoomNumber(unsigned short number)
{
	if(room_were_in == number)
		return;
	if(room_were_in)
	{
		BoardDispatch * bd = getIfBoardDispatch(room_were_in);
		if(bd)
			bd->clearObservers();
	}
	room_were_in = number;
}

/* There's also FIXME FIXME FIXME, 
 * these massive 0x8129 messages at the start of the game, 
 * looks like an opponents records or something... maybe a picture?
 * I have no idea. and a 0x7c29 right before it, short but who knows? 
 * And then after an 0x8629, again, short. check "truegame" and "truegame2".
 * Actually wait, I think these are coming from after a game!! like result
 * messages. */

/* These needs to be more versatile later */
void CyberOroConnection::onReady(void)
{
	//sendInvitationSettings(true);	//for now
	qDebug("Ready!\n");
	NetworkConnection::onReady();
}

int CyberOroConnection::gd_verifyBoardSize(int v)
{
	/* If game is ranked, has to be 19x19 but right now
	 * game dialog has an old flag that sets that up */
	if(v < 5)
		return 5;
	if(v > 13)
		return 19;
	if(v > 9)
		return 13;
	if(v > 7)
		return 9;
	if(v > 5)
		return 7;
	return v;
}

QTime CyberOroConnection::gd_checkMainTime(TimeSystem s, const QTime & t)
{
	int seconds = (t.minute() * 60) + t.second();
	switch(s)
	{
		case byoyomi:		//korean
			if(t.hour() > 8 || (t.hour() == 8 && t.minute() > 20))
				return QTime(8, 20, 0);
			else
				return QTime(t.hour(), t.minute(), 0);
			break;
		case canadian:
			if(t.hour() >= 1)
				return QTime(1, 0, 0);
			else
				return QTime(t.hour(), t.minute(), 0);
		case tvasia:
			if(t.minute() >= 5)
				return QTime(0, 5, 0);
			else
			{
				if(seconds % 10)
					seconds += (10 - (seconds % 10));
				int minutes = seconds;
				seconds %= 60;
				minutes -= seconds;
				minutes /= 60;
				return QTime(0, minutes, seconds);
			}
			break;
		default:
			qDebug("unsupported CyberORO time type");
			break;
	}
	return t;
}

QTime CyberOroConnection::gd_checkPeriodTime(TimeSystem s, const QTime & t)
{
	int seconds = (t.minute() * 60) + t.second();
	switch(s)
	{
		case byoyomi:
			if(t.minute() >= 1)
				return QTime(0, 1, 0);
			else
			{
				if(seconds % 10)
					seconds += (10 - (seconds % 10));
				if(seconds == 60)
					return QTime(0, 1, 0);
				else
					return QTime(0, 0, seconds);
			}
			break;
		case canadian:
			if(t.hour() >= 1)
				return QTime(1, 0, 0);
			else if(t.minute() == 0)
				return QTime(0, 1, 0);
			else
				return QTime(t.hour(), t.minute(), 0);
		case tvasia:
			if(t.minute() >= 5)
				return QTime(0, 5, 0);
			else
			{
				if(seconds % 10)
					seconds += (10 - (seconds % 10));
				int minutes = seconds;
				seconds %= 60;
				minutes -= seconds;
				minutes /= 60;
				return QTime(0, minutes, seconds);
			}
			break;
		default:
			qDebug("unsupported CyberORO time type");
			break;
	}
	return t;
}

unsigned int CyberOroConnection::gd_checkPeriods(TimeSystem s, unsigned int p)
{
	switch(s)
	{
		case byoyomi:
			if(p == 0)
				return 1;
			else if(p > 15)
				return 15;
			else
				return p;
			break;
		case canadian:
			if(p < 2)
				return 2;
			else if(p > 50)
				return 50;
			else
				return p;
			break;
		case tvasia:
			if(p == 0)
				return 1;
			else if(p > 10)
				return 10;
			else
				return p;
			break;	
		default:
			qDebug("unsupported CyberORO time type");
			break;
	}
	return p;
}

const char * CyberOroConnection::getCodecString(void)
{
	return serverList[current_server_index]->codec;
}

QString CyberOroConnection::getPlaceString(void)
{
	return "CyberOro: " + serverList[current_server_index]->name;
}

int CyberOroConnection::time_to_seconds(const QString & time)
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
