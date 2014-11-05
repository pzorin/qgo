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


/***************************************************************************
*
* This class is the most important
* It  deals with distributing the move (and  other commands) requests 
* It is subclassed in the different proxies that can be used : local game,
* server game, and computer game
* Its the only class (apart from 'sgf parser') to modify the game tree
* 
***************************************************************************/

#ifndef QGOBOARD_H
#define QGOBOARD_H

#include "defines.h"

class BoardWindow;
class Tree;
class GameData;
class Move;
class QGtp;
class Sound;

/* I think its easier to have the computer and normal modes override a few functions to
 * null then have review and match provide the same complicated net functionality */


class qGoBoard : public QObject //, public Misc<QString>
{
	Q_OBJECT

public:
	qGoBoard(BoardWindow *bw, Tree * t, GameData *gd);
//	qGoBoard(qGoBoard &qgoboard );
    virtual ~qGoBoard() {}
	virtual void setHandicap(int handicap);
	virtual void addStone(StoneColor c, int x, int y);

	virtual void addMark( int x, int y, MarkType t );
	virtual void removeMark( int x, int y);

//	virtual void localMoveRequest(int x, int y)=0;
	virtual bool getBlackTurn(bool time = false);
    virtual void startGame() {}
    virtual void stopTime() {}
    virtual void handleMove(class MoveRecord *) {}
    virtual void moveControl(QString &) {}
	int getMoveNumber(void);
	//virtual void set_havegd(bool b) 		{ have_gameData = b; }
	virtual void setModified(bool b= true)		{ isModified = b;}
	virtual bool getModified()			{ return isModified; }
	virtual bool getPlaySound()			{ return playSound;}
	virtual void setPlaySound(bool b) 		{ playSound = b; }
	
	virtual void setResult(class GameResult & );
	virtual void kibitzReceived(const QString& txt);
	virtual void setTimerInfo(const QString&, const QString&, const QString&, const QString&) {}
	virtual void timerEvent(QTimerEvent*);
	class TimeRecord getOurTimeRecord(void);		//awkward
	class TimeRecord getTheirTimeRecord(void);
	virtual void enterScoreMode();
	virtual void leaveScoreMode();
	void toggleGroupAt(int x, int y);
	virtual void markDeadStone(int x, int y);
	virtual void markLiveStone(int x, int y);
	virtual void markDeadArea(int x, int y);
	virtual void markLiveArea(int x, int y);

	virtual void setNode(int , StoneColor , int , int ) {}
    virtual void requestAdjournDialog(void) {}
    virtual void requestCountDialog(void) {}
    virtual void requestMatchModeDialog(void) {}
    virtual void requestDrawDialog(void) {}
    virtual void adjournGame(void) {}
    virtual void recvRefuseAdjourn(void) {}
    virtual void recvRefuseCount(void) {}
    virtual void recvRefuseMatchMode(void) {}
    virtual void recvRefuseDraw(void) {}
//	int get_id() const { return id; }
//	void set_id(int i) { id = i; /*gd.gameNumber = i;*/ }
//	GameData get_gameData() { return gd; }

//	void set_title(const QString&);
//	bool get_haveTitle() { return haveTitle; }
//	void set_komi(const QString&);
//	void set_freegame(bool);
//	bool get_havegd() { return have_gameData; }
//	bool get_sentmovescmd() { return sent_movescmd; }
//	void set_sentmovescmd(bool m) { sent_movescmd = m; }
//	bool get_adj() { return adjourned; }
//	QString get_bplayer() { return gd.playerBlack; }
//	QString get_wplayer() { return gd.playerWhite; }
//	void set_adj(bool a) { adjourned = a; }
//	void set_game(Game *g);

//	void set_Mode(int);
//	GameMode get_Mode() { return gameMode; }
//	void set_move(StoneColor, QString, QString);
//	void send_kibitz(const QString);
//	MainWindow *get_win() { return win; }
//	void initGame() { win->getBoard()->initGame(&gd); }
//	void setMode() { win->getBoard()->setMode(gameMode); }

//	QString secToTime(int);
/*	void set_stopTimer();
	void set_runTimer();
	void set_gamePaused(bool p) { game_paused = p; }
	int get_boardsize() { return gd.size; }
	int get_mvcount() { return mv_counter; }
	void set_myColorIsBlack(bool b);
	bool get_myColorIsBlack() { return myColorIsBlack; }
	void set_requests(const QString &handicap, const QString &komi, assessType);
	void check_requests();
	QString get_reqKomi() { return req_komi; }
	QString get_currentKomi() { return QString::number(gd.komi); }
	void dec_mv_counter() { mv_counter--; }
	int get_mv_counter() { return mv_counter; }
	bool get_requests_set() { return requests_set; }
	qGo* get_qgo() { return qgo; }
	void set_gsName(GSName g) { gsName = g; }
	void addtime_b(int m);
	void addtime_w(int m);
	void set_myName(const QString &n) { myName = n; }

	// teaching features
	bool        ExtendedTeachingGame;
	bool        IamTeacher;
	bool        IamPupil;
	bool        havePupil;
	bool        haveControls;
	QString     ttOpponent;
	bool        mark_set;
	int         mark_counter;
	GameData    gd;

signals:
	// to qGoIF
//	void signal_closeevent(int);
	void signal_sendcommand(const QString&, bool);
	void signal_2passes(const QString&, const QString&);
*/
public slots:


