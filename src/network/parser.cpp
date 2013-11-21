/*
 *   parser.cpp
 */

/* There are a lot of possibilities for segmentation faults in this
 * file that need to be fixed, places where the dispatch is null */

#include "parser.h"
#include "networkconnection.h"
#include "igsc.h"
#include "networkdispatch.h"
#include "consoledispatch.h"
#include "roomdispatch.h"
#include "boarddispatch.h"
#include "gamedialogdispatch.h"
#include "talkdispatch.h"

// Parsing of Go Server messages
Parser::Parser() //: QObject()//, Misc<QString>()
{
	// generate buffers
	memory = 0;
	created_match_request = 0;
	memory_str = QString();
	myname = QString();

	// init
	gsName = GS_UNKNOWN;
//	cmd = NONE;
	aMove = new MoveRecord();
	aGameRecord = new GameRecord();
	aTime = new TimeRecord();
    aGameResult = new GameResult();
	/* Player needs initalization stuff... and more,
	 * maybe we need to track bit changes ??*/
	gameListA = new QList <unsigned int>();
	gameListB = new QList <unsigned int>();
	scoring_game_id = -1;
}

Parser::~Parser()
{
	delete aMove;
	delete aGameRecord;
	delete aTime;
    delete aGameResult;
	delete gameListA;
	delete gameListB;
}

/* 
 * extratcs and returns the element between 2 delimiters, at a given delimiter count
 */
QString Parser::element(const QString &line, int index, const QString &del1, const QString &del2, bool killblanks)
{
	int len = line.length();
	int idx = index;
	int i;
	QString sub;

	// kill trailing white spaces
	while (/*(int)*/ line[len-1] < 33 && len > 0)
		len--;

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

/*
 * put a line from host to parser
 * if info is recognized, a signal is sent, and, however,
 * the return type indicates the type of information
 */
InfoType Parser::put_line(const QString &txt, class NetworkConnection * connection, GSName gsName)
{
	
	QString line = txt.trimmed();
	int pos;
	class BoardDispatch * boarddispatch = 0;
	class RoomDispatch * roomdispatch = connection->getDefaultRoomDispatch();
	class NetworkDispatch * dispatch = connection->getDefaultDispatch();
	class ConsoleDispatch * console = connection->getConsoleDispatch();
	qDebug(line.toLatin1().constData());
	if (line.length() == 0)
	{
		// skip empty lines but write message if
		// a) not logged in
		// b) help files
		if ((gsName == GS_UNKNOWN) || (!memory_str.isEmpty() && memory_str.contains("File")))
		{
			console->recvText(txt.toLatin1().constData());
			return MESSAGE;
		}

		// white space only
		return WS;
	}

	// skip console commands
	if (line.indexOf(CONSOLECMDPREFIX,0) != -1)
		return NONE;

	// check for connection status
	if (line.indexOf("Connection closed",0,Qt::CaseInsensitive) != -1)
	{
		emit signal_connexionClosed();
		console->recvText(txt.toLatin1().constData());
		gsName = GS_UNKNOWN;
		return IT_OTHER;
	}
	else if (line.indexOf("IGS entry on",0) != -1)
	{
		connection->onReady();
		return SERVERNAME;
	}

	//
	// LOGON MODE ----------------------
	//
	// try to find out server and set mode
#ifdef OLD
	// why would we not know the server type?
	if (gsName == GS_UNKNOWN)
	{
		if (line.indexOf("IGS entry on",0) != -1)
		{
			gsName = IGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("LGS #",0) != -1)
		{
			gsName = LGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("NNGS #",0) != -1)
		{
			gsName = NNGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// suggested by Rod Assard for playing with NNGS version 1.1.14
		if (line.indexOf("Server (NNGS)",0) != -1)
		{
			gsName = NNGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("WING #",0) != -1)
		{
			gsName = WING;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("CTN #",0) != -1)
		{
			gsName = CTN;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// adapted from NNGS, chinese characters
		if (line.indexOf("CWS #",0) != -1 || line.indexOf("==CWS",0) != -1)
		{
			gsName = CWS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// critical: TO BE WATCHED....
		if (line.indexOf("#>",0) != -1)
		{
			gsName = DEFAULT;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// account name
		if (line.indexOf("Your account name is",0) != -1)
		{
			buffer = line.right(line.length() - 21);
			buffer.replace(QRegExp("[\".]"), "");
			emit signal_accname(buffer);
			return ACCOUNT;
		}

		// account name as sent from telnet.cpp
		if (line.indexOf("...sending:") != -1)
		{
			if ((buffer = element(line, 0, "{", "}")) == NULL)
				return IT_OTHER;
			emit signal_accname(buffer);
			return ACCOUNT;
		}

		if ((line.indexOf("guest account",0) != -1) || line.contains("logged in as a guest"))
		{
			emit signal_status(GUEST);
			return STATUS;
		}

		if (line.at(0) != '9' && !memory)
		{
			//emit signal_message(txt);
			console->recvText(txt.toLatin1().constData());
			return MESSAGE;
		}
	}
#endif //OLD

	//
	// LOGON HAS DONE, now parse: ----------------------
	//
	// get command type:
	bool ok;
	int cmd_nr = element(line, 0, " ").toInt(&ok);
	if (!ok && !memory_str.isEmpty() && memory_str.contains("CHANNEL"))
	{
		connection->onReady();
		// special case: channel info
		cmd_nr = 9;
	}
	else if (!ok || !memory_str.isEmpty() && memory_str.contains("File") && !line.contains("File"))
	{
		// memory_str == "File": This is a help message!
		// skip action if entering client mode
		if (line.indexOf("Set client to be True", 0, Qt::CaseInsensitive) != -1)
			return IT_OTHER;

		if (memory == 14)
			// you have message
			emit signal_msgBox(line);
		else
			console->recvText(txt.toLatin1().constData());

		if (line.indexOf("#>") != -1 && !memory_str.isEmpty() && !memory_str.contains("File"))
			return NOCLIENTMODE;

		return MESSAGE;
	}
	else
	{
		// remove command number
		line = line.remove(0, 2).trimmed();
	}
	//qDebug("%s", line.toLatin1().constData());

	// correct cmd_nr for special case; if quiet is set to false a game may not end...
	if (cmd_nr == 9 && line.contains("{Game"))
	{
		qDebug("command changed: 9 -> 21 !!!");
		cmd_nr = 21;
	}

	// case 42 is equal to 7
	if (cmd_nr == 42 && gsName != IGS)
	{
		// treat as game info
		cmd_nr = 7;
		qDebug("command changed: 42 -> 7 !!!");
	}

	// process for different servers has particular solutions
	// command mode -> expect result
	switch (cmd_nr)
	{
		// PROMPT
		

	
// IGS:48 Game 354 qGoDev requests an adjournment
		case 48:
			// have a look at case 9

			break;
// 51 Say in game 583
		case 51:
			// we could get the game id number here and store it
			// its weird that there's also a case 11 message
			break;
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

		case 56:
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



			break;


	}

	return IT_OTHER;
}

unsigned int Parser::idleTimeToSeconds(QString time)
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

void Parser::fixRankString(QString * rank)
{
	if(rank->at(rank->length() - 1) == '*')
		rank->truncate(rank->length() - 1);
	else if(*rank == "NR") {}
	else
		*rank += "?";
}

GSName Parser::get_gsname()
{
	return gsName;
}

void Parser::set_gsname(const GSName gs)
{
	gsName = gs;
}
