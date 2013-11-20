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


#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

#include "ui_gamedialog.h"
#include "defines.h"
#include <QtWidgets>

class NetworkConnection;
class PlayerListing;

class GameDialog : public QDialog,/*public NewGameDialog,*/public Ui::GameDialog //, public Misc<QString>
{ 
	Q_OBJECT

public:
    GameDialog(NetworkConnection * conn, const PlayerListing * opp);//QWidget* parent = 0, const char* name = 0, bool modal = true, WFlags fl = 0);
	~GameDialog();
	void set_oppRk(QString &rk) 	{ oppRk = rk; qDebug("oppRk: %s",  rk.toLatin1().constData()); }
	void set_myRk(QString &rk) 	{ myRk = rk; qDebug("myRk: %s",  rk.toLatin1().constData()); }
	void set_myName(QString &name) 	{ myName = name; }
	void set_is_nmatch (bool b) 	{ is_nmatch = b; }
	Ui::GameDialog	getUi() 	{return ui;}
	void recvRequest(class MatchRequest * mr, unsigned long flags = 0);
	void recvRefuseMatch(int motive = 0);
	class MatchRequest * getMatchRequest(void);
protected:
	void closeEvent(QCloseEvent *e);

signals:
	void signal_sendCommand(const QString &cmd, bool localecho);
	void signal_matchSettings(const QString&, const QString&, const QString&, assessType);
	void signal_removeDialog(GameDialog *);//const QString&);

public slots:
	void slot_play_black_button(void);
	void slot_play_white_button(void);
	void slot_play_nigiri_button(void);
	void ratedCB_changed(bool checked);
	void slot_boardSizeSpin(int);
	void slot_handicapSpin(int);
	void slot_komiSpin(int);
	void slot_timeTab(int);
	void slot_timeSpin(const QTime &);
	void slot_stonesTimeSpin(const QTime &);
	void slot_stonesSpin(int);
	void slot_BYTimeSpin(const QTime &);
	void slot_BYPeriodTimeSpin(const QTime &);
	void slot_BYPeriodsSpin(int);
	void slot_ASIATimeSpin(const QTime &);
	void slot_ASIAPeriodTimeSpin(const QTime &);
	void slot_ASIAPeriodsSpin(int);
	// pushbuttons
//	void slot_swapcolors();
//	void slot_pbsuggest();
	void slot_offer(bool);
	void slot_decline();
	void slot_changed();
	void slot_cancel();
	// parser
//	void slot_suggest(const QString&, const QString&, const QString&, const QString&, int);
//	void slot_matchCreate(const QString&, const QString&);
	
	void slot_komiRequest(const QString&, int, int, bool);
//	void slot_opponentopen(const QString&);
	//void slot_dispute(const QString&, const QString&);

private:
	void save_to_preferences(void);
	QTime qtimeFromSeconds(int);
	unsigned int timeToSeconds(QString time);	//duplicated in connect code FIXME
	unsigned int timeToSeconds(const QTime & t);
	void clearChangedFlags(void);
	bool getProperKomiHandicap(QString rankA, QString rankB, float * komi, unsigned int * handicap);
	bool we_are_challenger;	//FIXME, do we need this?
	Ui::GameDialog ui;
	bool have_suggestdata;
	int dialog_changed;
	bool color_request_changed;
	bool ratedchanged;
	bool boardSizechanged;
	bool handicapchanged;
	bool komichanged;
	bool ttchanged;
	bool timechanged;
	bool stonesTimechanged;
	bool stoneschanged;
	bool BYPeriodTimechanged;
	bool BYPeriodschanged;
	bool ASIATimechanged;
	bool ASIAPeriodTimechanged;
	bool ASIAPeriodschanged;
	QString pwhite;
	QString pblack;
	QString h19, h13, h9;
	QString k19, k13, k9;
	QString oppRk;
	QString myRk;
	QString myName;
	bool komi_request;
	bool is_nmatch;
	NetworkConnection * connection;
	class MatchRequest * current_match_request;
    const PlayerListing * opponent;
	class Sound * gameSound;
	bool offered_and_unrefused;
	unsigned long flags;
};

#endif
