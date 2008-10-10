#ifndef ROOMDISPATCH_H
#define ROOMDISPATCH_H
#include "networkdispatch.h"

class TalkDispatch;
class PlayerListing;
class GameListing;
class PlayerListingRegistry;
class PlayerListingIDRegistry;
class GameListingRegistry;
class Room;

class RoomDispatch : public NetworkDispatch
{
	public:
		RoomDispatch(Room * r);
		RoomDispatch();
		virtual void onError();
		virtual void setConnection(NetworkConnection * c);
		virtual void recvGameListing(class GameListing * g);
		virtual void recvPlayerListing(class PlayerListing * g);
		virtual void recvExtPlayerListing(class PlayerListing * g);
		virtual class BoardDispatch * getNewBoardDispatch(unsigned int key);
		void talkOpened(TalkDispatch * d);
		void recvToggle(int type, bool val);
		PlayerListing * getPlayerListing(const QString & name);
		PlayerListing * getPlayerListing(const unsigned int id);
		GameListing * getGameListing(unsigned int key);
		
		void sendStatsRequest(const PlayerListing & opponent);
		void sendObserve(const GameListing & game);
		void sendObserveOutside(const GameListing & game);
		void sendMatchInvite(const PlayerListing & opponent);
		void sendPlayersRequest(void);
		void sendGamesRequest(void);
		
		bool supportsObserveOutside(void) { if(connection) return connection->supportsObserveOutside(); else return false; };
		unsigned long getPlayerListColumns(void) { if(connection) return connection->getPlayerListColumns(); else return 0; };
		void clearPlayerList(void);
		void clearGamesList(void);
		
		virtual ~RoomDispatch();
	protected:
		GameListingRegistry * gameListingRegistry;
		PlayerListingRegistry * playerListingRegistry;
		PlayerListingIDRegistry * playerListingIDRegistry;
		GameListing * registerGameListing(GameListing * l);
		void unRegisterGameListing(unsigned int key);
		Room * room;
};
#endif //ROOMDISPATCH_H
