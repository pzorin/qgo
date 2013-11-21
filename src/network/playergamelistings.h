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


#ifndef PLAYERGAMELISTING_H
#define PLAYERGAMELISTING_H

#include <QString>

class GameData;

/* Private constructors and destructors ensure that only the
 * friend class Room
 * can create or destroy instances of these classes */

class PlayerListing
{
private:
    PlayerListing() : id(0),
    online(false),
    name(""),
    notnickname(""),
    info(""),
    idletime(""),
    seconds_idle(0),
    rank(""),
    rank_score(0),
    country(""),
    wins(0),
    losses(0),
    rated_games(0),
    observing(0),
    playing(0),
    extInfo(""),
    email_address(""),
    nmatch(0),
    nmatch_handicapMin(0),
    specialbyte(0),
    pro(false),
    dialog_opened(false),
    game_dialog_opened(false),
    friendWatchType(none),
    notify(false),
    hidden(false) {}
    ~PlayerListing() {}
    friend class Room;
public:
	unsigned short id;
	bool online;
	QString name;
	QString notnickname;
	QString info;
	QString idletime;	
	unsigned int seconds_idle;		//for sorting
	//maybe should be in int form, but we just display, no calculations
	QString rank;
	unsigned int rank_score;		//for sorting, rating points
	QString country;
	unsigned char country_id;
	unsigned int wins;
	unsigned int losses;
	unsigned int rated_games;
	unsigned int observing;			//used by IGS and ORO, so not ready to get rid of it
	std::vector <unsigned short> room_list; 	//used by Tygem et al.
	unsigned int playing;
	QString extInfo;
	QString email_address;

	/* I should really use MatchRequest structure somehow for this: FIXME */
	bool nmatch;
	TimeSystem nmatch_timeSystem;
	bool nmatch_white, nmatch_black, nmatch_nigiri;
	int nmatch_timeMin, nmatch_timeMax;
	int nmatch_BYMin, nmatch_BYMax;
	int nmatch_stonesMin, nmatch_stonesMax;
	QString nmatch_settings;
	int nmatch_handicapMin, nmatch_handicapMax;
	unsigned char specialbyte;		//ORO weirdness personalchat
	bool pro;
	bool dialog_opened;
	bool game_dialog_opened;
	enum FriendWatchType { none, friended, watched, blocked } friendWatchType;
	bool notify;
    bool hidden;
};

/* We need to alter copy constructor to have and respect a bit field.
 * this is heavy, but I think its necessary... unless we get the
 * existing listing before change and update.*/
class GameListing
{
private:
    GameListing() : running(false),
    moves(0),
    board_size(19),
    handicap(0),
    komi(6.5),
    white(0),
    black(0),
    _white_name("-"),
    _black_name("-"),
    _white_rank("-"),
    _black_rank("-"),
    _white_rank_score(0),
    _black_rank_score(0),
    observers(0),
    result(""),
      By(""), FR(""), comment(""),
    flags(IN_PROGRESS),
    rated(0),
    owner_id(0),
    isRoomOnly(false),
    isBroadcast(false),
    isBetting(false),
    isLocked(false),
    gameData(NULL) {}
    ~GameListing() {}
    friend class Room;
public:
    bool running;
	unsigned int number;
	unsigned short game_code;	//ORO join games code FIXME
	unsigned int moves;
	unsigned int board_size;
	unsigned int handicap;
	float komi;
	PlayerListing * white;
	PlayerListing * black;
	QString _white_name;
	QString _black_name;
	QString _white_rank;
	QString _black_rank;
	unsigned int _white_rank_score;
    unsigned int _black_rank_score;
	const QString & white_name(void) const
    {
        if(white)
			return white->name;
        else
			return _white_name;
    }
	const QString & white_rank(void) const
    {
        if(white)
			return white->rank;
        else
			return _white_rank;
    }
	unsigned int white_rank_score(void) const		//for sorting
	{
        if(white)
			return white->rank_score;
        else
			return _white_rank_score;
    }
	const QString & black_name(void) const
	{
        if(black)
			return black->name;
        else
			return _black_name;
    }
	const QString & black_rank(void) const
	{
        if(black)
			return black->rank;
        else
			return _black_rank;
    }
	unsigned int black_rank_score(void) const	//for sorting
	{
		if(black)
			return black->rank_score;
		else
			return _black_rank_score;
    }

	unsigned int observers;
	QString result;
	GameMode gameType;		//this needs to be consistent
	QString By;			//these two need to be changed somehow
	QString FR;
	QString comment;
	/* Other possibilities for flags:
	 * NO_FLAG, BROADCAST, CHAT_ROOM */
	enum Flags { BLACK_WON, WHITE_WON, IN_PROGRESS, REVIEW, LOOKING } flags;
	bool rated;
	unsigned short owner_id;	//this is if we're treating them as rooms
	bool isRoomOnly;
	bool isBroadcast;
	bool isBetting;
	bool isLocked;
	bool white_first_flag;
	std::vector <PlayerListing *> observer_list;
	GameData * gameData;
	/* Also need byomi time, one color go, running, private, flags, ranked, etc.*/
};
#endif //PLAYERGAMELISTINGS_H
