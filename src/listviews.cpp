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


#include "listviews.h"
#include "network/messages.h"
#include "playergamelistings.h"
#include "gamedata.h"
#include <QColor>

ObserverListModel::ObserverListModel()
{
}

ObserverListModel::~ObserverListModel()
{
}

void ObserverListModel::insertListing(PlayerListing * const listing)
{
	for(int i = 0; i < items.count(); i++)
	{
        if(((PlayerListing *)items[i])->name == listing->name)
			return;
	}
    PlayerListing * item = new PlayerListing(*listing);
    ListModel::insertListing(item);
}

void ObserverListModel::removeListing(PlayerListing * const listing)
{
	for(int i = 0; i < items.count(); i++)
	{
        const PlayerListing * item = static_cast<const PlayerListing *>(items[i]);
        if(item == listing)
		{
            beginRemoveRows(QModelIndex(), i, i);
			items.removeAt(i);
            endRemoveRows();
			delete item;
			return;
		}
	}
}

int ObserverListModel::columnCount(const QModelIndex &) const
{
    return O_TOTALCOLUMNS;
}

QVariant ObserverListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch(section)
        {
        case OC_NAME:
            return QVariant("Name");
        case OC_RANK:
            return QVariant("Rank");
        }
    return QVariant();
}

void ObserverListModel::clearList(void)
{
	if(items.count() == 0)
		return;
    beginResetModel();
    for(int i=0; i < items.count(); i++)
	{
        const PlayerListing * item = static_cast<const PlayerListing *>(items[i]);
		delete item;
	}
    items.clear();
    endResetModel();
}

QVariant ObserverListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
		return QVariant();

    PlayerListing * item = static_cast<PlayerListing*>(index.internalPointer());
	if(role == Qt::DisplayRole)
    {
        switch(index.column())
            {
                case OC_NAME:
                    return item->name;
                    break;
                case OC_RANK:
                    return item->rank;
                    break;
                default:
                    return QVariant();
                    break;
            }
    }
	else if(role == Qt::ForegroundRole)
	{
        if(item->name == account_name)
            return QColor(Qt::blue);
		else
			return QVariant();
	}
	else
		return QVariant();
}

PlayerListModel::PlayerListModel()
{
}

PlayerListModel::~PlayerListModel()
{
}

int PlayerListModel::columnCount(const QModelIndex &) const
{
    return P_TOTALCOLUMNS;
}

QVariant PlayerListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch(section)
        {
        case PC_STATUS:
            return QVariant("Stat");
        case PC_NAME:
            return QVariant("Name");
        case PC_RANK:
            return QVariant("Rank");
        case PC_PLAYING:
            return QVariant("Play");
        case PC_OBSERVING:
            return QVariant("Obs");
        case PC_WINS:
            return QVariant("Won");
        case PC_LOSSES:
            return QVariant("Lost");
        case PC_IDLE:
            return QVariant("Idle");
        case PC_COUNTRY:
            return QVariant("Country");
        case PC_MATCHPREFS:
            return QVariant("March prefs");
        }
    return QVariant();
}

void PlayerListModel::insertListing(PlayerListing * const l)
{
    PlayerListing * item = new PlayerListing(*l);
    ListModel::insertListing(item);
}

void PlayerListModel::removeListing(PlayerListing * const l)
{
	for(int i = 0; i < items.count(); i++)
	{
        PlayerListing * item = static_cast<PlayerListing *>(items[i]);
        if(item == l)
		{
            removeRow(i);
			delete item;
			return;
		}
	}
}

void PlayerListModel::clearList(void)
{
    const int listSize = items.size();
    if(listSize == 0)
		return;
    beginRemoveRows(QModelIndex(), 0, listSize - 1);
    for(int i = 0; i < items.count(); i++)
    {
        const PlayerListing * item = static_cast<const PlayerListing *>(items[i]);
        delete item;
    }
    items.clear();
    endRemoveRows();
}

