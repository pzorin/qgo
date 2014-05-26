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

#include <QDebug>
#include "room.h"
#include "listviews.h"
#include "talk.h"
#include "gamedialog.h"
#include "defines.h"
#include "connectionwidget.h"
#include "playergamelistings.h"
#include "boarddispatch.h"	//so we can remove observers
#include "networkconnection.h"

#include "ui_connectionwidget.h" // Contains declaration of Ui::ConnectionWidget, needed in Room::Room, FIXME

Room::Room(NetworkConnection * c)
{
    connection = c; // has to be set before the signals are connected.
    connectionWidget->room = this;

    gamesListModel = new GamesListModel();
    gamesView = connectionWidget->ui->gamesView;
    connectionWidget->gamesListProxyModel->setSourceModel(gamesListModel);
    gamesView->setColumnWidth ( GC_ID, 40 );
    gamesView->setColumnWidth ( GC_WHITENAME, 150 );
    gamesView->setColumnWidth ( GC_WHITERANK, 40 );
    gamesView->setColumnWidth ( GC_BLACKNAME, 150 );
    gamesView->setColumnWidth ( GC_BLACKRANK, 40 );
    gamesView->setColumnWidth ( GC_MOVES, 40 );
    gamesView->setColumnWidth ( GC_SIZE, 40 );
    gamesView->setColumnWidth ( GC_HANDICAP, 40 );
    gamesView->setColumnWidth ( GC_KOMI, 40 );
    gamesView->setColumnWidth ( GC_BYOMI, 40 );
    gamesView->setColumnWidth ( GC_FLAGS, 40 );
    gamesView->setColumnWidth ( GC_OBSERVERS, 40 );


    playerListModel = new PlayerListModel();
    playerView = connectionWidget->ui->playerView;
    connectionWidget->playerListProxyModel->setSourceModel(playerListModel);
    playerView->setColumnWidth ( PC_STATUS, 40 );
    playerView->setColumnWidth ( PC_NAME, 200 );
    playerView->setColumnWidth ( PC_RANK, 40 );
    playerView->setColumnWidth ( PC_PLAYING, 40 );
    playerView->setColumnWidth ( PC_OBSERVING, 40);
    playerView->setColumnWidth ( PC_WINS, 60 );
    playerView->setColumnWidth ( PC_LOSSES, 60 );
    playerView->setColumnWidth ( PC_IDLE, 60 );
    playerView->setColumnWidth ( PC_COUNTRY, 100 );
    playerView->horizontalHeader()->setStretchLastSection(true);
    //playerView->setColumnWidth ( PC_MATCHPREFS, 200 );

    connect(playerView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playerOpenTalk(const QModelIndex &)));
    connect(playerView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopup(const QPoint &)));
    connect(gamesView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_gamesDoubleClicked(const QModelIndex &)));
    connect(gamesView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showGamesPopup(const QPoint &)));
    connect(connectionWidget->ui->refreshPlayersButton, SIGNAL(pressed()), SLOT(slot_refreshPlayers()));
    connect(connectionWidget->ui->refreshGamesButton, SIGNAL(pressed()), SLOT(slot_refreshGames()));

    playerView->blockSignals(false);
    gamesView->blockSignals(false);

    connect(connection,SIGNAL(playerListingReceived(PlayerListing*)),this,SLOT(recvPlayerListing(PlayerListing*)));
    connect(connection,SIGNAL(gameListingReceived(GameListing*)),this,SLOT(recvGameListing(GameListing*)));

	players = 0;
	games = 0;

    matchAct = new QAction(tr("Match"), 0);
    statsAct = new QAction(tr("Stats"), 0);
    talkAct = new QAction(tr("Talk"), 0);
    removeFriendAct = new QAction(tr("Remove from Friends"), 0);
    addFriendAct = new QAction(tr("Add to Friends"), 0);
    removeWatchAct = new QAction(tr("Remove from Watches"), 0);
    addWatchAct = new QAction(tr("Add to Watches"), 0);
    blockAct = new QAction(tr("Block"), 0);

    joinObserveAct = new QAction(tr("Join and Observe"), 0);
    observeOutsideAct = new QAction(tr("Observe Outside"), 0);
}

