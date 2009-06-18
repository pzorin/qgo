/***************************************************************************
* Mainwindow.h
* headers for the main client window           *
 ***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "mainwindow_settings.h"
#include "defines.h"
#include "igsconnection.h"
//#include "oldparser.h"
//#include "qgo_interface.h"
#include "audio.h"

#include <QtGui>

class HostList;
class RoomListing;
class LoginDialog;
class NetworkConnection;
class ServerListStorage;
class Talk;
class GameDialog;
class SGFParser;
class BoardWindow;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 );
	~MainWindow();

	void set_sessionparameter(QString par, bool val); //FIXME
	void loadSgfFile(QString fn);
	void onConnectionError(void);
	
	void recvSeekCondition(class SeekCondition * s);
	void recvSeekCancel(void);
	void recvSeekPlayer(QString player, QString condition);

	/* This is awkward here but has the same life as mainwindow */
	ServerListStorage & getServerListStorage(void) { return *serverliststorage; };
public slots:
	void slot_expanded(const QModelIndex & i);
	// sfg slots
	void slot_fileNewBoard();
	void slot_fileOpenBoard();
	void slot_fileOpenBoard(const QModelIndex &);
	void slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	
	// go engine slots
	void slot_computerNewBoard();
	void slot_computerOpenBoard();
	void slot_computerOpenBoard(const QModelIndex &);
	void slot_loadComputerFile(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	void slot_getComputerPath();
	void slot_computerPathChanged(const QString &);

	//preferences tabs slots
	void slot_cancelPressed();
	void slot_currentChanged(int );
	void slot_getGobanPath();
	void slot_getTablePath();
	void slot_newFile_HandicapChange(int);
	void slot_newComputer_HandicapChange(int);
	// client slots
	void slot_connect(bool b);
	//void slot_textReceived(const QString &txt);
	void slot_message(QString, QColor = Qt::black);
//	void slot_message(QString txt);
	void slot_createRoom(void);
	void slot_changeServer(void);
	void set_tn_ready();
	//void slot_sendCommand(const QString &, bool);
//	void slot_cmdactivated_int(int);
	void slot_cmdactivated(const QString&);
	
	void slot_roomListClicked(const QString &);
	void slot_seek(bool);
	void slot_seek(QAction *);
	void slot_talk(const QString& , const QString &, bool );
	void talkOpened(Talk * d);
	void talkRecv(Talk * d);
	void slot_pbRelOneTab(QWidget *w);
	void slot_cbconnectChanged(int);
	void slot_cblooking();
	void slot_cbopen();
	void slot_cbquiet();
	//void slot_matchRequest(const QString &);
	//void slot_talkTo(QString &, QString &);
	void slot_removeDialog(GameDialog *);
	void slot_matchCanceled(const QString&);

	// parser slots
	void recvRoomListing(const RoomListing & room, bool b);
	void slot_statsPlayer(PlayerListing*);
	void slot_checkbox(int , bool );
	//void slot_playerConnected(Player*);
	void slot_connexionClosed();
	void slot_removeDialog(const QString &, const QString &);
	void slot_msgBox(const QString&);
	Ui::MainWindow * getUi(void) { return &ui; };		//for room class... FIXME?
	void setNetworkConnection(NetworkConnection * conn) { connection = conn; };
protected:
	void closeEvent(QCloseEvent *e);
	void loadSettings();
	void saveSettings();

private:
	void setupConnection(void);
	void closeConnection(void);
	
	void cleanupServerData(void);
	
	friend class Room;	//FIXME awkward
	Ui::MainWindow ui;
	QDirModel *model;
	SGFParser * MW_SGFparser;
	QString SGFloaded, SGFloaded2, fileLoaded , fileLoaded2 ;
	GameData * GameLoaded , * GameLoaded2 ;
	//qGoIF * qgoif;
	Sound *connectSound, *gameSound;

	QLabel *statusMessage, *statusUsers, *statusGames, *statusServer,*statusOnlineTime;
	void initStatusBar();
	void displayGame();

	// online time
	int	onlineCount;
	bool	youhavemsg;
	bool	playerListEmpty;
//	bool    gamesListEmpty;
	bool	autoAwayMessage;
	int 	mainServerTimer;
	// cmd_xx get current cmd number, and, if higher than last, cmd is valid,
	//    else cmd is from history list
	int	cmd_count;
	bool	cmd_valid;

	// telnet ready
	bool	tn_ready;
	bool	tn_wait_for_tn_ready;
	HostList hostlist;
	LoginDialog * logindialog;
	NetworkConnection * connection;
	ServerListStorage * serverliststorage;
	//QList<sendBuf*>  sendBuffer;
	//sendBuf		*currentCommand;
	QMenu 		*seekMenu;
	QList<Talk*>	talkList;
	QList<GameDialog*> matchList;
	std::vector<const RoomListing *> roomList;
	int 	seekButtonTimer;

	//players table
	void showOpen(bool show);
	void setColumnsForExtUserInfo();
	QString rkToKey(QString txt, bool integer=FALSE);
	QString rkMax, rkMin;

	// timing aids
	void 		timerEvent(QTimerEvent*);
};

#endif


