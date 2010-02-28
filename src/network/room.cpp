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
#include "roomregistries.h"
#include "talk.h"
#include "messages.h"
#include "gamedialog.h"
#include "../mainwindow.h"
#include "playergamelistings.h"
#include "boarddispatch.h"	//so we can remove observers
#include "friendslistdialog.h"

//#define PLAYERLISTING_ISSUES
Room::Room()
{
	playerView = mainwindow->getUi()->playerView;
	gamesView = mainwindow->getUi()->gamesView;
	refreshPlayersButton = mainwindow->getUi()->pbRefreshPlayers;
	refreshGamesButton = mainwindow->getUi()->pbRefreshGames;
	whoBox1 = mainwindow->getUi()->whoBox1;
	whoBox2 = mainwindow->getUi()->whoBox2;
	whoOpenCheckBox = mainwindow->getUi()->whoOpenCheck;
	friendsCheckBox = mainwindow->getUi()->friendsCheck;
	watchesCheckBox = mainwindow->getUi()->watchesCheck;
	editFriendsWatchesListButton = mainwindow->getUi()->editFriendsWatchesPB;
	/* Normally the dispatch and the UI are created at the sametime
	* by the connection or dispatch code.  In this case,
	* the UI already exists and is being passed straight in here,
	* so we'll do things backwards and have this create the dispatch */
	/* UI MUST be created before dispatch.  FIXME
	* This is an ugly non-apparent dependency because
	* the dispatch registries depend on the UI's models */
	/* There are further strangenesses here because of the account_name
	* for making our player listing blue */
	setupUI();
	
	playerListingRegistry = 0;
	playerListingIDRegistry = 0;
	
	players = 0;
	games = 0;
}

