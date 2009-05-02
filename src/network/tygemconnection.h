#ifndef TYGEMCONNECTION_H
#define TYGEMCONNECTION_H
#include <QtNetwork>
#include "networkconnection.h"
#include "messages.h"
#include "newline_pipe.h"
#include <vector>

class BoardDispatch;
class GameDialogDispatch;
class TalkDispatch;

class TygemConnection : public NetworkConnection
{
	public:
		TygemConnection(const QString & user, const QString & pass, ConnectionType = TypeTYGEM);
		~TygemConnection();
		/* Protocols are way similar so probably most of these don't have to be
		 * virtual FIXME */
		virtual void sendText(QString text);
		virtual void sendText(const char * text);
		virtual void sendDisconnect(void);
		virtual void sendMsg(unsigned int game_id, QString text);
		virtual void sendMsg(const PlayerListing & player, QString text);
		virtual void sendToggle(const QString & param, bool val);
		virtual void sendObserve(const GameListing & game);
		virtual void stopObserving(const GameListing & game);
		virtual void stopReviewing(const GameListing & game);
		virtual void sendStatsRequest(const PlayerListing &) {};
		virtual void sendPlayersRequest(void);
		virtual void sendGamesRequest(void) {};
		virtual void sendMatchInvite(const PlayerListing & opponent);
		virtual void adjournGame(const GameListing & game);
		virtual void sendTime(BoardDispatch * boarddispatch);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		
		//EndgameMsg should probably be protected or even private
		enum EGVersion {pass, done_scoring, accept_count, reject_count};
		virtual void sendEndgameMsg(const class GameData * game, enum EGVersion version);
		virtual void sendStopTime(BoardDispatch * boarddispatch, enum EGVersion version);
		
		virtual void sendTimeLoss(unsigned int game_id);
		virtual void sendResult(class GameData * game, class GameResult * result);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void sendRematchRequest(void);
		virtual void sendRematchAccept(void);
		virtual void declineMatchOffer(const PlayerListing & opponent);
		virtual void cancelMatchOffer(const PlayerListing & opponent);
		virtual void acceptMatchOffer(const PlayerListing & opponent, class MatchRequest * mr);
		virtual void sendRejectCount(class GameData * data);
		virtual void sendAcceptCount(class GameData * data);
		virtual void sendAdjournRequest(void);
		virtual void sendAdjourn(void);
		virtual void sendRefuseAdjourn(void);
		virtual void handlePendingData(newline_pipe <unsigned char> * p);
		virtual bool isReady(void);
		virtual void onReady(void);
		virtual void changeServer(void);
		virtual void createRoom(void);
		virtual void sendCreateRoom(class RoomCreate * room);
		virtual void sendJoinRoom(const RoomListing & room, const char * password = 0);
		
		virtual void addFriend(PlayerListing & player);
		virtual void removeFriend(PlayerListing & player);
		virtual void addFan(PlayerListing & player);
		virtual void removeFan(PlayerListing & player);
		virtual void addBlock(PlayerListing & player);
		virtual void removeBlock(PlayerListing & player);

		//FIXME these are unused right now:
		virtual void requestLongInfo(PlayerListing & player);
		virtual void requestShortInfo(PlayerListing & player);

		BoardDispatch * getBoardFromAttrib(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi);
		BoardDispatch * getBoardFromOurOpponent(QString opponent);
		virtual const PlayerListing & getOurListing(void);
		virtual unsigned short getRoomNumber(void) { return room_were_in; };
		
		virtual void closeBoardDispatch(unsigned int game_id);
		
		virtual void requestGameInfo(unsigned int game_id);
		virtual void requestGameStats(unsigned int game_id);
		virtual unsigned int rankToScore(QString rank);
		virtual unsigned long getGameDialogFlags(void);	
		virtual bool supportsMultipleUndo(void) { return false; };	//for now
		virtual bool supportsObserveOutside(void) { return false; };
		virtual bool clientCountsTime(void) { return false; };
		virtual bool clientSendsTime(void) { return true; };
		virtual bool unmarkUnmarksAllDeadStones(void) { return true; };
		virtual bool cantMarkOppStonesDead(void) { return true; };
		virtual bool twoPassesEndsGame(void) { return true; };
		virtual bool supportsServerChange(void) { return true; };
		virtual bool supportsRematch(void) { return true; };
		virtual bool supportsFriendList(void) { return true; };
		virtual bool supportsBlockList(void) { return true; };
		virtual unsigned long getPlayerListColumns(void) { return PL_NOMATCHPREFS; };
		virtual bool supportsCreateRoom(void) { return true; };
		virtual unsigned long getRoomStructureFlags(void) { return (RS_NOROOMLIST | RS_ONEROOMATATIME | RS_ONEGAMEPERROOM); };
		virtual void timerEvent(QTimerEvent * event);
	protected:
		virtual int requestServerInfo(void);
		virtual void handleServerInfo(unsigned char * msg, unsigned int length);
		int reconnectToServer(void);
		//FIXME secondsToDate obviously should not be here
		void secondsToDate(unsigned short & year, unsigned char & month, unsigned char & day, unsigned char & hour, unsigned char & minute, unsigned char & second);
		virtual QString getTygemGameRecordQString(class GameData * game);
		enum TimeFlags { BLACK_LOSES, WHITE_LOSES, GAME_ON };
		enum TimeFlags handleTimeChunk(BoardDispatch * boarddispatch, unsigned char chunk[8], bool black_first);
		void fillTimeChunk(BoardDispatch * boarddispatch, unsigned char * packet);
		/* FIXME */
		QTextCodec * serverCodec;
		std::vector <class ServerItem *> serverList;
		int current_server_index;
		
