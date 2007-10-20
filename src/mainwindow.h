/***************************************************************************
* Mainwindow.h
* headers for the main client window           *
 ***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "mainwindow_settings.h"
#include "globals.h"
#include "sgfparser.h"
#include "boardwindow.h"
#include "igsconnection.h"
#include "parser.h"
#include "qgo_interface.h"
#include "talk.h"
#include "gamedialog.h"
#include "audio.h"

#include <QtGui>

class HostList;


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 );
	~MainWindow();

	int sendTextFromApp(const QString&, bool localecho=true);
	void sendcommand(const QString&, bool localecho=false);
	void set_sessionparameter(QString, bool);
	void sendNmatchParameters();

public slots:
	// sfg slots
	void slot_fileNewBoard();
	void slot_fileOpenBoard();
	void slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	
	// go engine slots
	void slot_computerNewBoard();
	void slot_computerOpenBoard();
	void slot_loadComputerFile(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	void slot_getComputerPath();
	void slot_computerPathChanged(const QString &);

	//preferences tabs slots
	void slot_addServer();
	void slot_clickedListView(QTreeWidgetItem *lvi ,  int i);
	void slot_new();
	void slot_serverChanged( const QString &server);
	void slot_cancelPressed();
	void slot_currentChanged(int );

	// client slots
	void slot_connect(bool b);
	void slot_textReceived(const QString &txt);
	void slot_message(QString, QColor = Qt::black);
//	void slot_message(QString txt);
	void set_tn_ready();
	void slot_sendCommand(const QString &, bool);
//	void slot_cmdactivated_int(int);
	void slot_cmdactivated(const QString&);
	void slot_RefreshGames();
	void slot_RefreshPlayers();
	void slot_roomListClicked(const QString &);
	void slot_talk(const QString& , const QString &, bool );
	void slot_pbRelOneTab(QWidget *w);
	void slot_cblooking();
	void slot_cbopen();
	void slot_cbquiet();
	void slot_cancelSeek();
	void slot_setRankSpread();
	void slot_matchRequest(const QString &, bool);
	void slot_talkTo(QString &, QString &);
	void slot_removeDialog(GameDialog *);
	void slot_matchCanceled(const QString&);
	void slot_seek(QAction *);
	void slot_seek(bool);

	// parser slots
	void slot_refresh(int i);
	void slot_accname(QString &name);
	void slot_svname(GSName &gs);
	void slot_room(const QString& room, bool b);
	void slot_game(Game* g);
	void slot_player(Player *p, bool cmdplayers);
	void slot_statsPlayer(Player*);
	void slot_sortGames (int);
	void slot_sortPlayers (int);
	void slot_gamesDoubleClicked(QTreeWidgetItem* );
	void slot_playersDoubleClicked(QTreeWidgetItem*);
	void slot_checkbox(int , bool );
	void slot_playerConnected(Player*);
	void slot_connexionClosed();
	void slot_removeDialog(const QString &);
	void slot_clearSeekCondition();
	void slot_addSeekCondition(const QString& , const QString& , const QString& , const QString& , const QString& );
	void slot_seekList(const QString& , const QString& );
	void slot_msgBox(const QString&);

protected:
	void closeEvent(QCloseEvent *e);
	void loadSettings();
	void saveSettings();

private:
	Ui::MainWindow ui;
	QDirModel *model;
	SGFParser * MW_SGFparser;
	QString SGFloaded, SGFloaded2, fileLoaded , fileLoaded2 ;
	GameData * GameLoaded , * GameLoaded2 ;
	qGoIF * qgoif;
	Sound *connectSound, *gameSound;

	QLabel *statusMessage, *statusUsers, *statusGames, *statusServer,*statusOnlineTime;
	void initStatusBar();
	void displayGame();

	// online time
	int	onlineCount;
	bool	youhavemsg;
	bool	playerListEmpty;
	bool	playerListSteadyUpdate;
	bool	gamesListSteadyUpdate;
//	bool    gamesListEmpty;
	bool	autoAwayMessage;
	int 	timer;
	// cmd_xx get current cmd number, and, if higher than last, cmd is valid,
	//    else cmd is from history list
	int	cmd_count;
	bool	cmd_valid;

	// telnet ready
	bool	tn_ready;
	bool	tn_wait_for_tn_ready;
	HostList hostlist;
	IGSConnection 	*igsConnection;

	QList<sendBuf*>  sendBuffer;
	sendBuf		*currentCommand;
	Account		*myAccount;
	Parser		*parser;
	QMenu 		*seekMenu;
	QList<Talk*>	talkList;
	QList<GameDialog*> matchList;
	int 	seekButtonTimer;

	//players table
	void showOpen(bool show);
	void prepareTables(InfoType cmd);
	void setColumnsForExtUserInfo();
	QString rkToKey(QString txt, bool integer=FALSE);
	QString rkMax, rkMin;

	// timing aids
	void 		timerEvent(QTimerEvent*);
};

#endif


