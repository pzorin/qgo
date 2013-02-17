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

#define QUICKQUICKSORT

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

ObserverListModel::ObserverListModel()
{
    for(int i = 0; i < columnCount(); i++)
    {
        sort_priority.append(i);
        sort_order.append(Qt::AscendingOrder);
    }
}

ObserverListModel::~ObserverListModel()
{
}

void ObserverListModel::insertListing(PlayerListing * const listing)
{
	for(int i = 0; i < items.count(); i++)
	{
		if(((ObserverListItem *)items[i])->data(OC_NAME) == listing->name)
			return;
	}
	ObserverListItem * item = new ObserverListItem(listing);
	ListModel::insertListing(*item);	
}

void ObserverListModel::removeListing(PlayerListing * const listing)
{
	for(int i = 0; i < items.count(); i++)
	{
		const ObserverListItem * item = static_cast<const ObserverListItem *>(items[i]);
		if(item->getListing() == listing)
		{
			emit beginRemoveRows(QModelIndex(), i, i);
			items.removeAt(i);
			emit endRemoveRows();
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
    emit beginResetModel();
    for(int i=0; i < items.count(); i++)
	{
        const ObserverListItem * item = static_cast<const ObserverListItem *>(items[i]);
		delete item;
	}
    items.clear();
    emit endResetModel();
}

QVariant ObserverListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
		return QVariant();

    ObserverListItem * item = static_cast<ObserverListItem*>(index.internalPointer());
	if(role == Qt::DisplayRole)
		return item->data(index.column());
	else if(role == Qt::ForegroundRole)
	{
		if(item->getListing() && item->getListing()->name == account_name)
			return Qt::blue;
		else
			return QVariant();
	}
	else
		return QVariant();
}

PlayerListModel::PlayerListModel()
{
    for(int i = 0; i < columnCount(); i++)
    {
        sort_priority.append(i);
        sort_order.append(Qt::AscendingOrder);
    }
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
	PlayerListItem * item = new PlayerListItem(l);	
	ListModel::insertListing(*item);
}

void PlayerListModel::updateListing(PlayerListing * const l)
{
	//if(!l)
	//	return;	//nothing to update
	//removeListing(l);
	for(int i = 0; i < items.count(); i++)
	{
		if(static_cast<PlayerListItem const *>(items[i])->getListing() == l)
		{
			//this listing needs to be reloaded
            if(view) view->_dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
			return;
		}
	}
	insertListing(l);
}

void PlayerListModel::removeListing(PlayerListing * const l)
{
	for(int i = 0; i < items.count(); i++)
	{
		PlayerListItem * item = static_cast<PlayerListItem *>(items[i]);
		if(item->getListing() == l)
		{
			emit beginRemoveRows(QModelIndex(), i, i);
			items.removeAt(i);
			emit endRemoveRows();
			delete item;
			if(view) view->_dataChanged(createIndex(0, 0, 0),
                    createIndex(items.count() - 1, columnCount() - 1, 0));
			return;
		}
	}
}

void PlayerListModel::clearList(void)
{
    const int listSize = items.size();
    if(listSize == 0)
		return;
    emit beginRemoveRows(QModelIndex(), 0, listSize - 1);
    for(int i = 0; i < items.count(); i++)
    {
        const PlayerListItem * item = static_cast<const PlayerListItem *>(items[i]);
        delete item;
    }
    items.clear();
    emit endRemoveRows();
}

QVariant PlayerListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
        return QVariant();
	
    PlayerListItem * item = static_cast<PlayerListItem*>(index.internalPointer());
	
	if(role == Qt::DisplayRole)
		return item->data(index.column());
	else if(role == Qt::ForegroundRole)
	{
		if(item->getListing()->name == account_name)
			return Qt::blue;
		else
			return QVariant();
	}
	else
		return QVariant();
}

PlayerListing * PlayerListModel::playerListingFromIndex(const QModelIndex & index)
{
	//PlayerListItem * item = static_cast<PlayerListItem*>(index.internalPointer());
	return playerListItemFromIndex(index)->getListing();
}

PlayerListItem * PlayerListModel::playerListItemFromIndex(const QModelIndex & index) const
{
    return static_cast<PlayerListItem*>(items[index.row()]);
}

