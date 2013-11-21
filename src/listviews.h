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

#ifndef LISTVIEWS_H
#define LISTVIEWS_H
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
class GameListing;
class PlayerListing;

enum ObserverListingColumn { OC_NAME=0, OC_RANK, O_TOTALCOLUMNS };

enum PlayerListingColumn { PC_STATUS=0, PC_NAME, PC_RANK, PC_PLAYING,
                PC_OBSERVING, PC_WINS, PC_LOSSES, PC_IDLE, PC_COUNTRY,
                PC_MATCHPREFS, P_TOTALCOLUMNS, PC_NOTIFY, PC_ONLINE};

enum SimplePlayerListingColumn { SPC_NAME=0, SPC_NOTIFY };

enum GameListingColumn { GC_ID=0, GC_WHITENAME, GC_WHITERANK, GC_BLACKNAME,
                GC_BLACKRANK,
                GC_MOVES, GC_SIZE, GC_HANDICAP, GC_KOMI,
                GC_BYOMI, GC_FLAGS,
                GC_OBSERVERS, G_TOTALCOLUMNS, GC_RUNNING };

#define LIST_SORT_ROLE Qt::UserRole

class PlayerListSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PlayerListSortFilterProxyModel(QObject * parent = NULL);
public slots:
    void setFilterOpen(bool state);
    void setFilterFriends(bool state);
    void setFilterFans(bool state);
    void setFilterMinRank(int rank);
    void setFilterMaxRank(int rank);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
private:
    int rankMin;
    int rankMax;
    bool open, friends, fans, noblock;
};

class GamesListSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    GamesListSortFilterProxyModel(QObject * parent = NULL);
public slots:
    void setFilterWatch(bool state);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
private:
    bool watches;
};

class PlayerListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    PlayerListModel();
    ~PlayerListModel();

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data(const QModelIndex & index, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;

    void insertListing(PlayerListing * item);
    void removeListing(PlayerListing * const l);
    void clearList(void);
    PlayerListing * playerListingFromIndex(const QModelIndex &);
    void setAccountName(const QString & name) { account_name = name; }
    PlayerListing * getEntry(const QString & name);
    PlayerListing * getEntry(unsigned int id);
    PlayerListing * getPlayerFromNotNickName(const QString & notnickname);
public slots:
    PlayerListing * updateEntry(PlayerListing * listing);
protected:
    friend class PlayerListSortFilterProxyModel;
    friend class Room;
    QList <PlayerListing *> items;
    QString account_name;
};

class ObserverListModel : public PlayerListModel
{
    public:
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex & index, int role) const;
};

class SimplePlayerListModel: public PlayerListModel
{
	public:
		SimplePlayerListModel(bool _notify_column);
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex & index, int role) const;
	private:
		bool notify_column;
};

class GamesListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    GamesListModel();
    ~GamesListModel();
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    GameListing * getEntry(unsigned int id);
    void updateListing(GameListing *listing);
    void clearList(void);
    virtual QVariant data(const QModelIndex & index, int role) const;
    GameListing * gameListingFromIndex(const QModelIndex &);
    void insertListing(GameListing *item);
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
signals:
    void countChanged(int);
protected:
    friend class GamesListSortFilterProxyModel;
    friend class Room;
    QList <GameListing *> items;
};

#endif //!LISTVIEWS_H
