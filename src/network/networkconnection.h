#ifndef NETWORKCONNECTION_H
#define NETWORKCONNECTION_H
#include <map>
#include <QtCore>
#include <QtNetwork>
#include "messages.h"
#include "newline_pipe.h"

class GameListing;
class PlayerListing;
class NetworkDispatch;
class ConsoleDispatch;
class BoardDispatch;

class GameDialog;
class Talk;

class NetworkConnection;
class BoardDispatchRegistry;
class GameDialogRegistry;
class TalkRegistry;

class Room;

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
		int getConnectionState();	//should probably return enum
		void userCanceled(void) { connectionState = CANCELED; /* anything else? */};
		void onClose(void);	//so far private? we're going to have tygem use it...
		virtual void sendText(QString text) = 0;
		virtual void sendText(const char * text) = 0;
		void sendConsoleText(const char * text);
		virtual void sendDisconnect(void) = 0;
		virtual void sendMsg(unsigned int game_id, QString text) = 0;
		virtual void sendMsg(PlayerListing & player, QString text) = 0;
		virtual void sendToggle(const QString & param, bool val) = 0;
		virtual void sendObserve(const GameListing & game) = 0;
		virtual void sendObserveOutside(const GameListing &) {};	//optional
		virtual void stopObserving(const GameListing &) {};
		virtual void stopReviewing(const GameListing &) {};
		virtual void sendStatsRequest(const PlayerListing & opponent) = 0;
		virtual void sendPlayersRequest(void) = 0;
		virtual void sendGamesRequest(void) = 0;
		virtual void sendMatchInvite(const PlayerListing &) = 0;
		virtual void adjournGame(const GameListing &) {};
		virtual void sendTime(BoardDispatch *) {};
		virtual void sendMove(unsigned int game_id, class MoveRecord * m) = 0;
		virtual void sendRequestCount(unsigned int) {};
		virtual void sendAcceptCountRequest(class GameData *) {};
		virtual void sendRefuseCountRequest(class GameData *) {};
		virtual void sendRequestDraw(unsigned int) {};
		virtual void sendAcceptDrawRequest(class GameData *) {};
		virtual void sendRefuseDrawRequest(class GameData *) {};
		virtual void sendRequestMatchMode(unsigned int) {};
		virtual void sendTimeLoss(unsigned int) {};	//overwrite or not
		virtual void sendResult(class GameData *, class GameResult *) {};	//optional
		virtual void sendMatchRequest(class MatchRequest * mr) = 0;
		virtual void sendRematchRequest(void) {};	//optional
		virtual void sendRematchAccept(void) {};
		virtual void declineMatchOffer(const PlayerListing & opponent) = 0;
		virtual void cancelMatchOffer(const PlayerListing & ) {};
		virtual void acceptMatchOffer(const PlayerListing & opponent, class MatchRequest * mr) = 0;
		virtual QTime gd_checkMainTime(TimeSystem, const QTime & t);
		virtual QTime gd_checkPeriodTime(TimeSystem, const QTime & t) { return t; };
		virtual unsigned int gd_checkPeriods(TimeSystem, unsigned int p) { return p; };
		virtual void sendRejectCount(class GameData *) {};
		virtual void sendAcceptCount(class GameData *) {};
		virtual void sendAdjournRequest(void) = 0;
		virtual void sendAdjourn(void) = 0;
		virtual void sendRefuseAdjourn(void) = 0;
		int write(const char * packet, unsigned int size);
		void setConsoleDispatch(ConsoleDispatch * c);
		void setDefaultRoom(Room * r) { default_room = r; };
		ConsoleDispatch * getConsoleDispatch(void) { return console_dispatch; };
		Room * getDefaultRoom(void) { return default_room; };
		class PlayerListing * getPlayerListingFromFriendFanListing(class FriendFanListing & f);
		virtual bool isReady(void) = 0;
		virtual void handlePendingData(newline_pipe <unsigned char> * p) = 0;
		virtual void setKeepAlive(int) {};
		virtual void changeServer(void) {};
		virtual void createRoom(void) {};
		virtual void sendCreateRoom(class RoomCreate *) {};
		virtual void sendJoinRoom(const RoomListing &, const char * = 0) {};
		
		
		virtual char * sendRequestAccountInfo(int *, void *) { return NULL;} ;
		virtual void handleAccountInfoMsg(int, char *) {};
		
		virtual void addFriend(PlayerListing & player);
		virtual void removeFriend(PlayerListing & player);
		virtual void addFan(PlayerListing & player);
		virtual void removeFan(PlayerListing & player);
		virtual void addBlock(PlayerListing & player);
		virtual void removeBlock(PlayerListing & player);
		virtual char * sendAddFriend(int *, void *) { return NULL;};
		virtual void recvFriendResponse(int, char *) {};
		virtual char * sendRemoveFriend(int *, void *) { return NULL;};
		virtual char * sendAddBlock(int *, void *) { return NULL; };
		virtual char * sendRemoveBlock(int *, void *) { return NULL; };
		
		std::vector<class FriendFanListing *> & getFriendsList(void) { return friendedList; };
		std::vector<class FriendFanListing *> & getFansList(void) { return watchedList; };
		std::vector<class FriendFanListing *> & getBlockedList(void) { return blockedList; };
		
		// FIXME Not certain but maybe this chunk below should be protected:??
		BoardDispatch * getBoardDispatch(unsigned int game_id);
		BoardDispatch * getIfBoardDispatch(unsigned int game_id);
		virtual void closeBoardDispatch(unsigned int game_id);
		int getBoardDispatches(void);
		GameDialog * getGameDialog(const PlayerListing & opponent);
		GameDialog * getIfGameDialog(const PlayerListing & opponent);
		void closeGameDialog(const PlayerListing & opponent);
		class MatchRequest * getAndCloseGameDialog(const PlayerListing & opponent);
		Talk * getTalk(PlayerListing & opponent);
		Talk * getIfTalk(PlayerListing & opponent);
		virtual void closeTalk(PlayerListing & opponent);
		
		const QString & getUsername(void) { return username; };
		virtual const PlayerListing & getOurListing(void) = 0;
		virtual unsigned short getRoomNumber(void) { return 0; };
		virtual void requestGameInfo(unsigned int) {};		//for IGS on board open
		virtual void requestGameStats(unsigned int) {};		//same
		virtual void periodicListRefreshes(bool) {};
		virtual unsigned int rankToScore(QString rank) = 0;
		virtual unsigned long getGameDialogFlags(void) { return 0; };
		virtual void getAndSetFriendFanType(PlayerListing & player);
		virtual void checkGameWatched(GameListing & game);
		
		virtual int gd_verifyBoardSize(int v) { return v; };
		
		virtual bool playerTrackingByID(void) { return false; };
		virtual bool supportsMultipleUndo(void) { return false; };
		virtual bool supportsRequestMatchMode(void) { return false; };
		virtual bool supportsRequestAdjourn(void) { return false; };
		virtual bool supportsRequestDraw(void) { return false; };
		virtual bool supportsRequestCount(void) { return false; };
		virtual bool supportsObserveOutside(void) { return false; };
		virtual bool supportsServerChange(void) { return false; };
		virtual bool supportsRefreshListsButtons(void) { return false; };
		virtual bool supportsRematch(void) { return false; };
		virtual bool startTimerOnOpen(void) { return false; };	//name?? no "supports"?
		virtual bool clientCountsTime(void) { return true; };
		virtual bool clientSendsTime(void) { return false; };
		virtual bool unmarkUnmarksAllDeadStones(void) { return false; };
		virtual bool cantMarkOppStonesDead(void) { return false; };
		virtual bool twoPassesEndsGame(void) { return false; };		//used?? FIXME
		virtual bool supportsFriendList(void) { return false; };
		virtual bool supportsFanList(void) { return false; };
		virtual bool supportsBlockList(void) { return false; };
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
	protected:
		virtual bool readyToWrite(void) { return true; };
		virtual void setReadyToWrite(void) {};
		virtual void onAuthenticationNegotiated(void);
		virtual void onReady(void);
		QTcpSocket * getQSocket(void) { return qsocket; };
		void writeFromBuffer(void);
		void writeZeroPaddedString(char * dst, const QString & src, int size);
		class ServerListStorage & getServerListStorage(void);
		bool firstonReadyCall;
		friend class GameDialogRegistry;
		friend class TalkRegistry;
		Room * default_room;
		ConsoleDispatch * console_dispatch;
		bool openConnection(const QString & host, const unsigned short port);
		
		newline_pipe <unsigned char> pending;
		newline_pipe <unsigned char> send_buffer;	//not always used

		BoardDispatchRegistry * boardDispatchRegistry;
		GameDialogRegistry * gameDialogRegistry;
		TalkRegistry * talkRegistry;

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
			PROTOCOL_ERROR,
   			ALREADY_LOGGED_IN,
      			CONN_REFUSED
		} connectionState;
		
		bool friendfan_notify_default;
		
		std::vector<FriendFanListing *> friendedList;
		std::vector<FriendFanListing *> watchedList;
		std::vector<FriendFanListing *> blockedList;
		
	private:
		void setupRoomAndConsole(void);
		void tearDownRoomAndConsole(void);
		void loadfriendsfans(void);
		void savefriendsfans(void);
		
		QTcpSocket * qsocket;	//unnecessary?
		
		Room * mainwindowroom;
		
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

struct FriendFanListing
{
	FriendFanListing(QString n, bool b) : name(n), id(0), notify(b), online(false) {};
	FriendFanListing(QString n) : name(n), id(0), notify(false), online(false) {};		//blocked has no notify
	QString name;
	unsigned short id;		//if necessary
	bool notify;
	bool online;
};
#endif //NETWORKCONNECTION_H