Room::~Room()
{
    qDebug("Room::~Room()");
	/* blockSignals necessary in qt 4.7 otherwise setModel(0) crashes stupidly */
	playerView->blockSignals(true);
	gamesView->blockSignals(true);
    connectionWidget->playerListProxyModel->setSourceModel(NULL);
    connectionWidget->gamesListProxyModel->setSourceModel(NULL);
    connectionWidget->room = NULL;
    while (! playerListModel->items.isEmpty())
        delete playerListModel->items.takeLast();
    while (! gamesListModel->items.isEmpty())
        delete gamesListModel->items.takeLast();
	delete playerListModel;
	delete gamesListModel;
    talkMap.clear();
}

void Room::onError(void)
{
	/* We're going to call this straight from the network dispatch
	 * until we work out any room UI issues */
	//mainwindow->onConnectionError();
}

void Room::setConnection(NetworkConnection * c)
{
	connection = c;
	playerListModel->setAccountName(connection->getUsername());
}

void Room::slot_playerOpenTalk(const QModelIndex & index)
{
    QModelIndex sourceIndex = connectionWidget->playerListProxyModel->mapToSource(index);
    PlayerListing * opponent = playerListModel->playerListingFromIndex(sourceIndex);
    openTalk(opponent);
}

void Room::slot_showPopup(const QPoint & iPoint)
{
    popup_item = playerView->indexAt(iPoint);
    if (popup_item == QModelIndex())
        return;
    popup_item = connectionWidget->playerListProxyModel->mapToSource(popup_item);

    /* If we have the listing now, we don't need to look it up
     * again later FIXME */
    popup_playerlisting = playerListModel->playerListingFromIndex(popup_item);
    if(popup_playerlisting->name == connection->getUsername())
        return;
			
    QMenu * menu = new QMenu(playerView);

    menu->addAction(statsAct);
    menu->addAction(matchAct);
    menu->addAction(talkAct);
    menu->addSeparator();
    matchAct->setEnabled(!(popup_playerlisting->info.contains("X")));
    if(popup_playerlisting->friendWatchType == PlayerListing::friended)
        menu->addAction(removeFriendAct);
    else
        menu->addAction(addFriendAct);
    if(popup_playerlisting->friendWatchType == PlayerListing::watched)
        menu->addAction(removeWatchAct);
    else
        menu->addAction(addWatchAct);
    menu->addAction(blockAct);

    QAction * result = menu->exec(playerView->mapToGlobal(iPoint));
    if (result == statsAct)
        connection->sendStatsRequest(popup_playerlisting);
    if (result == matchAct)
        connection->sendMatchInvite(popup_playerlisting);
    if (result == talkAct)
        openTalk(popup_playerlisting);
    if (result == removeFriendAct)
        connection->removeFriend(popup_playerlisting);
    if (result == addFriendAct)
        connection->addFriend(popup_playerlisting);
    if (result == removeWatchAct)
        connection->removeWatch(popup_playerlisting);
    if (result == addWatchAct)
        connection->addWatch(popup_playerlisting);
    if (result == blockAct)
        connection->addBlock(popup_playerlisting);
    menu->deleteLater();
    menu = NULL;
    return;
}

void Room::slot_showGamesPopup(const QPoint & iPoint)
{
	if(!connection)
		return;
	popup_item = gamesView->indexAt(iPoint);
    if (popup_item == QModelIndex())
        return;
    popup_item = connectionWidget->gamesListProxyModel->mapToSource(popup_item);

    popup_gamelisting = gamesListModel->gameListingFromIndex(popup_item);
    if(popup_gamelisting->isRoomOnly)
        return;
    QMenu * menu = new QMenu(gamesView);
    menu->addAction(joinObserveAct);
    if(connection->supportsObserveOutside())
        menu->addAction(observeOutsideAct);

    QAction * result = menu->exec(gamesView->mapToGlobal(iPoint));
    if (result == observeOutsideAct)
        connection->sendObserveOutside(popup_gamelisting);
    if (result == joinObserveAct)
        connection->sendObserve(popup_gamelisting);
    menu->deleteLater();
    menu = NULL;
    return;
}

