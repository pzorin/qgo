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


#include <string.h>
#include "lgs.h"
#include "consoledispatch.h"
#include "room.h"
#include "boarddispatch.h"
#include "gamedialog.h"
#include "talk.h"
#include "playergamelistings.h"

LGSConnection::LGSConnection(const ConnectionCredentials credentials) :
IGSConnection(credentials) {}

QString LGSConnection::getPlaceString(void)
{
	return "LGS";
}

void LGSConnection::sendPlayersRequest(void)
{
	sendText("who\r\n");	//no userlist, user gives simple info
}

void LGSConnection::sendToggle(const QString & param, bool val)
{
	if(param == "looking" || param == "open")
		return;  //FIXME
	IGSConnection::sendToggle(param, val);
}

void LGSConnection::requestGameInfo(unsigned int game_id)
{
	char string[20];
	snprintf(string, 20, "refresh %d\r\n", game_id);
	sendText(string);
	snprintf(string, 20, "all %d\r\n", game_id);
	sendText(string);
}

void LGSConnection::onReady(void)
{
	if(firstonReadyCall)
	{
		if(connectionState != PASSWORD_SENT)
			return;
		firstonReadyCall = 0;
        setState(CONNECTED);
		setKeepAlive(600);
		sendPlayersRequest();
		sendGamesRequest();
		//dispatch->recvRoomListing(new RoomListing(0, "Lobby"));
		//sendRoomListRequest();
		//qDebug("Ready!\n");
    }
}