QVariant PlayerListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
        return QVariant();
	
    PlayerListing * item = static_cast<PlayerListing*>(index.internalPointer());
	
    if (role == Qt::ForegroundRole)
        {
            if(item->name == account_name)
                return QColor(Qt::blue);
            else
                return QVariant();
        }
    else if ((role == Qt::DisplayRole) || (role == LIST_SORT_ROLE))
    {
        switch(index.column())
            {
                case PC_STATUS:
                    return QVariant(item->info);
                    break;
                case PC_NAME:
                    return QVariant(item->name);
                    break;
                case PC_RANK:
            if (role == LIST_SORT_ROLE)
                return QVariant(item->rank_score);
            else
                return QVariant(item->rank);
                    break;
                case PC_PLAYING:
                    if(item->playing == 0)
                        return QVariant("-");
                    return QVariant(item->playing);
                    break;
                case PC_OBSERVING:
                    if(item->observing == 0 ||
                        item->observing == item->playing)
                        return QVariant("-");
                    return QVariant(item->observing);
                    break;
                case PC_WINS:
                    return QVariant(item->wins);
                    break;
                case PC_LOSSES:
                    return QVariant(item->losses);
                    break;
                case PC_IDLE:
            if (role == LIST_SORT_ROLE)
                return QVariant(item->seconds_idle);
            else
                return QVariant(item->idletime);
                    break;
                case PC_COUNTRY:
                    return QVariant(item->country);
                    break;
                case PC_MATCHPREFS:
                    //is this right?  FIXME takes up too much space, should go with simple numbers, abbrev.
                    return QVariant(item->nmatch_settings);
                    break;
                case PC_NOTIFY:		//was used for SimplePlayerListModel
                    return QVariant(item->notify);
                    break;
                default:
                    return QVariant();
                    break;
            }
    }
    return QVariant();
}

PlayerListing * PlayerListModel::playerListingFromIndex(const QModelIndex & index)
{
    return static_cast<PlayerListing*>(index.internalPointer());
}


PlayerListing * PlayerListModel::getEntry(unsigned int id)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->id == id)
        {
            return result;
        }
    }
    return NULL;
}

PlayerListing * PlayerListModel::getEntry(const QString &name)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->name == name)
        {
            return result;
        }
    }
    return NULL;
}

PlayerListing * PlayerListModel::getPlayerFromNotNickName(const QString & notnickname)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->notnickname == notnickname)
            return result;
    }
    return NULL;
}

PlayerListing * PlayerListModel::updateEntryByName(PlayerListing * listing)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->name == listing->name)
        {
            *result = *listing;
            emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            return result;
        }
    }
    result = new PlayerListing(*listing); // Should not create a copy here? FIXME
    insertListing(result);
    return result;
}

PlayerListing * PlayerListModel::updateEntryByID(PlayerListing *listing)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->id == listing->id)
        {
            *result = *listing;
            emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            return result;
        }
    }
    result = new PlayerListing(*listing); // Should not create a copy here? FIXME
    insertListing(result);
    return result;
}

void PlayerListModel::deleteEntry(const QString & name)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->name == name)
        {
            removeRow(i);
            delete result;
            return;
        }
    }
    return;
}

void PlayerListModel::deleteEntry(unsigned int id)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result->id == id)
        {
            removeRow(i);
            delete result;
            return;
        }
    }
    return;
}

SimplePlayerListModel::SimplePlayerListModel(bool _notify_column)
{
    notify_column = _notify_column;
}

int SimplePlayerListModel::columnCount(const QModelIndex &) const
{
    if (notify_column)
        return 2;
    else
        return 1;
}

QVariant SimplePlayerListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch(section)
        {
        case 0:
            return QVariant("Name");
        case 1:
            return QVariant("Notify");
        }
    return QVariant();
}

QVariant SimplePlayerListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
		return QVariant();

    PlayerListing * item = static_cast<PlayerListing*>(index.internalPointer());
	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
			case SPC_NAME:
                return item->name;
				break;
			case SPC_NOTIFY:
                return item->notify;
				break;
			default:
				return QVariant();
				break;
		}
	}
	else
		return QVariant();
}

GamesListModel::GamesListModel()
{
}

GamesListModel::~GamesListModel()
{
}

int GamesListModel::columnCount(const QModelIndex &) const
{
    return G_TOTALCOLUMNS;
}

