#ifndef PLAYERGAMELISTING_H
#define PLAYER_GAMELISTING_H

struct PlayerListing
{
	unsigned short id;
	bool online;
	QString name;
	QString ascii_name;	//in case two names supplied
	QString info;
	QString idletime;	
	unsigned int seconds_idle;		//for sorting
	//maybe should be in int form, but we just display, no calculations
	QString rank;
	unsigned int rank_score;		//for sorting, rating points
	QString country;
	unsigned int wins;
	unsigned int losses;
	unsigned int rated_games;
	int observing;			//used by IGS and ORO, so not ready to get rid of it			
	std::vector <unsigned short> room_list; 	//used by Tygem et al.
	int playing;
	QString extInfo;
	QString email_address;

	/* I should really use MatchRequest structure for this
	* but I don't want to allocate another object inside of this here,
	* maybe changes this later once I understand it better.  FIXME */
	bool nmatch;
	TimeSystem nmatch_timeSystem;
	bool nmatch_white, nmatch_black, nmatch_nigiri;
	int nmatch_timeMin, nmatch_timeMax;
	int nmatch_BYMin, nmatch_BYMax;
	int nmatch_stonesMin, nmatch_stonesMax;
	QString nmatch_settings;
	int nmatch_handicapMin, nmatch_handicapMax;
	unsigned char specialbyte;		//ORO weirdness personalchat FIXME
	bool pro;
	PlayerListing() : id(0), 
	online(0), 
	name(0), 
	ascii_name(0),
	info(0), 
	idletime(0), 
	seconds_idle(0),
	rank(0),
	rank_score(0),
	country(0), 
	wins(0), 
	losses(0),
	rated_games(0),
	observing(0),
	playing(0), 
	extInfo(0),
	email_address(0),
	specialbyte(0),
	pro(false){};
	// there's also some setttings as well as match requirements
	//PlayerListing(const QString* n, const char * i, const char * r, const char * c, unsigned int w, unsigned int l, unsigned o) : name(n), idletime(i), rank(r), country(c), wins(w), losses(l), observing_str() {};
	//~PlayerListing();
};

/* We need to alter copy constructor to have and respect a bit field.
 * this is heavy, but I think its necessary... unless we get the
 * existing listing before change and update.*/
struct GameListing
{
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
	/* FIXME neither const or & removal kept the below from
	* crashing */
	const QString & white_name(void) const
	{
		if(white)
			return white->name;
		else
			return _white_name;
	};
	const QString & white_rank(void) const
	{
		if(white)
			return white->rank;
		else
			return _white_rank;
	};
	unsigned int white_rank_score(void) const		//for sorting
	{
		if(white)
			return white->rank_score;
		else
			return _white_rank_score;
	};
	const QString & black_name(void) const
	{
		if(black)
			return black->name;
		else
			return _black_name;
	};
	const QString & black_rank(void) const
	{
		if(black)
			return black->rank;
		else
			return _black_rank;
	};
	unsigned int black_rank_score(void) const	//for sorting
	{
		if(black)
			return black->rank_score;
		else
			return _black_rank_score;
	};
	unsigned int observers;
	QString result;
	GameMode gameType;		//this needs to be consistent
	QString By;			//these two need to be changed somehow
	QString FR;
	bool rated;
	unsigned short owner_id;	//this is if we're treating them as rooms
	bool isRoomOnly;
	bool isBroadcast;
	bool isBetting;
	bool isLocked;
	bool white_first_flag;
	std::vector <PlayerListing *> observer_list;
	/* Also need byomi time, one color go, running, private, flags, ranked, etc.*/
	GameListing() : running(0), 
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
	result(0), 
	rated(0), 
	owner_id(0), 
	isRoomOnly(false),
	isBroadcast(false),
	isBetting(false),
	isLocked(false) {};
};
#endif //PLAYERGAMELISTINGS_H
