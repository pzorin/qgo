
/*
 * This class handles the differnet board windows that can be open at a given time
 * It receives the signals from the parser that processes the IGS server commands
 * and distributes it to the relevant board
 */

#ifndef QGO_INTERFACE_H
#define QGO_INTERFACE_H

#include "globals.h"
#include "boardwindow.h"




class qGoIF : public QObject
{
	Q_OBJECT

public:
	qGoIF(QWidget*);
	~qGoIF();
	void set_gsName(GSName n) { gsName = n; }
	void set_myName(const QString &n) { myName = n; }
	void createGame(GameMode _gameMode, GameData * _gameData, bool _myColorIsBlack , bool _myColorIsWhite );

	BoardWindow * getBoardWindow(int n)	{return (boardlist->contains(n) ? boardlist->value(n) : NULL) ;}
	BoardWindow * getBoardWindow(QString &player);
/*	bool set_observe(QString);
	void set_initIF();
	void set_myName(const QString &n) { myName = n; }
	qGo  *get_qgo() { return qgo; };
	void set_localboard(QString file=0);
	void set_localgame();
	QWidget *get_parent() { return parent; }
	void wrapupMatchGame(qGoBoard *, bool);
*/

signals:
	void signal_sendCommandFromInterface(const QString&, bool);
	void signal_addToObservationList(int);

public slots:
	void slot_boardClosed(int);
	// parser/mainwindow
	void slot_move(GameInfo*);
	void slot_sendCommandFromInterface(const QString&, bool b = FALSE);
	void slot_gameInfo(Game *);
	void slot_score(const QString&, const QString&, bool, const QString&);
	void slot_result(Game *);
	void slot_kibitz(int, const QString&, const QString&);
	void slot_observers(int , const QString &, const QString&);
	void slot_clearObservers(int);
/*	void slot_move(Game*);
	void slot_computer_game(QNewGameDlg*);
	void slot_title(const QString&);
	void slot_komi(const QString&, const QString&, bool);
	void slot_freegame(bool);
	void slot_matchcreate(const QString&, const QString&);
	void slot_removestones(const QString&, const QString&);
	void slot_undo(const QString&, const QString&);
	
	void slot_matchsettings(const QString&, const QString&, const QString&, assessType);
	void slot_requestDialog(const QString&, const QString&, const QString&, const QString&);
	void slot_timeAdded(int, bool);
	//void slot_undoRequest(const QString &);

//	void slot_closeevent(int) {};
	void slot_closeevent();
	void slot_sendcommand(const QString&, bool);
	void set_observe(const QString&);

protected:
	bool parse_move(int src, GameInfo* gi=0, Game* g=0, QString txt=0);
*/
private:
//	qGo     *qgo;
	QWidget *parent;
	// actual pointer, for speedup reason
//	qGoBoard *qgobrd;
	QString  myName;
	QHash<int, BoardWindow*> *boardlist;
	GSName   gsName;
	int      localBoardCounter;
//	int      lockObserveCmd;
};

#endif