void Room::setupUI(void)
{
	gamesSortProxy = new GamesSortProxy();
	gamesListModel = new GamesListModel();
    gamesSortProxy->setSourceModel(gamesListModel);

	/*ui.ListView_games->header()->setSortIndicatorShown ( FALSE );
	ui.ListView_games->hideColumn(12);
	ui.ListView_games->hideColumn(13);*/
        //gamesView->setModel(gamesListModel);
	gamesView->setModel(gamesSortProxy);
	/* Justifications??? */
	gamesSortProxy->setDynamicSortFilter(true);
	gamesListModel->setSortProxy(gamesSortProxy);
	/* No sort indicator??? */
	/* Qt 4.4.1 made sortIndicatorShown necesssary for sort behavior
	 * !!!! doublecheck now FIXME*/
	gamesView->header()->setSortIndicatorShown ( true );

	//gamesView->setSelectionBehavior(QAbstractItemView::SelectRows);
	/*ui.gamesView->setColumnWidth ( 0, 40 );	//35
	ui.gamesView->setColumnWidth ( 1, 100 );
	ui.gamesView->setColumnWidth ( 2, 48 );	//35
	ui.gamesView->setColumnWidth ( 3, 100 );
	ui.gamesView->setColumnWidth ( 4, 48 );	//35
	ui.gamesView->setColumnWidth ( 5, 45 );	//30
	ui.gamesView->setColumnWidth ( 6, 45 ); //25
	ui.gamesView->setColumnWidth ( 7, 38 ); //20
	ui.gamesView->setColumnWidth ( 8, 35 ); //30
	ui.gamesView->setColumnWidth ( 9, 43 ); //25
	ui.gamesView->setColumnWidth ( 10, 60 ); //20
	ui.gamesView->setColumnWidth ( 11, 55 ); //25
	*/
	gamesView->setColumnWidth ( 0, 35 );	//35
	gamesView->setColumnWidth ( 1, 100 );
	gamesView->setColumnWidth ( 2, 35 );	//35
	gamesView->setColumnWidth ( 3, 100 );
	gamesView->setColumnWidth ( 4, 35 );	//35
	gamesView->setColumnWidth ( 5, 30 );	//30
	gamesView->setColumnWidth ( 6, 30 ); //25
	gamesView->setColumnWidth ( 7, 20 ); //20
	gamesView->setColumnWidth ( 8, 30 ); //30
	gamesView->setColumnWidth ( 9, 35 ); //25
	gamesView->setColumnWidth ( 10, 35 ); //20
	gamesView->setColumnWidth ( 11, 30 ); //25
	//ui.gamesView->show();
	
	playerSortProxy = new PlayerSortProxy();
	playerListModel = new PlayerListModel();
	playerSortProxy->setSourceModel(playerListModel);
	
   // playerView->setIconSize(QSize(20, 20));
	playerView->setModel(playerSortProxy);
	//playerView->setModel(playerListModel);
	playerSortProxy->setDynamicSortFilter(true);
	playerListModel->setSortProxy(playerSortProxy);
	//connect(playerListModel, SIGNAL(dataChanged(QModelIndex())), playerSortProxy, SLOT(clear()));
	//connect(playerView->header(), SIGNAL(sectionClicked(int)), playerSortProxy, SLOT(sortByColumn(int))); 
	playerView->header()->setSortIndicatorShown ( true );
	//playerView->setSortingEnabled(true);
	
	playerView->setColumnWidth ( 0, 40 );
	playerView->setColumnWidth ( 1, 100 );
	playerView->setColumnWidth ( 2, 40 );
	playerView->setColumnWidth ( 3, 40 );
	playerView->setColumnWidth ( 4, 40);
	playerView->setColumnWidth ( 5, 40 );
	playerView->setColumnWidth ( 6, 40 );
	playerView->setColumnWidth ( 7, 40 );
	playerView->setColumnWidth ( 8, 80 );
	/*ui.ListView_players->hideColumn(6);
	ui.ListView_players->hideColumn(7);
	ui.ListView_players->hideColumn(8);
	ui.ListView_players->hideColumn(9);
	ui.ListView_players->hideColumn(12);
	ui.ListView_players->setColumnWidth ( 0, 30 );
	ui.ListView_players->setColumnWidth ( 1, 100 );
	ui.ListView_players->setColumnWidth ( 2, 30 );
	ui.ListView_players->setColumnWidth ( 3, 30 );
	ui.ListView_players->setColumnWidth ( 4, 30);
	ui.ListView_players->setColumnWidth ( 5, 30 );
//	ui.ListView_players->setColumnWidth ( 6, 80 );
	ui.ListView_players->setColumnWidth ( 7, 80 );
//	ui.ListView_players->setColumnWidth ( 8, 30 );
//	ui.ListView_players->setColumnWidth ( 9, 25 );
	ui.ListView_players->setColumnWidth ( 10, 50 );
//	ui.ListView_players->setColumnWidth ( 11, 25 );*/

	whoBox1->setCurrentIndex(0);
	whoBox2->setCurrentIndex(0);
	/* Maybe also a "clicked" for something? */
	connect(playerView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playersDoubleClicked(const QModelIndex &)));
	connect(playerView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopup(const QPoint &)));
	connect(gamesView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_gamesDoubleClicked(const QModelIndex &)));
	connect(gamesView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showGamesPopup(const QPoint &)));
	connect(refreshPlayersButton, SIGNAL(pressed()), SLOT(slot_refreshPlayers()));
	connect(refreshGamesButton, SIGNAL(pressed()), SLOT(slot_refreshGames()));
	connect(whoBox1, SIGNAL(currentIndexChanged(int)), SLOT(slot_setRankSpreadView()));
	connect(whoBox2, SIGNAL(currentIndexChanged(int)), SLOT(slot_setRankSpreadView()));
	connect(whoOpenCheckBox, SIGNAL(stateChanged(int)), SLOT(slot_showOpen(int)));
	connect(friendsCheckBox, SIGNAL(stateChanged(int)), SLOT(slot_showFriends(int)));
	connect(watchesCheckBox, SIGNAL(stateChanged(int)), SLOT(slot_showWatches(int)));
	connect(editFriendsWatchesListButton, SIGNAL(pressed()), SLOT(slot_editFriendsWatchesList()));
	editFriendsWatchesListButton->setEnabled(true);
}

