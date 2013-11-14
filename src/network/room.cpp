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


#include "room.h"
#include "../listviews.h"
#include "talk.h"
#include "gamedialog.h"
#include "../defines.h"
#include "../connectionwidget.h"
#include "playergamelistings.h"
#include "boarddispatch.h"	//so we can remove observers
#include "networkconnection.h"

#include "ui_connectionwidget.h" // Contains declaration of Ui::ConnectionWidget, needed in Room::Room, FIXME

Room::Room(NetworkConnection * c)
{
    connection = c; // has to be set before the signals are connected.

    gamesListModel = new GamesListModel();
    gamesView = connectionWidget->ui->gamesView;
    connectionWidget->gamesListProxyModel->setSourceModel(gamesListModel);

    playerListModel = new PlayerListModel();
    playerView = connectionWidget->ui->playerView;
    connectionWidget->playerListProxyModel->setSourceModel(playerListModel);

    connect(playerView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playerOpenTalk(const QModelIndex &)));
    connect(playerView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopup(const QPoint &)));
    connect(gamesView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_gamesDoubleClicked(const QModelIndex &)));
    connect(gamesView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showGamesPopup(const QPoint &)));
    connect(connectionWidget->ui->refreshPlayersButton, SIGNAL(pressed()), SLOT(slot_refreshPlayers()));
    connect(connectionWidget->ui->refreshGamesButton, SIGNAL(pressed()), SLOT(slot_refreshGames()));

    playerView->blockSignals(false);
    gamesView->blockSignals(false);

    connect(connection,SIGNAL(playerListingUpdated(PlayerListing*)),playerListModel,SLOT(updateEntryByName(PlayerListing*)));
    connect(playerListModel,SIGNAL(countChanged(int)),connectionWidget,SLOT(setPlayerCountStat(int)));
    connect(gamesListModel,SIGNAL(countChanged(int)),connectionWidget,SLOT(setGameCountStat(int)));
	
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
	/* If this was a stand alone room, we'd also destroy the UI
	 * here, I just want to clear the lists */
    qDebug("Room::~Room()");
	/* blockSignals necessary in qt 4.7 otherwise setModel(0) crashes stupidly */
	playerView->blockSignals(true);
	gamesView->blockSignals(true);
    connectionWidget->playerListProxyModel->setSourceModel(NULL);
    connectionWidget->gamesListProxyModel->setSourceModel(NULL);
	delete playerListModel;
	delete gamesListModel;
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

//stats
void Room::slot_playerOpenTalk(const QModelIndex & index)
{
    QModelIndex sourceIndex = connectionWidget->playerListProxyModel->mapToSource(index);
    PlayerListing * opponent = playerListModel->playerListingFromIndex(sourceIndex);
    Talk * talk;
    /* Whenever a talk window is opened, we want stats.  This
     * means its easier to create the talk window and let it
     * always create stats, then send out stats messages
     * that generate talk windows */
    /* This is a little weird now...almost like we're just asking
     * for an update on the references */
    talk = connection->getTalk(opponent);
    //connection->sendStatsRequest(opponent);
    /* This is really only for ORO, and let's see if it works but... */
    if(talk)
        talk->updatePlayerListing();
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
	if(popup_playerlisting->info.contains("X"))
		matchAct->setEnabled(false);
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
        connection->sendStatsRequest(*popup_playerlisting);
    if (result == matchAct)
        connection->sendMatchInvite(*popup_playerlisting);
    if (result == talkAct)
        slot_playerOpenTalk(popup_item);
    if (result == removeFriendAct)
        connection->removeFriend(*popup_playerlisting);
    if (result == addFriendAct)
        connection->addFriend(*popup_playerlisting);
    if (result == removeWatchAct)
        connection->removeWatch(*popup_playerlisting);
    if (result == addWatchAct)
        connection->addWatch(*popup_playerlisting);
    if (result == blockAct)
        connection->addBlock(*popup_playerlisting);
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
        connection->sendObserveOutside(*popup_gamelisting);
    if (result == joinObserveAct)
        connection->sendObserve(*popup_gamelisting);
    menu->deleteLater();
    menu = NULL;
    return;
}

