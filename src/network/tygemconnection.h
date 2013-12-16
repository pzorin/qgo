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


#ifndef TYGEMCONNECTION_H
#define TYGEMCONNECTION_H
#include <QtNetwork>
#include "networkconnection.h"
#include "messages.h"
#include <vector>
#include <stdint.h>

class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;

class TygemConnection : public NetworkConnection
{
	public:
        TygemConnection(const ConnectionCredentials credentials);
		~TygemConnection();
		/* Protocols are way similar so probably most of these don't have to be
		 * virtual FIXME */
		virtual void sendText(QString);
		virtual void sendText(const char * text);
		virtual void sendDisconnect(void);
		virtual void sendMsg(unsigned int game_id, QString text);
        virtual void sendMsg(PlayerListing * player, QString text);
		virtual void sendToggle(const QString & param, bool val);
        virtual void sendObserve(const GameListing * game);
        virtual void sendStatsRequest(const PlayerListing *) {}
        virtual void sendPlayersRequest(void) {}
        virtual void sendGamesRequest(void) {}
		
        virtual void sendMatchInvite(const PlayerListing * opponent);
		virtual void sendTime(BoardDispatch * boarddispatch);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		
		//EndgameMsg should probably be protected or even private
		enum EGVersion {pass, done_scoring, accept_count, reject_count,
				request_count, accept_count_request, refuse_count_request,
				request_draw, accept_draw_request, refuse_draw_request,
    				request_undo, accept_undo_request, refuse_undo_request,
				opponent_disconnect_timer, we_resume, opponent_reconnects};
		virtual void sendEndgameMsg(const class GameData * game, enum EGVersion version);
		virtual void sendStopTime(BoardDispatch * boarddispatch, enum EGVersion version);
		
		virtual void sendResult(class GameData * game, class GameResult * result);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void sendRematchRequest(void);
		virtual void sendRematchAccept(void);
        virtual void declineMatchOffer(const PlayerListing * opponent);
        virtual void cancelMatchOffer(const PlayerListing * opponent);
        virtual void acceptMatchOffer(const PlayerListing * opponent, class MatchRequest * mr);
		virtual void sendRequestCount(unsigned int game_id);
		virtual void sendAcceptCountRequest(class GameData * data);
		virtual void sendRefuseCountRequest(class GameData * data);
		virtual void sendRequestDraw(unsigned int game_id);
		virtual void sendAcceptDrawRequest(class GameData * data);
		virtual void sendRefuseDrawRequest(class GameData * data);
		virtual void sendRejectCount(class GameData * data);
		virtual void sendAcceptCount(class GameData * data);
		virtual void handlePendingData();
        virtual void onReady(void);
		virtual void changeServer(void);
		virtual void createRoom(void);
		virtual void sendCreateRoom(class RoomCreate * room);
		virtual void sendJoinRoom(const RoomListing & room, const char * password = 0);
		
        virtual void addFriend(PlayerListing * player);
        virtual void removeFriend(PlayerListing * player);
        virtual void addBlock(PlayerListing * player);
        virtual void removeBlock(PlayerListing * player);

		//FIXME these are unused right now:
        virtual void requestLongInfo(PlayerListing * player);
        virtual void requestShortInfo(PlayerListing * player);

		virtual unsigned short getRoomNumber(void); //FIXME for gamedialog unsure of necessity or reliability here
		
        virtual void closeTalk(PlayerListing *opponent);
		virtual void closeBoardDispatch(unsigned int game_id);
		
		virtual int gd_verifyBoardSize(int v);
		virtual QTime gd_checkMainTime(TimeSystem s, const QTime & t);
		virtual QTime gd_checkPeriodTime(TimeSystem s, const QTime & t);
		virtual unsigned int gd_checkPeriods(TimeSystem s, unsigned int p);
		
