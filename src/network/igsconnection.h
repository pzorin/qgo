#ifndef IGSCONNECTION_H
#define IGSCONNECTION_H
#include <QtNetwork>
#include "networkconnection.h"
#include "messages.h"
#include "newline_pipe.h"
#include "gamedata.h"

class BoardDispatch;
class GameDialog;
class Talk;

class IGSConnection : public NetworkConnection
{
	public:
		IGSConnection(const QString & user, const QString & pass);
		IGSConnection();
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
	
		/* This is because of that silly IGShandler crap and because I don't want
		* connectionState to be public, just protected */
		void setPassFailed(void) { connectionState = PASS_FAILED; };
		void setProtocolError(void) { connectionState = PROTOCOL_ERROR; };
			
	protected:
		virtual bool readyToWrite(void);
		virtual void setReadyToWrite(void) { writeReady = true; };
		void handleLogin(QString msg);
		void handlePassword(QString msg);
		void handleMessage(QString msg);

		void handle_loginmsg(QString line);
		void handle_prompt(QString line);
		void handle_beep(QString line);
		void handle_down(QString line);
		void handle_error(QString line);
		void handle_games(QString line);
		void handle_file(QString line);
		virtual void handle_info(QString line);
		virtual void handle_kibitz(QString line);
		void handle_messages(QString line);
		void handle_move(QString line);
		void handle_say(QString line);
		void handle_score_m(QString line);
		void handle_shout(QString line);
		void handle_status(QString line);
		void handle_stored(QString line);
		void handle_tell(QString line);
		void handle_thist(QString line);
		void handle_who(QString line);
		void handle_undo(QString line);
		void handle_yell(QString line);
		void handle_automatch(QString line);
		void handle_serverinfo(QString line);
		void handle_dot(QString line);
		void handle_userlist(QString line);
		void handle_removed(QString line);
		void handle_ingamesay(QString line);
		void handle_adjourndeclined(QString line);
		void handle_seek(QString line);
		void handle_review(QString line);
		
		QString element(const QString &line, int index, const QString &del1, const QString &del2="", bool killblanks=FALSE);
		unsigned int idleTimeToSeconds(QString time);
		void fixRankString(QString * rank);
		
		bool writeReady;
		int keepAliveTimer;
		
		QString protocol_save_string;
		int protocol_save_int;
		QString match_playerName;
		TimeRecord * btime, * wtime;
	
	//private:
		int time_to_seconds(const QString & time);
		
		QTextCodec * textCodec;
		
		virtual void timerEvent(QTimerEvent*);
	private:
		void init(void);
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

#endif //IGSCONNECTION_H
