#ifndef TALKDISPATCH_H
#define TALKDISPATCH_H
#include "networkdispatch.h"
#include "messages.h"

class NetworkConnection;
class GameDialogDispatch;
class RoomDispatch;
class Talk;
class PlayerListing;

class TalkDispatch : public NetworkDispatch
{
	public:
		TalkDispatch(NetworkConnection * c, const PlayerListing & opp);
		TalkDispatch(NetworkConnection * c, const PlayerListing & opp, RoomDispatch * r);
		~TalkDispatch();
		void sendTalk(QString text);
		void recvTalk(QString text);
		void updatePlayerListing(void);
		Talk * getDlg(void);
		void closeDispatchFromDialog(void);
		void sendMatchInvite(const PlayerListing & opp) { connection->sendMatchInvite(opp); };
	private:
		const PlayerListing & opponent;
		Talk * dlg;
		RoomDispatch * room;
};

#endif //TalkDISPATCH_H