		virtual unsigned int rankToScore(QString rank);
		virtual unsigned long getGameDialogFlags(void);	
        virtual bool supportsMultipleUndo(void) { return false; }
        virtual bool supportsObserveOutside(void) { return false; }
        virtual bool supportsRequestCount(void) { return false; }
        virtual bool supportsRequestDraw(void) { return true; }
        virtual bool clientCountsTime(void) { return false; }
        virtual bool clientSendsTime(void) { return true; }
        virtual bool unmarkUnmarksAllDeadStones(void) { return true; }
        virtual bool canMarkStonesDeadinScore(void) { return false; }
        virtual bool cantMarkOppStonesDead(void) { return true; }
        virtual bool twoPassesEndsGame(void) { return true; }
        virtual bool supportsServerChange(void) { return true; }
        virtual bool supportsRematch(void) { return true; }
        virtual bool supportsFriendList(void) { return true; }
        virtual bool supportsBlockList(void) { return true; }
        virtual unsigned long getPlayerListColumns(void) { return PL_NOMATCHPREFS; }
        virtual bool supportsCreateRoom(void) { return true; }
        virtual unsigned long getRoomStructureFlags(void) { return (RS_NOROOMLIST | RS_ONEROOMATATIME | RS_ONEGAMEPERROOM); }
		virtual const char * getCodecString(void);
		virtual QString getPlaceString(void);
		virtual void timerEvent(QTimerEvent * event);
	protected:
		virtual int requestServerInfo(void);
		/* this is called handleServerList in ORO, not sure why we changed
		 * the name minor FIXME */
		virtual void handleServerInfo(unsigned char * msg, unsigned int length);
		int reconnectToServer(void);
		virtual void handleGamesList(unsigned char * msg, unsigned int size);
		
		//FIXME secondsToDate obviously should not be here
		void secondsToDate(unsigned short & year, unsigned char & month, unsigned char & day, unsigned char & hour, unsigned char & minute, unsigned char & second);
		virtual QByteArray getTygemGameRecordQByteArray(class GameData * game);
		QByteArray getTygemGameTagQByteArray(GameData * game, unsigned char white_level, unsigned char black_level, int margin);
		unsigned char getVictoryCode(GameResult & aGameResult);
		enum TimeFlags { BLACK_LOSES, WHITE_LOSES, GAME_ON };
		enum TimeFlags handleTimeChunk(BoardDispatch * boarddispatch, unsigned char chunk[8], bool black_first);
		void fillTimeChunk(BoardDispatch * boarddispatch, unsigned char * packet);
		/* FIXME */
		QTextCodec * serverCodec;
		std::vector <class ServerItem *> serverList;
		int current_server_index;
		
		int move_message_number;	//FIXME can't use if more than one game
		int move_message_number_ack;
		int seconds_until_opp_forfeits;
	private:
		void sendJoin(unsigned short game_number);
        void sendMatchMsg1(const PlayerListing * player, unsigned short game_number);
		void sendLogin(bool response_bit = false, bool change_server = false);
		void handleLogin(unsigned char * msg, unsigned int length);
		void sendRequestGames(void);
		void sendRequestPlayers(void);
		void sendName(void);
		void sendRequest(void);
		void sendRequest2(void);
		void sendFriendsBlocksRequest(void);
		void promptResumeMatch(void);
        void sendOpenConversation(PlayerListing * player);
		enum MIVersion {offer, accept, decline, decline_all, modify, create, acknowledge, alreadyingame};
        void sendConversationReply(PlayerListing * player, enum MIVersion version);
        void sendConversationMsg(PlayerListing * player, const char * text);
        void sendCloseConversation(PlayerListing * player);
        void sendPersonalChat(const PlayerListing * player, const char * text);
		void sendServerChat(QString text);
		void sendResume(unsigned short game_number);
		void sendFinishObserving(unsigned short game_number);
		void sendObserversRequest(unsigned short game_number);
		void sendServerKeepAlive(uint32_t keepaliveIV);
		uint32_t encodeKeepAliveIV(uint32_t a);
        void sendMatchInvite(const PlayerListing * player, enum MIVersion version = offer, unsigned short room_number = 0xffff);
		void sendMatchOffer(const MatchRequest & mr, enum MIVersion version = offer);
		void sendStartGame(const MatchRequest & mr);
		void sendInvitationSettings(bool invite);
		void sendResumeFromDisconnect(unsigned int game_number);
		enum OpponentDisconnectUpdate { opponent_disconnect, opponent_reconnect };
		void sendOpponentDisconnect(unsigned int game_number, enum OpponentDisconnectUpdate opp);
		void sendLongOpponentDisconnect(unsigned int game_number);
		void sendOpponentDisconnectTimer(unsigned int game_number);
		void sendMatchResult(unsigned short game_code);
		void sendLongMatchResult(unsigned short game_code);
		void sendDisconnectMsg(void);
		void encode(unsigned char * h, unsigned int cycle_size);
		void handleMessage(QString msg);
		void handleMessage(unsigned char * msg, unsigned int size);
		void handleConnected(unsigned char * msg, unsigned int size);
		void handlePlayerList(unsigned char * msg, unsigned int size);
		void handleFriendsBlocksList(unsigned char * msg, unsigned int size);
		void handleServerPlayerCounts(unsigned char * msg, unsigned int size);
		QString rating_pointsToRank(unsigned int rp);
		QString getCountryFromCode(unsigned char code);
		int getPhase(unsigned char byte);
		QString getRoomTag(unsigned char byte);

