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
            return QVariant("Match prefs");
        }
    return QVariant();
}

void PlayerListModel::removeListing(PlayerListing * const l)
{
    int i = items.indexOf(l);
    if (i != -1)
    {
        beginRemoveRows(QModelIndex(), i, i);
        items.removeAt(i);
        endRemoveRows();
    }
}

void PlayerListModel::clearList(void)
{
    if(items.count() == 0)
        return;
    beginResetModel();
    items.clear();
    endResetModel();
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
        case PC_PLAYING:
            if(item->playing == 0)
                return QVariant("-");
            else
                return QVariant(item->playing);
        case PC_OBSERVING:
            if(item->observing == 0 ||
                    item->observing == item->playing)
                return QVariant("-");
            else
                return QVariant(item->observing);
        case PC_WINS:
            return QVariant(item->wins);
        case PC_LOSSES:
            return QVariant(item->losses);
        case PC_IDLE:
            if (role == LIST_SORT_ROLE)
                return QVariant(item->seconds_idle);
            else
                return QVariant(item->idletime);
        case PC_COUNTRY:
            return QVariant(item->country);
        case PC_MATCHPREFS:
            //is this right?  FIXME takes up too much space, should go with simple numbers, abbrev.
            return QVariant(item->nmatch_settings);
        case PC_NOTIFY:		//was used for SimplePlayerListModel
            return QVariant(item->notify);
        case PC_ONLINE:
            return QVariant(item->online);
        default:
            return QVariant();
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

PlayerListing * PlayerListModel::updateEntry(PlayerListing * listing)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = static_cast<PlayerListing*>(items[i]);
        if (result == listing)
        {
            emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            return result;
        }
    }
    return NULL;
}

void PlayerListModel::insertListing(PlayerListing *item)
{
    beginInsertRows(QModelIndex(), items.count(), items.count());
    items.append(item);
    endInsertRows();
}

int PlayerListModel::rowCount(const QModelIndex &) const
{
    return items.count();
}

QModelIndex PlayerListModel::index ( int row, int column, const QModelIndex & ) const
{
    return createIndex(row,column,(void*)items[row]);
}

Qt::ItemFlags PlayerListModel::flags(const QModelIndex & index) const
{
    if(!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
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
        case OC_RANK:
            return item->rank;
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
        case SPC_NOTIFY:
            return item->notify;
        default:
            return QVariant();
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
    clearList();
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

GameListing * GamesListModel::getEntry(unsigned int id)
{
    for (int i=0; i < items.count(); i++)
    {
        if (items[i]->number == id)
            return items[i];
    }
    return NULL;
}

void GamesListModel::updateListing(GameListing * listing)
{
    int i = items.indexOf(listing); // Returns index if found or -1 if not found
    if (i != -1)
        emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
}

void GamesListModel::clearList(void)
{
    beginRemoveRows(QModelIndex(), 0, items.count() - 1);
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
        case GC_WHITENAME:
            return QVariant(item->white_name());
        case GC_WHITERANK:
            if (role == LIST_SORT_ROLE)
                return QVariant(item->white_rank_score());
            else
                return QVariant(item->white_rank());
        case GC_BLACKNAME:
            return QVariant(item->black_name());
        case GC_BLACKRANK:
            if (role == LIST_SORT_ROLE)
                return QVariant(item->black_rank_score());
            else
                return QVariant(item->black_rank());
        case GC_MOVES:
            if(item->moves == (unsigned)-1)
                return QVariant("-");
            return QVariant(item->moves);
        case GC_SIZE:
            return QVariant(item->board_size);
        case GC_HANDICAP:
            return QVariant(item->handicap);
        case GC_KOMI:
            return QVariant(item->komi);
        case GC_BYOMI:
            return QVariant(item->By);
        case GC_FLAGS:
            return QVariant(item->FR);
        case GC_OBSERVERS:
            return QVariant(item->observers);
        case GC_RUNNING:
            return QVariant(item->running);
        default:
            return QVariant();
        }
    }
    return QVariant();
}

GameListing * GamesListModel::gameListingFromIndex(const QModelIndex & index)
{
    return static_cast<GameListing*>(index.internalPointer());
}

void GamesListModel::insertListing(GameListing *item)
{
    beginInsertRows(QModelIndex(), items.count(), items.count());
    items.append(item);
    endInsertRows();
    emit countChanged(items.count());
}

int GamesListModel::rowCount(const QModelIndex &) const
{
    return items.count();
}

QModelIndex GamesListModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row,column,(void*)items[row]);
}

PlayerListSortFilterProxyModel::PlayerListSortFilterProxyModel(QObject * parent) :
    QSortFilterProxyModel(parent), rankMin(0), rankMax(100000), open(false), friends(false), fans(false), noblock(false)
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

    if(!(p->online) || p->hidden)
        return false;
    if(!(noblock))
    {
        if(p->friendWatchType == PlayerListing::blocked)
            return false;
    }
    if(open)
    {
        if(p->info.contains("X") || p->playing != 0)
            return false;
    }
    if(p->rank_score > (unsigned int)rankMax || p->rank_score + 1 < (unsigned int)rankMin)
        return false;
    if(friends || fans)
    {
        if(friends && p->friendWatchType == PlayerListing::friended)
            return true;
        if(fans && p->friendWatchType == PlayerListing::watched)
            return true;
        return false;
    }

    return true;
}

void PlayerListSortFilterProxyModel::setFilterOpen(bool state)
{
    if (open != state)
    {
        open = state;
        invalidateFilter();
    }
}

void PlayerListSortFilterProxyModel::setFilterFriends(bool state)
{
    if (friends != state)
    {
        friends = state;
        invalidateFilter();
    }
}

void PlayerListSortFilterProxyModel::setFilterFans(bool state)
{
    if (fans != state)
    {
        fans = state;
        invalidateFilter();
    }
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
    if (watches != state)
    {
        watches = state;
        invalidateFilter();
    }
}

bool GamesListSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    const GameListing * g = static_cast<GamesListModel*>(sourceModel())->items[sourceRow];
    if(!(g->running) || (g->isRoomOnly)) // It is very weird that rooms are saved as games, FIXME
        return false;
    if(watches)
    {
        return ((g->black && g->black->friendWatchType == PlayerListing::watched) ||
                (g->white && g->white->friendWatchType == PlayerListing::watched));
    }
    return true;
}
