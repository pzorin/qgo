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

#include "boardwindow.h"
#include "tree.h"
#include "globals.h"
#include "qgtp.h"



class qGoBoard : public QObject //, public Misc<QString>
{
	Q_OBJECT

public:
	qGoBoard(BoardWindow *bw, Tree * t, GameData *gd);
//	qGoBoard(qGoBoard &qgoboard );
	virtual ~qGoBoard() {}
	virtual void setHandicap(int handicap);
	virtual void addStone(StoneColor c, int x, int y);
	virtual void removeStone(int x, int y);

	virtual void addMark( int x, int y, MarkType t );
	virtual void removeMark( int x, int y);

//	virtual void localMoveRequest(int x, int y)=0;
	virtual bool getBlackTurn();
	virtual void startGame() {}
	virtual void set_move(StoneColor , QString , QString ) {}
	virtual void set_statedMoveCount(int n) 	{ stated_mv_count = n;}
	virtual void set_havegd(bool b) 		{ have_gameData = b; }
	virtual bool modified()				{ return isModified;}
	virtual void setModified(bool b= true)		{ isModified = b;}
	virtual bool getModified()			{ return isModified; }
	virtual bool getPlaySound()			{ return playSound;}
	virtual void setPlaySound(bool b) 		{ playSound = b; }
	virtual	void set_gsName(GSName g) 		{ gsName = g; }
	
	virtual void setResult(QString , QString ) {}
	virtual void kibitzReceived(const QString& txt);
	virtual void setTimerInfo(const QString&, const QString&, const QString&, const QString&) {}
	virtual void timerEvent(QTimerEvent*);
	virtual void deleteNode();
	virtual void enterScoreMode() ;
	virtual void markDeadStone(int x, int y);

	virtual void setNode(int , StoneColor , int , int ) {}
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
//	void setGameData() {win->getBoard()->setGameData(&gd); }
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
	void clearObserverList() { win->getListView_observers()->clear(); }
	void playComputer(StoneColor);

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
	virtual void slotDonePressed() {}
	virtual void slotReviewPressed() {}
	virtual void slotUndoPressed() {}
	virtual void slotScoreToggled(bool);
	virtual void slotUpdateComment();
//	virtual void slot_remoteMove(bool ok, const QString &answer);
/*	void slot_stoneComputer(enum StoneColor, int, int);    
	void slot_PassComputer(StoneColor c) ;                 
	void slot_UndoComputer(StoneColor c) ;                 
 	void slot_DoneComputer() ;                 
	
	void slot_doResign();
	void slot_doUndo();
	void slot_doAdjourn();
	void slot_doDone();
	void slot_doRefresh();

	// normaltools
	void slot_addtimePauseW();
	void slot_addtimePauseB();

	// teachtools
	void slot_ttOpponentSelected(const QString&);
	void slot_ttControls(bool);
	void slot_ttMark(bool);
*/
protected:
	BoardWindow *boardwindow;
	Tree *tree;
	GameData *gameData;
	Sound *clickSound;

//	bool        timer_running;
//	bool        game_paused;
	bool	have_gameData;
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
	int	stated_mv_count ;
	bool	playSound;
//	int         bt_i, wt_i;
//	QString     bt, wt;
//	QString     b_stones, w_stones;
//	QString     req_handicap;
//	QString     req_komi;
//	assessType  req_free;
//	bool		    requests_set;
	GSName      gsName;
//	QString     myName;
//	int         BY_timer;

//#ifdef SHOW_INTERNAL_TIME
//	int chk_b, chk_w;
//#endif

	virtual void localMoveRequest(StoneColor c, int x, int y);
	virtual void localMarkDeadRequest(int x, int y);
	virtual void sendMoveToInterface(StoneColor c,int x, int y) =0;
	virtual bool doMove(StoneColor c, int x, int y);
	virtual void doPass(); //TODO check wether it's usefull to pass the color as in doMove
//	virtual void set_move(StoneColor sc, QString pt, QString mv_nr);
	virtual void sendPassToInterface(StoneColor c)=0;
	virtual void localPassRequest();
//	virtual void enterScoreMode() ;
	virtual void leaveScoreMode() {};//= 0;
	
};



