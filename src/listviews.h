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
#include <QtWidgets>
class GameListing;
class PlayerListing;
class ListModel;

/* ListItem can't have pure virtual functions because
 * we do actually use it for the header items */
class ListItem
{
	public:
		ListItem() {};
		ListItem(const QList <QVariant> & data);
		virtual ~ListItem() {};
		virtual QVariant data(int column) const;
        virtual int columnCount() const;
	private:
		QList <QVariant> itemData;
};

class ObserverListItem : public ListItem
{
	public:
    friend class ObserverListItemLessThan;
		ObserverListItem(PlayerListing * const l);	
		virtual QVariant data(int column) const;
		int columnCount() const;
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
		int columnCount() const;
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
		int columnCount() const;
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

class ListFilter : public QObject
{
    Q_OBJECT
public:
    ListFilter() : QObject(), listModel(NULL) {};
		virtual ~ListFilter() {};
		virtual bool filterAcceptsRow(int row) const = 0;
    void setListModel(ListModel * l) { listModel = l; };
		class ListModel * getListModel(void) { return listModel; };
signals:
        void updated(void);
	protected:
        class ListModel * listModel;
};

class PlayerListFilter : public ListFilter
{
    Q_OBJECT
	public:
		/* IGS ranks probably up to 10000, but ORO would need 100000 so... */
        PlayerListFilter() : ListFilter(), rankMin(0), rankMax(100000), flags(none) {};
		enum PlayerSortFlags { none = 0x0, open = 0x1, friends = 0x2, fans = 0x4, noblock = 0x8 };
		virtual bool filterAcceptsRow(int row) const;
public slots:
    void setFilterOpen(bool state);
    void setFilterFriends(bool state);
    void setFilterFans(bool state);
    void setFilterMinRank(int rank);
    void setFilterMaxRank(int rank);
private:
    int rankMin;
    int rankMax;
    unsigned char flags;
};

class GamesListFilter : public ListFilter
{
    Q_OBJECT
	public:
        GamesListFilter() : ListFilter(), watches(false) {};
		virtual bool filterAcceptsRow(int row) const;
public slots:
        void setFilterWatch(bool state);
	private:
		bool watches;
};

class FilteredView : public QTableView
{
    Q_OBJECT
	public:
        FilteredView(QWidget * p = 0) : QTableView(p), listFilter(0) {};
		~FilteredView() { delete listFilter; };
        void setFilter(ListFilter * l);
		ListFilter * getFilter(void) { return listFilter; };
        virtual void setModel ( ListModel * model );
    protected slots:
        virtual void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight);
        virtual void rowsInserted ( const QModelIndex & parent, int start, int end );
        void updateFilter(void);
    public:
		ListFilter * listFilter;
};

class ListModel : public QAbstractTableModel
{
    Q_OBJECT
	 public:
	 	ListModel();
		~ListModel();
		void insertListing(ListItem & item);
		//void updateListing(const ListItem & item);
        //void removeListing(const ListItem & item);
        virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
        QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        virtual QVariant data(const QModelIndex & index, int role) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
        //virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
        void setView(FilteredView * v) { view = v; };
        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
signals:
        void countChanged(int);
    protected:
        void removeItem(int i);
		friend class PlayerListFilter;
		friend class GamesListFilter;
        friend class FilteredView;
        QList <ListItem *> items;
        // FIXME: insert new items at the correct place
        int sort_column;
        Qt::SortOrder sort_order;
		FilteredView * view;
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
        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
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
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
        PlayerListing * getEntry(const QString & name, const PlayerListing * listing = NULL);
        PlayerListing * getEntry(unsigned int id, const PlayerListing * listing = NULL);
        PlayerListing * getPlayerFromNotNickName(const QString & notnickname);
        void deleteEntry(const QString & name);
        void deleteEntry(unsigned int id);
public slots:
        void updateListing(PlayerListing * l);
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
		void updateListing(GameListing * const l);
		void removeListing(GameListing * const l);
        GameListing * getEntry(unsigned int id, const GameListing * listing = NULL);
        void deleteEntry(unsigned int id);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
        GameListing * gameListingFromIndex(const QModelIndex &);
        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
};

#endif //!LISTVIEWS_H
