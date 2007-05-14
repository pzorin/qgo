/*
* defines.h
*/

#ifndef DEFINES_H
#define DEFINES_H

#include <QtCore>

/*
* Global defines
*/

#define PACKAGE "qgo2"
#define VERSION "0.1"

#define DEFAULT_BOARD_SIZE 19
#define BOARD_X 500
#define BOARD_Y 500

#define WHITE_STONES_NB 8

#define SLIDER_INIT 0

#define CONSOLECMDPREFIX "--->"

/*
* GraphicsItems types for the board class
*/
#define RTTI_STONE 1001
#define RTTI_MARK_SQUARE 1002
#define RTTI_MARK_CIRCLE 1003
#define RTTI_MARK_TRIANGLE 1004
#define RTTI_MARK_CROSS 1005
#define RTTI_MARK_TEXT 1006
#define RTTI_MARK_NUMBER 1007
#define RTTI_MARK_TERR 1008
#define RTTI_MARK_OTHERLINE 1009


/*
* Marks used in editing a game
*/
#define MARK_TERRITORY_VISITED     99
#define MARK_TERRITORY_DAME       999
#define MARK_SEKI                  10
#define MARK_TERRITORY_DONE_BLACK 997
#define MARK_TERRITORY_DONE_WHITE 998

/*
* GNUgo default level
*/
#define GNUGO_LEVEL 10


/*
* Enum definitions
*/
enum StoneColor { stoneNone, stoneWhite, stoneBlack, stoneErase };
enum GameMode { modeNormal, modeObserve, modeMatch, modeTeach, modeComputer, modeReview };
enum GamePhase { phaseInit, phaseOngoing, phaseEdit, phaseNavTo, phaseScore, phaseEnded};
enum MarkType { markNone, markSquare, markCircle, markTriangle, markCross, markText, markNumber, markTerrBlack, markTerrWhite };
enum Codec { codecNone, codecBig5, codecEucJP, codecJIS, codecSJIS, codecEucKr, codecGBK, codecTscii };
enum player_type {HUMAN=0,COMPUTER} ;
enum assessType { noREQ, FREE, RATED, TEACHING };

enum State { stateVarBegin, stateNode, stateVarEnd };
enum Property { moveBlack, moveWhite, editBlack, editWhite, editErase, comment, editMark, unknownProp, nodeName, timeLeft, openMoves, nextMove};
enum TimeSystem { none, absolute, byoyomi, canadian };

enum CursorType { cursorIdle, cursorGhostBlack , cursorGhostWhite , cursorWait , cursorNavTo };

/*
 * Game server enums
 */
enum Status {GUEST, REGISTERED, OFFLINE};
enum InfoType {PLAYER, GAME, MESSAGE, YOUHAVEMSG, SERVERNAME, 
		ACCOUNT, STATUS, IT_OTHER, CMD, READY,
		NOCLIENTMODE, TELL, KIBITZ, MOVE, BEEP,
		WHO, STATS, GAMES, NONE, HELP, CHANNELS, SHOUT,
		PLAYER27, PLAYER27_START, PLAYER27_END, GAME7, GAME7_START,
		PLAYER42, PLAYER42_START, PLAYER42_END, WS};
enum GSName {IGS, NNGS, LGS, WING, CTN, CWS, DEFAULT, GS_UNKNOWN};

/*
 * Go Text Protocol (GnuGo) enums
 */
enum CommandType {PROTOCOL, BOARDSIZE, KNOWN_COMMAND, LEVEL, KOMI, PLAY_BLACK, PLAY_WHITE, GENMOVE};



/*
* Global structs
*/
struct ASCII_Import
{
	char blackStone, whiteStone, starPoint, emptyPoint, hBorder, vBorder;
};

struct FastLoadMark
{
	int x, y;
	MarkType t;
	QString txt;
};

struct MatrixStone
{
	int x, y;
	StoneColor c;
};

struct Position { int x, y; };
struct MoveNum {int n; };

struct GameInfo
{
	QString nr;
	QString type;
	QString wname;
	QString wprisoners;
	QString wtime;
	QString wstones;
	QString bname;
	QString bprisoners;
	QString btime;
	QString bstones;
	QString mv_col;	// T..Time, B..Black, W..White, I..Init
	QString mv_nr;
	QString mv_pt;
};


struct Game
{
	QString nr;
	QString	wname;
	QString	wrank;
	QString	bname;
	QString	brank;
	QString res;
	QString	mv;
	QString Sz;
	QString H;
	QString K;
	QString By;
	QString FR;
	QString ob;
	bool    running;
  	bool oneColorGo ;
};

struct Player
{

	QString info;
	QString name;
	QString idle;
	QString rank;
	QString play_str;
	QString obs_str;
	QString extInfo;
	QString won;
	QString lost;
	QString country;
	QString nmatch_settings;
  	QString rated;
	QString address;
	int     playing;
	int     observing;
	bool 	nmatch;
	bool    online;
	// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
	bool nmatch_black, nmatch_white, nmatch_nigiri;
	int 	nmatch_handicapMin, nmatch_handicapMax, 
		nmatch_timeMin, nmatch_timeMax, 
		nmatch_BYMin, nmatch_BYMax, 
		nmatch_stonesMin, nmatch_stonesMax,
		nmatch_KoryoMin, nmatch_KoryoMax;

};


#endif
