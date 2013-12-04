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


#ifndef FRIENDSLISTDIALOG_H
#define FRIENDSLISTDIALOG_H

#include "ui_friendslistdialog.h"
#include <QtWidgets>

class NetworkConnection;
class Room;

class FriendsListDialog : public QDialog, Ui::FriendsListDialog
{
	Q_OBJECT
	public:
        FriendsListDialog(NetworkConnection * c, Room * r);
		~FriendsListDialog();
	private:
		void populateLists(void);
		Ui::FriendsListDialog ui;
		NetworkConnection * connection;
        Room * room;
		
		QModelIndex popup_item;
		class PlayerListing * popup_playerlisting;
		class SimplePlayerListModel * friendsListModel;
		class SimplePlayerListModel * watchesListModel;
		class SimplePlayerListModel * blockedListModel;
	private slots:
		void slot_showPopupFriends(const QPoint & iPoint);
		void slot_showPopupWatches(const QPoint & iPoint);
		void slot_showPopupBlocked(const QPoint & iPoint);
		void slot_addFriend(void);
		void slot_removeFriend(void);
		void slot_addWatch(void);
		void slot_removeWatch(void);
		void slot_addBlock(void);
		void slot_removeBlock(void);
		void slot_playersDoubleClickedFriends(const QModelIndex & index);
		void slot_playersDoubleClickedWatches(const QModelIndex & index);
		void slot_popupMatch(void);
		void slot_popupTalk(void);
};
#endif //FRIENDSLISTDIALOG_H