void LGSConnection::handle_info(QString line)
{
	static PlayerListing * statsPlayer;
	BoardDispatch * boarddispatch;
	Room * room = getDefaultRoom();
	MoveRecord * aMove = new MoveRecord();
	static QString memory_str;
	static int memory = 0;
	qDebug("9: %s", line.toLatin1().constData());
	line = line.remove(0, 2).trimmed();	//remove command code
			// status messages
	if (line.contains("Set open to be"))
	{
		bool val = (line.indexOf("False") == -1);
		room->recvToggle(0, val);
	}
	else if (line.contains("Setting you open for matches"))
		room->recvToggle(0, true);
	else if (line.contains("Set looking to be"))
	{
		bool val = (line.indexOf("False") == -1);
		room->recvToggle(1, val);
	}
			// 9 Set quiet to be False.
	else if (line.contains("Set quiet to be"))
	{
		bool val = (line.indexOf("False") == -1);
		room->recvToggle(2, val);
	}
	else if (line.indexOf("Channel") == 0) 
	{
				// channel messages
		QString e1 = element(line, 1, " ");
		if (e1.at(e1.length()-1) == ':')
			e1.truncate(e1.length()-1);
#ifdef FIXME
		int nr = e1.toInt();

		if (line.contains("turned on."))
		{
					// turn on channel
			emit signal_channelinfo(nr, QString("*on*"));
		}
		else if (line.contains("turned off."))
		{
					// turn off channel
			emit signal_channelinfo(nr, QString("*off*"));
		}
		else if (!line.contains("Title:") || gsName == GS_UNKNOWN)
		{
					// keep in memory to parse next line correct
			memory = nr;
			emit signal_channelinfo(memory, line);
			memory_str = "CHANNEL";
		}
#endif //FIXME
//				return IT_OTHER;
	}
#ifdef FIXME
	else if (memory != 0 && !memory_str.isEmpty() && memory_str == "CHANNEL")
	{
		//emit signal_channelinfo(memory, line);

				// reset memory
		memory = 0;
		memory_str = QString();
//				return IT_OTHER;
	}
#endif //FIXME
			// IGS: channelinfo
			// 9 #42 Title: Untitled -- Open
			// 9 #42    broesel    zero815     Granit
	else if (line.contains("#"))
	{
#ifdef FIXME
		int nr = element(line, 0, "#", " ").toInt();
		QString msg = element(line, 0, " ", "EOL");
		emit signal_channelinfo(nr, msg);
#endif //FIXME
	}
			// NNGS: channels
	else if (line.contains("has left channel") || line.contains("has joined channel"))
	{
		QString e1 = element(line, 3, " ", ".");
#ifdef FIXME
		int nr = e1.toInt();

				// turn on channel to get full info
		emit signal_channelinfo(nr, QString("*on*"));
#endif //FIXME
	}
	else if (line.contains("Game is titled:"))
	{
		QString t = element(line, 0, ":", "EOL");
		//emit signal_title(t);
		return;
	}
	else if (line.contains("offers a new komi "))
	{
				// NNGS: 9 physician offers a new komi of 1.5.
		QString komi = element(line, 6, " ");
		if (komi.at(komi.length()-1) == '.')
			komi.truncate(komi.length() - 1);
		QString opponent = element(line, 0, " ");

				// true: request
		//emit signal_komi(opponent, komi, true);
	}
	else if (line.contains("Komi set to"))
	{
				// NNGS: 9 Komi set to -3.5 in match 10
		QString komi = element(line, 3, " ");
		QString game_id = element(line, 6, " ");

				// false: no request
		//emit signal_komi(game_id, komi, false);
	}
	else if (line.contains("wants the komi to be"))
	{
				// IGS: 9 qGoDev wants the komi to be  1.5
		QString komi = element(line, 6, " ");
		QString opponent = element(line, 0, " ");

				// true: request
		//emit signal_komi(opponent, komi, true);
	}
	else if (line.contains("Komi is now set to"))
	{
				// 9 Komi is now set to -3.5. -> oppenent set for our game
		QString komi = element(line, 5, " ");
				// error? "9 Komi is now set to -3.5.9 Komi is now set to -3.5"
		if (komi.contains(".9"))
			komi = komi.left(komi.length() - 2);

				// false: no request
		//emit signal_komi(QString(), komi, false);
	}
	else if (line.contains("Set the komi to"))
	{
				// NNGS: 9 Set the komi to -3.5 - I set for own game
		QString komi = element(line, 4, " ");

				// false: no request
		//emit signal_komi(QString(), komi, false);
	}
	else if (line.contains("game will count"))
	{
				// IGS: 9 Game will not count towards ratings.
				//      9 Game will count towards ratings.
		//emit signal_freegame(false);
	}
	else if (line.contains("game will not count", Qt::CaseInsensitive))
	{
				// IGS: 9 Game will not count towards ratings.
				//      9 Game will count towards ratings.
		//emit signal_freegame(true);
	}
	else if ((line.contains("[") || line.contains("yes")) && line.length() < 6)
	{
				// 9 [20] ... channelinfo
				// 9 yes  ... ayt
		return;
	}
	else if (line.contains("has restarted your game") ||
			line.contains("has restored your old game"))
	{
		if (line.contains("restarted"))
					// memory_str -> see case 15 for continuation
			memory_str = element(line, 0, " ");
	}
	else if (line.contains("I suggest that"))
	{
		memory_str = line;
		return;
	}
	else if (line.contains("and set komi to"))
	{
#ifdef FIXME
				// suggest message ...
		if (!memory_str.isEmpty())
					// something went wrong...
			return;

		line = line.simplified();

		QString p1 = element(memory_str, 3, " ");
		QString p2 = element(memory_str, 6, " ", ":");
		bool p1_play_white = memory_str.contains("play White");

		QString h, k;
		if (line.contains("even game"))
			h = "0";
		else
			h = element(line, 3, " ");

		k = element(line, 9, " ", ".");

		int size = 19;
		if (line.contains("13x13"))
			size = 13;
		else if (line.contains("9x 9"))
		{
			size = 9;
			memory_str = QString();
		}

		if (p1_play_white)
			emit signal_suggest(p1, p2, h, k, size);
		else
			emit signal_suggest(p2, p1, h, k, size);
#endif //FIXME
				return;
	}
			// 9 Match [19x19] in 1 minutes requested with xxxx as White.
			// 9 Use <match xxxx B 19 1 10> or <decline xxxx> to respond.
			// 9 NMatch requested with yfh2test(B 3 19 60 600 25 0 0 0).
			// 9 Use <nmatch yfh2test B 3 19 60 600 25 0 0 0> or <decline yfh2test> to respond.
	else if (line.contains("<decline") && line.contains("match"))
	{
                // false -> not my request: used in mainwin.cpp
		line = element(line, 0, "<", ">");
		MatchRequest * aMatch = new MatchRequest();
		aMatch->opponent = line.section(" ", 1, 1);
		if(line.section(" ",2,2) == "B")
			aMatch->color_request = MatchRequest::BLACK;
		else if(line.section(" ",2,2) == "N")
			aMatch->color_request = MatchRequest::NIGIRI;
		else
			aMatch->color_request = MatchRequest::WHITE;
		if(line.contains("nmatch"))
		{
			aMatch->handicap = line.section(" ",3,3).toInt();
			aMatch->board_size = line.section(" ",4,4).toInt();
					//what kind of time? var name?
			aMatch->maintime = line.section(" ",5,5).toInt();
			aMatch->periodtime = line.section(" ",6,6).toInt();
			aMatch->stones_periods = line.section(" " ,7,7).toInt();
					//what is "nmatch"?
			aMatch->nmatch = true;
			qDebug("nmatch in parser");
		}
		else
		{
			aMatch->board_size = line.section(" ",3,3).toInt();
			aMatch->maintime = line.section(" ",4,4).toInt();
			aMatch->periodtime = line.section(" ",5,5).toInt();
			aMatch->nmatch = false;
		}
		PlayerListing * p = getPlayerListingNeverFail(aMatch->opponent);
        const PlayerListing * us = getOurListing();
		if(us)
		{	
			aMatch->our_name = us->name;
			aMatch->our_rank = us->rank;
		}
		aMatch->their_rank = p->rank;

        GameDialog * gameDialogDispatch = getGameDialog(p);
		gameDialogDispatch->recvRequest(aMatch);
		delete aMatch;
	}
			// 9 Match [5] with guest17 in 1 accepted.
			// 9 Creating match [5] with guest17.
	else if (line.contains("Creating match"))
	{
		QString nr = element(line, 0, "[", "]");
				// maybe there's a blank within the brackets: ...[ 5]...
		QString dummy = element(line, 0, "]", ".").trimmed();
		QString opp = element(dummy, 1, " ");

				// We let the 15 game record message create the board
				//GameDialog * gameDialogDispatch = getGameDialog(opp);
				//gameDialogDispatch->closeAndCreate();
				//GameDialog * gameDialogDispatch = 
				//		getGameDialog(opp);
				//MatchRequest * mr = gameDialogDispatch->getMatchRequest();
				//created_match_request = new MatchRequest(*mr);
				//closeGameDialog(opp);
				////emit signal_matchCreate(nr, opp);
        			// automatic opening of a dialog tab for further conversation
        			////emit signal_talk(opp, "", true);
	}
	else if (line.contains("Match") && line.contains("accepted"))
	{
		QString nr = element(line, 0, "[", "]");
		QString opp = element(line, 3, " ");
				////emit signal_matchCreate(nr, opp);
				
	}
			// 9 frosla withdraws the match offer.
			// 9 guest17 declines your request for a match.	
	else if (line.contains("declines your request for a match") ||
			line.contains("withdraws the match offer"))
	{
		QString opp = element(line, 0, " ");
		PlayerListing * p = getPlayerListingNeverFail(opp);
        GameDialog * gameDialogDispatch = getGameDialog(p);
		gameDialogDispatch->recvRefuseMatch(1);
	}
			//9 yfh2test declines undo
	else if (line.contains("declines undo"))
	{
				// not the cleanest way : we should send this to a message box
		//emit signal_kibitz(0, element(line, 0, " "), line);
		return;
	}
		
			//9 yfh2test left this room
			//9 yfh2test entered this room
	else if (line.contains("this room"))
	{}	//emit signal_refresh(10);				

			//9 Requesting match in 10 min with frosla as Black.
	else if (line.contains("Requesting match in"))
	{
		QString opp = element(line, 6, " ");
		//emit signal_opponentopen(opp);
	}
			// NNGS: 9 Removing @ K8
			// IGS:	9 Removing @ B5
			//     49 Game 265 qGoDev is removing @ B5
	else if (line.contains("emoving @"))
	{
		/* FIXME DOUBLE CHECK!!! */
				/*if (gsName != IGS)
		{
		QString pt = element(line, 2, " ");
		//emit signal_removeStones(pt, 0);
	}*/
		if(protocol_save_int < 0)
		{
			qDebug("Received stone removal message without game in scoring mode");
			return;
		}
		else
		{
			boarddispatch = getBoardDispatch(protocol_save_int);
			QString pt = element(line, 2, " ");
			aMove->flags = MoveRecord::REMOVE;
			aMove->x = (int)(pt.toLatin1().at(0));
			aMove->x -= 'A';
			if(aMove->x < 9)	//no I on IGS
				aMove->x++;
			pt.remove(0,1);
			aMove->y = element(pt, 0, " ").toInt();
			/* Do we need the board size here???*/
			aMove->y = 20 - aMove->y;
			boarddispatch->recvMove(aMove);
			// FIXME Set protocol_save_int to -1???
			delete aMove;
		}
	}
			// 9 You can check your score with the score command, type 'done' when finished.
	else if (line.contains("check your score with the score command"))
	{
//				if (gsName == IGS)
					// IGS: store and wait until game number is known
//					memory_str = QString("rmv@"); // -> continuation see 15
//				else
					////emit signal_removestones(0, 0);
					////emit signal_enterScoreMode();
		memory_str = QString("rmv@");		
	}
			// IGS: 9 Board is restored to what it was when you started scoring
	else if (line.contains("what it was when you"))
	{
		//emit signal_restoreScore();
	}
			// WING: 9 Use <adjourn> to adjourn, or <decline adjourn> to decline.
	else if (line.contains("Use adjourn to") || line.contains("Use <adjourn> to"))
	{
		qDebug("parser->case 9: Use adjourn to");
				////emit signal_requestDialog("adjourn", "decline adjourn", 0, 0);
		boarddispatch = getBoardDispatch(memory);
		boarddispatch->recvRequestAdjourn();
	}
			// 9 frosla requests to pause the game.
	else if (line.contains("requests to pause"))
	{
		//emit signal_requestDialog("pause", 0, 0, 0);
	}
	else if (line.contains("been adjourned"))
	{
				// remove game from list - special case: own game
#ifdef FIXME
		aGame->nr = "@";
		aGame->running = false;

		//emit signal_game(aGame);
#endif //FIXME
		/* There's several after the fact server INFO messages about
		* games adjourning so we only need to close for one of them
		* The 21 message might actually be better FIXME*/
		boarddispatch = getIfBoardDispatch(memory);
		if(boarddispatch)
		{
			boarddispatch->adjournGame();
			closeBoardDispatch(memory);
		}
	}
			// 9 Game 22: frosla vs frosla has adjourned.
	else if (line.contains("has adjourned"))
	{
        int number = element(line, 0, " ", ":").toInt();
        GameListing * aGame = room->getGameListing(number);
        aGame->running = false;

				// for information
				//aGame->Sz = "has adjourned.";

#ifdef OLD
		//emit signal_game(aGame);
//				//emit signal_move(aGame);
#endif //OLD
				// No need to get existing listing because
				// this is just to falsify the listing
		room->recvGameListing(aGame);
	}
			// 9 Removing game 30 from observation list.
	else if (line.contains("from observation list"))
	{
				// is done from qGoIF
				// //emit signal_addToObservationList(-1);
//				aGame->nr = element(line, 2, " ").toInt();
//				aGame->Sz = "-";
//				aGame->running = false;
		//emit signal_observedGameClosed(element(line, 2, " ").toInt());
		return;
	}
			// 9 Adding game to observation list.
	else if (line.contains("to observation list"))
	{
		/* Unfortunately, LGS and IGS have no number here, so
		* we have to either guess or get it from the observe send
		* which is easiest */
		getBoardDispatch(protocol_save_int);
		protocol_save_int = -1;
		return;
	}
			// 9 Games currently being observed:  31, 36, 45.
	else if (line.contains("Games currently being observed"))
	{
		if (line.contains("None"))
		{
			//emit signal_addToObservationList(0);
		}
		else
		{
#ifdef FIXME
					// don't work correct at IGS!!!
			int i = line.count(',');
			qDebug(QString("observing %1 games").arg(i+1).toLatin1());
//					//emit signal_addToObservationList(i+1);
#endif //FIXME
		}

//				return IT_OTHER;
	}
			// 9 1 minutes were added to your opponents clock
	else if (line.contains("minutes were added"))
	{
#ifdef FIXME
		int t = element(line, 0, " ").toInt();
		emit signal_timeAdded(t, false);
#endif //FIXME
	}
			// 9 Your opponent has added 1 minutes to your clock.
	else if (line.contains("opponent has added"))
	{
#ifdef FIXME
		int t = element(line, 4, " ").toInt();
		emit signal_timeAdded(t, true);
#endif //FIXME
	}
			// NNGS: 9 Game clock paused. Use "unpause" to resume.
	else if (line.contains("Game clock paused"))
	{
#ifdef FIXME
		emit signal_timeAdded(-1, true);
#endif //FIXME
	}
			// NNGS: 9 Game clock resumed.
	else if (line.contains("Game clock resumed"))
	{
#ifdef FIXME
		emit signal_timeAdded(-1, false);
#endif //FIXME
	}
			// 9 Increase frosla's time by 1 minute
	else if (line.contains("s time by"))
	{
#ifdef FIXME
		int t = element(line, 4, " ").toInt();
		if (line.contains(getUsername()))
			emit signal_timeAdded(t, true);
		else
			emit signal_timeAdded(t, false);
#endif //FIXME
	}
			// 9 Setting your . to Banana  [text] (idle: 0 minutes)
	else if (line.contains("Setting your . to"))
	{
		QString busy = element(line, 0, "[", "]");
		if (!busy.isEmpty())
		{
			QString player = element(line, 4, " ");
					// true = player
					////emit signal_talk(player, "[" + busy + "]", true);
		}
	}
	/* I don't think we care about the case 9 messages like this.
	 * maybe in order to update the game lists !!! 
	 * but not for the board. */
	else if (line.contains("resigns.")		||
		   line.contains("adjourned.")	||
			//don't intefere with status   
		//line.contains(" : W ", Qt::CaseSensitive)	||
		   //line.contains(" : B ", Qt::CaseSensitive)	||
		   line.contains("forfeits on")	||
		   line.contains("lost by"))
	{
        GameResult * aGameResult;
		int number = element(line, 0, " ", ":").toInt();
        GameListing * aGame = room->getGameListing(number);
		aGame->running = false;
					// for information
		aGame->result = element(line, 4, " ", "}");

		boarddispatch = getIfBoardDispatch(aGame->number);
		/* FIXME: This shouldn't create a new board if
		 * we're not watching it.
		 * Also WING sometimes sends 9 and sometimes sends 21 perhaps
		 * depending on whether its observed or resign versus score/result
		 * we don't want to send double messages.*/
		if(boarddispatch)
		{
			aGameResult = new GameResult();
						
			if(line.contains(" : W ", Qt::CaseSensitive) ||
						line.contains(" : B ", Qt::CaseSensitive))
			{
				aGameResult->result = GameResult::SCORE;

				int posw, posb;
				posw = aGame->result.indexOf("W ");
				posb = aGame->result.indexOf("B ");
				bool wfirst = posw < posb;
				float sc1, sc2;
				sc1 = aGame->result.mid(posw+1, posb-posw-2).toFloat();
					//sc2 = aGame->result.right(aGame->result.length()-posb-1).toFloat();
				/* FIXME This may be right for IGS and wrong for WING */
					//sc2 = aGame->result.mid(posb +1, posb + 6).toFloat();
				sc2 = element(aGame->result, 4, " ").toFloat();
				qDebug("sc1: %f sc2: %f\n", sc1, sc2);
				if(sc1 > sc2)
				{
					aGameResult->winner_score = sc1;
					aGameResult->loser_score = sc2;
				}
				else
				{
					aGameResult->winner_score = sc2;
					aGameResult->loser_score = sc1;

				}

				if (!wfirst)
				{
					int h = posw;
					posw = posb;
					posb = h;
					if(sc2 > sc1)

						aGameResult->winner_color = stoneWhite;
					else
						aGameResult->winner_color = stoneBlack;
				}
				else
				{
					if(sc1 > sc2)
						aGameResult->winner_color = stoneWhite;
					else
						aGameResult->winner_color = stoneBlack;
				}


			}
			else if(line.contains("forfeits on time"))
			{
				aGameResult->result = GameResult::TIME;
				if(line.contains("Black"))
				{
					aGameResult->winner_color = stoneWhite;
				}
				else
				{
					aGameResult->winner_color = stoneBlack;
				}
			}
			else if(line.contains("resign", Qt::CaseInsensitive))
			{
				aGameResult->result = GameResult::RESIGN;
				if(line.contains("Black"))
				{
					aGameResult->winner_color = stoneWhite;
				}
				else
				{
					aGameResult->winner_color = stoneBlack;
				}
			}
			else if(line.contains("lost by"))
			{
				aGameResult->result = GameResult::SCORE;
				if(line.contains("Black"))
				{
					aGameResult->winner_color = stoneWhite;
				}
				else
				{
					aGameResult->winner_color = stoneBlack;

				}

			}
		}


		if (aGame->result.isEmpty())
			aGame->result = "-";
		else if (aGame->result.indexOf(":") != -1)
			aGame->result.remove(0,2);

		if(boarddispatch)
		{
			qDebug("Receiving result!!!");
			boarddispatch->recvResult(aGameResult);
			/* is kibitzing this here what we want?*/
			/* FIXME... should be a shortened list, not the full
			 * result msg kibitz */
			boarddispatch->recvKibitz("", line);
		}
		room->recvGameListing(aGame);
		return;
	}
			// NNGS: 9 Messages: (after 'message' cmd)
			//       8 File
			//       <msg>
			//       8 File
			//       9 Please type "erase" to erase your messages after reading
	else if (line.contains("Messages:"))
	{
				// parse like IGS cmd nr 14: Messages
		memory = 14;
	}
			// IGS:  9 You have a message. To read your messages, type:  message
			// NNGS: 9 You have 1 messages.  Type "messages" to display them
	else if (line.contains("You have") && line.contains("messages"))
	{
				// remove cmd nr
		line = line.trimmed();
		line = line.remove(0, 2);
		getConsoleDispatch()->recvText(line.toLatin1().constData());
		return;
	}
			// 9 Observing game  2 (chmeng vs. myao) :
			// 9        shanghai  9k*           henry 15k  
			// 9 Found 2 observers.
	else if (line.contains("Observing game ", Qt::CaseSensitive))
	{
				// right now: only need for observers of teaching game
				// game number
		bool ok;
		memory = element(line, 2, " ").toInt(&ok);
		if (ok)
		{
			memory_str = "observe";
					// FIXME
			//emit signal_clearObservers(memory); 
			BoardDispatch * boarddispatch = getIfBoardDispatch(memory);
			if(boarddispatch)
			{
				GameData * g = boarddispatch->getGameData();
				g->white_name = element(line, 0, "(", " ");
				g->black_name = element(line, 4, " ", ")");
				boarddispatch->gameDataChanged();
			}
			return;
		}
	}
	else if (!memory_str.isEmpty() && memory_str == "observe" && line.contains("."))
	{
//				QString cnt = element(line, 1, " ");
//				//emit signal_kibitz(memory, "00", "");

		memory = 0;
		memory_str = QString();

		return;
	}
	else if (!memory_str.isEmpty() && memory_str == "observe")
	{
		QString name =  element(line, 0, " ");
		QString rank;
		boarddispatch = getBoardDispatch(memory);
		if(!boarddispatch)
		{
			qDebug("No boarddispatch for observer list\n");
			return;
		}
				
		for (int i = 1; ! name.isEmpty(); i++)
		{
			rank = element(line, i, " ");
			fixRankString(&rank);
					// send as kibitz from "0"
			PlayerListing * p = getPlayerListingNeverFail(name);
			boarddispatch->recvObserver(p, true);
			name = element(line, ++i , " ");
		}

		return;
	}
	else if(line.contains("Found") && line.contains("observers"))
	{
		memory = 0;
		memory_str = QString();
	}
	else if (line.contains("****") && line.contains("Players"))
        return;

			// 9 qGoDev has resigned the game.
	else if (line.contains("has resigned the game"))
	{
		boarddispatch = getBoardDispatch(memory);
		if(!boarddispatch)
		{
			qDebug("No board dispatch for \"resigned the game\"\n");
			return;
		}
#ifdef FIXME
		aGame->running = false;
#endif //FIXME
			
		GameResult * aGameResult = new GameResult();
		aGameResult->loser_name = element(line, 0, " ");
		aGameResult->result = GameResult::RESIGN;
		/* We need to set unknown/none color here */
		boarddispatch->recvResult(aGameResult);
		return;

	}
	else if	(line.contains("has run out of time"))
	{
		boarddispatch = getBoardDispatch(protocol_save_int);
		if(!boarddispatch)
		{
			qDebug("No board dispatch for \"has run out of time\"\n");
			return;
		}
#ifdef FIXME
		aGame->running = false;
#endif //FIXME
		
		GameResult * aGameResult = new GameResult();
		aGameResult->loser_name = element(line, 0, " ");
		aGameResult->result = GameResult::TIME;
		/* We need to set unknown/none color here */
		boarddispatch->recvResult(aGameResult);
		return;

	}



		//9 Player:      yfh2
		//9 Game:        go (1)
		//9 Language:    default
		//9 Rating:      6k*  23
		//9 Rated Games:     21
		//9 Rank:  8k  21
		//9 Wins:        13
		//9 Losses:      16
		//9 Idle Time:  (On server) 0s
		//9 Address:  yfh2@tiscali.fr
		//9 Country:  France
		//9 Reg date: Tue Nov 18 04:01:05 2003
		//9 Info:  yfh2
		//9 Defaults (help defs):  time 0, size 0, byo-yomi time 0, byo-yomi stones 0
		//9 Verbose  Bell  Quiet  Shout  Automail  Open  Looking  Client  Kibitz  Chatter
		//9     Off    On     On     On        On   Off      Off      On      On   On

	else if (line.contains("Player:"))
	{
		QString name = element(line, 1, " ");
		statsPlayer = getPlayerListingNeverFail(name);
#ifdef FIXME
				/* So this would have cleared the structure, but
		* we're just creating a new empty object later.
		* One HUGE FIXME question is what we might be
		* overriding on the other side.  Maybe we need
		* a bit vector specifying what has updated when
		* the message is passed.  So the notion before
		* was that the structure was held until
				* completely filled and then passed up*/
				
		statsPlayer->extInfo = "";
		statsPlayer->won = "";
		statsPlayer->lost = "";
		statsPlayer->country = "";
		statsPlayer->nmatch_settings = "";
		statsPlayer->rank = "";
		statsPlayer->info = "";
		statsPlayer->address = "";
		statsPlayer->play_str = "";
		statsPlayer->obs_str = "";
		statsPlayer->idle = "";
		statsPlayer->rated = "";
				// not sure it is the best way : above code seem to make use of "signal"
				// but we don't need this apparently for handling stats
#endif //FIXME
		protocol_save_string = "STATS";
		return;
	}
			
	else if (line.contains("Address:"))
	{
		statsPlayer->email_address = element(line, 1, " ");
		return;
	}
			
	else if (line.contains("Last Access"))
	{
		statsPlayer->idletime = element(line, 4, " ")+ " " + element(line, 5, " ")+" " + element(line, 6, " ");
		statsPlayer->seconds_idle = idleTimeToSeconds(statsPlayer->idletime);
		return;
	}
				
	else if (line.contains("Rating:"))
	{
		statsPlayer->rank = element(line, 1, " ");
		fixRankString(&(statsPlayer->rank));
		statsPlayer->rank_score = rankToScore(statsPlayer->rank);
		return;
	}
			
	else if (line.contains("Wins:"))
	{
		statsPlayer->wins = element(line, 1, " ").toInt();
		return;         
	}
				
	else if (line.contains("Losses:"))
	{
		statsPlayer->losses = element(line, 1, " ").toInt();
		return;  
	}
			
	else if ((line.contains("Country:"))||(line.contains("From:")))   //IGS || LGS
	{
		statsPlayer->country = element(line, 0, " ","EOL");
		return; 
	}
	else if (line.contains("User Level:")) //LGS
	{
		statsPlayer->extInfo = line;
		return;
	}
			
				
	else if ((line.contains("Info:"))&& !(line.contains("Rank Info:")))
	{
#ifdef FIXME
		if (! statsPlayer->info.isEmpty())
			statsPlayer->info.append("\n");
		statsPlayer->info.append(line);
		//emit signal_statsPlayer(statsPlayer);
#endif //FIXME
		return;          
	}
	else if (line.contains("Idle Time:"))
	{
		statsPlayer->idletime = element(line, 2, " ");
		statsPlayer->seconds_idle = idleTimeToSeconds(statsPlayer->idletime);
		return;
	}
	else if (line.contains("Playing in game:"))       //IGS and LGS
	{
		statsPlayer->playing = element(line, 3, " ").toInt();

		return;
	}
	/*else if (line.contains("(playing game"))    
	{
		//FIXME this is missing!!
		statsPlayer->playing = element(line, 1, " ",":").toInt();
		return;
	}*/	
	else if (line.left(5) == "19x19")     //LGS syntax
	{
		statsPlayer->rank = element(line, 4, " ");
		fixRankString(&(statsPlayer->rank));
		statsPlayer->rank_score = rankToScore(statsPlayer->rank);
		statsPlayer->rated_games = element(line, 7, " ").toInt();
		return;
	}
			
	else if (line.contains("Wins/Losses"))     //LGS syntax
	{
		statsPlayer->wins = element(line, 2, " ").toInt();
		statsPlayer->losses = element(line, 4, " ").toInt();
		return;
	}
			
	else if (line.contains("Last Access"))
	{
		statsPlayer->idletime = "not on";
		return;
	}
				
	else if(line.contains("Full Name"))
	{
		if(statsPlayer)
		{
			qDebug("talk name: %s", statsPlayer->name.toLatin1().constData());
            Talk * talk = getDefaultRoom()->getTalk(statsPlayer);
			if(talk)
				talk->updatePlayerListing();
			statsPlayer = 0;
		}
	}
				
		//9 ROOM 01: FREE GAME ROOM;PSMNYACF;自由対局室;19;1;10;1,19,60,600,30,25,10,60,0
		//9 ROOM 10: SLOW GAME ROOM;PSMNYACF;ｽﾛｰ対局室;19;1;20
		//9 ROOM 92: PANDA OPEN ROOM;X;月例大会;19;10;15
	else if (! line.left(5).compare("ROOM "))
	{

		//emit signal_room(element(line,0," ",";"),(element(line, 1,";")=="X") || (element(line, 1,";")=="P"));
		return;
	}
	else if(line.contains("File"))
		return;
	if (protocol_save_string != "STATS")
		getConsoleDispatch()->recvText(line.toLatin1().constData());
}
#ifdef OLD
void WING_kibitz::handleMsg(QString line)
{
	static QString memory_str;
	static int memory = 0;
	line = line.remove(0, 2).trimmed();
	BoardDispatch * boarddispatch;
	qDebug("kibitz: %s", line.toLatin1().constData());
	if (line.contains("Kibitz"))
	{
				// who is kibitzer
		memory_str = element(line, 0, " ", ":");
				// game number
		memory = element(line, 1, "[", "]").toInt();
	}
	else
	{
		if (memory_str.isEmpty())
					// something went wrong...
			return;
		if(line.contains("Starting observation"))
		{
			QString name =  element(memory_str, 0 , " ");
			QString rank = element(memory_str, 1, "[", "]");
			fixRankString(&rank);
			qDebug("%s %s joining", name.toLatin1().constData(), rank.toLatin1().constData());
					// send as kibitz from "0"
			ObserverListing ob(true, name, rank, rankToScore(rank));
			boarddispatch = getBoardDispatch(memory);
			if(!boarddispatch)
			{
				qDebug("No board dispatch for this game!");
				return;
			}
			boarddispatch->recvObserver(&ob);
			memory = 0;
			memory_str.clear();
			return;
		}
		else if(line.contains("has entered byo-yomi again") ||
		        line.contains("is entering byo-yomi"))
		{
			qDebug("byo-yomi message");
			memory = 0;
			memory_str.clear();
			// just ignore though we could FIXME use it to
			// reset the time
			return;
		}
		else if(memory_str.contains("SgfComment"))
		{
			if(line.contains("dead @"))
			{
				boarddispatch = getBoardDispatch(memory);
				if(!boarddispatch)
				{
					qDebug("No board dispatch for this game!");
					return;
				}
				boarddispatch->recvEnterScoreMode();
				/* FIXME What if we're already in score mode,
				 * what about the double pass signifying this??? */
				MoveRecord * aMove = new MoveRecord();
				aMove->flags = MoveRecord::REMOVE;
				QString point = element(line, 5, " ", "EOL");
				qDebug("Removal point: %s", point.toLatin1().constData());
				
				aMove->x = (int)(point.toLatin1().at(0));
				aMove->x -= 'A';
				point.remove(0,1);
				aMove->y = element(point, 0, " ").toInt();
				GameListing * l = getDefaultRoom()->getGameListing(memory);
				if(!l)
				{
					qDebug("Move for unlisted game");
					delete aMove;
					return;
				}	
				if(l->board_size > 9)
				{
					if(aMove->x < 9)	//no I on IGS
						aMove->x++;
					aMove->y = 20 - aMove->y;
				}
				boarddispatch->recvMove(aMove);
				delete aMove;
			}
			memory = 0;
			memory_str.clear();
			return;
		}
		/* FIXME: Kibitz has this ugly little "->" which we might
		 * want to cut off */
		//emit signal_kibitz(memory, memory_str, line);
		boarddispatch = getBoardDispatch(memory);
		if(boarddispatch)
			boarddispatch->recvKibitz(memory_str, line);
		memory = 0;
		memory_str.clear();
	}
}
#endif //OLD