Room::~Room()
{
	delete playerListingRegistry;
	delete playerListingIDRegistry;
	delete gameListingRegistry;
	
	/* FIXME: Should probably be part of something else this P G stuff */
	mainwindow->statusUsers->setText(" P: 0");
	mainwindow->statusGames->setText(" G: 0");
	//mainwindow->ui.changeServerPB->hide();	//done in mainwindow_server
	//mainwindow->ui.createRoomPB->hide();
	
	/* If this was a stand alone room, we'd also destroy the UI
	 * here, I just want to clear the lists */
	playerView->setModel(0);
	gamesView->setModel(0);
	delete playerListModel;
	delete playerSortProxy;
	delete gamesListModel;
	delete gamesSortProxy;
	disconnect(whoBox1, SIGNAL(currentIndexChanged(int)), 0, 0);
	disconnect(whoBox2, SIGNAL(currentIndexChanged(int)), 0, 0);
	disconnect(editFriendsWatchesListButton, SIGNAL(pressed()), 0, 0);
	editFriendsWatchesListButton->setDisabled(true);
	
	QSettings settings;
	settings.setValue("LOWRANKFILTER", whoBox1->currentIndex());
	settings.setValue("HIGHRANKFILTER", whoBox2->currentIndex());
	whoBox1->setCurrentIndex(0);
	whoBox2->setCurrentIndex(0);
	
	settings.setValue("OPENFILTER", whoOpenCheckBox->isChecked());
	settings.setValue("FRIENDSFILTER", friendsCheckBox->isChecked());
	settings.setValue("WATCHESFILTER", watchesCheckBox->isChecked());
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
	if(connection->playerTrackingByID())
	{
		qDebug("Creating player ID Registry");
		playerListingIDRegistry = new PlayerListingIDRegistry(playerListModel);
	}
	else
	{
		qDebug("Creating player Registry");
		playerListingRegistry = new PlayerListingRegistry(playerListModel);
	}
	gameListingRegistry = new GameListingRegistry(gamesListModel);
	
	playerListModel->setAccountName(connection->getUsername());
	// FIXME, what about observerListModels when they come up on boards?
	unsigned long flags = connection->getPlayerListColumns();
	if(flags & PL_NOWINSLOSSES)
	{
		playerView->hideColumn(5);
		playerView->hideColumn(6);
	}
	if(flags & PL_NOMATCHPREFS)
	{
		playerView->hideColumn(9);	
	}
	
	QSettings settings;
	QVariant var = settings.value("LOWRANKFILTER");
	if(var != QVariant())
	{
		whoBox1->setCurrentIndex(var.toInt());
		whoBox2->setCurrentIndex(settings.value("HIGHRANKFILTER").toInt());
	}
	
	whoOpenCheckBox->setChecked(settings.value("OPENFILTER").toBool());
	friendsCheckBox->setChecked(settings.value("FRIENDSFILTER").toBool());
	watchesCheckBox->setChecked(settings.value("WATCHESFILTER").toBool());
}

void Room::updateRoomStats(void)
{
	/* With sort, it might be better to get these from the registries? 
	 * possible FIXME */
	players = playerListModel->rowCount(QModelIndex());
	games = gamesListModel->rowCount(QModelIndex());
	
	//qDebug("%d %d", players, games);
	mainwindow->statusUsers->setText(" P: " + QString::number(players));
	mainwindow->statusGames->setText(" G: " + QString::number(games));
}

//stats
void Room::slot_playersDoubleClicked(const QModelIndex & index)
{
	QModelIndex translated = playerSortProxy->mapToSource(index);
	PlayerListing * opponent = playerListModel->playerListingFromIndex(translated);
	sendStatsRequest(*opponent);
}

