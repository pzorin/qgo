/***************************************************************************
 *   Copyright (C) 2009- by The qGo Project                                *
 *                                                                         *
 *   This file is part of qGo.                                             *
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
#ifndef CONNECTIONWIDGET_H
#define CONNECTIONWIDGET_H

#include "defines.h"

#include <QWidget>
#include <QtGui>

class HostList;
class RoomListing;
class LoginDialog;
class NetworkConnection;
class ServerListStorage;
class Talk;
class ChannelListing;
class GameDialog;
class PlayerListing;

namespace Ui {
class ConnectionWidget;
}

class ConnectionWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit ConnectionWidget(QWidget *parent = 0);
    ~ConnectionWidget();

    void onConnectionError(void);
    void set_sessionparameter(QString par, bool val);

    void recvSeekCondition(class SeekCondition * s);
    void recvSeekCancel(void);
    void recvSeekPlayer(QString player, QString condition);

    /* This is awkward here but has the same life as mainwindow */
    ServerListStorage & getServerListStorage(void) { return *serverliststorage; };

    void recvRoomListing(const RoomListing & room, bool b);
    void recvChannelListing(const ChannelListing & channel, bool b);
    void changeChannel(const QString & s);
    Ui::ConnectionWidget * getUi(void) { return ui; };		//for room class... FIXME?
    void setNetworkConnection(NetworkConnection *);

    int closeConnection(bool error = false);

public slots:
    void loadConnectionSettings(void);
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
    void slot_checkbox(int, bool);

    void slot_roomListClicked(const QString &);
    void slot_channelListClicked(const QString &);
    void slot_seek(bool);
    void slot_seek(QAction *);
    void slot_talk(const QString& , const QString &, bool );
    void talkOpened(Talk * d);
    void talkRecv(Talk * d);
    void slot_pbRelOneTab(QWidget *w);
    void slot_cbconnectChanged(int);
    void setLooking(bool val);
    void slot_cbopen();
    void slot_cbquiet();
    void slot_alternateListColorsCB(bool);

    void slot_msgBox(const QString&);
    void slot_connexionClosed();

    void slot_statsPlayer(PlayerListing*);

    void saveHostList(void);
    void loadHostList(QSettings * settings);
    
private:
    Ui::ConnectionWidget *ui;

    void setupButtons(void);

    void cleanupServerData(void);
    // online time
    QDateTime	connectionEstablished;
    bool	youhavemsg;
    bool	playerListEmpty;
//	bool    gamesListEmpty;
    bool	autoAwayMessage;
    // cmd_xx get current cmd number, and, if higher than last, cmd is valid,
    //    else cmd is from history list
    int	cmd_count;
    bool	cmd_valid;

    HostList * hostlist;

    // telnet ready
    bool	tn_ready;
    bool	tn_wait_for_tn_ready;
    LoginDialog * logindialog;
    NetworkConnection * connection;
    ServerListStorage * serverliststorage;
    QMenu 		*seekMenu;
    QList<Talk*>	talkList;
    QList<GameDialog*> matchList;
    QList<const RoomListing *> roomList;
    QList<const ChannelListing *> channelList;
    int 	seekButtonTimer;

    //players table
    void showOpen(bool show);
    void setColumnsForExtUserInfo();
    QString rkToKey(QString txt, bool integer=FALSE);
    QString rkMax, rkMin;

    // timing aids
    void 		timerEvent(QTimerEvent*);
};

#endif // CONNECTIONWIDGET_H
