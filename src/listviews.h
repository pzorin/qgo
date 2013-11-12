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
class ListModel;

enum ObserverListingColumn { OC_NAME=0, OC_RANK, O_TOTALCOLUMNS };

enum PlayerListingColumn { PC_STATUS=0, PC_NAME, PC_RANK, PC_PLAYING,
                PC_OBSERVING, PC_WINS, PC_LOSSES, PC_IDLE, PC_COUNTRY,
                PC_MATCHPREFS, P_TOTALCOLUMNS, PC_NOTIFY};

enum SimplePlayerListingColumn { SPC_NAME=0, SPC_NOTIFY };

enum GameListingColumn { GC_ID=0, GC_WHITENAME, GC_WHITERANK, GC_BLACKNAME,
                GC_BLACKRANK,
                GC_MOVES, GC_SIZE, GC_HANDICAP, GC_KOMI,
                GC_BYOMI, GC_FLAGS,
                GC_OBSERVERS, G_TOTALCOLUMNS };

#define LIST_SORT_ROLE Qt::UserRole

/* ListItem and its subclasses serve no purpose and should be eliminated */
class ListItem
{
	public:
        ListItem() {};
        virtual ~ListItem() {};
};

class ObserverListItem : public ListItem
{
	public:
    friend class ObserverListItemLessThan;
		ObserverListItem(PlayerListing * const l);	
        virtual QVariant data(int column) const;
		PlayerListing * getListing(void) const { return listing; };
	private:
		PlayerListing * const listing;
};

class GamesListItem : public ListItem
{
	public:
    friend class GamesListItemLessThan;
		GamesListItem(GameListing * const l);
        virtual QVariant data(int column) const;
        GameListing * getListing(void) const { return listing; };
	private:
		GameListing * const listing;
};

class PlayerListItem : public ListItem
{
	public:
        friend class PlayerListItemLessThan;
		PlayerListItem(PlayerListing * const l);
        virtual QVariant data(int column) const;
		PlayerListing * getListing(void) const { return listing; };
	private:
		PlayerListing * const listing;
};

class ObserverListItemLessThan
{
public:
    ObserverListItemLessThan(int _column, Qt::SortOrder _order);
    bool operator()(const ListItem *left, const ListItem *right ) const;
private:
    int column;
    bool reverseOrder;
};

class GamesListItemLessThan
{
public:
    GamesListItemLessThan(int _column, Qt::SortOrder _order);
    bool operator()(const ListItem *left, const ListItem *right ) const;
private:
    int column;
    bool reverseOrder;
};

class PlayerListItemLessThan
{
public:
    PlayerListItemLessThan(int _column, Qt::SortOrder _order);
    bool operator()(const ListItem *left, const ListItem *right ) const;
private:
    int column;
    bool reverseOrder;
};

class PlayerListSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PlayerListSortFilterProxyModel(QObject * parent = NULL);
    /* IGS ranks probably up to 10000, but ORO would need 100000 so... */
    enum PlayerSortFlags { none = 0x0, open = 0x1, friends = 0x2, fans = 0x4, noblock = 0x8 };

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
    unsigned char flags;
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

class ListModel : public QAbstractTableModel
{
    Q_OBJECT
	 public:
	 	ListModel();
		~ListModel();
        void insertListing(ListItem & item);
        virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
        QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        virtual QVariant data(const QModelIndex & index, int role) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
        //virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
        virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex() );
signals:
        void countChanged(int);
    protected:
        friend class PlayerListSortFilterProxyModel;
        friend class GamesListSortFilterProxyModel;
        friend class GamesListFilter;
        QList <ListItem *> items;
};

class ObserverListModel : public ListModel
{
	public:
		ObserverListModel();
		~ObserverListModel();
		void insertListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
        void setAccountName(QString name) { account_name = name; };
	private:
		QString account_name;
};

class PlayerListModel : public ListModel
{
    Q_OBJECT
	public:
		PlayerListModel();
		~PlayerListModel();
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
        void insertListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		PlayerListing * playerListingFromIndex(const QModelIndex &);
		PlayerListItem * playerListItemFromIndex(const QModelIndex &) const;
        void setAccountName(const QString & name) { account_name = name; };
        PlayerListing * getEntry(const QString & name);
        PlayerListing * getEntry(unsigned int id);
        PlayerListing * getPlayerFromNotNickName(const QString & notnickname);
        void deleteEntry(const QString & name);
        void deleteEntry(unsigned int id);
public slots:
        PlayerListing * updateEntryByName(PlayerListing * listing);
        PlayerListing * updateEntryByID(PlayerListing * listing);
	private:
		friend class PlayerListFilter;
		QString account_name;
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

class GamesListModel : public ListModel
{
	public:
		GamesListModel();
		~GamesListModel();
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
        void insertListing(GameListing * const l);
        void removeListing(GameListing * const l);
        GameListing * getEntry(unsigned int id);
        GameListing * updateListing(const GameListing * listing);
        void deleteEntry(unsigned int id);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
        GameListing * gameListingFromIndex(const QModelIndex &);
};

#endif //!LISTVIEWS_H
