#ifndef GAMEDATA_H
#define GAMEDATA_H

/*
* Global game data structure
*/

class GameResult;

class GameData
{
	public:
		GameData() :
			 white_name(QObject::tr("White")),
			 black_name(QObject::tr("Black")),
			 white_rank(""),
			 black_rank(""),
			 number(0),
			 moves(0),
			 board_size(19),
			 handicap(0),
			 komi(5.5),
			 timeSystem(canadian),
			 //byoTime(0),
			 //byoPeriods(0),
			 //byoStones(25),
			 timelimit(0),
			 maintime(0),
			 periodtime(0),
			 stones_periods(0),
			 white_prisoners(0),
			 black_prisoners(0),
			 result(""),
			 fullresult(0),
			 gameMode(modeUndefined),
			 game_code(0),
			 nigiriToBeSettled(false),
			 undoAllowed(false),
			 oneColorGo(false),
			 style(1),
			 date(""),
			 place(""),
			 copyright(""),
			 codec(""),
			 gameName(""),
			 fileName(""),
			 overtime(""),
			 free_rated(noREQ),
			 move_list_received(false),
			 white_first_flag(false),
			 our_invitation(false),
			 opponentdropcount(0) {};
		GameData(GameData *d)
		{
			if (d)
			{
				white_name = d->white_name;
				black_name = d->black_name;
				white_rank = d->white_rank;
				black_rank = d->black_rank;
				moves = d->moves;
				result = d->result;
				fullresult = d->fullresult;
				date = d->date;
				place = d->place;
				copyright = d->copyright;
				codec = d->codec;
				gameName = d->gameName;
				fileName = d->fileName;
				overtime = d->overtime;
				board_size = d->board_size;
				handicap = d->handicap;
				number = d->number;
				//byoTime = d->byoTime;
				//byoPeriods = d->byoPeriods;
				//byoStones = d->byoStones;
				timelimit = d->timelimit;
				style = d->style;
				komi = d->komi;
				free_rated = d->free_rated;
				timeSystem = d->timeSystem;
				oneColorGo = d->oneColorGo;
				maintime = d->maintime;
				stones_periods = d->stones_periods;
				periodtime = d->periodtime;
				white_prisoners = d->white_prisoners;
				black_prisoners = d->black_prisoners;
				nigiriToBeSettled = d->nigiriToBeSettled;
				undoAllowed = d->undoAllowed;
				move_list_received = d->move_list_received;
				white_first_flag = d->white_first_flag;
				our_invitation = d->our_invitation;
				opponentdropcount = d->opponentdropcount;
				game_code = d->game_code;
				gameMode = d->gameMode;
			}
		}

		~GameData() {};
		
		QString white_name;
		QString black_name;
		QString white_rank;
		QString black_rank;
		
		bool running;
		unsigned int number;
		unsigned int moves;
		unsigned int board_size;
		unsigned int handicap;
		float komi;
		
		unsigned int observers;
				
		TimeSystem timeSystem;
		int timelimit;	//what is this?
		int maintime, periodtime, stones_periods;
		unsigned int white_prisoners;
		unsigned int white_stones;
		QString white_time;	
		unsigned int black_prisoners;
		unsigned int black_stones;
		QString black_time;
		
		QString result;
		GameResult * fullresult;
	//enum GameType { UNKNOWN, TEACHING, FREE, RANKED, DEMONSTRATION } gameType;
		QString By;			//these two need to be changed somehow
		QString FR;
		GameMode gameMode;
		//struct BoardRecord * board;
		
		unsigned short game_code;
		unsigned short opponent_id;
		
		bool nigiriToBeSettled;
		bool undoAllowed;
		bool oneColorGo;
		int style;
		QString date, place, copyright, codec, gameName, fileName, overtime;
		assessType free_rated;
		/* We can receive moves in an observed IGS game before receiving
		 * the board state.  But there's another issue with observing
		 * multiple games that might suggest some other kind of fix */
		bool move_list_received;
		//for tygem, I would love to remove but I think we need it
		//for time messages and I don't want to pick it off
		//the listing since that's possibly less reliable:
		bool white_first_flag;
		// apparently we also need this, we might later try to simplify this
		// or take the one off...
		bool our_invitation;
		unsigned int opponentdropcount;
};


#endif //GAMEDATA_H