void Room::slot_showPopup(const QPoint & iPoint)
{
	popup_item = playerView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
		/* If we have the listing now, we don't need to look it up
		 * again later FIXME */
		QModelIndex translated = playerSortProxy->mapToSource(popup_item);
		popup_playerlisting = playerListModel->playerListingFromIndex(translated);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
    	QMenu menu(playerView);
	menu.addAction(tr("Stats"), this, SLOT(slot_popupStats()));
	QAction * matchAct = new QAction(tr("Match"), 0);
	if(popup_playerlisting->info.contains("X"))
		matchAct->setEnabled(false);
	else
		connect(matchAct, SIGNAL(triggered()), this, SLOT(slot_popupMatch()));
		
	menu.addAction(matchAct);
	
		
	menu.addAction(tr("Talk"), this, SLOT(slot_popupTalk()));
	menu.addSeparator();
    	if(popup_playerlisting->friendWatchType == PlayerListing::friended)
    		menu.addAction(tr("Remove from Friends"), this, SLOT(slot_removeFriend()));
    	else
    		menu.addAction(tr("Add to Friends"), this, SLOT(slot_addFriend()));
    	if(popup_playerlisting->friendWatchType == PlayerListing::watched)
    		menu.addAction(tr("Remove from Watches"), this, SLOT(slot_removeWatch()));
    	else
    		menu.addAction(tr("Add to Watches"), this, SLOT(slot_addWatch()));
    	menu.addAction(tr("Block"), this, SLOT(slot_addBlock()));
    	menu.exec(playerView->mapToGlobal(iPoint));
	delete matchAct;
	}
}

void Room::slot_showGamesPopup(const QPoint & iPoint)
{
	if(!connection)
		return;
	if(!connection->supportsObserveOutside())
		return;
	popup_item = gamesView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
		/* Do not give options on rooms without games */
		QModelIndex translated = gamesSortProxy->mapToSource(popup_item);
		popup_gamelisting = gamesListModel->gameListingFromIndex(translated);
		if(popup_gamelisting->isRoomOnly)
			return;
		QMenu menu(gamesView);
		if(preferences.observe_outside_on_doubleclick)
		{
			menu.addAction(tr("Observe Outside"), this, SLOT(slot_popupObserveOutside()));
			menu.addAction(tr("Join and Observe"), this, SLOT(slot_popupJoinObserve()));
		}
		else
		{
			menu.addAction(tr("Join and Observe"), this, SLOT(slot_popupJoinObserve()));
			menu.addAction(tr("Observe Outside"), this, SLOT(slot_popupObserveOutside()));
		}
		
		menu.exec(gamesView->mapToGlobal(iPoint));
	}
}

void Room::slot_addFriend(void)
{
	connection->addFriend(*popup_playerlisting);
}

/* This needs to call a resort or something on the list
 * if we have "friends" checked, same thing with blocked
 * if we don't, just to update that one entry FIXME */
void Room::slot_removeFriend(void)
{
	connection->removeFriend(*popup_playerlisting);
}

void Room::slot_addWatch(void)
{
	connection->addWatch(*popup_playerlisting);
}

void Room::slot_removeWatch(void)
{
	connection->removeWatch(*popup_playerlisting);
}

void Room::slot_addBlock(void)
{
	connection->addBlock(*popup_playerlisting);
}

void Room::slot_popupStats(void)
{
	/* For now, just request stats.  Later, we might open a separate window with a tab for
	 * game records */
	connection->sendStatsRequest(*popup_playerlisting);
}

void Room::slot_popupMatch(void)
{
	connection->sendMatchInvite(*popup_playerlisting);
}

void Room::slot_popupTalk(void)
{
	slot_playersDoubleClicked(popup_item);
}

