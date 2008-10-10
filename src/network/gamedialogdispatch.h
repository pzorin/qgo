#ifndef GAMEDIALOGDISPATCH_H
#define GAMEDIALOGDISPATCH_H
#include "networkdispatch.h"
#include "messages.h"

class NetworkConnection;
class GameDialog;

class GameDialogDispatch : public NetworkDispatch
{
	public:
		GameDialogDispatch(NetworkConnection * c, const PlayerListing & opp);
		~GameDialogDispatch();
		void closeDispatchFromDialog(void);
		void recvRequest(class MatchRequest * mr, unsigned long flags = 0);
		void recvRefuseMatch(int motive = 0);
		void sendRequest(class MatchRequest * mr);
		void declineOffer(void);
		void cancelOffer(void);
		void acceptOffer(class MatchRequest * mr);
		class MatchRequest * getMatchRequest(void);
		const PlayerListing & getOpponent(void) { return opponent; };
		const PlayerListing & getOurListing(void) { return connection->getOurListing(); };
		unsigned short getRoomNumber(void) { return connection->getRoomNumber(); };
	private:
		const PlayerListing & opponent;
		GameDialog * dlg;
};

#endif //GAMEDIALOGDISPATCH_H
