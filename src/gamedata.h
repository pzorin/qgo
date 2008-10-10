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
			 playerBlack(QObject::tr("Black")),
			 playerWhite(QObject::tr("White")),
			 rankBlack(""),
			 rankWhite(""),
			 white_name(""),
			 black_name(""),
			 white_rank(""),
			 black_rank(""),
			 number(0),
			 gameNumber(0),
			 moves(0),
			 board_size(19),
			 boardSize(19),
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
			 result(""),
			 fullresult(0),
			 game_code(0),
			 rated(false),
			 nigiriToBeSettled(false),
			 oneColorGo(false),
			 style(1),
			 date(""),
			 place(""),
			 copyright(""),
			 gameName(""),
			 fileName(""),
			 overtime(""),
			 freegame(noREQ) {};
		GameData(GameData *d)
		{
			if (d)
			{
				playerBlack = d->playerBlack;
				playerWhite = d->playerWhite;
				rankBlack = d->rankBlack;
				rankWhite = d->rankWhite;
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
				gameName = d->gameName;
				fileName = d->fileName;
				overtime = d->overtime;
				boardSize = d->boardSize;
				board_size = d->board_size;
				handicap = d->handicap;
				gameNumber = d->gameNumber;
				number = d->number;
				//byoTime = d->byoTime;
				//byoPeriods = d->byoPeriods;
				//byoStones = d->byoStones;
				timelimit = d->timelimit;
				style = d->style;
				komi = d->komi;
				freegame = d->freegame;
				timeSystem = d->timeSystem;
				oneColorGo = d->oneColorGo;
				maintime = d->maintime;
				stones_periods = d->stones_periods;
				periodtime = d->periodtime;
				nigiriToBeSettled = d->nigiriToBeSettled;
				rated = d->rated;
				game_code = d->game_code;
			}
		}

		~GameData() {};
		
		QString playerBlack, playerWhite, rankBlack, rankWhite;
		QString white_name;
		QString black_name;
		QString white_rank;
		QString black_rank;
		
		bool running;
		unsigned int number;
		int gameNumber;
		unsigned int moves;
		unsigned int board_size;
		int boardSize;
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
		GameMode gameType;
		//struct BoardRecord * board;
		
		unsigned short game_code;
		unsigned short opponent_id;
		
		bool rated;
		bool nigiriToBeSettled;
		bool oneColorGo;
		int style;
		QString date, place, copyright, gameName, fileName, overtime;
		assessType freegame;
};


#endif //GAMEDATA_H