		ConnectionType connectionType;

	private:
		void sendObserve(unsigned short game_number);	//FIXME awkward with other sendObserve
		void sendMatchMsg1(const PlayerListing & player, unsigned short game_number);
		void sendLogin(bool response_bit = false, bool change_server = false);
		void handleLogin(unsigned char * msg, unsigned int length);
		void sendLoginConfirm(void);
		void sendName(void);
		void sendRequest(void);
		void sendFriendsBlocksRequest(void);
		void handleServerList(unsigned char * msg);
		void sendPersonalChat(const PlayerListing & player, const char * text);
		void sendServerChat(QString text);
		void sendJoin(unsigned short game_number);
		void sendResume(unsigned short game_number);
		void sendFinishObserving(unsigned short game_number);
		void sendObserversRequest(unsigned short game_number);
		void sendLeave(const GameListing & game);
		void sendGameUpdate(unsigned short game_code);
		void sendServerKeepAlive(void);
		void sendKeepAlive(const GameListing & game);
		enum MIVersion {offer, accept, decline, create, acknowledge};
		void sendMatchInvite(const PlayerListing & player, enum MIVersion version = offer, unsigned short room_number = 0xffff);
		void sendMatchOffer(const MatchRequest & mr, enum MIVersion version = offer);
		void sendStartGame(const MatchRequest & mr);
		void sendInvitationSettings(bool invite);
		void sendUndo(unsigned int game_code, const MoveRecord * move);
		void sendDeclineUndo(unsigned int game_code, const MoveRecord * move);
		void sendAcceptUndo(unsigned int game_code, const MoveRecord * move);
		void sendRemoveStones(unsigned int game_code, const MoveRecord * move);
		void sendEnterScoring(unsigned int game_code);
		void sendDoneScoring(unsigned int game_id, unsigned short opp_id);
		void sendCountMsg(const class GameData * r, enum MIVersion version);
		void sendOpponentDisconnectTimer(unsigned int game_number);
		void sendResign(unsigned int game_code);
		void sendMatchResult(unsigned short game_code);
		void sendLongMatchResult(unsigned short game_code);
		void sendNigiri(unsigned short game_code, bool odd);
		void sendDisconnectMsg(void);
		void encode(unsigned char * h, unsigned int cycle_size);
		void handlePassword(QString msg);
		void handleMessage(QString msg);
		void handleMessage(unsigned char * msg, unsigned int size);
		void handleConnected(unsigned char * msg, unsigned int size);
		void handlePlayerList(unsigned char * msg, unsigned int size);
		void handleFriendsBlocksList(unsigned char * msg, unsigned int size);
		void handleList(unsigned char * msg, unsigned int size);
		QString rating_pointsToRank(unsigned int rp);
		QString getCountryFromCode(unsigned char code);
		int getPhase(unsigned char byte);
		QString getRoomTag(unsigned char byte);
		void handleGamesList(unsigned char * msg, unsigned int size);
		void handlePlayerRoomJoin(unsigned char * msg, unsigned int size);
		void handlePlayerDisconnect2(unsigned char * msg, unsigned int size);
		void handleServerAnnouncement(unsigned char * msg, unsigned int size);
		void handleServerRoomChat(unsigned char * msg, unsigned int size);
		void handlePersonalChat(unsigned char * msg, unsigned int size);
		void handleGameChat(unsigned char * msg, unsigned int size);
		void handleSetPhraseChatMsg(unsigned char * msg, unsigned int size);
		void handleNewGame(unsigned char * msg, unsigned int size);
		void handleBettingMatchStart(unsigned char * msg, unsigned int size);
		void handleBettingMatchResult(unsigned char * msg, unsigned int size);
		void handleTime(unsigned char * msg, unsigned int size);
		void handleMove(unsigned char * msg, unsigned int size);
		void handlePass(unsigned char * msg, unsigned int size, int pass_number);
		void handleUndo(unsigned char * msg, unsigned int size);
		void handleDeclineUndo(unsigned char * msg, unsigned int size);
		void handleAcceptUndo(unsigned char * msg, unsigned int size);
		void handleMoveList(unsigned char * msg, unsigned int size);
		void handleMoveList2(unsigned char * msg, unsigned int size);
		void handleObserverList(unsigned char * msg, unsigned int size);
		void handleBoardOpened(unsigned char * msg, unsigned int size);
		void handleMatchOpened(unsigned char * msg, unsigned int size);
		void handleResumeMatch(unsigned char * msg, unsigned int size);
		void handleInvitationSettings(unsigned char * msg, unsigned int size);
		QString getStatusFromCode(unsigned char code, QString rank);
		int compareRanks(QString rankA, QString rankB);
		void handleCreateRoom(unsigned char * msg, unsigned int size);
		void handleAdjournRequest(unsigned char * msg, unsigned int size);
		void handleAdjournDecline(unsigned char * msg, unsigned int size);
		void handleAdjourn(unsigned char * msg, unsigned int size);
		void handleMatchInvite(unsigned char * msg, unsigned int size);
		void handleMatchOffer(unsigned char * msg, unsigned int size, enum MIVersion = offer);
		void handleRematchRequest(unsigned char * msg, unsigned int size);
		void handleRematchAccept(unsigned char * msg, unsigned int size);
		void handleMatchDecline(unsigned char * msg, unsigned int size);
		void sendCreateRoom(void);
		void handleMatchInviteResponse(unsigned char * msg, unsigned int size);
		void handleCreateRoomResponse(unsigned char * msg, unsigned int size);
		void handleMatchRoomOpen(unsigned char * msg, unsigned int size);
		//FIXME
		void handleScoreMsg1(unsigned char * msg, unsigned int size);
		void handleEndgameMsg(unsigned char * msg, unsigned int size);
		void handleRequestCount(unsigned char * msg, unsigned int size);
		void handleEnterScoring(unsigned char * msg, unsigned int size);
		void handleRemoveStones(unsigned char * msg, unsigned int size);
		void handleGameResult(unsigned char * msg, unsigned int size);
		void handleStonesDone(unsigned char * msg, unsigned int size);
		void handleGamePhaseUpdate(unsigned char * msg, unsigned int size);
		void handleMsg3(unsigned char * msg, unsigned int size);
		void handleMsg2(unsigned char * msg, unsigned int size);
		void handleNigiri(unsigned char * msg, unsigned int size);
		class GameData * getGameData(unsigned int game_id);
		int time_to_seconds(const QString & time);
		void killActiveMatchTimers(void);
		void startMatchTimers(bool ourTurn);
		void setRoomNumber(unsigned short number);
		void setAttachedGame(PlayerListing * const player, unsigned short game_id);
		
