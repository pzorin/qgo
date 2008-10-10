#include "orosetphrasechat.h"
#include <QCoreApplication>

std::map<unsigned short, QString> ORO_setphrase;

/* I considered having the actual strings in the header file, but 
 * why?  They're objects with QString and tr, so we only really need
 * them here */
void ORO_setup_setphrases(void)
{
	/* Maybe these should all be in [brackets] ?? Also, wording may be iffy, its iffy on the ORO client */
	ORO_setphrase[OROSP_HELLO] = QCoreApplication::translate("ORO_setphrases", "Hello", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_WELCOME] = QCoreApplication::translate("ORO_setphrases", "Welcome", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HELLOEXC] = QCoreApplication::translate("ORO_setphrases", "Hello!", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_NICETOMEETYOU] = QCoreApplication::translate("ORO_setphrases", "Nice to meet you", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_GLADTOKNOWYOU] = QCoreApplication::translate("ORO_setphrases", "Glad to know you", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_DOYOUWANTQUICKGAME] = QCoreApplication::translate("ORO_setphrases", "Do you want a quick game?", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_DOYOUWANTGAME] = QCoreApplication::translate("ORO_setphrases", "Do you want a game?", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_ILLCREATEMULTROOM] = QCoreApplication::translate("ORO_setphrases", "I'll create multi room.", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_ENTERFORPAIR] = QCoreApplication::translate("ORO_setphrases", "Enter for pair baduk", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_REQUESTMEFORREGGAME] = QCoreApplication::translate("ORO_setphrases", "Request me for regular game", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_ALRIGHTNOPROBLEM] = QCoreApplication::translate("ORO_setphrases", "Alright, no problem", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_YESYOUARERIGHT] = QCoreApplication::translate("ORO_setphrases", "Yes, you are right", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_AGREEWITHYOU] = QCoreApplication::translate("ORO_setphrases", "I agree with you", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_IAGREE] = QCoreApplication::translate("ORO_setphrases", "I agree", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_SUITYOURSELF] = QCoreApplication::translate("ORO_setphrases", "Suit yourself", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_IMSORRY] = QCoreApplication::translate("ORO_setphrases", "I'm sorry", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_SORRYIWASCHATTING] = QCoreApplication::translate("ORO_setphrases", "Sorry, I was chatting", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_IJUSTPLAYEDAGAME] = QCoreApplication::translate("ORO_setphrases", "I just played a game", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_ITSTIMETOGONOW] = QCoreApplication::translate("ORO_setphrases", "I have to go now", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_SHALLWEPLAYNEXTTIME] = QCoreApplication::translate("ORO_setphrases", "Shall we play next time?", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_YOUAREDOINGVERYWELL] = QCoreApplication::translate("ORO_setphrases", "You are doing very well", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_THATISANICEMOVE] = QCoreApplication::translate("ORO_setphrases", "That is a nice move", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_YOUAREWELLMANNERED] = QCoreApplication::translate("ORO_setphrases", "You are well mannered", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_THANKYOU] = QCoreApplication::translate("ORO_setphrases", "Thank you", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_THANKS] = QCoreApplication::translate("ORO_setphrases", "Thanks", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_NOSWEARINGPLEASE] = QCoreApplication::translate("ORO_setphrases", "No swearing please", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_LETSBEPOLITE] = QCoreApplication::translate("ORO_setphrases", "Let's be polite", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_SWEARINGNOTTOLERATED] = QCoreApplication::translate("ORO_setphrases", "Swearing will not be tolerated!", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_NOFLOODINGPLEASE] = QCoreApplication::translate("ORO_setphrases", "No flooding please!", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_DONTBEESCAPER] = QCoreApplication::translate("ORO_setphrases", "Don't be a notorious escaper!", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_PLEASETEACHMEALOT] = QCoreApplication::translate("ORO_setphrases", "Please teach me a lot", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_ILLDOMYBEST] = QCoreApplication::translate("ORO_setphrases", "I'll do my best", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_THANKSFOROPPORTUNITY] = QCoreApplication::translate("ORO_setphrases", "Thanks for the opportunity", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HOPEGAMEISFANTASTIC] = QCoreApplication::translate("ORO_setphrases", "I hope the game will be fantastic", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_GAMEHELLO] = QCoreApplication::translate("ORO_setphrases", "Hello", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_IENJOYEDGAME] = QCoreApplication::translate("ORO_setphrases", "I enjoyed the game", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_CANYOUPLAYONEMORE] = QCoreApplication::translate("ORO_setphrases", "Can you play another?", "", QCoreApplication::DefaultCodec);
	/* This is, "As you played loosely, I could win" */
	ORO_setphrase[OROSP_YOULETMEWIN] = QCoreApplication::translate("ORO_setphrases", "You let me win", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_ILEARNEDALOTFROMYOU] = QCoreApplication::translate("ORO_setphrases", "I learned a lot thanks to you", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_YOUAREPLAYINGVERYWELL] = QCoreApplication::translate("ORO_setphrases", "You are playing very well", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_WHEREDOYOULIVE] = QCoreApplication::translate("ORO_setphrases", "Where do you live?", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HOWOLDAREYOU] = QCoreApplication::translate("ORO_setphrases", "How old are you?", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HOWAREYOUTODAY] = QCoreApplication::translate("ORO_setphrases", "How are you today?", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HOWLONGHAVEYOUPLAYED] = QCoreApplication::translate("ORO_setphrases", "How long have you played?", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HAVEYOUEATENSOMETHING] = QCoreApplication::translate("ORO_setphrases", "Have you eaten something?", "", QCoreApplication::DefaultCodec);

	ORO_setphrase[OROSP_GOODBYE] = QCoreApplication::translate("ORO_setphrases", "Goodbye", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_SEEYOULATER] = QCoreApplication::translate("ORO_setphrases", "See you later", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_SEEYOUNEXTTIME] = QCoreApplication::translate("ORO_setphrases", "See you next time", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_HAVEAGOODTIME] = QCoreApplication::translate("ORO_setphrases", "Have a good time", "", QCoreApplication::DefaultCodec);
	ORO_setphrase[OROSP_IMLEAVINGNOW] = QCoreApplication::translate("ORO_setphrases", "I'm leaving now", "", QCoreApplication::DefaultCodec);

}
