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


#include <QtNetwork>
#include "networkconnection.h"
#include "messages.h"
#include <vector>

class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;
class QuickConnection;

class CyberOroConnection : public NetworkConnection
{
	public:
        CyberOroConnection(const ConnectionCredentials credentials);
		~CyberOroConnection();
		virtual void sendText(QString text);
		virtual void sendText(const char * text);
		virtual void sendDisconnect(void);
		virtual void sendMsg(unsigned int game_id, QString text);
        virtual void sendMsg(PlayerListing * player, QString text);
		virtual void sendToggle(const QString & param, bool val);
        virtual void sendObserve(const GameListing * game);
        virtual void sendObserveOutside(const GameListing * game);
        virtual void sendStatsRequest(const PlayerListing *) {}
        virtual void sendPlayersRequest(void) {}
        virtual void sendGamesRequest(void) {}
        virtual void sendMatchInvite(const PlayerListing * opponent);
        virtual void adjournGame(const GameListing * game);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		virtual void sendRequestCount(unsigned int game_id);
		virtual void sendRequestMatchMode(unsigned int game_id);
		virtual void sendAcceptCountRequest(class GameData * data);
		virtual void sendRefuseCountRequest(class GameData * data);
		virtual void sendAcceptRequestMatchMode(unsigned int game_id);
		virtual void sendDeclineRequestMatchMode(unsigned int game_id);
		virtual void sendTimeLoss(unsigned int game_id);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void sendRematchRequest(void);
		virtual void sendRematchAccept(void);
        virtual void declineMatchOffer(const PlayerListing * opponent);
        virtual void cancelMatchOffer(const PlayerListing * opponent);
        virtual void acceptMatchOffer(const PlayerListing * opponent, class MatchRequest * mr);
		virtual void sendAdjournRequest(void);
		virtual void sendAdjourn(void);
		virtual void sendRefuseAdjourn(void);
		virtual void handlePendingData();
        virtual void onReady(void);
		virtual void changeServer(void);
		virtual void createRoom(void);
		virtual void sendCreateRoom(class RoomCreate * room);
		virtual void sendJoinRoom(const RoomListing & room, const char * password = 0);
		
		virtual char * sendRequestAccountInfo(int * size, void * p);
		virtual void handleAccountInfoMsg(int size, char * msg);
		
        virtual void addFriend(PlayerListing * player);
        virtual void removeFriend(PlayerListing * player);
		virtual char * sendAddFriend(int * size, void * p);
		virtual void recvFriendResponse(int size, char * msg);
		virtual char * sendRemoveFriend(int * size, void * p);
        virtual void addBlock(PlayerListing * player);
        virtual void removeBlock(PlayerListing * player);
		virtual char * sendAddBlock(int * size, void * p);
		virtual char * sendRemoveBlock(int * size, void * p);
		
        virtual const PlayerListing * getOurListing(void);
        virtual unsigned short getRoomNumber(void) { return room_were_in; }
		
		virtual void closeBoardDispatch(unsigned int game_id);
		
		virtual int gd_verifyBoardSize(int v);
		virtual QTime gd_checkMainTime(TimeSystem s, const QTime & t);
		virtual QTime gd_checkPeriodTime(TimeSystem s, const QTime & t);
		virtual unsigned int gd_checkPeriods(TimeSystem s, unsigned int p);
		