void PlayerListModel::sort(int column, Qt::SortOrder order)
{
	ListModel::sort(column, order);
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

    PlayerListItem * item = static_cast<PlayerListItem*>(index.internalPointer());
	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
			case SPC_NAME:
				return item->data(PC_NAME);
				break;
			case SPC_NOTIFY:
				return item->data(PC_NOTIFY);
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
    for(int i = 0; i < columnCount(); i++)
    {
        sort_priority.append(i);
        sort_order.append(Qt::AscendingOrder);
    }
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
	GamesListItem * item = new GamesListItem(l);	
	ListModel::insertListing(*item);	
	/*emit dataChanged(createIndex(0, 0),
			createIndex(items.count() - 1, headerItem->columnCount() - 1));
	*/
}

void GamesListModel::updateListing(GameListing * const l)
{
	//if(!l)
	//	return;	//nothing to update
	/* FIXME:
	 * I've come to the conclusion that this isn't really a bug.  It has
	 * to do with the template registries.  We sort of misused our
	 * own template when we adapted to the player/games registries.
	 * Specifically, if there's no player or game there, there's no
	 * information to create one.  We could create it with just the
	 * name itself, but then we might as well wait to get the stats
	 * information.  At any rate, that means we pass a null to the initEntry
	 * which then calls this.  I think... I think I'm going to alter it to
	 * send out stats requests all around */
	//if(!l)
	//	qDebug("GamesListModel::updateListing passed null listing!!!!!");
	for(int i = 0; i < items.count(); i++)
	{
		if(static_cast<const GamesListItem *>(items[i])->getListing() == l)
		{
			//this item needs to be reloaded
            emit dataChanged(createIndex(i,0), createIndex(i, columnCount() - 1));
			return;
		}
	}
	insertListing(l);
}

void GamesListModel::removeListing(GameListing * const l)
{
	for(int i = 0; i < items.count(); i++)
	{
		const GamesListItem * item = static_cast<const GamesListItem *>(items[i]);
		if(item->getListing() == l)
		{
			/* Really this is supposed to be not QModelIndex() but the
			 * parent model index of the model... ?!?? */
			emit beginRemoveRows(QModelIndex(), i, i);
			items.removeAt(i);
			emit endRemoveRows();
			//qDebug("Removing %p %s %p %s", item->getListing()->white, item->getListing()->white_name().toLatin1().constData(),
			 //     item->getListing()->black, item->getListing()->black_name().toLatin1().constData());
			delete item;
			return;
		}
	}
	/* IGS with updates does get here */
	qDebug("Couldn't find listing to remove for game id %d", l->number);
}

void GamesListModel::clearList(void)
{
    if (items.count() == 0)
        return;

    emit beginRemoveRows(QModelIndex(), 0, items.count() - 1);
    for(int i = 0; i < items.count(); i++)
	{
        const GamesListItem * item = static_cast<const GamesListItem *>(items[i]);
		delete item;
	}
    items.clear();
    emit endRemoveRows();
}

QVariant GamesListModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
		return QVariant();
	
    GamesListItem * item = static_cast<GamesListItem*>(index.internalPointer());
	if(role == Qt::DisplayRole)
		return item->data(index.column());
	else if(role == Qt::ForegroundRole)
	{
		return QVariant();
	}
	else
		return QVariant();
}

GameListing * GamesListModel::gameListingFromIndex(const QModelIndex & index)
{
    GamesListItem * item = static_cast<GamesListItem*>(items[index.row()]);
	return item->getListing();
}

/* FIXME we should possibly use that order variable somehow. */
void ListModel::sort(int column, Qt::SortOrder order)
{
	/* Take column out of previous place in sort_priority and
	 * put it in first place */
    for(int i = 0; i < columnCount(); i++)
    {
        if(sort_priority[i] == column)
        {
            sort_priority.removeAt(i);
            sort_priority.prepend(column);
            sort_order.removeAt(i);
            sort_order.prepend(order);

            //qDebug("sort! %d %d\n", sort_priority[0], order);
            quicksort(0, items.count() - 1);
            break;
        }
    }
}

void ListModel::quicksort(int b, int e)
{
	if(b < e)
	{
		int partition = qs_partition(b, e);
		quicksort(b, partition - 1);
		quicksort(partition + 1, e);
	}
}

int ListModel::qs_partition(int b, int e)
{
	ListItem * item, * pivot_item;
	pivot_item = items[e];
	int i, j;
	i = b - 1;
	for(j = b; j < e; j++)
	{
		item = items[j];
		switch(priorityCompare(*item, *pivot_item))
		{
			case LESSTHAN:
			case EQUALTO:
				i++;
				items[j] = items[i];
				items[i] = item;
				break;
			case GREATERTHAN:
				break;
		}
	}
	items[e] = items[i + 1];
	items[i + 1] = pivot_item;
	return i + 1;
}

