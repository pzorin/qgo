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


#ifndef MESSAGES_H
#define MESSAGES_H
#include <QtCore>
#include "defines.h"
/* This file is defined as a source file in src.pro because of a
 * linker complaint about undefined destructors.  Might be a cleaner
 * solution though. */
/* Also, for the most part these are just constructed and deleted immediately
 * but there's a few that do odd things, like GameResult so it might be
 * worth it to modify constructors/destructors to print out info so 
 * we could make sure there's no memory leaks FIXME */
// this messages won't exist very long so no need to store anything
// they shouldn't be used past the one iteration of a socket read
/* FIXME: I think we imagined that we could do the below because each string
 * was set only at the creation of the structure... but I'm not sure we can...
 * we thought that was how "const" worked then, but maybe its not. It might
 * just depend on that data not getting deleted while a connectionInfo struct
 * uses it... so far this is a safe bet... but its something to worry about.*/

#define tr_macro(x)   QCoreApplication::translate("Game messages", x, "")
	

class PlayerListing;

class MoveRecord
{
public:
	unsigned int number;
#define NOMOVENUMBER		0xffff
	unsigned int x, y;
	StoneColor color;	//maybe we update this with mark class or something later
	// having accept and refuse undo here is a little weird but its almost a kind
	// of move... we'll see how other protocols handle this stuff
	enum Flags { NONE, HANDICAP, PASS, RESIGN, REMOVE, UNREMOVE, UNDO, REQUESTUNDO, REFUSEUNDO, TERRITORY, UNDO_TERRITORY, REMOVE_AREA, UNREMOVE_AREA, DONE_SCORING,
			SETMOVE, BACKWARD, FORWARD, RESETBRANCH, DELETEBRANCH, RESETGAME, TOEND } flags;
	MoveRecord() : number(NOMOVENUMBER), color(stoneNone), flags(NONE) {};
	MoveRecord(Flags f) : number(NOMOVENUMBER), flags(f) {};
	MoveRecord(unsigned int n, Flags f) : number(n), flags(f) {};
	MoveRecord(unsigned int n, unsigned int _x, unsigned int _y, StoneColor c) : number(n), x(_x), y(_y), color(c), flags(NONE) {}; 
};

/* FIXME Clockdisplay doesn't appear to use the periods, stones just
 * makes up for it... perhaps this is okay, boarddispatch::recvTime
 * has been altered as well to just ignore the periods... but its
 * ugly, unclear, we should change this all around.  It is true though
 * that since the server updates us, there's almost no need to know
 * how much is left... except when do we enter byoyomi/period time??? */
class TimeRecord
{
public:
	int time;
	int stones_periods;
	TimeRecord() : time(0), stones_periods(-1) {};
	TimeRecord(int t, int s) : time(t), stones_periods(s) {};
};

class GameResult
{
public:
	unsigned int game_number;
	StoneColor winner_color;	//this needs to be standardized obviously
	QString winner_name;	
	QString loser_name;	
	/* Could be better to do black score, white score, we'll see how this
	 * is used !!!*/
	float winner_score;
	float loser_score;
	enum ResultType { SCORE, RESIGN, TIME, FORFEIT, DRAW, NOGAME, ADJOURNED } result;
	GameResult() :  winner_color(stoneNone), winner_name(QString()), loser_name(QString()), winner_score(0.0), loser_score(0.0) {};
	GameResult(StoneColor c, ResultType r) : winner_color(c), winner_name(QString()), loser_name(QString()), winner_score(0.0), loser_score(0.0), result(r) {};
	QString shortMessage(void)
	{
		QString msg;
		if(winner_color == stoneWhite)
			msg = tr_macro("W+");	
		else
			msg = tr_macro("B+");	

		switch(result)
		{
			case FORFEIT:
				msg += tr_macro("F");
				break;
			case TIME:
				msg += tr_macro("T");
				break;
			case RESIGN:
				msg += tr_macro("R");
				break;
			case SCORE:
				msg += QString::number(winner_score - loser_score); 
				break;
			case DRAW:
				msg = "=";	//FIXME
				break;
			default:
				break;
		}
		return msg;
	}
	QString longMessage(void)
	{
		QString msg;
		float margin = winner_score - loser_score;

		switch(result)
		{
			case FORFEIT:
				if(winner_color == stoneWhite)
				{
					if(loser_name != QString())
						msg = loser_name + tr_macro("(B) forfeits");
					else
						msg = tr_macro("Black forfeits");
				}
				else
				{
					if(loser_name != QString())
						msg = loser_name + tr_macro("(W) forfeits");
					else
						msg = tr_macro("White forfeits");
				}
				break;
			case TIME:
				if(winner_color == stoneWhite)
				{
					if(loser_name != QString())
						msg = loser_name + tr_macro("(B) has lost on time");
					else
						msg = tr_macro("Black has lost on time");
				}
				else
				{
					if(loser_name != QString())
						msg = loser_name + tr_macro("(W) has lost on time");
					else
						msg = tr_macro("White has lost on time");
				}
				break;
			case RESIGN:
				if(winner_color == stoneWhite)
				{
					if(loser_name != QString())
						msg = loser_name + tr_macro("(B) resigns");
					else
						msg = tr_macro("Black resigns");
				}
				else
				{
					if(loser_name != QString())
						msg = loser_name + tr_macro("(W) resigns");
					else
						msg = tr_macro("White resigns");
				}
				break;
				/* FIXME score translation is likely inappropriate given
				 * number and different translation grammars */
			case SCORE:
				if(winner_color == stoneWhite)
				{
					if(winner_name != QString())
						msg = winner_name + tr_macro("(W)");
					else
						msg = tr_macro("White");
					if(margin)
						msg += tr_macro(" wins by ") + \
						QString::number(margin);
					else
						msg += tr_macro(" wins");
				}
				else
				{
					if(winner_name != QString())
						msg = winner_name + tr_macro("(B)");
					else
						msg = tr_macro("Black");
					if(margin)
						msg = tr_macro("Black wins by ") + \
						QString::number(margin);
					else
						msg += tr_macro(" wins");
				}
				break;
			case DRAW:
				msg = tr_macro("Draw");	//FIXME
				break;
			default:
				break;
		}
		return msg;
	}
};