		virtual unsigned int rankToScore(QString rank);
		virtual unsigned long getGameDialogFlags(void);	
        virtual bool playerTrackingByID(void) { return true; }
        virtual bool netWillEnterScoreMode(void) { return true; }
        virtual bool supportsMultipleUndo(void) { return true; }
        virtual bool supportsRequestMatchMode(void) { return true; }
        virtual bool supportsRequestAdjourn(void) { return true; }
        virtual bool supportsRequestCount(void) { return true; }
        virtual bool supportsObserveOutside(void) { return true; }
        virtual bool supportsServerChange(void) { return true; }
        virtual bool supportsRematch(void) { return true; }
        virtual bool supportsFriendList(void) { return true; }
        virtual bool supportsBlockList(void) { return true; }
        virtual unsigned long getPlayerListColumns(void) { return PL_NOMATCHPREFS; }
        virtual bool supportsCreateRoom(void) { return true; }
		virtual void saveIfDoesntSave(GameData * data);
        virtual unsigned long getRoomStructureFlags(void) { return (RS_NOROOMLIST | RS_ONEROOMATATIME | RS_ONEGAMEPERROOM); }
		virtual const char * getCodecString(void);
		virtual QString getPlaceString(void);
		virtual void timerEvent(QTimerEvent * event);
	private:
		void handleServerList(unsigned char *msg);
		int reconnectToServer(void);
        void sendPersonalChat(const PlayerListing * player, const char * text);
		void sendRoomChat(const char * text);
		friend class SetPhrasePalette;
		void sendServerChat(QString text);
		void requestAccountInfo(void);
		void sendLogin(void);
		void sendSetChatMsg(unsigned short phrase_id);
        void sendObserveAfterJoining(const GameListing * game);
        void sendFinishObserving(const GameListing * game);
        void sendLeave(const GameListing *);
		void sendGameUpdate(unsigned short game_code);
        void sendKeepAlive(const GameListing * game);
        void sendRequestKeepAlive(const GameListing * game);
		enum FriendsBlocksMsgType { fbm_addFriend, fbm_removeFriend, fbm_addBlock, fbm_removeBlock };
		char * sendFriendsBlocksMsg(int * size, void * p, enum FriendsBlocksMsgType f);
        void sendMatchInvite(const PlayerListing * player, bool accepting);
        void sendDeclineMatchInvite(const PlayerListing * player);
        void sendDeclineMatchOffer(const PlayerListing * player);
        void sendMatchOfferPending(const PlayerListing * player);
        void sendMatchOfferCancel(const PlayerListing * player);
		void sendMatchOffer(const MatchRequest & mr, bool offercounteroffer = false);
		void sendInvitationSettings(bool invite);
		void sendUndo(unsigned int game_code, const MoveRecord * move);
		void sendDeclineUndo(unsigned int game_code, const MoveRecord * move);
		void sendAcceptUndo(unsigned int game_code, const MoveRecord * move);
		void sendRemoveStones(unsigned int game_code, const MoveRecord * move);
		void sendEndgame12Msg(unsigned int game_id, unsigned short msg_code);
		void sendEnterScoring(unsigned int game_code);
		void sendDoneScoring(unsigned int game_id, unsigned short opp_id);
		void sendResign(unsigned int game_code);
		void sendMatchResult(unsigned short game_code);
		void sendNigiri(unsigned short game_code, bool odd);
		void sendDisconnectMsg(void);
		void sendChallengeResponse(void);
		void encode(unsigned char * h, unsigned int cycle_size);
		void handleMessage(QString msg);
		void handleMessage(unsigned char * msg, unsigned int size);
		void handleCodeTable(unsigned char * msg, unsigned int size);
		void handleConnected(unsigned char * msg, unsigned int size);
		void handlePlayerList(unsigned char * msg, unsigned int size);
		QString rating_pointsToRank(unsigned int rp);
		QString getCountryFromCode(unsigned char code);
		void handleRoomList(unsigned char * msg, unsigned int size);
		void handleBroadcastGamesList(unsigned char * msg, unsigned int size);
		int getPhase(unsigned char byte);
		QString getRoomTag(unsigned char byte);
		void handleGamesList(unsigned char * msg, unsigned int size);
		void handlePlayerConnect(unsigned char * msg, unsigned int size);
		void handlePlayerRoomJoin(unsigned char * msg, unsigned int size);
		void removeObserverFromGameListing(const PlayerListing * p);
		void handlePlayerDisconnect2(unsigned char * msg, unsigned int size);
		void handleServerAnnouncement(unsigned char * msg, unsigned int size);
		void handleServerAnnouncementwithLink(unsigned char * msg, unsigned int size);
		void handleServerRoomChat(unsigned char * msg, unsigned int size);
		void handlePersonalChat(unsigned char * msg, unsigned int size);
		void handleRoomChat(unsigned char * msg, unsigned int size);
		void handleSetPhraseChatMsg(unsigned char * msg, unsigned int size);
		void handleNewGame(unsigned char * msg, unsigned int size);
		void handleGameEnded(unsigned char * msg, unsigned int size);
		void handleNewRoom(unsigned char * msg, unsigned int size);
		void handleGameMsg(unsigned char * msg, unsigned int size);
		void handleBettingMatchStart(unsigned char * msg, unsigned int size);
		void handleBettingMatchResult(unsigned char * msg, unsigned int size);
		void handleRequestCount(unsigned char * msg, unsigned int size);
		void handleAcceptCountRequest(unsigned char * msg, unsigned int size);
		void handleRejectCountRequest(unsigned char * msg, unsigned int size);
		void handleRequestMatchMode(unsigned char * msg, unsigned int size);
		void handleRejectMatchModeRequest(unsigned char * msg, unsigned int size);
		void handleTimeLoss(unsigned char * msg, unsigned int size);
		void handleMove(unsigned char * msg, unsigned int size);
		void handleUndo(unsigned char * msg, unsigned int size);
		void handleDeclineUndo(unsigned char * msg, unsigned int size);
		void handleAcceptUndo(unsigned char * msg, unsigned int size);
		void handleMoveList(unsigned char * msg, unsigned int size);
		void handleMoveList2(unsigned char * msg, unsigned int size);
		void handleObserverList(unsigned char * msg, unsigned int size);
		void handleMatchOpened(unsigned char * msg, unsigned int size);
		void addObserverListToBoard(BoardDispatch * boarddispatch);
		void handleResumeMatch(unsigned char * msg, unsigned int size);
		void handleObserveAfterJoining(unsigned char * msg, unsigned int size);
		void handleInvitationSettings(unsigned char * msg, unsigned int size);
		QString getStatusFromCode(unsigned char code, QString rank);
		int compareRanks(QString rankA, QString rankB);
		void handleCreateRoom(unsigned char * msg, unsigned int size);
		void handleAdjournRequest(unsigned char * msg, unsigned int size);
		void handleAdjournDecline(unsigned char * msg, unsigned int size);
		void handleAdjourn(unsigned char * msg, unsigned int size);
		void handleMatchInvite(unsigned char * msg, unsigned int size);
		void handleMatchOffer(unsigned char * msg, unsigned int size);
		void handleRematchRequest(unsigned char * msg, unsigned int size);
		void handleRematchAccept(unsigned char * msg, unsigned int size);
		void handleMatchDecline(unsigned char * msg, unsigned int size);
		void handleAcceptMatchInvite(unsigned char * msg, unsigned int size);
		void handleDeclineMatchInvite(unsigned char * msg, unsigned int size);
		void handleMatchOfferPending(unsigned char * msg, unsigned int size);
		void handleResign(unsigned char * msg, unsigned int size);
		void handleEnterScoring(unsigned char * msg, unsigned int size);
		void handleRemoveStones(unsigned char * msg, unsigned int size);
		void handleStonesDone(unsigned char * msg, unsigned int size);
		void handleScore(unsigned char * msg, unsigned int size);
		void handleGamePhaseUpdate(unsigned char * msg, unsigned int size);
		void handleMsg3(unsigned char * msg, unsigned int size);
		void handleFriends(unsigned char * msg, unsigned int size);
		void handleMsg2(unsigned char * msg, unsigned int size);
		void handleNigiri(unsigned char * msg, unsigned int size);
		int time_to_seconds(const QString & time);
		void killActiveMatchTimers(void);
		void startMatchTimers(bool ourTurn);
		void setRoomNumber(unsigned short number);
		void setAttachedGame(PlayerListing * const player, unsigned short game_id);
		
