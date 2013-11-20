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


#include "eweiqiconnection.h"
#include "codecwarndialog.h"
#include "playergamelistings.h"
#include "gamedata.h"
#include "room.h"
#include "serverlistdialog.h"

EWeiQiConnection::EWeiQiConnection(const ConnectionCredentials credentials)
: TygemConnection(credentials)
{
	serverCodec = QTextCodec::codecForName(getCodecString());
	if(!serverCodec)
	{
		new CodecWarnDialog(getCodecString());
		serverCodec = QTextCodec::codecForLocale();
    }
}

const char * EWeiQiConnection::getCodecString(void)
{
	return "GB2312";
}

QString EWeiQiConnection::getPlaceString(void)
{
	return "EWeiqi: " + serverList[current_server_index]->name;
}

int EWeiQiConnection::requestServerInfo(void)
{
	qDebug("Requesting eWeiQi Server Info");
    if(!openConnection(hostname, port, NOT_MAIN_CONNECTION))
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
        delete[] packet;
		return -1;
	}
	delete[] packet;
	
    setState(INFO);
	return 0;
}

QByteArray EWeiQiConnection::getTygemGameRecordQByteArray(GameData * game)
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
	white = room->getPlayerListing(game->white_name); 
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
	string += "\\[GIBOKIND=China\\]\r\n";
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
			//3f20 margin 3f3f203f	//black wins
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
	string += "\\[SZAUDIO=0\\]\r\n";
	string += "\\[GAMENAME=";
	string.append("\xd3\xd1\xd2\xea\xb6\xd4\xbe\xd6\\]\r\n");
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
    const char eWeiQi_game_place_array[] = {static_cast<const char>(0xde),static_cast<const char>(0xc4),
                                            static_cast<const char>(0xb3),static_cast<const char>(0xc7),
                                            static_cast<const char>(0x54),static_cast<const char>(0x59),
                                            static_cast<const char>(0x47),static_cast<const char>(0x45),
                                            static_cast<const char>(0x4d),static_cast<const char>(0xb6),
                                            static_cast<const char>(0xd4),static_cast<const char>(0xde),
                                            static_cast<const char>(0xc4)};
	QByteArray eWeiQi_game_place(eWeiQi_game_place_array, 13);
	string += "\\[GAMEPLACE=" + eWeiQi_game_place + "\\]\r\n";
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

	return string;
}
