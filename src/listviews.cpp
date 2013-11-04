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
	PlayerListItem * item = new PlayerListItem(l);	
	ListModel::insertListing(*item);
}

void PlayerListModel::updateListing(PlayerListing * l)
{
	//if(!l)
	//	return;	//nothing to update
	//removeListing(l);
	for(int i = 0; i < items.count(); i++)
	{
		if(static_cast<PlayerListItem const *>(items[i])->getListing() == l)
		{
			//this listing needs to be reloaded
            emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
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
            removeItem(i);
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
            return QColor(Qt::blue);
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

PlayerListing * PlayerListModel::getEntry(unsigned int id, const PlayerListing * listing)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = ((PlayerListItem *)(items[i]))->getListing();
        if (result->id == id)
        {
            if (listing)
            {
                *result = *listing;
                emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            }
            return result;
        }
    }
    if (listing)
    {
        result = new PlayerListing(*listing); // Should not create a copy here? FIXME
        insertListing(result);
    } else
        result = NULL;
    return result;
}

PlayerListing * PlayerListModel::getEntry(const QString &name, const PlayerListing * listing)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = ((PlayerListItem *)(items[i]))->getListing();
        if (result->name == name)
        {
            if (listing)
            {
                *result = *listing;
                emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            }
            return result;
        }
    }
    if (listing)
    {
        result = new PlayerListing(*listing); // Should not create a copy here? FIXME
        insertListing(result);
    } else
        result = NULL;
    return result;
}

PlayerListing * PlayerListModel::getPlayerFromNotNickName(const QString & notnickname)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = ((PlayerListItem *)(items[i]))->getListing();
        if (result->notnickname == notnickname)
            return result;
    }
    return NULL;
}

void PlayerListModel::deleteEntry(const QString & name)
{
    PlayerListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = ((PlayerListItem *)(items[i]))->getListing();
        if (result->name == name)
        {
            removeItem(i);
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
        result = ((PlayerListItem *)(items[i]))->getListing();
        if (result->id == id)
        {
            removeItem(i);
            delete result;
            return;
        }
    }
    return;
}

ObserverListItemLessThan::ObserverListItemLessThan(int _column, Qt::SortOrder _order)
    : column(_column)
{
    reverseOrder = (_order == Qt::DescendingOrder);
}

bool ObserverListItemLessThan::operator()(const ListItem *left, const ListItem *right ) const
{
    bool result = true;
    switch(column)
    {
        case OC_NAME:
            result = ( QString::compare(((ObserverListItem *)left)->listing->name, ((ObserverListItem *)right)->listing->name, Qt::CaseInsensitive) < 0);
            break;
        case OC_RANK:
            result = (((ObserverListItem *)left)->listing->rank_score < ((ObserverListItem *)right)->listing->rank_score);
            break;
        default:
            break;
    }
    return result ^ reverseOrder;
}

GamesListItemLessThan::GamesListItemLessThan(int _column, Qt::SortOrder _order)
    : column(_column)
{
    reverseOrder = (_order == Qt::DescendingOrder);
}