void Room::slot_popupObserveOutside(void)
{
	connection->sendObserveOutside(*popup_gamelisting);
}

void Room::slot_popupJoinObserve(void)
{
	connection->sendObserve(*popup_gamelisting);
}

//observe
void Room::slot_gamesDoubleClicked(const QModelIndex & index)
{
	QModelIndex translated = gamesSortProxy->mapToSource(index);
	const GameListing * g = gamesListModel->gameListingFromIndex(translated);
	if(preferences.observe_outside_on_doubleclick &&
		  connection->supportsObserveOutside() && !g->isRoomOnly)
		connection->sendObserveOutside(*g);
	else
		connection->sendObserve(*g);
}

void Room::slot_refreshPlayers(void)
{
	//we don't have to clear the players list
	//if we're checking for existing player records
	//clearPlayerList();
	connection->sendPlayersRequest();
}

void Room::clearPlayerList(void)
{
	playerListModel->clearList();
	if(connection->playerTrackingByID())
	{
		qDebug("Creating player ID Registry");
		delete playerListingIDRegistry;
		playerListingIDRegistry = new PlayerListingIDRegistry(playerListModel);
	}
	else
	{
		qDebug("Creating player Registry");
		delete playerListingRegistry;
		playerListingRegistry = new PlayerListingRegistry(playerListModel);
	}
}

void Room::clearGamesList(void)
{
	gamesListModel->clearList();
	delete gameListingRegistry;
	gameListingRegistry = new GameListingRegistry(gamesListModel);
}

void Room::slot_refreshGames(void)
{
	/* First clear list.  There doesn't
	* seem to be a good way to update it
	* or update everything [IGS issue].  Since we're
	* getting all info again anyway... 
	* same with players and perhaps 
	* observers as well. */
	clearGamesList();
	connection->sendGamesRequest();
}

/* This code was taken from mainwindow_server.cpp
 * I guess it figures out which is min and which
 * is max?? */
void Room::slot_setRankSpreadView(void)
{
	QString rkMin, rkMax;
	QString r1,r2;

	if ((whoBox1->currentIndex() == 0) && (whoBox2->currentIndex() == 0))
	{
		rkMin = "NR";
		rkMax = "9p";
	}
	else if ( ((whoBox1->currentIndex() == 0) && (whoBox2->currentIndex() == 1)) ||
		((whoBox1->currentIndex() == 1) && (whoBox2->currentIndex() == 0))  ||
		((whoBox1->currentIndex() == 1) && (whoBox2->currentIndex() ==1)) )
	{
		rkMin = "1p";
		rkMax = "9p";
	}
	else if ((whoBox1->currentIndex() == 0) && (whoBox2->currentIndex() > 1))
	{
		rkMin = whoBox2->currentText();
		rkMax = whoBox2->currentText();
	}
	else if ((whoBox1->currentIndex() > 1) && (whoBox2->currentIndex() == 0))
	{
		rkMin = whoBox1->currentText();
		rkMax = whoBox1->currentText();
	}
	else if ((whoBox1->currentIndex() > 1) && (whoBox2->currentIndex() == 1))
	{
		rkMin = whoBox1->currentText();
		rkMax = "9p";
	}
	else if ((whoBox1->currentIndex() == 1) && (whoBox2->currentIndex() > 1))
	{
		rkMin = whoBox2->currentText();
		rkMax = "9p";
	}
	else if ((whoBox2->currentIndex() >= whoBox1->currentIndex() ))
	{
		rkMin = whoBox2->currentText();
		rkMax = whoBox1->currentText();
	} 
	else 
	{
		rkMin = whoBox1->currentText();
		rkMax = whoBox2->currentText();
	} 

	playerSortProxy->setFilter(connection->rankToScore(rkMin), connection->rankToScore(rkMax));
	qDebug( "rank spread : %s - %s" , rkMin.toLatin1().constData() , rkMax.toLatin1().constData());
}