		/* We should really just have the player listing for ourself FIXME */
		unsigned short our_player_id;
		unsigned char our_special_byte;
		unsigned char invite_byte;
		bool opponent_is_challenger;
		bool previous_opponent_move_pass;
		int move_message_number;	//FIXME can't use if more than one game
		PlayerListing * player_accepted_match;
		QTextCodec * textCodec;
		unsigned long encode_offset;
		
		/* Since 0a7d comes before 1a81 and one has the number for human
		 * consumption and the other the game_code necessary for joinging,
		 * etc., we'll just combine them both at 1a81, checking names */
		std::vector <GameListing *> rooms_without_games;
		std::vector <GameListing *> rooms_without_owners;
		std::map <unsigned short, unsigned short> game_code_to_number;
		
		unsigned short room_were_in;
		unsigned short connecting_to_game_number;	//awkward FIXME
		unsigned short playing_game_number;		//awkward
		bool we_send_nigiri;
		
		/* We can only play one game at a time so:...*/
		unsigned short our_game_being_played;	//redundant with playing_game_number??
		int matchKeepAliveTimerID, matchRequestKeepAliveTimerID;
		int serverKeepAliveTimerID;
		int retryLoginTimerID;
		int opponentDisconnectTimerID;
		//unsigned short opp_requests_undo_move_number;
		unsigned short done_response;
		std::vector <MoveRecord> deadStonesList;
		/* FIXME, below are awkward, we should just do some kind of state thing */
		bool receivedOppDone;
		bool sentDone;
		bool receivedOppAccept;
		bool receivedOppReject;
		
	private slots:
		virtual void OnConnected();
};
#endif //TYGEMCONNECTION_H
