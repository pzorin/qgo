#include "roomdispatch.h"
#include "messages.h"
#include "listviews.h"
#include "roomregistries.h"
#include "room.h"
#include "talkdispatch.h"	
#include "boarddispatch.h"	//so we can remove observers
#include "playergamelistings.h"	
//#include <string.h>


RoomDispatch::RoomDispatch()
{
	/* Where do we get the pointer to the gamesListModel???*/
	/* This is a more interesting question than it looks like.
	 * Right now, we just have a mainwindowdispatch room, but
	 * eventually we'd have many rooms and the question is
	 * what creates them and gives them listviews, etc.
	 * I'm thinking we might want to pass a pointer to a 
	 * room to the roomdispatch... or have the roomdispatch
	 * create a room object.  We should figure out which.
	 * But either way, it means that we need to sort of
	 * hotwire that DefaultRoom, the mainwindowdispatch FIXME */
}

/* Another possibility is to just pass the two models because
 * I think that's all we're going to need */
RoomDispatch::RoomDispatch(Room * r) : room(r)
{
	/* In this version the Room UI MUST be setup first */
	/* See Note in mainwindowroom.cpp FIXME */
	qDebug("Constructing RoomDispatch");
	playerListingRegistry = 0;
	playerListingIDRegistry = 0;
}

RoomDispatch::~RoomDispatch()
{
	/*  FIXME, is this necessary ??
	if(connection)
		connection->setDefaultRoomDispatch(0);*/
	qDebug("Deconstructing RoomDispatch");
	delete playerListingRegistry;
	delete playerListingIDRegistry;		//delete safe for 0
	delete gameListingRegistry;
}

void RoomDispatch::onError(void)
{
	room->onError();
}

void RoomDispatch::setConnection(NetworkConnection * c)
{
	NetworkDispatch::setConnection(c);
	if(connection->getConnectionType() == TypeORO)
	{
		qDebug("Creating player ID Registry");
		playerListingIDRegistry = new PlayerListingIDRegistry(room->playerListModel);
	}
	else
	{
		qDebug("Creating player Registry");
		playerListingRegistry = new PlayerListingRegistry(room->playerListModel);
	}
	gameListingRegistry = new GameListingRegistry(room->gamesListModel);
	room->onConnectionAssignment(connection->getUsername());
}

void RoomDispatch::clearPlayerList(void)
{ 
	if(room) 
		room->clearPlayerList();
}

void RoomDispatch::clearGamesList(void) 
{ 
	if(room) 
		room->clearGamesList(); 
}

GameListing * RoomDispatch::registerGameListing(GameListing * l)
{
	return gameListingRegistry->getEntry(l->number, l);
}

/* This appears to be unused, recvGameListing does stuff
 * with "->running" FIXME */
void RoomDispatch::unRegisterGameListing(unsigned int key)
{
	gameListingRegistry->deleteEntry(key);
}

/* Remember that this and getPlayerListing do return 0.
 * getEntry functions like getIfEntry because getNewEntry
 * is not defined, since we create registry entries for
 * the games/player registries by passing them */
GameListing * RoomDispatch::getGameListing(unsigned int key)
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

void GameListingRegistry::initEntry(GameListing * l)
{
	gamesListModel->updateListing(l);
}

