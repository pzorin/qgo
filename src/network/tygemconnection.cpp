#include <string.h>
#include <stdint.h>
#include <time.h>
#include <algorithm>
#include "tygemconnection.h"
#include "tygemprotocol.h"
#include "consoledispatch.h"
#include "room.h"
#include "boarddispatch.h"
#include "gamedialog.h"
#include "talk.h"
#include "dispatchregistries.h"
#include "serverlistdialog.h"
#include "codecwarndialog.h"
#include "matchinvitedialog.h"
#include "gamedialogflags.h"
//FIXME set phrases would be useful... 
//#include "orosetphrasechat.h"
//#include "setphrasepalette.h"
#include "createroomdialog.h"
#include "gamedata.h"
#include "playergamelistings.h"
#include "serverliststorage.h"
#include "matchnegotiationstate.h"
#include <QMessageBox>

//#define RE_DEBUG

unsigned char zodiac_byte;
/* I now think you have to buy avatar icons at the store,
 * so we're not going to mess with this */
/* 57 is our zodiac sign 
		01 is rat, 02 is bull, 03 is tiger, 04 is rabbit
		05 is dragon, 06 is snake, 07 is horse
		08 is sheep (ram? goat?), 09 is monkey
		0a is rooster, 0b is dog, 0c is pig
		
		0d is mercury like chibi figure with golden shield
		0e is red warrior palace chibi
		0f is wild west gunman on horse
		10 is chibi in snow sweeping
		11 red warrior bamboo forest
		12 calligrapher
		13 red haired bureaucrat, purple cap?
		14 red haired bureacrat, scroll, white cap
		15 guy with shield on battlefield?
		16 two blade zhong warrior night time balcony
		17 same with no zhong, bigger blades
		24 man running with bunny
		30 sports uniform kid with korean flag
		34 child spinning soccer ball on finger
		35 small girl playing go on computer
		40 soccer kid
		43 woman reading book with headphones
		50 purple mouse, bowtie, cheese on platter
		55 snake around yellow stone
		56 horse
		57 giant chicken
		58 strange blue purple dog with bone around neck
		59 childs drawing of pig in love
		5a woman walking dog
		5b ballet dancer stretching
		5c woman on balcony looking out over ocean
		5d girl blue flowers in hair, blue dress
		5e 31 gangster on dark blue background
		5f pig tailed beach girl
		60 blackhaired girl rainbow very blue
		61 ghetto guy against graffiti wall
		62 green haired boxing girl on sun like background
		63 frightened cloaked guy holding skull
		64 fierce blonde pigtailed girl on blue sky
		65 dominatrix infront of night time castle
		66 pigtailed brunette princess in red and gold background
		67 studying girl
		68 strong guy saying something, korean letters, blue star hat
		69 woman purple sheath pink background
		6a black haired girl black dress, roses
		6b pink girl holding valentine at night with moon
		7e ninja nighttime, gold crown
		7f japanese warlord
		80 new tygem congratulations, white smiling figure, gold badge
		81 same with black smirking figure
		82 same with out gold badge
		90 green haired happy girl under night sky smiling moon?
		91 grey pig eating or happy at table with food
		92 2 kids in love
		93 bowl of orange fruit
		94 night life guy with flowers
		95 night life business lady?
		96 blonde smiling guy showbiz stars
		97 korean infinity girl picture
		98 white creature with brown hat, fall leaves
		99 white creatures riding dolphins
		9a white and black creatures playing game
		a0 thin santa claus, merry christmas
		b0 animated bouncing girl on pink hearts
		c0 korean letters, boiling chicken, and dancing chickens
		d0 scarecrow golden field
		e0 no picture
		FIXME */

TygemConnection::TygemConnection(const QString & user, const QString & pass, ConnectionType connType)
{
	username = user;
	password = pass;
	/*if(openConnection(info))
	{
		connectionState = LOGIN;
		
	}
	else
		qDebug("Can't open Connection\n");	//throw error?
*/
	textCodec = QTextCodec::codecForLocale();
	serverCodec = QTextCodec::codecForLocale();
	//FIXME check for validity of codecs ??? 
	//ORO_setup_setphrases();
	//serverCodec = QTextCodec::codecForName("GB2312");
	
	http_connect_content_length = 0;
	current_server_index = -1;
	encode_offset = 0;
	
	matchKeepAliveTimerID = 0;
	matchRequestKeepAliveTimerID = 0;
	retryLoginTimerID = 0;
	opponentDisconnectTimerID = 0;
	/* We should either create the palette on creation of
	 * the connection, or have a button to activate it that
	 * appears when the room is connected */
	//setphrasepalette = new SetPhrasePalette(this);
	//setphrasepalette->show();
	
	//for server list
	//error handling FIXME
	connectionType = connType;
	if(connType == TypeTYGEM)
	{
		serverCodec = QTextCodec::codecForName("eucKR");
		if(!serverCodec)
		{
			new CodecWarnDialog("eucKR");
			serverCodec = QTextCodec::codecForLocale();
		}
		if(!getServerListStorage().restoreServerList(TypeTYGEM, serverList))
			requestServerInfo();
		else
		{
			if(reconnectToServer() < 0)
			{
				qDebug("User canceled");
				connectionState = CANCELED;
				return;
			}
		}
	}
}

/* I think we can't request this too often which is why hitting cancel and
 * trying to reconnect screws things up.  So I'm thinking we need to store
 * this some where for at least the three tygem protocols and possibly oro
 * as well. retrieve on application start, gray out the tabs if we can't
 * get the info. */
int TygemConnection::requestServerInfo(void)
{
	qDebug("Requesting Tygem Server Info");
	if(!openConnection("121.189.9.52", 80, NOT_MAIN_CONNECTION))
	{
		qDebug("Can't get server info");
		return -1;
	}
	unsigned int length = 0x96;
	unsigned char * packet = new unsigned char[length];  //term 0x00
	snprintf((char *)packet, length,
		 "GET /service/down_korea/livebaduk3.cfg HTTP/1.1\r\n" \
		 "User-Agent: Tygem HTTP\r\n" \
		 "Host: service.tygem.com\r\n" \
		 "Connection: Keep-Alive\r\n" \
		 "Cache-Control: no-cache\r\n\r\n");
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
	{
		qWarning("*** failed sending server info request");
		return -1;
	}
	delete[] packet;
	
	connectionState = INFO;
	return 0;
}

TygemConnection::~TygemConnection()
{
	//if(setphrasepalette)
	//	delete setphrasepalette;
	// FIXME for non tygem subclasses!
	qDebug("Destroying Tygem connection");
	closeConnection();
	//backup serverlist
	getServerListStorage().saveServerList(connectionType, serverList);
}

void TygemConnection::OnConnected()
{
}

void TygemConnection::sendDisconnect(void)
{
	/* Yeah, FIXME, we don't want to send this
	 * when we're reconnecting... which means we
	 * need a special connection state ... yada yada */
	if(connectionState == CONNECTED)
		sendDisconnectMsg();
}

void TygemConnection::sendText(const char * text)
{
	qDebug("Wanted to send %s", text);
	return;
}

void TygemConnection::sendMsg(PlayerListing & player, QString text)
{
	Talk * t = getIfTalk(player);
	if(!t)
	{
		qDebug("sendMsg called with no talk dialog");
		return;
	}
	if(!t->getConversationOpened())
	{
		sendOpenConversation(player);
		pendingConversationMsg[&player] = text;
	}
	else
	{
		sendConversationMsg(player, text.toLatin1().constData());
	}
}

void TygemConnection::sendToggle(const QString & param, bool val)
{
	/* FIXME, more to do here probably */
	qDebug("Setting open %d", val);
	if(param == "open")
		sendInvitationSettings(val);
}

/* If we can observe something besides games,
 * like rooms or people, then this name is a bit
 * iffy too... */
void TygemConnection::sendObserve(const GameListing & game)
{
	sendJoin(game.number);
}

void TygemConnection::sendJoin(unsigned short game_number)
{
	unsigned int length = 24;
	char *packet = new char[length];
	int i;
	if(getBoardDispatches() == 3)
	{
		//legit client won't let you watch more than 3 games
		QMessageBox::information(0, tr("3 Boards Open"), tr("You must close a board before you can open another one"));
		//message box, FIXME
		//it is allowed but probably taxes server, as server sets
		//time, etc... and who watches more than three games at a time...
		//besides people testing hacked together clients?
		return;
	}
	/* We need to turn this to a join if its a room, not a game
	* otherwise bad things happen FIXME */
	
	packet[0] = 0x00;
	packet[1] = 0x18;
	packet[2] = 0x06;
	packet[3] = 0x38;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	packet[6] = 0x00;
	packet[7] = 0x00;	
	for(i = 8; i < 0x18; i++)
		packet[i] = 0x00;
#ifdef RE_DEBUG
	//printf("Sending observe  to %s vs %s: %d", game.white_name().toLatin1().constData(), game.black_name().toLatin1().constData(), game.game_code);
	printf("Sending observe to %d\n", game_number);
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
#ifdef RE_DEBUG
	printf("After encode\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending observe outside");
	delete[] packet;
}

#ifdef OLD
/* This is sent after "sendObserve" when we join a match */
/* FIXME if it ever becomes an issue, we might want to have
 * the 0639 message handleBoardOpen or whatever sendJoin
 * instead of just calling them one after the other.  Can't
 * see why it would be an issue though */
void TygemConnection::sendJoin(unsigned short game_number)
{
	unsigned int length = 12;
	char * packet = new char[length];
	int i;
	
	packet[0] = 0x00;
	packet[1] = 0x0c;
	packet[2] = 0x06;
	packet[3] = 0x65;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	for(i = 6; i < 12; i++)
		packet[i] = 0x00;
#ifdef RE_DEBUG
	printf("Sending join %d", game_number);
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
#ifdef RE_DEBUG
	printf("After encode\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending observe outside");
	delete[] packet;
}
#endif //OLD

/* This is sent after sendJoin when we've been disconnected.
 * Presumably its to resume the game */
void TygemConnection::sendResume(unsigned short game_number)
{
	unsigned int length = 12;
	char * packet = new char[length];
	int i;
	
	packet[0] = 0x00;
	packet[1] = 0x0c;
	packet[2] = 0x06;
	packet[3] = 0x86;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	for(i = 6; i < 12; i++)
		packet[i] = 0x00;
#ifdef RE_DEBUG
	printf("Sending resume %d", game_number);
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
#ifdef RE_DEBUG
	printf("After encode\n");
	for(i = 0; i < (int)length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending resume");
	delete[] packet;
	//match_negotiation_state FIXME
}

void TygemConnection::closeTalk(PlayerListing & opponent)
{
	Talk * t = getIfTalk(opponent);
	if(!t)
	{
		qDebug("closing talk but no talk registered!!");
		return;
	}
	if(t->getConversationOpened())
		sendCloseConversation(opponent);
	NetworkConnection::closeTalk(opponent);
}

void TygemConnection::closeBoardDispatch(unsigned int game_id)
{
	sendFinishObserving((unsigned short)game_id);
	NetworkConnection::closeBoardDispatch(game_id);
}


/* I'd like to just send the match invite, but only if we're not
 * in the same room... actually, maybe its almost like a room invite */
void TygemConnection::sendMatchRequest(MatchRequest * mr)
{
#ifdef RE_DEBUG
	qDebug("send match offer");
#endif //RE_DEBUG
	if(mr->first_offer)
	{
		sendMatchOffer(*mr, offer);
		mr->first_offer = false;
	}
	else
		sendMatchOffer(*mr, modify);
}

/* FIXME gamedialog maintime supports m:ss, not h:mm:ss,
 * for some reason 90 minutes is getting set as 50 minutes
 * certainly not an hour and a half, but its sent okay.
 * but this is a BIG ISSUE for later, okay 5 hours screws
 * things up even more.*/
unsigned long TygemConnection::getGameDialogFlags(void)
{
	/* I think there is CANADIAN, but haven't seen it or implemented it yet */
	return (/*GDF_CANADIAN | */GDF_BYOYOMI | /*GDF_TVASIA |*/ GDF_FREE_RATED_CHOICE 
		  | GDF_RATED_SIZE_FIXED | GDF_RATED_HANDICAP_FIXED
		  | GDF_RATED_NO_HANDICAP
		  | GDF_HANDICAP1
	          | GDF_NIGIRI_EVEN /*| GDF_ONLY_DISPUTE_TIME*/
		  | GDF_BY_CAN_MAIN_MIN);
}

int TygemConnection::gd_verifyBoardSize(int v)
{
	return 19;
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

QTime TygemConnection::gd_checkMainTime(TimeSystem s, const QTime & t)
{
	//int seconds = (t.minute() * 60) + t.second();
	switch(s)
	{
		default:
		case byoyomi:
			//it looks like there's an unlimited time
			//option, its a dropdown menu...
			if(t.hour() > 8)
				return QTime(16, 40, 0);
			if(t.hour() > 5)
				return QTime(8, 0, 0);
			if(t.hour() > 3)
				return QTime(5, 0, 0);
			if(t.hour() > 2)
				return QTime(3, 0, 0);
			if(t.hour() > 1 && t.minute() > 30)
				return QTime(2, 0, 0);
			if(t.hour() > 1)
				return QTime(1, 30, 0);
			if(t.minute() > 50)
				return QTime(1, 0, 0);
			if(t.minute() > 40)
				return QTime(0, 50, 0);
			if(t.minute() > 30)
				return QTime(0, 40, 0);
			if(t.minute() > 20)
				return QTime(0, 30, 0);
			if(t.minute() > 15)
				return QTime(0, 20, 0);
			if(t.minute() > 10)
				return QTime(0, 15, 0);
			if(t.minute() > 5)
				return QTime(0, 10, 0);
			if(t.minute() > 1)
				return QTime(0, 5, 0);
			if(t.minute() > 0 || t.second() > 0)
				return QTime(0, 1, 0);
			return QTime(0, 0, 0);
			break;
		case canadian:
			//haven't seen yet FIXME
			if(t.hour() > 1 || (t.hour() == 1 && t.minute()))
				return QTime(1, 0, 0);
			else
				return QTime(t.hour(), t.minute(), 0);
			break;
	}
	return t;
}

QTime TygemConnection::gd_checkPeriodTime(TimeSystem s, const QTime & t)
{
	//int seconds = (t.minute() * 60) + t.second();
	switch(s)
	{
		default:
		case byoyomi:
			if(t.minute() || t.second() > 40)
				return QTime(0, 1, 0);
			if(t.second() > 30)
				return QTime(0, 0, 40);
			if(t.second() > 20)
				return QTime(0, 0, 30);
			return QTime(0, 0, 20);
			break;
		case canadian:
			//haven't seen FIXME
			if(t.hour() > 1 || (t.hour() == 1 && t.minute()))
				return QTime(1, 0, 0);
			else if(t.minute() == 0)
				return QTime(0, 1, 0);
			else
				return QTime(t.hour(), t.minute(), 0);
			break;
	}
	return t;
}

unsigned int TygemConnection::gd_checkPeriods(TimeSystem s, unsigned int p)
{
	switch(s)
	{
		default:
		case byoyomi:
			if(p == 0)
				return 1;
			else if(p > 1)
				return 3;
			else
				return 5;
			break;
		case canadian:
			//haven't seen yet FIXME
			if(p < 2)
				return 2;
			else if(p > 50)
				return 50;
			else
				return p;
			break;
	}
	return p;
}

void TygemConnection::declineMatchOffer(const PlayerListing & opponent)
{
	qDebug("Declining match offer");
	unsigned short playing_game_number = match_negotiation_state->getGameId();
	if(!playing_game_number)
		return;
	MatchRequest mr;
	mr.number = playing_game_number;
	mr.opponent = opponent.name;
	BoardDispatch * boarddispatch = getIfBoardDispatch(playing_game_number);
	if(boarddispatch)
	{
		/* Awkward but to prevent issues on board close */
		delete match_negotiation_state;
		match_negotiation_state = 0;
		GameResult g;
		g.result = GameResult::NOGAME;
		boarddispatch->recvResult(&g);
	}
	sendMatchOffer(mr, decline);
}

void TygemConnection::cancelMatchOffer(const PlayerListing & opponent)
{
	qDebug("Canceling match offer");
	declineMatchOffer(opponent);
}

void TygemConnection::acceptMatchOffer(const PlayerListing & opponent, MatchRequest * mr)
{
	qDebug("accept match offer");
	if(!mr->opponent_is_challenger)		//should be our_invitation var name maybe
	{
		getAndCloseGameDialog(opponent);
		sendStartGame(*mr);
	}
	else
		sendMatchOffer(*mr, accept);
}

void TygemConnection::handlePendingData(newline_pipe <unsigned char> * p)
{
	unsigned char * c;
	unsigned char header[4];
	unsigned int packet_size;
	int bytes;
	
	/* We need better connection states, but we'll use the old
	 * for now */
	
	switch(connectionState)
	{
		case INFO:
			if(http_connect_content_length)
			{
				bytes = p->canRead();
				if(bytes == (int)http_connect_content_length)
				{
					c = new unsigned char[bytes + 1];
					p->read(c, bytes);
					handleServerInfo(c, bytes);
					delete[] c;
				}
				break;
			}
			bytes = p->canReadHTTPLine();
			if(bytes)
			{
				c = new unsigned char[bytes + 1];
				p->read(c, bytes);
				int i;
				
				if(strncmp((const char *)c, "HTTP/1.1 200 OK\r\n", 17) != 0)
				{
					qDebug("Server info response not OK!");
					connectionState = PROTOCOL_ERROR;
					//closeConnection();
				}
				else
				{
					i = 0;
					while(i < bytes - 18 && (c[i] != '\r' || c[i + 1] != '\n'))
					{
				 		if(sscanf((const char *)&(c[i]), "Content-Length: %d", &http_connect_content_length) == 1)
							break;
						while(i < bytes - 1 && (c[i] != '\r' || c[i + 1] != '\n'))
							i++;
						i += 2;
					}
				}
				delete[] c;
			}
			break;
		case LOGIN:
			bytes = p->canRead();
			if(bytes)
			{
				c = new unsigned char[bytes];
				p->read(c, bytes);
#ifdef RE_DEBUG
				for(int i = 0; i < bytes; i++)
					printf("%02x", c[i]);
				printf("\n");
#endif //RE_DEBUG
				/* First packet is list of servers and ips */
				//handleServerList(c);
				
				//00080698ff040068		//password incorrect
				if(c[1] == 0x08)
				{
					//00080698ff010065	//maybe because too recent?
								//could also be already
								//logged in
					//00080698ff040013	//error on reconnect...
					//00080698ff060065	//room full
					//0008069801000000	//everything okay?
					if(c[5] == 0x04/*c[7] == 0x68*/ || c[5] == 0x03)  //00080698ff040065 //(03 is Tom)
					{
						if(c[7] == 0x00)
						{
							//this might be okay for eweiqi and tygem
							//on change server, probably not okay for tom
							// Totally wrong FIXME
							qDebug("Trying again...");
							retryLoginTimerID = startTimer(2000);
							delete[] c;
							return;
						}
						//00080698ff040000 tom returns from already logged in
						qDebug("Bad password or login");
						connectionState = PASS_FAILED;
						delete[] c;
						//closeConnection();
						return;
					}
					else if(c[5] == 0x06)
					{
						qDebug("Server full");
						QMessageBox::information(0, tr("Server full"), tr("Server full, try another"));
						if(reconnectToServer() < 0)
						{
							qDebug("User canceled");
							connectionState = CANCELED;
							delete[] c;
							return;
						}
						delete[] c;
						return;
					}
					else if(c[5] == 0x01)	//c5 = 4? 
					{
						if(c[7] == 0x72 || c[7] == 0x00)	//00 is tom
						{
							//for eweiqi we get this on server change...
							//so this seems to work better without
							//connectionState = ALREADY_LOGGED_IN;
							//delete[] c;
							//return;
						}
						//we seem to get 01 65 every time
						//we change servers which makes me
						//think it might be like an already
						//logged in message... then again,
						//if we don't get the encode offset,
						//we can't continue
						//special response
						qDebug("Already logged in? Too recent?");
						fflush(stdout);
						//00080698ff010072
						retryLoginTimerID = startTimer(2000);
						/* FIXME its real ugly if this happens
						 * more than once and we hammer the
						 * server until it lets us in.  We need
						 * to sleep or put out a message or
						 * something. */
						/* There's a chance that 0x38 vs 0x08
						 * isn't the only distinction here in
						 * which case, maybe we ought to
						 * try "handleLogin"? even from 0x08?
						 * requires testing to see what gets
						 * us cut off */
					}
					else
					{
						qDebug("Received %02x %02x from login", c[5], c[7]);
					}
					//00080698ff010000, this is after changing servers on TOM
					//00080698ff010072, changing ervers tygem

				}
				else if(c[1] == 0x38)
					handleLogin(c, bytes);
				
				delete[] c;
			}
			break;
		case SETUP:
		case CONNECTED:
			while((bytes = p->canRead()))
			{
				p->peek(header, 4);
#ifdef RE_DEBUG
#ifdef NOISY_DEBUG
				printf("%02x%02x%02x%02x\n", header[0], header[1], header[2], header[3]);
#endif //NOISY_DEBUG
#endif //RE_DEBUG
				packet_size = header[1] + (header[0] << 8);
				if((unsigned)bytes < packet_size)
					break;
				c = new unsigned char[packet_size + 1];
				p->read(c, packet_size);
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
			qDebug("connection state not handled by Tygem type protocol");
			break;
	}
}

/* Really we should have a legitimate parse function, but I feel
 * like this is the only time we're going to do this.
 * Yeah, FIXME, I've decided we should write a parser just because...
 * handleServerInfo?? not handleServerList?? anyway, we don't need both*/
void TygemConnection::handleServerInfo(unsigned char * msg, unsigned int length)
{
	char * p = (char *)msg;
	int i, j;
	ServerItem * si;

#ifdef RE_DEBUG
	/*for(i = 0; i < length; i++)
		printf("%c", p[i]);
	printf("\n");*/
#endif //RE_DEBUG

	/* We're having the connection handle the http header, nevertheless
	 * we should check something for it */
	/*if(strncmp(p, "HTTP/1.1 200 OK\r\n", 17) != 0)
	{
		qDebug("Server info response not OK!");
		closeConnection();
		return;
	}*/
	while(p < ((char *)msg + length))
	{
		while(p < ((char *)msg + length - 10) && (p[0] != '\r' || p[1] != '\n'))
			p++;
		p += 2;
		//FIXME, check for size
		if(strncmp(p, "/LIVE ", 6) == 0)
		{
			//same as for /SERVER but with only 2 [], name and ip
		}
		else if(strncmp(p, "/LECTURE ", 9) == 0)
		{
			//same as for /SERVER but with only 2 [], name and ip
		}
		else if(strncmp(p, "/SERVER ", 8) == 0 ||
		        strncmp(p, "/_SERVER ", 9) == 0)	//whats the "_" FIXME
		{
			if(p[1] == '_')
				p++;
			p += 8;
			unsigned char ipaddr[16];
			unsigned char name[20];
			i = 0;
			while(i < 7 && p < ((char *)msg + length))
			{
				if(*p != '[')
				{
					qDebug("Server info parse error");
					closeConnection();
					return;
				}
				p++;
				si = new ServerItem();
				unsigned char * dest;
				if(i == 1)
					dest = ipaddr;
				else if(i == 0)
					dest = name;
				j = 0;
				while(*p != ']' && p < ((char *)msg + length))
				{
					if((i == 1 && j < 15) || (i == 0 && j < 19))
						dest[j++] = *p;
					p++;
				}
				if(i == 0 || i == 1)
				{
					dest[j++] = 0x00;
				}
				p += 2;
				if(i == 1)
				{
					strncpy(si->ipaddress, (const char *)ipaddr, 16);
					si->name =  serverCodec->toUnicode((char *)name, strlen((char *)name));
					qDebug("Server: %s", ipaddr);
					serverList.push_back(si);
				}
				i++;
				if(i == 7)
					p--;	//no space
			}
		}
		else if(strncmp(p, "/ENCRYPT ", 9) == 0)
		{
			//FIXME
			//[encode] [on]	
		}
	}
	/* We close here because this first time, its going to close
	* anyway, and if we don't close here, we'll get an error
	* and lose the object */
	connectionState = RECONNECTING;
	closeConnection(false);
	
	if(reconnectToServer() < 0)
	{
		qDebug("User canceled");
		connectionState = CANCELED;
		return;
	}
}

void TygemConnection::sendLogin(bool response_bit, bool change_server)
{	
	unsigned int length = 0x28;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	
	packet[0] = 0x00;
	packet[1] = 0x28;
	packet[2] = 0x06;
	packet[3] = 0xa5;
	for(i = 0; i < (unsigned)username.length(); i++)
		packet[i + 4] = username.toLatin1().data()[i];
	for(i = username.length() + 4; i < 16; i++)
		packet[i] = 0x00;
	for(i = 16; i < 20; i++)
		packet[i] = 0x00;
	packet[19] = response_bit;
	packet[20] = change_server;
	for(i = 21; i < 24; i++)
		packet[i] = 0x00;
	for(i = 0; i < (unsigned)password.length(); i++)
		packet[24 + i] = password.toLatin1().data()[i];
	for(i = 24 + password.length(); i < length; i++)
		packet[i] = 0x00;
	//before encode
#ifdef RE_DEBUG
	printf("Sending login: ");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode_offset = 0;	//clear for new server
	encode(packet, 0x8);
	//after encode
#ifdef RE_DEBUG
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG

	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending login");
	delete[] packet;
}

void TygemConnection::handleLogin(unsigned char * msg, unsigned int length)
{
	if(length != 0x38)
	{
		qDebug("Login of strange size: %d", length);
	}
	unsigned char * p = msg;

	//0038 0698 0100 0200 228b ae64 0000 0000
	//0000 0000 0000 0000 0000 0000 0000 0000
	//0000 0000 0000 0000 7065 7465 7269 7573
	//0000 0002 0001 0000
	/* Note that that 02 is likely our country code!! we should store that
	 * perhaps? (or maybe just get it from player lists*/
	/* One of these is also the zodiac byte, but I don't know which one
	 * here, I'm thinking its probably not that first 1, so I'll use the
	 * second one, I think we get the country_id from the player lists*/
	p += 8;
	encode_offset = *(unsigned long *)p;
	p += 4;
	p += 0x1c;
	// this could be the nickname if we passed the username
	//pretty certain this is the nickname, so that kind of
	//casts some of this into question. FIXME
	//we either have to change the username or... I just...
	//we could check that its not the same
	p += 11;
	//country_id
	p++;
	p++;
	zodiac_byte = p[0];
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < length; i++)
		printf("%02x ", msg[i]);
	printf("\n");
	//printf("encode offset: %08x\n", encode_offset);
#endif //RE_DEBUG
	
	received_games = false;
	received_players = false;
	
	/* What happens if both players get disconnected FIXME */
	/* Also, its ridiculous to have no less than four variables
	 * just to track disconnect and resume */
	initial_connect = true;
	
	sendRequestGames();
	sendRequest();
	connectionState = SETUP;
	sendName();
	serverKeepAliveTimerID = startTimer(119000);	//not fast enough? 2 minutes I thought
							//maybe we're supposed to send something
							//else soon on login too...
	/* FIXME currently we have handleFriends doing setConnected which closes the please wait
	 * getting us out of SETUP.  This means that the players haven't come in yet.
	 * the problem is that handleFriends puts the friends on a list that's then checked when
	 * we receive players.  But there's some other issues with this as well and we should get
	 * CyberORO working better with this, as well as IGS Before we worry about this
	 * relatively minor problem */
	sendFriendsBlocksRequest();
	sendRequestPlayers();
	//53
	//11 ae 14 name(91) 95 53
	onReady();		//sends invite settings as well
}

void TygemConnection::sendRequestGames(void)
{
	unsigned int length = 8;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x08;
	packet[2] = 0x06;
	packet[3] = 0x11;
	packet[4] = 0x00;
	packet[5] = 0x00;
	packet[6] = 0x00;
	packet[7] = 0x00;
	
	//before encode
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	/*encode(packet, 0x8);
	//after encode
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
*/
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending login confirm");
	delete[] packet;
}

//we send
//00 0C 06 AE 5C C8 E9 D4 00 E8 03 00
void TygemConnection::sendRequest(void)
{
	unsigned int length = 12;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x0c;
	packet[2] = 0x06;
	packet[3] = 0xae;
	packet[4] = 0x0d;
	packet[5] = 0xf0;
	packet[6] = 0xad;
	packet[7] = 0xba;
	packet[8] = 0x0d;
	packet[9] = 0xf0;
	packet[10] = 0xad;
	packet[11] = 0xba;
	//a8a88cce
	//before encode
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	encode(packet, 1);		//WOAH FIXME double check this

	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending request");
	delete[] packet;
}

/* Its possible we're supposed to send only ascii or only encode here */
/* Note that this is also a request for some 0692 information */
void TygemConnection::sendName(void)
{
	unsigned int length = 24;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x18;
	packet[2] = 0x06;
	packet[3] = 0x91;
	writeZeroPaddedString((char *)&(packet[4]), getUsername(), 20);
	//before encode
#ifdef RE_DEBUG
	printf("Sending name");
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);

	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending name");
	delete[] packet;
}

void TygemConnection::sendRequestPlayers(void)
{
	unsigned int length = 8;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x08;
	packet[2] = 0x06;
	packet[3] = 0x14;
	packet[4] = 0x00;
	packet[5] = 0x00;
	packet[6] = 0x00;
	packet[7] = 0x00;
	
	//before encode
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	/*encode(packet, 0x8);
	//after encode
	for(i = 0; i < length; i++)
	printf("%02x", packet[i]);
	printf("\n");
	*/
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending players list request");
	delete[] packet;
}

/* Note that as currently set up, this MUST be sent
 * before the player list request since we only set
 * the friends/blocks flags when the player is received
 * and so the friends list must be filled out before
 * we receive any players on it */
void TygemConnection::sendFriendsBlocksRequest(void)
{
	unsigned int length = 8;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x08;
	packet[2] = 0x06;
	packet[3] = 0x95;
	packet[4] = 0x00;
	packet[5] = 0x00;
	packet[6] = 0x00;
	packet[7] = 0x00;
	
	//before encode
#ifdef RE_DEBUG
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	/*encode(packet, 0x8);
	//after encode
	for(i = 0; i < length; i++)
	printf("%02x", packet[i]);
	printf("\n");
	*/
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending friends/blocks list request");
	delete[] packet;
}

void TygemConnection::sendObserversRequest(unsigned short game_number)
{
	unsigned int length = 12;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x0c;
	packet[2] = 0x06;
	packet[3] = 0x65;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	packet[6] = 0x00;
	packet[7] = 0x00;
	packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;
	
#ifdef RE_DEBUG
	printf("observer list request:\n");
	//before encode
	for(unsigned int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);

	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending observers request");
	delete[] packet;
}

void TygemConnection::changeServer(void)
{
	if(connectionState != CONNECTED)
		return;
	if(reconnectToServer() < 0)
	{
		qDebug("User canceled");
		return;
	}
}

void TygemConnection::createRoom(void)
{
#ifdef FIXME
	CreateRoomDialog * createroomdialog = new CreateRoomDialog(this);
	createroomdialog->exec();	
#endif //FIXME
}

/* FIXME we need error handling here, a return value, something
 * or at least we need to call the right destructors throughout,
 * and safely. */
int TygemConnection::reconnectToServer(void)
{
	bool reconnecting;
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
	
	qDebug("Reconnecting to %s: %s...", serverList[server_i]->name.toLatin1().constData(), serverList[server_i]->ipaddress);
	
	if(connectionState == CONNECTED)
	{
		killTimer(serverKeepAliveTimerID);
		sendDisconnectMsg();
		closeConnection(false);
		reconnecting = true;
	}
	else if(connectionState == LOGIN)
	{
		closeConnection(false);
		reconnecting = false;
	}
	else
		reconnecting = false;
	if(openConnection(serverList[server_i]->ipaddress, 12320))
	{
		qDebug("Reconnected %d", reconnecting);
		connectionState = LOGIN;
		sendLogin(false, false);
		//sendLogin(reconnecting, reconnecting);
		fflush(stdout);
		fflush(stderr);
		onAuthenticationNegotiated();
	}
	else
		qDebug("Can't open Connection!!");

	return 0;
}

/* Check comment in igs code, FIXME */
const PlayerListing & TygemConnection::getOurListing(void)
{
	PlayerListing * p;
	p = getDefaultRoom()->getPlayerListing(getUsername());
	return *p;
}

unsigned short TygemConnection::getRoomNumber(void)
{
	return match_negotiation_state->getGameId();
}

//change text to QString
//FIXME why is this commented out?!?
/* I'm not certain why this is commented out,
 * but there's an email type message and then
 * there's a conversation message.  You have to
 * request the conversation message and then
 * there's a message sent when you leave.  I have it
 * in tygem_endgame4.txt on spacecoyote I think FIXME */
 /* I think we can use existing messages in UI but that should be fixed up
  * anyway */
/* Okay, there's a simple reason this is commented out.  There's no UI element
 * to handle it.  We'd need to set something up to send messages and I know there's
 * some facility for that, as in IGS, its like an on-server email service, but we
 * haven't set that feature up in general and that would need to be done first.
 * also note that that 0617 conflicts with the server announcement code as
 * far as I can tell.  See handlePersonalChat with 069f I think.  Good chance
 * this stuff is copy and pasted. FIXME */
void TygemConnection::sendPersonalChat(const PlayerListing & , const char * )
{
	/*unsigned int length = 42 + text.size();
	while(length % 4 != 0)
		length++;
	length += 4;
	unsigned char * packet = new unsigned char[length];
	char * our_name, * msg;
	int i;
	int ordinal;
	unsigned char qualifier;
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x17;
	msg = (char *)text.toLatin1().constData();
	writeZeroPaddedString((char *)&(packet[4]), getUsername(), 14);
	packet[18] = 0x00;
	sscanf(ourPlayer->rank.toLatin1().constData(), "%d%c", &ordinal, &qualifier);
	if(qualifier == 'k')
		packet[19] = 0x12 - ordinal;
	else if(qualifier == 'p')
		packet[19] = ordinal + 0x1a;
	else
		packet[19] = ordinal + 0x11;
	//this would actually be our ascii name, with the earlier
	//being the encoded version
	for(i = 0; i < strlen(our_name); i++)
		packet[i + 20] = our_name[i];
	for(i = strlen(our_name) + 20; i < 30; i++) 	  
		packet[i] = 0x00;
	packet[30] = 0x00;
	packet[31] = 0x02;	//what is this?
	packet[32] = 0x00;
	packet[33] = 0x00;
	packet[34] = 0x00;
	packet[35] = ((strlen(msg) + 1) >> 8);
	packet[36] = (strlen(msg) + 1) & 0x00ff;
	packet[37] = 0x00;
	packet[38] = 0xff;
	packet[39] = 0xff;	//possibly implied destination username?
	packet[40] = 0x33;	//don't know, sometimes 0x32, something to do with encoding?
	for(i = 0; i < strlen(msg); i++)
		packet[41 + i] = msg[i];
	packet[41 + strlen(msg)] = 0x00;
	i = 42 + strlen(msg);
	while(i % 4 != 0)
	{
		packet[i] = 0x00;
		i++;
	}
	packet[i] = 0x00;
	packet[i + 1] = 0x00;
	packet[i + 2] = 0x00;
	packet[i + 3] = 0x00;
	
	printf("personal chat before encode\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
	
	encode(packet, (length / 4) - 2);
	
	printf("personal chat after encode\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending personal chat");
	delete[] packet;*/
}

void TygemConnection::sendOpenConversation(PlayerListing & player)
{
	unsigned int length = 0x4c;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x33;
	
	PlayerListing * ourPlayer;
	ourPlayer = getDefaultRoom()->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	writeNotNicknameAndRank((char *)&(packet[4]), player);
	writeNotNicknameAndRank((char *)&(packet[20]), *ourPlayer);
	writeNicknameAndCID((char *)&(packet[36]), player);
	writeNicknameAndCID((char *)&(packet[48]), *ourPlayer);
	for(i = 60; i < 66; i++)
		packet[i] = 0x00;
	packet[66] = 0x02;
	
	for(i = 67; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending open conversation");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending close conversation");
	delete[] packet;
}

void TygemConnection::sendConversationReply(PlayerListing & player, enum MIVersion version)
{
	if(version != accept && version != decline)
	{
		qDebug("sendConversationReply passed unhandled version");
		return;
	}
	unsigned int length = 0x60;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x34;
	
	PlayerListing * ourPlayer;
	ourPlayer = getDefaultRoom()->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	writeNotNicknameAndRank((char *)&(packet[4]), player);
	writeNotNicknameAndRank((char *)&(packet[20]), *ourPlayer);
	writeNicknameAndCID((char *)&(packet[36]), player);
	writeNicknameAndCID((char *)&(packet[48]), *ourPlayer);
	for(i = 60; i < 66; i++)
		packet[i] = 0x00;
	if(version == accept)
		packet[66] = 0x01;
	else
		packet[66] = 0x00;
	
	for(i = 67; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending open conversation reply");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending open converation reply");
	delete[] packet;
}

void TygemConnection::sendConversationMsg(PlayerListing & player, const char * text)
{
	unsigned int text_length = strlen(text);
	unsigned int length = 0x44 + text_length;
	if(length % 4 == 0)
		length += 4;
	else
	{
		while(length % 4 != 0)
			length++;
	}
	length += 4;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x35;
	
	PlayerListing * ourPlayer;
	ourPlayer = getDefaultRoom()->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	writeNotNicknameAndRank((char *)&(packet[4]), player);
	writeNotNicknameAndRank((char *)&(packet[20]), *ourPlayer);
	writeNicknameAndCID((char *)&(packet[36]), player);
	writeNicknameAndCID((char *)&(packet[48]), *ourPlayer);
	for(i = 60; i < 64; i++)
		packet[i] = 0x00;
	//these four zeroes have to do with text color I think
	//32 cd 32 00		//green
	//ff 00 00 00		//after a background change? also red
	//actually 63 maybe be the high byte of the text size
	//assuming text can be more than 255? double check FIXME
	packet[64] = text_length;
	packet[65] = 0x00;
	packet[66] = 0x05;
	packet[67] = 0x00;
	for(i = 0; i < text_length; i++)
		packet[68 + i] = text[i];
	packet[68 + text_length] = 0x00;
	
	for(i = 69 + text_length; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending conversation msg");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending conversation msg");
	delete[] packet;
}

void TygemConnection::sendCloseConversation(PlayerListing & player)
{
	QString closeMsg = '\'' + getUsername() + '\'';
	//FIXME pretty sure close Msg says closeMsg in korean, only
	//our hacked client sends a bunch of ????????????
	const char * text = closeMsg.toLatin1().constData();
	unsigned int text_length = strlen(text);
	unsigned int length = 0x44 + text_length;
	while(length % 4 != 0)
		length++;
	length += 4;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x35;
	
	PlayerListing * ourPlayer;
	ourPlayer = getDefaultRoom()->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	writeNotNicknameAndRank((char *)&(packet[4]), player);
	writeNotNicknameAndRank((char *)&(packet[20]), *ourPlayer);
	writeNicknameAndCID((char *)&(packet[36]), player);
	writeNicknameAndCID((char *)&(packet[48]), *ourPlayer);
	for(i = 60; i < 64; i++)
		packet[i] = 0x00;
	//these four zeroes have to do with text color I think
	//32 cd 32 00		//green
	//ff 00 00 00		//after a background change? also red
	//actually 63 maybe be the high byte of the text size
	//assuming text can be more than 255? double check FIXME
	packet[64] = text_length;
	packet[65] = 0x00;
	packet[66] = 0x04;		//this would be the close bit, rather than 5
	packet[67] = 0x00;
	for(i = 0; i < text_length; i++)
		packet[68 + i] = text[i];
	packet[68 + text_length] = 0x00;
	
	for(i = 69 + text_length; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending close conversation");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending close conversation");
	delete[] packet;
}

/* FIXME I think both IGS and maybe even oro have a chat echo.
 * but with tygem its particularly ugly */
 /* This is in game, name should be changed */
void TygemConnection::sendMsg(unsigned int room_number, QString text)
{
	if(text.size() > 0x7e)
	{
		qDebug("send Message too large according to tygem standard");
		return;
	}
	unsigned int length = 74 + text.size();
	if(length % 4 == 0)
		length += 4;
	else
	{
		while(length % 4 != 0)
			length++;
	}
	length += 4;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x61;
	packet[4] = (room_number >> 8);
	packet[5] = room_number & 0x00ff;
	packet[6] = 0x00;		
	packet[7] = 0x01;	//our player?
	for(i = 8; i < 24; i++)
		packet[i] = 0x00;
	writeNotNicknameAndRank((char *)&(packet[24]), *ourPlayer);
	writeNicknameAndCID((char *)&(packet[40]), *ourPlayer);
	for(i = 53; i < 68; i++)
		packet[i] = 0x00;
	packet[68] = (text.size() + 1) & 0x00ff;
	packet[69] = 0x00; //size cannot apparently be larger than 0x7f
	packet[70] = 0x00;
	packet[71] = 0x00;
	packet[72] = 0x33;	//don't know, sometimes 0x32, something to do with encoding?
	for(i = 0; i < (unsigned)text.size(); i++)
		packet[73 + i] = text.toLatin1().constData()[i];
	packet[73 + text.size()] = 0x20;		//append space, don't know why
	
	for(i = 74 + text.size(); i < length; i++)
		packet[i] = 0x00;
	
	
#ifdef RE_DEBUG
	printf("room chat before encode\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	printf("room chat after encode\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending room chat");
	delete[] packet;
}

void TygemConnection::sendServerChat(QString text)
{
	unsigned int length = 42 + text.size();
	while(length % 4 != 0)
		length++;
	length += 4;
	unsigned char * packet = new unsigned char[length];
	char * msg;
	unsigned int i;
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x17;
	msg = (char *)text.toLatin1().constData();
	/*our_name = (char *)getUsername().toLatin1().constData();
	for(i = 0; i < strlen(our_name); i++)
		packet[i + 4] = our_name[i];
	for(i = strlen(our_name) + 4; i < 18; i++) 	  
		packet[i] = 0x00;*/
	writeNotNicknameAndRank((char *)&(packet[4]), *ourPlayer);
	/*writeZeroPaddedString((char *)&(packet[4]), getUsername(), 14);
	packet[18] = 0x00;
	sscanf(ourPlayer->rank.toLatin1().constData(), "%d%c", &ordinal, &qualifier);
	if(qualifier == 'k')
		packet[19] = 0x12 - ordinal;
	else if(qualifier == 'p')
		packet[19] = ordinal + 0x1a;
	else
		packet[19] = ordinal + 0x11;*/
	//this would actually be our nick name FIXME
	/*for(i = 0; i < strlen(our_name); i++)
		packet[i + 20] = our_name[i];
	for(i = strlen(our_name) + 20; i < 30; i++) 	  
		packet[i] = 0x00;*/
	writeNicknameAndCID((char *)&(packet[20]), *ourPlayer);
	/*writeZeroPaddedString((char *)&(packet[20]), getUsername(), 10);
	packet[30] = 0x00;
	packet[31] = ourPlayer->country_id;*/
	packet[32] = 0x00;
	packet[33] = 0x00;
	packet[34] = 0x00;
	packet[35] = ((strlen(msg) + 1) >> 8);
	packet[36] = (strlen(msg) + 1) & 0x00ff;
	packet[37] = 0x00;
	packet[38] = 0xff;
	packet[39] = 0xff;	//possibly implied destination username?
	packet[40] = 0x33;	//don't know, sometimes 0x32, something to do with encoding?
	for(i = 0; i < strlen(msg); i++)
		packet[41 + i] = msg[i];
	packet[41 + strlen(msg)] = 0x00;
	i = 42 + strlen(msg);
	while(i % 4 != 0)
	{
		packet[i] = 0x00;
		i++;
	}
	packet[i] = 0x00;
	packet[i + 1] = 0x00;
	packet[i + 2] = 0x00;
	packet[i + 3] = 0x00;
	
	printf("Server chat before encode\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
	
	encode(packet, (length / 4) - 2);
	
	printf("Server chat after encode\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending server chat");
	delete[] packet;
}

//FIXME fix this when we do create rooms
void TygemConnection::sendJoinRoom(const RoomListing & /*room*/, const char * /*password*/)
{
	//FIXME
}

void TygemConnection::addFriend(PlayerListing & player)
{
	unsigned int length = 0x18;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x93;
	writeZeroPaddedString((char *)&(packet[4]), player.notnickname, 14);
	//there's some bytes here that the official client doesn't even zero I don't
	//think
	packet[19] = 0x00;		//this is 0x00 if they are friend
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending add friend");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending add friend");
	delete[] packet;
	
	NetworkConnection::addFriend(player);
}

void TygemConnection::removeFriend(PlayerListing & player)
{
	unsigned int length = 0x18;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x94;
	writeZeroPaddedString((char *)&(packet[4]), player.notnickname, 14);
	//there's some bytes here that the official client doesn't even zero I don't
	//think
	packet[19] = 0x01;		//this is weird because 93 to 94 distinguishes removal
					//why not 0 here for friend as with addFriend?!??	
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending remove friend");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending remove friend");
	delete[] packet;
	
	NetworkConnection::removeFriend(player);
}

/* This could be fan instead of block, need to double check, likely block though */
void TygemConnection::addBlock(PlayerListing & player)
{
	unsigned int length = 0x18;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x93;
	writeZeroPaddedString((char *)&(packet[4]), player.notnickname, 14);

	//there's some bytes here that the official client doesn't even zero I don't
	//think
	packet[19] = 0xff;		//this is 0x00 if they are friend
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending add block");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending add block");
	delete[] packet;
	
	NetworkConnection::addBlock(player);
}

