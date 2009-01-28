#include <QtNetwork>
#include "igsconnection.h"
#include "networkdispatch.h"
#include "messages.h"
#include "newline_pipe.h"

class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;

class WingConnection : public IGSConnection
{
	public:
		WingConnection(NetworkDispatch * _dispatch, const QString & user, const QString & pass);
		virtual void sendPlayersRequest(void);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		virtual void onReady(void);
		virtual void requestGameInfo(unsigned int game_id);
	private:
		virtual void registerMsgHandlers(void);
};

class WING_info : public MsgHandler
{ 
	public:
		WING_info(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class WING_kibitz : public MsgHandler
{ 
	public:
		WING_kibitz(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};