/* Maybe we need consistent names for these messages!!! FIXME*/
class MatchRequest
{
public:
	QString opponent;
	QString our_name;	//for convenience (multiple accounts?)
	QString our_rank;
	QString their_rank;
	enum COLORREQUEST { WHITE = 0, BLACK, NIGIRI } color_request;
	//enum TIMERULES { CANADIAN = 0, BYOYOMI } time_rules;
	TimeSystem timeSystem;
	unsigned int handicap;
	unsigned int board_size;
	float komi;		//what about that french "," FIXME??
	/* I long for some standard time class but I don't know
	 * enough about go time rules to write one yet ... (and maybe its not necessary) */
	// IGS requires maintime to, I think, be a multiple of 60 FIXME
	/* FIXME, shouldn't we move this to stones_periods unified as well ? */
	int maintime;
	int periodtime;
	int stones_periods;
	bool nmatch;		//want to get rid of this!!! but can't...
	int nmatch_timeMax;
	int nmatch_BYMax;
	int nmatch_handicapMax;
	/* There's basically a bunch of stuff that the match request needs
	* to carry even though there's currently no way to change it.  This
	* is because it needs to be the same on the accept/mods */
	unsigned char flags;
	assessType free_rated;
	unsigned short number;
	unsigned short opponent_id;
	bool opponent_is_challenger;	//i.e., he initiated the exchange
	bool challenger_is_black;
	unsigned short last_game_code;
	bool undoAllowed;
	bool rematch;
	bool first_offer;
	MatchRequest() : opponent(""),
		color_request(WHITE),
		timeSystem(byoyomi), 
		handicap(0), 
		board_size(19),
		komi(6.5),
		maintime(0), 
		periodtime(0),
		stones_periods(0), 
		nmatch(0), 
		nmatch_timeMax(0), 
		nmatch_BYMax(0), 
		nmatch_handicapMax(0), 
		free_rated(noREQ), 
		number(0), opponent_id(0), 
		last_game_code(0), undoAllowed(false), rematch(false), first_offer(false) {};
};

/* Confusing name since we use games as rooms with ORO, maybe not an issue */
class RoomListing
{
public:
	int number;
	QString name;
	bool locked;
	RoomListing() : number(0), name(""), locked(false) {};
	RoomListing(int n) : number(n), name("") {};
	RoomListing(QString n) : number(0), name(n) {};
	RoomListing(int n, QString t) : number(n), name(t) {};	
};

class ChannelListing
{
public:
	int number;
	QString name;
	std::vector<PlayerListing *> members;
	ChannelListing() : number(0), name(""){};
	ChannelListing(int n) : number(n), name("") {};
	ChannelListing(QString n) : number(0), name(n) {};
	ChannelListing(int n, QString t) : number(n), name(t) {};
};

class SeekCondition
{
public:
	int number;
	int maintime;
	int periodtime;
	int periods;
	QString bline;
	QString strength_wished;
};

#endif //MESSAGES_H
