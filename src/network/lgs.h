#include <QtNetwork>
#include "igsconnection.h"
#include "networkdispatch.h"
#include "messages.h"
#include "newline_pipe.h"

class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;

class LGSConnection : public IGSConnection
{
	public:
		LGSConnection(NetworkDispatch * _dispatch, const QString & user, const QString & pass);
		virtual void sendPlayersRequest(void);
		virtual void onReady(void);
		virtual unsigned long getRoomStructureFlags(void) { return RS_NOROOMLIST; };
		virtual void requestGameInfo(unsigned int game_id);
	private:
		virtual void registerMsgHandlers(void);
};

class LGS_info : public MsgHandler
{ 
	public:
		LGS_info(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
/*class WING_kibitz : public MsgHandler
{ 
	public:
		WING_kibitz(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};*/

