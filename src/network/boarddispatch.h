#include "networkdispatch.h"

class BoardDispatch : public NetworkDispatch
{
	public:
		BoardDispatch(class GameListing * l);
		~BoardDispatch();
		void closeBoard(void);
		void recvMove(class MoveRecord * m);
		void sendMove(class MoveRecord * m);
		void sendTimeLoss(void);
		void recvRecord(class GameData * g);
		void recvTime(const class TimeRecord & wt, const class TimeRecord & bt);
		void recvResult(class GameResult * r);
		void recvObserver(class PlayerListing * p, bool present);
		void clearObservers(void);
		void recvKibitz(QString name, QString text);
		void sendKibitz(QString text);
		void recvEnterScoreMode(void);
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
		bool isAttribBoard(QString black_player, unsigned int black_captures, float black_komi, QString white_player, unsigned int white_captures, float white_komi);
		bool isOpponentBoard(QString us, QString them);
		void swapColors(bool noswap = false);
		
		void requestGameInfo(void);
		class GameData * getGameData(void);
		class TimeRecord getOurTimeRecord(void);
		QString getOpponentName(void);
		bool supportsMultipleUndo(void) { if(connection) return connection->supportsMultipleUndo(); return false; };
		bool supportsRematch(void);
		bool startTimerOnOpen(void) { if(connection) return connection->startTimerOnOpen(); else return false; };
	private:
		void mergeListingIntoRecord(class GameData * r, class GameListing * l);
		class MainWindow * mainwindow;
		class BoardWindow * boardwindow;
		NetworkDispatch * networkdispatch;
		class GameData * gameData;
		class GameListing * gameListing;
		class ResultDialog * resultdialog;
		GameMode gameMode;
};