        void writeNotNicknameAndRank(char * p, const PlayerListing * player);
        void writeNicknameAndCID(char * p, const PlayerListing * player);
		PlayerListing * getOrCreatePlayerListingFromRecord(char * r);

		void handleServerAnnouncement(unsigned char * msg, unsigned int size);
		void handleRequestKeepAlive(unsigned char * msg, unsigned int size);
		void handleServerRoomChat(unsigned char * msg, unsigned int size);
		void handlePersonalChat(unsigned char * msg, unsigned int size);
		void handleOpenConversation(unsigned char * msg, unsigned int size);
		void handleConversationReply(unsigned char * msg, unsigned int size);
		void handleConversationMsg(unsigned char * msg, unsigned int size);
		void handleGameChat(unsigned char * msg, unsigned int size);
		void handleBettingMatchStart(unsigned char * msg, unsigned int size);
		void handleTime(unsigned char * msg, unsigned int size);
		void handleMove(unsigned char * msg, unsigned int size);
		void handlePass(unsigned char * msg, unsigned int size, int pass_number);
		void handleObserverList(unsigned char * msg, unsigned int size);
		void handleBoardOpened(unsigned char * msg, unsigned int size);
		void handleMatchOpened(unsigned char * msg, unsigned int size);
		void handleResumeMatch(unsigned char * msg, unsigned int size);
		QString getStatusFromCode(unsigned char code, QString rank);
		int compareRanks(QString rankA, QString rankB);
		void handleMatchInvite(unsigned char * msg, unsigned int size);
		void handleMatchOffer(unsigned char * msg, unsigned int size, enum MIVersion = offer);
		void sendCreateRoom(void);
		void handleMatchInviteResponse(unsigned char * msg, unsigned int size);
		void handleCreateRoomResponse(unsigned char * msg, unsigned int size);
		//FIXME
		void handleScoreMsg1(unsigned char * msg, unsigned int size);
		void handleEndgameMsg(unsigned char * msg, unsigned int size);
		void handleGameStateChange(unsigned char * msg, unsigned int size);
		void handleCountRequest(unsigned char * msg, unsigned int size);
		void handleCountRequestResponse(unsigned char * msg, unsigned int size);
		void handleGameResult(unsigned char * msg, unsigned int size);
		void handleGameResult2(unsigned char * msg, unsigned int size);
		void handleMsg2(unsigned char * msg, unsigned int size);
		int time_to_seconds(const QString & time);
		
		unsigned int http_connect_content_length;
        bool http_response_checked;
		/* We should really just have the player listing for ourself FIXME */
		unsigned short our_player_id;
		bool opponent_is_challenger;
		bool havenot_requested_friendsblocks;

		std::vector<PlayerListing *> decline_all_invitations;
		QTextCodec * textCodec;
		unsigned long encode_offset;
		
		std::map <PlayerListing *, QString> pendingConversationMsg;
		
		int matchKeepAliveTimerID, matchRequestKeepAliveTimerID;
		int retryLoginTimerID;
		int opponentDisconnectTimerID;
		
		bool received_players;
		bool received_games;
		bool initial_connect;
		bool dont_validate_maintime, dont_validate_periodtime, dont_validate_periods;
		
	private slots:
		virtual void OnConnected();
};
#endif //TYGEMCONNECTION_H
