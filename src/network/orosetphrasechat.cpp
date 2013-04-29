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


#include "orosetphrasechat.h"
#include <QCoreApplication>

std::map<unsigned short, QString> ORO_setphrase;

/* I considered having the actual strings in the header file, but 
 * why?  They're objects with QString and tr, so we only really need
 * them here */
void ORO_setup_setphrases(void)
{
	/* Maybe these should all be in [brackets] ?? Also, wording may be iffy, its iffy on the ORO client */
	ORO_setphrase[OROSP_HELLO] = QCoreApplication::translate("ORO_setphrases", "Hello", "");
	ORO_setphrase[OROSP_WELCOME] = QCoreApplication::translate("ORO_setphrases", "Welcome", "");
	ORO_setphrase[OROSP_HELLOEXC] = QCoreApplication::translate("ORO_setphrases", "Hello!", "");
	ORO_setphrase[OROSP_NICETOMEETYOU] = QCoreApplication::translate("ORO_setphrases", "Nice to meet you", "");
	ORO_setphrase[OROSP_GLADTOKNOWYOU] = QCoreApplication::translate("ORO_setphrases", "Glad to know you", "");
	ORO_setphrase[OROSP_DOYOUWANTQUICKGAME] = QCoreApplication::translate("ORO_setphrases", "Do you want a quick game?", "");
	ORO_setphrase[OROSP_DOYOUWANTGAME] = QCoreApplication::translate("ORO_setphrases", "Do you want a game?", "");
	ORO_setphrase[OROSP_ILLCREATEMULTROOM] = QCoreApplication::translate("ORO_setphrases", "I'll create multi room.", "");
	ORO_setphrase[OROSP_ENTERFORPAIR] = QCoreApplication::translate("ORO_setphrases", "Enter for pair baduk", "");
	ORO_setphrase[OROSP_REQUESTMEFORREGGAME] = QCoreApplication::translate("ORO_setphrases", "Request me for regular game", "");
	ORO_setphrase[OROSP_ALRIGHTNOPROBLEM] = QCoreApplication::translate("ORO_setphrases", "Alright, no problem", "");
	ORO_setphrase[OROSP_YESYOUARERIGHT] = QCoreApplication::translate("ORO_setphrases", "Yes, you are right", "");
	ORO_setphrase[OROSP_AGREEWITHYOU] = QCoreApplication::translate("ORO_setphrases", "I agree with you", "");
	ORO_setphrase[OROSP_IAGREE] = QCoreApplication::translate("ORO_setphrases", "I agree", "");
	ORO_setphrase[OROSP_SUITYOURSELF] = QCoreApplication::translate("ORO_setphrases", "Suit yourself", "");

	ORO_setphrase[OROSP_IMSORRY] = QCoreApplication::translate("ORO_setphrases", "I'm sorry", "");
	ORO_setphrase[OROSP_SORRYIWASCHATTING] = QCoreApplication::translate("ORO_setphrases", "Sorry, I was chatting", "");
	ORO_setphrase[OROSP_IJUSTPLAYEDAGAME] = QCoreApplication::translate("ORO_setphrases", "I just played a game", "");
	ORO_setphrase[OROSP_ITSTIMETOGONOW] = QCoreApplication::translate("ORO_setphrases", "I have to go now", "");
	ORO_setphrase[OROSP_SHALLWEPLAYNEXTTIME] = QCoreApplication::translate("ORO_setphrases", "Shall we play next time?", "");

	ORO_setphrase[OROSP_YOUAREDOINGVERYWELL] = QCoreApplication::translate("ORO_setphrases", "You are doing very well", "");
	ORO_setphrase[OROSP_THATISANICEMOVE] = QCoreApplication::translate("ORO_setphrases", "That is a nice move", "");
	ORO_setphrase[OROSP_YOUAREWELLMANNERED] = QCoreApplication::translate("ORO_setphrases", "You are well mannered", "");
	ORO_setphrase[OROSP_THANKYOU] = QCoreApplication::translate("ORO_setphrases", "Thank you", "");
	ORO_setphrase[OROSP_THANKS] = QCoreApplication::translate("ORO_setphrases", "Thanks", "");

	ORO_setphrase[OROSP_NOSWEARINGPLEASE] = QCoreApplication::translate("ORO_setphrases", "No swearing please", "");
	ORO_setphrase[OROSP_LETSBEPOLITE] = QCoreApplication::translate("ORO_setphrases", "Let's be polite", "");
	ORO_setphrase[OROSP_SWEARINGNOTTOLERATED] = QCoreApplication::translate("ORO_setphrases", "Swearing will not be tolerated!", "");
	ORO_setphrase[OROSP_NOFLOODINGPLEASE] = QCoreApplication::translate("ORO_setphrases", "No flooding please!", "");
	ORO_setphrase[OROSP_DONTBEESCAPER] = QCoreApplication::translate("ORO_setphrases", "Don't be a notorious escaper!", "");

	ORO_setphrase[OROSP_PLEASETEACHMEALOT] = QCoreApplication::translate("ORO_setphrases", "Please teach me a lot", "");
	ORO_setphrase[OROSP_ILLDOMYBEST] = QCoreApplication::translate("ORO_setphrases", "I'll do my best", "");
	ORO_setphrase[OROSP_THANKSFOROPPORTUNITY] = QCoreApplication::translate("ORO_setphrases", "Thanks for the opportunity", "");
	ORO_setphrase[OROSP_HOPEGAMEISFANTASTIC] = QCoreApplication::translate("ORO_setphrases", "I hope the game will be fantastic", "");
	ORO_setphrase[OROSP_GAMEHELLO] = QCoreApplication::translate("ORO_setphrases", "Hello", "");

	ORO_setphrase[OROSP_IENJOYEDGAME] = QCoreApplication::translate("ORO_setphrases", "I enjoyed the game", "");
	ORO_setphrase[OROSP_CANYOUPLAYONEMORE] = QCoreApplication::translate("ORO_setphrases", "Can you play another?", "");
	/* This is, "As you played loosely, I could win" */
	ORO_setphrase[OROSP_YOULETMEWIN] = QCoreApplication::translate("ORO_setphrases", "You let me win", "");
	ORO_setphrase[OROSP_ILEARNEDALOTFROMYOU] = QCoreApplication::translate("ORO_setphrases", "I learned a lot thanks to you", "");
	ORO_setphrase[OROSP_YOUAREPLAYINGVERYWELL] = QCoreApplication::translate("ORO_setphrases", "You are playing very well", "");

	ORO_setphrase[OROSP_WHEREDOYOULIVE] = QCoreApplication::translate("ORO_setphrases", "Where do you live?", "");
	ORO_setphrase[OROSP_HOWOLDAREYOU] = QCoreApplication::translate("ORO_setphrases", "How old are you?", "");
	ORO_setphrase[OROSP_HOWAREYOUTODAY] = QCoreApplication::translate("ORO_setphrases", "How are you today?", "");
	ORO_setphrase[OROSP_HOWLONGHAVEYOUPLAYED] = QCoreApplication::translate("ORO_setphrases", "How long have you played?", "");
	ORO_setphrase[OROSP_HAVEYOUEATENSOMETHING] = QCoreApplication::translate("ORO_setphrases", "Have you eaten something?", "");

	ORO_setphrase[OROSP_GOODBYE] = QCoreApplication::translate("ORO_setphrases", "Goodbye", "");
	ORO_setphrase[OROSP_SEEYOULATER] = QCoreApplication::translate("ORO_setphrases", "See you later", "");
	ORO_setphrase[OROSP_SEEYOUNEXTTIME] = QCoreApplication::translate("ORO_setphrases", "See you next time", "");
	ORO_setphrase[OROSP_HAVEAGOODTIME] = QCoreApplication::translate("ORO_setphrases", "Have a good time", "");
	ORO_setphrase[OROSP_IMLEAVINGNOW] = QCoreApplication::translate("ORO_setphrases", "I'm leaving now", "");

	ORO_setphrase[OROSP_HOWDOYOUDO] = QCoreApplication::translate("ORO_setphrases", "How do you do?", "");
	ORO_setphrase[OROSP_NICETOMEETEVERYONE] = QCoreApplication::translate("ORO_setphrases", "Nice to meet you everyone", "");
	ORO_setphrase[OROSP_HOWAREYOUEVERYONE] = QCoreApplication::translate("ORO_setphrases", "How are you everyone?", "");
	ORO_setphrase[OROSP_ANYONEFORQUICKGAME] = QCoreApplication::translate("ORO_setphrases", "Anyone want a quick game?", "");
	ORO_setphrase[OROSP_LOOKINGFORFRIENDLY] = QCoreApplication::translate("ORO_setphrases", "I'm looking for a friendly match.", "");
	ORO_setphrase[OROSP_STRONGPLAYERSHANDICAP] = QCoreApplication::translate("ORO_setphrases", "Strong players!  How about playing handicap with me?", "");
	ORO_setphrase[OROSP_ANYONEWANTSTOPLAYWITHME] = QCoreApplication::translate("ORO_setphrases", "Anyone want to play a game with me?", "");
	ORO_setphrase[OROSP_DONTMINDANYGAME] = QCoreApplication::translate("ORO_setphrases", "I don't mind any sorts of games with anyone.  Just request me.", "");
}
