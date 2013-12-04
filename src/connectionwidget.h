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
#include <QtWidgets>

class RoomListing;
class NetworkConnection;
class Talk;
class ChannelListing;
class GameDialog;
class PlayerListing;
class Room;
class PlayerListSortFilterProxyModel;
class GamesListSortFilterProxyModel;

namespace Ui {
class ConnectionWidget;
}

/* This class should probably be merged into Room */
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
    void recvRoomListing(const RoomListing & room, bool b);
    void recvChannelListing(const ChannelListing & channel, bool b);
    void changeChannel(const QString & s);
    void setNetworkConnection(NetworkConnection *);

    int closeConnection(bool error = false);

    /* Ugly hack, need a unique function
     * that is called when connection is established */
    bool isConnected(void);
    void setupButtons(void);

protected:
    friend class Room;
    Room * room;
    PlayerListSortFilterProxyModel * playerListProxyModel;
    GamesListSortFilterProxyModel * gamesListProxyModel;

public slots:
    void loadConnectionSettings(void);
    void disconnectFromServer(void);
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
    void talkOpened(Talk * d);
    void setLooking(bool val);
    void slot_cbopen();
    void slot_cbquiet();
    void slot_alternateListColorsCB(bool);

    void slot_msgBox(const QString&);
    void slot_connexionClosed();

private slots:
    void setRankSpreadView(void);
    void slot_editFriendsWatchesList(void);
    
private:
    Ui::ConnectionWidget *ui;

    void cleanupServerData(void);
    bool	youhavemsg;
    bool	playerListEmpty;
//	bool    gamesListEmpty;
    bool	autoAwayMessage;
    // cmd_xx get current cmd number, and, if higher than last, cmd is valid,
    //    else cmd is from history list
    int	cmd_count;
    bool	cmd_valid;

    // telnet ready
    bool	tn_ready;
    bool	tn_wait_for_tn_ready;
    NetworkConnection * connection;
    QMenu 		*seekMenu;
    QList<GameDialog*> matchList;
    QList<const RoomListing *> roomList;
    QList<const ChannelListing *> channelList;
    int 	seekButtonTimer;

    //players table
    void showOpen(bool show);
    void setColumnsForExtUserInfo();
    QString rkMax, rkMin;

    // timing aids
    void 		timerEvent(QTimerEvent*);
    QDateTime	connectionEstablished; // online time
};

#endif // CONNECTIONWIDGET_H
