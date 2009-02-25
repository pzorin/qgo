#ifndef ROOM_H
#define ROOM_H

#include <QtGui>

class NetworkConnection;
class BoardDispatch;
class PlayerListing;
class GameListing;
class GameListingRegistry;
class PlayerListingRegistry;
class PlayerListingIDRegistry;
class Talk;
class PlayerListModel;
class PlayerSortProxy;
class GamesListModel;
class MainWindow;

class Room : public QObject
{
	Q_OBJECT

	public:
		Room();
		void setConnection(NetworkConnection * c);
		virtual ~Room();
		void onError(void);
		void updateRoomStats(void);
		void clearPlayerList(void);
		void clearGamesList(void);
		void talkOpened(Talk * d);
		void recvToggle(int type, bool val);
		PlayerListing * getPlayerListing(const QString & name);
		PlayerListing * getPlayerListing(const unsigned int id);
		GameListing * getGameListing(unsigned int key);
		BoardDispatch * getNewBoardDispatch(unsigned int key);
		void recvPlayerListing(class PlayerListing * g);
		void recvExtPlayerListing(class PlayerListing * player);
		void recvGameListing(class GameListing * g);
		void sendStatsRequest(PlayerListing & opponent);
	protected:
		void setupUI(void);
		GameListing * registerGameListing(GameListing * l);
		void unRegisterGameListing(unsigned int key);
		
		unsigned int players;
		unsigned int games;
		GameListingRegistry * gameListingRegistry;
		PlayerListingRegistry * playerListingRegistry;
		PlayerListingIDRegistry * playerListingIDRegistry;
	private:
		PlayerListModel * playerListModel;
		PlayerSortProxy * playerSortProxy;
		GamesListModel * gamesListModel;

		QTreeView * playerView, * gamesView;
		QToolButton * refreshGamesButton, * refreshPlayersButton;
		QComboBox * whoBox1, * whoBox2;
		QCheckBox * whoOpenCheckBox;
		NetworkConnection * connection;
		QModelIndex popup_item;
	private slots:
		void slot_playersDoubleClicked(const QModelIndex &);
		void slot_gamesDoubleClicked(const QModelIndex &);
		void slot_refreshPlayers(void);
		void slot_refreshGames(void);
		void slot_setRankSpreadView(void);
		void slot_showOpen(int state);
		void slot_showPopup(const QPoint & iPoint);
		void slot_showGamesPopup(const QPoint & iPoint);
		void slot_popupMatch(void);
		void slot_popupTalk(void);
		void slot_popupObserveOutside(void);
		void slot_popupJoinObserve(void);	
};

#endif //ROOM_H