void Room::slot_showOpen(int)
{
	playerSortProxy->setFilter(PlayerSortProxy::open);
}

void Room::slot_showFriends(int)
{
	playerSortProxy->setFilter(PlayerSortProxy::friends);
}

void Room::slot_showWatches(int)
{
	playerSortProxy->setFilter(PlayerSortProxy::fans);
	gamesSortProxy->toggleWatches();
}

void Room::slot_editFriendsWatchesList(void)
{
	//does this crash if we do it too soon?
	FriendsListDialog * fld = new FriendsListDialog(connection);
	fld->exec();
	delete fld;		//okay?
}

void Room::talkOpened(Talk * d)
{
	mainwindow->talkOpened(d);
}

void Room::recvToggle(int type, bool val)
{
	mainwindow->slot_checkbox(type, val);
}

GameListing * Room::registerGameListing(GameListing * l)
{
	return gameListingRegistry->getEntry(l->number, l);
}

/* This appears to be unused, recvGameListing does stuff
 * with "->running" FIXME */
void Room::unRegisterGameListing(unsigned int key)
{
	gameListingRegistry->deleteEntry(key);
}

PlayerListing * Room::getPlayerListing(const QString & name)
{
	PlayerListing * p;
	if(!playerListingRegistry)
	{
		if(playerListingIDRegistry)
			p = playerListingIDRegistry->getPlayerFromName(name);
		else
		{
			qDebug("No player listing registry!");
			return NULL;
		}
	}
	else
		p = playerListingRegistry->getEntry(name);
#ifdef FIXME
	/* this is tricky FIXME */
	if(!p)
		connection->sendStatsRequest(name);		
#endif //FIXME
	/* Should probably return & FIXME */
	return p;
}

PlayerListing * Room::getPlayerListing(const unsigned int id)
{
	if(!playerListingIDRegistry)	//FIXME
	{
		qDebug("No player listing registry!");
		return NULL;
	}
	PlayerListing * p = playerListingIDRegistry->getEntry(id);
	// FIXME
	/*if(!p)
		connection->sendStatsRequest(id);*/	
#ifdef PLAYERLISTING_ISSUES	
	if(!p)
		qDebug("Can't get player listing for %d", id);
#endif //PLAYERLISTING_ISSUES
	return p;
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
	PlayerListing * p;
	if(!playerListingRegistry)
	{
		qDebug("No player listing registry!");
		return NULL;
	}
	else
		p = playerListingRegistry->getPlayerFromNotNickName(notnickname);

	/* Should probably return & FIXME */
	return p;	
}

/* Remember that this and getPlayerListing do return 0.
 * getEntry functions like getIfEntry because getNewEntry
 * is not defined, since we create registry entries for
 * the games/player registries by passing them */