/* With a large list, if you're looking at the middle of it and its
 * changing quickly, then all those new entries are shifting the list
 * around.  Even if the selection stays where it is, its hard to
 * even double click.  I might change this back later, but for
 * now, let's see what it looks like with just, essentially, appends */
/* The old fix performed very badly (application basically unusable),
 * so I have removed it for the time being.
 * I might revisit it when models are ported to QAbstractTableModel */
void ListModel::insertListing(ListItem & item)
{
    emit beginInsertRows(QModelIndex(), items.count(), items.count());
    items.append(&item);
    emit endInsertRows();
}

int ListModel::priorityCompare(const ListItem & i, const ListItem & j)
{
    int k = 0;
    int columns = columnCount(QModelIndex());
    int result;
    do {
        result = i.compare(j, sort_priority[k]);
        if (sort_order[k] == Qt::DescendingOrder)
            result = -result;
        k++;
    } while (result == EQUALTO && k < 3 && k < columns - 1);
    return result;
}

ObserverListItem::ObserverListItem(PlayerListing * const l) : listing(l)
{
}

QVariant ObserverListItem::data(int column) const
{
	switch(column)
	{
		case OC_NAME:
			return listing->name;
			break;
		case OC_RANK:
			return listing->rank;
			break;
		default:
			return QVariant();
			break;
	}
}

int ObserverListItem::columnCount(void) const
{
	return O_TOTALCOLUMNS;
}

int ObserverListItem::compare(const ListItem & i, int column) const
{
	const ObserverListItem & l = (const ObserverListItem &)i;
	switch(column)
	{
		case OC_NAME:
			return compareNames(listing->name, l.listing->name);
			break;
		case OC_RANK:
			if(listing->rank_score > l.listing->rank_score)
				return GREATERTHAN;
			else if(listing->rank_score == l.listing->rank_score)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		default:
			break;
	}
	return EQUALTO;
}

GamesListItem::GamesListItem(GameListing * const l) : listing(l)
{
}

int GamesListItem::compare(const ListItem & i, int column) const
{
	const GamesListItem & g = (const GamesListItem &)i;
	if(!listing)
	{
		qDebug("!!! No listing on GamesListItem!!!");
		return LESSTHAN;
	}
	switch((GameListingColumn)column)
	{
		case GC_ID:
			if(listing->number > g.getListing()->number)
				return GREATERTHAN;
			else if(listing->number == g.getListing()->number)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_WHITENAME:
			return compareNames(listing->white_name(), g.getListing()->white_name());
			break;
		case GC_WHITERANK:
			if(listing->white_rank_score() > g.getListing()->white_rank_score())
				return GREATERTHAN;
			else if(listing->white_rank_score() == g.getListing()->white_rank_score())
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_BLACKNAME:
			return compareNames(listing->black_name(), g.getListing()->black_name());
			break;
		case GC_BLACKRANK:
			if(listing->black_rank_score() > g.getListing()->black_rank_score())
				return GREATERTHAN;
			else if(listing->black_rank_score() == g.getListing()->black_rank_score())
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_MOVES:
			if(listing->moves > g.getListing()->moves)
				return GREATERTHAN;
			else if(listing->moves == g.getListing()->moves)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_SIZE:
			if(listing->board_size > g.getListing()->board_size)
				return GREATERTHAN;
			else if(listing->board_size == g.getListing()->board_size)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_HANDICAP:
			if(listing->handicap > g.getListing()->handicap)
				return GREATERTHAN;
			else if(listing->handicap == g.getListing()->handicap)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_KOMI:
			if(listing->komi > g.getListing()->komi)
				return GREATERTHAN;
			else if(listing->komi == g.getListing()->komi)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_BYOMI:
			if(listing->By > g.getListing()->By)
				return GREATERTHAN;
			else if(listing->By == g.getListing()->By)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_FLAGS:
			if(listing->FR > g.getListing()->FR)
				return GREATERTHAN;
			else if(listing->FR == g.getListing()->FR)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case GC_OBSERVERS: 
			if(listing->observers > g.getListing()->observers)
				return GREATERTHAN;
			else if(listing->observers == g.getListing()->observers)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		default:
			break;
	}
	return EQUALTO;
}

/* Probably better ways to do this, an enum, something, maybe
 * an enum keyed with the names set in the header above, but
 * right now we're just getting it out there */
