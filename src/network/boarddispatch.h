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


class NetworkConnection;
class QString;

class BoardDispatch
{
	public:
		BoardDispatch(NetworkConnection * conn, class GameListing * l);
		~BoardDispatch();
		bool canClose(void);
		void closeBoard(void);
        void setConnection(NetworkConnection * conn);
		void recvMove(class MoveRecord * m);
		void sendMove(class MoveRecord * m);
		void sendRequestMatchMode();
		bool isClockStopped(void) { return clockStopped; };
		void stopTime(void) { clockStopped = true; };
		void startTime(void) { clockStopped = false; };
		void moveControl(QString & player);
		void sendTimeLoss(void);
		void gameDataChanged(void);
		void openBoard(void);
		void recvTime(const class TimeRecord & wt, const class TimeRecord & bt);
		void recvAddTime(int minutes, QString player_name);
		void recvResult(class GameResult * r);
		void saveRecordToGameData(void);
		void recvObserver(class PlayerListing * p, bool present);
		void clearObservers(void);
		void recvKibitz(QString name, QString text);
		void sendKibitz(QString text);
		void recvEnterScoreMode(void);
		void recvLeaveScoreMode(void);
		void createCountDialog(void);
		void sendRequestCount(void);
		void recvRequestCount(void);
		void recvAcceptCountRequest(void);
		void recvRejectCountRequest(void);
		void sendAcceptCountRequest(void);
		void sendRefuseCountRequest(void);
		void recvAcceptDrawRequest(void);
		void recvRefuseDrawRequest(void);
		void sendRequestDraw(void);
		void recvRequestDraw(void);
		void sendAcceptDrawRequest(void);
		void sendRefuseDrawRequest(void);
		void recvRequestMatchMode(void);
		void sendAcceptMatchModeRequest(void);
		void sendRefuseMatchModeRequest(void);
		void recvRejectMatchModeRequest(void);
		void clearCountDialog(void) { countdialog = 0; };
		void recvRejectCount(void);
		void recvAcceptCount(void);
		void sendRejectCount(void);
		void sendAcceptCount(void);
		void sendResult(class GameResult * r);
		
		void recvRequestAdjourn(void);
		void sendAdjournRequest(void);
		void sendAdjourn(void);
		void sendRefuseAdjourn(void);
		void adjournGame(void);
		void recvRefuseAdjourn(void);
		void setRematchDialog(class ResultDialog * r);
		void sendRematchRequest(void);
		void sendRematchAccept(void);
		void recvRematchRequest(void);
		void sendTime(void);
		void sendAddTime(int minutes);
		void startGame(void);
		bool isAttribBoard(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi);
		bool isOpponentBoard(QString us, QString them);
		void swapColors(bool noswap = false);
		
		void requestGameInfo(void);
		int getMoveNumber(void);
		bool getReviewInVariation(void) { return reviewInVariation; };	//tygem
		void setReviewInVariation(bool b) { reviewInVariation = b; };	//tygem
		class GameData * getGameData(void);
		class TimeRecord getOurTimeRecord(void);
		class TimeRecord getTheirTimeRecord(void);
		QString getOpponentName(void);
		QString getUsername(void);
		QString getOurRank(void);
		bool getBlackTurn(void);
		class ObserverListModel * getObserverListModelForRematch(void);
		void setObserverListModel(class ObserverListModel * olm);
		bool flipCoords(void);
		bool supportsMultipleUndo(void);
		bool supportsRequestMatchMode(void);
		bool supportsAddTime(void);
		bool supportsRequestCount(void);
		bool supportsRequestDraw(void);
		bool supportsRequestAdjourn(void);
		bool supportsRematch(void);
		bool startTimerOnOpen(void);
		bool clientCountsTime(void);
		bool clientSendsTime(void);
		bool twoPassesEndsGame(void);
		bool netWillEnterScoreMode(void);
		bool undoResetsScore(void);
		bool canMarkStonesDeadinScore(void);
		bool unmarkUnmarksAllDeadStones(void);
		bool cantMarkOppStonesDead(void);
	private:
		void mergeListingIntoRecord(class GameData * r, class GameListing * l);
        class BoardWindow * boardwindow;
		NetworkConnection * connection;
		class GameData * gameData;
		class GameListing * gameListing;
		class ResultDialog * resultdialog;
		class CountDialog * countdialog;
		class ObserverListModel * observerListModel;
		
		bool clockStopped;
		bool reviewInVariation;
};
