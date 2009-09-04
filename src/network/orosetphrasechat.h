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


#ifndef OROSETPHRASECHAT_H
#define OROSETPHRASECHAT_H

#include <map>
#include <QString>

//ea
#define OROSP_HELLO			0xea60
#define OROSP_WELCOME			0xea61
#define OROSP_HELLOEXC			0xea62
#define OROSP_NICETOMEETYOU		0xea63
#define OROSP_GLADTOKNOWYOU		0xea64
	
#define OROSP_DOYOUWANTQUICKGAME	0xeac5
#define OROSP_DOYOUWANTGAME		0xeac4
#define OROSP_ILLCREATEMULTROOM		0xeac6
#define OROSP_ENTERFORPAIR		0xeac7
#define OROSP_REQUESTMEFORREGGAME	0xeac8
//eb
#define OROSP_ALRIGHTNOPROBLEM		0xeb28
#define OROSP_YESYOUARERIGHT		0xeb29
#define OROSP_AGREEWITHYOU		0xeb2a
#define OROSP_IAGREE			0xeb2b
#define OROSP_SUITYOURSELF		0xeb2c

#define OROSP_IMSORRY			0xeb8c
#define OROSP_SORRYIWASCHATTING		0xeb8d
#define OROSP_IJUSTPLAYEDAGAME		0xeb8e
#define OROSP_ITSTIMETOGONOW		0xeb8f
#define OROSP_SHALLWEPLAYNEXTTIME	0xeb90

#define OROSP_YOUAREDOINGVERYWELL	0xebf0
#define OROSP_THATISANICEMOVE		0xebf1
#define OROSP_YOUAREWELLMANNERED	0xebf2
#define OROSP_THANKYOU			0xebf3
#define OROSP_THANKS			0xebf4
//ec
#define OROSP_NOSWEARINGPLEASE		0xec54
#define OROSP_LETSBEPOLITE		0xec55
#define OROSP_SWEARINGNOTTOLERATED	0xec56
#define OROSP_NOFLOODINGPLEASE		0xec57
#define OROSP_DONTBEESCAPER		0xec58

#define OROSP_PLEASETEACHMEALOT		0xecb8
#define OROSP_ILLDOMYBEST		0xecb9
#define OROSP_THANKSFOROPPORTUNITY	0xecba
#define OROSP_HOPEGAMEISFANTASTIC	0xecbb
#define OROSP_GAMEHELLO			0xecbc
//ed
#define OROSP_IENJOYEDGAME		0xed1c
#define OROSP_CANYOUPLAYONEMORE		0xed1d
#define OROSP_YOULETMEWIN		0xed1e
#define OROSP_ILEARNEDALOTFROMYOU	0xed1f	
#define OROSP_YOUAREPLAYINGVERYWELL	0xed20

#define OROSP_WHEREDOYOULIVE		0xed80
#define OROSP_HOWOLDAREYOU		0xed81
#define OROSP_HOWAREYOUTODAY		0xed82
#define OROSP_HOWLONGHAVEYOUPLAYED	0xed83
#define OROSP_HAVEYOUEATENSOMETHING	0xed84

#define	OROSP_GOODBYE			0xede4
#define OROSP_SEEYOULATER		0xede5
#define OROSP_SEEYOUNEXTTIME		0xede6
#define OROSP_HAVEAGOODTIME		0xede7
#define OROSP_IMLEAVINGNOW		0xede8

#define OROSP_HOWDOYOUDO		0x2713
#define OROSP_NICETOMEETEVERYONE	0x2716
#define OROSP_HOWAREYOUEVERYONE		0x2718
#define OROSP_ANYONEFORQUICKGAME	0x2775
#define OROSP_LOOKINGFORFRIENDLY	0x2779
#define OROSP_STRONGPLAYERSHANDICAP	0x277a
#define OROSP_ANYONEWANTSTOPLAYWITHME	0x2774
#define OROSP_DONTMINDANYGAME		0x277c

//61b0 is directed "Thank you"  //FIXME

extern std::map<unsigned short, QString> ORO_setphrase;

void ORO_setup_setphrases(void);
#endif //OROSETPHRASECHAT_H
