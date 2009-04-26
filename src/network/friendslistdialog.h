#ifndef FRIENDSLISTDIALOG_H
#define FRIENDSLISTDIALOG_H

#include "ui_friendslistdialog.h"
#include <QtGui>

class NetworkConnection;

class FriendsListDialog : public QDialog, Ui::FriendsListDialog
{
	Q_OBJECT
	public:
		FriendsListDialog(NetworkConnection * c);
		~FriendsListDialog();
	private:
		void populateLists(void);
		Ui::FriendsListDialog ui;
		NetworkConnection * connection;
		
		QModelIndex popup_item;
		class PlayerListing * popup_playerlisting;
		class SimplePlayerListModel * friendsListModel;
		class SimplePlayerListModel * fansListModel;
		class SimplePlayerListModel * blockedListModel;
	private slots:
		void slot_showPopupFriends(const QPoint & iPoint);
		void slot_showPopupFans(const QPoint & iPoint);
		void slot_showPopupBlocked(const QPoint & iPoint);
		void slot_addFriend(void);
		void slot_removeFriend(void);
		void slot_addFan(void);
		void slot_removeFan(void);
		void slot_addBlock(void);
		void slot_removeBlock(void);
		void slot_playersDoubleClickedFriends(const QModelIndex & index);
		void slot_playersDoubleClickedFans(const QModelIndex & index);
		void slot_popupMatch(void);
		void slot_popupTalk(void);
};
#endif //FRIENDSLISTDIALOG_H