QVariant GamesListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch(section)
        {
        case GC_ID:
            return QVariant("Id");
        case GC_WHITENAME:
            return QVariant("White");
        case GC_WHITERANK:
            return QVariant("WR");
        case GC_BLACKNAME:
            return QVariant("Black");
        case GC_BLACKRANK:
            return QVariant("BR");
        case GC_MOVES:
            return QVariant("Mv");
        case GC_SIZE:
            return QVariant("Sz");
        case GC_HANDICAP:
            return QVariant("H");
        case GC_KOMI:
            return QVariant("K");
        case GC_BYOMI:
            return QVariant("B");
        case GC_FLAGS:
            return QVariant("Flags");
        case GC_OBSERVERS:
            return QVariant("Obs");
        }
    return QVariant();
}

void GamesListModel::insertListing(GameListing * const l)
{
    GameListing * item = new GameListing(*l);
    ListModel::insertListing(item);
}

void GamesListModel::removeListing(GameListing * const l)
{
	for(int i = 0; i < items.count(); i++)
	{
        const GameListing * item = static_cast<const GameListing *>(items[i]);
        if(item == l)
		{
            removeRow(i);
            delete item;
			return;
		}
	}
	/* IGS with updates does get here */
	qDebug("Couldn't find listing to remove for game id %d", l->number);
}

GameListing * GamesListModel::getEntry(unsigned int id)
{
    GameListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<GameListing*>(items[i]);
        if (result->number == id)
        {
            return result;
        }
    }
    return NULL;
}

GameListing * GamesListModel::updateListing(const GameListing * listing)
{
    GameListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<GameListing*>(items[i]);
        if (result->number == listing->number)
        {
            *result = *listing;
            emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            return result;
        }
    }
    result = new GameListing(*listing); // Should not create a copy here? FIXME
    insertListing(result);
    return result;
}

void GamesListModel::deleteEntry(unsigned int id)
{
    GameListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<GameListing*>(items[i]);
        if (result->number == id)
        {
            removeRow(i);
            delete result->gameData;		//used by ORO
            delete result;
            return;
        }
    }
    return;
}

void GamesListModel::clearList(void)
{
    beginRemoveRows(QModelIndex(), 0, items.count() - 1);
    for(int i = 0; i < items.count(); i++)
	{
        const GameListing * item = static_cast<const GameListing *>(items[i]);
		delete item;
	}
    items.clear();
    endRemoveRows();
}

QVariant GamesListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
		return QVariant();
	
    GameListing * item = static_cast<GameListing*>(index.internalPointer());
    if ((role == Qt::DisplayRole) || (role == LIST_SORT_ROLE))
    {
        switch(index.column())
            {
                case GC_ID:
                    return QVariant(item->number);
                    break;
                case GC_WHITENAME:
                    //qDebug("%s", item->white_name().toLatin1().constData());
                    return QVariant(item->white_name());
                    break;
                case GC_WHITERANK:
                    //qDebug("%s", item->white_rank().toLatin1().constData());
            if (role == LIST_SORT_ROLE)
                return QVariant(item->white_rank_score());
            else
                return QVariant(item->white_rank());
                    break;
                case GC_BLACKNAME:
                    //qDebug("%s", item->black_name().toLatin1().constData());
                    return QVariant(item->black_name());
                    break;
                case GC_BLACKRANK:
            if (role == LIST_SORT_ROLE)
                return QVariant(item->black_rank_score());
            else
                return QVariant(item->black_rank());
                    break;
                case GC_MOVES:
                    if(item->moves == (unsigned)-1)
                        return QVariant("-");
                    return QVariant(item->moves);
                    break;
                case GC_SIZE:
                    return QVariant(item->board_size);
                    break;
                case GC_HANDICAP:
                    return QVariant(item->handicap);
                    break;
                case GC_KOMI:
                    return QVariant(item->komi);
                    break;
                case GC_BYOMI:
                    return QVariant(item->By);
                    break;
                case GC_FLAGS:
                    return QVariant(item->FR);
                    break;
                case GC_OBSERVERS:
                    return QVariant(item->observers);
                    break;
                default:
                    return QVariant();
                    break;
            }
    }
    return QVariant();
}