	// Board
	virtual void slotBoardClicked(bool, int, int , Qt::MouseButton );
	virtual void slotPassPressed();
	virtual void slotDonePressed();
	virtual void slotResignPressed();
    virtual void slotReviewPressed() {}
    virtual void slotDrawPressed() {}
    virtual void slotCountPressed() {}
	virtual void slotUndoPressed();
    virtual void slotScoreToggled(bool);
//	virtual void slot_remoteMove(bool ok, const QString &answer);
protected:
	BoardWindow *boardwindow;
	Tree *tree;
	GameData *gameData;
	Sound *clickSound;
	int boardTimerId;

//	bool        timer_running;
//	bool        game_paused;
//	bool	have_gameData;
	bool	isModified;
//	bool        sent_movescmd;
//	bool        adjourned;
//	bool        myColorIsBlack;
//	bool        myColorIsWhite;
//	bool        haveTitle;
//	GameMode    gameMode;
	//GameData    gd;
	int         id;
//	MainWindow  *win;
//	qGo         *qgo;
//	int         mv_counter;
	int	stated_mv_count;
	Move * lastMoveInGame;
	bool	playSound;
//	int         bt_i, wt_i;
//	QString     bt, wt;
//	QString     b_stones, w_stones;
//	QString     req_handicap;
//	QString     req_komi;
//	assessType  req_free;
//	bool		    requests_set;
//	QString     myName;
//	int         BY_timer;

//#ifdef SHOW_INTERNAL_TIME
//	int chk_b, chk_w;
//#endif

    /*
     * This functions gets the move request (from a board click)
     * and displays the resulting stone (if valid)
     * The base class implementation is empty because network and local games need completely different hadling.
     */
    virtual void localMoveRequest(StoneColor, int, int) {}
	virtual void localMarkDeadRequest(int x, int y);
    virtual void sendMoveToInterface(StoneColor ,int, int) {}
    virtual void sendPassToInterface(StoneColor ) { doPass(); }
    virtual Move *doMove(StoneColor c, int x, int y) {}
    virtual void doPass(StoneColor c = stoneNone); // By default the player who is on turn will pass
protected:
	bool dontCheckValidity;
	QTime lastSound;
};

/* We can override the virtuals above with nulls below if the option
 * isn't supported or needs to be drastically changed */

class qGoBoardNetworkInterface : public qGoBoard
{
	Q_OBJECT
public:
    virtual ~qGoBoardNetworkInterface() {}
public slots:
	virtual void slotUndoPressed();
	virtual void slotDonePressed();
	virtual void slotResignPressed();
    virtual void slotReviewPressed() {}		//should FIXME these two
    virtual void slotAdjournPressed() {}
protected:
	qGoBoardNetworkInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	virtual void sendMoveToInterface(StoneColor c,int x, int y);
	virtual void sendPassToInterface(StoneColor c);
	virtual void handleMove(MoveRecord * m);
    virtual void moveControl(QString & player) { controlling_player = player; }
	virtual void adjournGame(void);
    virtual void startGame(void) {}
	virtual void stopTime(void);
    virtual void onFirstMove(void) {}
	
	QString game_Id;
	bool dontsend;
	QString controlling_player;
    Move * reviewCurrent;
    virtual Move *doMove(StoneColor c, int x, int y);
};

class qGoBoardObserveInterface : public qGoBoardNetworkInterface
{
	Q_OBJECT

public:
	qGoBoardObserveInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardObserveInterface() {}

	void setModified(bool)	{} //we don't modify an observed game
	
public slots:
	void slotUpdateComment() {}		//what is this ?!?!?
    virtual void slotUndoPressed(void){}
    virtual void slotDonePressed(void){}
    virtual void slotResignPressed(void){}
    virtual void slotAdjournPressed(void){}

signals:
	void signal_sendCommandFromBoard(const QString&, bool);

private:
//	bool doMove(StoneColor c, int x, int y);
	virtual void onFirstMove(void);

	

};

class qGoBoardMatchInterface : public qGoBoardNetworkInterface 
{
	Q_OBJECT

public:
	qGoBoardMatchInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardMatchInterface() {}

	void setModified(bool)	{} //we don't modify a match game
	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
	void enterScoreMode();
	void leaveScoreMode();
	void timerEvent(QTimerEvent*);
	virtual void requestAdjournDialog(void);
	virtual void requestCountDialog(void);
	virtual void requestMatchModeDialog(void);
	virtual void requestDrawDialog(void);
	virtual void recvRefuseAdjourn(void);
	virtual void recvRefuseCount(void);
	virtual void recvRefuseMatchMode(void);
	virtual void recvRefuseDraw(void);
public slots:
	void slotUpdateComment() {}
	virtual void slotReviewPressed();
	virtual void slotDrawPressed();
	virtual void slotCountPressed();
	virtual void slotAdjournPressed();

signals:
	void signal_sendCommandFromBoard(const QString&, bool);

private:
	void localMoveRequest(StoneColor c, int x, int y);
	void localMarkDeadRequest(int x, int y);
	virtual void startGame(void);
	virtual void onFirstMove(void);
//	bool warningSound;
//	int warningSecs;

};

class qGoBoardReviewInterface : public qGoBoardNetworkInterface 
{
	Q_OBJECT

public:
	qGoBoardReviewInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardReviewInterface() {}

//	void setModified(bool)	{} //we don't modify an  game
//	void setResult(QString res, QString xt_res);
//	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
	//void set_move(StoneColor sc, QString pt, QString mv_nr);
	void setNode(int move_nr, StoneColor c, int x, int y);

public slots:
	void slotUpdateComment() {}
//	void slotDonePressed();
	void slotUndoPressed() ;

signals:
	void signal_sendCommandFromBoard(const QString&, bool);

private:
	void localMoveRequest(StoneColor c, int x, int y);

};


#endif
