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


#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QString>
#include <QTableView>

class NetworkConnection;
class BoardDispatch;
class PlayerListing;
class GameListing;
class FilteredView;
class PlayerListModel;
class GamesListModel;

class Room : public QObject
{
	Q_OBJECT

	public:
        Room(NetworkConnection * c);
		void setConnection(NetworkConnection * c);
		virtual ~Room();
        void onError(void);
		void recvToggle(int type, bool val);
		PlayerListing * getPlayerListing(const QString & name);
		PlayerListing * getPlayerListing(const unsigned int id);
		PlayerListing * getPlayerListingByNotNickname(const QString & notnickname);
		GameListing * getGameListing(unsigned int key);
		BoardDispatch * getNewBoardDispatch(unsigned int key);
		void recvPlayerListing(class PlayerListing * g);
        void recvGameListing(class GameListing * g);
    protected:
		unsigned int players;
		unsigned int games;
	private:
		PlayerListModel * playerListModel;
		GamesListModel * gamesListModel;

        QTableView * playerView, * gamesView;
		NetworkConnection * connection;
		QModelIndex popup_item;
		
		GameListing * popup_gamelisting;
		PlayerListing * popup_playerlisting;

        QAction *statsAct, *matchAct, *talkAct, *removeFriendAct, *addFriendAct, *removeWatchAct, *addWatchAct, *blockAct;
        QAction *joinObserveAct, *observeOutsideAct;
public slots:
        void slot_refreshGames(void);
        void slot_refreshPlayers(void);
	private slots:
        void slot_playerOpenTalk(const QModelIndex &);
        void slot_gamesDoubleClicked(const QModelIndex &);
		void slot_showPopup(const QPoint & iPoint);
		void slot_showGamesPopup(const QPoint & iPoint);
};

#endif //ROOM_H
