#include "tomconnection.h"
#include "room.h"
#include "playergamelistings.h"
#include "gamedata.h"
#include "serverlistdialog.h"
#include "serverliststorage.h"
#include "codecwarndialog.h"
/* Note that Tom either has additional checks on this or simply that one cannot play against
 * the same username meaning that sendMatchOffers may not be received. */


TomConnection::TomConnection(const QString & user, const QString & pass)
: TygemConnection(user, pass, TypeTOM)
{
	serverCodec = QTextCodec::codecForName("GB2312");
	if(!serverCodec)
	{
		new CodecWarnDialog("GB2312");
		serverCodec = QTextCodec::codecForLocale();
	}
	if(!getServerListStorage().restoreServerList(TypeTOM, serverList))
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

int TomConnection::requestServerInfo(void)
{
	qDebug("Requesting Tom Server Info");
	if(!openConnection("61.135.158.147", 80, NOT_MAIN_CONNECTION))
	{
		qDebug("Can't get server info");
		return -1;
	}
	unsigned int length = 0x94;
	unsigned char * packet = new unsigned char[length];  //term 0x00
	snprintf((char *)packet, length,
		 "GET /service/files/livebaduk3.cfg HTTP/1.1\r\n" \
		 "User-Agent: Tygem HTTP\r\n" \
		 "Host: duiyi.sports.tom.com\r\n" \
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

/* We need to write a tygem parser that doesn't need to be different for tom FIXME */
void TomConnection::handleServerInfo(unsigned char * msg, unsigned int length)
{
	char * p = (char *)msg;
	int i, j;
	ServerItem * si;

#ifdef RE_DEBUG
	for(i = 0; i < length; i++)
		printf("%c", p[i]);
	printf("\n");
#endif //RE_DEBUG
	
	while(p < ((char *)msg + length))
	{
		while(p < ((char *)msg + length - 1) && (p[0] != '\r' || p[1] != '\n'))
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
			//TOM has only 2 [], name and ip
			if(p[1] == '_')
				p++;
			p += 8;
			unsigned char ipaddr[16];
			unsigned char name[20];
			i = 0;
			while(i < 2 && p < ((char *)msg + length))
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
		closeConnection(false);
		connectionState = CANCELED;
		//if(dispatch)
		//	dispatch->onError();	//not great... FIXME
		return;
	}
	//sendLogin();
}

QByteArray TomConnection::getTygemGameRecordQByteArray(GameData * game)
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
	sscanf(black->rank.toLatin1().constData(), "%d%c", &black_ordinal, &black_qualifier);
	if(black_qualifier == 'k')
	{
		black_level = 0x12 - black_ordinal;
		black_qualifier_string.append("\xbc\xb6");
	}
	else if(black_qualifier == 'p')
		black_level = black_ordinal + 0x1a;
	else
	{
		black_level = black_ordinal + 0x11;
		//double check that this isn't the pro tag?
		black_qualifier_string.append("\xb6\xce");
	}
	sscanf(white->rank.toLatin1().constData(), "%d%c", &white_ordinal, &white_qualifier);
	if(white_qualifier == 'k')
	{
		white_level = 0x12 - white_ordinal;
		white_qualifier_string.append("\xbc\xb6");
	}
	else if(white_qualifier == 'p')
		white_level = white_ordinal + 0x1a;
	else
	{
		white_level = white_ordinal + 0x11;
		white_qualifier_string.append("\xb6\xce");
	}
	
	QByteArray string;
	/* GIBOKIND and GAMELECNAME and maybe some others are missing
	 * from Tom record, so there's a question of whether that's only
	 * in the 0672s and is left out of the records or whether that's
	 * just not part of Tom.  I do get the feeling that, except for
	 * a few particular entries, we can be pretty easy going with
	 * this */
	//string += "\\[GIBOKIND=China\\]\r\n";	//I don't see this in file, necessary?
	string += "\\[TYPE=0\\]\r\n";
	string += "\\[GAMECONDITION=";
	string.append("\xc8\xce\xcf\xc8");
							//c8c3cfc8 seems suffient for a finished
							//game
							//I think cbc0bbee313ab7d6 is possibly
							//some unfinished game or something
	string += "\\]\r\n";
	string += "\\[GAMETIME=";
	string += "\\]\r\n";
	string += "\\[GAMERESULT=";	//3f3332353f3f	//w + 325 margin ONLY, no color
						//badac6e5cab1bce4caa4 B + T
						//b0d7c6e5d6d0c5cccaa4 W + R
						//bada38d4d6332f34d7d3caa4   B + margin?
	switch(game->fullresult->result)
	{
		case GameResult::DRAW:
			//string += QString(0xb9ab) + QString(0xbdc2) + QString(0xbace);
			break;
		case GameResult::FORFEIT:
			//c8e6 20 bdc3 b0a3 20 c3ca b0fa bdc2
			
			break;
		case GameResult::TIME:
			break;
		case GameResult::RESIGN:
			if(game->fullresult->winner_color == stoneWhite)
			{
				//string += QString(0xb9e9);
			}
			else
			{
				//string += QString(0xc8e6);
			}
			//string += " " + QString(0xbad2) + QString(0xb0e8) + QString(0xbdc2);
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
			string += QByteArray::number(margin);
			//string += QString(0x3f) + " " + QByteArray::number(margin) +
			//		QString(0x3f3f) + " " + QString(0x3f);
			}
			break;
		default:
			break;
	}
	string += "\\]\r\n";
	string += "\\[GAMEZIPSU=" + QByteArray::number(margin) + "\\]\r\n";
	string += "\\[GAMEDUM=0\\]\r\n";
	string += "\\[GAMEGONGJE=0\\]\r\n";
	string += "\\[GAMETOTALNUM=";	//0xd7dc: 3335cafd" 35?
	string += " " + QByteArray::number(game->moves);
	string += "\\]\r\n";
	string += "\\[SZAUDIO=0\\]\r\n";
	string += "\\[GAMENAME=";
	string.append("\xd3\xd1\xd2\xea\xb6\xd4\xbe\xd6\\]\r\n");
			//cdfd bdb5 bcb6 b6d4 bed6 5c5d   //a broadcast game I saw
	/* Be careful here also, no idea what this says, we should
	 * probably leave it blank */
	string += "\\[GAMEDATE=" + QByteArray::number(year) + "-";
	string.append("\xc4\xea");
	string += QByteArray::number(month) +
	string.append("xd4\xc2");
	string += QByteArray::number(day);
	string.append("\xc8\xd5");
	string += "  "; /*+ QString(0xcfc2) +
			QString(0xcee7) */
	string += " " + QByteArray::number(hour % 12) +
			(minute < 10 ? ":0" : ":") + QByteArray::number(minute) + "\\]\r\n";
	/* Note that its very likely that one of the above strings contains a PM versus
	 * AM */
	const char tom_game_place_array[] = {0x54, 0x6f, 0x6d, 0xb6, 0xd4, 0xde, 0xc4};
	QByteArray tom_game_place(tom_game_place_array, 7);
	string += "\\[GAMEPLACE=" + tom_game_place + "\\]\r\n";
	string += "\\[GAMELECNAME=\\]\r\n";
	/* FIXME, note that this is very likely going to be different on the korean
	 * server.  This is for eweiqi really */
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
	string += ",AUSZ:0\\]\r\n";
	string += "\\[GAMEINFOSUB=GNAMEF:0,GPLCF:0,GNAME:";
	string += "GDATE:" + QByteArray::number(year) + "- " + QByteArray::number(month)
			+ "- " + QByteArray::number(day) + "-" + QByteArray::number(hour)
			+ "-" + QByteArray::number(minute) + "-" + QByteArray::number(second);
	string += ",GPLC:";
	string += ",GCMT:\\]\r\n";
	string += "\\[WUSERINFO=WID:" + serverCodec->fromUnicode(white->notnickname) + ",WLV:" + QByteArray::number(white_level) + 
			",WNICK:" + serverCodec->fromUnicode(white->name) + ",WNCD:" + QByteArray::number(white->country_id) +
				",WAID:60001,WIMG:\\]\r\n";
	string += "\\[BUSERINFO=BID:" + serverCodec->fromUnicode(black->notnickname) + ",BLV:" + QByteArray::number(black_level) +
			",BNICK:" + serverCodec->fromUnicode(black->name) + ",BNCD:" + QByteArray::number(black->country_id) +
				",BAID:60001,BIMG:\\]\r\n";
	/* Here, I'm thinking S0 is black wins, S1 is white wins
	 * then again, there's also W1 indicating white win with W0 as white loss */
	/* I've also seen W4 as indicating W + R */
	/* I've also seen S1 W7 as B + T also with Z0 indicating that's time remaining
	 * or something */
	/* T is likely time settings, 30 seconds, 3 periods, 1200 total time maybe (20min)*/
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

/* It looks like Tom doesn't have upgraded servers */
#ifdef RE_DEBUG
//#define GAMELIST_DEBUG
#endif //RE_DEBUG
//0612
void TomConnection::handleGamesList(unsigned char * msg, unsigned int size)
{
	unsigned char * p = msg;
	unsigned int number_of_games;
	unsigned int id;
	unsigned int name_length;
	unsigned char name[15];
	unsigned char flags;
	Room * room = getDefaultRoom();
	QString encoded_nameA2, encoded_nameB2, rankA, rankB;
	QString encoded_nameA, encoded_nameB;
	GameListing * aGameListing;
	GameListing * ag = new GameListing();
	
	ag->running = true;
	name[14] = 0x00;
	
	number_of_games = (p[0] << 8) + p[1];
	p += 4;
	while(p < (msg + size - 0x48) || (p < (msg + size - 3) && p[2] == 0x01))
	{
		id = (p[0] << 8) + p[1];
#ifdef GAMELIST_DEBUG
		printf("Id: %d:\n", id);
#endif //GAMELIST_DEBUG
		aGameListing = room->getGameListing(id);
		if(!aGameListing)
			aGameListing = ag;
		aGameListing->number = id;
		if(p[2] == 0x01)
		{
#ifdef RE_DEBUG
			// might mean new game?  seems to follow the record
			// for that id, or might mean game over
			// they come up fast but I'm pretty sure this
			// means the game is ended.
//#ifdef GAMELIST_DEBUG
#ifndef GAMELIST_DEBUG
			printf("Id: %d:\n", id);
#endif // !GAMELIST_DEBUG
			printf("is ended!!!\n");
			for(int i = 0; i < 4; i++)
				printf("%c", p[i]);
			printf("\n");
//#endif //GAMELIST_DEBUG
#endif //RE_DEBUG
			p += 4;
			aGameListing->running = false;
			room->recvGameListing(aGameListing);
			continue;
		}
#ifdef GAMELIST_DEBUG
		for(int i = 0; i < 0x48; i++)
		{
			printf("%c", p[i]);	
		}
		printf("\n");
		for(int i = 0; i < 0x48; i++)
		{
			printf("%02x", p[i]);	
		}
		printf("\n");
#endif //GAMELIST_DEBUG
		p += 2;
		//0000 0001 0002 0000 ff00 7978 7973 7a00 
		p += 2;
		p++;
		//impacts time
		//there's also 0x30, I think 0x50, etc.. issues here
		aGameListing->white_first_flag = p[0] & 0x01;
		/* 04 means done, 06 is waiting room or looking, 01 and 00
		 * probably mean playing */
		aGameListing->FR = QString::number(p[0], 16);
		p++;
		aGameListing->observers = (p[0] << 8) + p[1];
		//We have an issue here I think FIXME with observer numbers
		p += 2;
		p++;
		//aGameListing->FR = "";
		p += 2;
		flags = p[0];
		//0000 0001 0001 ffff ff00 7479633435320000000000000000001274796334353200003800000200000000000000000000000000000000003337323
		//13535380000000000000003

		
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
#ifdef GAMELIST_DEBUG
		printf("%s vs.", name);
#endif //GAMELIST_DEBUG
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
#ifdef GAMELIST_DEBUG
			printf(" %s\n", name);
#endif //GAMELIST_DEBUG
			encoded_nameB2 = serverCodec->toUnicode((char *)name, strlen((char *)name));
		}
		p += 12;
		p++;
		//this p[0] should be game status
		aGameListing->FR += QString::number(p[0], 16);
		p++;
		p++;
		name_length = p[0];
		p++;
		p += name_length;
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
	}
	delete ag;
}
