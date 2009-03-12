#include "registry.h"

class PlayerListing;

class BoardDispatchRegistry : public Registry <unsigned int, BoardDispatch>
{
	public:
		BoardDispatchRegistry(NetworkConnection * c) : _c(c) {};
		std::map<unsigned int, BoardDispatch *> * getRegistryStorage(void);
	private:
		virtual BoardDispatch * getNewEntry(unsigned int game_id);
		//virtual void initEntry(BoardDispatch *);
		virtual void onErase(BoardDispatch *);
		NetworkConnection * _c;
};

class GameDialogRegistry : public Registry <const PlayerListing *, GameDialog>
{
	public:
		GameDialogRegistry(NetworkConnection * c) : _c(c) {};
	private:
		virtual GameDialog * getNewEntry(const PlayerListing * opponent);
		//virtual void initEntry(GameDialog *);
		virtual void onErase(GameDialog *);
		NetworkConnection * _c;
};

class TalkRegistry : public Registry <PlayerListing *, Talk>
{
	public:
		TalkRegistry(NetworkConnection * c) : _c(c) {};
	private:
		virtual Talk * getNewEntry(PlayerListing * opponent);
		//virtual void initEntry(Talk *);
		virtual void onErase(Talk *);
		NetworkConnection * _c;
};

