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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "mainwindow_settings.h"
#include "defines.h"
#include "igsconnection.h"
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
	bool loadSGF(QString);
	void onConnectionError(void);
	
	void recvSeekCondition(class SeekCondition * s);
	void recvSeekCancel(void);
	void recvSeekPlayer(QString player, QString condition);

	/* This is awkward here but has the same life as mainwindow */
	ServerListStorage & getServerListStorage(void) { return *serverliststorage; };

	void recvRoomListing(const RoomListing & room, bool b);
	void recvChannelListing(const ChannelListing & channel, bool b);
	void changeChannel(const QString & s);
	Ui::MainWindow * getUi(void) { return &ui; };		//for room class... FIXME?
	void setNetworkConnection(NetworkConnection * conn) { connection = conn; };
	void addBoardWindow(BoardWindow *);
	void removeBoardWindow(BoardWindow *);
	int checkForOpenBoards(void);
public slots:
	void slot_expanded(const QModelIndex & i);
	// sfg slots
	void slot_fileNewBoard();
	void slot_fileOpenBoard();
	void slot_fileOpenBoard(const QModelIndex &);
	void slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	
	// go engine slots
	void slot_computerNewBoard();
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
	void slot_channelListClicked(const QString &);
	void slot_seek(bool);
	void slot_seek(QAction *);
	void slot_talk(const QString& , const QString &, bool );
	void talkOpened(Talk * d);
	void talkRecv(Talk * d);
	void slot_pbRelOneTab(QWidget *w);
	void slot_cbconnectChanged(int);
	void slot_languageChanged(int);
	void slot_cblooking();
	void slot_cbopen();
	void slot_cbquiet();
	void slot_alternateListColorsCB(bool);

	// parser slots
	void slot_statsPlayer(PlayerListing*);
	void slot_checkbox(int , bool );
	//void slot_playerConnected(Player*);
	void slot_connexionClosed();
	void slot_msgBox(const QString&);
protected:
	void closeEvent(QCloseEvent *e);
	void loadSettings();
	void saveSettings();

private:
	void setupConnection(void);
	int closeConnection(bool error = false);
	
	void cleanupServerData(void);
	bool selectFile(const QModelIndex &);
	
	friend class Room;	//FIXME awkward
	Ui::MainWindow ui;
	QDirModel *model;
	SGFParser * MW_SGFparser;
	QString SGFloaded, fileLoaded;
	GameData * GameLoaded;
	Sound *connectSound, *gameSound;

	QLabel *statusMessage, *statusUsers, *statusGames, *statusServer,*statusOnlineTime;
	void initStatusBar();
	void displayGame(DisplayBoard *);

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
	QMenu 		*seekMenu;
	QList<Talk*>	talkList;
	QList<GameDialog*> matchList;
	std::vector<BoardWindow *> boardWindowList;
	std::vector<const RoomListing *> roomList;
	std::vector<const ChannelListing *> channelList;
	int 	seekButtonTimer;
	
	//players table
	void showOpen(bool show);
	void setColumnsForExtUserInfo();
	QString rkToKey(QString txt, bool integer=FALSE);
	QString rkMax, rkMin;
	QString currentWorkingDir;

	// timing aids
	void 		timerEvent(QTimerEvent*);
};

#endif


