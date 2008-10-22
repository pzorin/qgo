#ifndef IGSCONNECTION_H
#define IGSCONNECTION_H
#include <QtNetwork>
#include "networkconnection.h"
#include "networkdispatch.h"
#include "msghandler.h"
#include "messages.h"
#include "newline_pipe.h"
#include "gamedata.h"

class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;

class IGSConnection : public NetworkConnection
{
	public:
		IGSConnection(NetworkDispatch * _dispatch, const class ConnectionInfo & info);
		~IGSConnection();
		virtual void sendText(QString text);
		virtual void sendText(const char * text);
		virtual void sendDisconnect(void);
		virtual void sendMsg(unsigned int game_id, QString text);
		virtual void sendMsg(const PlayerListing & player, QString text);
		virtual void sendToggle(const QString & param, bool val);
		virtual void sendObserve(const GameListing & game);
		virtual void stopObserving(const GameListing & game);
		virtual void stopReviewing(const GameListing & game);
		virtual void sendStatsRequest(const PlayerListing & opponent);
		virtual void sendPlayersRequest(void);
		virtual void sendGamesRequest(void);
		void sendRoomListRequest(void);
		virtual void sendMatchInvite(const PlayerListing & player);
		virtual void adjournGame(const GameListing & game);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void declineMatchOffer(const PlayerListing & opponent);
		virtual void acceptMatchOffer(const PlayerListing & opponent, class MatchRequest * mr);
		virtual void sendAdjournRequest(void);
		virtual void sendAdjourn(void);
		virtual void sendRefuseAdjourn(void);
		virtual void handlePendingData(newline_pipe <unsigned char> * p);
		virtual bool isReady(void);
		virtual void onReady(void);
		virtual void setKeepAlive(int);

		virtual void sendJoinRoom(const RoomListing & room, const char * password = 0);
		virtual void sendSeek(class SeekCondition *);
		virtual void sendSeekCancel(void);
		
		BoardDispatch * getBoardFromAttrib(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi);
		BoardDispatch * getBoardFromOurOpponent(QString opponent);
		virtual const PlayerListing & getOurListing(void);
		
		virtual void requestGameInfo(unsigned int game_id);
		virtual void requestGameStats(unsigned int game_id);
		virtual unsigned int rankToScore(QString rank);
		virtual bool supportsSeek(void) { return true; };
		virtual unsigned long getPlayerListColumns(void) { return PL_NOWINSLOSSES; };
		virtual bool supportsChannels(void) { return true; };
		virtual unsigned long getGameDialogFlags(void);
		virtual unsigned long getRoomStructureFlags(void) { return (RS_SHORTROOMLIST | RS_ONEROOMATATIME); };
	protected:
		virtual bool readyToWrite(void);
		virtual void setReadyToWrite(void) { writeReady = true; };
		MsgHandler * getMsgHandler(unsigned int type);
		void handleLogin(QString msg);
		void handlePassword(QString msg);
		void handleMessage(QString msg);
		
		MsgHandlerRegistry * msgHandlerRegistry;
		virtual void registerMsgHandlers(void);
		
		bool writeReady;
		int keepAliveTimer;
		
	//private:
		int time_to_seconds(const QString & time);
		
		QTextCodec * textCodec;

		enum {
			LOGIN,
			PASSWORD,
			SESSION,
			AUTH_FAILED
		} authState;
		
		virtual void timerEvent(QTimerEvent*);
	private:
		void sendNmatchParameters(void);
		/* I'm thinking of a map or a hash table here
		 * mapping room and game/board ids to particular
		 * NetworkDispatches.  Then we probably need
		 * some kind of abstract message class so that we can
		 * pass whatever message along to the proper network
		 * dispatch.  This would be so that the networkconnection
		 * doesn't have to be statically aware of the different
		 * types of network dispatches although we do already
		 * have a console dispatch. */
		/* We have to look at what the program assumes is there
		 * like boards or rooms and that's what the network
		 * connection interface should support.  We could then
		 * subclass the dispatch for different types room/board
		 * or we could just create a different type for each,
		 * inheritance would only be useful if we didn't want
		 * to know the type of dispatch before sending it
		 * data.  The thing is, the specific connection is
		 * reading packets and constructing messages to dispatches
		 * so it does really know the type of dispatch.  The
		 * ability to open multiple boards is probably supported
		 * by connections and commands so this is tricky, and each
		 * map of open boards might use a different value as a key
		 * so that needs to be stored on the specific subclass of
		 * NC and we can't add NC routines for adding dispatches
		 * to maps I don't think... but we don't want the app
		 * to know the connection type... a new board is called
		 * by some command through the dispatch, so we can just
		 * have the dispatch create new dispatches as necessary and
		 * associate them with created boards.  If the connection
		 * doesn't support that, dispatch can kill the old board.
		 * But then dispatch is coupled with the connection...
		 * well we can just have dispatch query the connection
		 * somehow... or actually I guess the type information for
		 * keys has to be pretty standard anyway. So this all
		 * kind of a moot point.*/
		/* We do have to subclass dispatch for different room/board
		 * objects because each has to implment specific routines
		 * for handling messages.  But they're probably all the
		 * same in that we can just pass an initialization
		 * pointer to the board or room app interface to be
		 * used in those routines.  The real question still
		 * is whether or not the different dispatches should inherit
		 * from a common class?  It might just make things easier,
		 * like if we wanted to do something with all of them.  But
		 * at the same time, connection is aware of the different types
		 * and there's no real list of dispatches either.  They might
		 * just support an overriden common destructor... */
};

