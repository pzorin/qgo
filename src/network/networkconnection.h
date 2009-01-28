#ifndef NETWORKCONNECTION_H
#define NETWORKCONNECTION_H
#include <map>
#include <QtCore>
#include <QtNetwork>
//#include "networkdispatch.h"
#include "messages.h"
#include "newline_pipe.h"

class GameListing;
class PlayerListing;

class NetworkDispatch;
class ConsoleDispatch;
class RoomDispatch;
class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;

class NetworkConnection;
class BoardDispatchRegistry;
class GameDialogDispatchRegistry;
class TalkDispatchRegistry;

/* I think this needs to inherit from QObject so that it can pick up signals, if those
 * are still necessary */
/*game_id and key are used interchangeably, we should unify them under
 * name game_id */
class NetworkConnection : public QObject
{
	Q_OBJECT

	public:
		NetworkConnection();
		~NetworkConnection();
		int getConnectionState();
		virtual void sendText(QString text) = 0;
		virtual void sendText(const char * text) = 0;
		virtual void sendDisconnect(void) = 0;
		virtual void sendMsg(unsigned int game_id, QString text) = 0;
		virtual void sendMsg(const PlayerListing & player, QString text) = 0;
		virtual void sendToggle(const QString & param, bool val) = 0;
		virtual void sendObserve(const GameListing & game) = 0;
		virtual void sendObserveOutside(const GameListing &) {};	//optional
		virtual void stopObserving(const GameListing & game) = 0;
		virtual void stopReviewing(const GameListing & game) = 0;
		virtual void sendStatsRequest(const PlayerListing & opponent) = 0;
		virtual void sendPlayersRequest(void) = 0;
		virtual void sendGamesRequest(void) = 0;
		virtual void sendMatchInvite(const PlayerListing &) = 0;
		virtual void adjournGame(const GameListing &) = 0;
		virtual void sendTime(BoardDispatch *) {};
		virtual void sendMove(unsigned int game_id, class MoveRecord * m) = 0;
		virtual void sendTimeLoss(unsigned int) {};	//overwrite or not
		virtual void sendMatchRequest(class MatchRequest * mr) = 0;
		virtual void sendRematchRequest(void) {};	//optional
		virtual void sendRematchAccept(void) {};
		virtual void declineMatchOffer(const PlayerListing & opponent) = 0;
		virtual void cancelMatchOffer(const PlayerListing & ) {};
		virtual void acceptMatchOffer(const PlayerListing & opponent, class MatchRequest * mr) = 0;
		virtual void sendAdjournRequest(void) = 0;
		virtual void sendAdjourn(void) = 0;
		virtual void sendRefuseAdjourn(void) = 0;
		int write(const char * packet, unsigned int size);
		void setConsoleDispatch(ConsoleDispatch * c);
		void setDefaultRoomDispatch(RoomDispatch * r) { default_room_dispatch = r; };
		ConsoleDispatch * getConsoleDispatch(void) { return console_dispatch; };
		RoomDispatch * getDefaultRoomDispatch(void) { return default_room_dispatch; };
		NetworkDispatch * getDefaultDispatch(void) { return dispatch; };
		virtual bool isReady(void) = 0;
		virtual void handlePendingData(newline_pipe <unsigned char> * p) = 0;
		virtual void onReady(void) = 0;
		virtual void setKeepAlive(int) {};
		virtual void changeServer(void) {};
		virtual void createRoom(void) {};
		virtual void sendCreateRoom(class RoomCreate *) {};
		virtual void sendJoinRoom(const RoomListing &, const char *) {};
		
		// FIXME Not certain but maybe this chunk below should be protected:??
		BoardDispatch * getBoardDispatch(unsigned int game_id);
		BoardDispatch * getIfBoardDispatch(unsigned int game_id);
		virtual void closeBoardDispatch(unsigned int game_id);
		GameDialogDispatch * getGameDialogDispatch(const PlayerListing & opponent);
		GameDialogDispatch * getIfGameDialogDispatch(const PlayerListing & opponent);
		void closeGameDialogDispatch(const PlayerListing & opponent);
		class MatchRequest * getAndCloseGameDialogDispatch(const PlayerListing & opponent);
		TalkDispatch * getTalkDispatch(const PlayerListing & opponent);
		void closeTalkDispatch(const PlayerListing & opponent);
		