void Room::openTalk(PlayerListing *listing)
{
    Talk * talk = getTalk(listing);
    //connection->sendStatsRequest(opponent);
    /* This is really only for ORO, and let's see if it works but... */
    if(talk)
        talk->updatePlayerListing();
}

//observe
void Room::slot_gamesDoubleClicked(const QModelIndex & index)
{
    QModelIndex sourceIndex = connectionWidget->gamesListProxyModel->mapToSource(index);
    const GameListing * g = gamesListModel->gameListingFromIndex(sourceIndex);
	if(preferences.observe_outside_on_doubleclick &&
		  connection->supportsObserveOutside() && !g->isRoomOnly)
        connection->sendObserveOutside(g);
	else
        connection->sendObserve(g);
}

void Room::slot_refreshGames(void)
{
	connection->sendGamesRequest();
}

void Room::slot_refreshPlayers(void)
{
    connection->sendPlayersRequest();
}

void Room::recvToggle(int type, bool val)
{
    connectionWidget->slot_checkbox(type, val);
}

PlayerListing * Room::getPlayerListing(const QString & name)
{
    //qDebug() << "Room::getPlayerListing() called with argument " << name;
    PlayerListing * result = playerListModel->getEntry(name);
    if (result == NULL)
    {
        result = new PlayerListing();
        result->online = true;
        result->name = name;
        playerListModel->insertListing(result);
    }
    return result;
}

PlayerListing * Room::getPlayerListing(const unsigned int id)
{
    PlayerListing * result = playerListModel->getEntry(id);
    if (result == NULL)
    {
        result = new PlayerListing();
        result->online = true;
        result->id = id;
        playerListModel->insertListing(result);
    }
    return result;
}

// The reason to have this function seems to date from performance issues around 2011.
/* Here's the deal with this.  Tygem has a username and a nickname.
 * the nickname is displayed and generally present, but not always.  The username
 * seems to always be present.  At first glance, it makes sense to lookup by
 * the username since that seems how the information is stored and it seems
 * unique to username.  However, if we need to display and sort by the
 * nickname, that introduces overhead in the listviews that are slow enough
 * already.  It might be minor overhead, and it might be really ugly
 * to search through all the listings to find a particular username like
 * this, but I think its better since its done rarely.  I may regret this
 * and change it later. */
 /* FIXME don't we usually say "from" instead of "by" change this */
PlayerListing * Room::getPlayerListingByNotNickname(const QString & notnickname)
{
    PlayerListing * result = NULL;
    /* It is illogical to try to look up things by username first and by "notnickname" second
     * in this function, but this apparently has some historical reason that I am unaware of. */
    if (result == NULL)
        result = playerListModel->getEntry(notnickname);
    if (result == NULL)
        playerListModel->getPlayerFromNotNickName(notnickname);
    if (result == NULL)
    {
        result = new PlayerListing();
        result->online = true;
        result->notnickname = notnickname;
        playerListModel->insertListing(result);
    }
    return result;
}

GameListing * Room::getGameListing(unsigned int key)
{
    GameListing * result = gamesListModel->getEntry(key);
    if (result == NULL)
    {
        result = new GameListing();
        result->number = key;
        gamesListModel->insertListing(result);
    }
    return result;
}