class qGoBoardNormalInterface : public qGoBoard 
{

public:
	qGoBoardNormalInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	virtual ~qGoBoardNormalInterface() {}


private:
	void sendMoveToInterface(StoneColor /*c*/,int /*x*/, int /*y*/ ) {}
	void sendPassToInterface(StoneColor /*c*/) {}
//	bool doMove(StoneColor c, int x, int y);
//	void enterScoreMode();
	void leaveScoreMode() ;
};

class qGoBoardComputerInterface : public qGoBoard 
{
	Q_OBJECT

public:
	qGoBoardComputerInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardComputerInterface() {}
	void set_move(StoneColor sc, QString pt, QString mv_nr);

public slots:
	void slot_playComputer(bool ok, const QString &computer_answer);

private:
	void sendMoveToInterface(StoneColor c,int x, int y) ;
	void sendPassToInterface(StoneColor c);
//	bool doMove(StoneColor c, int x, int y);

	void playComputer(StoneColor c);
	void localMoveRequest(StoneColor c, int x, int y);
	void localPassRequest();
	void startGame();
//	void enterScoreMode() {}
	void leaveScoreMode() {}

	QGtp *gtp;
};

class qGoBoardObserveInterface : public qGoBoard 
{
	Q_OBJECT

public:
	qGoBoardObserveInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardObserveInterface() {}

	void setModified(bool)	{} //we don't modify an observed game
	void setResult(QString res, QString xt_res);
	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
	void set_move(StoneColor sc, QString pt, QString mv_nr);

public slots:
	void slotUpdateComment() {}
	void slotSendComment();

signals:
	void signal_sendCommandFromBoard(const QString&, bool);

private:
	void sendMoveToInterface(StoneColor /*c*/,int /*x*/, int /*y*/ ) {}
	void sendPassToInterface(StoneColor /*c*/) {}
//	bool doMove(StoneColor c, int x, int y);
//	void enterScoreMode() {}
	void leaveScoreMode() {}

	QString game_Id;

};

class qGoBoardMatchInterface : public qGoBoard 
{
	Q_OBJECT

public:
	qGoBoardMatchInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardMatchInterface() {}

	void setModified(bool)	{} //we don't modify a match game
	void setResult(QString res, QString xt_res);
	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
	void set_move(StoneColor sc, QString pt, QString mv_nr);
	void enterScoreMode();
	void timerEvent(QTimerEvent*);

public slots:
	void slotUpdateComment() {}
	void slotSendComment();
	void slotDonePressed();
	void slotReviewPressed();
	void slotUndoPressed() ;
	void slotResignPressed() ;
	void slotAdjournPressed();

signals:
	void signal_sendCommandFromBoard(const QString&, bool);

private:
	void sendMoveToInterface(StoneColor c,int x, int y );
	void sendPassToInterface(StoneColor c);
	void leaveScoreMode() {}
	void localMoveRequest(StoneColor c, int x, int y);
	void localMarkDeadRequest(int x, int y);
	QString game_Id;
//	bool warningSound;
//	int warningSecs;

};



class qGoBoardReviewInterface : public qGoBoard 
{
	Q_OBJECT

public:
	qGoBoardReviewInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
	~qGoBoardReviewInterface() {}

//	void setModified(bool)	{} //we don't modify an  game
//	void setResult(QString res, QString xt_res);
//	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
	void set_move(StoneColor sc, QString pt, QString mv_nr);
	void setNode(int move_nr, StoneColor c, int x, int y);

public slots:
	void slotUpdateComment() {}
	void slotSendComment();
//	void slotDonePressed();
	void slotUndoPressed() ;

signals:
	void signal_sendCommandFromBoard(const QString&, bool);

private:
	void sendMoveToInterface(StoneColor c,int x, int y );
	void sendPassToInterface(StoneColor c);
	void localMoveRequest(StoneColor c, int x, int y);
	QString game_Id;

};


#endif