GameListing * Room::getGameListing(unsigned int key)
{
	GameListing * l = gameListingRegistry->getEntry(key);
	/* I'm not sure we'd ever want to automatically
	 * requestGameStats.  Either the listing is on the way
	 * or we request it from some other place.  The main
	 * thing is we don't want to be requesting it from
	 * the games listing messages every time we try to
	 * get an existing listing.  But if its a problem
	 * to just comment out the below two lines, then
	 * we'll need a getIfGameListing as with the 
	 * getIfBoardDispatch FIXME */
	//if(!l)
	//	connection->requestGameStats(key);
	return l;
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

std::map<PlayerListing *, unsigned short> removed_player;
void Room::recvPlayerListing(PlayerListing * player)
{
	if(!player->online)
	{
		removed_player[player] = player->playing;
#ifdef PLAYERLISTING_ISSUES
		qDebug("Removing player %s %p on attached game %d", player->name.toLatin1().constData(), player, player->playing);
#endif //PLAYERLISTING_ISSUES
		/* To prevent crashes due to GameListing link to PlayerListing
		 * FIXME */
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
#ifdef PLAYERLISTING_ISSUES
				qDebug("Removing player %s on observing game %d", player->name.toLatin1().constData(), player->observing);
#endif //PLAYERLISTING_ISSUES
				b->recvObserver(player, false);
			}
			if(player->dialog_opened)
				connection->closeTalk(*player);
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
	if(playerListingIDRegistry)
	{
		if(player->online)
			registered_player = playerListingIDRegistry->getEntry(player->id, player);
		else
			playerListingIDRegistry->deleteEntry(player->id);	
	}
	else
	{
		if(player->online)
			registered_player = playerListingRegistry->getEntry(player->name, player);
		else
			playerListingRegistry->deleteEntry(player->name);
	}
	
	if(registered_player && player->online)
	{
		/* FIXME consider changing name of getEntry with the object?
		 * so that its more clear that it returns a new stored object
		 * based on the one passed. (i.e., looking it up if
		 * alread there)*/
		if(player->dialog_opened)
		{
			Talk * t = connection->getIfTalk(*player);
			if(t)
				t->updatePlayerListing();
			else
				qDebug("dialog_opened flag set but no talk dialog");
		}
	}
	
	/* This is just for those listing bugs... its weird...remove it
	 * soon */
	if(registered_player && player->online)
	{
		std::map<PlayerListing *, unsigned short>::iterator it;
		it = removed_player.find(registered_player);
		if(it != removed_player.end())
			removed_player.erase(it);
	}
	updateRoomStats();	//in case we lost one or added one
}

/* These two may want to check that they are the mainwindowroom or the
 * default room or whatever.  But its not necessary to have a whole
 * other subclass for mainwindowrooms because they both need some
 * kind of ui.  It would be more effective to do something with the
 * ui classes */
void Room::recvExtPlayerListing(class PlayerListing * player)
{
	mainwindow->slot_statsPlayer(player);
}

void Room::updatePlayerListing(class PlayerListing & player)
{
	playerListModel->updateListing(&player);
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
		game = gameListingRegistry->getEntry(key, game);
		connection->checkGameWatched(*game);
	}
	else
		gameListingRegistry->deleteEntry(key);
	updateRoomStats();	//in case we lost one or added one
}

/* This isn't really just a "sendStats" function anymore, that's
 * done by the talk object I think and... well its ugly, responsibilities
 * are crossed, etc.. FIXME */
void Room::sendStatsRequest(PlayerListing & opponent)
{
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

/*** GameListing/PlayerListing Registry functions ***/

void GameListingRegistry::initEntry(GameListing * l)
{
	gamesListModel->updateListing(l);
}

void GameListingRegistry::onErase(GameListing * l)
{
	gamesListModel->removeListing(l);
	delete l->gameData;		//used by ORO
	delete l;
}

void PlayerListingRegistry::initEntry(PlayerListing * l)
{
	playerListModel->updateListing(l);
}

void PlayerListingRegistry::onErase(PlayerListing * l)
{
	playerListModel->removeListing(l);
	delete l;
}

PlayerListing * PlayerListingRegistry::getPlayerFromNotNickName(const QString & notnickname)
{
	std::map<QString, PlayerListing *>::iterator i;
	for(i = getStorage()->begin(); i != getStorage()->end(); i++)
	{
		if(i->second->notnickname == notnickname)
			return (i->second);
	}
	return NULL;
}

void PlayerListingIDRegistry::initEntry(PlayerListing * l)
{
	playerListModel->updateListing(l);
}

void PlayerListingIDRegistry::onErase(PlayerListing * l)
{
	playerListModel->removeListing(l);
	delete l;
}

PlayerListing * PlayerListingIDRegistry::getPlayerFromName(const QString & name)
{
	std::map<unsigned int, PlayerListing *>::iterator i;
	for(i = getStorage()->begin(); i != getStorage()->end(); i++)
	{
		if(i->second->name == name)
			return (i->second);
	}
	return NULL;
}
