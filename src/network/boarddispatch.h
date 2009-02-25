#include "networkconnection.h"

class BoardDispatch
{
	public:
		BoardDispatch(NetworkConnection * conn, class GameListing * l);
		~BoardDispatch();
		void closeBoard(void);
		void recvMove(class MoveRecord * m);
		void sendMove(class MoveRecord * m);
		void sendTimeLoss(void);
		void gameDataChanged(void);
		void openBoard(void);
		void recvTime(const class TimeRecord & wt, const class TimeRecord & bt);
		void recvResult(class GameResult * r);
		void recvObserver(class PlayerListing * p, bool present);
		void clearObservers(void);
		void recvKibitz(QString name, QString text);
		void sendKibitz(QString text);
		void recvEnterScoreMode(void);
		void createCountDialog(void);
		void recvRequestCount(void);
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
		void startGame(void);
		bool isAttribBoard(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi);
		bool isOpponentBoard(QString us, QString them);
		void swapColors(bool noswap = false);
		
		void requestGameInfo(void);
		class GameData * getGameData(void);
		class TimeRecord getOurTimeRecord(void);
		class TimeRecord getTheirTimeRecord(void);
		QString getOpponentName(void);
		QString getUsername(void) { return connection->getUsername(); };
		bool supportsMultipleUndo(void) { return connection->supportsMultipleUndo(); };
		bool supportsRematch(void);
		bool startTimerOnOpen(void) {return connection->startTimerOnOpen(); };
		bool clientCountsTime(void) { return connection->clientCountsTime(); };
		bool clientSendsTime(void) { return connection->clientSendsTime(); };
		bool twoPassesEndsGame(void) { return connection->twoPassesEndsGame(); };
		bool unmarkUnmarksAllDeadStones(void) { return connection->unmarkUnmarksAllDeadStones(); };
		bool cantMarkOppStonesDead(void) { return connection->cantMarkOppStonesDead(); };
	private:
		void mergeListingIntoRecord(class GameData * r, class GameListing * l);
		class MainWindow * mainwindow;
		class BoardWindow * boardwindow;
		NetworkConnection * connection;
		class GameData * gameData;
		class GameListing * gameListing;
		class ResultDialog * resultdialog;
		class CountDialog * countdialog;
};
