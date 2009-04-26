#include "../registry.h"

class PlayerListModel;
class GamesListModel;

class PlayerListingRegistry : public Registry <QString, PlayerListing>
{
	public:
		PlayerListingRegistry(PlayerListModel * model) : playerListModel(model) {};
	private:
		virtual void initEntry(PlayerListing * l);
		virtual void onErase(PlayerListing * l);		
		PlayerListModel * playerListModel;
};

class PlayerListingIDRegistry : public Registry <unsigned int, PlayerListing>
{
	public:
		PlayerListingIDRegistry(PlayerListModel * model) : playerListModel(model) {};
		PlayerListing * getPlayerFromName(const QString & name);
	private:
		virtual void initEntry(PlayerListing * l);
		virtual void onErase(PlayerListing * l);		
		PlayerListModel * playerListModel;
};

class GameListingRegistry : public Registry <unsigned int, GameListing>
{
	public:
		GameListingRegistry(GamesListModel * model) : gamesListModel(model) {};
	private:
		virtual void initEntry(GameListing * l);
		virtual void onErase(GameListing * l);		
		GamesListModel * gamesListModel;
};
