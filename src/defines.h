/*
* defines.h
*/

#ifndef DEFINES_H
#define DEFINES_H

#include "audio.h"

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
/* FIXME, I multiplied these by 10 because they prevented the use of countLiberties
 * by checkFalseEye by corrupting the stone.  It seems to me rather silly to
 * have defines like these in addition to the marking system, but they are sort
 * of internal marks.  At any rate, things still seem to work so I guess
 * this fixed things.  But its still ugly and there could still be more problems */
#define MARK_TERRITORY_VISITED    0x0800
#define MARK_TERRITORY_DAME       0x0400
#define MARK_SEKI                 0x1000
#define MARK_TERRITORY_DONE_BLACK 0x0200
#define MARK_TERRITORY_DONE_WHITE 0x0100

//first nibble is for stone color/erase
#define MX_STONEDEAD	0x8000
#define MX_STONEEDIT	0x4000

//3 looked a little small
#define SMALL_STONE_TERR_DIVISOR	2.5

/*
* GNUgo default level
*/
#define GNUGO_LEVEL 10

/* Game refusal motives... like we need a reason.  */
#define	GD_REFUSE_NOTOPEN		0
#define GD_REFUSE_DECLINE		1
#define GD_REFUSE_CANCEL		2
#define GD_REFUSE_INGAME		3
#define GD_REFUSE_NODIRECT		4
#define GD_INVALID_PARAMETERS		5
#define GD_OPP_NO_NMATCH		6
#define GD_RESET			7

/* NetworkDispatch codes */
#define ND_CONNECTED			1
#define ND_WAITING			0
#define ND_BADLOGIN			-1
#define ND_BADPASSWORD			-2
#define ND_BADHOST			-3
#define ND_BADCONNECTION		-4		//something of a default
#define ND_PROTOCOL_ERROR		-5
#define ND_USERCANCELED			-6

/*
* Enum definitions
*/
enum StoneColor { stoneNone, stoneWhite, stoneBlack, stoneErase };
enum GameMode { modeNormal, modeObserve, modeMatch, modeTeach, modeComputer, modeReview, modeUndefined };
enum GamePhase { phaseInit, phaseOngoing, phaseEdit, phaseNavTo, phaseScore, phaseEnded};
enum MarkType { markNone, markSquare, markCircle, markTriangle, markCross, markText, markNumber, markTerrBlack, markTerrWhite, markKoMarker };
enum Codec { codecNone, codecBig5, codecEucJP, codecJIS, codecSJIS, codecEucKr, codecGBK, codecTscii };
enum player_type {HUMAN=0,COMPUTER} ;
enum assessType { noREQ, FREE, RATED, TEACHING };

enum State { stateVarBegin, stateNode, stateVarEnd };
enum Property { moveBlack, moveWhite, editBlack, editWhite, editErase, comment, editMark, unknownProp, nodeName, timeLeft, openMoves, nextMove};
enum TimeSystem { none, absolute, byoyomi, canadian, tvasia };

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
/* ConnectionType in line with ui comboBox_server entry. FIXME to make more securely connected */
enum ConnectionType { TypeNone = 0, TypeIGS, TypeWING, TypeLGS,
			 TypeORO,
			 TypeTYGEM, TypeEWEIQI, TypeTOM,
			TypeUNKNOWN, TypeNNGS, TypeCTN, TypeCWS, TypeDEFAULT };
/* TypeCyberORO?? just IGS, and CyberORO, instead of with Type?? FIXME 
 * and reconcile with mainwindow_settings.h */

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


/*struct Game
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
	QString player;
	bool running;
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

};*/

/* This should really probably have access functions or
 * something and then friend the mainwindow settings code so
 * that only it can change it */
struct _preferences
{
	bool nmatch_black;
	bool nmatch_white;
	bool nmatch_nigiri;
	QString nmatch_handicap;
	bool draw_ko_marker;
	bool number_current_move;
	bool terr_stone_mark;
	bool observe_outside_on_doubleclick;
	int default_size;
	int default_komi;
	
	int default_stonesmaintime;
	int default_stonestime;
	int default_stones;
	int default_byomaintime;
	int default_byoperiodtime;
	int default_byoperiods;
	int default_asiamaintime;
	int default_asiaperiodtime;
	int default_asiaperiods;
	void fill(void)
	{
		QSettings settings;
		
		nmatch_black = settings.value("NMATCH_BLACK").toBool();
		nmatch_white = settings.value("NMATCH_WHITE").toBool();
		nmatch_nigiri = settings.value("NMATCH_NIGIRI").toBool();
		nmatch_handicap = settings.value("NMATCH_HANDICAP").toString();
		
		draw_ko_marker = (settings.value("KOMARKER") == 1);
		number_current_move = (settings.value("NUMBER_CURRENT_MOVE") == 1);
		terr_stone_mark = (settings.value("TERR_STONE_MARK") == 1);
		observe_outside_on_doubleclick = (settings.value("OBSERVEOUTSIDE") == 1);
		
		default_size = settings.value("DEFAULT_SIZE").toInt();
		default_komi = settings.value("DEFAULT_KOMI").toInt();
		
		default_stonesmaintime = settings.value("DEFAULT_STONESMAIN").toInt();
		default_stonestime = settings.value("DEFAULT_STONEST").toInt();
		default_stones = settings.value("DEFAULT_STONES").toInt();
		default_byomaintime = settings.value("DEFAULT_BYOMAIN").toInt();
		default_byoperiodtime = settings.value("DEFAULT_BYOPERIODT").toInt();
		default_byoperiods = settings.value("DEFAULT_BYOPERIODS").toInt();
		default_asiamaintime = settings.value("DEFAULT_TVASIAMAIN").toInt();
		default_asiaperiodtime = settings.value("DEFAULT_TVASIAPERIODT").toInt();
		default_asiaperiods = settings.value("DEFAULT_TVASIAPERIODS").toInt();
		
		
	}
	void save(void)
	{
		QSettings settings;
		
		settings.setValue("DEFAULT_STONESMAIN", default_stonesmaintime);
		settings.setValue("DEFAULT_STONEST", default_stonestime);
		settings.setValue("DEFAULT_STONES", default_stones);
		settings.setValue("DEFAULT_BYOMAIN", default_byomaintime);
		settings.setValue("DEFAULT_BYOPERIODT", default_byoperiodtime);
		settings.setValue("DEFAULT_BYOPERIODS", default_byoperiods);
		settings.setValue("DEFAULT_TVASIAMAIN", default_asiamaintime);
		settings.setValue("DEFAULT_TVASIAPERIODT", default_asiaperiodtime);
		settings.setValue("DEFAULT_TVASIAPERIODS", default_asiaperiods);
	}
};

/* There's only one of each of these, and they're both
 * used in weird places, so we'll make them global */
extern _preferences preferences;
extern class MainWindow * mainwindow;

#ifdef Q_WS_WIN
extern QString applicationPath;
#endif

#endif
