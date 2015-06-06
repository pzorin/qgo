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


#ifndef DEFINES_H
#define DEFINES_H

#include <QtCore>

/*
* Global defines
*/

#define PACKAGE "qGo"
#define VERSION "2.1.0"

#define DEFAULT_BOARD_SIZE 19
#define BOARD_X 500
#define BOARD_Y 500
#define PASS_XY -1

#define WHITE_STONES_NB 8

#define CONSOLECMDPREFIX "--->"

//3 looked a little small
#define SMALL_STONE_TERR_DIVISOR	2.5

/*
* GNUgo default level
*/
#define DEFAULT_ENGINE "gnugo"
#define DEFAULT_ENGINE_PATH "/usr/games/"
#define DEFAULT_ENGINE_OPTIONS "--mode gtp --quiet --level 10"


/*
* Enum definitions
*/
enum StoneColor { stoneNone = 0x0, stoneWhite = 0x1, stoneBlack = 0x2, stoneErase = 0x3 };
enum MarkType { markNone = 0x00, markTerrBlack = 0x04, markTerrWhite = 0x08, markTerrDame = 0x0c,
                markSquare = 0x10, markCircle = 0x20, markTriangle = 0x30, markCross = 0x40,
                markText = 0x50, markNumber = 0x60, markKoMarker = 0x80, markAll = 0xfc };

enum GameMode { modeLocal, modeObserve, modeMatch, modeTeach, modeEdit, modeReview, modeUndefined };
enum GamePhase { phaseInit, phaseOngoing, phaseEdit, phaseNavTo, phaseScore, phaseEnded};
enum Codec { codecNone, codecBig5, codecEucJP, codecJIS, codecSJIS, codecEucKr, codecGBK, codecTscii };
enum Language { None, German, French, Italian, Danish, Dutch, Czech, Chinese, Portugese, Polish, Russian, Turkish };
enum player_type {HUMAN=0,COMPUTER} ;
enum assessType { noREQ, FREE, RATED, TEACHING };

enum State { stateVarBegin, stateNode, stateVarEnd };
enum Property { moveBlack, moveWhite, editBlack, editWhite, editErase, comment, editMark, unknownProp, nodeName, timeLeft, openMoves, nextMove};
enum TimeSystem { none, absolute, byoyomi, canadian, tvasia };

/*
 * Game server enums
 * We rely on the fact that the known types come first,
 * followed by TypeDEFAULT, followed by other types
 *
 * Note that a list of user-visible strings corresponding to these types
 * is maintained in login.cpp; that list must be in the same order.
 */
enum ConnectionType { TypeIGS, TypeWING, TypeLGS, TypeORO, TypeTYGEM, TypeEWEIQI, TypeTOM,
                      TypeDEFAULT, TypeNone, TypeUNKNOWN,
                      TypeNNGS, TypeCTN, TypeCWS}; // These are not implemented

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

	int playerslist_sortcolumn;
	bool playerslist_sortascending;
	int gameslist_sortcolumn;
	bool gameslist_sortascending;

	int default_size;
	int default_komi;
	bool warn_about_codecs;
	bool simple_player_names;
	
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
		QVariant var;
		
		nmatch_black = settings.value("NMATCH_BLACK").toBool();
		nmatch_white = settings.value("NMATCH_WHITE").toBool();
		nmatch_nigiri = settings.value("NMATCH_NIGIRI").toBool();
		nmatch_handicap = settings.value("NMATCH_HANDICAP").toString();
		
		draw_ko_marker = (settings.value("KOMARKER") == 1);
		number_current_move = (settings.value("NUMBER_CURRENT_MOVE") == 1);
		terr_stone_mark = (settings.value("TERR_STONE_MARK") == 1);
		observe_outside_on_doubleclick = (settings.value("OBSERVEOUTSIDE") == 1);
		
		playerslist_sortcolumn = settings.value("PLAYERSLIST_SORTCOLUMN").toInt();
		playerslist_sortascending = settings.value("PLAYERSLIST_SORTASCENDING").toBool();
		gameslist_sortcolumn = settings.value("GAMESLIST_SORTCOLUMN").toInt();
		gameslist_sortascending = settings.value("GAMESLIST_SORTASCENDING").toBool();

		default_size = settings.value("DEFAULT_SIZE").toInt();
		default_komi = settings.value("DEFAULT_KOMI").toInt();
		simple_player_names = settings.value("SIMPLEPLAYERNAMES").toBool();
		
		warn_about_codecs = !settings.value("DONT_WARN_ABOUT_CODECS").toBool();
		
		if((var = settings.value("DEFAULT_STONESMAIN")) == QVariant())
			default_stonesmaintime = 600;
		else
			default_stonesmaintime = var.toInt();
		if((var = settings.value("DEFAULT_STONEST")) == QVariant())
			default_stonestime = 600;
		else
			default_stonestime = var.toInt();
		if((var = settings.value("DEFAULT_STONES")) == QVariant())
			default_stones = 25;
		else
			default_stones = var.toInt();
		if((var = settings.value("DEFAULT_BYOMAIN")) == QVariant())
			default_byomaintime = 600;
		else
			default_byomaintime = var.toInt();
		if((var = settings.value("DEFAULT_BYOPERIODT")) == QVariant())
			default_byoperiodtime = 30;
		else
			default_byoperiodtime = var.toInt();
		if((var = settings.value("DEFAULT_BYOPERIODS")) == QVariant())
			default_byoperiods = 3;
		else
			default_byoperiods = var.toInt();
		if((var = settings.value("DEFAULT_TVASIAMAIN")) == QVariant())
			default_asiamaintime = 30;
		else
			default_asiamaintime = var.toInt();
		if((var = settings.value("DEFAULT_TVASIAPERIODT")) == QVariant())
			default_asiaperiodtime = 30;
		else
			default_asiaperiodtime = var.toInt();
		if((var = settings.value("DEFAULT_TVASIAPERIODS")) == QVariant())
			default_asiaperiods = 4;
		else
			default_asiaperiods = var.toInt();
		
		
	}
	void save(void)
	{
		QSettings settings;
		
		settings.setValue("PLAYERSLIST_SORTCOLUMN", playerslist_sortcolumn);
		settings.setValue("PLAYERSLIST_SORTASCENDING", playerslist_sortascending);
		settings.setValue("GAMESLIST_SORTCOLUMN", gameslist_sortcolumn);
		settings.setValue("GAMESLIST_SORTASCENDING", gameslist_sortascending);
		
		settings.setValue("DONT_WARN_ABOUT_CODECS", !warn_about_codecs);

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
extern class ConnectionWidget * connectionWidget;

#ifdef Q_OS_WIN
extern QString applicationPath;
#endif

#ifdef Q_OS_WIN
    #define SOUND_PATH_PREFIX		"sounds/"
    #define TRANSLATIONS_PATH		"translations/"
#elif defined(Q_OS_MAC)
	#define SOUND_PATH_PREFIX			"qGo.app/Contents/Resources/Sounds/"
    #define TRANSLATIONS_PATH		"qGo.app/Contents/Resources/Translations/"
#else
    #define SOUND_PATH_PREFIX		"/usr/share/qgo/sounds/"
    #define TRANSLATIONS_PATH		"/usr/share/qgo/languages"
#endif

#endif
