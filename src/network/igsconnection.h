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


#ifndef IGSCONNECTION_H
#define IGSCONNECTION_H
#include <QtNetwork>
#include "networkconnection.h"
#include "messages.h"
#include "gamedata.h"

class BoardDispatch;
class GameDialog;
class Talk;

class IGSConnection : public NetworkConnection
{
	public:
        IGSConnection(const ConnectionCredentials credentials);
		~IGSConnection();
		virtual QString getPlaceString();
		virtual void sendText(QString text);
		virtual void sendText(const char * text);
		virtual void sendDisconnect(void);
		virtual void sendMsg(unsigned int game_id, QString text);
        virtual void sendMsg(PlayerListing * player, QString text);
		virtual void sendToggle(const QString & param, bool val);
        virtual void sendObserve(const GameListing * game);
        virtual void stopObserving(const GameListing * game);
        virtual void stopReviewing(const GameListing * game);
        virtual void sendStatsRequest(const PlayerListing * opponent);
		virtual void sendPlayersRequest(void);
        virtual void sendGamesRequest(void);
		void sendRoomListRequest(void);
        virtual void sendMatchInvite(const PlayerListing * player);
		virtual void sendAddTime(int);
        virtual void adjournGame(const GameListing * game);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		virtual void sendMatchRequest(class MatchRequest * mr);
        virtual void declineMatchOffer(const PlayerListing * opponent);
        virtual void acceptMatchOffer(const PlayerListing * opponent, class MatchRequest * mr);
		virtual QTime gd_checkMainTime(TimeSystem s, const QTime & t);
		virtual QTime gd_checkPeriodTime(TimeSystem s, const QTime & t);
		virtual unsigned int gd_checkPeriods(TimeSystem s, unsigned int p);
		virtual void sendAdjournRequest(void);
		virtual void sendAdjourn(void);
		virtual void sendRefuseAdjourn(void);
		virtual void handlePendingData();
        virtual void onReady(void);

		virtual void sendJoinRoom(const RoomListing & room, const char * password = 0);
		virtual void sendJoinChannel(const ChannelListing & channel);
		virtual void sendSeek(class SeekCondition *);
		virtual void sendSeekCancel(void);
		
		BoardDispatch * getBoardFromAttrib(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi);
		BoardDispatch * getBoardFromOurOpponent(QString opponent);

		virtual void requestGameInfo(unsigned int game_id);
		virtual void requestGameStats(unsigned int game_id);
		
		virtual unsigned int rankToScore(QString rank);
		/* I thought we didn't start time until after first move
		 * but I check today and we do, so I'll change it... doublecheck */
        virtual bool startTimerOnOpen(void) { return true; }
        virtual bool flipCoords(void) { return false; }
        virtual bool supportsRequestAdjourn(void) { return true; }
        virtual bool supportsAddTime(void) { return true; }
        virtual bool undoResetsScore(void) { return true; }
        virtual bool supportsSeek(void) { return true; }
        virtual unsigned long getPlayerListColumns(void) { return PL_NOWINSLOSSES; }
        virtual bool supportsChannels(void) { return true; }
        virtual bool supportsRefreshListsButtons(void) { return true; }
        virtual bool consoleIsChat(void) { return false; }
		virtual unsigned long getGameDialogFlags(void);
        virtual unsigned long getRoomStructureFlags(void) { return (RS_SHORTROOMLIST | RS_ONEROOMATATIME); }
			
	protected:
        virtual void onAuthenticationNegotiated(void);
		virtual void setKeepAlive(int);
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
		
		void handleRemovingAt(unsigned int game, QString pt);
		void sendToggleClientOn(void);
		void sendListChannels(void);

		QString element(const QString &line, int index, const QString &del1, const QString &del2="", bool killblanks=false);
		unsigned int idleTimeToSeconds(QString time);
		void fixRankString(QString * rank);
		PlayerListing * getPlayerListingNeverFail(QString & name);

		int keepAliveTimer;
		int playersListRefreshTimer;
		int gamesListRefreshTimer;
		
		QString protocol_save_string;
		int protocol_save_int;
        ChannelListing * channel;
		int game_were_playing;
		QString match_playerName;
		TimeRecord * btime, * wtime;
	
	//private:
		int time_to_seconds(const QString & time);
		
		QTextCodec * textCodec;
		
		virtual void timerEvent(QTimerEvent*);
	private:
		void init(void);
		void sendNmatchParameters(void);
        void setCurrentRoom(const RoomListing & room) { currentRoom = &room; }
        const RoomListing * getCurrentRoom(void) { return currentRoom; }
		
		bool guestAccount;
		bool needToSendClientToggle;
		const RoomListing * currentRoom;
};

#endif //IGSCONNECTION_H
