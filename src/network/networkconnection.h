/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


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

class MatchNegotiationState;

class Room;
class QMessageBox;

#define NOT_MAIN_CONNECTION		true

/* Some of these methods should be converted to slots.
 * Also, NetworkConnection should emit signals for many events */
class NetworkConnection : public QObject
{
	Q_OBJECT

	public:
		NetworkConnection();
		~NetworkConnection();
		int getConnectionState();	//should probably return enum
		void userCanceled(void) { connectionState = CANCELED; /* anything else? */};
		int checkForOpenBoards(void);
public slots:
        virtual void sendPlayersRequest(void) = 0;
public:
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
		virtual void sendGamesRequest(void) = 0;
		virtual void sendMatchInvite(const PlayerListing &) = 0;
		virtual void adjournGame(const GameListing &) {};
		virtual void sendTime(BoardDispatch *) {};
		virtual void sendAddTime(int) {};
		virtual void sendMove(unsigned int game_id, class MoveRecord * m) = 0;
		virtual void sendRequestCount(unsigned int) {};
		virtual void sendAcceptCountRequest(class GameData *) {};
		virtual void sendRefuseCountRequest(class GameData *) {};
		virtual void sendAcceptRequestMatchMode(unsigned int) {};
		virtual void sendDeclineRequestMatchMode(unsigned int) {};
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
		virtual void sendAdjournRequest(void) {};
		virtual void sendAdjourn(void) {};
		virtual void sendRefuseAdjourn(void) {};
public:
		int write(const char * packet, unsigned int size);
		void setConsoleDispatch(ConsoleDispatch * c);
		void setDefaultRoom(Room * r) { default_room = r; };
		ConsoleDispatch * getConsoleDispatch(void) { return console_dispatch; };
		Room * getDefaultRoom(void) { return default_room; };
		class PlayerListing * getPlayerListingFromFriendWatchListing(class FriendWatchListing & f);
		virtual bool isReady(void) = 0;
		virtual void handlePendingData(newline_pipe <unsigned char> * p) = 0;
		virtual void changeServer(void) {};
		virtual void createRoom(void) {};
		virtual void sendCreateRoom(class RoomCreate *) {};
		virtual void sendJoinRoom(const RoomListing &, const char * = 0) {};
		virtual void sendJoinChannel(const ChannelListing &) {};
		
		
		virtual char * sendRequestAccountInfo(int *, void *) { return NULL;} ;
		virtual void handleAccountInfoMsg(int, char *) {};
		
		virtual void addFriend(PlayerListing & player);
		virtual void removeFriend(PlayerListing & player);
		virtual void addWatch(PlayerListing & player);
		virtual void removeWatch(PlayerListing & player);
		virtual void addBlock(PlayerListing & player);
		virtual void removeBlock(PlayerListing & player);
		virtual char * sendAddFriend(int *, void *) { return NULL;};
		virtual void recvFriendResponse(int, char *) {};
		virtual char * sendRemoveFriend(int *, void *) { return NULL;};
		virtual char * sendAddBlock(int *, void *) { return NULL; };
		virtual char * sendRemoveBlock(int *, void *) { return NULL; };
		
		std::vector<class FriendWatchListing *> & getFriendsList(void) { return friendedList; };
		std::vector<class FriendWatchListing *> & getWatchesList(void) { return watchedList; };
		std::vector<class FriendWatchListing *> & getBlockedList(void) { return blockedList; };
		
		// FIXME Not certain but maybe this chunk below should be protected:??
		BoardDispatch * getBoardDispatch(unsigned int game_id);
		BoardDispatch * getIfBoardDispatch(unsigned int game_id);
		virtual void closeBoardDispatch(unsigned int game_id);
		int getBoardDispatches(void);
        GameDialog * getGameDialog(const PlayerListing * opponent);
        GameDialog * getIfGameDialog(const PlayerListing * opponent);
        void closeGameDialog(const PlayerListing *opponent);
        class MatchRequest * getAndCloseGameDialog(const PlayerListing *opponent);
        Talk * getTalk(PlayerListing *opponent);
        Talk * getIfTalk(PlayerListing *opponent);
        virtual void closeTalk(const PlayerListing *opponent);
		
		const QString & getUsername(void) { return username; };
        virtual const PlayerListing & getOurListing(void);
		virtual unsigned short getRoomNumber(void) { return 0; };
		virtual void requestGameInfo(unsigned int) {};		//for IGS on board open
		virtual void requestGameStats(unsigned int) {};		//same
		virtual unsigned int rankToScore(QString rank) = 0;
		virtual unsigned long getGameDialogFlags(void) { return 0; };
		virtual void getAndSetFriendWatchType(PlayerListing & player);
		virtual void checkGameWatched(GameListing & game);
		
		virtual int gd_verifyBoardSize(int v) { return v; };
		
