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

//#define LISTVIEW_ICONS

#ifdef LISTVIEW_ICONS
QPixmap * testIcon;
QIcon * stoneWhiteIcon;
QIcon * stoneBlackIcon;
#endif //LISTVIEW_ICONS

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

				
ListModel::ListModel(int columns) : QStandardItemModel(0, columns)
{
#ifdef LISTVIEW_ICONS
	QPixmap * pix;
	testIcon = new QPixmap(":/new/prefix1/board/ressources/pics/coord.png");
	if(testIcon->isNull())
		exit(0);
	pix = new QPixmap(":/new/prefix1/board/ressources/pics/stone_white.png");
	if(pix->isNull())
		{ qDebug("Can't load list view icons\n"); return; }
	stoneWhiteIcon = new QIcon(pix->scaled(20, 20,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation));
	delete pix;
	pix = new QPixmap(":/new/prefix1/board/ressources/pics/stone_black.png");
	if(pix->isNull())
		{ qDebug("Can't load list view icons\n"); return; }
	stoneBlackIcon = new QIcon(pix->scaled(20, 20,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation));
	delete pix;
	//we need stonewhite and stone black for game victory flags
	//and then a board for inplay and a white board for review or study
	//an eye of some kind would be nice for a player 
	//or room looking for a game
	//and a fancy X would be nice for a player not open for a game
#endif //LISTVIEW_ICONS
}

ListModel::~ListModel()
{
#ifdef LISTVIEW_ICONS
	delete stoneWhiteIcon;
	delete stoneBlackIcon;
#endif //LISTVIEW_ICONS
}

