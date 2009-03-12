#include "eweiqiconnection.h"
#include "codecwarndialog.h"
#include "playergamelistings.h"
#include "gamedata.h"
#include "../room.h"
#include "serverliststorage.h"

EWeiQiConnection::EWeiQiConnection(const QString & user, const QString & pass)
: TygemConnection(user, pass, TypeEWEIQI)
{
	serverCodec = QTextCodec::codecForName("GB2312");
	if(!serverCodec)
	{
		new CodecWarnDialog("GB2312");
		serverCodec = QTextCodec::codecForLocale();
	}
	if(!getServerListStorage().restoreServerList(TypeEWEIQI, serverList))
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

int EWeiQiConnection::requestServerInfo(void)
{
	qDebug("Requesting eWeiQi Server Info");
	if(!openConnection("121.189.9.52", 80))
	{
		qDebug("Can't get server info");
		return -1;
	}
	unsigned int length = 0x96;
	unsigned char * packet = new unsigned char[length];  //term 0x00
	snprintf((char *)packet, length,
		 "GET /service/down_china/livebaduk3.cfg HTTP/1.1\r\n" \
		 "User-Agent: Tygem HTTP\r\n" \
		 "Host: service.tygem.com\r\n" \
		 "Connection: Keep-Alive\r\n" \
		 "Cache-Control: no-cache\r\n\r\n");
#ifdef RE_DEBUG
	for(int i = 0; i < length; i++)
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

QString EWeiQiConnection::getTygemGameRecordQString(GameData * game)
{
	Room * room = getDefaultRoom();
	const PlayerListing * black, * white;
	int black_ordinal, white_ordinal;
	char black_qualifier, white_qualifier;
	QString white_qualifier_string, black_qualifier_string;
	unsigned char black_level, white_level;
	unsigned short year;
	unsigned char month, day, hour, minute, second;
	secondsToDate(year, month, day, hour, minute, second);
	
	black = room->getPlayerListing(game->black_name); 
	if(!black)
	{
		qDebug("Can't get player listing for black: \"%s\"", game->black_name.toLatin1().constData());
		return QString();
	}
	white = room->getPlayerListing(game->white_name); 
	if(!white)
	{
		qDebug("Can't get player listing for white: \"%s\"", game->white_name.toLatin1().constData());
		return QString();
	}
	sscanf(black->rank.toLatin1().constData(), "%d%c", &black_ordinal, &black_qualifier);
	if(black_qualifier == 'k')
	{
		black_level = 0x12 - black_ordinal;
		black_qualifier_string = QString(0xBCB6);
	}
	else if(black_qualifier == 'p')
		black_level = black_ordinal + 0x1a;
	else
	{
		black_level = black_ordinal + 0x11;
		black_qualifier_string = QString(0xB6CE);
	}
	sscanf(white->rank.toLatin1().constData(), "%d%c", &white_ordinal, &white_qualifier);
	if(white_qualifier == 'k')
	{
		white_level = 0x12 - white_ordinal;
		white_qualifier_string = QString(0xBCB6);
	}
	else if(white_qualifier == 'p')
		white_level = white_ordinal + 0x1a;
	else
	{
		white_level = white_ordinal + 0x11;
		white_qualifier_string = QString(0xB6CE);
	}
	/* What we thought was "ascii_name" is clearly a nick in addition to the username */

	QString string;
	string += "\\[GIBOKIND=China\\]\r\n";
	string += "\\[TYPE=0\\]\r\n";
	string += "\\[GAMECONDITION="
				+ QString(0xc8ce) + QString(0xcfc8);
							//c8c3cfc8 seems suffient for a finished
							//game
							//I think cbc0bbee313ab7d6 is possibly
							//some unfinished game or something
	string += "\\[GAMETIME=";
	string += "\\[GAMERESULT=";	//3f3332353f3f	//w + 325 margin ONLY, no color
						//badac6e5cab1bce4caa4 B + T
						//b0d7c6e5d6d0c5cccaa4 W + R
	string += "\\[GAMEZIPSU=60\\]\r\n";
	string += "\\[GAMEDUM=0\\]\r\n";
	string += "\\[GAMEGONGJE=0\\]\r\n";
	string += "\\[GAMETOTALNUM=";	//0xd7dc: 3335cafd" 35?
	string += "\\[SZAUDIO=0\\]\r\n";
	string += "\\[GAMENAME=" + QString(0xd3d1) + QString(0xd2ea) +
			QString(0xb6d4) + QString(0xbed6) + "\\]\r\n";
	/* Be careful here also, no idea what this says, we should
	 * probably leave it blank */
	string += "\\[GAMEDATE=" + QString::number(year) + "-" + 
			QString(0xc4ea) + QString::number(month) +
			QString(0xd4c2) + QString::number(day) +
			QString(0xc8d5) + " " + QString(0xcfc2) +
			QString(0xcee7) + " " + QString::number(hour % 12) +
			(minute < 10 ? ":0" : ":") + QString::number(minute) + "\\]\r\n";
	/* Note that its very likely that one of the above strings contains a PM versus
	 * AM */
	const char eWeiQi_game_place_array[] = {0xde,0xc4,0xb3,0xc7,0x54,0x59,0x47,0x45,0x4d,0xb6,0xd4,0xde,0xc4};
	QByteArray eWeiQi_game_place(eWeiQi_game_place_array, 13);
	string += "\\[GAMEPLACE=" + QString(eWeiQi_game_place) + "\\]\r\n";
	string += "\\[GAMELECNAME=\\]\r\n";
	/* FIXME, note that this is very likely going to be different on the korean
	 * server.  This is for eweiqi really */
	string += "\\[GAMEWHITENAME=" + white->name + "("
				+ QString::number(white_ordinal)
				+ white_qualifier_string + ")\\]\r\n";
	string += "\\[GAMEWHITELEVEL=" + QString::number(white_level)
				+ white_qualifier_string + "\\]\r\n";
	string += "\\[GAMEWHITENICK=" + white->ascii_name + "\\]\r\n";
	string += "\\[GAMEWHITECOUNTRY=" + QString::number(white->country_id) + "\\]\r\n";
	string += "\\[GAMEWAVATA=1\\]\r\n";
	string += "\\[GAMEWIMAGE=\\]\r\n";
	string += "\\[GAMEBLACKNAME=" + black->name + "(" 
				+ QString::number(black_ordinal)
				+ black_qualifier_string + ")\\]\r\n";
	string += "\\[GAMEBLACKLEVEL=" + QString::number(black_level)
				+ black_qualifier_string + "\\]\r\n";
	string += "\\[GAMEBLACKNICK=" + black->ascii_name + "\\]\r\n";
	string += "\\[GAMEBLACKCOUNTRY=" + QString::number(black->country_id) + "\\]\r\n";
	string += "\\[GAMEBAVATA=1\\]\r\n";
	string += "\\[GAMEBIMAGE=\\]\r\n";
	string += "\\[GAMECOMMENT=\\]\r\n";
	//country = 2 = gbkind = gibokind
	string += "\\[GAMEINFOMAIN=GBKIND:2,GTYPE:0,GCDT:1,GTIME:" +
				QString::number(game->maintime) + "-" +
				QString::number(game->periodtime) + "-" +
				QString::number(game->stones_periods) + ",GRLT:0";
	string += ",ZIPSU:60,DUM:0,GONGJE:0,TCNT:143";	//last is moves
	string += ",AUSZ:0\\]\r\n";
	string += "\\[GAMEINFOSUB=GNAMEF:0,GPLCF:0,GNAME:";
	string += "GDATE:" + QString::number(year) + "- " + QString::number(month)
				+ "- " + QString::number(day) + "-" + QString::number(hour)
				+ "-" + QString::number(minute) + "-" + QString::number(second);
	string += ",GPLC:";
	string += ",GCMT:\\]\r\n";
	string += "\\[WUSERINFO=WID:" + white->name + ",WLV:" + QString::number(white_level) + 
				",WNICK:" + white->ascii_name + ",WNCD:" + QString::number(white->country_id) +
				",WAID:,WIMG:\\]\r\n";
	string += "\\[BUSERINFO=BID:" + black->name + ",BLV:" + QString::number(black_level) +
				",BNICK:" + black->ascii_name + ",BNCD:" + QString::number(black->country_id) +
				",BAID:,BIMG:\\]\r\n";
	/* Here, I'm thinking S0 is black wins, S1 is white wins
	 * then again, there's also W1 indicating white win with W0 as white loss */
	/* I've also seen W4 as indicating W + R */
	/* I've also seen S1 W7 as B + T also with Z0 indicating that's time remaining
	 * or something */
	/* T is likely time settings, 30 seconds, 3 periods, 1200 total time maybe (20min)*/
	string += "\\[GAMETAG=S0,R1,D0,G0,W0,Z60,T" +
				QString::number(game->periodtime) + "-" +
				QString::number(game->stones_periods) + "-" +
				QString::number(game->maintime) + ",C" + 
				QString::number(year) + (month < 10 ? ":0" : ":") + 
				QString::number(month) + (day < 10 ? ":0" : ":") + 
				QString::number(day) + (hour < 10 ? ":0" : ":") + 
				QString::number(hour) + (minute < 10 ? ":0" : ":") + 
				QString::number(minute);/* + (second < 10 ? ":0" : ":") +
				QString::number(second);*/	
	string += ",I:" + white->name + ",L:" + QString::number(white_level) +
				",M:" + black->name + ",N:" + QString::number(black_level) + 
				",A:" + white->ascii_name + ",B:" + black->ascii_name + 
				",J:" + QString::number(white->country_id) +
				",K:" + QString::number(black->country_id) + "\\]\r\n";	
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
	
		Note that this suggests first that those two names are not encoded and
		ascii, but an account name and a nickname, maybe multiple nicknames per
		account?  But either way, that could be important.
		Also GAMETAG has some weird fields, and one can see date and time around
		5pm in different places
	
		packet ends 8 0s, possibly padded out to 16 multiple
	
	*/
	return string;
}
