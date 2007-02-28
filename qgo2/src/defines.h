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

#define DEFAULT_BOARD_SIZE 19
#define BOARD_X 500
#define BOARD_Y 500

#define WHITE_STONES_NB 8

#define SLIDER_INIT 0

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
* Enum definitions
*/
enum StoneColor { stoneNone, stoneWhite, stoneBlack, stoneErase };
enum GameMode { modeNormal, modeEdit, modeObserve, modeMatch, modeTeach, modeComputer, modeReview };
enum GamePhase { phaseInit, phaseOngoing, phaseCounting, phaseEnded};
enum MarkType { markNone, markSquare, markCircle, markTriangle, markCross, markText, markNumber, markTerrBlack, markTerrWhite };
enum Codec { codecNone, codecBig5, codecEucJP, codecJIS, codecSJIS, codecEucKr, codecGBK, codecTscii };
enum player_type {HUMAN=0,COMPUTER} ;
enum assessType { noREQ, FREE, RATED, TEACHING };

enum State { stateVarBegin, stateNode, stateVarEnd };
enum Property { moveBlack, moveWhite, editBlack, editWhite, editErase, comment, editMark, unknownProp, nodeName, timeLeft, openMoves, nextMove};
enum TimeSystem { none, absolute, byoyomi, canadian };

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


#endif