/* Called from getNewEntry in BoardDispatchRegistry in networkconnection.cpp */
class BoardDispatch * Room::getNewBoardDispatch(unsigned int key)
{
	/* Oro might pass a game code rather than a number here.  But
	* the listings sometimes only have numbers.  We need to
	* look up in game stuff like moves and results by game code.
	* we don't have the number there.  But this means always
	* passing game codes to this, but then we can't use getGameListing
	* initially.  Even if we use the number on the initial board get/create
	* and the game code thereafter, assuming there aren't collisions in
	* the id spaces, we still have to alter this to be aware of the
	* ORO protocol.  Its a fucking mess. I think we better just store
	* everything by the key and have a separate lookup, code to key */
	GameListing * listing = getGameListing(key);
    return new BoardDispatch(connection, listing);
}

void Room::recvPlayerListing(PlayerListing *player)
{
    playerListModel->updateEntry(player);

    if(!player->online)
    {
		if(player->playing)
		{
			/* FIXME, something should clear this when games end */
			GameListing * g = getGameListing(player->playing);
            if(g->black == player)
            {
                g->black = NULL;
                g->_black_name = player->name;
                g->_black_rank = player->rank;
            }
            else if(g->white == player)
            {
                g->white = NULL;
                g->_white_name = player->name;
                g->_white_rank = player->rank;
            }
		}
		if(player->observing)
		{
			/* FIXME remove this check, room_list should be sufficient */
			/* FIXME, something should clear this when games end */
			BoardDispatch * b = connection->getIfBoardDispatch(player->observing);
			if(b)
			{
				b->recvObserver(player, false);
			}
			if(player->dialog_opened)
                closeTalk(player);
		}
		std::vector<unsigned short>::iterator room_listit = player->room_list.begin();
		while(room_listit != player->room_list.end())
		{
			BoardDispatch * boarddispatch = connection->getIfBoardDispatch(*room_listit);
			if(boarddispatch)
				boarddispatch->recvObserver(player, false);
			else
				player->room_list.erase(room_listit);
			room_listit = player->room_list.begin();
		}
		if(player->friendWatchType != PlayerListing::none)
            connection->getAndSetFriendWatchType(player);  //removes
	}

    if(player->online)
        connection->getAndSetFriendWatchType(player);
	
    if(player->online && player->dialog_opened)
    {
        Talk * t = getIfTalk(player);
        if(t)
            t->updatePlayerListing();
        else
            qDebug("dialog_opened flag set but no talk dialog");
    }
}

void Room::recvGameListing(GameListing * game)
{
    gamesListModel->updateListing(game);
	/* This WAS iffy, we WERE using the other ID registry's
	 * existence to indicate an ORO connection in order
	 * to use a different key.  Ugly.  Once this works, 
	 * we need to find a way to hide this within the
	 * connection.  It shouldn't be running around here */
	/* FIXME, turns out that game_number is better for listings
	 * even if we need the code for joining, etc.  Its unique
	 * anyway, so who cares */
	//if(playerListingIDRegistry)
	//	key = game->game_code;
    //else
	
	if(game->running)
    {
        connection->checkGameWatched(game);
	}
}

Talk * Room::getTalk(PlayerListing * opponent)
{
    Talk * result = getIfTalk(opponent);
    if (result == NULL)
    {
        // Create if it does not exist
        result = new Talk(connection, opponent, this);
        /* Here's how the talk stuff works for future reference. FIXME.
         * double clicking on a name or typing a console command (maybe) opens
         * a talk dialog through the room which also, at that time, might request
         * stats from the network connection.  We also notify
         * the mainwindow to add the new dialog to the list of talk dialogs */
        connectionWidget->talkOpened(result);
        talkMap.insert(opponent, result);
    }
    return result;
}

Talk * Room::getIfTalk(const PlayerListing * opponent)
{
    QMap <const PlayerListing *, Talk *>::const_iterator i = talkMap.find(opponent);
    if(i == talkMap.end())
        return NULL;
    else
        return i.value();
}

void Room::closeTalk(const PlayerListing * opponent)
{
    QMap <const PlayerListing *, Talk *>::iterator i = talkMap.find(opponent);
    if(i == talkMap.end())
        return;

    i.value()->deleteLater();
    talkMap.erase(i);
}
