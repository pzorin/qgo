#ifndef ROOM_H
#define ROOM_H

#include <QtGui>

class RoomDispatch;
class TalkDispatch;
class PlayerListModel;
class PlayerSortProxy;
class GamesListModel;
class MainWindow;

class Room : public QObject
{
	Q_OBJECT

	public:
		Room();
		void onConnectionAssignment(QString username);
		virtual ~Room();
		RoomDispatch * getDispatch(void) { return dispatch; };
		void onError(void);
		void updateRoomStats(void);
		void clearPlayerList(void);
		void clearGamesList(void);
		void recvExtPlayerListing(class PlayerListing * player);
		void talkOpened(TalkDispatch * d);
		void recvToggle(int type, bool val);
	protected:
		void setupUI(void);
		unsigned int players;
		unsigned int games;
	private:
		friend class RoomDispatch;
		PlayerListModel * playerListModel;
		PlayerSortProxy * playerSortProxy;
		GamesListModel * gamesListModel;

		QTreeView * playerView, * gamesView;
		QToolButton * refreshGamesButton, * refreshPlayersButton;
		QComboBox * whoBox1, * whoBox2;
		QCheckBox * whoOpenCheckBox;
		RoomDispatch * dispatch;
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