		virtual bool playerTrackingByID(void) { return false; };
		virtual bool flipCoords(void) { return true; };
		virtual bool supportsMultipleUndo(void) { return false; };
		virtual bool supportsRequestMatchMode(void) { return false; };
		virtual bool supportsRequestAdjourn(void) { return false; };
		virtual bool supportsRequestDraw(void) { return false; };
		virtual bool supportsRequestCount(void) { return false; };
		virtual bool supportsAddTime(void) { return false; };
		virtual bool supportsObserveOutside(void) { return false; };
		virtual bool supportsServerChange(void) { return false; };
		virtual bool supportsRefreshListsButtons(void) { return false; };
		virtual bool consoleIsChat(void) { return true; };
		virtual bool supportsRematch(void) { return false; };
		virtual bool startTimerOnOpen(void) { return false; };	//name?? no "supports"?
		virtual bool clientCountsTime(void) { return true; };
		virtual bool clientSendsTime(void) { return false; };
		virtual bool undoResetsScore(void) { return false; };
		virtual bool netWillEnterScoreMode(void) { return false; };
		virtual bool canMarkStonesDeadinScore(void) { return true; };
		virtual bool unmarkUnmarksAllDeadStones(void) { return false; };
		virtual bool cantMarkOppStonesDead(void) { return false; };
		virtual bool twoPassesEndsGame(void) { return false; };		//used?? FIXME
		virtual bool supportsFriendList(void) { return false; };
		virtual bool supportsWatchList(void) { return false; };
		virtual bool supportsBlockList(void) { return false; };
		virtual bool supportsSeek(void) { return false; };
		virtual const char * getCodecString(void) { return ""; };
		virtual QString getPlaceString(void) { return ""; };
		virtual void saveIfDoesntSave(GameData *) {};
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
		void recvChannelListing(class ChannelListing * r);
		/* again, recvRoom is solely for igs handlers which aren't
		 * part of networkconnection FIXME */
		
		void recvSeekCondition(class SeekCondition * s);
		void recvSeekCancel(void);
		void recvSeekPlayer(QString player, QString condition);
		virtual void sendSeek(class SeekCondition *) {};
		virtual void sendSeekCancel(void) {};

    signals:
        void ready(void);
        void playerListingUpdated(PlayerListing *);
		
	protected:
		void closeConnection(bool send_disconnect = true);
		virtual bool readyToWrite(void) { return true; };
		virtual void setReadyToWrite(void) {};
		void setConnected(void);
		virtual void onAuthenticationNegotiated(void);
		virtual void onReady(void);
		QTcpSocket * getQSocket(void) { return qsocket; };
		void writeFromBuffer(void);
		void writeZeroPaddedString(char * dst, const QString & src, int size);
		class ServerListStorage & getServerListStorage(void);
		bool openConnection(const QString & host, const unsigned short port, bool not_main_connection = false);
		void latencyOnSend(void);
		void latencyOnRecv(void);
		void changeChannel(const QString & s);

		bool firstonReadyCall;
		Room * default_room;
		ConsoleDispatch * console_dispatch;
		
		
		newline_pipe <unsigned char> pending;
		newline_pipe <unsigned char> send_buffer;	//not always used

		QString username;
		QString password;
		
		enum {
			LOGIN,
   			PASSWORD,
            		PASSWORD_SENT,
   			AUTH_FAILED,
      			PASS_FAILED,
      			INFO,
			SETUP,
     			CONNECTED,
     			RECONNECTING,
     			CANCELED,
			PROTOCOL_ERROR,
   			ALREADY_LOGGED_IN,
      			CONN_REFUSED,
			HOST_NOT_FOUND,
			SOCK_TIMEOUT,
			UNKNOWN_ERROR
		} connectionState;
		
		bool friendwatch_notify_default;
		
		std::vector<FriendWatchListing *> friendedList;
		std::vector<FriendWatchListing *> watchedList;
		std::vector<FriendWatchListing *> blockedList;

		MatchNegotiationState * match_negotiation_state;
		int lastMainTimeChecked, lastPeriodTimeChecked, lastPeriodsChecked;

	private:
		void setupRoomAndConsole(void);
		void tearDownRoomAndConsole(void);
		void loadfriendswatches(void);
		void savefriendswatches(void);

		void drawPleaseWait(void);
		
		QTcpSocket * qsocket;	//unnecessary?
		
		Room * mainwindowroom;

		QMessageBox * connectingDialog;
#ifdef LATENCY
		struct timeval latencyLast;
		unsigned long latencyAverage;
#endif //LATENCY

protected:
        QMap <unsigned int, BoardDispatch *> boardDispatchMap;
        // Accessed by IGS connection class
private:
        QMap <const PlayerListing *, GameDialog *> gameDialogMap;
        QMap <const PlayerListing *, Talk *> talkMap;

	protected slots:
		virtual void OnConnected();
	private slots:
		//void OnHostFound();
		void OnReadyRead();
		void OnConnectionClosed();
		//OnDelayedCloseFinish();
		//OnBytesWritten(int);
		void OnError(QAbstractSocket::SocketError);
	
		void slot_cancelConnecting(void);
};

struct FriendWatchListing
{
	FriendWatchListing(QString n, bool b) : name(n), id(0), notify(b), online(false) {};
	FriendWatchListing(QString n) : name(n), id(0), notify(false), online(false) {};		//blocked has no notify
	QString name;
	unsigned short id;		//if necessary
	bool notify;
	bool online;
};
#endif //NETWORKCONNECTION_H