Qt::ItemFlags ListModel::flags(const QModelIndex & index) const
{
	if(!index.isValid())
		return Qt::ItemIsEnabled;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ListModel::removeListing(const PlayerGameListingPtr l)
{
	for(int i = 0; i < rowCount(); i++)
	{
		if(data(index(i, 0), ListingDataRole).toULongLong() == l)
		{
			//setData(index(i, 0), (PlayerGameListingPtr)0, ListingDataRole);
			removeRow(i);
			return;
		}
	}
}

void ListModel::clearList(void)
{
	if(!rowCount())
		return;
	removeRows(0, rowCount());
}

PlayerGameListingPtr ListModel::listingFromIndex(const QModelIndex & index)
{
	return data(index, ListingDataRole).toULongLong();
}

int ListModel::getInsertPos(const PlayerGameListingPtr l)
{
	int row;
	int mod;
	bool cont = true;
	PlayerGameListingPtr r;
	if(!rowCount())
		return 0;
	if(getSortOrder() == Qt::AscendingOrder)
	{
		row = 0;
		mod = 1;
	}
	else
	{
		row = rowCount() - 1;
		mod = -1;
	}
	while(cont && row > -1 && row < rowCount())
	{
		r = data(index(row, 0), ListingDataRole).toULongLong();	//FIXME
		row += mod;
		if(!r)
			continue;
		cont = !lessThan(l, r, getSortColumn());
	}
	row -= mod;
	return row;
}

ObserverListModel::ObserverListModel() : ListModel(O_TOTALCOLUMNS)
{
	setHeaderData(OC_NAME, Qt::Horizontal, tr("Name"));
	setHeaderData(OC_RANK, Qt::Horizontal, tr("Rank"));
			
	sortProxy = new ObserverSortProxy();
	sortProxy->setSourceModel(this);
	sortProxy->setDynamicSortFilter(true);
}

ObserverListModel::~ObserverListModel()
{
	delete sortProxy;
}

void ObserverListModel::insertListing(PlayerListing * const l)
{
	for(int i = 0; i < rowCount(); i++)
	{
		if(data(index(i, 0), ListingDataRole).toULongLong() == (PlayerGameListingPtr)l)
			return;
	}
	if(l->hidden)	//note: cannot become unhidden!!
		return;
	int i = getInsertPos((PlayerGameListingPtr)l);
	insertRow(i);
	setData(index(i, OC_NAME), QVariant(l->name));
	setData(index(i, OC_RANK), QVariant(l->rank));
	if(l->name == account_name)
	{
		for(int c = 0; c < columnCount(); c++)
			setData(index(i, c), Qt::blue, Qt::ForegroundRole);
	}
	setData(index(i, 0), (PlayerGameListingPtr)l, ListingDataRole);
}

void ObserverListModel::removeListing(PlayerListing * const l)
{
	ListModel::removeListing((PlayerGameListingPtr)l);
}

PlayerListing * ObserverListModel::playerListingFromIndex(const QModelIndex & index)
{
	if(index.column() != 0)
		return (PlayerListing *)ListModel::listingFromIndex(ListModel::index(index.row(), 0));
	else
		return (PlayerListing *)ListModel::listingFromIndex(index);
}

bool ObserverListModel::lessThan(const PlayerGameListingPtr l, const PlayerGameListingPtr r, int column) const
{
	const PlayerListing * const leftlisting = (const PlayerListing * const)(l);
	const PlayerListing * const rightlisting = (const PlayerListing * const)(r);
	switch((ObserverListingColumn)column)
	{
		case OC_NAME:
			{
			int c = leftlisting->name.toLower().localeAwareCompare(rightlisting->name.toLower());
			if(c < 0)
				return true;
			}
			break;
		case OC_RANK:
			if(leftlisting->rank_score < rightlisting->rank_score)
				return true;
			break;
		default:
			break;
	}
	return false;
}

Qt::SortOrder ObserverListModel::getSortOrder(void) const { return sortProxy->sortOrder(); };
int ObserverListModel::getSortColumn(void) const { return sortProxy->sortColumn(); };

PlayerListModel::PlayerListModel() : ListModel(P_TOTALCOLUMNS)
{
	setHeaderData(PC_STATUS, Qt::Horizontal, tr("Stat"));
	setHeaderData(PC_NAME, Qt::Horizontal, tr("Name"));
	setHeaderData(PC_RANK, Qt::Horizontal, tr("Rank"));
	setHeaderData(PC_PLAYING, Qt::Horizontal, tr("Play"));
	setHeaderData(PC_OBSERVING, Qt::Horizontal, tr("Obs"));
	setHeaderData(PC_WINS, Qt::Horizontal, tr("Won"));
	setHeaderData(PC_LOSSES, Qt::Horizontal, tr("Lost"));
	setHeaderData(PC_IDLE, Qt::Horizontal, tr("Idle"));
	setHeaderData(PC_COUNTRY, Qt::Horizontal, tr("Country"));
	setHeaderData(PC_MATCHPREFS, Qt::Horizontal, tr("Match Prefs"));
}

PlayerListModel::~PlayerListModel()
{
}

void PlayerListModel::insertListing(PlayerListing * const l)
{
	if(l->hidden)	//note: cannot become unhidden!!
		return;
	int i = getInsertPos((PlayerGameListingPtr)l);
	insertRow(i);
#ifdef LISTVIEW_ICONS
	if(l->info.contains("X"))
		setData(index(i, PC_STATUS), 
			QIcon(testIcon->scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
	else
		setData(index(i, PC_STATUS), QVariant());
#else
	setData(index(i, PC_STATUS), QVariant(l->info));
#endif //LISTVIEW_ICONS
	setData(index(i, PC_NAME), QVariant(l->name));
	setData(index(i, PC_RANK), QVariant(l->rank));
	if(l->playing == 0)
		setData(index(i, PC_PLAYING), QVariant("-"));
	else
		setData(index(i, PC_PLAYING), QVariant(l->playing));
	if(l->observing == 0 ||
		l->observing == l->playing)
		setData(index(i, PC_OBSERVING), QVariant("-"));
	else
		setData(index(i, PC_OBSERVING), QVariant(l->observing));
	setData(index(i, PC_WINS), QVariant(l->wins));
	setData(index(i, PC_LOSSES), QVariant(l->losses));
	setData(index(i, PC_IDLE), QVariant(l->idletime));
	setData(index(i, PC_COUNTRY), QVariant(l->country));
	setData(index(i, PC_MATCHPREFS), QVariant(l->nmatch_settings));
	setData(index(i, PC_NOTIFY), QVariant(l->notify));
	setData(index(i, 0), (PlayerGameListingPtr)l, ListingDataRole);
	if(l->name == account_name)
	{
		for(int c = 0; c < columnCount(); c++)
			setData(index(i, c), Qt::blue, Qt::ForegroundRole);
	}
}

void PlayerListModel::updateListing(PlayerListing * const l)
{
	for(int i = 0; i < rowCount(); i++)
	{
		if(data(index(i, 0), ListingDataRole).toULongLong() == (PlayerGameListingPtr)l)
		{
#ifdef LISTVIEW_ICONS
			if(l->info.contains("X"))
				setData(index(i, PC_STATUS), 
					QIcon(testIcon->scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
			else
				setData(index(i, PC_STATUS), QVariant(), Qt::DecorationRole);
#else
			setData(index(i, PC_STATUS), QVariant(l->info));
#endif //LISTVIEW_ICONS
			setData(index(i, PC_NAME), QVariant(l->name));
			setData(index(i, PC_RANK), QVariant(l->rank));
			if(l->playing == 0)
				setData(index(i, PC_PLAYING), QVariant("-"));
			else
				setData(index(i, PC_PLAYING), QVariant(l->playing));
			if(l->observing == 0 ||
				l->observing == l->playing)
				setData(index(i, PC_OBSERVING), QVariant("-"));
			else
				setData(index(i, PC_OBSERVING), QVariant(l->observing));
			setData(index(i, PC_WINS), QVariant(l->wins));
			setData(index(i, PC_LOSSES), QVariant(l->losses));
			setData(index(i, PC_IDLE), QVariant(l->idletime));
			setData(index(i, PC_COUNTRY), QVariant(l->country));
			setData(index(i, PC_MATCHPREFS), QVariant(l->nmatch_settings));
			setData(index(i, PC_NOTIFY), QVariant(l->notify));
			return;
		}
	}
	insertListing(l);
}

void PlayerListModel::removeListing(PlayerListing * const l)
{
	ListModel::removeListing((PlayerGameListingPtr)l);
}

#ifdef FIXME
QVariant PlayerListModel::data(const QModelIndex & index, int role) const
{
	if(!index.isValid())
		return QVariant();

	PlayerListItem * item = static_cast<PlayerListItem*>(index.internalPointer());
	//PlayerListItem * item = playerListItemFromIndex(index);
#ifdef LISTVIEW_ICONS
	if(index.column() == PC_STATUS)
	{
		if(role == Qt::DisplayRole)
			return QVariant();
		else if(role == Qt::DecorationRole)
			return item->data(index.column());
	}
#endif //LISTVIEW_ICONS
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
#endif //FIXME

PlayerListing * PlayerListModel::playerListingFromIndex(const QModelIndex & index)
{
	if(index.column() != 0)
		return (PlayerListing *)ListModel::listingFromIndex(ListModel::index(index.row(), 0));
	else
		return (PlayerListing *)ListModel::listingFromIndex(index);
}

bool PlayerListModel::lessThan(const PlayerGameListingPtr l, const PlayerGameListingPtr r, int column) const
{
	const PlayerListing * const leftlisting = (const PlayerListing * const)(l);
	const PlayerListing * const rightlisting = (const PlayerListing * const)(r);
	switch((PlayerListingColumn)column)
	{
		case PC_STATUS:
			if(leftlisting->info < rightlisting->info)
				return true;
			break;
		case PC_NAME:
			{
			int c = leftlisting->name.toLower().localeAwareCompare(rightlisting->name.toLower());
			if(c < 0)
				return true;
			}
			break;
		case PC_RANK:
			if(leftlisting->rank_score < rightlisting->rank_score)
				return true;
			break;
		case PC_PLAYING:
			if(leftlisting->playing < rightlisting->playing)
				return true;
			break;
		case PC_OBSERVING:
			if(leftlisting->observing < rightlisting->observing)
				return true;
			break;
		case PC_WINS:
			if(leftlisting->wins < rightlisting->wins)
				return true;
			break;
		case PC_LOSSES:
			if(leftlisting->losses < rightlisting->losses)
				return true;
			break;
		case PC_IDLE:
			if(leftlisting->seconds_idle < rightlisting->seconds_idle)
				return true;
			break;
		case PC_COUNTRY:
			{
			int c = leftlisting->country.toLower().localeAwareCompare(rightlisting->country.toLower());
			if(c < 0)
				return true;
			}
			break;
		case PC_MATCHPREFS:
			// FIXME Need to add this all around
			break;
		default:
			break;
	}
	return false;
}

Qt::SortOrder PlayerListModel::getSortOrder(void) const { return sortProxy->sortOrder(); };
int PlayerListModel::getSortColumn(void) const { return sortProxy->sortColumn(); };

SimplePlayerListModel::SimplePlayerListModel(bool _notify_column) : ListModel(2)
{
	setHeaderData(SPC_NAME, Qt::Horizontal, tr("Name"));
	setHeaderData(SPC_NOTIFY, Qt::Horizontal, tr("Notify"));
	
	sortProxy = new SimplePlayerSortProxy();
	sortProxy->setSourceModel(this);
	sortProxy->setDynamicSortFilter(true);
}

SimplePlayerListModel::~SimplePlayerListModel()
{
	delete sortProxy;
}

void SimplePlayerListModel::insertListing(PlayerListing * const l)
{
	int i = getInsertPos((PlayerGameListingPtr)l);
	insertRow(i);
	setData(index(i, SPC_NAME), QVariant(l->name));
	setData(index(i, SPC_NOTIFY), QVariant(l->rank));
}

void SimplePlayerListModel::removeListing(PlayerListing * const l)
{
	ListModel::removeListing((PlayerGameListingPtr)l);
}

PlayerListing * SimplePlayerListModel::playerListingFromIndex(const QModelIndex & index)
{
	if(index.column() != 0)
		return (PlayerListing *)ListModel::listingFromIndex(ListModel::index(index.row(), 0));
	else
		return (PlayerListing *)ListModel::listingFromIndex(index);
}

bool SimplePlayerListModel::lessThan(const PlayerGameListingPtr l, const PlayerGameListingPtr r, int column) const
{
	const PlayerListing * const leftlisting = (const PlayerListing * const)(l);
	const PlayerListing * const rightlisting = (const PlayerListing * const)(r);
	switch((SimplePlayerListingColumn)column)
	{
		case SPC_NAME:
			{
			int c = leftlisting->name.toLower().localeAwareCompare(rightlisting->name.toLower());
			if(c < 0)
				return true;
			}
			break;
		default:
			break;
	}
	return false;
}

Qt::SortOrder SimplePlayerListModel::getSortOrder(void) const { return sortProxy->sortOrder(); };
int SimplePlayerListModel::getSortColumn(void) const { return sortProxy->sortColumn(); };

GamesListModel::GamesListModel() : ListModel(G_TOTALCOLUMNS)
{
	setHeaderData(GC_ID, Qt::Horizontal, tr("Id"));
	setHeaderData(GC_WHITENAME, Qt::Horizontal, tr("White"));
	setHeaderData(GC_WHITERANK, Qt::Horizontal, tr("WR"));
	setHeaderData(GC_BLACKNAME, Qt::Horizontal, tr("Black"));
	setHeaderData(GC_BLACKRANK, Qt::Horizontal, tr("BR"));
	setHeaderData(GC_MOVES, Qt::Horizontal, tr("Mv"));
	setHeaderData(GC_SIZE, Qt::Horizontal, tr("Sz"));
	setHeaderData(GC_HANDICAP, Qt::Horizontal, tr("H"));
	setHeaderData(GC_KOMI, Qt::Horizontal, tr("K"));
	setHeaderData(GC_BYOMI, Qt::Horizontal, tr("By"));
	setHeaderData(GC_FLAGS, Qt::Horizontal, tr("Flags"));
	setHeaderData(GC_OBSERVERS, Qt::Horizontal, tr("Obs"));
}

GamesListModel::~GamesListModel()
{
}

void GamesListModel::insertListing(GameListing * const l)
{
	int i = getInsertPos((PlayerGameListingPtr)l);
	insertRow(i);
	setData(index(i, GC_ID), QVariant(l->number));
	setData(index(i, GC_WHITENAME), QVariant(l->white_name()));
	setData(index(i, GC_WHITERANK), QVariant(l->white_rank()));
	setData(index(i, GC_BLACKNAME), QVariant(l->black_name()));
	setData(index(i, GC_BLACKRANK), QVariant(l->black_rank()));
	if(l->moves == (unsigned)-1)
		setData(index(i, GC_MOVES), QVariant("-"));
	else
		setData(index(i, GC_MOVES), QVariant(l->moves));
	setData(index(i, GC_SIZE), QVariant(l->board_size));
	setData(index(i, GC_HANDICAP), QVariant(l->handicap));
	setData(index(i, GC_KOMI), QVariant(l->komi));
	setData(index(i, GC_BYOMI), QVariant(l->By));
#ifdef LISTVIEW_ICONS
	switch(l->flags)
	{
		case GameListing::Flags::BLACK_WON:
			setData(index(i, GC_FLAGS), 
				QIcon(blackStoneIcon->scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
			break;
		case GameListing::Flags::WHITE_WON:
			setData(index(i, GC_FLAGS), 
				QIcon(whiteStoneIcon->scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
			break;
		case GameListing::Flags::IN_PROGRESS:
			setData(index(i, GC_FLAGS), QVariant(), Qt::DecorationRole);
			break;
		case GameListing::Flags::LOOKING:
			setData(index(i, GC_FLAGS), QVariant(), Qt::DecorationRole);
			break;
		default:
			setData(index(i, GC_FLAGS), QVariant(), Qt::DecorationRole);
			break;
	}
#else
	setData(index(i, GC_FLAGS), QVariant(l->FR));		//FIXME l->flags ??
#endif //LISTVIEW_ICONS
	setData(index(i, GC_OBSERVERS), QVariant(l->observers));
	setData(index(i, 0), (PlayerGameListingPtr)l, ListingDataRole);
}

void GamesListModel::updateListing(GameListing * const l)
{
	for(int i = 0; i < rowCount(); i++)
	{
		if(data(index(i, 0), ListingDataRole).toULongLong() == (PlayerGameListingPtr)l)
		{
			setData(index(i, GC_ID), QVariant(l->number));
			setData(index(i, GC_WHITENAME), QVariant(l->white_name()));
			setData(index(i, GC_WHITERANK), QVariant(l->white_rank()));
			setData(index(i, GC_BLACKNAME), QVariant(l->black_name()));
			setData(index(i, GC_BLACKRANK), QVariant(l->black_rank()));
			if(l->moves == (unsigned)-1)
				setData(index(i, GC_MOVES), QVariant("-"));
			else
				setData(index(i, GC_MOVES), QVariant(l->moves));
			setData(index(i, GC_SIZE), QVariant(l->board_size));
			setData(index(i, GC_HANDICAP), QVariant(l->handicap));
			setData(index(i, GC_KOMI), QVariant(l->komi));
			setData(index(i, GC_BYOMI), QVariant(l->By));
#ifdef LISTVIEW_ICONS
			switch(l->flags)
			{
				case GameListing::Flags::BLACK_WON:
					setData(index(i, GC_FLAGS), 
						QIcon(blackStoneIcon->scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
					break;
				case GameListing::Flags::WHITE_WON:
					setData(index(i, GC_FLAGS), 
						QIcon(whiteStoneIcon->scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
					break;
				case GameListing::Flags::IN_PROGRESS:
					setData(index(i, GC_FLAGS), QVariant(), Qt::DecorationRole);
					break;
				case GameListing::Flags::LOOKING:
					setData(index(i, GC_FLAGS), QVariant(), Qt::DecorationRole);
					break;
				default:
					setData(index(i, GC_FLAGS), QVariant(), Qt::DecorationRole);
					break;
			}
#else
			setData(index(i, GC_FLAGS), QVariant(l->FR));		//FIXME l->flags ??
#endif //LISTVIEW_ICONS
			setData(index(i, GC_OBSERVERS), QVariant(l->observers));
			return;
		}
	}
	insertListing(l);
}

void GamesListModel::removeListing(GameListing * const l)
{
	ListModel::removeListing((PlayerGameListingPtr)l);
}

GameListing * GamesListModel::gameListingFromIndex(const QModelIndex & index)
{
	if(index.column() != 0)
		return (GameListing *)ListModel::listingFromIndex(ListModel::index(index.row(), 0));
	else
		return (GameListing *)ListModel::listingFromIndex(index);
}

bool GamesListModel::lessThan(const PlayerGameListingPtr l, const PlayerGameListingPtr r, int column) const
{
	const GameListing * const leftlisting = (const GameListing * const)(l);
	const GameListing * const rightlisting = (const GameListing * const)(r);
	switch((GameListingColumn)column)
	{
		case GC_ID:
			if(leftlisting->number < rightlisting->number)
				return true;
			break;
		case GC_WHITENAME:
			{
			int c = leftlisting->white_name().toLower().localeAwareCompare(rightlisting->white_name().toLower());
			if(c < 0)
				return true;
			}
			break;
		case GC_WHITERANK:
			if(leftlisting->white_rank_score() < rightlisting->white_rank_score())
				return true;
			break;
		case GC_BLACKNAME:
			{
			int c = leftlisting->black_name().toLower().localeAwareCompare(rightlisting->black_name().toLower());
			if(c < 0)
				return true;
			}
			break;
		case GC_BLACKRANK:
			if(leftlisting->black_rank_score() < rightlisting->black_rank_score())
				return true;
			break;
		case GC_MOVES:
			if(leftlisting->moves < rightlisting->moves)
				return true;
			break;
		case GC_SIZE:
			if(leftlisting->board_size < rightlisting->board_size)
				return true;
			break;
		case GC_HANDICAP:
			if(leftlisting->handicap < rightlisting->handicap)
				return true;
			break;
		case GC_KOMI:
			if(leftlisting->komi < rightlisting->komi)
				return true;
			break;
		case GC_BYOMI:
			if(leftlisting->By < rightlisting->By)
				return true;
			break;
		case GC_FLAGS:
			if(leftlisting->FR < rightlisting->FR)
				return true;
			break;
		case GC_OBSERVERS: 
			if(leftlisting->observers < rightlisting->observers)
				return true;
			break;
		default:
			break;
	}
	return false;
}

Qt::SortOrder GamesListModel::getSortOrder(void) const { return sortProxy->sortOrder(); };
int GamesListModel::getSortColumn(void) const { return sortProxy->sortColumn(); };

bool ObserverSortProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(!left.isValid())
        return true;
	if(!right.isValid())
        return false;
	//QModelIndex leftzero = sourceModel()->index(left.row(), 0);
	//QModelIndex rightzero = sourceModel()->index(right.row(), 0);
	const PlayerGameListingPtr leftlisting = (PlayerGameListingPtr)dynamic_cast<ObserverListModel *>(sourceModel())->playerListingFromIndex(left);
    const PlayerGameListingPtr rightlisting = (PlayerGameListingPtr)dynamic_cast<ObserverListModel *>(sourceModel())->playerListingFromIndex(right);
	if(!leftlisting)
		return true;
	if(!rightlisting)
		return false;
	return dynamic_cast<ObserverListModel *>(sourceModel())->lessThan(leftlisting, rightlisting, right.column());
}

void PlayerSortProxy::setFilter(int rn, int rm)
{
	rankMin = rn;
	rankMax = rm;
	qDebug("player sort: %d - %d", rn, rm);
	invalidateFilter();
}

void PlayerSortProxy::setFilter(enum PlayerSortFlags f)
{
	flags ^= f;
	invalidateFilter();
}

bool PlayerSortProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex flagIndex = sourceModel()->index(sourceRow, PC_STATUS, sourceParent);
	if(!flagIndex.isValid())
		return false;
	const PlayerListing * p = dynamic_cast<PlayerListModel *>(sourceModel())->playerListingFromIndex(flagIndex);
	if(!p)
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
		if(sourceModel()->data(flagIndex).toString().contains("X") ||
			p->playing != 0)
			return false;
	}
	
	if(p->rank_score > (unsigned int)rankMax || p->rank_score + 1 < (unsigned int)rankMin)
		return false;
	return true;
}

bool PlayerSortProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(!left.isValid())
        return true;
	if(!right.isValid())
        return false;
	//QModelIndex leftzero = sourceModel()->index(left.row(), 0);
	//QModelIndex rightzero = sourceModel()->index(right.row(), 0);
	const PlayerGameListingPtr leftlisting = (PlayerGameListingPtr)dynamic_cast<PlayerListModel *>(sourceModel())->playerListingFromIndex(left);
    const PlayerGameListingPtr rightlisting = (PlayerGameListingPtr)dynamic_cast<PlayerListModel *>(sourceModel())->playerListingFromIndex(right);
	if(!leftlisting)
		return true;
	if(!rightlisting)
		return false;
	return dynamic_cast<PlayerListModel *>(sourceModel())->lessThan(leftlisting, rightlisting, right.column());
}

bool SimplePlayerSortProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(!left.isValid())
        return true;
	if(!right.isValid())
        return false;
	//QModelIndex leftzero = sourceModel()->index(left.row(), 0);
	//QModelIndex rightzero = sourceModel()->index(right.row(), 0);
	const PlayerGameListingPtr leftlisting = (PlayerGameListingPtr)dynamic_cast<SimplePlayerListModel *>(sourceModel())->playerListingFromIndex(left);
    const PlayerGameListingPtr rightlisting = (PlayerGameListingPtr)dynamic_cast<SimplePlayerListModel *>(sourceModel())->playerListingFromIndex(right);
	if(!leftlisting)
		return true;
	if(!rightlisting)
		return false;
	return dynamic_cast<SimplePlayerListModel *>(sourceModel())->lessThan(leftlisting, rightlisting, right.column());
}

void GamesSortProxy::toggleWatches(void)
{
	watches = !watches;
	invalidateFilter();
}

bool GamesSortProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex idIndex = sourceModel()->index(sourceRow, GC_ID, sourceParent);
	if(!idIndex.isValid())
		return false;
	const GameListing * g = dynamic_cast<GamesListModel *>(sourceModel())->gameListingFromIndex(idIndex);
	if(watches)
	{
		if((g->black && g->black->friendWatchType == PlayerListing::watched) ||
			(g->white && g->white->friendWatchType == PlayerListing::watched))
			return true;
		return false;
	}
	return true;
}

bool GamesSortProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	if(!left.isValid())
        return true;
	if(!right.isValid())
        return false;
	//QModelIndex leftzero = sourceModel()->index(left.row(), 0);
	//QModelIndex rightzero = sourceModel()->index(right.row(), 0);
    const PlayerGameListingPtr leftlisting = (PlayerGameListingPtr)dynamic_cast<GamesListModel *>(sourceModel())->gameListingFromIndex(left);
	const PlayerGameListingPtr rightlisting = (PlayerGameListingPtr)dynamic_cast<GamesListModel *>(sourceModel())->gameListingFromIndex(right);
	if(!leftlisting)
		return true;
	if(!rightlisting)
		return false;
	return dynamic_cast<GamesListModel *>(sourceModel())->lessThan(leftlisting, rightlisting, right.column());
}