/* I'm still not certain about this.  It seems like all these objects
 * are a huge amount of overhead when we could just have them as 
 * functions that could be overridden by the subclassed WING.
 * The reason to do them as objects is so that we can have a registry
 * and a single handler for WING and IGS and NNGS although that's
 * now defunct.  So doing it this way gives us flexibility in both
 * the msg handlers as well as the msg handler dispatcher but
 * it does seem like more overhead than necessary for only a slightly
 * more elegant or readable design */
 class IGS_loginmsg : public MsgHandler 
{ 
	public:
		IGS_loginmsg(NetworkConnection * c) : MsgHandler(c) {};				
		virtual void handleMsg(QString); 
};
class IGS_prompt : public MsgHandler 
{ 
	public:
		IGS_prompt(NetworkConnection * c) : MsgHandler(c) {};				
		virtual void handleMsg(QString); 
};
class IGS_beep : public MsgHandler
{
	public:
		IGS_beep(NetworkConnection * c) : MsgHandler(c) {};	
		virtual void handleMsg(QString);
};
class IGS_down : public MsgHandler
{
	public:
		IGS_down(NetworkConnection * c) : MsgHandler(c) {};	
		virtual void handleMsg(QString);
};
class IGS_error : public MsgHandler
{ 
	public:
		IGS_error(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_games : public MsgHandler
{ 
	public:
		IGS_games(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_file : public MsgHandler
{ 
	public:
		IGS_file(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_info : public MsgHandler
{ 
	public:
		IGS_info(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_kibitz : public MsgHandler
{ 
	public:
		IGS_kibitz(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_messages : public MsgHandler
{ 
	public:
		IGS_messages(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_move : public MsgHandler
{ 
	public:
		IGS_move(NetworkConnection * c) : MsgHandler(c)
		{
			btime = new TimeRecord();
			wtime = new TimeRecord();
		};
		~IGS_move() { delete btime; delete wtime; };
		virtual void handleMsg(QString);
	private:
		/* FIXME, maybe these should just be on stack or whatever,
		 * why allocate and deallocte explicitly? */
		TimeRecord * btime, * wtime;
};
class IGS_say : public MsgHandler
{ 
	public:
		IGS_say(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_score_m : public MsgHandler
{ 
	public:
		IGS_score_m(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_shout : public MsgHandler
{ 
	public:
		IGS_shout(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_status : public MsgHandler
{ 
	public:
		IGS_status(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_stored : public MsgHandler
{ 
	public:
		IGS_stored(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_tell : public MsgHandler
{ 
	public:
		IGS_tell(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_thist : public MsgHandler
{ 
	public:
		IGS_thist(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_who : public MsgHandler
{ 
	public:
		IGS_who(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_undo : public MsgHandler
{ 
	public:
		IGS_undo(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_yell : public MsgHandler
{ 
	public:
		IGS_yell(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_automatch : public MsgHandler
{ 
	public:
		IGS_automatch(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_serverinfo : public MsgHandler
{ 
	public:
		IGS_serverinfo(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_dot : public MsgHandler
{ 
	public:
		IGS_dot(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_userlist : public MsgHandler
{ 
	public:
		IGS_userlist(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_removed : public MsgHandler
{ 
	public:
		IGS_removed(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_adjourndeclined : public MsgHandler
{ 
	public:
		IGS_adjourndeclined(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_review : public MsgHandler
{ 
	public:
		IGS_review(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
class IGS_seek : public MsgHandler
{ 
	public:
		IGS_seek(NetworkConnection * c) : MsgHandler(c) {};
		virtual void handleMsg(QString);
};
#endif //IGSCONNECTION_H