		const QString & getUsername(void) { return username; };
		virtual const PlayerListing & getOurListing(void) = 0;
		virtual unsigned short getRoomNumber(void) { return 0; };
		virtual void requestGameInfo(unsigned int game_id) = 0;
		virtual void requestGameStats(unsigned int game_id) = 0;
		virtual unsigned int rankToScore(QString rank) = 0;
		virtual unsigned long getGameDialogFlags(void) { return 0; };
		virtual bool playerTrackingByID(void) { return false; };
		virtual bool supportsMultipleUndo(void) { return false; };
		virtual bool supportsObserveOutside(void) { return false; };
		virtual bool supportsServerChange(void) { return false; };
		virtual bool supportsRematch(void) { return false; };
		virtual bool startTimerOnOpen(void) { return false; };	//name?? no "supports"?
		virtual bool clientCountsTime(void) { return true; };
		virtual bool clientSendsTime(void) { return false; };
		virtual bool unmarkUnmarksAllDeadStones(void) { return false; };
		virtual bool supportsSeek(void) { return false; };
		virtual unsigned long getPlayerListColumns(void) { return 0; };
		#define PL_NOWINSLOSSES		0x01
		#define PL_NOMATCHPREFS		0x02
		virtual bool supportsChannels(void) { return false; };
		virtual bool supportsCreateRoom(void) { return false; };
		virtual unsigned long getRoomStructureFlags(void) = 0;
		#define RS_NOROOMLIST	 	0x01
		#define RS_SHORTROOMLIST	0x02
		#define RS_LONGROOMLIST		0x04
		#define RS_ONEROOMATATIME	0x08
		#define RS_ONEGAMEPERROOM	0x10
		std::vector<class RoomListing *> * getRoomList(void) { return 0; };
		void recvRoomListing(class RoomListing * r);
		/* again, recvRoom is solely for igs handlers which aren't
		 * part of networkconnection FIXME */
		
		void recvSeekCondition(class SeekCondition * s);
		void recvSeekCancel(void);
		void recvSeekPlayer(QString player, QString condition);
		virtual void sendSeek(class SeekCondition *) {};
		virtual void sendSeekCancel(void) {};
		/* The only reason closeConnection is public as opposed to
		 * protected is so the msghandlers can close it.  FIXME
		 * We need to have just one way that things get disconnected,
		 * error or not... not a bunch of ways from different places */
		void closeConnection(bool send_disconnect = true);
		/* These are ugly but we need them for IGS and maybe WING
		 * FIXME if possible Note that these are here rather than
		 * on the IGS class because they have to be accessed
		 * by IGS handlers which are not part of IGS class,
		 * maybe they should be or something... FIXME*/
		QString protocol_save_string;
		int protocol_save_int;
		QString match_playerName;	//again, this should be moved FIXME
	protected:
		virtual bool readyToWrite(void) { return true; };
		virtual void setReadyToWrite(void) {};
		void writeFromBuffer(void);
		bool firstonReadyCall;
		friend class GameDialogDispatchRegistry;
		friend class TalkDispatchRegistry;
		NetworkDispatch * dispatch;
		RoomDispatch * default_room_dispatch;
		ConsoleDispatch * console_dispatch;
		bool openConnection(const QString & host, const unsigned short port);
		
		newline_pipe <unsigned char> pending;
		newline_pipe <unsigned char> send_buffer;	//not always used

		BoardDispatchRegistry * boardDispatchRegistry;
		GameDialogDispatchRegistry * gameDialogDispatchRegistry;
		TalkDispatchRegistry * talkDispatchRegistry;

		QString username;
		QString password;
		
		enum {
			LOGIN,
   			PASSWORD,
                  	PASSWORD_SENT,
   			AUTH_FAILED,
      			PASS_FAILED,
      			INFO,
     			CONNECTED,
     			RECONNECTING,
     			CANCELED,
			PROTOCOL_ERROR
		} connectionState;
		
	private:
		QTcpSocket * qsocket;	
		
	protected slots:
		virtual void OnConnected();
	private slots:
		void OnHostFound();
		void OnReadyRead();
		void OnConnectionClosed();
		//OnDelayedCloseFinish();
		//OnBytesWritten(int);
		void OnError(QAbstractSocket::SocketError);
};
#endif //NETWORKCONNECTION_H