void GameListingRegistry::onErase(GameListing * l)
{
	gamesListModel->removeListing(l);
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

void PlayerListingIDRegistry::initEntry(PlayerListing * l)
{
	playerListModel->updateListing(l);
}

void PlayerListingIDRegistry::onErase(PlayerListing * l)
{
	playerListModel->removeListing(l);
	delete l;
}
std::map<PlayerListing *, unsigned short> removed_player;
void RoomDispatch::recvPlayerListing(PlayerListing * player)
{
	PlayerListing * p = 0;	//unused ?
	if(!player->online)
	{
		removed_player[player] = player->playing;
#ifdef FIXME
		qDebug("Removing player %s %p on attached game %d", player->name.toLatin1().constData(), player, player->playing);
#endif //FIXME
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
			/* FIXME, something should clear this when games end */
			BoardDispatch * b = connection->getIfBoardDispatch(player->observing);
			if(b)
			{
#ifdef FIXME
				qDebug("Removing player %s on observing game %d", player->name.toLatin1().constData(), player->observing);
#endif //FIXME
				b->recvObserver(player, false);
			}
		}
	}
	if(playerListingIDRegistry)
	{
		if(player->online)
			p = playerListingIDRegistry->getEntry(player->id, player);
		else
			playerListingIDRegistry->deleteEntry(player->id);	
	}
	else
	{
		if(player->online)
			p = playerListingRegistry->getEntry(player->name, player);
		else
			playerListingRegistry->deleteEntry(player->name);
	}
	if(p && player->online)
	{
		std::map<PlayerListing *, unsigned short>::iterator it;
		it = removed_player.find(p);
		if(it != removed_player.end())
			removed_player.erase(it);
	}
	room->updateRoomStats();	//in case we lost one or added one
}

void RoomDispatch::recvExtPlayerListing(class PlayerListing * player)
{
	room->recvExtPlayerListing(player);
}

void RoomDispatch::recvGameListing(GameListing * game)
{
	unsigned int key;
	/* FIXME This is iffy, we're using the other ID registry's
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
		gameListingRegistry->getEntry(key, game);
	else
		gameListingRegistry->deleteEntry(key);
	room->updateRoomStats();	//in case we lost one or added one
}

PlayerListing * RoomDispatch::getPlayerListing(const QString & name)
{
	PlayerListing * p = playerListingRegistry->getEntry(name);
	if(!p)
		qDebug("rd: gpl no player listing");
	
#ifdef FIXME
	/* this is tricky FIXME */
	if(!p)
		connection->sendStatsRequest(name);		
#endif //FIXME
	/* Should probably return & FIXME */
	return p;
}

PlayerListing * RoomDispatch::getPlayerListing(const unsigned int id)
{
	PlayerListing * p = playerListingIDRegistry->getEntry(id);
	// FIXME
	/*if(!p)
		connection->sendStatsRequest(id);*/		
	return p;
}

void RoomDispatch::sendStatsRequest(const PlayerListing & opponent)
{
	TalkDispatch * talk;
	/* Whenever a talk window is opened, we want stats.  This
	 * means its easier to create the talk window and let it
	 * always create stats, then send out stats messages
	 * that generate talk windows */
	/* This is a little weird now...almost like we're just asking
	 * for an update on the references */
	talk = connection->getTalkDispatch(opponent);
	//connection->sendStatsRequest(opponent);
	/* This is really only for ORO, and let's see if it works but... */
	if(talk)
		talk->updatePlayerListing();
}

void RoomDispatch::sendObserve(const GameListing & game)
{
	connection->sendObserve(game);
}

void RoomDispatch::sendObserveOutside(const GameListing & game)
{
	connection->sendObserveOutside(game);
}

void RoomDispatch::sendPlayersRequest(void)
{
	connection->sendPlayersRequest();
}

void RoomDispatch::sendGamesRequest(void)
{
	qDebug("Sending games request");
	connection->sendGamesRequest();
}

void RoomDispatch::sendMatchInvite(const PlayerListing & opponent)
{
	connection->sendMatchInvite(opponent);
}

/* Called from getNewEntry in BoardDispatchRegistry in networkconnection.cpp */
class BoardDispatch * RoomDispatch::getNewBoardDispatch(unsigned int key)
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
	
	return new BoardDispatch(listing);
}

/* This is so that talk dialogs can be added to mainwindow_server
 * without forcing them to always be added to mainwindow_server */
void RoomDispatch::talkOpened(TalkDispatch * d)
{	
	room->talkOpened(d);
}

/* We need to change this one.  Maybe its more of a connection
 * thing then a room thing. FIXME 
 * Its also the only thing that uses slot_checkbox anymore
 * and more than that, but really it should be on the
 * mainwindowroom side, not the dispatch side but...
 * I guess the whole distinction is a little extraneous. */
void RoomDispatch::recvToggle(int type, bool val)
{
	room->recvToggle(type, val);
}