bool GamesListItemLessThan::operator()(const ListItem *left, const ListItem *right ) const
{
    bool result = true;
    switch((GameListingColumn)column)
    {
        case GC_ID:
            result = (((GamesListItem*)left)->listing->number < ((GamesListItem*)right)->listing->number);
            break;
        case GC_WHITENAME:
            result = (QString::compare(((GamesListItem*)left)->listing->white_name(), ((GamesListItem*)right)->listing->white_name(), Qt::CaseInsensitive) < 0);
            break;
        case GC_WHITERANK:
            result = (((GamesListItem*)left)->listing->white_rank_score() < ((GamesListItem*)right)->listing->white_rank_score());
            break;
        case GC_BLACKNAME:
            result = (QString::compare(((GamesListItem*)left)->listing->black_name(), ((GamesListItem*)right)->listing->black_name(), Qt::CaseInsensitive) < 0);
            break;
        case GC_BLACKRANK:
            result = (((GamesListItem*)left)->listing->black_rank_score() < ((GamesListItem*)right)->listing->black_rank_score());
            break;
        case GC_MOVES:
            result = (((GamesListItem*)left)->listing->moves < ((GamesListItem*)right)->listing->moves);
            break;
        case GC_SIZE:
            result = (((GamesListItem*)left)->listing->board_size < ((GamesListItem*)right)->listing->board_size);
            break;
        case GC_HANDICAP:
            result = (((GamesListItem*)left)->listing->handicap < ((GamesListItem*)right)->listing->handicap);
            break;
        case GC_KOMI:
            result = (((GamesListItem*)left)->listing->komi < ((GamesListItem*)right)->listing->komi);
            break;
        case GC_BYOMI:
            result = (((GamesListItem*)left)->listing->By < ((GamesListItem*)right)->listing->By);
            break;
        case GC_FLAGS:
            result = (((GamesListItem*)left)->listing->FR < ((GamesListItem*)right)->listing->FR);
            break;
        case GC_OBSERVERS:
            result = (((GamesListItem*)left)->listing->observers < ((GamesListItem*)right)->listing->observers);
            break;
        default:
            break;
    }
    return result ^ reverseOrder;
}

PlayerListItemLessThan::PlayerListItemLessThan(int _column, Qt::SortOrder _order)
    : column(_column)
{
    reverseOrder = (_order == Qt::DescendingOrder);
}
bool PlayerListItemLessThan::operator()(const ListItem *left, const ListItem *right ) const
{
    bool result = true;
    switch((GameListingColumn)column)
    {
        case PC_STATUS:
            result = (((PlayerListItem*)left)->listing->info < ((PlayerListItem*)right)->listing->info);
            break;
        case PC_NAME:
            result = (QString::compare(((PlayerListItem*)left)->listing->name, ((PlayerListItem*)right)->listing->name, Qt::CaseInsensitive) < 0);
            break;
        case PC_RANK:
            result = (((PlayerListItem*)left)->listing->rank_score < ((PlayerListItem*)right)->listing->rank_score);
            break;
        case PC_PLAYING:
            result = (((PlayerListItem*)left)->listing->playing < ((PlayerListItem*)right)->listing->playing);
            break;
        case PC_OBSERVING:
            result = (((PlayerListItem*)left)->listing->observing < ((PlayerListItem*)right)->listing->observing);
            break;
        case PC_WINS:
            result = (((PlayerListItem*)left)->listing->wins < ((PlayerListItem*)right)->listing->wins);
            break;
        case PC_LOSSES:
            result = (((PlayerListItem*)left)->listing->losses < ((PlayerListItem*)right)->listing->losses);
            break;
        case PC_IDLE:
            result = (((PlayerListItem*)left)->listing->seconds_idle < ((PlayerListItem*)right)->listing->seconds_idle);
            break;
        case PC_COUNTRY:
            result = (QString::compare(((PlayerListItem*)left)->listing->country, ((PlayerListItem*)right)->listing->country, Qt::CaseInsensitive) < 0);
            break;
        case PC_MATCHPREFS:
            /* FIXME Need to add this all around */
            break;
        default:
            break;
    }
    return result ^ reverseOrder;
}

void ListModel::sort(int column, Qt::SortOrder order)
{
    sort_column = column;
    sort_order = order;
}

void PlayerListModel::sort(int column, Qt::SortOrder order)
{
    ListModel::sort(column,order);
    qStableSort(items.begin(),items.end(),PlayerListItemLessThan(column,order));
    emit dataChanged(createIndex(0, 0), createIndex(items.count() - 1, columnCount() - 1));
    /* Qt documentation says to emit "layoutChanged()" and not "dataChanged()" here,
     * but as of now this conflicts with filtering */
}

void ObserverListModel::sort(int column, Qt::SortOrder order)
{
    ListModel::sort(column,order);
    emit layoutAboutToBeChanged();
    qStableSort(items.begin(),items.end(),ObserverListItemLessThan(column,order));
    emit layoutChanged();
}

