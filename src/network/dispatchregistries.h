#include "registry.h"

class PlayerListing;

class BoardDispatchRegistry : public Registry <unsigned int, BoardDispatch>
{
	public:
		BoardDispatchRegistry(NetworkConnection * c) : _c(c) {};
		std::map<unsigned int, BoardDispatch *> * getRegistryStorage(void);
	private:
		virtual BoardDispatch * getNewEntry(unsigned int game_id);
		virtual void initEntry(BoardDispatch *);
		virtual void onErase(BoardDispatch *);
		NetworkConnection * _c;
};

class GameDialogDispatchRegistry : public Registry <const PlayerListing *, GameDialogDispatch>
{
	public:
		GameDialogDispatchRegistry(NetworkConnection * c) : _c(c) {};
	private:
		virtual GameDialogDispatch * getNewEntry(const PlayerListing * opponent);
		virtual void initEntry(GameDialogDispatch *);
		virtual void onErase(GameDialogDispatch *);
		NetworkConnection * _c;
};

class TalkDispatchRegistry : public Registry <const PlayerListing *, TalkDispatch>
{
	public:
		TalkDispatchRegistry(NetworkConnection * c) : _c(c) {};
	private:
		virtual TalkDispatch * getNewEntry(const PlayerListing * opponent);
		virtual void initEntry(TalkDispatch *);
		NetworkConnection * _c;
};