		/* We should really just have the player listing for ourself FIXME */
		unsigned short our_player_id;
		unsigned char our_special_byte;
		QTextCodec * textCodec;
		QTextCodec * serverCodec;
		class SetPhrasePalette * setphrasepalette;
		/* FIXME */
		std::vector <class ServerItem *> serverList;
		int current_server_index;
		unsigned char * challenge_response;
		unsigned char * codetable;
		unsigned int codetable_IV, codetable_IV2;
		
		QuickConnection * metaserverQC;
		
		/* Since 0a7d comes before 1a81 and one has the number for human
		 * consumption and the other the game_code necessary for joining,
		 * etc., we'll just combine them both at 1a81, checking names */
		std::vector <GameListing *> rooms_without_owners;
		std::map <unsigned short, unsigned short> game_code_to_number;
		unsigned short playerlist_skipnumber;
		unsigned short playerlist_received;
		unsigned short playerlist_roomnumber;
		unsigned short playerlist_observernumber;
		unsigned short roomlist_observers;
		std::vector <PlayerListing *> playerlist_inorder;
		
		unsigned short room_were_in;
		unsigned short connecting_to_game_number;	//awkward FIXME
		
		bool we_send_nigiri;
		
		
		int matchKeepAliveTimerID, matchRequestKeepAliveTimerID;
		std::vector <MoveRecord> deadStonesList;
		
	private slots:
		virtual void OnConnected();
};