//observe
void Room::slot_gamesDoubleClicked(const QModelIndex & index)
{
    QModelIndex sourceIndex = connectionWidget->gamesListProxyModel->mapToSource(index);
    const GameListing * g = gamesListModel->gameListingFromIndex(sourceIndex);
	if(preferences.observe_outside_on_doubleclick &&
		  connection->supportsObserveOutside() && !g->isRoomOnly)
		connection->sendObserveOutside(*g);
	else
		connection->sendObserve(*g);
}

void Room::slot_refreshGames(void)
{
	/* First clear list.  There doesn't
	* seem to be a good way to update it
    * or update everything [IGS issue]. */
    gamesListModel->clearList();
	connection->sendGamesRequest();
}

void Room::slot_refreshPlayers(void)
{
    /* Also have to refresh the list of games here
     * because game listings point to player listings that
     * have to be destroyed */
    gamesListModel->clearList();
    playerListModel->clearList();
    connection->sendGamesRequest();
    connection->sendPlayersRequest();
}

void Room::recvToggle(int type, bool val)
{
    connectionWidget->slot_checkbox(type, val);
}

GameListing * Room::registerGameListing(GameListing * l)
{
    return gamesListModel->updateListing(l);
}

PlayerListing * Room::getPlayerListing(const QString & name)
{
    return playerListModel->getEntry(name); //returns 0 if name not found
}

PlayerListing * Room::getPlayerListing(const unsigned int id)
{
    return playerListModel->getEntry(id);
}

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
    if(connection->playerTrackingByID())
	{
        qDebug("Room::getPlayerListingByNotNickname() : not supported by connection");
		return NULL;
	}
	else
        return playerListModel->getPlayerFromNotNickName(notnickname);
}

/* Remember that this and getPlayerListing do return 0.
 * getEntry functions like getIfEntry because getNewEntry
 * is not defined, since we create registry entries for
 * the games/player registries by passing them */
GameListing * Room::getGameListing(unsigned int key)
{
    return gamesListModel->getEntry(key);
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
	if(!listing)
	{
		/* I'm not so sure about this.  FIXME We want BoardDispatch
		* to have an initial listing maybe but perhaps we should
		* alter BoardDispatch to be more adaptable in case there
		* is no listing rather than create a dummy one like this
		* and just hope it gets filled in later */
		listing = new GameListing();
		listing->number = key;
		registerGameListing(listing);
	}
	/* Look up game information to pass to board dispatch, if any 
	* ... actually that might be circular, let's NOT do that.
	* Except, we don't want to overwrite an existing listing */

	/*GameListing * temp_listing = new GameListing();
	temp_listing->number = key;
	listing = registerGameListing(temp_listing);
	delete temp_listing;*/
	
	return new BoardDispatch(connection, listing);
}

void Room::recvPlayerListing(PlayerListing * player)
{
	if(!player->online)
    {
		if(player->playing)
		{
			/* FIXME, something should clear this when games end */
			GameListing * g = getGameListing(player->playing);
			if(g)
			{
				if(g->black == player)
				{
					g->black = 0;
					g->_black_name = player->name;
					g->_black_rank = player->rank;
				}
				else if(g->white == player)
				{
					g->white = 0;
					g->_white_name = player->name;
					g->_white_rank = player->rank;
				}
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
                connection->closeTalk(player);
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
			connection->getAndSetFriendWatchType(*player);  //removes
	}
	PlayerListing * registered_player = 0;
	if(player->online)
		connection->getAndSetFriendWatchType(*player);
    if(connection->playerTrackingByID())
	{
		if(player->online)
            registered_player = playerListModel->updateEntryByID(player);
		else
            playerListModel->deleteEntry(player->id);
	}
	else
	{
		if(player->online)
            registered_player = playerListModel->updateEntryByName(player);
		else
            playerListModel->deleteEntry(player->name);
	}
	
	if(registered_player && player->online)
	{
		/* FIXME consider changing name of getEntry with the object?
		 * so that its more clear that it returns a new stored object
		 * based on the one passed. (i.e., looking it up if
		 * alread there)*/
		if(player->dialog_opened)
		{
            Talk * t = connection->getIfTalk(player);
			if(t)
				t->updatePlayerListing();
			else
				qDebug("dialog_opened flag set but no talk dialog");
		}
	}
}

void Room::recvGameListing(GameListing * game)
{
	unsigned int key;
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
		key = game->number;
	
	if(game->running)
	{
        game = gamesListModel->updateListing(game);
		connection->checkGameWatched(*game);
	}
	else
        gamesListModel->deleteEntry(key);
}