void GamesListModel::sort(int column, Qt::SortOrder order)
{
    ListModel::sort(column,order);
    emit layoutAboutToBeChanged();
    qStableSort(items.begin(),items.end(),GamesListItemLessThan(column,order));
    emit layoutChanged();
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
            removeItem(i);
			//qDebug("Removing %p %s %p %s", item->getListing()->white, item->getListing()->white_name().toLatin1().constData(),
			 //     item->getListing()->black, item->getListing()->black_name().toLatin1().constData());
			delete item;
			return;
		}
	}
	/* IGS with updates does get here */
	qDebug("Couldn't find listing to remove for game id %d", l->number);
}

GameListing * GamesListModel::getEntry(unsigned int id, const GameListing * listing)
{
    GameListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = ((GamesListItem *)(items[i]))->getListing();
        if (result->number == id)
        {
            if (listing)
            {
                *result = *listing;
                emit dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
            }
            return result;
        }
    }
    if (listing)
    {
        result = new GameListing(*listing); // Should not create a copy here? FIXME
        insertListing(result);
    } else
        result = NULL;
    return result;
}

void GamesListModel::deleteEntry(unsigned int id)
{
    GameListing * result;
    for (int i=0; i < items.count(); i++)
    {
        result = ((GamesListItem *)(items[i]))->getListing();
        if (result->number == id)
        {
            removeItem(i);
            delete result->gameData;		//used by ORO
            delete result;
            return;
        }
    }
    return;
}

void GamesListModel::clearList(void)
{
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
    emit countChanged(items.count());
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

GamesListItem::GamesListItem(GameListing * const l) : listing(l)
{
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
        qDebug("GamesListItem::data() : missing listing !!!");
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

void ListModel::removeItem(int i)
{
    emit beginRemoveRows(QModelIndex(), i, i);
    items.removeAt(i);
    emit endRemoveRows();
    emit countChanged(items.count());
}

void FilteredView::setFilter(ListFilter * l)
{
    listFilter = l;
    connect(l,SIGNAL(updated()),this,SLOT(updateFilter()));
}

void FilteredView::setModel ( ListModel * model )
{
    listFilter->setListModel(model);
    QTableView::setModel(model);
}

void FilteredView::updateFilter(void)
{
    if (!(this->model()))
        return;
    for(int i = 0; i < this->model()->rowCount(); i++)
    {
        setRowHidden(i, !(listFilter->filterAcceptsRow(i)));
    }
}

void FilteredView::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
    QTableView::dataChanged(topLeft, bottomRight);
	for(int i = topLeft.row(); i <= bottomRight.row(); i++)
	{
        setRowHidden(i, !(listFilter->filterAcceptsRow(i)));
	}
}

void FilteredView::rowsInserted ( const QModelIndex & parent, int start, int end )
{
    QTableView::rowsInserted(parent,start,end);
    for(int i = start; i <= end; i++)
    {
        setRowHidden(i, !(listFilter->filterAcceptsRow(i)));
    }
}


bool PlayerListFilter::filterAcceptsRow(int row) const
{
    if (listModel == NULL)
        return true;
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

void PlayerListFilter::setFilterOpen(bool state)
{
    if (state)
        flags |= open;
    else
        flags &= (~open);
    emit updated();
}

void PlayerListFilter::setFilterFriends(bool state)
{
    if (state)
        flags |= friends;
    else
        flags &= (~friends);
    emit updated();
}

void PlayerListFilter::setFilterFans(bool state)
{
    if (state)
        flags |= fans;
    else
        flags &= (~fans);
    emit updated();
}

void PlayerListFilter::setFilterMinRank(int rank)
{
    rankMin = rank;
    emit updated();
}

void PlayerListFilter::setFilterMaxRank(int rank)
{
    rankMax = rank;
    emit updated();
}

// FIXME: unused?
void GamesListFilter::setFilterWatch(bool state)
{
    watches = state;
    emit updated();
}

bool GamesListFilter::filterAcceptsRow(int row) const
{
    if (listModel == NULL)
        return true;
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
