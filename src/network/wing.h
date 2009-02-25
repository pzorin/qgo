#include <QtNetwork>
#include "igsconnection.h"

class WingConnection : public IGSConnection
{
	public:
		WingConnection(const QString & user, const QString & pass);
		virtual void sendPlayersRequest(void);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		virtual void onReady(void);
		virtual void requestGameInfo(unsigned int game_id);
	private:
		virtual void handle_info(QString);
		virtual void handle_kibitz(QString);
};