QVariant GamesListItem::data(int column) const
{
	if(!listing)
	{
		/* I think this has to do with us getting a games message
		 * from a game we're playing that isn't in the list FIXME */
		qDebug("!!! No listing on GamesListItem!!!");
		return QVariant();
	}
	
	std::map<PlayerListing *, unsigned short>::iterator it;
	if(listing->white)
		it = removed_player.find(listing->white);
	if(listing->white && it != removed_player.end())
	{
		qDebug("Listing: %p %d", listing, listing->number);
		qDebug("Found removed white player: %p, %d game %d", listing->white, removed_player[listing->white], listing->number);
		//QMessageBox here is BAD, its in some Qt draw loop here
		//QMessageBox::information(0 , "Crash Imminent!", "Game list corruption!");
		return QVariant();
	}
	if(listing->black)
		it = removed_player.find(listing->black);
	if(listing->black && it != removed_player.end())
	{
		qDebug("Listing: %p %d", listing, listing->number);
		qDebug("Found removed black player: %p, %d game %d", listing->black, removed_player[listing->black], listing->number);
		//QMessageBox::information(0 , "Crash Imminent!", "Game list corruption!");
		return QVariant();
	}
	switch(column)
	{
		case GC_ID:
			return QVariant(listing->number);
			break;
		case GC_WHITENAME:
			//qDebug("%s", listing->white_name().toLatin1().constData());
			return QVariant(listing->white_name());
			break;
		case GC_WHITERANK:
			//qDebug("%s", listing->white_rank().toLatin1().constData());
			return QVariant(listing->white_rank());
			break;
		case GC_BLACKNAME:
			//qDebug("%s", listing->black_name().toLatin1().constData());
			return QVariant(listing->black_name());
			break;
		case GC_BLACKRANK:
			return QVariant(listing->black_rank());
			break;
		case GC_MOVES:
			if(listing->moves == (unsigned)-1)
				return QVariant("-");
			return QVariant(listing->moves);
			break;
		case GC_SIZE:
			return QVariant(listing->board_size);
			break;
		case GC_HANDICAP:
			return QVariant(listing->handicap);
			break;
		case GC_KOMI:
			return QVariant(listing->komi);
			break;
		case GC_BYOMI:
			return QVariant(listing->By);
			break;
		case GC_FLAGS:
			return QVariant(listing->FR);
			break;
		case GC_OBSERVERS: 
			return QVariant(listing->observers);
			break;
		default:
			return QVariant();
			break;
	}
}

int GamesListItem::columnCount(void) const
{
	return G_TOTALCOLUMNS;
}

PlayerListItem::PlayerListItem(PlayerListing * const l) : listing(l)
{
}

/* We changed the playing and observing
 * to ints to point to game id but this
 * will be a problem if there's ever a game 0.
 * -1 would fix it but I'll hold off on it
 *  for now.  The way it is is better for sorting. */
QVariant PlayerListItem::data(int column) const
{
	switch(column)
	{
		case PC_STATUS:
			return QVariant(listing->info);
			break;
		case PC_NAME:
			return QVariant(listing->name);
			break;
		case PC_RANK:
			return QVariant(listing->rank);
			break;
		case PC_PLAYING:
			if(listing->playing == 0)
				return QVariant("-");
			return QVariant(listing->playing);
			break;
		case PC_OBSERVING:
			if(listing->observing == 0 ||
			  	listing->observing == listing->playing)
				return QVariant("-");
			return QVariant(listing->observing);
			break;
		case PC_WINS:
			return QVariant(listing->wins);
			break;
		case PC_LOSSES:
			return QVariant(listing->losses);
			break;
		case PC_IDLE:
			return QVariant(listing->idletime);
			break;
		case PC_COUNTRY:
			return QVariant(listing->country);
			break;
		case PC_MATCHPREFS:
			//is this right?  FIXME takes up too much space, should go with simple numbers, abbrev.
			return QVariant(listing->nmatch_settings);
			break;
		case PC_NOTIFY:		//awkward, but used for SimplePlayerListModel
			return QVariant(listing->notify);
			break;
		default:
			return QVariant();
			break;
	}
}

int PlayerListItem::columnCount(void) const
{
	return P_TOTALCOLUMNS;
}