void TygemConnection::removeBlock(PlayerListing & player)
{
	unsigned int length = 0x18;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x94;
	writeZeroPaddedString((char *)&(packet[4]), player.notnickname, 14);

	//there's some bytes here that the official client doesn't even zero I don't
	//think
	packet[19] = 0xff;		//this is 0x00 if they are friend
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending remove block");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending remove block");
	delete[] packet;
	
	NetworkConnection::removeBlock(player);
}

/* Not sure what to do with this yet */
void TygemConnection::requestLongInfo(PlayerListing & player)
{
	unsigned int length = 0x18;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0x91;
	writeZeroPaddedString((char *)&(packet[4]), player.notnickname, 14);

	//there's some bytes here that the official client doesn't even zero I don't
	//think
	packet[19] = 0xff;		//this is 0x00 if they are friend
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending request long info");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending request long info");
	delete[] packet;
}

/* Not sure what to do with this yet: this may not even be
 * an info thing, its like a list of options, I don't know
 * why it sends anything but it might request some small
 * info */
void TygemConnection::requestShortInfo(PlayerListing & player)
{
	unsigned int length = 0x18;
	char * packet = new char[length];
	unsigned int i;

	packet[0] = (length >> 8);
	packet[1] = (length & 0xff);
	packet[2] = 0x06;
	packet[3] = 0xa3;
	writeZeroPaddedString((char *)&(packet[4]), player.notnickname, 14);

	//there's some bytes here that the official client doesn't even zero I don't
	//think
	packet[19] = 0xff;		//this is 0x00 if they are friend
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	qDebug("Sending request short info");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending request short info");
	delete[] packet;
}


/* This looks like its also for leaving games were playing */
void TygemConnection::sendFinishObserving(unsigned short game_number)
{
	unsigned int length = 12;
	char *packet = new char[length];

	/* We need to turn this to a join if its a room, not a game
	* otherwise bad things happen FIXME */
	
	packet[0] = 0x00;
	packet[1] = 0x0c;
	packet[2] = 0x06;
	packet[3] = 0x3d;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	/* I'm pretty certain that 6 and 7 are just garbage
	 * remember that bytes aren't zeroed */
	packet[6] = 0xad;	
	packet[7] = 0xba;
	packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;
#ifdef RE_DEBUG
	printf("Sending finishing observe to %d ", game_number);
	for(unsigned int i = 0; i < length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);

	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending finish observing");
	delete[] packet;
}

/* I get the feeling that this requests the room,
 * maybe we can specify password or name if we were creating
 * it from button rather than auto on match accept.  And
 * then maybe a number comes back 
 * Incidentally, if we break on this, take too long, we can get
 * a bad answer.  Or maybe our invite response is bad and there's
 * decent checking on it now.*/
