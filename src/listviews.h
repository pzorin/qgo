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
#include <QtGui>
class GameListing;
class PlayerListing;

#define LESSTHAN	-1
#define EQUALTO		0
#define GREATERTHAN	1

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
		virtual int compare(const ListItem &, int) const { return 0;};
	protected:
		int compareNames(const QString & name1, const QString & name2) const;
	private:
		QList <QVariant> itemData;
};

class ObserverListItem : public ListItem
{
	public:
		ObserverListItem(PlayerListing * const l);	
		virtual QVariant data(int column) const;
		int columnCount() const;
		PlayerListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		PlayerListing * const listing;
};

class GamesListItem : public ListItem
{
	public:
		GamesListItem(GameListing * const l);
		virtual QVariant data(int column) const;
		int columnCount() const;
		GameListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		GameListing * const listing;
};

class PlayerListItem : public ListItem
{
	public:
		PlayerListItem(PlayerListing * const l);
		virtual QVariant data(int column) const;
		int columnCount() const;
		PlayerListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		PlayerListing * const listing;
};

class ListFilter
{
	public:
		ListFilter(class ListModel * l) : listModel(l) {};
		virtual ~ListFilter() {};
		virtual bool filterAcceptsRow(int row) const = 0;
	protected:
		class ListModel * listModel;
};

class PlayerListFilter : public ListFilter
{
	public:
		/* IGS ranks probably up to 10000, but ORO would need 100000 so... */
		PlayerListFilter(ListModel * l) : ListFilter(l), rankMin(0), rankMax(100000), flags(none) {};
		void setFilter(int rn, int rm);
		enum PlayerSortFlags { none = 0x0, open = 0x1, friends = 0x2, fans = 0x4, noblock = 0x8 };
		void setFilter(enum PlayerSortFlags f);
		virtual bool filterAcceptsRow(int row) const;
	private:
		int rankMin;
		int rankMax;
		unsigned char flags;
};

class GamesListFilter : public ListFilter
{
	public:
		GamesListFilter(ListModel * l) : ListFilter(l), watches(false) {};
		virtual bool filterAcceptsRow(int row) const;
		void toggleWatches(void);
	private:
		bool watches;
};

class FilteredView : public QTreeView
{
	public:
		FilteredView(QWidget * p = 0) : QTreeView(p), listFilter(0) {};
		~FilteredView() { delete listFilter; };
		void setFilter(ListFilter * l) { listFilter = l; };
		ListFilter * getFilter(void) { return listFilter; };
	private:
		virtual void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight);
		ListFilter * listFilter;
};

class ListModel : public QAbstractItemModel	//QAbstractItemModel??
{
	 public:
	 	ListModel();
		~ListModel();
		void insertListing(ListItem & item);
		//void updateListing(const ListItem & item);
		//void removeListing(const ListItem & item);
		int priorityCompare(const ListItem & i, const ListItem & j);
		QModelIndex parent(const QModelIndex & index) const;
		//bool hasChildren(const QModelIndex & parent) const;
		virtual int rowCount(const QModelIndex & parent) const;
		int columnCount(const QModelIndex & parent) const;
		//bool hasIndex(int row, int column, const QModelIndex & parent) const;
		QModelIndex index(int row, int column, const QModelIndex & parent) const;
		virtual QVariant data(const QModelIndex & index, int role) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const;
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
		QModelIndex getIndex(int row, int column) { return createIndex(row, column); };		
	protected:
		friend class PlayerListFilter;
		friend class GamesListFilter;
		ListItem * headerItem;
		QList <ListItem *> items;	//try it with the one list
		QList <int> sort_priority;
		Qt::SortOrder list_sort_order;
		bool isGamesListAwkwardVariable;
	private:
		void quicksort(int b, int e);
		int qs_partition(int b, int e);
};

class ObserverListModel : public ListModel
{
	public:
		ObserverListModel();
		~ObserverListModel();
		void insertListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		void setAccountName(QString name) { account_name = name; };
	private:
		QString account_name;
};

class PlayerListModel : public ListModel
{
	public:
		PlayerListModel();
		~PlayerListModel();
		void insertListing(PlayerListing * const l);
		void updateListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		PlayerListing * playerListingFromIndex(const QModelIndex &);
		PlayerListItem * playerListItemFromIndex(const QModelIndex &) const;
		void setAccountName(QString name) { account_name = name; };
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	private:
		friend class PlayerListFilter;
		QString account_name;
};

class SimplePlayerListModel: public PlayerListModel
{
	public:
		SimplePlayerListModel(bool _notify_column);
		virtual QVariant data(const QModelIndex & index, int role) const;
	private:
		bool notify_column;
};

class GamesListModel : public ListModel
{
	public:
		GamesListModel();
		~GamesListModel();
		void insertListing(GameListing * const l);
		void updateListing(GameListing * const l);
		void removeListing(GameListing * const l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		GameListing * gameListingFromIndex(const QModelIndex &);
};

extern std::map<class PlayerListing *, unsigned short> removed_player;
#endif //!LISTVIEWS_H