int PlayerListItem::compare(const ListItem & i, int column) const
{
	const PlayerListItem & p = (const PlayerListItem &)i;
	switch((GameListingColumn)column)
	{
		case PC_STATUS:
			if(listing->info > p.getListing()->info)
				return GREATERTHAN;
			else if(listing->info == p.getListing()->info)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_NAME:
			return compareNames(listing->name, p.getListing()->name);
			break;
		case PC_RANK:
			if(listing->rank_score > p.getListing()->rank_score)
				return GREATERTHAN;
			else if(listing->rank_score == p.getListing()->rank_score)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_PLAYING:
			if(listing->playing > p.getListing()->playing)
				return GREATERTHAN;
			else if(listing->playing == p.getListing()->playing)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_OBSERVING:
			if(listing->observing > p.getListing()->observing)
				return GREATERTHAN;
			else if(listing->observing == p.getListing()->observing)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_WINS:
			if(listing->wins > p.getListing()->wins)
				return GREATERTHAN;
			else if(listing->wins == p.getListing()->wins)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_LOSSES:
			if(listing->losses > p.getListing()->losses)
				return GREATERTHAN;
			else if(listing->losses == p.getListing()->losses)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_IDLE:
			if(listing->seconds_idle > p.getListing()->seconds_idle)
				return GREATERTHAN;
			else if(listing->seconds_idle == p.getListing()->seconds_idle)
				return EQUALTO;
			else
				return LESSTHAN;
			break;
		case PC_COUNTRY:
			return compareNames(listing->country, p.getListing()->country);
			break;
		case PC_MATCHPREFS:
			/* FIXME Need to add this all around */
			break;
		default:
			break;
	}
	return EQUALTO;
}

ListItem::ListItem(const QList <QVariant> & data)
{
	itemData = data;	
}

QVariant ListItem::data(int column) const
{
	return itemData.value(column);
}

int ListItem::columnCount(void) const
{
	return itemData.count();
}

/* This assumes WAY WAY too much about the character sets and locale I think */
int ListItem::compareNames(const QString & name1, const QString & name2) const
{
	int shorter = (name1.count() < name2.count() ? name1.count() : name2.count());
	char c1, c2;
	for(int i = 0; i < shorter; i++)
	{
		c1 = (char)name1[i].toAscii();
		c2 = (char)name2[i].toAscii();
		if(c1 >= 'a')
			c1 -= ('a' - 'A');
		if(c2 >= 'a')
			c2 -= ('a' - 'A');
		if(c1 < c2)
			return LESSTHAN;
		else if(c1 > c2)
			return GREATERTHAN;
	}
	if(name1.count() < name2.count())
		return LESSTHAN;
	else if(name1.count() > name2.count())
		return GREATERTHAN;
	else
		return EQUALTO;
}

ListModel::ListModel()
{
	view = 0;
}

ListModel::~ListModel()
{
	const ListItem * item;
	for(int i = 0; i < items.count(); i++)
	{
		item = items[i];
		delete item;
	}
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
	if(!index.isValid())
		return QVariant();
	
	ListItem * item = static_cast<ListItem*>(index.internalPointer());
	if(role == Qt::DisplayRole)
		return item->data(index.column());
	else if(role == Qt::ForegroundRole)
		return QVariant();
	else
		return QVariant();
}

Qt::ItemFlags ListModel::flags(const QModelIndex & index) const
{
	if(!index.isValid())
		return Qt::ItemIsEnabled;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void FilteredView::_dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
	for(int i = topLeft.row(); i <= bottomRight.row(); i++)
	{
		if(listFilter->filterAcceptsRow(i))
		{	
            setRowHidden(i, false);
		}
		else
		{
            setRowHidden(i, true);
		}
	}
}

bool PlayerListFilter::filterAcceptsRow(int row) const
{
	const PlayerListing * p;
	p = static_cast<PlayerListItem*>(listModel->items[row])->getListing();
	
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

void PlayerListFilter::setFilter(int rn, int rm)
{
	// might be worth calling sort here ???
	// but what's the active sort column? That
	// needs to be fixed anyway with the ascending
	// stuff FIXME
	rankMin = rn;
	rankMax = rm;
	qDebug("player sort: %d - %d", rn, rm);
	listModel->sort(P_TOTALCOLUMNS);	//resort as current
}

void PlayerListFilter::setFilter(enum PlayerSortFlags f)
{
	// might be worth calling sort here ??? FIXME
	flags ^= f;
	listModel->sort(P_TOTALCOLUMNS);	//resort as current
}

void GamesListFilter::toggleWatches(void)
{
	watches = !watches;
	listModel->sort(G_TOTALCOLUMNS);
}

bool GamesListFilter::filterAcceptsRow(int row) const
{
	const GameListing * g;
	g = static_cast<GamesListItem *>(listModel->items[row])->getListing();
	if(watches)
	{
		if((g->black && g->black->friendWatchType == PlayerListing::watched) ||
			(g->white && g->white->friendWatchType == PlayerListing::watched))
			return true;
		return false;
	}
	return true;
}
