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

#define TYGEM_PROTOCOL_VERSION		0x13		//was 06

#define TYGEM_GAMESINIT			0x12
#define TYGEM_GAMESUPDATE		0x13
#define TYGEM_PLAYERSINIT		0x15
#define TYGEM_PLAYERSUPDATE		0x16

#define TYGEM_ROOMCREATED		0x37
#define TYGEM_BOARDOPENED		0x39
#define TYGEM_SERVERPING		0x4d
#define TYGEM_SERVERPLAYERCOUNTS	0x54
#define TYGEM_OBSERVERSINIT		0x66
#define TYGEM_OBSERVERSUPDATE		0x67
#define TYGEM_MOVE			0x68
#define TYGEM_TIME			0x7d

#define TYGEM_MATCHINVITE		0x43
#define TYGEM_MATCHINVITERESPONSE	0x44
#define TYGEM_MATCHOFFER		0x45
#define TYGEM_MATCHOFFERRESPONSE	0x46

#define TYGEM_SERVERDISCONNECT		0x51

//#define TYGEM_REQUESTCOUNT		0x6b
//#define TYGEM_MATCHRESULT		0x72
#define TYGEM_ENDGAMEMSG		0x7b

#define TYGEM_OPENCONVERSATION		0x33
#define TYGEM_CONVERSATIONREPLY		0x34
#define TYGEM_CONVERSATIONMSG		0x35
#define TYGEM_GAMECHAT			0x61
#define TYGEM_MATCH			0x71
#define TYGEM_MATCHRESULT		0x72

#define TYGEM_RESUMEBROKENMATCH		0x8e

#define TYGEM_FRIENDSBLOCKSLIST		0x96
#define TYGEM_PERSONALCHAT		0x9f

#define TPC(x) (TYGEM_PROTOCOL_VERSION << 8) + x

