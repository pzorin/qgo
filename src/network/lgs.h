#include <QtNetwork>
#include "igsconnection.h"

class LGSConnection : public IGSConnection
{
	public:
		LGSConnection(const QString & user, const QString & pass);
		virtual void sendPlayersRequest(void);
		virtual void sendToggle(const QString & param, bool val);
		virtual void onReady(void);
		virtual unsigned long getRoomStructureFlags(void) { return RS_NOROOMLIST; };
		virtual void requestGameInfo(unsigned int game_id);
	private:
		virtual void handle_info(QString);
};
