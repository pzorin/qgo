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


#include <QtGui>
class GameListing;
class PlayerListing;

#define LESSTHAN	-1
#define EQUALTO		0
#define GREATERTHAN	1

#define ListingDataRole			Qt::UserRole
#define PlayerGameListingPtr	qulonglong

class ListModel : public QStandardItemModel
{
	public:
		ListModel(int columns);
		~ListModel();
		Qt::ItemFlags flags(const QModelIndex & index) const;
		void removeListing(const PlayerGameListingPtr l);
		void clearList(void);
		int getInsertPos(const PlayerGameListingPtr l);
		virtual bool lessThan(PlayerGameListingPtr l, PlayerGameListingPtr r, int column) const = 0;
	protected:
		virtual Qt::SortOrder getSortOrder(void) const = 0;
		virtual int getSortColumn(void) const = 0;
		PlayerGameListingPtr listingFromIndex(const QModelIndex & index);
};

class ObserverListModel : public ListModel
{
	public:
		ObserverListModel();
		~ObserverListModel();
		void insertListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		PlayerListing * playerListingFromIndex(const QModelIndex &);
		void setAccountName(QString name) { account_name = name; };
		class ObserverSortProxy * getProxy(void) { return sortProxy; };
		virtual bool lessThan(PlayerGameListingPtr l, PlayerGameListingPtr r, int column) const;
	private:
		virtual Qt::SortOrder getSortOrder(void) const;
		virtual int getSortColumn(void) const;

		QString account_name;
		class ObserverSortProxy * sortProxy;
};

class PlayerListModel : public ListModel
{
	public:
		PlayerListModel();
		~PlayerListModel();
		void insertListing(PlayerListing * const l);
		void updateListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		PlayerListing * playerListingFromIndex(const QModelIndex &);
		void setAccountName(QString name) { account_name = name; };
		void setSortProxy(class PlayerSortProxy * p) { sortProxy = p; };
		virtual bool lessThan(PlayerGameListingPtr l, PlayerGameListingPtr r, int column) const;
	private:
		virtual Qt::SortOrder getSortOrder(void) const;
		virtual int getSortColumn(void) const;
		
		QString account_name;
		class PlayerSortProxy * sortProxy;
};

class SimplePlayerListModel: public ListModel
{
	public:
		SimplePlayerListModel(bool _notify_column);
		~SimplePlayerListModel();
		void insertListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		PlayerListing * playerListingFromIndex(const QModelIndex &);
		class SimplePlayerSortProxy * getProxy(void) { return sortProxy; };
		virtual bool lessThan(PlayerGameListingPtr l, PlayerGameListingPtr r, int column) const;
	private:
		virtual Qt::SortOrder getSortOrder(void) const;
		virtual int getSortColumn(void) const;

		bool notify_column;
		class SimplePlayerSortProxy * sortProxy;
};

class GamesListModel : public ListModel
{
	public:
		GamesListModel();
		~GamesListModel();
		void insertListing(GameListing * const l);
		void updateListing(GameListing * const l);
		void removeListing(GameListing * const l);
		GameListing * gameListingFromIndex(const QModelIndex &);
		void setSortProxy(class GamesSortProxy * p) { sortProxy = p; };
		virtual bool lessThan(PlayerGameListingPtr l, PlayerGameListingPtr r, int column) const;
	private:
		virtual Qt::SortOrder getSortOrder(void) const;
		virtual int getSortColumn(void) const;
		
		class GamesSortProxy * sortProxy;
};

class ObserverSortProxy : public QSortFilterProxyModel
{
	public:
		bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

class PlayerSortProxy : public QSortFilterProxyModel
{
	/* IGS ranks probably up to 10000, but ORO would need 100000 so... */
	public:
		PlayerSortProxy() : rankMin(0), rankMax(100000), flags(none) {};
		void setFilter(int rn, int rm);
		enum PlayerSortFlags { none = 0x0, open = 0x1, friends = 0x2, fans = 0x4, noblock = 0x8 };
		void setFilter(enum PlayerSortFlags f);
		bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const;
        bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	private:
		int rankMin;
		int rankMax;
		unsigned char flags;
};

class SimplePlayerSortProxy : public QSortFilterProxyModel
{
	public:
		bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

class GamesSortProxy : public QSortFilterProxyModel
{
	public:
		GamesSortProxy() : watches(false) {};
		void toggleWatches(void);
		bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const;
		bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	private:
		bool watches;
};

extern std::map<class PlayerListing *, unsigned short> removed_player;