void TygemConnection::sendCreateRoom(void)
{
	unsigned int length = 40;
	char *packet = new char[length];
	unsigned int i;
	
	packet[0] = 0x00;
	packet[1] = 0x28;
	packet[2] = 0x06;
	packet[3] = 0x36;
	for(i = 4; i < length; i++)
		packet[i] = 0x00;
#ifdef FIXME
	//*** create room first option
	for(i = 4; i < 26; i++)
		packet[i] = 0x00;
	
	packet[26] = 0xff;
	packet[27] = 0xff;
	packet[28] = 0xff;
	packet[29] = 0xff;
	for(i = 30; i < length; i++)
		packet[i] = 0x00;
	//followed by a 0665 sendObserverRequest  ?? weird

	//*** create room second option with text "hello"
	packet[4] = 0x02;
	packet[5] = 0x00;
	packet[6] = 0x00;
	packet[7] = 0x00;
	for(i = 8; i < 26; i++)
		packet[i] = 0x00;
	packet[26] = 0xff;
	packet[27] = 0xff;
	packet[28] = 0xff;
	packet[29] = 0xff;
	packet[30] = 0x02;
	packet[31] = 0x00;
	packet[32] = 0x00;
	packet[33] = 0x00;
	
	packet[34] = (text.size() >> 8);
	packet[35] = text.size & 0xff;
	packet[36] = //null terminated text string
	//6 more zeroes

	//*** create room third option with 2 checkboxes below checked, same checkboxes on second option
	//there was a dropdown to select a room as I recall
	//4 and 5 are likely a room number to create a review room off of
	//6 and 7 are 0
	//up to 26 is 0
	//26 is 0, 27 is 1, 28 is 1, 29 is 0
	//then there's the 4 and 5 room number again, what I think is a room number
	//rest is 0 to 2c/40
	
	//then there's weird packets
	//one I missed because I thought it was an 0665
	//after is 0018063b 00 01 00 01 with the rest 0s
	//then 00140683 stop time which I won't bother to copy right now
	//then the 0665 sendObserversRequest
	//I'm not going to screw with this one right now, but if I did, I'd have to get that one I missed.
#endif //FIXME
#ifdef RE_DEBUG
	printf("Sending create room");
	for(i = 0; i < length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode((unsigned char *)packet, (length / 4) - 2);
#ifdef RE_DEBUG
	printf("After encode\n");
	for(i = 0; i < length; i++)
		printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
	
	qDebug("Sending create room");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending create room");
	delete[] packet;
	match_negotiation_state->sendCreateRoom();
}

//FIXME
void TygemConnection::sendCreateRoom(RoomCreate * /*room*/)
{
	qDebug("create room unimplemented on tygem");
	return;
}

/* Tygem has a different system.  We send a basic invite and if
 * its accepted we enter a board room where there is then the debate
 * on match specifics. */
void TygemConnection::sendMatchInvite(const PlayerListing & player)
{
	if(getBoardDispatches() == 3)
		QMessageBox::information(0, tr("3 Boards Open"), tr("You must close a board before you can start a game"));
	else if(player.info == QString('X'))
	{
		QMessageBox mb(tr("Not open"), tr("%1 is not accepting invitations").arg(player.name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
			       QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	}
	else
		sendMatchInvite(player, offer);
}

/* note that this only enters the match offer dialog
 * it doesn't accept the game.  Haven't decided whether
 * to auto send this or to have a special popup for it.*/
void TygemConnection::sendMatchInvite(const PlayerListing & player, enum MIVersion version, unsigned short room_number)
{
	unsigned int length = 0x44;
	unsigned char * packet = new unsigned char[length];
	int i;
	if(version == offer)
		opponent_is_challenger = false;
	//else we actually might want to set it here instead of in handleMatchInvite
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	if(version == accept || version == decline || version == decline_all || version == alreadyingame)
		packet[3] = 0x44;
	else	//offer or create
		packet[3] = 0x43;
	/* Below is similar to sendServerChat, there should be some
	 * object for creating these records automatically, like addPlayerToHeader
	 * FIXME */
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	/* Careful, they could have logged off in the mean time possibly? */
	writeNotNicknameAndRank((char *)&(packet[4]), player);

	if(version == create)
	{
		/* This is after the room has been created */
		/* really erase rank, double double check?? FIXME */
		for(i = 19; i < 32; i++)
			packet[i] = 0x00;
	}
	else
	{
		writeNicknameAndCID((char *)&(packet[20]), player);
	}
	writeNotNicknameAndRank((char *)&(packet[32]), *ourPlayer);
	writeNicknameAndCID((char *)&(packet[48]), *ourPlayer);
		
	if(version == accept)
	{
		packet[60] = 0x01;
		packet[61] = 0x01;
		//packet[61] = FOURTH_BYTE(mi_flags);
		packet[62] = 0x00;
		packet[63] = 0x00;
		//below three had no effect
		//packet[61] = 0x13;	//??
		//packet[62] = 0xff;
		//packet[63] = 0xff;
		match_negotiation_state->sendMatchAccept((PlayerListing *)&player);
	}
	else if(version == offer || version == decline)
	{
		packet[60] = 0x01;
		if(getBoardDispatches() == 3)	//implied: version = decline
			packet[61] = 0x0d;
		else
			packet[61] = 0x00;
		packet[62] = 0xff;
		packet[63] = 0xff;	//not implied username... see server chat comment
		if(version == offer)
			match_negotiation_state->sendMatchInvite((PlayerListing *)&player);
	}
	else if(version == alreadyingame)
	{
		packet[60] = 0x01;
		packet[61] = 0x0b;		//doublecheck FIXME
		packet[62] = 0xff;
		packet[63] = 0xff;	
	}
	else if(version == decline_all)
	{
		packet[60] = 0x01;
		//packet[61] = 0x0c;		//doublecheck FIXME
		packet[61] = 0x0e;
		packet[62] = 0xff;
		packet[63] = 0xff;
	}
	else if(version == create)
	{
		packet[60] = 0x02;
		packet[61] = 0x00;
		/* Mr Wang to Niu Yen:
		 * I know, let's randomly change the byte order just on this one 
		 * piece of information, just to screw up people trying to reverse
		 * the protocol: */
		packet[62] = room_number & 0x00ff;
		packet[63] = (room_number >> 8);
		/* No no, they're probably just inconsistent with their ntohs calls... */
		match_negotiation_state->createdRoom();
	}
	for(i = 64; i < 68; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	printf("match %s packet before encoding: ", (version == offer ? "offer" :
						     (version == accept ? "accept" :
						     (version == decline || version == decline_all 
							|| version == alreadyingame ? "decline" : "create"))));
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	qDebug("Sending accept match request or invite");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending accept match request or invite");
	delete[] packet;
}

/* This looks like its a declaration of the match for the game listings */
void TygemConnection::sendMatchMsg1(const PlayerListing & player, unsigned short game_number)
{
	unsigned int length = 0x2c;
	unsigned char * packet = new unsigned char[length];
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0xc6;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	packet[6] = 0x00;
	packet[7] = 0x00;
	
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	writeNotNicknameAndRank((char *)&(packet[8]), *ourPlayer);
	writeNotNicknameAndRank((char *)&(packet[24]), player);
	packet[40] = 0x00;
	packet[41] = 0x00;
	packet[42] = 0x00;
	packet[43] = 0x00;
	
#ifdef RE_DEBUG
	printf("Match msg 1: \n");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	qDebug("Sending match msg 1");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending match msg 1");
	delete[] packet;	
}

void TygemConnection::sendMatchOffer(const MatchRequest & mr, enum MIVersion version)
{
	unsigned int length = 0x5c;
	unsigned char * packet = new unsigned char[length];
	int i;
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	PlayerListing * opponent = room->getPlayerListing(mr.opponent);
	if(!opponent)
	{
		qDebug("Can't get opponent listing for match offer\n");
		return;
	}
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	if(version == offer)
		packet[3] = 0x45;
	else //if(version == accept || version == modify)
		packet[3] = 0x46;
	
	writeNotNicknameAndRank((char *)&(packet[4]), *opponent);
	writeNotNicknameAndRank((char *)&(packet[20]), *ourPlayer);
	
	packet[36] = (mr.number >> 8);
	packet[37] = mr.number & 0x00ff;
	//012c 2801 0100 0100 0002	//offer?
	//012c 2801 0100 0100 0001	//accept?
	//0bb8 2803 0100 0100 0002	//offer
	//time settings
	//0007 012c 1e03 0100 0100
	if(version == decline)
	{
		for(i = 38; i < 47; i++)
			packet[i] = 0x00;
	}
	else
	{
		/*I've seen an accept sent with these as 0s
		 * and yet that literally sets time to 0 so... */
		//if(version == offer)
		//{
			packet[38] = (mr.maintime >> 8);
			packet[39] = mr.maintime & 0x00ff;
			packet[40] = mr.periodtime;
			packet[41] = mr.stones_periods;
			packet[42] = mr.handicap;	//this is handicap bit!!!
			/* FIXME  it sets the handicap but doesn't know
			 * whos turn it is or something !?!?! */
			qDebug("Handicap %d\n", mr.handicap);
			packet[43] = (unsigned char)(mr.komi - 0.5);
		/*}
		else if(version == accept)
		{
			for(i = 38; i < 44; i++)
				packet[i] = 0x00;
		}*/
		switch(mr.color_request)
		{
			case MatchRequest::WHITE:
				if(version == offer)
					packet[44] = 0x01;
				else
					packet[44] = 0x00;
				break;
			case MatchRequest::BLACK:
				if(version == offer)     //their color when we offer?
					packet[44] = 0x00;
				else
					packet[44] = 0x01;//our color when we accept
				break;
			case MatchRequest::NIGIRI:
				packet[44] = 0x02;
				break;	
		
		}
		//printf("*** match offer packet 44 should be 1: %02x\n", packet[44]);
		//FIXME
		/* This bit, if we're offering is basically ignored since
		 * sent 0671 sendStartGame overrides colors, etc. */
		//if(version == accept)
		//	packet[44] = 0x00;	//definitely we're black in tests right now
		/* Changing the accept above from 01 to 00 fixed the names but broke our move
		 * ability as white */
		/* On the other hand, changing the offer from 01 to 00 screwed the names
		 * but allows us to play white */
		/* Trying both as 01 with 01 below instead of 02 after our name */
		/* That last 02 to 01 broke our move ability, so thinking, that's
		 * offerer's color? */
		//02 seemed to do something strange
					//maybe swapped the pictures but not the names
		/* For 44, 0x00 means that, for this as an accept send,
		 * the challenger, that first name, is black, and we're white
		 * 0x01 probably means the opposite, 02 might be nigiri or something
		 * but there's flags later, that also need to be set I think */
		/* Actually 02 instantly makes challenger black and allow them to play...
		 * but then looks like inconsistent, so yeah, nigiri */
		//00 here is, as accept send, we play white, they play black,
		//according to bits below and name orders as they are, this seems
		// to work except we can't play
		/* Okay.  These bits, 45 and 46 determine bits 47 and 49 in
		 * 0671 sendStartGame, whose name should be changed to sendMatchOpen 
		 * the also seem to be carried through from here in 0645 to 0646 the
		 * accept, to 0671 to open the game */
		if(mr.free_rated == FREE)
		{
			/* FIXME doublecheck */
			packet[45] = 0x01;
			packet[46] = 0x01;
		}
		else	//RATED
		{
			packet[45] = 0x00;
			packet[46] = 0x00;
		}
		//on tom sending accept I've seen 45 and 46 as 10 00
		//might even be critical
		
		/* These two bytes are likely about non rated games... or they're
		 * the ones we're looking for */
		//packet[46] = SECOND_BYTE(mo_flags);
	}
	if(version == offer)
	{
		packet[47] = 0x02;	//offer bit
		packet[48] = 0x00;
		packet[49] = 0x00;
		packet[50] = 0x00;
		packet[51] = 0x00;
		match_negotiation_state->offerMatchTerms((MatchRequest *)&mr);
	}
	else if(version == accept)
	{
		packet[47] = 0x01;	//accept bit
		/* Could also be challenger bit, but if we change 47 to 02
		 * it definitely rechallenges or disputes or something */
		packet[48] = 0xff;
		packet[49] = 0xff;
		packet[50] = 0xff;
		packet[51] = 0xff;
		match_negotiation_state->acceptMatchTerms((MatchRequest *)&mr);
	}
	else if(version == modify)
	{
		packet[47] = 0x02;	//accept bit
		packet[48] = 0xff;
		packet[49] = 0xff;
		packet[50] = 0xff;
		packet[51] = 0xff;
		match_negotiation_state->modifyMatchTerms((MatchRequest *)&mr);
	}
	else if(version == decline)
	{
		packet[47] = 0x03;
		packet[48] = 0xff;
		packet[49] = 0xff;
		packet[50] = 0xff;
		packet[51] = 0xff;
		delete match_negotiation_state;
		match_negotiation_state = 0;
	}
	
	if(version == offer)
	{
		/* Apparently we don't have opponent's nickname before
		 * they've responded to the offer ? */
		for(i = 52; i < 76; i++)
			packet[i] = 0x00;
	}
	else //if(version == accept || version == modify || version == decline)
	{
		for(i = 52; i < 56; i++)
			packet[i] = 0x00;
		packet[56] = 0x00;
		packet[57] = zodiac_byte;
		packet[58] = 0x00;
		packet[59] = 0x00;	//their pic?
		for(i = 60; i < 64; i++)
			packet[i] = 0x00;
		
		writeNicknameAndCID((char *)&(packet[64]), *opponent);
	}
	
	writeNicknameAndCID((char *)&(packet[76]), *ourPlayer);
	
	for(i = 88; i < 92; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	printf("match packet we are sending now: ");
	for(i = 0; i < (int)length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	encode(packet, (length / 4) - 2);
	qDebug("Sending match offer");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending match offer");
	delete[] packet;
	return;
}

/* Somewhat inconsistent name... */
void TygemConnection::sendStartGame(const MatchRequest & mr)
{
	unsigned int length = 0x60;
	unsigned char * packet = new unsigned char[length];
	int i;
	Room * room = getDefaultRoom();
	PlayerListing * ourPlayer;
	PlayerListing * opponent = room->getPlayerListing(mr.opponent);
	if(!opponent)
	{
		qDebug("Can't get opponent listing for start game\n");
		return;
	}
	ourPlayer = room->getPlayerListing(getUsername());
	if(!ourPlayer)
	{
		qDebug("Can't get our player listing");
		return;
	}
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x71;
	packet[4] = (mr.number >> 8);
	packet[5] = mr.number & 0x00ff;
	packet[6] = 0x00;
	packet[7] = 0x01;	//probably color
	
	writeNotNicknameAndRank((char *)&(packet[8]), *opponent);
	writeNotNicknameAndRank((char *)&(packet[24]), *ourPlayer);
	
	//012c 2801 0100 0100 0002
	//time settings
	packet[40] = (mr.maintime >> 8);
	packet[41] = mr.maintime & 0x00ff;
	packet[42] = mr.periodtime;
	packet[43] = mr.stones_periods;
	packet[44] = mr.handicap;
#ifdef OLD
	/* Just make sure this works okay */
	//for now since time isn't really set right
	packet[40] = 0x01;	//04	//time setting type?
	packet[41] = 0x2c;	//b0	//minutes?
	packet[42] = 0x28;	//1e	//seconds?
	packet[43] = 0x01;	//03	//periods?
	//packet[44] = 0x01;
#endif //OLD
	
	packet[45] = 0x00;
	//this is our color
	switch(mr.color_request)
	{
		case MatchRequest::WHITE:
			packet[46] = 0x00;
			break;
		case MatchRequest::BLACK:
			packet[46] = 0x01;
			break;
		case MatchRequest::NIGIRI:
			packet[46] = 0x02;
			break;	
		
	}
	
	packet[47] = 0x01;		//was 0
	packet[48] = 0x01;
	packet[49] = 0x01;		//was 0
	packet[50] = 0x00;
	packet[51] = 0x12;
	
	packet[52] = 0xff;
	packet[53] = 0xff;
	packet[54] = 0xff;
	packet[55] = 0xff;
	
	packet[56] = 0xff;
	packet[57] = 0xff;
	packet[58] = 0xff;
	packet[59] = 0xff;
	
	packet[60] = 0x00;
	/* This is likely their zodiac_byte but I think its ignored */
	packet[61] = 0x01;		//pics?
	packet[62] = 0x00;
	packet[63] = zodiac_byte;
#ifdef FIXME
	//proper order here seems to be their name then our name
	//careful might change if we play black
	writeZeroPaddedString((char *)&(packet[64]), opponent->notnickname, 10);
	packet[74] = 0x00;
	/* The 0x02 bytes (75 and 87) are most likely the country and these second
	 * names are refered to by tygem as "WHITENICK" as opposed to
	 * "WHITENAME" which is the first name.  So as far as I know,
	 * nothing to do with ascii, versus encoding which makes sense */
	//packet[75] = 0x02;      //changed from 2 in the hope of making opp black
	packet[75] = opponent->country_id;
	writeZeroPaddedString((char *)&(packet[76]), getUsername(), 10);
	packet[86] = 0x00;
	packet[87] = ourPlayer->country_id;
#endif //FIXME
	/* This could be a problem, typically, the below is right, but I have "notnickname"
	 * used above.  Often it doesn't matter, but I'm assuming this packet conforms
	 * to the general format and maybe it doesn't. Doublecheck FIXME */
	writeNicknameAndCID((char *)&(packet[64]), *opponent);
	writeNicknameAndCID((char *)&(packet[76]), *ourPlayer);
		
	for(i = 88; i < 96; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	printf("start match we are sending now: ");
	for(i = 0; i < (int)length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	qDebug("Sending start match");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending start match");
	delete[] packet;
}

void TygemConnection::sendServerKeepAlive(void)
{
	unsigned int length = 8;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x08;
	packet[2] = 0x06;
	packet[3] = 0x4e;
	packet[4] = 0x00;
	packet[5] = 0x00;
	packet[6] = 0x00;
	packet[7] = 0x00;

	qDebug("Sending server keep alive");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending server keep alive");
	delete[] packet;
}

void TygemConnection::timerEvent(QTimerEvent * event)
{
	if(event->timerId() == serverKeepAliveTimerID)
		sendServerKeepAlive();
	else if(event->timerId() == retryLoginTimerID)
	{
		//response bit seems necessary here for tygem
		//at least, double check for tom
		//still screwy FIXME
		sendLogin(true); //was true (nothing seems to work for tom)
		killTimer(retryLoginTimerID);
		retryLoginTimerID = 0;
	}
	else if(event->timerId() == opponentDisconnectTimerID)
	{
		unsigned short playing_game_number = match_negotiation_state->getGameId();
		if(!playing_game_number)
			return;
		sendOpponentDisconnectTimer(playing_game_number);
	}
}

/* FIXME This may be sent even if clock is stopped (with stopped time)*/
void TygemConnection::sendTime(BoardDispatch * boarddispatch)
{
	unsigned int length = 0x14;
	unsigned char * packet = new unsigned char[length];
	GameData * gd = boarddispatch->getGameData();
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x7d;
	packet[4] = (gd->number >> 8);
	packet[5] = gd->number & 0x00ff;
	//we're black sending this
	//            f   p s  m
	//0101 013b 0440 0100 0580 0000 0000
	packet[6] = (gd->black_name == getUsername()) ^ gd->white_first_flag;
	//packet[6] = (gd->black_name == getUsername());
	packet[7] = 0x01;
	
	fillTimeChunk(boarddispatch, &(packet[8]));
	
	packet[16] = 0x00;
	packet[17] = 0x00;
	packet[18] = 0x00;
	packet[19] = 0x00;
#ifdef RE_DEBUG
	printf("TIME SENT: ");
	for(int i = 0; i < 20; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
	
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending time");
	delete[] packet;
	return;
}

void TygemConnection::fillTimeChunk(BoardDispatch * boarddispatch, unsigned char * packet)
{
	GameData * game = boarddispatch->getGameData();
	TimeRecord us = boarddispatch->getOurTimeRecord();
	TimeRecord them = boarddispatch->getTheirTimeRecord();	
	
	if(us.stones_periods == -1)
		packet[0] = game->stones_periods;
	else
		packet[0] = us.stones_periods;
	packet[1] = us.time % 60;
	packet[2] = (us.time - packet[1]) / 60;
	//0x40 and 0x80 are also used for something as
	//well as a time loss flag as well
	if(game->black_name == getUsername())
		packet[3] = 0x40;
	else
		packet[3] = 0x80;
	if(us.stones_periods != -1)
		packet[3] |= 0x20
				;
	if(them.stones_periods == -1)
		packet[4] = game->stones_periods;
	else
		packet[4] = them.stones_periods;
	packet[5] = them.time % 60;
	packet[6] = (them.time - packet[5]) / 60;
	if(game->black_name == getUsername())
		packet[7] = 0x80;
	else
		packet[7] = 0x40;
	if(them.stones_periods != -1)
		packet[7] |= 0x20;
}

//their first move as black
//0 STO 0 2 1 15 15
//0 SUR 0 3 1 //their surrender after our bad 2nd move
void TygemConnection::sendMove(unsigned int game_id, MoveRecord * move)
{
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_id);
	if(!boarddispatch)
	{
		qDebug("Can't get board for send move: %d", game_id);
	}
	GameData * r = boarddispatch->getGameData();
	
	char move_str[32];
	int player_number;
	unsigned int i;
	static bool previous_move_pass = false;
	/*if(move->number + 2 > move_message_number)
		move_message_number = move->number + 2;
	else*/
	move_message_number++;
	//0001	//color
	//0000 0000 0000 ADBA	//probably like 8 zeroes
	//text string ending in 200a and 0 padded out to the full 32
	//like:
	//SUR 0 2 2		white surrenders
	//STO 0 move_number player_number x y 0a
	//SUR 0 move_number player_number 0a
	//REM 0 312 -1 -1 0 1 
	//REM 0 307 5 6 1 0 
	//REM 0 306 10 14 2 0 			un remove?
	//REM 0 number x y player_number 0
	//DSC 1 0 1			//possibly disagreement?
	//DSC 1 0 2 
	
	//BAC 0 246 1 	//part of review
	//CSP 0 233 0
	//CST 0 234 
	//FOR 0 186 1
	/* Might also just be stone color here without the flag */
	//player_number = ((r->black_name == getUsername()) ^ r->white_first_flag) + 1;
	//1 is likely black, 2, white
	player_number = (r->black_name != getUsername()) + 1;
	//player_number = SECOND_BYTE(send_flags);
	switch(move->flags)
	{
		case MoveRecord::RESIGN:
			//if this is resign, we also have to 
			//send the 0670 or 0672 or whatever if its our offer/game
			sprintf(move_str, "SUR %d %d %d \n", 0, move_message_number, player_number); 
			break;
		case MoveRecord::DONE_SCORING:
			/* Looks like this is a little different at the end... 
			 * doesn't seem to be a player number*/
			sprintf(move_str, "REM %d %d -1 -1 %d %d \n", 0, move_message_number, 0, 1 );
			break;
		case MoveRecord::REMOVE:
			//move number continues here ! FIXME
			/* I sent a move_message_number here of 33763 by accident and
			 * it crashed the official eWeiQi client.  There's no error checking
			 * there, its potentially an exploit even, not that our client
			 * doesn't have those as well but... our client's not done yet... */
			sprintf(move_str, "REM %d %d %d %d %d %d \n", 0, move_message_number, move->x - 1, move->y - 1, player_number, 0 );
			break;
		case MoveRecord::UNREMOVE:
			//we can only unremove all stones!, dead stone clear
			sprintf(move_str, "DSC %d %d %d\n", 1, 0, player_number);
			break;
		case MoveRecord::PASS:
			//we might always send a 0669 as well before the SKI
			//0669 might be the real pass and SKI just the board skip
			sendEndgameMsg(r, pass);
			sprintf(move_str, "SKI %d %d \n", 0, move_message_number);	//is that space newline okay? FIXME
			break;
		case MoveRecord::REQUESTUNDO:
			//shouldn't be sent during rated games I don't think
			sendEndgameMsg(r, request_undo);
			return;
			break;
		case MoveRecord::REFUSEUNDO:
			sendEndgameMsg(r, refuse_undo_request);
			return;
			break;
		case MoveRecord::UNDO:
			sendEndgameMsg(r, accept_undo_request);
			int undo_player_number;
			// double check FIXME
			if((boarddispatch->getBlackTurn() && r->black_name == getUsername()) ||
			    (!boarddispatch->getBlackTurn() && r->white_name == getUsername()))
				undo_player_number = (player_number == 1 ? 2 : 1);
			else
				undo_player_number = player_number;
			sprintf(move_str, "WIT %d %d %d\n", 0, move_message_number, undo_player_number);
			break;
		case MoveRecord::NONE:
		default:
			sprintf(move_str, "STO %d %d %d %d %d \n", 0, move_message_number, player_number, move->x - 1, move->y - 1);
			break;
	}
	unsigned int length = 16 + strlen(move_str);
	while(length % 4 != 0)
		length++;
	length += 4;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x68;
	packet[4] = (r->number >> 8);
	packet[5] = r->number & 0x00ff;
	//definitely seen black here with 0, and our invite
	//also seen black here with 1 and not our invite
	//FIXME we could really try our_invitation here
	//but remember there's that 8th byte that gets
	//wrapped to the next row in the packet dumps
	/*if(!r->our_invitation)
		packet[6] = (r->black_name != getUsername());
	else
		packet[6] = (r->black_name == getUsername());*/
	packet[6] = !r->our_invitation;
	//packet[6] = 0x00;
	//packet[6] = (r->black_name != getUsername());
	//packet[6] = r->white_first_flag;
	//packet[6] = FIRST_BYTE(send_flags);
	packet[7] = 0x01;		//this as 0 didn't come back
	//packet[8] is definitely whether we accepted game
	packet[8] = !r->our_invitation;
	for(i = 9; i < 16; i++)
		packet[i] = 0x00;
	
	strncpy((char *)&(packet[16]), move_str, strlen(move_str));
	for(i = strlen(move_str) + 16; i < length; i++)
		packet[i] = 0x00;

#ifdef RE_DEBUG
	printf("movepacket: ");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
	for(i = 0; i < length; i++)
		printf("%c", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	qDebug("Sending move");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending move packet");
	delete[] packet;

	if(move->flags == MoveRecord::RESIGN)
	{
		//FIXME, pretty sure this is just if its our game
		//actually maybe loser does always send this
		// !!person who accepted offer sends 0672!! FIXME
		sendMatchResult(r->number);
	}
	else if(move->flags == MoveRecord::DONE_SCORING)
	{
		sendEndgameMsg(r, done_scoring);	
	}
	else
	{
		//don't need to set this in resign?
		previous_move_pass = false;
	}
}

/* Move this elsewhere in file to be appropriately ordered */
void TygemConnection::writeNotNicknameAndRank(char * p, const PlayerListing & player)
{
	unsigned int ordinal;
	char qualifier;
	
	writeZeroPaddedString(p, player.notnickname, 14);
	p[14] = 0x00;	//correct? doublecheck FIXME
	sscanf(player.rank.toLatin1().constData(), "%d%c", &ordinal, &qualifier);
	if(qualifier == 'k')
		p[15] = 0x12 - ordinal;
	else if(qualifier == 'p')
		p[15] = ordinal + 0x1a;
	else
		p[15] = ordinal + 0x11;
}

void TygemConnection::writeNicknameAndCID(char * p, const PlayerListing & player)
{
	writeZeroPaddedString(p, player.name, 11);
	p[11] = player.country_id;
}

/* We could use this in handlePlayerListing except that that would double the calls
 * to new since player listings are copied by room in entering them into the registry.
 * FIXME however, I can't off hand remember why it does that, so I should consider
 * fixing that to not copy construct the registry entries if that's okay
 * with all the registries. */
PlayerListing * TygemConnection::getOrCreatePlayerListingFromRecord(char * r)
{
	Room * room = getDefaultRoom();
	unsigned char name[15];
	name[14] = 0x00;

	strncpy((char *)name, r, 14);
	//QString encoded_name = QString((char *)name);
	QString rank;
	unsigned char country_id;
	QString encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	if(r[15] < 0x12)
		rank = QString::number(0x12 - r[15]) + 'k';
	else if(r[15] > 0x1a)
		rank = QString::number(r[15] - 0x1a) + 'p';
	else
		rank = QString::number(r[15] - 0x11) + 'd';
	strncpy((char *)name, &(r[16]), 11);
	//QString encoded_name2 = QString((char *)name);
	QString encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	country_id = r[27];
	PlayerListing * player = room->getPlayerListing(encoded_name2);
	if(!player)
	{
		player = new PlayerListing();
		player->online = true;
		player->info = "??";
		player->playing = 0;
		player->observing = 0;
		player->idletime = "-";
		player->notnickname = encoded_name;
		player->name = encoded_name2;
		player->rank = rank;
		player->country_id = country_id;
		room->recvPlayerListing(player);
		player = room->getPlayerListing(encoded_name2);
	}
	return player;
}
/* This is in addition to and before the board SKI msg from sendMove()
 * and since its the same form, we're adding the counting messages to
 * it as well. Remember to delete sendCountMsg() FIXME*/
void TygemConnection::sendEndgameMsg(const GameData * game, enum EGVersion version)
{
	unsigned int length;
	if(version == opponent_disconnect_timer)
		length = 0x38;
	else if(version == opponent_reconnects)
		length = 0x34;
	else
		length = 0x30;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	PlayerListing * opponent;
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	if(version == pass)
		packet[3] = 0x69;
	else if(version == request_count)
		packet[3] = 0x6a;
	else if(version == accept_count_request ||
			version == refuse_count_request)
		packet[3] = 0x6b;
	else
		packet[3] = 0x7b;
	packet[4] = (game->number >> 8);
	packet[5] = game->number & 0x00ff;
	/*if(!game->our_invitation)
		packet[6] = (game->black_name != getUsername());
	else
		packet[6] = (game->black_name == getUsername());*/
	packet[6] = !game->our_invitation;
	//packet[6] = (game->black_name == getUsername());
	packet[7] = 0x01;	//see discussions in sendMove about bytes 6, 7 and 8
	
	//is opp_name encoded or normal?  we need to look up so, this matters FIXME
	// Also, there's a boarddispatch->getOpponentName() function, clean this stuff up FIXME
	opponent = getDefaultRoom()->getPlayerListing((game->black_name != getUsername() ? game->black_name : game->white_name));
	if(!opponent)
	{
		/* This is going to happen a lot, we need to wait until we have
		 * all players in list before doing this.  Same thing with
		 * not popping up resume dialogs too often FIXME FIXME */
		qDebug("Can't get opponent for game!!!\n");
		delete[] packet;
		return;
	}

	writeNotNicknameAndRank((char *)&(packet[8]), *opponent);
	writeNicknameAndCID((char *)&(packet[24]), *opponent);
	
	for(i = 36; i < length; i++)
		packet[i] = 0x00;
	
	switch(version)
	{
		case done_scoring:
			packet[37] = 0xc5;
			break;
		case reject_count:
			packet[37] = 0xc6;
			break;
		case accept_count:
			packet[36] = 0x01;
			packet[37] = 0xc6;
			break;
		// sure this is draw and not adjourn ?!?!
		// looks like it but translate the chinese
		// FIXME
		case request_draw:
			packet[36] = 0x01;
			packet[37] = 0xc3;
			break;
		case accept_draw_request:
			packet[36] = 0x01;
			packet[37] = 0xc4;
			break;
		case refuse_draw_request:
			//packet[36] = 0x00;	//implied, same below
			packet[37] = 0xc4;
			break;
		case accept_count_request:
			packet[36] = 0x01;
			break;
		case opponent_disconnect_timer:
		{
			packet[36] = 0x01;
			packet[37] = 0xf1;
			packet[38] = 0x0c;
			//packet[39] = 0x00;
			BoardDispatch * boarddispatch = getIfBoardDispatch(game->number);
			if(!boarddispatch)
			{
				qDebug("Opponent disconect but no board dispatch");
				return;
			}
			fillTimeChunk(boarddispatch, &(packet[40]));
			packet[48] = seconds_until_opp_forfeits & 0x00ff;
			packet[49] = (seconds_until_opp_forfeits >> 8);
		}
			break;
		case we_resume:
			packet[36] = 0x01;
			packet[37] = 0xf0;
			//packet[38] = 0x00;
			//packet[39] = 0x00;
			break;
		case opponent_reconnects:
		{
			//packet[36] = 0x00;
			packet[37] = 0xf1;
			packet[38] = 0x08;
			//packet[39] = 0x00;
			BoardDispatch * boarddispatch = getIfBoardDispatch(game->number);
			if(!boarddispatch)
			{
				qDebug("Opponent disconect but no board dispatch");
				return;
			}
			fillTimeChunk(boarddispatch, &(packet[40]));
		}
			break;
		case request_undo:
			packet[37] = 0xc1;
			break;
		case refuse_undo_request:
			packet[37] = 0xc2;
			break;
		case accept_undo_request:
			packet[36] = 0x01;
			packet[37] = 0xc2;
			break;
		default:
			break;
	}
	
#ifdef RE_DEBUG
	printf("Endgame packet:\n");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	/* FIXME, we should somehow have this standard thing attached to all send
	 * messages instead of duplicating the code everywhere */
	encode(packet, (length / 4) - 2);
	qDebug("Sending endgame msg");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending endgame msg packet");
	delete[] packet;
	
	BoardDispatch * boarddispatch = getBoardDispatch(game->number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for our game: %d", game->number);
		return;
	}
	
	if(version != pass && version != opponent_disconnect_timer &&
	   version != we_resume && version != opponent_reconnects &&
	   version != request_undo && version != refuse_undo_request &&
	   version != accept_undo_request)	//double check undo and stop time 0683 FIXME
		sendStopTime(boarddispatch, version);
}

/* These seem like they're sent really liberally:
	- after match offerer hits done_scoring first
	- after match offerer accepts count
	- after match accepter hits done_scoring first
	- after match accepter rejects count
	- after match accepter hits done_scoring second
	- after match acccepter accepts count
   And they're all basically the same... I imagine they're some kind of
   server update but clients could use them for info too, they do get
   them as far as I know so the server does pass them on.
   
   There also might be a stop/start flag... maybe... FIXME
*/
void TygemConnection::sendStopTime(BoardDispatch * boarddispatch, enum EGVersion version)
{
	unsigned int length = 0x18;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	GameData * game = boarddispatch->getGameData();
	TimeRecord us = boarddispatch->getOurTimeRecord();
	TimeRecord them = boarddispatch->getTheirTimeRecord();
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x83;
	packet[4] = (game->number >> 8);
	packet[5] = game->number & 0x00ff;
	
	packet[6] = 0xaa;		//no idea
	packet[7] = 0x01;		//color == black ?
	packet[8] = (version == done_scoring ? 0x03 : 0x05);
	packet[9] = 0x00;
	packet[10] = 0x00;		//this might be 0x01 if after accept count
	packet[11] = 0x00;
	
	/* Below is taken from sendTime(): (and shifted by 4 bytes, gd => game)*/
	//03 0a 12 80 03 19 12 40 00 00 00 00
	//03 07 0f 80 03 30 11 40 00 00 00 00
	//03 33 13 80 03 23 13 40 00 00 00 00
	
	fillTimeChunk(boarddispatch, &(packet[12]));
	for(i = 20; i < length; i++)
		packet[i] = 0x00;
	
#ifdef RE_DEBUG
	printf("stop time packet:\n");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	
	encode(packet, (length / 4) - 2);
	qDebug("Sending stop time");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending stop time packet");
	delete[] packet;
	//boarddispatch->stopTime();
}

/* This appears to be sent by both players as part of
 * resuming a game after a disconnect */
void TygemConnection::sendResumeFromDisconnect(unsigned int game_number)
{
	unsigned int length = 0x0c;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x8d;
	packet[4] = (game_number >> 8);
	packet[5] = game_number & 0x00ff;
	for(i = 6; i < length; i++)
		packet[i] = 0x00;
	
	
	encode(packet, (length / 4) - 2);
	qDebug("Sending resume from disconnect");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending resume from disconnect");
	delete[] packet;
}

void TygemConnection::sendOpponentDisconnect(unsigned int game_number, enum OpponentDisconnectUpdate opp)
{
	unsigned int length = 0x18;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	BoardDispatch * boarddispatch;
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Can't send opponent disconnect, no board for %d", game_number);
		return;
	}
	GameData * game = boarddispatch->getGameData();
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x81;
	packet[4] = (game->number >> 8);
	packet[5] = game->number & 0x00ff;
	
	for(i = 8; i < length; i++)
		packet[i] = 0x00;
	
	if(opp == opponent_disconnect)
	{
		//packet[6] = 0x06;	//??
		packet[6] = !game->our_invitation;
		packet[7] = 0x01;	//not color
	}
	else if(opp == opponent_reconnect)
	{
		//packet[6] = 0x01;
		packet[6] = !game->our_invitation;
		packet[7] = 0x01;
		fillTimeChunk(boarddispatch, &(packet[8]));
	}
	
	encode(packet, (length / 4) - 2);
	qDebug("Sending opp disconnect");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending opp disconnect packet");
	delete[] packet;
}

/* Sent following the normal one */
void TygemConnection::sendLongOpponentDisconnect(unsigned int game_number)
{
	unsigned int length = 0x40;
	unsigned char * packet = new unsigned char[length];
	unsigned int i;
	BoardDispatch * boarddispatch;
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Can't send long opponent disconnect, no board for %d", game_number);
		return;
	}
	GameData * game = boarddispatch->getGameData();
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x52;
	
	game->opponentdropcount++;
	QString disconnect_info = "**Halt Daekuk GAME_BLOCK_IND! - room:" +
			QString::number(game_number) + " <" +
			//be careful here because there's usernames and nicknames FIXME
			(game->white_name == getUsername() ? game->black_name : game->white_name) +
			"> cnt:" + QString::number(game->opponentdropcount);
	unsigned int str_len = strlen(disconnect_info.toLatin1().constData());
	for(i = 4; i < str_len; i++)
		packet[i] = disconnect_info.toLatin1().constData()[i - 4];
	for(i = 4 + str_len; i < length; i++)
		packet[i] = 0x00;
	
	encode(packet, (length / 4) - 2);
	qDebug("Sending long opp disconnect");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending long opp disconnect packet");
	delete[] packet;
	
	seconds_until_opp_forfeits = 300;
}

/* FIXME header uses "game_code" everywhere, but that's for oro,
 * tygem uses game_numbers */
void TygemConnection::sendOpponentDisconnectTimer(unsigned int game_number)
{	
	BoardDispatch * boarddispatch;
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Can't send opponent disconnect timer, no board for %d", game_number);
		return;
	}
	GameData * game = boarddispatch->getGameData();
	seconds_until_opp_forfeits--;
	if(seconds_until_opp_forfeits == -1)
	{
		//just send a straight 0x0672
		GameResult aGameResult;
		aGameResult.result = GameResult::FORFEIT;
		aGameResult.winner_name = getUsername();
		if(game->white_name == getUsername())
		{
			aGameResult.winner_color = stoneWhite;
			aGameResult.loser_name = game->black_name;
		}
		else
		{
			aGameResult.winner_color = stoneBlack;
			aGameResult.loser_name = game->white_name;
		}
		boarddispatch->recvResult(&aGameResult);
		sendLongMatchResult(game_number);
		killTimer(opponentDisconnectTimerID);
		opponentDisconnectTimerID = 0;
		/* FIXME we probably shouldn't have to look this up */
		PlayerListing * opponent = getDefaultRoom()->getPlayerListing(game->black_name == getUsername() ? game->white_name : game->black_name);
		if(!opponent)
		{
			qDebug("Can't get opponent listing to remove after drop\n");
			return;
		}
		opponent->online = false;	
		getDefaultRoom()->recvPlayerListing(opponent);
		return;
	}
	/* Next, we need to figure when all these things get sent
	 * as well as their being some other messages and custom 0672s
	 * both for forfeits as well as DRAW messages
	 * but the three bytes in the move and other headers are still
	 * wrong, so we should fix that so we can, for instance, send resigns
	 * again at least. 
	 * And then I think that the accepted_match_player needs to be cleared
	 * or something and anyway, its an ugly way of doing it, but that
	 * affects all the protocols so its a big deal */
	if((seconds_until_opp_forfeits + 1) % 60 == 0)
	{
		boarddispatch->recvKibitz(0, QString("Opponent forfeits game in %1 minutes.").arg((seconds_until_opp_forfeits + 1) / 60));
	}
	sendEndgameMsg(game, opponent_disconnect_timer);
}

/* As far as I can tell, we send the match result if we win... */
void TygemConnection::sendResult(GameData * game, GameResult * result)
{
	//if(!game->our_invitation)
	if(result->winner_name == getUsername())
		sendLongMatchResult(game->number);
}

void TygemConnection::sendRequestCount(unsigned int game_id)
{
	BoardDispatch * bd = getIfBoardDispatch(game_id);
	if(!bd)
	{
		qDebug("No board dispatch for %d", game_id);	
	}
	GameData * gd = bd->getGameData();
	sendEndgameMsg(gd, request_count);
}

void TygemConnection::sendAcceptCountRequest(GameData * data)
{
	sendEndgameMsg(data, accept_count_request);
}

void TygemConnection::sendRefuseCountRequest(GameData * data)
{
	sendEndgameMsg(data, refuse_count_request);
}

void TygemConnection::sendRequestDraw(unsigned int game_id)
{
	BoardDispatch * bd = getIfBoardDispatch(game_id);
	if(!bd)
	{
		qDebug("No board dispatch for %d", game_id);	
	}
	GameData * gd = bd->getGameData();
	sendEndgameMsg(gd, request_draw);
}

void TygemConnection::sendAcceptDrawRequest(GameData * data)
{
	sendEndgameMsg(data, accept_draw_request);
}

void TygemConnection::sendRefuseDrawRequest(GameData * data)
{
	sendEndgameMsg(data, refuse_draw_request);
}

void TygemConnection::sendRejectCount(GameData * data)
{
	sendEndgameMsg(data, reject_count);
}

void TygemConnection::sendAcceptCount(GameData * data)
{
	sendEndgameMsg(data, accept_count);
}
/* FIXME pretty much all messages that I can think of off the top of my head
 * come back from the server meaning that perhaps we should not act on any
 * messages until they come back?  Much like placing stones? */


/* Loser sends this after resign, but I think only if they offered
 * the game 
 * Not sure here, looks like winner sends 0672, but again, I've so
 * far only seen that when it was the person offering...
 * offer loser does not send, non-offer-loser does send, I think (0672)
 * also, FIXME, we use "match" and "game" inconsistently */
 /* I think loser has to send this upon accepting a lost match... ?
  * or maybe I just screwed up the 7b */
void TygemConnection::sendMatchResult(unsigned short game_code)
{
	unsigned int length = 0x30;
	unsigned char * packet = new unsigned char[length];
	unsigned int i, ordinal;
	char qualifier;
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for match result send", game_code);
		return;
	}
	/* We need to notify boarddispatch of finished game so we can
	 * get the full result */
	//boarddispatch->recvResult(0);		//doublecheck FIXME don't think we need to do this anymore
	/* Unless we ever send it with the score margin from somewhere else */
	GameData *aGameData = boarddispatch->getGameData();
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x70;
	packet[4] = (game_code >> 8);
	packet[5] = game_code & 0x00ff;
	
	//packet[6] = 0x00;	//winner color
	//packet[6] = (aGameData->black_name == getUsername()) ^ aGameData->white_first_flag;
	//definitely who offered
	packet[6] = !aGameData->our_invitation;
	packet[7] = 0x01;
	
	PlayerListing * opponent = getDefaultRoom()->getPlayerListing(aGameData->black_name == getUsername() ? aGameData->white_name : aGameData->black_name);
	if(!opponent)
	{
		qDebug("Can't get opponent listing for match result send\n");
		return;
	}
	/* FIXME This header is same as sendEnterScoring, combine */
	/* FIXME FIXME FIXME, and sendLongMatchResult, etc. */
	writeZeroPaddedString((char *)&(packet[8]), opponent->notnickname, 14);
	packet[22] = 0x00;
	sscanf(opponent->rank.toLatin1().constData(), "%d%c", &ordinal, &qualifier);
	if(qualifier == 'k')
		packet[23] = 0x12 - ordinal;
	else if(qualifier == 'p')
		packet[23] = ordinal + 0x1a;
	else
		packet[23] = ordinal + 0x11;
	//their_name = (char *)opponent->name.toLatin1().constData();
	writeZeroPaddedString((char *)&(packet[24]), opponent->name, 11);
	packet[34] = 0x00;
	packet[35] = opponent->country_id;
	//certainly all zeroes if resign
	/*
	if(!aGameData->fullresult)
	{
		qDebug("No result on game data to send result");
		return;
	}
	float margin = aGameData->fullresult->winner_score - aGameData->fullresult->loser_score;
	packet[10] = (int)margin & 0x00ff;
	packet[11] = ((int)margin >> 8);
	*/	
	for(i = 36; i < length; i++)
		packet[i] = 0x00;
#ifdef RE_DEBUG
	printf("Sending match result:\n");
	for(i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	qDebug("Sending match result");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending match result");
	delete[] packet;
}

/* FIXME we also use game_code and number inconsistently as holdover from
 * oro code. */
void TygemConnection::sendLongMatchResult(unsigned short game_code)
{
	unsigned int tygem_game_record_str_length;
	unsigned int i;
	
	BoardDispatch * boarddispatch = getIfBoardDispatch(game_code);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for match result send", game_code);
		return;
	}
	/* We need to notify boarddispatch of finished game so we can
	* get the full result */
	//boarddispatch->recvResult(0);
	GameData * game = boarddispatch->getGameData();
	

	QByteArray tygem_game_record = getTygemGameRecordQByteArray(game);
	if(tygem_game_record == QByteArray())
		return;	//error already reported by function
	tygem_game_record_str_length = tygem_game_record.size();
#ifdef RE_DEBUG
	printf("Tygem game record length: %d\n", tygem_game_record_str_length);
#endif //RE_DEBUG
	unsigned int length = 0x28 + tygem_game_record_str_length;
	while(length % 0x10 != 0)
		length++;
	unsigned char * packet = new unsigned char[length];
	packet[0] = (length >> 8);
	packet[1] = length & 0x00ff;
	packet[2] = 0x06;
	packet[3] = 0x72;
	packet[4] = (game_code >> 8);
	packet[5] = game_code & 0x00ff;
	//6 may not matter?
	//FIXME need to consolidate these headers
	//packet[6] = (game->black_name == getUsername());
	/*if(!game->our_invitation)
		packet[6] = (game->black_name != getUsername());
	else
		packet[6] = (game->black_name == getUsername());*/
	packet[6] = !game->our_invitation;
	packet[7] = 0x01;	//see discussions in sendMove about bytes 6, 7 and 8
	
	PlayerListing * opponent = getDefaultRoom()->getPlayerListing(game->black_name == getUsername() ? game->white_name : game->black_name);
	if(!opponent)
	{
		qDebug("Can't get opponent listing for long match result send\n");
		return;
	}
	writeNotNicknameAndRank((char *)&(packet[8]), *opponent);
	
	packet[24] = getVictoryCode(*(game->fullresult));
	packet[25] = 0x00;
	packet[26] = 0x00;
	//packet[27] = 0x3c;	//what is this FIXME (score?)
	packet[27] = 0x00;
	packet[28] = (game->moves >> 8);
	packet[29] = game->moves & 0xff;
	packet[30] = 0x00;
	packet[31] = 0x00;
	//03 00 00 00 00 01 00 00   //opp white resigns
	//04 00 00 00 00 0e 00 00   //opp black resigns
	//               0b 	    //draw
	//01 00 05 73 00 6c 00 00   //likely scores
	//
	fillTimeChunk(boarddispatch, &(packet[32]));

	for(i = 40; i < tygem_game_record_str_length + 40; i++)
		packet[i] = tygem_game_record.constData()[i - 40];
	for(i = 40 + tygem_game_record_str_length; i < length; i++)
		packet[i] = 0x00;

#ifdef RE_DEBUG
	printf("long match packet:\n");
	for(i = 0; i < length; i++)
		printf("%c ", packet[i]);
	printf("\nandinhex:\n");
	for(i = 0; i < length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
	fflush(stdout);

	//printf("And we're not sending this yet\n");
	//delete[] packet;
	//return;
#endif //RE_DEBUG
	
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	qDebug("Sending long match result");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending long match result");
	delete[] packet;
}

/* FIXME convert from the tygem packets */
/* Okay, I think this also happens to be the ".gib" gibo game file format
 * and further, I think you can request listings of players games.  Now while
 * that latter requires some other windows to display and track games which
 * might even warrant a thumbnail explorer type window, etc., etc., which
 * basically puts it behind the "watch/fans/friends" list windows and
 * features for me, although I still might want to get the protocol
 * part down... but anyway, the point is, we might eventually change these
 * to a more generic function where the virtuals would just return the
 * specific labels somehow, or fill those labels in, for the different
 * servers and places 
 * Actually, I think we should investigate the gamerecord retrieve protocols
 * because that might hint at what sort of info is used where and also
 * allow us to more precisely pin down the differences between the three
 * servers.*/
 /* Another note is that, I almost feel like putting english words in here
  * or something.  For the most part only that last tag is I think read.
  * They also seem to change it sometimes.  Like they change which words
  * are used, etc.. Anything mildly appropriate is probably fine.  The worst
  * that could happen is someone would have an illegible game record, maybe
  * and even that's assuming the tag isn't read */
QByteArray TygemConnection::getTygemGameRecordQByteArray(GameData * game)
{
	Room * room = getDefaultRoom();
	const PlayerListing * black, * white;
	int black_ordinal, white_ordinal;
	char black_qualifier, white_qualifier;
	QByteArray white_qualifier_string, black_qualifier_string;
	unsigned char black_level, white_level;
	unsigned short year;
	unsigned char month, day, hour, minute, second;
	secondsToDate(year, month, day, hour, minute, second);
	
	int margin = 0;
	
	black = room->getPlayerListing(game->black_name); 
	if(!black)
	{
		qDebug("Can't get player listing for black: \"%s\"", game->black_name.toLatin1().constData());
		return QByteArray();
	}
	white = room->getPlayerListing(game->white_name); 
	if(!white)
	{
		qDebug("Can't get player listing for white: \"%s\"", game->white_name.toLatin1().constData());
		return QByteArray();
	}
	/* FIXME we need the korean dan qualifier character */
	sscanf(black->rank.toLatin1().constData(), "%d%c", &black_ordinal, &black_qualifier);
	if(black_qualifier == 'k')
	{
		black_level = 0x12 - black_ordinal;
		black_qualifier_string.append("\xb1\xde");
	}
	else if(black_qualifier == 'p')
		black_level = black_ordinal + 0x1a;
	else
	{
		black_level = black_ordinal + 0x11;
		black_qualifier_string.append("\xb4\xdc");
	}
	sscanf(white->rank.toLatin1().constData(), "%d%c", &white_ordinal, &white_qualifier);
	if(white_qualifier == 'k')
	{
		white_level = 0x12 - white_ordinal;
		white_qualifier_string.append("\xb1\xde");
	}
	else if(white_qualifier == 'p')
		white_level = white_ordinal + 0x1a;
	else
	{
		white_level = white_ordinal + 0x11;
		white_qualifier_string.append("\xb4\xdc");
	}
	
	QByteArray string;
	string += "\\[GIBOKIND=China\\]\r\n";
	string += "\\[TYPE=0\\]\r\n";
	string += "\\[GAMECONDITION=";
				
							//there's also:
							//bbe7c8b02031203a20c8a3bcb1
							//which might be not enough moves
							//or no actually that was when I played a friendly
							//game with a huge handicap
			//c8a3bcb1203a20
			//room number??
			//c1fdb9dd20
			//b0f8c1a6
			//white player forfeits
			//c8a3bcb1203a20
			//room number (I think)
			//c1fdb9dd20
			//b0f8c1a6
			string.append("\xc1\xa4\xbc\xb1");
			//c1a4bcb1 seems sufficient for a finished
			//game
	string += "\\]\r\n";
	string += "\\[GAMETIME=";
	//FIXME doublecheck
	//c1a6 c7d1 bdc3 b0a3 20 minutes ba d0 0d 0a number c3 ca 20 c3 ca c0
	//d0 b1e2 20 periods b9f8
	string.append("\xc1\xa6\xc6\xd1\xbd\xc3\xb0\xa3");
	string += " " + QByteArray::number(game->maintime);
	string.append("\xba\xd0\x0d\x0a");
	string += QByteArray::number(game->periodtime);
	string.append("\xc3\xca \xc3\xca\xc0\xd0\xb1\xe2");
	string += " " + QByteArray::number(game->stones_periods);
	string.append("\xb9\xf8");
	string += "\\]\r\n";
	string += "\\[GAMERESULT=";		//tygem is 3f203332393f203f with the 2 spaces in there
						//3f3332353f3f	//w + 325 margin ONLY, no color
						//badac6e5cab1bce4caa4 B + T
						
	switch(game->fullresult->result)
	{
		case GameResult::DRAW:
			string.append("\xb9\xab\xbd\xc2\xba\xce");
			break;
		case GameResult::FORFEIT:
			if(game->fullresult->winner_color == stoneWhite)
				string.append("\xb9\xe9");
			else
				string.append("\xc8\xe6");
			//c8e6 20 bdc3 b0a3 20 c3ca b0fa bdc2	//white forfeits
			string.append(" \xbd\xc3\xb0\xa3 \xc3\xca\xb0\xfa\xbd\xc2");
			break;
		case GameResult::TIME:
			if(game->fullresult->winner_color == stoneWhite)
				string.append("\xb9\xe9");
			else
				string.append("\xc8\xe6");
			//c8e6 20 bdc3 b0a3 bdc2		//white loses on time
			string.append(" \xbd\xc3\xb0\xa3\xbd\xc2");
			break;
		case GameResult::RESIGN:
			if(game->fullresult->winner_color == stoneWhite)
			{
				//b9e9 20 bad2 b0e8 bdc2
				//b0d7 c6e5 d6d0 c5cc caa4 W + R
				string.append("\xb9\xe9");
			}
			else
			{
				//c8e6 20 bad2 b0e8 bdc2
				string.append("\xc8\xe6");
			}
			string.append(" \xba\xd2\xb0\xe8\xbd\xc2");
			break;
		case GameResult::SCORE:
			{
			float fmargin = game->fullresult->winner_score = game->fullresult->loser_score;
			margin = (int)fmargin;
			if(fmargin > (float)margin)
				margin = (margin * 10) + 5;
			else
				margin *= 10;
			qDebug("FIXME margin: %d\n", margin);
			//3f20 margin 3f3f203f	//black wins
			//3f20 margin 3f203f    //white wins?
			//string += QChar(0x3f);
			string += " " + QByteArray::number(margin);
			//	QChar(0x3f3f) + " " + QChar(0x3f);
			//pretty sure these ??? are a mistake
			}
			break;
		default:
			break;
	}
	string += "\\]\r\n";
	string += "\\[GAMEZIPSU=" + QByteArray::number(margin) + "\\]\r\n";
	string += "\\[GAMEDUM=0\\]\r\n";
	string += "\\[GAMEGONGJE=0\\]\r\n";
	//often 65
	string += "\\[GAMETOTALNUM=";	//0xd7dc: 3335cafd" 35?
	//c3d13a20 moves bcf6
	//this has the number of moves in it I think
	string.append("\xc3\xd1\x3a");
	string += " " + QByteArray::number(game->moves);
	string.append("\xbc\xf6");
	string += "\\]\r\n";
	string += "\\[SZAUDIO=0\\]\r\n";
	string += "\\[GAMENAME=";
	string.append("\xd3\xd1\xd2\xea\xb6\xd4\xbe\xd6");
	string += "\\]\r\n";		//c8b3bcb1
	/* Be careful here also, no idea what this says, we should
	 * probably leave it blank */
	string += "\\[GAMEDATE=" + QByteArray::number(year) + "-"; 
	string.append("\xc4\xea");
	string += QByteArray::number(month);
	string.append("\xd4\xc2");
	string += QByteArray::number(day);
	string.append("\xc8\xd5 \xcf\xc2\xce\xe7");
	string += " " + QByteArray::number(hour % 12) +
			(minute < 10 ? ":0" : ":") + QByteArray::number(minute) + "\\]\r\n";
	/* Note that its very likely that one of the above strings contains a PM versus
	 * AM */
	const char tygem_game_place_array[] = {0xc5,0xb8,0xc0,0xcc,0xc1,0xaa,0x4c,0x69,0x76,0x65,0xb9,0xd9,0xb5,0xcf};
	QByteArray tygem_game_place(tygem_game_place_array, 14);
	string += "\\[GAMEPLACE=";
	string += tygem_game_place;
	string += "\\]\r\n";
	string += "\\[GAMELECNAME=\\]\r\n";
	string += "\\[GAMEWHITENAME=" + serverCodec->fromUnicode(white->notnickname) + "("
				+ QByteArray::number(white_ordinal)
				+ white_qualifier_string + ")\\]\r\n";
	string += "\\[GAMEWHITELEVEL=" + QByteArray::number(white_level)
				+ white_qualifier_string + "\\]\r\n";
	string += "\\[GAMEWHITENICK=" + serverCodec->fromUnicode(white->name) + "\\]\r\n";
	string += "\\[GAMEWHITECOUNTRY=" + QByteArray::number(white->country_id) + "\\]\r\n";
	string += "\\[GAMEWAVATA=1\\]\r\n";
	string += "\\[GAMEWIMAGE=\\]\r\n";
	string += "\\[GAMEBLACKNAME=" + serverCodec->fromUnicode(black->notnickname) + "(" 
				+ QByteArray::number(black_ordinal)
				+ black_qualifier_string + ")\\]\r\n";
	string += "\\[GAMEBLACKLEVEL=" + QByteArray::number(black_level)
				+ black_qualifier_string + "\\]\r\n";
	string += "\\[GAMEBLACKNICK=" + serverCodec->fromUnicode(black->name) + "\\]\r\n";
	string += "\\[GAMEBLACKCOUNTRY=" + QByteArray::number(black->country_id) + "\\]\r\n";
	string += "\\[GAMEBAVATA=1\\]\r\n";
	string += "\\[GAMEBIMAGE=\\]\r\n";
	string += "\\[GAMECOMMENT=\\]\r\n";
	//country = 2 = gbkind = gibokind
	string += "\\[GAMEINFOMAIN=GBKIND:2,GTYPE:0,GCDT:1,GTIME:" +
				QByteArray::number(game->maintime) + "-" +
				QByteArray::number(game->periodtime) + "-" +
				QByteArray::number(game->stones_periods) + ",GRLT:0";
	string += ",ZIPSU:" + QByteArray::number(margin) + 
		  ",DUM:0,GONGJE:0,TCNT:" +
				QByteArray::number(game->moves);
	//FIXME doublecheck game->moves is accurate
	qDebug("FIXME Game moves: %d\n", game->moves);
	string += ",AUSZ:0\\]\r\n";
	string += "\\[GAMEINFOSUB=GNAMEF:0,GPLCF:0,GNAME:";	//name is c8a3bcb1 just like name of game
	string += "GDATE:" + QByteArray::number(year) + "- " + QByteArray::number(month)
				+ "- " + QByteArray::number(day) + "-" + QByteArray::number(hour)
				+ "-" + QByteArray::number(minute) + "-" + QByteArray::number(second);
	string += ",GPLC:" + tygem_game_place;
	string += ",GCMT:\\]\r\n";
	string += "\\[WUSERINFO=WID:" + serverCodec->fromUnicode(white->notnickname) + ",WLV:" + QByteArray::number(white_level) + 
			",WNICK:" + serverCodec->fromUnicode(white->name) + ",WNCD:" + QByteArray::number(white->country_id) +
				",WAID:60001,WIMG:\\]\r\n";
	string += "\\[BUSERINFO=BID:" + serverCodec->fromUnicode(black->notnickname) + ",BLV:" + QByteArray::number(black_level) +
			",BNICK:" + serverCodec->fromUnicode(black->name) + ",BNCD:" + QByteArray::number(black->country_id) +
				",BAID:60001,BIMG:\\]\r\n";
	/* I've seen 60001 for the BAID and WAID,
	 * could be the image they use... maybe, ... */
	/* Here, I'm thinking S0 is black wins, S1 is white wins
	 * then again, there's also W1 indicating white win with W0 as white loss */
	/* I've also seen W4 as indicating W + R */
	/* I've also seen S1 W7 as B + T also with Z0 indicating that's time remaining
	 * or something */
	/* T is likely time settings, 30 seconds, 3 periods, 1200 total time maybe (20min)*/
	/* Also, I'm thinking Z and ZIPSU are the score, the margin */

	/* From tygem: W0 B wins by score
		       W3 B + R */
	/* if 3 and 4 are resigns, we might assume 6 and 7 are time, but then there's
	 * the question of what 2 and 5 are... we need to see W win by score and by time */
	/* I suspect this is the stuff that really matters */
	string += getTygemGameTagQByteArray(game, white_level, black_level, margin);
	
				//tag is simple of earlier info

	/* the next huge chunk is some kind of record list
	 * a lot of 0x5c '\' in it, maybe escaping '[' and ']'
	 * the following fields are in this chunk, excepting
	 * the weird stuff I couldn't easily type, also I may
	 * have transcripted it wrong:
		GIBOKIND=China
		TYPE=0
		GAMECONDITION=++-+
		GAMETIME=
		GAMERESULT=
		GAMEZIPSU=60
		GAMEDUM=0
		GAMEGONGJE=0
		GAMETOTALNUM=
		SZAUDIO=0
		GAMENAME=
		GAMEDATE=
		GAMEPLACE=
		GAMELECNAME=\]	//empty
		GAMEWHITENAME=
		GAMEWHITELEVEL=
		GAMEWHITENICK=intrusion
		GAMEWHITECOUNTRY=2		//is this passed as 0x02 byte in places !!! FIXME
		GAMEWAVATA=1
		GAMEWIMAGE=\]	//empty
		GAMEBLACKNAME=
		GAMEBLACKLEVEL=
		GAMEBLACKNICK=peterius
		GAMEBLACKCOUNTRY=2
		GAMEBAVATA=1
		GAMEBIMAGE=
		GAMECOMMENT=\]	//empty
		GAMEINFOMAIN=GBKIND:2,GTYPE:0,GCDT:1,GTIME:1200-30-3,GRLT:0
			,ZIPSU:60,DUM:0,GONGJE:0,TCNT:143,AUSZ:0
		GAMEINFOSUB=GNAMEF:0,GPLCF:0,GNAME:(untypeable)
			,GDATE:2009- 1-31-17-56-38,GPLC:(untypeable),GCMT:\]
		WUSERINFO=WID:intrusion,WLV:9,WNICK:intrusion,WNCD:2,WAID:1,WIMG:\]
		BUSERINFO=BID:peterius,BLV:8,BNICK:peterius,BNCD:2,BAID:1,BIMG:\]
		GAMETAG=S0,R1,D0,G0,W0,Z60,T30-3-1200,C2009:01:31:17:56
			,I:intrusion,L:9,M:peterius,N:8,A:intrusion,B:peterius
			,J:2,K:2\]
	
		Also GAMETAG has some weird fields, and one can see date and time around
		5pm in different places
	
		packet ends 8 0s, possibly padded out to 16 multiple
	
	*/
	return string;
}

/* Arguably, this is the only part that matters */
QByteArray TygemConnection::getTygemGameTagQByteArray(GameData * game, unsigned char white_level, unsigned char black_level, int margin)
{
	/* This first bit is redundant with the only caller, but
	 * I'm starting to feel lazier tonight and I don't want to pass
	 * all those arguments or find some more elegant way of doing
	 * this nonsense. FIXME */
	unsigned short year;
	unsigned char month, day, hour, minute, second;
	secondsToDate(year, month, day, hour, minute, second);
	
	const PlayerListing * white, * black;
	
	black = getDefaultRoom()->getPlayerListing(game->black_name); 
	if(!black)
	{
		qDebug("Can't get player listing for black: \"%s\"", game->black_name.toLatin1().constData());
		return QByteArray();
	}
	white = getDefaultRoom()->getPlayerListing(game->white_name); 
	if(!white)
	{
		qDebug("Can't get player listing for white: \"%s\"", game->white_name.toLatin1().constData());
		return QByteArray();
	}
	
	QByteArray string;
	string = "\\[GAMETAG=S0,R1,D0,GO";
	//I see a lot of R0 and G65
	//R could be rated... 
	//S1 or maybe R3 could refer to a rated game
	//R0 means something too though
	string += ",W" + QByteArray::number(getVictoryCode(*(game->fullresult)));
	string += ",Z" + QByteArray::number(margin);
	string += ",T" +
			QByteArray::number(game->periodtime) + "-" +
			QByteArray::number(game->stones_periods) + "-" +
			QByteArray::number(game->maintime) + ",C" + 
			QByteArray::number(year) + (month < 10 ? ":0" : ":") + 
			QByteArray::number(month) + (day < 10 ? ":0" : ":") + 
			QByteArray::number(day) + (hour < 10 ? ":0" : ":") + 
			QByteArray::number(hour) + (minute < 10 ? ":0" : ":") + 
			QByteArray::number(minute);/* + (second < 10 ? ":0" : ":") +
	QByteArray::number(second);*/	
	string += ",I:" + serverCodec->fromUnicode(white->notnickname) + ",L:" + QByteArray::number(white_level) +
			",M:" + serverCodec->fromUnicode(black->notnickname) + ",N:" + QByteArray::number(black_level) + 
			",A:" + serverCodec->fromUnicode(white->name) + ",B:" + serverCodec->fromUnicode(black->name) + 
			",J:" + QByteArray::number(white->country_id) +
			",K:" + QByteArray::number(black->country_id) + "\\]\r\n";
	qDebug("GAMETAG: %s", string.constData());
	return string;
}

unsigned char TygemConnection::getVictoryCode(GameResult & aGameResult)
{
	switch(aGameResult.result)
	{
		case GameResult::TIME:
			if(aGameResult.winner_color == stoneWhite)
				return 8;
			else
				return 7;
			break;
		case GameResult::RESIGN:
			if(aGameResult.winner_color == stoneWhite)
				return 4;
			else
				return 3;
			break;
		case GameResult::SCORE:
			if(aGameResult.winner_color == stoneWhite)
				return 1;
			else
				return 0;
			break;
		case GameResult::FORFEIT:
			if(aGameResult.winner_color == stoneWhite)
				return 6;
			else
				return 5;
			break;
		case GameResult::DRAW:
			return 2;
			break;
		default:
			break;
	}
	qDebug("Unhandled GameResult code!!");
	return 0;
}

/* It seems like this appears like a match invite if opponent closes
 * the window FIXME */
void TygemConnection::sendRematchRequest(void)
{
}

void TygemConnection::sendRematchAccept(void)
{
}

/* This really needs to be a whole packet of stuff but for now,
 * since the full thing requires some chinese translation: */
void TygemConnection::sendInvitationSettings(bool invite)
{
	unsigned int length = 0x0c;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x0c;
	packet[2] = 0x06;
	packet[3] = 0xa2;
	packet[4] = (invite ? 0 : 2);
	packet[5] = 0xf0;
	packet[6] = 0xad;
	packet[7] = 0xba;
	packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;	//weird byte???

#ifdef RE_DEBUG
	printf("Sending invite settings: ");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	encode(packet, (length / 4) - 2);
#ifdef RE_DEBUG
	printf("encoded: ");
	for(int i = 0; i < (int)length; i++)
		printf("%02x ", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	qDebug("Sending invitation settings\n");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending invitation settings");
	delete[] packet;
}

void TygemConnection::sendDisconnectMsg(void)
{	
	unsigned int length = 8;
	unsigned char * packet = new unsigned char[length];
	
	packet[0] = 0x00;
	packet[1] = 0x08;
	packet[2] = 0x06;
	packet[3] = 0x51;
	packet[4] = 0x00;
	packet[5] = 0x00;
	packet[6] = 0x00;
	packet[7] = 0x00;
	
	qDebug("Sending server disconnect");
	if(write((const char *)packet, length) < 0)
		qWarning("*** failed sending server disconnect");
	delete[] packet;
}

void TygemConnection::encode(unsigned char * p, unsigned int cycles)
{
	uint32_t b, c = 0;
	uint32_t header = *(uint32_t *)p;
	header ^= encode_offset;
	p += 4;
	while(cycles--)
	{
		b = *(uint32_t *)p;
		c ^= b;
		b ^= header;
		*(uint32_t *)p = b;
		p += 4;
	}
	*(uint32_t *)p = c;
}

bool TygemConnection::isReady(void)
{
	if(connectionState == CONNECTED)
		return 1;
	else
		return 0;
}

/* Non IGS servers can override this function and pass
 * their own GSName type for now */
void TygemConnection::handleMessage(QString)
{
}

/* We may convert everything here to unsigned char *s, delete the old...
 * we're still sort of hung up on all the things we were doing
 * to work with the IGS code. */
void TygemConnection::handleMessage(unsigned char * msg, unsigned int size)
{
	unsigned short message_type = msg[3] + (msg[2] << 8);
	int i;
	//message_type = msg[0];
	//message_type >>= 8;
	//message_type += msg[1];
	/* Third byte is likely the size of the message meaning multiple
	 * messages per packet, etc. */
	/*if((message_type & 0x00ff) == 0x00b3)
	{*/
		/*printf("\n****: \n");
		for(i = 0; i < (int)size; i++)
			printf("%02x", msg[i]);
		printf("\n\n");*/
	/*}*/
	
	msg += 4;
	size -=4;
	switch(message_type)
	{
		case TYGEM_GAMESINIT:
		case TYGEM_GAMESUPDATE:
			handleGamesList(msg, size);
			break;
	// possibly update after game won, first 00df
	//009806130002000000df0000000100020000ff00706c6d64000000000000000000000011706c6d64003200390000000294ca
	//8ee1944c000000000000000000116c6c36366c6c006e673200010004000000110000010000020000ff00d0c4ccacc8e7c7ef
	//31000000000000006c6f6f706f706f7000390002ccecc9bdc0cf4b0000000000000000086c616f6b31320068657200020001
	//0000

			
		case TYGEM_PLAYERSINIT:
			handlePlayerList(msg, size);
			//0653 request should also be sent here FIXME
			break;
		case TYGEM_PLAYERSUPDATE:
			handlePlayerList(msg, size);
			break;
		case 0x0617:
			handleServerRoomChat(msg, size);
			break;
		case 0x0618:
			//baec cec0 b1f8 0000 0000 0000 0000 0012
			//6877 6200 0000 0000 0000 0002
			//3030ff003d007a00
			//323fc7ebc7f3b0efd6faa3ac5b3132325db7bfbce4baeccec0b1f8c7ebc7f3b0efd6faa1a
			//328b6d4b2bbcec4c3f7b5c4c6e5d3d1cdb6cbdfa1a3290000000000
			//some kind of game announcement likely
			
			//obviously announcements
			/* I got one of these on windows shortly before a server disconnect! weird FIXME */
#ifdef RE_DEBUG
			printf("0x0618: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case TYGEM_OPENCONVERSATION:
			handleOpenConversation(msg, size);
			break;
		case TYGEM_CONVERSATIONREPLY:
			handleConversationReply(msg, size);
			break;
		case TYGEM_CONVERSATIONMSG:
			handleConversationMsg(msg, size);
			break;
		case 0x0637:
			handleCreateRoomResponse(msg, size);
			break;
		case 0x0638:
#ifdef RE_DEBUG
			printf("0x0638: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			//break;
		case 0x0639:	//in response to 0638 sendJoin
			handleBoardOpened(msg, size);
			break;
		case TYGEM_MATCHINVITE:
			//0x0643: 70657465726975730000000000000001706574657269757300000000696e74727573696f6e00000000000
			//		009696e74727573696f6e0000020100ffff
			handleMatchInvite(msg, size);
			break;
		case TYGEM_MATCHINVITERESPONSE:
			handleMatchInviteResponse(msg, size);
			break;
		case TYGEM_MATCHOFFER:
#ifdef RE_DEBUG
			printf("0645: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			if(msg[43] == 0x02)
				handleMatchOffer(msg, size, offer);
			else if(msg[43] == 0x03)	//double check
				handleMatchOffer(msg, size, decline);
			else
				printf("*** 0645 match offer has strange type: %02x!!!\n", msg[43]);
			//type 0x1e could be rematch, or something else
			break;
		case TYGEM_MATCHOFFERRESPONSE:
			if(msg[43] == 0x01)
				handleMatchOffer(msg, size, accept);
			else if(msg[43] == 0x02)
				handleMatchOffer(msg, size, modify);
			//match offer accept
			//00580646
			//0x0646: 70657465726975730000000000000008696e74727573696f6e000000000000090032012c2801010001
			//000001ffffffff000000000001000000000000706574657269757300000002696e74727573696f6e000002
			//00a00616

			
			//rematch related?
			//0646
			//c4ba d3ea b3bf d4c6 0000 0000 0000 0014
			//6269 7473 0000 0000 0000 0000 0000 0014
			//0003 012c 1e03 0000 0000 001e ffff ffff
			//ffff ffff 0001 0001 0000 0100 6a65 7070
			//3100 0000 0000 0002 6269 7473 0000 0000
			//0000 0002
			break;
		case 0x064d:
			//this might be when opponent leaves game? possibly?
			//maybe not, maybe we just have to pick that up
			//from observers
			printf("0x064d: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
			break;
		case TYGEM_GAMECHAT:	//in game chat
			handleGameChat(msg, size);
			break;
		//0633: 
		//063370657465726975730000000000000008696e74727573696f6e00000000000009706574657269757300000002696e7
		//4727573696f6e000002000000000000020000000000
		
		case TYGEM_OBSERVERSINIT:
		case TYGEM_OBSERVERSUPDATE:
				//maybe we do have to request with oro...
			handleObserverList(msg, size);
			break;
		case TYGEM_MOVE:
			handleMove(msg, size);
			break;
		case 0x0669:	//first pass
			handlePass(msg, size, 1);
			break;
		case 0x066a:
			handleCountRequest(msg, size);
			break;
		case 0x066b:
			handleCountRequestResponse(msg, size);
			break;
		case 0x0670:
			handleGameResult2(msg, size);
			break;
		case 0x0671:
			handleMatchOpened(msg, size);
			break;	
		case 0x0672:
			handleGameResult(msg, size);
			break;
		case 0x0674:
			//067401f90001
			//0x0674: 00910101
			//I think this is the enter score message... 
			//seems to happen after counts... but I don't get
			//who sends it... we get it if we request the count
			//from them and they agree FIXME
			printf("0x0674: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
			break;
			//an REM -1 -1 likely proceeds these as meaning done?
			//this is intrusion to peterius after marking all of his black stones dead
		case TYGEM_ENDGAMEMSG:   
			handleEndgameMsg(msg, size);
			break;
		case TYGEM_TIME:
			handleTime(msg, size);
			break;
		case 0x0681:
			//068101f7020103050340033b028001000000
#ifdef RE_DEBUG
			printf("0x0681: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x0683:	//clock stop? //enter score?
			/* This comes a lot during broadcasted games,  not
			 * certain but could be time update, like stopped time
			 * at the time listed */
			handleScoreMsg1(msg, size);
			break;
		
		// probably 068d requests
		//068e 004d 0000 7065 7465 7269 7573 0000
		//0000 0000 0000 696e 7472 7573 696f 6e00
		//0000 0000 0001
			
		case TYGEM_SERVERPLAYERCOUNTS:	//0653 requests
			//0651 might request this
			//in which case 0651 might not be
			//a disconnect
			handleServerPlayerCounts(msg, size);
			break;
		case 0x068e:
			handleResumeMatch(msg, size);
			break;
		case 0x06c7:
			//this is related to either a failed match create or a bad
			//sent message or maybe something legit
			//00280000696e74727573696f6e00000000000009 396b 0000000000000
			// I think we get these after we send 06c6, so that might
			// be like the creation confirm when its added to the list?
			// anyway, seems legit, I mean just names and ranks here:
			//000c000070657465726975730000000000000008696e74727573696f6e0000000000000900000000
#ifdef RE_DEBUG
			printf("***0x06c7: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		//Two bytestrange: 0692: 
		//0692696e74727573696f6e00000000000009696e74727573696f6e0049020000000000000000000000000000000000000
		//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
		//0000000000000000000000000000002710006f756e742c2073746f74616c5f636f756e742c20737769000000000000000
		//10000000300000004000000056368696e61005f636f756e742c206c746f74616c5f636f0000000000004218b34d6f7573
		//652e676966006f73735f636f756e742c206c7400000000000000000000000000000000000000000000000000000000004
		//8f80619000000000000000000000000000000000000000000000000
		
		//when we're trying to offer
		//706574657269757300000000000000087065746572697573002f490200000000000000000000000000
		//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
		//0000000000000000000000000000000000000000000000002328006f756e742c2073746f74616c5f636f756e74
		//2c2073776900000000000000010000000300000004000000056368696e61005f636f756e742c206c746f74616c
		//5f636f000000000000406c8b4d6f7573652e676966006f73735f636f756e742c206c7400000000000000000000
		//0000000000000000000000000000000000000048fa788200000000000000000000000000000000000000000000
		//0000

		//70657465726975730000000000000008
		//706574657269757300c7920200000015
		//00000011000000040000000000000009
		//0000000800000001000000000000000c
		//00000009000000030000000000000000
		//0000000000000000000000000000000c
		//00000009000000030000000000002457584f4f58584f4f4f4f4f4f4f005f636f756e742c2073776900000000000000010000000300000004000000056368696e61005f636f756e742c206c746f74616c5f636f00031b000000406c8b4d6f7573652e676966006f73735f636f756e742c206c74000000000000004e200000000000004e2000000000000000000000000049f2e7010000000000000000000000000000000000000000000061ea

		
		case 0x0692:
			//possibly related to match opening or negotiation
			//actually, unlikely, we send an 0691 at open
#ifdef RE_DEBUG
			printf("0x0692: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case TYGEM_FRIENDSBLOCKSLIST:
			handleFriendsBlocksList(msg, size);
			break;
		case 0x0698:
			//maybe number of games/players?
#ifdef RE_DEBUG
			printf("0x0698: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x069f:
			handlePersonalChat(msg, size);
			break;
		case 0x06b1:	//a name during game?  //maybe bets? probably bets FIXME
			//0001 0700 b0fc 
			//c3b6 b5bf bdc3 b4eb 0000 0000 0000 0000 01
			//0000 0000 0000 0003
			//e86f 6c64 6572 6100 0000 0000 00
#ifdef RE_DEBUG
			printf("0x06b1: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
		case 0x06af:	//login request response?
#ifdef RE_DEBUG
			printf("0x06af: ");
			for(i = 0; i < (int)size; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			//is this right here?!?!? commenting out for now
			//sendInvitationSettings(false);
			break;
		default:
#ifdef RE_DEBUG
			msg -= 4;	//FIXME
			printf("Two bytestrange: %02x%02x: \n", msg[2], msg[3]);
//#ifdef RE_DEBUG
			for(i = 2; i < (int)size + 4; i++)
				printf("%02x", msg[i]);
			printf("\n");
#endif //RE_DEBUG
			break;
	}
}

/* FIXME shouldn't this do something ?? */
void TygemConnection::handleConnected(unsigned char * , unsigned int )
{
}

#ifdef RE_DEBUG
//#define PLAYERLIST_DEBUG
#endif //RE_DEBUG
//0613
void TygemConnection::handlePlayerList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char name[14];
	QString encoded_name, encoded_name2, rank;
	int players;
	bool no_rank = false;
	Room * room = getDefaultRoom();
	PlayerListing * newPlayer = new PlayerListing();
	PlayerListing * aPlayer;
	newPlayer->online = true;
	newPlayer->info = "??";
	newPlayer->playing = 0;
	newPlayer->observing = 0;
	newPlayer->idletime = "-";
	
	players = (p[0] << 8) + p[1];
	p += 4;

	//make sure these aren't one off, FIXME,
	//i.e., cutting off last record, also check on ORO
	while(p < (msg + size - 0x37) || (p < (msg + size - 19) && p[0] == 0x01))
	{
#ifdef PLAYERLIST_DEBUG
		for(int j = 0; j < 0x38; j++)
			printf("%c", p[j]);
		printf("\n");
		for(int j = 0; j < 0x38; j++)
			printf("%02x", p[j]);
		printf("\n");
#endif //PLAYERLIST_DEBUG
		if(p[0] == 0x01)
		{
			p += 4;
			strncpy((char *)name, (char *)p, 14);
			encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
			aPlayer = room->getPlayerListing(encoded_name);	//in case they are the same
			if(!aPlayer)
				aPlayer = room->getPlayerListingByNotNickname(encoded_name);
			if(aPlayer)
			{
				for(std::vector<unsigned short>::iterator room_listit = aPlayer->room_list.begin();
					room_listit != aPlayer->room_list.end(); room_listit++)
				{
					BoardDispatch * boarddispatch = getIfBoardDispatch(*room_listit);
					if(boarddispatch)
						boarddispatch->recvObserver(aPlayer, false);
				}
				
				
				/*GameData * gameData;
				BoardDispatch * boarddispatch;
				if(playing_game_number && (boarddispatch = getIfBoardDispatch(playing_game_number))
				   && (gameData = boarddispatch->getGameData())
				   && (aPlayer->name == (gameData->black_name == getUsername() ? gameData->white_name : gameData->black_name))
				   && gameData->fullresult == 0)*/
				if(match_negotiation_state->isOngoingMatch() &&
					match_negotiation_state->verifyPlayer(aPlayer))
				{
					/* FIXME this is awkward here, also would be better if
					 * we just had an opponent QString to check.  The issue is that
					 * we already have player_accepted_match and I don't want to have another
					 * weird variable that we're not sure where its set and where its not,
					 * but its DEFINITELY something to think about.  Here, we're looking up the 
					 * boarddispatch for the game we're in EVERY TIME a player leaves the server
					 * come to think of it, we shouldn't even look up the boarddispatch for
					 * the game we're playing ever.  FIXME FIXME But I just want to get things
					 * working right now, but major FIXME.
					 * also basically, we can't disconnect this player because we need their info to
					 * send stuff */
					
					//FIXME nick or username? probably nick but is that ascii or normal?
					//don't delete the playerlisting if we're still in a game with them
					p += 16;
					continue;
				}
				aPlayer->online = false;	
				room->recvPlayerListing(aPlayer);
			}
			else
				printf("Can't find disconnecting player\n");
			p += 14;
#ifdef PLAYERLIST_DEBUG
			printf("Player %s disconnected, last bytes: %02x%02x\n", encoded_name.toLatin1().constData(), p[0], p[1]);
#endif //PLAYERLIST_DEBUG
			p += 2;
			continue;
		}
		p += 2;
		p++;
		if(!p[0])
		{
			//often [Gxxxx] player, I assume guest?
			//0002 0000 5b47 3236 3734 5d32 3000 0000
			//0000 0000 5b47 3236 3734 5d32 3000 0000
			//0000 0100 0000 0000 0000 0000 0000 0000
			//0000 45ab 0001 004b
#ifdef PLAYERLIST_DEBUG
			printf("Guest\n");
#endif //PLAYERLIST_DEBUG
			p += 0x35;
			continue;
		}
		/* I think p[0] is the flag icon, but it can be different
		 * pictures too so... */
		p++;
		strncpy((char *)name, (char *)p, 14);
		encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
		if(p[14])
			printf("FIXME player 14 char:\n\n");
		p += 15;
		//rank byte
		if(p[0] < 0x12)
			rank = QString::number(0x12 - p[0]) + 'k';
		else if(p[0] > 0x1a)
			rank = QString::number(p[0] - 0x1a) + 'p';
		else
			rank = QString::number(p[0] - 0x11) + 'd';
		p++;
		strncpy((char *)name, (char *)p, 11);
		encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
		//another name
		aPlayer = room->getPlayerListing(encoded_name2);
		if(!aPlayer)
			aPlayer = newPlayer;
		aPlayer->notnickname = encoded_name;
		aPlayer->name = encoded_name2;
		aPlayer->rank = rank;
		
		p += 11;
		aPlayer->country_id = p[0];
		p++;
		p += 4;
		// are these unsigned longs or shorts ?!?!?
		p += 2;
		aPlayer->wins = (p[0] << 8) + p[1];
		p += 2;
		p += 2;
		aPlayer->losses = (p[0] << 8) + p[1];
		p += 2;	//w/l
		p++;
		if(p[0])
		{
			//FIXME
			printf("player uses username, not nickname?!?\n");
			/* If this occurs often, ... well its one thing to store
			 * the player on the username, but if it affects the game
			 * listing displays, we have to check the byte on the player...
			 * meaning a separate variable.  I'll see if it happens
			 * a lot or if its an issue */
		}
		p++;
		p += 2;
		if(p[0] == 0xff && p[1] == 0xff)   //normally 0000 ----, then ffff ff40
			no_rank = true;
		p += 2;
		
		if(no_rank)
		{
			aPlayer->rank_score = 0;
			no_rank = false;
		}
		else
			aPlayer->rank_score = (p[0] << 8) + p[1];
		/* player rank scores MUST Be normalized to be filtered in view properly !! */
		//aPlayer->rank_score = rankToScore(aPlayer->rank);
		p += 2;
		//p[0] probably means keyi, can contact with game
		if(p[0])
			aPlayer->info = "X";
		else
			aPlayer->info = "O";
		//p[1]
		//00  
		//01  might mean observing
		//02  probably means in a game
		aPlayer->country = QString::number(p[0], 16) + QString::number(p[1], 16);
		p += 2;
		p += 2;
#ifdef PLAYERLIST_DEBUG
		/* FIXME there's some object code playerlist structure dependency
		 * such that if we alter the playerlisting structure, it can
		 * screw up the lists or something until we make clean and make again */
		printf("%s %s %d/%d\n", name, rank.toLatin1().constData(), aPlayer->wins, aPlayer->losses);
#endif //PLAYERLIST_DEBUG
		room->recvPlayerListing(aPlayer);
	}
	delete newPlayer;
	
	if(p != (msg + size))
	{
		qDebug("handlePlayerListing packet strange size %d", (msg + size) - p);
	}
	
	received_players = true;
	if(received_games && match_negotiation_state->canEnterRematchAdjourned())
		promptResumeMatch();
	return;
}

QString TygemConnection::rating_pointsToRank(unsigned int rp)
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

/* FIXME, I really hate using regular expressions in code that's
 * called this often, but okay for now.  Also, its basically the
 * same everywhere, don't know why I thought it would be different
 * per protocol. 

 * No, there is a problem, the rating points are maybe different and also
 * listed in view on tygem, so they have to be sorted differently.  i.e.
 * this should actually return the real tygem rank scores instead of
 * changing the rating_points or rank score, above to this normalized
 * thing. FIXME*/
unsigned int TygemConnection::rankToScore(QString rank)
{
	QString buffer = rank;
	//buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
	buffer.replace(QRegExp("[pdk]"), "");
	int ordinal = buffer.toInt();
	unsigned int rp;

	/* This is for ranking purposes, these are all kind of off and
	 * tygem doesn't equate rank to points directly I don't think.
	 * might be weighted by win/loss ratio or something. */
	if(rank.contains("k"))
		rp = (19 - ordinal) * 1000;
	else if(rank.contains("d"))
		rp = 18000 + (ordinal * 1000);
	else if(rank.contains("p"))
		rp = 28000 + (ordinal * 1000);	
	else
		return 0;
	
	return rp;
}

//FIXME this was oro code, but tygem has numbers for like 3 countries at least
QString TygemConnection::getCountryFromCode(unsigned char code)
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
			return "AUSTRAILIA";
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

/* FIXME I think this p++, p += n, stuff is a really
 * bad idea now.  I need to get that other protocol
 * idea working or just use the numbers somehow.  It would
 * be better to have msg[n + offset + offset] then what we're
 * doing now */
 /* FIXME, we may be getting this twice, meaning we may not have to request it
  * or possibly we may not have to request it for eweiqi or something. */
void TygemConnection::handleFriendsBlocksList(unsigned char * msg, unsigned int size)
{
	//0018 0696 
	//0001 2066 696e 7472 7573 696f 6e00 6672 6965 6e00
	unsigned char name[15];
	unsigned int i;
	
	QString encoded_name;
	name[15] = 0x00;
	int records = (msg[0] << 8) + msg[1];
	if((records * 16) + 4 != (int)size)
	{
		qDebug("FriendsBlock message of strange size: %d", size);
		//return;
	}
#ifdef RE_DEBUG
	printf("FriendsBlocksList: %d\n", records);
	for(i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	//i = 2;		//2066
	i = 4;
	while(records--)
	{
		strncpy((char *)name, (char *)&(msg[i]), 14);
		encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
		i += 15;
		/* FIXME somehow, its possible for the same name to be on the official
		 * server list more than once */
		if(msg[i] == 0x00)
			friendedList.push_back(new FriendWatchListing(encoded_name, friendwatch_notify_default));
		else if(msg[i] == 0xff)
			blockedList.push_back(new FriendWatchListing(encoded_name, friendwatch_notify_default));
		else
			printf("unknown indicator: %02x\n", msg[i]);
#ifdef RE_DEBUG
		printf("%02x %s %s\n", msg[i], encoded_name.toLatin1().constData(), name);
#endif //RE_DEBUG
		i++;
	}
	setConnected();		//might be a better place for this
}

/* These are the number of players on the other servers
 * FIXME */
//0x0654: 
void TygemConnection::handleServerPlayerCounts(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	int records;
	p += 2;
	records = (p[0] << 8) + p[1];
	p += 2;
#ifdef RE_DEBUG
	//printf("0654 %d unknown records: \n", records);
#endif //RE_DEBUG
	while(p < (msg + size - 3))
	{
#ifdef RE_DEBUG
	//	printf("%02x%02x for %d(%02x%02x) %d\n",
	//	       p[0], p[1], (p[2] << 8) + p[3], p[2], p[3],
	//	       (p[6] << 8) + p[7]);
#endif //RE_DEBUG
		p += 8;
	}
		//0100 000d 
		//79bd 0913 0000 0b3f 
		//79bd 092d 0000 0092 
		//79bd 0914 0000 0909 
		//79bd 0912 0000 001a 
		//79bd 094b 0000 04b5 
		//79bd 092c 0000 01ee 
		//79bd 097d 0000 00c9 
		//79bd 092f 0000 031a 
		//79bd 094c 0000 0302 
		//79bd 0915 0000 0000 
		//79bd 0911 0000 0729 
		//79bd 092e 0000 005c 
		//79bd 0915 0000 0004
}

/* We might FIXME replace the return on this with a QString
 * when we alter the list view to handle different columns.
 * But for now, moves is pretty accurate for the field */
int TygemConnection::getPhase(unsigned char byte)
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

//this was oro but certainly there's tygem flags we don't get
QString TygemConnection::getRoomTag(unsigned char byte)
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

#ifdef RE_DEBUG
#define GAMESLIST_DEBUG
#endif //RE_DEBUG
//0612
void TygemConnection::handleGamesList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * game_record;
	unsigned int number_of_games;
	unsigned int id;
	unsigned int name_length;
	unsigned char name[15];
	unsigned char flags;
	Room * room = getDefaultRoom();
	bool different_game_record = false;
	QString encoded_nameA, encoded_nameB, rankA, rankB;
	QString encoded_nameA2, encoded_nameB2;
	GameListing * aGameListing;
	GameListing * ag = new GameListing();
	
	ag->running = true;
	name[14] = 0x00;
	/*printf("Gamelist message:\n");
	for(i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");*/
	number_of_games = (p[0] << 8) + p[1];
	p += 4;
	while(p < (msg + size - 0x48) || (p < (msg + size - 3) && p[2] == 0x01))
	{
		game_record = p;
		id = (game_record[0] << 8) + game_record[1];
#ifdef GAMESLIST_DEBUG
		printf("Id: %d:\n", id);
#endif //GAMESLIST_DEBUG
		aGameListing = room->getGameListing(id);
		if(!aGameListing)
			aGameListing = ag;
		aGameListing->number = id;
		if(game_record[2] == 0x01)
		{

#ifdef GAMESLIST_DEBUG
			printf("Id: %d:\n", id);
			printf("is ended!!!\n");
			for(unsigned int i = 0; i < 4; i++)
				printf("%c", p[i]);
			printf("\n");
#endif //GAMESLIST_DEBUG

			p += 4;
			aGameListing->running = false;
			room->recvGameListing(aGameListing);
			continue;
		}
		//p[4] == 0x02 is some other kind of record
#ifdef GAMESLIST_DEBUG
		for(unsigned int i = 0; i < 0x4c; i++)
		{
			printf("%c", p[i]);	
		}
		printf("\n");
		for(unsigned int i = 0; i < 0x4c; i++)
		{
			printf("%02x", p[i]);	
		}
		printf("\n");
#endif //GAMESLIST_DEBUG
		p += 2;			//past id
		//0000 0001 0002 0000 ff00 7978 7973 7a00 
		p++;
		//if(p[0] == 0x00)
			//p += 2;
		p++;
		
		p++;
		//impacts time
		//there's also 0x30, I think 0x50, etc.. issues here
		aGameListing->white_first_flag = p[0] & 0x01;
		aGameListing->FR = "";
		//aGameListing->FR += QString::number(p[0], 16) + " ";
		/* 40 looks like a lit 2 computer screens, 50 like 2 dark computers */
		/* 30 is just betting and over*/
		/* Thinking 40 bit is broadcast and 20 bit is betting */
		p++;
		aGameListing->observers = (game_record[6] << 8) + game_record[7];
		//We have an issue here I think FIXME with observer numbers
		p += 2;
		int some_number = (p[0] << 8) + p[1];
		/* This number seems to be either 0 or -1 */
		if(some_number != 0)
		{
			different_game_record = true;
		}
		p += 2;
		p++;
		//aGameListing->FR = "";
		//p += 2;
		flags = p[0];
		
		//FIXME we need a flag parse function		
		//but note that we might want a variable return value in the sense
		//that maybe we want to get the flags, maybe we want to set the name of the black
		//player to ** review ** or something...
		if(flags & 0x80)
		{
			// Betting
			aGameListing->FR += "B";
		}
		if(flags & 0x20)
		{
			//probably broadcast, maybe as well as betting
			aGameListing->FR += "R";
		}
		//ff00
		p++;

		//name 1
		strncpy((char *)name, (char *)p, 14);
		encoded_nameA = serverCodec->toUnicode((char *)name, strlen((char *)name));
		p += 15;
		//rank byte
		if(p[0] < 0x12)
			rankA = QString::number(0x12 - p[0]) + 'k';
		else if(p[0] > 0x1a)
			rankA = QString::number(p[0] - 0x1a) + 'p';
		else
			rankA = QString::number(p[0] - 0x11) + 'd';
		p++;
		strncpy((char *)name, (char *)p, 11);
#ifdef GAMESLIST_DEBUG
		printf("%s vs.", name);
#endif //GAMESLIST_DEBUG
		encoded_nameA2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
		p += 12;
		if(p[0] == 0x00)
		{
			p += 16;
			encoded_nameB = QString();
			encoded_nameB2 = QString();
			rankB = QString();
		}
		else
		{
			strncpy((char *)name, (char *)p, 14);
			encoded_nameB = serverCodec->toUnicode((char *)name, strlen((char *)name));
			p += 15;
			if(p[0] < 0x12)
				rankB = QString::number(0x12 - p[0]) + 'k';
			else if(p[0] > 0x1a)
				rankB = QString::number(p[0] - 0x1a) + 'p';
			else
				rankB = QString::number(p[0] - 0x11) + 'd';
			p++;
			strncpy((char *)name, (char *)p, 11);		//chinese
#ifdef GAMESLIST_DEBUG
			printf(" %s\n", name);
#endif //GAMESLIST_DEBUG
			encoded_nameB2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
		}
		p += 12;
		
		p++;
		//this p[0] should be game status
		//aGameListing->FR += QString::number(p[0], 16);
		switch(game_record[73])
		{
			case 0:
				aGameListing->FR += "?";	//looking
				break;
			case 1:
				aGameListing->moves = 0;
				break;
			case 2:
				aGameListing->moves = 50;
				break;
			case 3:
				aGameListing->moves = 120;
				break;
			case 4:
				aGameListing->moves = (unsigned)-1;	//over
				break;
			case 5:
				aGameListing->FR += "V";	//review
				break;
			case 6:
				break;
		}
		p++;
		p++;
		//name_length = p[0];
		p++;
		//p += name_length;
		//2 bytes here, 0001 or 0004 or 0006 or 0003, etc..
		p += 2;		
		name_length = (p[0] << 8) + p[1];
		//if(different_game_record)
		//apparently has nothing to do with earlier,
		//might be another flag FIXME
		if(1)
		{
			p += name_length;
		}
		p += 2;
		/* Don't remember why this can't get the info from the players? FIXME*/
		if(aGameListing->white_first_flag)
		{
			aGameListing->_white_name = encoded_nameA2;
			aGameListing->_white_rank = rankA;
			aGameListing->_white_rank_score = rankToScore(rankA);
			aGameListing->_black_name = encoded_nameB2;
			aGameListing->_black_rank = rankB;
			aGameListing->_black_rank_score = rankToScore(rankB);
		}
		else
		{
			aGameListing->_white_name = encoded_nameB2;
			aGameListing->_white_rank = rankB;
			aGameListing->_white_rank_score = rankToScore(rankB);
			aGameListing->_black_name = encoded_nameA2;
			aGameListing->_black_rank = rankA;
			aGameListing->_black_rank_score = rankToScore(rankA);
		}
		room->recvGameListing(aGameListing);
		if(initial_connect && match_negotiation_state->newMatchAllowed())
		{
			if(encoded_nameA2 == getUsername())
				match_negotiation_state->setupRematchAdjourned(id, encoded_nameB2);
			else if(encoded_nameB2 == getUsername())
				match_negotiation_state->setupRematchAdjourned(id, encoded_nameA2);
		}
	}
	delete ag;
	initial_connect = false;
	received_games = true;
	if(received_players && match_negotiation_state->canEnterRematchAdjourned())
		promptResumeMatch();
}

//shouldn't this be somewhere more standard?  FIXME
void TygemConnection::promptResumeMatch(void)
{
	unsigned short game_number = match_negotiation_state->getGameId();
	GameListing * gameListing = getDefaultRoom()->getGameListing(game_number);
	if(!gameListing)
	{
		qDebug("promptResumeMatch can't get game listing");
		return;
	}
	QString opp_name = (gameListing->white_name() == getUsername() ? gameListing->black_name() : gameListing->white_name());
	QMessageBox mb(tr("Resume match?"),
		        QString(tr("Resume match in progress with %1?\n").arg(opp_name)),
			QMessageBox::Question,
	  		QMessageBox::Yes | QMessageBox::Default,
   			QMessageBox::No | QMessageBox::Escape,
   			QMessageBox::NoButton);
	mb.raise();
//	qgo->playPassSound();

	if (mb.exec() == QMessageBox::Yes)
	{
		qDebug("Trying to rejoin %d", game_number);
		sendJoin(game_number);
		sendObserversRequest(game_number);
		sendResume(game_number);
	}
}

void TygemConnection::handleServerAnnouncement(unsigned char * msg, unsigned int size)
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

void TygemConnection::handleServerRoomChat(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * text = new unsigned char[size - 0x23];
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name, encoded_name2;
#ifdef RE_DEBUG
	printf("serverchat: \n");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	p += 4;
	strncpy((char *)name, (char *)p, 14); 
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 15;
	strncpy((char *)name, (char *)p, 11); 
	//we lost first three letters here last I checked
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 0x11;
	//don't know what that one byte is, but its not part of text
	p++;
	strncpy((char *)text, (char *)p, size - 0x24);

	//Room * room = getDefaultRoom(); //3
	//PlayerListing * player = room->getPlayerListing(player_id);
	//if(player)
	//{
#ifdef RE_DEBUG
		printf("%s says: \n", name);
		for(int i = 0; i < (int)size - 0x24; i++)
			printf("%02x", text[i]);
		printf("\n");
#endif //RE_DEBUG
		QString u;
		
		u = serverCodec->toUnicode((const char *)text, strlen((char *)text));
			//u = codec->toUnicode(b, size - 4);
		if(console_dispatch)
			console_dispatch->recvText(encoded_name2 + ": " + u);
	/*}
	else
		printf("unknown player says something");*/
	delete[] text;
}

//has garbled stuff on the end, maybe due to unicode, still FIXME
//0661
void TygemConnection::handleGameChat(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * text;
	unsigned char name[15];
	name[14] = 0x00;
	BoardDispatch * boarddispatch;
	int room_number;
	unsigned int size_of_message;
	QString encoded_name, encoded_name2;
	QString rank;
#ifdef RE_DEBUG
	int i;

	printf("** game chat size: %d: ", size);
	for(i = 0; i < (int)size; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	//Room * room = getDefaultRoom();
	room_number = (p[0] << 8) + p[1];
	boarddispatch = getIfBoardDispatch(room_number);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for chat msg", room_number);
		return;
	}
	p += 0x14;
	strncpy((char *)name, (char *)p, 14); 
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 15;
	if(p[0] < 0x12)
		rank = QString::number(0x12 - p[0]) + 'k';
	else if(p[0] > 0x1a)
		rank = QString::number(p[0] - 0x1a) + 'p';
	else
		rank = QString::number(p[0] - 0x11) + 'd';
	p++;
	strncpy((char *)name, (char *)p, 11); 
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	if(encoded_name2 == getUsername())
	{
		//block as echo
		return;
	}
	p += 0x1c;
	size_of_message = p[0];
	if(size_of_message > size - 66)
	{
		qDebug("Bad in game chat message size");
		return;
	}
	
	p += 4;
	//again, init number not part of text
	p++;
	text = new unsigned char[size_of_message + 1];
	strncpy((char *)text, (char *)p, size_of_message);
	text[size_of_message] = 0x00;
	//PlayerListing * player = room->getPlayerListing(player_id);
	//if(player)
	//{
	
	QString u;
		
	u = serverCodec->toUnicode((const char *)text, strlen((char *)text));
			//u = codec->toUnicode(b, size - 4);
	boarddispatch->recvKibitz(encoded_name2 + "[" + rank + "]", u);
	/*}
	else
	printf("unknown player says something");*/
	delete[] text;
}

void TygemConnection::handleOpenConversation(unsigned char * msg, unsigned int size)
{
	/* We're not going to bother negotiating this, get the player
	 * and send a reply */
	PlayerListing * player;
	QString encoded_name, encoded_name2;
	QString rank;
	unsigned char name[15];
	name[14] = 0x00;
	
	if(size < 30)
	{
		qDebug("Bad conversation open size: %d", size);
		return;
	}
	
	strncpy((char *)name, (char *)msg, 14); 
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	if(msg[15] < 0x12)
		rank = QString::number(0x12 - msg[15]) + 'k';
	else if(msg[15] > 0x1a)
		rank = QString::number(msg[15] - 0x1a) + 'p';
	else
		rank = QString::number(msg[15] - 0x11) + 'd';
	strncpy((char *)name, (char *)&(msg[16]), 11); 
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	
	player = getDefaultRoom()->getPlayerListing(encoded_name2);
	if(!player)
	{
		qDebug("Unknown player asks for conversation: %s", encoded_name2.toLatin1().constData());
		return;
	}
	sendConversationReply(*player, accept);
	Talk * t = getIfTalk(*player);
	if(!t)
		t = getTalk(*player);
	t->setConversationOpened(true);
	t->updatePlayerListing();
}

void TygemConnection::handleConversationReply(unsigned char * msg, unsigned int size)
{
	PlayerListing * player;
	QString encoded_name, encoded_name2;
	QString rank;
	unsigned char name[15];
	name[14] = 0x00;
	if(size < 63)
	{
		qDebug("Bad conversation reply size: %d", size);
		return;
	}
	
	strncpy((char *)name, (char *)msg, 14); 
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	if(msg[15] < 0x12)
		rank = QString::number(0x12 - msg[15]) + 'k';
	else if(msg[15] > 0x1a)
		rank = QString::number(msg[15] - 0x1a) + 'p';
	else
		rank = QString::number(msg[15] - 0x11) + 'd';
	strncpy((char *)name, (char *)&(msg[16]), 11); 
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	
	player = getDefaultRoom()->getPlayerListing(encoded_name2);
	if(!player)
	{
		qDebug("Unknown player replies to conversation: %s", encoded_name2.toLatin1().constData());
		return;
	}
	Talk * t = getIfTalk(*player);
	if(!t)
	{
		qDebug("No talk for: %s", encoded_name2.toLatin1().constData());
		return;
	}
	
	if(msg[62] == 0x01)
	{
		t->setConversationOpened(true);
	}
	else if(msg[62] == 0x00)
	{
		t->recvTalk(QString("** Declines conversation **"));
	}
	else
	{
#ifdef RE_DEBUG
		printf("Conversation reply msg 62 = %02x\n", msg[62]);
#endif //RE_DEBUG
	}
	
	if(pendingConversationMsg.find(player) != pendingConversationMsg.end())
	{
		sendConversationMsg(*player, pendingConversationMsg[player].toLatin1().constData());
		pendingConversationMsg.erase(player);	
	}
}

void TygemConnection::handleConversationMsg(unsigned char * msg, unsigned int size)
{
	PlayerListing * player;
	QString encoded_name, encoded_name2;
	QString rank;
	unsigned char name[15];
	int size_of_message;
	int nickname_len;
	bool closing = false;
	char * text;
	name[14] = 0x00;
	if(size < 61)
	{
		qDebug("Received bad conversation msg");
		return;
	}
#ifdef RE_DEBUG
	printf("Conversation msg:\n");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x ", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	strncpy((char *)name, (char *)msg, 14); 
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	if(msg[15] < 0x12)
		rank = QString::number(0x12 - msg[15]) + 'k';
	else if(msg[15] > 0x1a)
		rank = QString::number(msg[15] - 0x1a) + 'p';
	else
		rank = QString::number(msg[15] - 0x11) + 'd';
	strncpy((char *)name, (char *)&(msg[16]), 11); 
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	nickname_len = strlen((char *)name);
	//FIXME 255 chat size limit ?!?!
	size_of_message = msg[60];
	unsigned int adjusted_size = size_of_message;
	if(size_of_message % 4 == 0)
		adjusted_size += 4;
	else
	{
		while(adjusted_size % 4 != 0)
			adjusted_size++;
	}
	if(size != adjusted_size + 60 + 4)
	{
		//76 72
		qDebug("Bad conversation message size %d %d", size, adjusted_size + 64);
		return;
	}
	
	player = getDefaultRoom()->getPlayerListing(encoded_name2);
	if(!player)
	{
		qDebug("Unknown player replies to conversation: %s", encoded_name2.toLatin1().constData());
		return;
	}
	Talk * t = getIfTalk(*player);
	if(!t)
	{
		qDebug("Unknown player replies to conversation: %s", encoded_name2.toLatin1().constData());
		return;
	}
	
	if(msg[62] == 0x04)
	{
		//FIXME  do we need to do anything else?
		qDebug("Conversation closing by other person");
		closing = true;
	}
	else if(msg[62] != 0x05)
	{
#ifdef RE_DEBUG
		printf("Conversation msg 62 = %02x\n", msg[62]);
#endif //RE_DEBUG
	}
	
	text = new char[size_of_message + 1];
	strncpy(text, (char *)&(msg[64]), size_of_message);
	
	text[size_of_message] = 0x00;		//doublecheck FIXME
	
	QString u;
		
	if(closing)
	{
		u = serverCodec->toUnicode((const char *)&(text[nickname_len + 2]), strlen((char *)&(text[nickname_len + 2])));
		u = "** " + u + " **";
	}
	else
		u = serverCodec->toUnicode((const char *)text, strlen((char *)text));
	t->recvTalk(u);
	if(closing)
		t->setConversationOpened(false);
	delete[] text;
}

/* I think these are actually supposed to be little email type msgs even though
 * they're small and conversations are supposed to be held over
 * something else that we have to respond to to open up */
void TygemConnection::handlePersonalChat(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char * text = new unsigned char[size - 7];
	unsigned char name[15];
	name[14] = 0x00;
	unsigned int subject_size, msg_size;
	QString our_name, encoded_name, encoded_name2, rank, subject;
#ifdef RE_DEBUG
	printf("** msg size: %d: ", size);
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	
	strncpy((char *)name, (char *)p, 14);
	our_name = QString((char *)name);
	p += 15;
	p++;
	strncpy((char *)name, (char *)p, 14);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 15;
	
	//rank byte
	if(p[0] < 0x12)
		rank = QString::number(0x12 - p[0]) + 'k';
	else if(p[0] > 0x1a)
		rank = QString::number(p[0] - 0x1a) + 'p';
	else
		rank = QString::number(p[0] - 0x11) + 'd';
	p++;		
	strncpy((char *)name, (char *)p, 11);
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 12;
	p++;
	subject_size = p[0];
	p++;
	printf("Subjectsize: %d\n", subject_size);
	if(subject_size > (size - 47))
	{
		qDebug("Bad message subject size: %d", subject_size);
		delete[] text;
		return;
	}
	strncpy((char *)text, (char *)p, subject_size);
	subject = QString((char *)text);
	p += subject_size;
	msg_size = (p[0] << 8) + p[1];		//tygem caps at 1024
	p += 2;
	printf("msg_size: %d\n", msg_size);
	if(msg_size > (size - (47 + subject_size + 2)))
	{
		qDebug("Bad message msg size: %d", msg_size);
		delete[] text;
		return;
	}
	strncpy((char *)text, (char *)p, msg_size + 1);
	
	Room * room = getDefaultRoom();
	PlayerListing * player = room->getPlayerListing(encoded_name2);
	
	if(player)
	{
#ifdef RE_DEBUG
		printf("%s says to you ", player->name.toLatin1().constData());
		for(unsigned int i = 0; i < size - 8; i++)
			printf("%02x", text[i]);
		printf("\n");
#endif //RE_DEBUG
		QString u;
		
		u = serverCodec->toUnicode((const char *)text, strlen((char *)text));
		Talk * talk = getTalk(*player);
		if(talk)
		{
			talk->recvTalk(u);
			talk->updatePlayerListing();
		}
		//FIXME highlight or something
		//if(console_dispatch)
		//	console_dispatch->recvText(player->name + "-> " + QString((char *)text));
	}
	else
		printf("unknown player says something");
	delete[] text;
}

//0666
void TygemConnection::handleObserverList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;

	unsigned int game_number;
	unsigned short number_of_observers;
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	PlayerListing * aPlayer;
	//PlayerListing * newPlayer = new PlayerListing();
	QString encoded_name, encoded_name2, rank;
	int country_size;
	unsigned char name[20];		//country can be larger than 11
	game_number = (p[0] << 8) + p[1];
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("no board dispatch for observer list");
		return;
	}
	
	p += 2;
	number_of_observers = (p[0] << 8) + p[1];
	p += 2;
#ifdef RE_DEBUG
	printf("observers %d:\n", number_of_observers);
	for(unsigned int i = 0; i < size - 4; i++)
		printf("%02x", p[i]);
	printf("\n");
#endif //RE_DEBUG
	/* FIXME possible issue with country name which is not included in
	 * 0x34 */
	
	while(p < (msg + size - 0x33) || (p[0] == 0x01 && p < (msg + size - 19)))
	{
		if(p[0] == 0x01)
		{
			//0131 1140 b9d9 b6f7 b3ad c1f8 c0cc 0000 0000 0013 c900 0000

			
			//0100 0000 696e 7472 7573 696f 6e00 0000 0000 0009
			/* FIXME FIXME FIXME
			 Okay.  A player can disconnect and we can get that message
			before we get their message about leaving the room they're in.
			Solutions: 
			-either we hold disconnecting players as pending for
			say, 3 seconds or something, awkward but might be effective and
			simple.
			-or we have a room count on each player listing that has the
			 the number of rooms we know that player to be in, when we leave
			 a room, we have to decrement each of the players we had listed
			 in that room, when we enter, we have to increment, when an
			 observer message comes in, same thing, decrement or increment
			 then we don't delete a player listing even after they've
			 disconnected until their room count is zero.  Then I guess
			 we check observer disconnects and if such a player goes to
			 0 and... see they also have to have a disconnect flag, okay,
			 that's out.
		 	-we could have a list of the rooms each player is known to be
			 in and then notify each when they disconnect...
			
			observer listings are pointers so unless we want to make
			them copies or smart pointers, we need to update them or
			something... a list of rooms or, etc.. 
			I think cyberoro has these same kinds of issues and I guess
			we sort of found away around it... or no, game listings have
			these issues...
			we need a solution that's easy and clean to implement, has little
			space or speed overhead, and makes sense.
		
			It was unrealistic to trust the protocol about such things
			anyway.  I wonder how they do it.  
			
			Smart pointers still seem like an extra sort of out of band 
			overhead.  room lists on the players seem best... there's
			few players we know of in rooms...
			
			You know though, I'd like to see cyberoro observers before I make
			a decision on this...
			
			
			*/
			p += 4;
			strncpy((char *)name, (char *)p, 14);
			encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
			aPlayer = room->getPlayerListing(encoded_name);
			if(!aPlayer)
				aPlayer = room->getPlayerListingByNotNickname(encoded_name);
			if(aPlayer)
			{
				for(std::vector<unsigned short>::iterator room_listit = aPlayer->room_list.begin();
					room_listit != aPlayer->room_list.end(); room_listit++)
				{
					if(*room_listit == game_number)
					{
						aPlayer->room_list.erase(room_listit);
						break;
					}
				}
				boarddispatch->recvObserver(aPlayer, false);
				if(match_negotiation_state->isOngoingMatch(game_number))
				{
					GameData * gameData = boarddispatch->getGameData();
					//FIXME nick or username? probably nick but is that ascii or normal?
					if(aPlayer->name == (gameData->black_name == getUsername() ? gameData->white_name : gameData->black_name))
					{
						if(gameData->fullresult == 0)
						{
							qDebug("opponent left game!!");
							sendOpponentDisconnect(game_number, opponent_disconnect);
							sendLongOpponentDisconnect(game_number);
							
							opponentDisconnectTimerID = startTimer(1000);
							/* FIXME may or may not be the right place
							 * for this, we could get another message or
							 * perhaps when we haven't received time
							 * from opponent for 4 or 5 seconds.
							 * Right now, I'll assume that's time it takes
							 * server to pass "observer left" msg */
							match_negotiation_state->opponentDisconnect();
						}
					}
				}
			}
			else
				printf("*** Can't find disconnecting player\n");
			p += 15;
			printf("Player %s leaving room, last byte: %02x\n", encoded_name.toLatin1().constData(), p[0]);
			p++;
			printf("Found leaving observer\n");
			continue;
		}
		if(p[1] == 0x39)
		{
			p += 4;
			printf("observer, what the hell is this\n");
			continue;
		}
		else if(p[1] != 0x02)
		{
			printf("%02x%02x\n", p[0], p[1]);
		}
#ifdef RE_DEBUG
		for(int i = 0; i < 0x34; i++)
			printf("%c", p[i]);
		printf("\n");
#endif //RE_DEBUG
		p += 2;
		p++;
		if(!p[0])
		{
			printf("Observer Guest\n");
			//0002 0000 5b47 3337 3039 5d32 3000 0000
			//0000 0000 5b47 3337 3039 5d32 3000 0000
			//0000 0100 0000 0000 0000 0000 0000 0000
			//0000 0000
			/* FIXME this looks a little funny.  A room with
			 * multiple observers that's empty because we don't
			 * add guests to the list... at the same time,
			 * they'd have to be special player listings..
			 * maybe just duplicates but still... */
			p += 48;
			p += p[0] + 1;
			continue;
		}
		
		/* I think p[0] is the flag icon, but it can be different
		* pictures too so... */
		p++;
		strncpy((char *)name, (char *)p, 14);
		//encoded_name = QString((char *)name);
		/* WARNING There's a chance that encoded names are not one
		 * to one and since the names are used to look players up,
		 * doing toUnicode could write to the wrong player.
		 * possible FIXME*/
		encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
		p += 14;
		p++;
		//rank byte
		if(p[0] < 0x12)
			rank = QString::number(0x12 - p[0]) + 'k';
		else if(p[0] > 0x1a)
			rank = QString::number(p[0] - 0x1a) + 'p';
		else
			rank = QString::number(p[0] - 0x11) + 'd';
		p++;
		strncpy((char *)name, (char *)p, 11);
		encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
		//another name
		aPlayer = room->getPlayerListing(encoded_name2);
		if(!aPlayer)
		{
			//aPlayer = newPlayer;
			printf("Can't find observer: %s in full list\n", encoded_name2.toLatin1().constData());
			p += 31;
			p += p[0] + 1;
			continue;
		}
		//unnecessary to set the name since we can't get
		//record without it... probably nickname as well
		//FIXME, but if we didn't need player lists... 
		//technically we should be able to display anyway...
		//aPlayer->name = encoded_name;
		//aPlayer->nickname = nickname;
		//aPlayer->rank = rank;
		p += 8;
		p += 4;
		p += 16;
		p += 2;
		p++;
		
		/* Below is player listing packet */
		//0002 009e 3737 3435 3532 0000 0000 0000 
		//0000 0000 3737 3435 3532 006f 6f6e 0002
		//0000 0000 0000 0064 0000 012b 0000 0000
		//ffff ff40 0001 004b
		/****/
		/* And observer listing */
		//0002 009e 7065 7465 7269 7573 0000 0000
		//0000 0008 7065 7465 7269 7573 0000 0002
		//0000 0000 0000 0000 0000 0000 0000 0000
		//0000 0005 6368 696e 61
		
		country_size = p[0];
		p++;
		if(p > (msg + size - country_size))
		{
			qDebug("Observer list packet overrun, discarding last record!");
			return;
		}
		if(country_size > 19)
		{
			qDebug("Strange country name size greater than hard coded array!!!");
			p += country_size;
			continue;	
		}
		strncpy((char *)name, (char *)p, country_size);
		name[country_size] = '\0';
		p += country_size;
		aPlayer->country = QString((char *)name);
		/*recvObserver does not make a copy at any point.
		 *its supposed that the player listing comes from
		 *the full listing and will be handled with that.*/
		
		/* Also notice that below is a problem if someone leaves
		 * and rejoins a game */
		aPlayer->room_list.push_back(game_number);
		boarddispatch->recvObserver(aPlayer, true);
		if(match_negotiation_state->isOurGame(game_number))
		{
			if(match_negotiation_state->justCreatedRoom() && match_negotiation_state->getPlayerListing() == aPlayer)
			{
				/* They've just joined our created room
				 * so we send the rest of this.  Presuming
				 * its not a resume... FIXME */
				/* This might be a resume or something weird
				 * I'm not seeing MatchMsg1 anymore !!! 
				 * just they join, we pop up the offer dialog
				 * and then we send it !!! FIXME*/
#ifdef FIXME			//necessary when?
				sendMatchMsg1(*aPlayer, game_number);
#endif //FIXME
				MatchRequest * mr = new MatchRequest();
				mr->number = game_number;
				mr->opponent = aPlayer->name;
				mr->opponent_is_challenger = false;
				/* FIXME this is same as "our_invitation" I think
				 * in game data, they should be the same name
				 * FIXME FIXME FIXME */
				mr->first_offer = true;
				mr->timeSystem = byoyomi;
				
				GameDialog * gd = getGameDialog(*aPlayer);
				gd->recvRequest(mr, getGameDialogFlags());
				delete mr;
			}
			else
			{
				GameData * gameData = boarddispatch->getGameData();
				//FIXME nick or username? probably nick but is that ascii or normal?
				if(aPlayer->name == (gameData->black_name == getUsername() ? gameData->white_name : gameData->black_name))
				{
					if(opponentDisconnectTimerID)
					{
						/* FIXME, quite likely we'll need to
						* send other things here or perhaps
						* we even get another message to
						* trigger this, not this one */
						killTimer(opponentDisconnectTimerID);
						opponentDisconnectTimerID = 0;
						qDebug("Opponent rejoined after leaving");
						/* I think these are probably sent in response to something maybe...
						 * we shouldn't just send them like this.  We need to do it
						 * one at a time like match negotiation FIXME */
						sendEndgameMsg(gameData, opponent_reconnects);
						sendOpponentDisconnect(game_number, opponent_reconnect);
						MoveRecord * aMove = new MoveRecord();
						aMove->flags = MoveRecord::UNREMOVE;
						sendMove(game_number, aMove);
						delete aMove;
						sendResumeFromDisconnect(game_number);
						match_negotiation_state->opponentReconnect();
					}
				}
			}
		}
	}
	if(p != (msg + size))
	{
		qDebug("handleObserverList strange size: %d(%d)", (msg + size) - p, size);
	}
	
	//delete newPlayer;
}

/* I think we get these when we try to observe broadcasted, or betting games
 * as well. 
 * oro FIXME, but tygem may have betting in general*/
void TygemConnection::handleBettingMatchStart(unsigned char * , unsigned int )
{
}

/* These are simple, sent by the loser I think 
 * note that sendMatchResult sends them which is awkward
 * because of "match" versus "game" FIXME */
//0670
void TygemConnection::handleGameResult2(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	GameData * gameData;
	//0670 
	//0003 0001 6269 7473 0000 0000 0000 0000
	//0000 0014 6269 7473 0000 0000 0000 0002
	//0000 0000 0000 0000
	if(size != 0x28)
	{
		qDebug("Game result 2 msg of strange size: %d", size);
	}
	char name[15];
	strncpy((char *)name, (char *)&(msg[4]), 14);
	
	strncpy((char *)name, (char *)&(msg[20]), 11);
	if(getUsername() != serverCodec->toUnicode((char *)name, strlen((char *)name)))
	{
#ifdef RE_DEBUG
		//FIXME note that if this happens a lot
		//we might need to handle it in observed games
		//actually, we get our own message back so... 
		printf("0670 not addressed to us\n");
#endif //RE_DEBUG
		return;
	}
	boarddispatch = getIfBoardDispatch((msg[0] << 8) + msg[1]);
	if(!boarddispatch)
	{
		qDebug("no board dispatch for %d", (msg[0] << 8) + msg[1]);
		return;
	}
	gameData = boarddispatch->getGameData();
	//msg[2] is likely color
#ifdef RE_DEBUG
	if(msg[2])
		boarddispatch->recvKibitz(0, "0x670 (resign msg? 01)");
	else
		boarddispatch->recvKibitz(0, "0x670 (resign msg? 00)");
	printf("0x0670: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
		//peterius resigns, peterius is black and not invite:
			//also, we use white_first_flag which might look
			//the same as the invite flag...
			//0002 0101 696e 7472 7573 696f 6e00 0000
			//0000 0009 696e 7472 7573 696f 6e00 0002
			//0000 0000 0000 0000
			//I think we need to respond to this with 0672
			//otherwise no result posted
			//sendLongMatchResult FIXME but only if this is
			//from opp
			//note also that this is very likely the resign
			//message and we should set result as such REGARDLESS
			//of being in score mode or not, we should switch out if it
			//if we are
	GameResult aGameResult;
	aGameResult.result = GameResult::RESIGN;
	aGameResult.winner_name = getUsername();
	//FIXME, we should be able to use
	//color byte above, msg[2] or whatever
	if(gameData->white_name == getUsername())
	{
		aGameResult.winner_color = stoneWhite;
		aGameResult.loser_name = gameData->black_name;
	}
	else
	{
		aGameResult.winner_color = stoneBlack;
		aGameResult.loser_name = gameData->white_name;
	}
	boarddispatch->recvResult(&aGameResult);
	sendResult(gameData, &aGameResult);		
}

//0672
void TygemConnection::handleGameResult(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	unsigned short game_number = (p[0] << 8) + p[1];
	bool white_wins2;	//FIXME with white_wins below
	bool white_loses_on_time = false, black_loses_on_time = false;
	unsigned int moves;
	unsigned int margin;
	unsigned char victory_condition_code;
	if(size != 0x24)
	{
		qDebug("Game result of strange size: %d", size);
	}
	//this is sent if we join an ended game as well
	//must be result where 0671 is match opened
	//0670 is sent if, for instance, resign
	//while we're watching it, but not after over
	
	//but 0670 can be sent without 0672 apparently
	//during game, but again, we would get 0672 alone
	//if it was after game
	//lastly, looks like 0672 might happen without SUR?
	//maybe only when adjourned or something?
	//but we definitely need to handle it and we might
	//want to have it send result (i.e., W+T, etc. ) FIXME
	
	//white has 4 seconds wins by R
	//0002 0001 d6ee b8f0 d0a1 bba8 0000 0000 
	//0000 001a 0400 0000 0076 0000 0204 00a0
	//021c 0060

	//intrusion gets this, presumably after intrusion
	//sends a timeloss prematurely due to some 0 period count error...
	//FIXME
 	//038d0001696e74727573696f6e000000000000090900000000010000000000a0011d0060

	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Can't get board for game number %d", game_number);
		return;
	}
	GameData * gd = boarddispatch->getGameData();
	p += 2;
	white_wins2 = p[0] ^ gd->white_first_flag;	//NOT VALID as is
#ifdef RE_DEBUG
	printf("0672 %02x vs %02x!!", p[0], gd->our_invitation);
#endif //RE_DEBUG
	/* FIXME double check this, but as is, it seems in our games, this is addressed
	 * to us and this is not the winner name, winner rank, but just the usual msg
	 * header, doublecheck against other games */

	//00 00 their invitation, they win
	//01 00 their invitation, we win
	p += 2;
	//winner name		//probably not, dest
	p += 14;
	//winner rank		//probably not, dest
	p += 2;
	//0300 0000 0115 0000
	victory_condition_code = p[0];
	//03		B + R
	//04		W + R
	//00		W + X
	//07		B + T
	//0000 0956 002e 0000 
	p += 2;
	margin = (p[0] << 8) + p[1];
	p += 2;
	moves = (p[0] << 8) + p[1];
	p += 2;
	
	p += 2;
	//times
	
	enum TimeFlags f = handleTimeChunk(boarddispatch, p, gd->white_first_flag);
	/* FIXME unreliable !! not necessary */
	if(f == BLACK_LOSES)
		black_loses_on_time = true;
	else if(f == WHITE_LOSES)
		white_loses_on_time = true;
	
#ifdef RE_DEBUG
	//B + R: white_wins2 = 1 here, FIXME
	if(boarddispatch)
		boarddispatch->recvKibitz(0, "0x672 " + QString::number(white_wins2));
	
	printf("***** 0x0672: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	//FIXME not necessary:
	GameResult aGameResult;
	if(white_loses_on_time || black_loses_on_time)
	{
		aGameResult.result = GameResult::TIME;
		aGameResult.game_number = game_number;
		if(white_loses_on_time)
		{
			aGameResult.loser_name = gd->white_name;
			aGameResult.winner_name = gd->black_name;
			aGameResult.winner_color = stoneBlack;
		}
		else
		{
			aGameResult.loser_name = gd->black_name;
			aGameResult.winner_name = gd->white_name;
			aGameResult.winner_color = stoneWhite;
		}
		boarddispatch->recvResult(&aGameResult);
		return;
	}
	
	//awkward, redundant with white_loses_on_time, black_loses_on_time above
	bool white_wins = false, black_wins = false;
	switch(victory_condition_code)
	{
		case 0x00:
			aGameResult.result = GameResult::SCORE;
			black_wins = true;
			aGameResult.winner_score = (float)margin/10.0;
			aGameResult.loser_score = 0;
			break;
		case 0x01:
			aGameResult.result = GameResult::SCORE;
			white_wins = true;
			aGameResult.winner_score = (float)margin/10.0;
			aGameResult.loser_score = 0;
			break;
		case 0x02:
			aGameResult.result = GameResult::DRAW;
			/* Don't know whether we get this or what */
			aGameResult.winner_color = stoneNone;
			boarddispatch->recvResult(&aGameResult);
			return;		//awkward
			break;
		case 0x03:
			aGameResult.result = GameResult::RESIGN;
			black_wins = true;		
			break;
		case 0x04:
			aGameResult.result = GameResult::RESIGN;
			white_wins = true;	
			break;
		case 0x05:
			aGameResult.result = GameResult::FORFEIT;
			black_wins = true;		
			break;
		case 0x06:
			aGameResult.result = GameResult::FORFEIT;
			white_wins = true;	
			break;
		case 0x07:
			aGameResult.result = GameResult::TIME;
			black_wins = true;	
			break;
		case 0x08:
			aGameResult.result = GameResult::TIME;
			white_wins = true;	
			break;
		/* FIXME doublecheck these, I think we get these
		 * if its an overwhelming victory.  It displays a
		 * cute message, its not really a resign but
		 * it may as well be. */
		case 0x09:
			aGameResult.result = GameResult::RESIGN;
			black_wins = true;
			break;
		case 0x0a:
			aGameResult.result = GameResult::RESIGN;
			white_wins = true;
			break;
		default:
			qDebug("%02x is unhandled victory condition code", victory_condition_code);
			break;
	}

	//definitely where we pop up result
	//EXCEPT that if its our match and we sent the 0672, I don't think we get one
	//back, but we don't want the countdialog to be looking at our_invitation
	//which is certainly tygem specific even though, at this point, the
	//distinction is meaningless
	//so we'll let countdialog set the result no matter what and we'll
	//drop the 0672 here if its our match, unless its loss on time, not after
	//countdialog is active
	//no unless its a resign, really, according to tygem FIXME
	//this messaage is where result is set
	//this is hideously awkward but its fine for now FIXME
	/*if(game_number == playing_game_number)
	{
		if(aGameResult.result == GameResult::SCORE)
		{
			//apparently, game result doesn't come up
			//from accept count at all or properly
			//which might be better anyway
		//playing_game_number = 0;		//good place?? FIXME
		//return;
		}
	}*/
	
	aGameResult.game_number = game_number;
	if(black_wins)
	{
		aGameResult.loser_name = gd->white_name;
		aGameResult.winner_name = gd->black_name;
		aGameResult.winner_color = stoneBlack;
	}
	else
	{
		aGameResult.loser_name = gd->black_name;
		aGameResult.winner_name = gd->white_name;
		aGameResult.winner_color = stoneWhite;
	}
	boarddispatch->recvResult(&aGameResult);
}

//0x0683:	//clock stop? //enter score?
/* As I recall, these are also used to set the time in professional games
 * being broadcast 
 * But on second thought, this is a 0683:
	//018e 0b01 bfee bfb5 c0da 3832 0000 0000
	//0000 0016 0000 0000 0000 0000 0000 0000
	//0000 1a00 c1a6 3134 b1e2 2047 53c4 aec5
	//d8bd bab9 e820 babb bcb1 b8ae b1d7 0000
 * so not really sure what they are... that's got some names in there, could
 * even be a comment of some kind
 * likely a game update
*/
#ifdef RE_DEBUG
void TygemConnection::handleScoreMsg1(unsigned char * msg, unsigned int size)
#else
void TygemConnection::handleScoreMsg1(unsigned char * msg, unsigned int /*size*/)
#endif //RE_DEBUG
{
	/* Change name FIXME, definitely not enter score, more likely
	 * time stop.  Except, they're variable size... Its even possible
	 * that they're image sends... maybe... */
	BoardDispatch * boarddispatch;
	//0087 aa01 0101 0000 031e 00a0 0334 0040
#ifdef RE_DEBUG
	printf("0x0683: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	boarddispatch = getIfBoardDispatch((msg[0] << 8) + msg[1]);
	if(!boarddispatch)
		return;
#ifdef RE_DEBUG
	boarddispatch->recvKibitz(0, "0x683");
#endif //RE_DEBUG
	//boarddispatch->stopTime();
	
	//after this 7b
	//0087 0101 6261 696e 6974 6500 0000 0000 
	//0000 0011 6261 696e 6974 6500 0000 0002
	//00c5 0000 0000 0000
	//0087 aa01 0300 0000 031e 00a0 0334 0040
	//FIXME
	//black_first = ??
	//handleTimeChunk(boarddispatch, &(msg[7]), black_first);
	//another 7b 83 pair:
	//0087 0001 baac d0a6 bba8 0000 0000 0000
	//0000 0011 6861 6e78 6961 6f68 7561 0002
	//00c5 0000 0000 0000
	
	//0087 aa01 0201 0000 031e 00a0 0334 0040
	
	// and two more pairs, same game
	//0x067b: 
	//00870001baacd0a6bba80000000000000000001168616e7869616f687561000201c6000000000000
	//0087aa0104010100031e00a003340040

	//008701016261696e6974650000000000000000116261696e697465000000000201c6000000000000
	
	//0087aa0105000100031e00a003340040
	//followed by 0672
	//0087 0101 6261 696e 6974 6500 0000 0000
	//0000 0011 0000 0055 0116 0000 031e 00a0
	//0334 0040 0024 0668
}

//an REM -1 -1 likely proceeds these as meaning done?
//this is intrusion to peterius after marking all of his black stones dead
//002c067b
//0002 0101 7065746572697573000000000000000865746572697573000000000200c5000000000000
//0x0683: 0002aa0103000000012a0480012c0440
//067b  black keeps hitting confirm confirm c7 versus c5 is likely score?
//0010 0101 7065746572697573000000000000000865746572697573000000000200c7000000000000
//0x067b:   
void TygemConnection::handleEndgameMsg(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	GameData * game;
	unsigned short game_number;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name, encoded_name2;
	if(size < 34)
	{
		qDebug("Bad endgame msg, size: %d", size);
		return;
	}
	//I"m thinking actually draw request
	//no I think this is a request, count maybe?  or is that 6a?
	//maybe winner message?, unlikely, several sent and 0201c5 as well below
	//00020101616438380000000000000000000000
	//16616438380000000000000002 00 c5000000000000
	//we saw alternating 067b and 0683, with the 83 starting the sequence
	//had something to do with hitting done or agree, in the end black
	//(I think black) just resigned
	//01c50101c4f4d2bbb5b6000000000000000000124e6965596944616f0000000200c7000000000000
	//01c5 aa01 0600 0000 030b 00a0 031e 0060
	//01c50001caa4c1fa3200000000000000000000127368656e676c6f6e6732000201c9000000000000
	//and then right before DSC move message:
	
	//0x067b: 01c50101c4f4d2bbb5b6000000000000000000124e6965596944616f0000000201c6000000000000
	//0x0683: 01c5 aa01 0401 0100 030b 00a0 031e 0060
	//0x067b: 01c50001caa4c1fa3200000000000000000000127368656e676c6f6e6732000200c6000000000000
	//0x0683: 01c5 aa01 0500 0000 030b 00a0 031e 0060
	//0x066b: 00da0001c6f7bcbcc0ccb5b700000000000000166d696e676d696e67303100000100000000000000

	//008701016261696e6974650000000000000000116261696e697465000000000200c5000000000000
	game_number = (msg[0] << 8) + msg[1];	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Unexpected: got 067b for board %d", game_number);
		return;
	}
	game = boarddispatch->getGameData();
#ifdef RE_DEBUG
	printf("***** 0x067b: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	//this is addressee
	strncpy((char *)name, (char *)&(msg[4]), 14);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	strncpy((char *)name, (char *)&(msg[20]), 11);
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	/* encoded_name2 is NOT reliable, for instance in resuming adjourned. */
	if(encoded_name2 != QString())
		encoded_name = encoded_name2;
	
	/*player = getDefaultroom->getPlayerListing(encoded_name);
	if(!player)
	{
		qDebug("Can't get player listing for %s in 067b", encoded_name.toLatin1().constData());
		return;
	}*/
	//035d 0001 696e 7472 7573 696f 6e00 0000 0000 0009
	//696e 7472 7573 696f 6e00 0002 00c3 0000 0000 0000
	
	//030a 0001 696e 7472 7573 696f 6e00 0000 0000 0009
	//696e 7472 7573 696f 6e00 0002 0000 0000 0000 0000
	if(msg[33] == 0xc1)
	{
		if(encoded_name != getUsername())	//probably FIXME
			return;	
		MoveRecord * move = new MoveRecord();
		move->flags = MoveRecord::REQUESTUNDO;
		move->number = NOMOVENUMBER;
		boarddispatch->recvMove(move);
		delete move;
		//sendEndgameMsg(game, refuse_undo_request);
		return;
	}
	else if(msg[33] == 0xc2)
	{
		if(encoded_name != getUsername())
			return;
		if(msg[32] == 0x00)
		{
			boarddispatch->recvKibitz(QString(), QString("%1 refused undo.").arg(
					(encoded_name == game->black_name ? game->white_name : game->black_name)));
			return;
		}
		else
		{
			/*MoveRecord * move = new MoveRecord();
			move->flags = MoveRecord::UNDO;
			move->number = NOMOVENUMBER;
			boarddispatch->recvMove(move);
			delete move;*/
			/* There's a WIT move message that triggers the UNDO */
			return;
		}
	}
	else if(msg[33] == 0xc3)		//this is a pre-pass request count
	{
		if(encoded_name != getUsername())
			return;		//awkward, should be consistent but we check it below
		//actually 066a is the request
		//boarddispatch->recvKibitz(QString(), QString("%1 requests count...").arg(
			//		  (encoded_name == game->black_name ? game->white_name : game->black_name)));
		/*if(encoded_name == getUsername())
			boarddispatch->recvRequestCount();*/
		/* FIXME, I've come to believe this is a draw request or
		 * less likely, an adjournment request */
		boarddispatch->recvRequestDraw();
	}
	else if(msg[33] == 0xc4)
	{
		/* Shouldn't this be the opposite?  Since the message is meant
		 * for us?!? i.e., encoded_name != FIXME*/
		if(encoded_name != getUsername())
			return;		//awkward, should be consistent but we check it below
		if(msg[32] == 0x01)
		{
			/* FIXME maybe all these sorts of messages should be on the boarddispatch.
			 * only issue is making sure we get the player name right but maybe we should rewrite
			 * in a more generic way */
			/*boarddispatch->recvKibitz(QString(), QString("%1 has accepted count request...").arg(
				   (encoded_name == game->black_name ? game->white_name : game->black_name)));
			if(encoded_name == getUsername())
				boarddispatch->recvAcceptCountRequest();*/
			//we also get this ?!?
			//we need to do a real count I think
			//because we're not getting the count request from opponent for
			//whatever reason, possibly button isn't working on eweiqi client
			//but anyway, I've wasted enough time on this tonight FIXME
			//0x067b: 009100010000000000000000000000000000000000000000000000000000000000cc000000000000
			//0x0674: 00910101
			boarddispatch->recvAcceptDrawRequest();
			GameResult aGameResult;
			aGameResult.result = GameResult::DRAW;
			boarddispatch->recvResult(&aGameResult);
			sendLongMatchResult(game_number);
		}
		else
		{
			/*boarddispatch->recvKibitz(QString(), QString("%1 has rejected count request...").arg(
					(encoded_name == game->black_name ? game->white_name : game->black_name)));
			if(encoded_name == getUsername())
				boarddispatch->recvRejectCountRequest();*/
			boarddispatch->recvRefuseDrawRequest();
		}
	}
	else if(msg[33] == 0xc5)
	{
		boarddispatch->recvKibitz(QString(), QString("%1 has hit done...").arg(
				  (encoded_name == game->black_name ? game->white_name : game->black_name)));
		// if we've already hit done, we need to popup a countdialog
		if(match_negotiation_state->isOurGame(game_number))
		{
			if(encoded_name == getUsername())
			{
				if(match_negotiation_state->sentDoneCounting())
					boarddispatch->createCountDialog();
				else
					match_negotiation_state->receiveDoneCounting();
			}
			else		//can we get this more than once for some reason?  duplicate count dialogs? FIXME
			{
				if(match_negotiation_state->receivedDoneCounting())
					boarddispatch->createCountDialog();
				else
					match_negotiation_state->sendDoneCounting();
			}
		}
	}
	else if(msg[33] == 0xc6)
	{
		/* Its not entirely standard to have our reject/accept sends
		 * set those flags without response from server.  But alternative
		 * is to complicate recvRejectCount with who is sending it.
		 * Note that presumably we don't bring up a CountDialog for games
		 * we're observing, but if we need to leave score mode during
		 * observed game, ... well we probably need to handle that from
		 * DSCs and REMs or some other 7b or time message or the like.
		 * We really haven't dealt with score rejection protocol yet
		 * FIXME */
		if(msg[32] == 0x00)
		{
			boarddispatch->recvKibitz(QString(), QString("%1 rejects result").arg(
					(encoded_name == game->black_name ? game->white_name : game->black_name)));
			if(encoded_name == getUsername())	//i.e., meant for us
				boarddispatch->recvRejectCount();
		}
		else //if(msg[36] == 0x01)
		{
			boarddispatch->recvKibitz(QString(), QString("%1 accepts result").arg(
					(encoded_name == game->black_name ? game->white_name : game->black_name)));
			if(encoded_name == getUsername())	//i.e., meant for us
				boarddispatch->recvAcceptCount();
		}
	}
	else if(msg[33] == 0xf0)
	{
		//FIXME do we need to handle these?
	}
	else if(msg[33] == 0xf1)
	{
		//opponent_resumes
		//0063 0001 7065 7465 7269 7573 0000 0000 0000 0000
		//0000 0000 0000 0000 0000 0000 01f1 0c00 031b 0060
		//0305 00a0 0201 0000
		/* These means, I think, our opponent has seen us
		 * rejoin... but what's weird is the above is sent
		 * from peterius to intrusion */
		/* And then after we get this:
		//0063 0000 696e 7472 7573 696f 6e00 0000 0000 0000
		//0000 0000 0000 0000 0000 0000 00f1 0800 031b 0060
		//0305 00a0
		*/
		/* Its possible that these are requesting our reply
		 * they might be sent out to all involved players but
		 * like they suggest color, that 4th byte, or the byte
		 * before the 00... again, intrusion dropped and was
		 * white I think.  Its weird to think we just
		 * echo one of these back though... and they clearly
		 * have the time also */
		//000c 0000 696e 7472 7573 696f 6e00 0000 
		//0000 0000 0000 0000 0000 0000 0000 0000
		//00f1 0800 031b 00a0 010d 0060
		/*if(msg[32] == 0x00)
		{*/
			if(encoded_name == getUsername())
			{
				sendEndgameMsg(game, we_resume);
				MoveRecord * aMove = new MoveRecord();
				aMove->flags = MoveRecord::UNREMOVE;
				sendMove(game_number, aMove);
				delete aMove;
				sendResumeFromDisconnect(game_number);
			}
			else
			{
			}
		/*}
		else //if(msg[32] == 0x01)
		{
			
		}*/
	}
}

void TygemConnection::handleCountRequest(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	BoardDispatch * boarddispatch;
	GameData * game;
	unsigned short game_number;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name;
	if(size != 40)
	{
		qDebug("Count request of strange size: %d", size);
	}
	//0x066a
	//030a 0001 696e 7472 7573 696f 6e00 0000 0000 0009
	//696e 7472 7573 696f 6e00 0002 0000 0000 0000 0000
	//0x066b: 00da0001c6f7bcbcc0ccb5b700000000000000166d696e676d696e67303100000100000000000000

	game_number = (msg[0] << 8) + msg[1];	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Unexpected: got 066a for board %d", game_number);
		return;
	}
	game = boarddispatch->getGameData();
#ifdef RE_DEBUG
	printf("***** 0x066a: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	p += 2;
	p += 2;
	//this is name for not the player who hit done
	strncpy((char *)name, (char *)p, 14);
	p += 16;
	strncpy((char *)name, (char *)p, 11);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	/*player = getDefaultroom->getPlayerListing(encoded_name);
	if(!player)
	{
	qDebug("Can't get player listing for %s in 067b", encoded_name.toLatin1().constData());
	return;
}*/
	boarddispatch->recvKibitz(QString(), QString("%1 requests count...").arg(
				  (encoded_name == game->black_name ? game->white_name : game->black_name)));
	if(encoded_name == getUsername())
		boarddispatch->recvRequestCount();
}

void TygemConnection::handleCountRequestResponse(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	BoardDispatch * boarddispatch;
	GameData * game;
	unsigned short game_number;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name;
	if(size != 0x24)	//eweiqi 40 FIXME
	{
		qDebug("Count request response of strange size: %d", size);
	}
	//0x066a
	//030a 0001 696e 7472 7573 696f 6e00 0000 0000 0009
	//696e 7472 7573 696f 6e00 0002 0000 0000 0000 0000
	//0x066b: 00da 0001 c6f7 bcbc c0cc b5b7 0000 0000 
			//0000 0016 6d69 6e67 6d69 6e67 3031 0000 
			//0100 0000 0000 0000

	game_number = (msg[0] << 8) + msg[1];	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Unexpected: got 066b for board %d", game_number);
		return;
	}
	game = boarddispatch->getGameData();
#ifdef RE_DEBUG
	printf("***** 0x066b: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	
	p += 2;
	p += 2;
	//this is name for not the player who hit done
	strncpy((char *)name, (char *)p, 14);
	p += 16;
	strncpy((char *)name, (char *)p, 11);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	/*player = getDefaultroom->getPlayerListing(encoded_name);
	if(!player)
	{
	qDebug("Can't get player listing for %s in 067b", encoded_name.toLatin1().constData());
	return;
}*/
	if(msg[32] == 0x01)
	{
		boarddispatch->recvKibitz(QString(), QString("%1 has accepted count request...").arg(
				   (encoded_name == game->black_name ? game->white_name : game->black_name)));
		if(encoded_name == getUsername())
			boarddispatch->recvAcceptCountRequest();
		else
		{
			boarddispatch->recvEnterScoreMode();
			match_negotiation_state->enterScoreMode();
		}
	}
	else
	{
		boarddispatch->recvKibitz(QString(), QString("%1 has rejected count request...").arg(
				(encoded_name == game->black_name ? game->white_name : game->black_name)));
		if(encoded_name == getUsername())
			boarddispatch->recvRejectCountRequest();
	}
}

void TygemConnection::handleTime(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	GameData * gr;
	unsigned short game_number = (p[0] << 8) + p[1];
	bool player_is_black;
	if(size != 12)
	{
		qDebug("time message of size: %d\n", size);
		//FIXME
	}
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", p[0], p[1]);
		return;
	}
	gr = boarddispatch->getGameData();
	if(!gr)
	{
		qDebug("Can't get game record for board: %d", game_number);
		return;
	}
#ifdef RE_DEBUG
//#ifdef NOISY_DEBUG
	printf("TIME: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", p[i]);
	printf("\n");
//#endif //NOISY_DEBUG
#endif //RE_DEBUG

	p += 2;
	// this gets reversed
	player_is_black = p[0] ^ gr->white_first_flag;
	p += 2;
	enum TimeFlags f = handleTimeChunk(boarddispatch, p, player_is_black);
	/* double check the below */
	if(f == BLACK_LOSES && gr->white_name == getUsername())
	{
		GameResult aGameResult;
		aGameResult.result = GameResult::TIME;
		aGameResult.winner_color = stoneWhite;
		aGameResult.winner_name = gr->white_name;
		aGameResult.loser_name = gr->black_name;
		boarddispatch->recvResult(&aGameResult);
		sendResult(gr, &aGameResult);
	}
	else if(f == WHITE_LOSES && gr->black_name == getUsername())
	{
		GameResult aGameResult;
		aGameResult.result = GameResult::TIME;
		aGameResult.winner_color = stoneBlack;
		aGameResult.winner_name = gr->black_name;
		aGameResult.loser_name = gr->white_name;
		boarddispatch->recvResult(&aGameResult);
		sendResult(gr, &aGameResult);
	}
}

enum TygemConnection::TimeFlags TygemConnection::handleTimeChunk(BoardDispatch * boarddispatch, unsigned char chunk[8], bool black_first)
{
	unsigned char c_periods, o_periods;
	unsigned char c_flags, o_flags;
	unsigned int c_seconds, o_seconds;
	int white_seconds, white_periods;
	int black_seconds, black_periods;
	bool black_loses_on_time = false, white_loses_on_time = false;
	
	/* Since we send time, we block it if its from us */
	if(black_first && boarddispatch->getGameData()->black_name == getUsername())
		return GAME_ON;
	if(!black_first && boarddispatch->getGameData()->white_name == getUsername())
		return GAME_ON;
	/* shouldn't we really be recving this time
	 * to sync with server lag? maybe FIXME*/
	
	//first four are this current players turn
	c_periods = chunk[0];
	c_seconds = chunk[1] + (chunk[2] * 60);
	c_flags = chunk[3];
	//second four are other player
	o_periods = chunk[4];
	o_seconds = chunk[5] + (chunk[6] * 60);
	o_flags = chunk[7];
	if(black_first)
	{
		black_seconds = c_seconds;
		white_seconds = o_seconds;
		if(c_flags & 0x20)
			black_periods = c_periods;
		else
			black_periods = -1;
		//if(c_flags & 0x80 && black_periods == 0)
		if(black_periods == 0 && black_seconds == 0)
			black_loses_on_time = true;
		if(o_flags & 0x20)
			white_periods = o_periods;
		else
			white_periods = -1;
		//if(o_flags & 0x80 && white_periods == 0)
		if(white_periods == 0 && white_seconds == 0)
			white_loses_on_time = true;
	}
	else
	{
		black_seconds = o_seconds;
		white_seconds = c_seconds;
		if(c_flags & 0x20)
			white_periods = c_periods;
		else
			white_periods = -1;
		//if(c_flags & 0x80 && white_periods == 0)
		if(white_periods == 0 && white_seconds == 0)
			white_loses_on_time = true;
		if(o_flags & 0x20)
			black_periods = o_periods;
		else
			black_periods = -1;
		//if(o_flags & 0x80 && black_periods == 0)
		if(black_periods == 0 && black_seconds == 0)
			black_loses_on_time = true;
	}

	/*
	if(boarddispatch->getGameData()->black_name == getUsername())
		black_seconds = 0;
	if(boarddispatch->getGameData()->white_name == getUsername())
		white_seconds = 0;*/
	
	/* We should most likely kill the timer then... */
	boarddispatch->recvTime(TimeRecord(white_seconds, white_periods), TimeRecord(black_seconds, black_periods));
	if(black_loses_on_time)
		return BLACK_LOSES;
	else if(white_loses_on_time)
		return WHITE_LOSES;
	return GAME_ON;
}

/* FIXME disturbing implication.  When we were sending the moves one off,
 * it would draw them on the place we clicked, but then draw the last move
 * on the place we got back.  This indicates inconsistencies in move
 * placement code, serious issue */
 /* FIXME Consider adding code to actually filter out by the move message
  * number per game in case that's a kind of prevention against replays
  * or something.  */
/* FIXME When resuming a game "Opp sends bad move message number" comes up
 * repeatedly because we're passed all the moves at once and sends none of
 * them.  bad move message number stuff needs to be fixed up somehow anyway. */
//0668
void TygemConnection::handleMove(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	GameData * gr;
	int x, y;
	unsigned short game_number = (p[0] << 8) + p[1];
	int player_number, player_number2, number0;
	bool opponents_move = false;
	if(size < 24)		//FIXME
	{
		qDebug("Bad move msg, size: %d", size);
		return;
	}
#ifdef RE_DEBUG
	printf("Move msg:\n");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x ", p[i]);
	printf("\n");
#endif //RE_DEBUG
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %02x%02x", p[0], p[1]);
		return;
	}
	gr = boarddispatch->getGameData();
	if(!gr)
	{
		//FIXME
		qDebug("Shouldn't be a board dispatch here\n");
		return;
	}
	p += 2;
	//redundant but make sure
	/* Fairly certain this is not the color but the invite
	 * byte !our_invitation */
	if(p[0] == 0x00)
		player_number2 = 0;
	else if(p[0] == 0x01)//white
		player_number2 = 1;
	else
		qDebug("Strange invitation byte in move");
#ifdef RE_DEBUG
	printf("%02x vs %02x!!\n", p[0], gr->our_invitation);
#endif //RE_DEBUG
	//00 00 their invitation, their move
	//01 00 their invitation our move
	//01 00 their invitation, our move
	if(match_negotiation_state->isOngoingMatch(game_number))
	{
		if(player_number2 == gr->our_invitation)
			opponents_move = true;
	}
	p += 2;
	p += 4;		//ffff ffff
	p += 4;		//could be time
	//STO 0 move_number player_number x y 0a
	//SUR 0 move_number player_number 0a
	//REM 0 312 -1 -1 0 1 
	//REM 0 307 5 6 1 0 
	//REM 0 306 10 14 2 0 			un remove?
	//REM 0 number x y player_number 0
	//DSC 1 0 1			//dead stone clear
	//DSC 1 0 2 
	//SKI 0 player_number		//skip? pass?
	
	//BAC 0 246 1 	//part of review
	//CSP 0 233 0
	//CST 0 234 
	//FOR 0 186 1
	
	// don't know, something on pro broadcast
	//WIT 0 8 2
	//WIT 0 9 1
	//WIT 0 10 2
	//WIT 0 11 1
	//WIT 0 12 2
#ifdef RE_DEBUG
	printf("%d %s\n", player_number2, (char *)p);
#endif //RE_DEBUG
	if(strncmp((char *)p, "STO ", 4) == 0)
	{
		MoveRecord * aMove = new MoveRecord();
		p += 4;
		if(sscanf((char *)p, "%d %d %d %d %d", &number0, &aMove->number, &player_number, &aMove->x, &aMove->y) != 5)
		{
			qDebug("Bad Move");
			delete aMove;
			return;
		}
		/* The move message number is just that, it doesn't have to
		 * have anything to do with the move number.  Also, we FIXME
		 * can't use a global in case we're watching multiple games? */
		if(opponents_move)
		{
			if(aMove->number != (unsigned)move_message_number + 1)
				qDebug("Opp sends bad move message number: %d", aMove->number);
			move_message_number = aMove->number;
		}
		//aMove->number = move_message_number;
		//aMove->number--;	//first move is... handicap?
		aMove->number = NOMOVENUMBER;
		aMove->x++;
		aMove->y++;
		if(player_number == 1)
			aMove->color = stoneBlack;
		else if(player_number == 2)
			aMove->color = stoneWhite;
		else
			qDebug("Strange player number: %d", player_number);
		if(number0 != 0)
			qDebug("Number0 = %d\n", number0);
		//if(aMove->x == 0 || aMove->y == 0)	//doubt it FIXME
		//	aMove->flags = MoveRecord::PASS;
		boarddispatch->recvMove(aMove);
		delete aMove;
	} 
	else if(strncmp((char *)p, "SKI ", 4) == 0)
	{
		p += 4;
		int move_number;
		if(sscanf((char *)p, "%d %d %d", &number0, &move_number, &player_number) != 2)
		{
			qDebug("Bad skip message");
			return;
		}
		if(opponents_move)
		{
			if(move_number != (int)(move_message_number + 1))
				qDebug("Opp sends bad move message number: %d", move_number);
			move_message_number = move_number;
		}
#ifdef RE_DEBUG
		boarddispatch->recvKibitz(QString(), "SKI");
#endif //RE_DEBUG
	} 
	else if(strncmp((char *)p, "SUR ", 4) == 0)
	{
		p += 4;
		int move_number;
		if(sscanf((char *)p, "%d %d %d", &number0, &move_number, &player_number) != 3)
		{
			qDebug("Bad resign message");
			return;
		}
		if(opponents_move)
		{
			if(move_number != move_message_number + 1)
				qDebug("Opp sends bad move message number: %d", move_number);
		}
		/* If opponent resigns, we act on the 0670 we receive */
	} 
	else if(strncmp((char *)p, "REM ", 4) == 0)
	{
		MoveRecord * aMove = new MoveRecord();
		/* FIXME, fix potential bug with unremoving as on cyberoro ?!?? */
		aMove->flags = MoveRecord::REMOVE_AREA;
		int remove_flag;
		p += 4;
		if(sscanf((char *)p, "%d %d %d %d %d", &number0, &aMove->number, &x, &y, &remove_flag) != 5)
		{
			qDebug("Bad Move");
			delete aMove;
			return;
		}
		if(opponents_move)
		{
			if(aMove->number != (unsigned)move_message_number + 1)
				qDebug("Opp sends bad move message number: %d", aMove->number);
			move_message_number = aMove->number;
		}
		if(x == -1 && y == -1)
		{
#ifdef RE_DEBUG
			boarddispatch->recvKibitz(QString(), "REM -1 -1");
#endif //RE_DEBUG
			/* We also get this on a resume and send it as well */
			//done scoring! recvKibitz, etc.
			//seems to be only one of these
			//no, I see two
			delete aMove;
			return;
		}
		aMove->x = x + 1;
		aMove->y = y + 1;
		aMove->number--;
		/*if(player_number == 1)
			aMove->color = stoneBlack;
		else if(player_number == 2)
			aMove->color = stoneWhite;
		else
			qDebug("Strange player number: %d", player_number);*/
		if(number0 != 0)
			qDebug("Number0 = %d\n", number0);
		//if(aMove->x == 0 || aMove->y == 0)	//doubt it FIXME
		//	aMove->flags = MoveRecord::PASS;
		boarddispatch->recvMove(aMove);
		delete aMove;
	}
	else if(strncmp((char *)p, "DSC ", 4) == 0)
	{
		MoveRecord * aMove = new MoveRecord();
		/* FIXME, fix potential bug with unremoving as on cyberoro ?!?? */
		aMove->flags = MoveRecord::UNREMOVE_AREA;
		int number2;
		p += 4;
		if(sscanf((char *)p, "%d %d %d", &number0, &number2, &player_number) != 3)
		{
			qDebug("Bad Move");
			delete aMove;
			return;
		}
		if(player_number == 2)
			aMove->color = stoneWhite;
		else
			aMove->color = stoneBlack;
		/*if(player_number == 1)
		aMove->color = stoneBlack;
		else if(player_number == 2)
		aMove->color = stoneWhite;
		else
		qDebug("Strange player number: %d", player_number);*/
		//1 DSC 1 0 1 
		if(number0 != 0)
			qDebug("Number0 = %d\n", number0);
		//if(aMove->x == 0 || aMove->y == 0)	//doubt it FIXME
		//	aMove->flags = MoveRecord::PASS;
		boarddispatch->recvMove(aMove);
		delete aMove;
	}
	else if(strncmp((char *)p, "WIT ", 4) == 0)
	{
		MoveRecord * aMove = new MoveRecord();
		/* WIT looks like a kind of review undo (withdraw?), player_number here is
		 * who's turn it now is.*/
		p += 4;
		if(sscanf((char *)p, "%d %d %d", &number0, &aMove->number, &player_number) != 3)
		{
			qDebug("Bad WIT move");
			delete aMove;
			return;
		}
		if(opponents_move)
		{
			//can this even happen?
			if(aMove->number != (unsigned)move_message_number + 1)
				qDebug("Opp sends bad move message number: %d", aMove->number);
			move_message_number = aMove->number;
		}
		
		aMove->number = NOMOVENUMBER;
		aMove->flags = MoveRecord::UNDO;
		boarddispatch->recvMove(aMove);
		delete aMove;
	}
	else if(strncmp((char *)p, "BAC ", 4) == 0)
	{
		MoveRecord * aMove = new MoveRecord();
		/* I assume this is backup, or go back to a particular point.  I'm not quite sure
		 * what to do with it right now, since currently observer can look at any move.
		 * but quite likely it just sets the move to something. */
		p += 4;
		if(sscanf((char *)p, "%d %d %d", &number0, &aMove->number, &player_number) != 3)
		{
			qDebug("Bad BAC move");
			delete aMove;
			return;
		}
		qDebug("BAC msg %d %d", aMove->number, player_number);
		/*if(opponents_move)
		{
			//can this even happen?
			if(aMove->number != (unsigned)move_message_number + 1)
				qDebug("Opp sends bad move message number: %d", aMove->number);
			move_message_number = aMove->number;
		}
		
		aMove->number = NOMOVENUMBER;
		aMove->flags = MoveRecord::UNDO;
		boarddispatch->recvMove(aMove);*/
		delete aMove;
	}
	else if(strncmp((char *)p, "FOR ", 4) == 0)
	{
		MoveRecord * aMove = new MoveRecord();
		/* I assume this is backup, or go back to a particular point.  I'm not quite sure
		 * what to do with it right now, since currently observer can look at any move.
		 * but quite likely it just sets the move to something. */
		p += 4;
		if(sscanf((char *)p, "%d %d %d", &number0, &aMove->number, &player_number) != 3)
		{
			qDebug("Bad BAC move");
			delete aMove;
			return;
		}
		qDebug("BAC msg %d %d", aMove->number, player_number);
		/*if(opponents_move)
		{
			//can this even happen?
			if(aMove->number != (unsigned)move_message_number + 1)
				qDebug("Opp sends bad move message number: %d", aMove->number);
			move_message_number = aMove->number;
		}
		
		aMove->number = NOMOVENUMBER;
		aMove->flags = MoveRecord::UNDO;
		boarddispatch->recvMove(aMove);*/
		delete aMove;
	}
	else
		printf("Unknown: %s\n", (char *)p);
}

void TygemConnection::handlePass(unsigned char * msg, unsigned int size, int /* FIXME */)
{
	if(size != 40)	//or 0x24??
	{
		qDebug("Pass msg of strange size: %d", size);
	}
	unsigned char * p = msg;
	BoardDispatch * boarddispatch;
	int game_number = (p[0] << 8) + p[1];
	p += 2;
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Can't get board for %d for pass\n", game_number);
		return;
	}
	GameData * game = boarddispatch->getGameData();
	bool white;
	/* Don't think so
	if(game->our_invitation)
		white = !p[0];	//doublecheck
	else
		white = p[0];*/
	p += 2;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name, encoded_name2;
	
	strncpy((char *)name, (char *)p, 14);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));

	p += 15;
	
	//rank byte
	p++;
	strncpy((char *)name, (char *)p, 11);
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 10;
	p++;
	if(encoded_name2 == game->white_name)
		white = false;
	else
		white = true;
	//white = (p[0] != 0x02);
	//there's an 02 much later here and that must be... black color?
	/* No, this is likely the 02 country code that floats around, 02 meaning
	 * china.  Its just part of the header */
	/* Do we do this here or on the handleMove SKI? FIXME
	 * I think this message is more reliable, but not sure for observed games
	 * probably here */
	MoveRecord * move = new MoveRecord();
	move->flags = MoveRecord::PASS;
	move->color = white ? stoneWhite : stoneBlack;

	//boarddispatch->recvKibitz(0, QString(encoded_name) + " passes " + QString::number(move->color));
	//not right
	//still not right, for instance if we offer the game and play black
	//it gets reversed
	boarddispatch->recvMove(move);
	match_negotiation_state->incrementPasses();
	if(match_negotiation_state->twoPasses())
		match_negotiation_state->enterScoreMode();
	//black
	//00870101 6261 696e 6974 6500 0000 0000 0000 00116261696e69746500000000020000000000000000
#ifdef RE_DEBUG
	printf("**** 0x0669: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
}

QString TygemConnection::getStatusFromCode(unsigned char /* FIXME */, QString /* FIXME */)
{
	return "";
}

int TygemConnection::compareRanks(QString rankA, QString rankB)
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

//0639
void TygemConnection::handleBoardOpened(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	GameData * aGameData;
	unsigned short game_number;
	if(size != 0x30)
	{
		qDebug("board opened msg of strange size: %d\n", size);
	}

	game_number = (p[0] << 8) + p[1];
	p += 2;
	
#ifdef RE_DEBUG
	printf("0639 match: \n");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG

	boarddispatch = getBoardDispatch(game_number);
	/* We have to pass the game data record on the 0639
	 * because the boardwindow is created by the dispatch
	 * upon recv a record.  The board window is necessary
	 * to receive the observers from 0666, 0667 illicited
	 * by the 0665 message, the 0665 message also illicits
	 * the 0671 info message.  So this is the way it must
	 * be done */
	aGameData = boarddispatch->getGameData();
	aGameData->number = game_number;
	
	/* Its possibly that we're getting this on response to 0638,
	 * trying to join a game we got disconnected from.  In which
	 * case ... well actually, I think we should get a proper
	 * debug, something else is screwed up, like player names
	 * or something FIXME */
	
	if(match_negotiation_state->isOurGame(game_number))
		aGameData->gameMode = modeMatch;
	/*GameListing * gl = room->getGameListing(game_number);
	//FIXME get first time records
	if(gl)
	{
		aGameData->black_name = gl->black_name();
		aGameData->black_rank = gl->black_rank();
		aGameData->white_name = gl->white_name();
		aGameData->white_rank = gl->white_rank();
		// Unfortunately relevant to time records
		aGameData->white_first_flag = gl->white_first_flag;
	}*/
	boarddispatch->openBoard();
	
	/* Moved here from 0639 because 0671 probably
	 * clears list or something on recv first data 
	 * and then back to 0639 because apparently,
	 * it is the observer request that illicits 0671?*/
	if(!match_negotiation_state->isOurGame(game_number))
		sendObserversRequest(game_number);
	
	return;
}

//0671
void TygemConnection::handleMatchOpened(unsigned char * msg, unsigned int size)
{
	Room * room = getDefaultRoom();
	BoardDispatch * boarddispatch;
	unsigned char * p = msg;
	PlayerListing * opponent = 0;
	GameData * aGameData;
	unsigned short game_number;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_nameA, encoded_nameB;
	QString encoded_nameA2, encoded_nameB2;
	QString rankA, rankB;
	if(size != 92)			//eweiqi 88 FIXME
	{
		qDebug("Match open msg of strange size: %d", size);
	}
	/* Is there a board_size FIXME or not? */
	game_number = (p[0] << 8) + p[1];
	p += 2;
	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("No board dispatch for %d", game_number);
		return;
	}
	if(match_negotiation_state->isOurGame(game_number) &&
		!match_negotiation_state->startMatchAcceptable())
	{
		qDebug("received match start at strange time");
		return;
	}
	aGameData = boarddispatch->getGameData();
	
	aGameData->number = game_number;
	
	/* Don't want to use gl for names, but maybe need it for
	 * white first flag?  Hope not */
	GameListing * gl = room->getGameListing(game_number);
	//FIXME get first time records
	if(gl)
	{
		//aGameData->black_name = gl->black_name();
		//aGameData->black_rank = gl->black_rank();
		//aGameData->white_name = gl->white_name();
		//aGameData->white_rank = gl->white_rank();
		/* Unfortunately relevant to time records */
		aGameData->white_first_flag = gl->white_first_flag;
	}
#ifdef RE_DEBUG
	printf("0671 match: \n");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
	for(unsigned int i = 0; i < size; i++)
		printf("%c", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	// we should see if there's time info in here or the
			// one we're currently using.  we need to initially set time
			// we also seem to get this one when rematch started
	
	/* FIXME If this is our match, we should
	* most likely do bd->startGame(); here */
			
	//0087 0001 baac d0a6 bba8 0000 0000 0000
	//0000 0011 6261 696e 6974 6500 0000 0000
	//0000 0011 012c 1e03 0000 0000 0100 0002
	//ffff ffff ffff ffff 0002 0002 6861 6e78
	//6961 6f68 7561 0002 6261 696e 6974 6500
	//0000 0002 48f6 56cd
	//definitely thinking game info
	//00050001c0cfcdb7000000000000000000000014ced2d0cdced2d0e30000000000000014
	//012c1e030000010001000002ffffffffffffffff000100044a61636b44616e6e7900000267647368
	//6a0000000000000248f6a64e

	//second option high minutes
	//0003000170657465726975730000000000000008696e74727573696f6e00000000000009
	//012c28010100010001000002ffffff
	//ffffffffff00010001706574657269757300000002696e74727573696f6e00000248fcebe0
	
	//probably players turn 0001
	
	//below is a rematch packet.  We should at the very least clear the board
	//for the new game.  Possibly we should close the dispatch and create a
	//new board, as in in case the observer wants to continue looking at the
	//match instead of instantly moving to the new one
	//yeah I like that
	//one issue though is that there doesn't seem to be a rematch flag
	//and 0639 opens the board dispatch.  Which means this has to
	//check that the game has ended, I guess just check for a result
	//after getting the 0671, since presumably any result messages
	//from broadcast games would come later and then if there is a result
	//close and reopen the boarddispatch.  But then we need to be careful
	//that we don't lose anything that the 0639 put up that the 0671
	//neglected to.  I don't think there's anything but... FIXME
	//00490001c4c1d2b0b5b6bfcd0000000000000015797031323131373400000000000000
	//14012c1e030100000001000002ffffffffffffffff000b000c6d79646b003137340000
	//0002797031323131373400000002498b58e4
	if(aGameData->fullresult)
	{
		qDebug("0671 received for game with result already set\nassuming rematch\n");
		class ObserverListModel * olm = boarddispatch->getObserverListModelForRematch();
		NetworkConnection::closeBoardDispatch(game_number);
		boarddispatch = getBoardDispatch(game_number);
		if(!boarddispatch)
		{
			qDebug("Can't create board dispatch for %d", game_number);
			return;
		}
		boarddispatch->setObserverListModel(olm);
		//below is copied from 0639, awkward FIXME
		boarddispatch->openBoard();

		if(match_negotiation_state->isOurGame(game_number))
		{
			match_negotiation_state->startMatch();
			//verify name and settings, possibly with switched color FIXME
		}
		aGameData = boarddispatch->getGameData();
		
		aGameData->number = game_number;
	}
	p += 2;
	//name and rank
	strncpy((char *)name, (char *)p, 14);
	encoded_nameA = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 15;
	//rank byte
	if(p[0] < 0x12)
		rankA = QString::number(0x12 - p[0]) + 'k';
	else if(p[0] > 0x1a)
		rankA = QString::number(p[0] - 0x1a) + 'p';
	else
		rankA = QString::number(p[0] - 0x11) + 'd';
	p++;
	//name and rank		
	strncpy((char *)name, (char *)p, 14);
	encoded_nameB = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 15;
	//rank byte
	if(p[0] < 0x12)
		rankB = QString::number(0x12 - p[0]) + 'k';
	else if(p[0] > 0x1a)
		rankB = QString::number(p[0] - 0x1a) + 'p';
	else
		rankB = QString::number(p[0] - 0x11) + 'd';
	p++;
	//time settings
	//type seconds minutes periods
	/*switch(p[0])
	{
		case 0x01:
			aGameData->timeSystem = byoyomi;
			break;
		default:
			printf("Time system: %02x!!!\n", p[0]);
			break;
	}*/
	// TIME is wrong FIXME
	aGameData->timeSystem = byoyomi;		//anything else??
	aGameData->maintime = (p[0] << 8) + p[1];
	aGameData->periodtime = p[2];
	aGameData->stones_periods = p[3];
#ifdef RE_DEBUG
	printf("0671 TIME SETTINGS: %d %d %d %d %d %d\n", p[0], p[1], p[2], p[3], p[4], p[5]);
#endif //RE_DEBUG
	aGameData->handicap = p[4];
	p += 4;
	p += 2;
	/* This isn't white first flag I don't think, its first player
	 * white I think... or maybe first player black, so I'll negate it...*/
	
	aGameData->white_first_flag = !p[0];
	/* FIXME okay, obviously this isn't right, at the same time, we use it below  BIG FIXME */
	aGameData->undoAllowed = p[0];
	if(p[1])
		aGameData->free_rated = FREE;
	else if(!p[1])
		aGameData->free_rated = RATED;
	else
		qDebug("strange rated byte on board open %02x", p[1]);
	p += 2;

#ifdef RE_DEBUG
	//FIXME really hope I don't use this anymore
	if(gl && gl->white_first_flag != aGameData->white_first_flag)
		qDebug("gl and 0671 white first flag differ !!!!!");
	printf("Setting white first flag to: %d\n", aGameData->white_first_flag);
#endif //RE_DEBUG

	p += 4;
	p += 4;
	p += 4;
	//this is white/black or our picture code: p[1]
	p += 2;
	//possibly this is black or their picture code: p[1]
	p += 2;
	strncpy((char *)name, (char *)p, 11);
	encoded_nameA2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 10;
	//p[1] is color byte
	/* We assume ascii names are in same order */
	//if(p[1] == 0x02)
#ifdef RE_DEBUG
	boarddispatch->recvKibitz(QString(), "0671 wff " + QString::number(aGameData->white_first_flag)
				+ " gl " + (gl ? QString::number(gl->white_first_flag) : "NA") +	
				+ " " + QString::number(p[1], 16) + " " + QString::number(p[13], 16));
#endif //RE_DEBUG
	if(aGameData->white_first_flag)
	{
		aGameData->white_name = encoded_nameA2;
		aGameData->white_rank = rankA;
		qDebug("white is %s", encoded_nameA2.toLatin1().constData());
	}
	//else if(p[1] == 0x01)
	else
	{
		aGameData->black_name = encoded_nameA2;
		aGameData->black_rank = rankA;
		qDebug("black is %s", encoded_nameA2.toLatin1().constData());
	}
	//else
	//	qDebug("Strange color byte %d, line: %d", p[1], __LINE__);
	p += 2;
	strncpy((char *)name, (char *)p, 11);
	encoded_nameB2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 10;
	if(match_negotiation_state->isOurGame(game_number))
	{
		opponent = room->getPlayerListing(encoded_nameB2);
		if(!match_negotiation_state->verifyPlayer(opponent))
		{
			qDebug("start match by unknown player");
			return;
		}
		if(!match_negotiation_state->verifyGameData(*aGameData))
		{
			qDebug("opponent attempted starting game with different match settings then negotiated upon");
			return;
		}
	}
	//p[1] is color byte
	//if(p[1] == 0x02)
	if(!aGameData->white_first_flag)
	{
		aGameData->white_name = encoded_nameB2;
		aGameData->white_rank = rankB;
		qDebug("white is %s", encoded_nameB2.toLatin1().constData());
	}
	//else if(p[1] == 0x01)
	else
	{
		aGameData->black_name = encoded_nameB2;
		aGameData->black_rank = rankB;
		qDebug("black is %s", encoded_nameB2.toLatin1().constData());
	}
	//else
	//	qDebug("Strange color byte %d, line: %d", p[1], __LINE__);
	p += 2;
	boarddispatch->gameDataChanged();
	if(aGameData->handicap)
	{
		/* This should really be cleaned up, done in some
		 * general place.  Handicap games on tygem are
		 * really rare and I don't know how to really
		 * set them up yet in matches.  For all the
		 * protocols this should be setup better.
		 * So this is just temporary. FIXME */
		MoveRecord aMove;
		aMove.number = NOMOVENUMBER;
		aMove.x = aGameData->handicap;
		aMove.flags = MoveRecord::HANDICAP;
		boarddispatch->recvMove(&aMove);
	}
	if(match_negotiation_state->isOurGame(game_number))
	{
		if(encoded_nameA2 == getUsername())
		{
			//we do this only if we didn't send the 0671 out
			if(!opponent)
				qDebug("Can't get opponent to close game dialog");
			else
				getAndCloseGameDialog(*opponent);
			/* Note that getAndClose will fail if we're rejoining
			 * a match in progress FIXME, networkconnection reports
			 * error. */
		}
		//FIXME time
		//doublecheck this same issue with 0 maintime on IGS and ORO
		if(aGameData->maintime == 0)
			boarddispatch->recvTime(TimeRecord(aGameData->periodtime, aGameData->stones_periods), TimeRecord(aGameData->periodtime, aGameData->stones_periods));
		else
			boarddispatch->recvTime(TimeRecord(aGameData->maintime, -1), TimeRecord(aGameData->maintime, -1));
		if(aGameData->free_rated == FREE)
			aGameData->undoAllowed = true;
		boarddispatch->startGame();	//starts timers
		boarddispatch->swapColors(true);
	}
	/* Actually, first move message number appears to be 2, maybe 1 is for handicap?
	 * this needs to be right for when we play black!! */
	move_message_number = 1;	//changed from 0... only relevant for our games!! FIXME
	
	return;
}

//join with 0646 as well
//0645
void TygemConnection::handleMatchOffer(unsigned char * msg, unsigned int size, MIVersion version)
{
	unsigned char * p = msg;
	if(size != 88)		//84
	{
		qDebug("Match offer msg of strange size: %d", size);
	}
	
	unsigned short game_number;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name, encoded_name2;
	PlayerListing * opponent;
	
	strncpy((char *)name, (char *)p, 14);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	if(encoded_name != getUsername())	//FIXME in case response doesn't readdress
		return;
	p += 15;
	//rank
	p++;
	strncpy((char *)name, (char *)p, 14);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 15;
	//rank
	p++;
	game_number = (p[0] << 8) + p[1];
	p += 2;
	//time settings
	MatchRequest * tempmr = new MatchRequest();
	tempmr->number = game_number;
	tempmr->timeSystem = byoyomi;
	tempmr->maintime = (p[0] << 8) + p[1];
	tempmr->periodtime = p[2];
	tempmr->stones_periods = p[3];
	tempmr->handicap = p[4];
	tempmr->komi = (float)p[5] + .5;
	switch(p[6])
	{
		case 0x00:
			tempmr->color_request = MatchRequest::WHITE;
			break;
		case 0x01:
			tempmr->color_request = MatchRequest::BLACK;
			break;
		case 0x02:
			tempmr->color_request = MatchRequest::NIGIRI;
			break;
		default:
			printf("Strange byte in match offer: %02x", p[6]);
			break;
	}
	p += 7;
#ifdef RE_DEBUG
	if(p[0] != p[1])
		printf("match offer with friendly/rated %02x %02x", p[0], p[1]);
#endif //RE_DEBUG
	tempmr->undoAllowed = p[0];
	//01 00 looks like rated with undo checked
	if(p[1] == 0x01)
		tempmr->free_rated = FREE;
	else if(p[1] == 0x00)
		tempmr->free_rated = RATED;
	else
		printf("match offer with friendly/rated %02x %02x", p[0], p[1]);
	p += 2;
	//message version byte
	p++;
	p += 4;		//ffff ffff
	p += 12;
	strncpy((char *)name, (char *)p, 11);		//our nickname
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 11;
	p++;	//country_id
	strncpy((char *)name, (char *)p, 11);
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 11;
	p++;	//country_id
	
	if(version == offer)
	{
		//0645 doesn't have opponent's nickname, apparently,
		//our nickname if we send it, but not opponents
		tempmr->opponent = encoded_name;
		tempmr->opponent_is_challenger = true;
		opponent = getDefaultRoom()->getPlayerListingByNotNickname(encoded_name);
		if(!opponent)
		{
			qDebug("Match offer from unknown opponent");
			delete tempmr;
			return;
		}
	}
	else
	{
		tempmr->opponent = encoded_name2;
		opponent = getDefaultRoom()->getPlayerListing(encoded_name2);
		if(!opponent)
		{
			qDebug("Match modify or accept from unknown opponent");
			delete tempmr;
			return;
		}
	}
	if(!match_negotiation_state->verifyPlayer(opponent))
	{
		qDebug("Match offer/accept from unknown opponent");
		return;
	}
	//0004 0e10 1e03 0100
	if(version == offer || version == modify)
	{	
		//apparently acknowledge no longer necessary?
		//sendMatchOffer(*tempmr, acknowledge);
		
		if(version == offer && !match_negotiation_state->waitingForMatchOffer())
		{
			qDebug("received match offer, not waiting for match offer");
			return;
		}
		else if(version == modify && !match_negotiation_state->sentMatchOffer())
			return;
		//Here, we actually want to pop up game dialog
		GameDialog * gameDialogDispatch = getGameDialog(*opponent);
		gameDialogDispatch->recvRequest(tempmr, getGameDialogFlags());
	}
	else if(version == accept)
	{
		if(!match_negotiation_state->sentMatchOffer())
			return;
		//match offer accept
		//00580646
		//0x0646: 70657465726975730000000000000008696e74727573696f6e000000000000090032012c2801010001
		//000001ffffffff000000000001000000000000706574657269757300000002696e74727573696f6e000002
		//00a00616
		MatchRequest * mr = getAndCloseGameDialog(*opponent);
		if(!mr)
		{
			qDebug("Can't get match request for opponent");
			//return;
		}
		if(!match_negotiation_state->verifyMatchRequest(*tempmr))
		{
			qDebug("opponent accepted with strange match sttings");
			return;
		}
		//FIXME, maybe should be mr here, but tempmr from filled out
		//above is probably better
		//FIXME there's an issue here
		//when we negotiate an offer... not clear yet
#ifdef OLD
		if(mr->periodtime != tempmr->periodtime)
		{
			//FIXME we need probably more checks here actually... 
			//but I'm not even sure real client has them.
			//also, technically should be such checks on startGame
			//as well
			qDebug("Match requests don't match, no pun intended!");
		}
#endif //OLD
		sendStartGame(*tempmr);
	}
	else if(version == decline)
	{
		GameDialog * gameDialogDispatch = getIfGameDialog(*opponent);
		if(!gameDialogDispatch)
		{
			qDebug("No game dialog open to be declined");
		}
		else
			gameDialogDispatch->recvRefuseMatch(GD_REFUSE_DECLINE);
	}
	delete tempmr;
	return;
}

void TygemConnection::handleResumeMatch(unsigned char * msg, unsigned int size)
{
	BoardDispatch * boarddispatch;
	GameData * game;
	int i;
	unsigned short game_number;
	unsigned char name[15];
	name[14] = 0x00;
	QString encoded_name, encoded_name2;
	
	game_number = (msg[0] << 8) + msg[1];	
	boarddispatch = getIfBoardDispatch(game_number);
	if(!boarddispatch)
	{
		qDebug("Unexpected: got 068e for board %d", game_number);
		return;
	}
	game = boarddispatch->getGameData();
	
	//this is addressee
	strncpy((char *)name, (char *)&(msg[4]), 14);
	encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	
	strncpy((char *)name, (char *)&(msg[20]), 14);
	encoded_name2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
	
	//I'm not certain that we're sending this in the right order,
	//but I know from something else we send or just because, official
	//client will get us sent one of these when opponent resumes
	//a game they disconnected from
	//the last byte msg[35] is 0x01
	//made a mistake, that was from us.. 
	printf("***0x068e: ");
	for(i = 0; i < (int)size; i++)
		printf("%02x", msg[i]);
	printf("\n");
	
	if(encoded_name == getUsername() && msg[35] == 0x01)
	{
		/* This might actually be from us ?? */
	}
	//068e 0005 0000
	//7065 7465 7269 7573 0000 0000 0000 0000 
	//696e 7472 7573 696f 6e00 0000 0000 0001
}

//0643
void TygemConnection::handleMatchInvite(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char name[15];
	name[14] = 0x00;
	unsigned short room_number;
	if(size != 60)
	{
		qDebug("MatchInvite of size %d\n", size);
		// probably should print it out and exit FIXME
	}
#ifdef RE_DEBUG
	printf("0x0643: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");

	//tom
	//7065 7465 7269 7573 0000 0000 0000 0008
	//7065 7465 7269 7573 0000 0000 696e 7472
	//7573 696f 6e00 0000 0000 0008 696e 7472
	//7573 696f 6e00 0002 0100 ffff
#endif //RE_DEBUG
	p += 14;
	//invite_byte = p[1];		//probably not
	p += 2;
	p += 12;
#ifdef OLD
	Room * room = getDefaultRoom();
	/* Can't we make a special thing to do all these repetitive string copies? */
	strncpy((char *)name, (char *)p, 14);
	QString encoded_name = QString((char *)name);
	//QString encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 14;
	p += 2;
	strncpy((char *)name, (char *)p, 11);
	QString encoded_name2 = QString((char *)name);
	PlayerListing * player = room->getPlayerListing(encoded_name2);
	if(!player)
	{
		qDebug("Match invite from unknown player");
		return;
	}
#else
	p += 16;
#endif //OLD
	PlayerListing * player = getOrCreatePlayerListingFromRecord((char *)&(msg[28]));
	p += 10;
	p++;
	opponent_is_challenger = (bool)(p[0] - 1);
	p++;
	p += 2;		//possible first, second player settings
	room_number = p[0] + (p[1] << 8);
	if(room_number != 0xffff)
	{
		if(!match_negotiation_state->verifyPlayer(player))
		{
			qDebug("Got room to join from bad player");
			return;
		}
		if(!match_negotiation_state->waitingForRoomNumber())
		{
			qDebug("Got room to join, haven't accepted");
			return;
		}
		// this is created room
		sendJoin(room_number);
		match_negotiation_state->sendJoinRoom(room_number);
		//this also asks for an observer list request which may
		//FIXME not be necessary if its our match, don't think
		//they send it, we're first ones in
		/* Do we need to wait for observe reply?? or no? */
		sendObserversRequest(room_number);
	}
	else if(std::find(decline_all_invitations.begin(), decline_all_invitations.end(), player) != decline_all_invitations.end())
	{
		//client is responsible for this:
		sendMatchInvite(*player, decline_all);
	}
	else if(getBoardDispatches() == 3)
	{
		// if we already have 3 games open, there's no warning, just decline
		sendMatchInvite(*player, decline);	//decline handles getBoardDispatches() 
	}
	else if(match_negotiation_state->inGame())
	{
		sendMatchInvite(*player, alreadyingame);		//doublecheck this isn't block
	}
	else
	{
		if(!match_negotiation_state->newMatchAllowed())
		{
			sendMatchInvite(*player, decline);
			return;
		}
		/* FIXME
		 * yet another point, it seems somehow that the dialog
		 * can close prematurely, somehow and then the seconds
		 * will start low for another invite or something, its weird */
		MatchInviteDialog * mid = new MatchInviteDialog(player->name, player->rank, true);
		int mid_return = mid->exec();
	
		if(mid_return == 1)
			sendMatchInvite(*player, accept);
		else if(mid_return == -1)
			sendMatchInvite(*player, decline);
		else if(mid_return == -2)
		{
			decline_all_invitations.push_back(player);
			sendMatchInvite(*player, decline_all);
		}
	}
}

//0644
void TygemConnection::handleMatchInviteResponse(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned char name[15];
	name[14] = 0x00;
	if(size != 60)
	{
		qDebug("MatchInvite of size %d\n", size);
		// probably should print it out and exit FIXME
	}
	//actually maybe our offer is still screwed up FIXME
#ifdef RE_DEBUG
	printf("*** 0x0644: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	Room * room = getDefaultRoom();
	p += 28;
	/* Can't we make a special thing to do all these repetitive string copies? */
	strncpy((char *)name, (char *)p, 14);
	QString encoded_name = QString((char *)name);
	//QString encoded_name = serverCodec->toUnicode((char *)name, strlen((char *)name));
	p += 14;
	p += 2;
	strncpy((char *)name, (char *)p, 11);
	QString encoded_name2 = QString((char *)name);
	PlayerListing * player = room->getPlayerListing(encoded_name2);
	if(!player)
	{
		qDebug("Match invite from unknown player");
		return;
	}
	if(!match_negotiation_state->verifyPlayer(player))
		return;
	p += 12;
	p++;
	//this byte seems to determine accept versus decline, etc.
	//0x0b is autodecline, 0x00 is offer or decline here, 0x01
	switch(p[0])
	{
		case 0x00:
		{
			QMessageBox mb(tr("Invite declined"), tr("%1 has declined invitation").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
				       QMessageBox::NoButton, QMessageBox::NoButton);
			mb.exec();
		}
			break;
		case 0x01:
			sendCreateRoom();
			return;
			break;
		case 0x0b:
		{
			/* FIXME, I'm thinking 0b may mean that they're already in a game
			 * in addition?*/
			QMessageBox mb(tr("In game?"), tr("%1 is not accepting invitations").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
				       QMessageBox::NoButton, QMessageBox::NoButton);
			mb.exec();
		}
			break;
		case 0x0c:
		{
			/* We shouldn't really get here, this means they have "decline"
			 * set */
			QMessageBox mb(tr("Not open"), tr("%1 is not accepting invitations").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
				       QMessageBox::NoButton, QMessageBox::NoButton);
			mb.exec();
		}
			break;
		case 0x0d:
		{
			QMessageBox mb(tr("Not open"), tr("%1 has the maximum boards (3) open").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
				       QMessageBox::NoButton, QMessageBox::NoButton);
			mb.exec();
		}
			break;
		case 0x0e:
		{
			/* I think you get this if you check the box */
			QMessageBox mb(tr("Invite declined"), tr("%1 has declined all invitations").arg(player->name), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Default,
				       QMessageBox::NoButton, QMessageBox::NoButton);
			mb.exec();
		}
			break;
		default:
#ifdef RE_DEBUG
			qDebug("Received strange byte %02x in match invite response", p[0]);
#endif //RE_DEBUG
			break;
	}
	delete match_negotiation_state;
	match_negotiation_state = 0;
}

//0637
void TygemConnection::handleCreateRoomResponse(unsigned char * msg, unsigned int size)
{
	unsigned short room_number = (msg[0] << 8) + msg[1];
	
	if(size != 12)	//4?
	{
		qDebug("create room response of size: %d", size);
		//return;
	}
	/* FIXME We're going to assume for now that we're getting
	 * this because we received a match response accept and
	 * so we'll just send the join message and pray for rain */
	if(msg[2] != 0x01 || msg[3] != 0xff)
	{
		printf("roomcreate: %02x %02x\n", msg[2], msg[3]);
	}
	//0x0637 room create?: 0129 01ff 0000 0000 0000 0000
#ifdef RE_DEBUG
	printf("0x0637 room create?: ");
	for(unsigned int i = 0; i < size; i++)
		printf("%02x", msg[i]);
	printf("\n");
#endif //RE_DEBUG
	if(!match_negotiation_state->sentCreateRoom())
		return;
	sendObserversRequest(room_number);
	//join should also put us in room with board?
	sendMatchInvite(*match_negotiation_state->getPlayerListing(), create, room_number);
	//not sure of timing of this
	
	//when we offer and its accepted and we create, we don't
	//get a starting message, I think we're supposed to open the
	//board on receipt of 0637 maybe, 0665 send is strange, don't
	//know what that's for, no response anyway, that we know of
	
	/* Try creating board here FIXME with MatchOpened 0639*/
	BoardDispatch * boarddispatch = getBoardDispatch(room_number);
	GameData * aGameData = boarddispatch->getGameData();
	aGameData->number = room_number;
	aGameData->gameMode = modeMatch;
	aGameData->our_invitation = true;
	/*GameListing * gl = room->getGameListing(game_number);
	//FIXME get first time records
	if(gl)
	{
		aGameData->black_name = gl->black_name();
		aGameData->black_rank = gl->black_rank();
		aGameData->white_name = gl->white_name();
		aGameData->white_rank = gl->white_rank();
		// Unfortunately relevant to time records
		//aGameData->white_first_flag = gl->white_first_flag;
	}*/
	boarddispatch->openBoard();
}

/* There's also FIXME FIXME FIXME, 
 * these massive 0x8129 messages at the start of the game, 
 * looks like an opponents records or something... maybe a picture?
 * I have no idea. and a 0x7c29 right before it, short but who knows? 
 * And then after an 0x8629, again, short. check "truegame" and "truegame2".
 * Actually wait, I think these are coming from after a game!! like result
 * messages. */

/* These needs to be more versatile later */
void TygemConnection::onReady(void)
{
	//sendInvitationSettings(true);	//for now
	qDebug("Ready!\n");
	NetworkConnection::onReady();
}

GameData * TygemConnection::getGameData(unsigned int game_id)
{
	/* FIXME, this is weird, I mean I know the caller usually
	 * checks that boarddispatch is solid but... */
	/* I don't think we use this ?!?!? */
	qDebug("getGameData on COC deprecated");
	return getBoardDispatch(game_id)->getGameData();
}

int TygemConnection::time_to_seconds(const QString & time)
{
	QRegExp re = QRegExp("([0-9]{1,2}):([0-9]{1,2})");
	int min, sec;
	
	if(re.indexIn(time) >= 0)
	{
		min = re.cap(1).toInt();
		sec = re.cap(2).toInt();	
	}
	else
		qDebug("Bad time string");
	
	return (60 * min) + sec;
}

void TygemConnection::secondsToDate(unsigned short & year, unsigned char & month, unsigned char & day, unsigned char & hour, unsigned char & minute, unsigned char & second)
{
	unsigned long seconds = time(0);
	year = (seconds / 31536000);
	seconds -= ((year * 31536000) + ((year / 4) * 86400));
	year += 1970;
	bool leap = (year % 4 == 0);
	day = (seconds / 86400);
	seconds -= (day * 86400);
	hour = (seconds / 3600);
	seconds -= (hour * 3600);
	minute = (seconds / 60);
	seconds -= (minute * 60);
	second = seconds;
	unsigned char days_in_each_month[12] = { 31, 28 + leap, 31, 30, 31, 30, 31, 31, 31, 31, 30, 31};
	int i;
	
	for(i = 1; i < 13; i++)
	{
		int sum, j;
		sum = 0;
		for(j = 0; j < i; j++)
			sum += days_in_each_month[j];
		if(day <= sum)
		{
			month = i;
			day -= (sum - days_in_each_month[i - 1]);
			break;
		}
	}
	
	printf("Todays date is: %d %d %d - %d %d %d\n", year, month, day, hour, minute, second);
}