GameListing * GamesListModel::gameListingFromIndex(const QModelIndex & index)
{
    return static_cast<GameListing*>(index.internalPointer());
}

void ListModel::insertListing(void *item)
{
    beginInsertRows(QModelIndex(), items.count(), items.count());
    items.append(item);
    endInsertRows();
    emit countChanged(items.count());
}

ListModel::ListModel()
{
}

ListModel::~ListModel()
{
}

int ListModel::rowCount(const QModelIndex &) const
{
	return items.count();
}

int ListModel::columnCount(const QModelIndex &) const
{
    return 0; // Overriden by inheriting classes
}

QModelIndex ListModel::index ( int row, int column, const QModelIndex & ) const
{
    return createIndex(row,column,items[row]);
}


/* This is overridden */
QVariant ListModel::data(const QModelIndex & index, int role) const
{
    return QVariant();
}

Qt::ItemFlags ListModel::flags(const QModelIndex & index) const
{
	if(!index.isValid())
		return Qt::ItemIsEnabled;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    count--;
    if (items.size() <= row+count)
        return false;
    beginRemoveRows(QModelIndex(), row, row+count);
    for(; count>=0; count--)
        items.removeAt(row);
    endRemoveRows();
    emit countChanged(items.count());
    return true;
}

PlayerListSortFilterProxyModel::PlayerListSortFilterProxyModel(QObject * parent) :
    QSortFilterProxyModel(parent), rankMin(0), rankMax(100000), flags(none)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(PC_NAME);
    this->setSortRole(LIST_SORT_ROLE);
}

bool PlayerListSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (! QSortFilterProxyModel::filterAcceptsRow(sourceRow,sourceParent))
        return false;

	const PlayerListing * p;
    p = static_cast<PlayerListing*>(static_cast<PlayerListModel*>(sourceModel())->items[sourceRow]);
	
	if(p->hidden)
		return false;
	if(!(flags & noblock))
	{
		if(p->friendWatchType == PlayerListing::blocked)
			return false;
	}
	if(flags & friends)
	{
		if(p->friendWatchType != PlayerListing::friended)
		{
			if(flags & fans)
			{
				if(p->friendWatchType != PlayerListing::watched)
					return false;
			}
			else
				return false;
		}
	}
	else if(flags & fans)
	{
		if(p->friendWatchType != PlayerListing::watched)
			return false;	
	}
	
	if(flags & open)
	{
		if(p->info.contains("X") ||
			p->playing != 0)
			return false;
	}
	
	if(p->rank_score > (unsigned int)rankMax || p->rank_score + 1 < (unsigned int)rankMin)
		return false;
	return true;
}

void PlayerListSortFilterProxyModel::setFilterOpen(bool state)
{
    if (state)
        flags |= open;
    else
        flags &= (~open);
    invalidateFilter();
}

void PlayerListSortFilterProxyModel::setFilterFriends(bool state)
{
    if (state)
        flags |= friends;
    else
        flags &= (~friends);
    invalidateFilter();
}

void PlayerListSortFilterProxyModel::setFilterFans(bool state)
{
    if (state)
        flags |= fans;
    else
        flags &= (~fans);
    invalidateFilter();
}

void PlayerListSortFilterProxyModel::setFilterMinRank(int rank)
{
    rankMin = rank;
    invalidateFilter();
}

void PlayerListSortFilterProxyModel::setFilterMaxRank(int rank)
{
    rankMax = rank;
    invalidateFilter();
}

GamesListSortFilterProxyModel::GamesListSortFilterProxyModel(QObject * parent) :
    QSortFilterProxyModel(parent), watches(false)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(LIST_SORT_ROLE);
}

void GamesListSortFilterProxyModel::setFilterWatch(bool state)
{
    watches = state;
    invalidateFilter();
}

bool GamesListSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	const GameListing * g;
    g = static_cast<GameListing *>(static_cast<GamesListModel*>(sourceModel())->items[sourceRow]);
	if(watches)
	{
		if((g->black && g->black->friendWatchType == PlayerListing::watched) ||
			(g->white && g->white->friendWatchType == PlayerListing::watched))
			return true;
		return false;
	}
	return true;
}
