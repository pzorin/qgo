/*
 *   parser.h
 */

#ifndef PARSER_H
#define PARSER_H

#include "defines.h"
//#include"

//class Player;
//class Game;
//class GameInfo;

class Parser : public QObject//, public Misc<QString>
{
	Q_OBJECT
public:
	Parser();
	~Parser();
	InfoType  put_line(const QString&, class NetworkConnection *, GSName);
	Player    get_player();
	//Player    *get_statsPlayer() {return statsPlayer ;}
	Game	  get_game();

	void       set_gsname(const GSName);
	GSName     get_gsname();
	void       set_myName(const QString &n) { myname = n; }
	QString    get_buffer();
	InfoType   cmdsent(const QString&);

signals:
	//new signals
	void signal_gameRestored(Game*);
//	void signal_matchResult(int,QString&,QString&);
	void signal_result(Game*);
	void signal_playerConnected(Player*);
	void signal_connexionClosed();
	void signal_enterScoreMode();
	void signal_removeStones(const QString&, const QString&);
//	void signal_gameFinished(Game*);
	// emit if info is found
	void signal_player(Player*, bool);
  	void signal_statsPlayer(Player*);
	void signal_game(Game*);
	void signal_gameInfo(Game*);
	void signal_observedGameClosed(int);
	void signal_requestDialog(const QString&, const QString&, const QString&, const QString&);
	void signal_undo(const QString&, const QString&, const QString&);
	void signal_msgBox(const QString&);
	void  signal_gameReview(Game*);
	void signal_reviewInvite(const QString&, const QString&);
	void signal_reviewNode(int, int, StoneColor, int, int);

//	void signal_move(Game*);
	void signal_move(GameInfo*);
	void signal_message(QString);
	void signal_svname(GSName&);
	void signal_accname(QString&);
	void signal_status(Status);
	void signal_talk(const QString&, const QString&, bool);
	void signal_checkbox(int, bool);
	void signal_channelinfo(int, const QString&);
	void signal_kibitz(int, const QString&, const QString&);
	void signal_observers(int , const QString&, const QString&);
	void signal_clearObservers(int); 
	void signal_title(const QString&);
	void signal_komi(const QString&, const QString&, bool);
	void signal_freegame(bool);
	void signal_suggest(const QString&, const QString&, const QString&, const QString&, int);
	void signal_matchRequest(const QString&, bool);
	void signal_matchCanceled(const QString&);
	void signal_matchCreate(const QString& , const QString&);
	void signal_notOpen(const QString&, int);
	void signal_komirequest(const QString&, int, int, bool);
	void signal_opponentopen(const QString&);
	void signal_score(const QString&, const QString&, bool, const QString&);
	void signal_addToObservationList(int);
	void signal_shout(const QString&, const QString&);
	void signal_timeAdded(int, bool);
	void signal_room(const QString&, bool );
	void signal_addSeekCondition(const QString&, const QString&, const QString&, const QString&, const QString&);
	void signal_clearSeekCondition();
	void signal_cancelSeek();
	void signal_seekList(const QString&, const QString&);
	void signal_refresh(int);
	void signal_dispute(const QString&, const QString&);
	void signal_set_observe(const QString&);
	void signal_restoreScore();
	//void signal_undoRequest(const QString&);



private:
	QString    myname;
	
	GSName     gsName;
	class PlayerListing * aPlayer;
	class PlayerListing * statsPlayer;
	class GameResult * aGameResult;
	class GameRecord * aGameRecord;
	class TimeRecord * aTime;
	class MoveRecord * aMove;
	class MatchRequest * created_match_request;
	QString    buffer;
	int        memory;
	QString    memory_str;
	int scoring_game_id;
	QString    element(const QString &line, int index, const QString &del1, const QString &del2="", bool killblanks = FALSE);
	
	QList <unsigned int> * gameListA;
	QList <unsigned int> * gameListB;
	

};

#endif
