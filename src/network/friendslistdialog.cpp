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


#include "friendslistdialog.h"
#include "network/networkconnection.h"
#include "network/playergamelistings.h"
#include "listviews.h"
#include "talk.h"
#include "room.h"

FriendsListDialog::FriendsListDialog(NetworkConnection * c, Room * r) : QDialog(), Ui::FriendsListDialog()
{
	connection = c;
    room = r;
	ui.setupUi(this);
	
	friendsView = ui.friendsView;
	watchesView = ui.watchesView;
	blockedView = ui.blockedView;
	
	//friendsSortProxy = new PlayerSortProxy();
	friendsListModel = new SimplePlayerListModel(true);
	//friendsSortProxy->setSourceModel(friendsListModel);
	//friendsView->setModel(friendsSortProxy);
	friendsView->setModel(friendsListModel);
	//friendsSortProxy->setDynamicSortFilter(true);
	//probably don't need to be able to change sort order or sort on notify FIXME
	watchesListModel = new SimplePlayerListModel(true);
	watchesView->setModel(watchesListModel);
	blockedListModel = new SimplePlayerListModel(true);
	blockedView->setModel(blockedListModel);
	
	connect(friendsView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopupFriends(const QPoint &)));
	connect(watchesView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopupWatches(const QPoint &)));
	connect(blockedView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopupBlocked(const QPoint &)));
	
	connect(friendsView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playersDoubleClickedFriends(const QModelIndex &)));
	connect(watchesView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playersDoubleClickedWatches(const QModelIndex &)));
	
	
	/* We need basically the same popup options as from room.cpp */
	populateLists();
	
}

FriendsListDialog::~FriendsListDialog()
{
	delete friendsListModel;
	delete watchesListModel;
	delete blockedListModel;
}

void FriendsListDialog::populateLists(void)
{
	/* Either we get the local lists from the network connection
	 * or we get server side lists... the issue is that
	 * different servers may have different stored info on
	 * the listings, which means we can't necessarily assume
	 * they come in the same form as the local lists or are
	 * loaded as such.  We might need to override that or have
	 * some kind of special list iterator or maybe have the
	 * other protocols subclass the FriendWatchListing and do
	 * some dynamic_cast ing here. 
	 * In any event, I think we need to checkup on ORO and tygem
	 * friend lists before we go any further here */
	 /* So far it looks like if there's a notification option,
	  * its global, which means ours would either be stuck on
	  * or we'd have to store them privately.
	  * Tygem doesn't seem to have watches, but it does have
	  * friends and blocks, IGS nothing.
	  * 
	  * I think though I've decided that the time checks and
	  * the friends lists are a lower priority than getting
	  * games playing on all three services */ 
	
	std::vector<FriendWatchListing *> & friends = connection->getFriendsList();
	std::vector<FriendWatchListing *> & watches = connection->getWatchesList();
	std::vector<FriendWatchListing *> & blocked = connection->getBlockedList();
	
	std::vector<FriendWatchListing *>::iterator i;
	PlayerListing * p;
	for(i = friends.begin(); i != friends.end(); i++)
	{
		p = connection->getPlayerListingFromFriendWatchListing(**i);
		if(p)
			friendsListModel->insertListing(p);
	}
	for(i = watches.begin(); i != watches.end(); i++)
	{
		p = connection->getPlayerListingFromFriendWatchListing(**i);
		if(p)
			watchesListModel->insertListing(p);
	}
	for(i = blocked.begin(); i != blocked.end(); i++)
	{
		p = connection->getPlayerListingFromFriendWatchListing(**i);
		if(p)
			blockedListModel->insertListing(p);
	}
}

/* FIXME this is getting the wrong entry, like if I click on 0th, it takes
 * the 1st off */
void FriendsListDialog::slot_showPopupFriends(const QPoint & iPoint)
{
	popup_item = friendsView->indexAt(iPoint);
	if (popup_item != QModelIndex())
    {
		popup_playerlisting = friendsListModel->playerListingFromIndex(popup_item);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
		QMenu menu(friendsView);
		menu.addAction(tr("Match"), this, SLOT(slot_popupMatch()));
		menu.addAction(tr("Talk"), this, SLOT(slot_popupTalk()));
		menu.addSeparator();
		menu.addAction(tr("Remove from Friends"), this, SLOT(slot_removeFriend()));
		menu.addAction(tr("Add to Watches"), this, SLOT(slot_addWatch()));
		menu.addAction(tr("Block"), this, SLOT(slot_addBlock()));
		menu.exec(friendsView->mapToGlobal(iPoint));
	}
}

void FriendsListDialog::slot_showPopupWatches(const QPoint & iPoint)
{
	popup_item = watchesView->indexAt(iPoint);
	if (popup_item != QModelIndex())
    {
		popup_playerlisting = watchesListModel->playerListingFromIndex(popup_item);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
		QMenu menu(watchesView);
		menu.addAction(tr("Match"), this, SLOT(slot_popupMatch()));
		menu.addAction(tr("Talk"), this, SLOT(slot_popupTalk()));
		menu.addSeparator();
		menu.addAction(tr("Add to Friends"), this, SLOT(slot_addFriend()));
		//Maybe we don't want to have match and talk as fan options?
		menu.addAction(tr("Remove from Watches"), this, SLOT(slot_removeWatch()));
		menu.addAction(tr("Block"), this, SLOT(slot_addBlock()));
		menu.exec(watchesView->mapToGlobal(iPoint));
	}
}

void FriendsListDialog::slot_showPopupBlocked(const QPoint & iPoint)
{
	popup_item = blockedView->indexAt(iPoint);
	if (popup_item != QModelIndex())
    {
		popup_playerlisting = blockedListModel->playerListingFromIndex(popup_item);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
		QMenu menu(blockedView);
		menu.addAction(tr("Remove from Blocked"), this, SLOT(slot_removeBlock()));
		menu.exec(blockedView->mapToGlobal(iPoint));
	}
}

void FriendsListDialog::slot_addFriend(void)
{
	if(popup_playerlisting->friendWatchType == PlayerListing::watched)
		watchesListModel->removeListing(popup_playerlisting);
	else if(popup_playerlisting->friendWatchType == PlayerListing::blocked)
		blockedListModel->removeListing(popup_playerlisting);
	friendsListModel->insertListing(popup_playerlisting);
    connection->addFriend(popup_playerlisting);
}

/* This needs to call a resort or something on the list
 * if we have "friends" checked, same thing with blocked
 * if we don't, just to update that one entry FIXME */
void FriendsListDialog::slot_removeFriend(void)
{
	friendsListModel->removeListing(popup_playerlisting);
    connection->removeFriend(popup_playerlisting);
}

void FriendsListDialog::slot_addWatch(void)
{
	if(popup_playerlisting->friendWatchType == PlayerListing::friended)
		friendsListModel->removeListing(popup_playerlisting);
	else if(popup_playerlisting->friendWatchType == PlayerListing::blocked)
		blockedListModel->removeListing(popup_playerlisting);
	watchesListModel->insertListing(popup_playerlisting);
    connection->addWatch(popup_playerlisting);
}

void FriendsListDialog::slot_removeWatch(void)
{
	watchesListModel->removeListing(popup_playerlisting);
    connection->removeWatch(popup_playerlisting);
}

void FriendsListDialog::slot_addBlock(void)
{
	if(popup_playerlisting->friendWatchType == PlayerListing::friended)
		friendsListModel->removeListing(popup_playerlisting);
	else if(popup_playerlisting->friendWatchType == PlayerListing::watched)
		watchesListModel->removeListing(popup_playerlisting);
	blockedListModel->insertListing(popup_playerlisting);
    connection->addBlock(popup_playerlisting);
}

void FriendsListDialog::slot_removeBlock(void)
{
	blockedListModel->removeListing(popup_playerlisting);
    connection->removeBlock(popup_playerlisting);
}

void FriendsListDialog::slot_popupMatch(void)
{
    connection->sendMatchInvite(popup_playerlisting);
}

void FriendsListDialog::slot_popupTalk(void)
{
    Talk * talk = room->getTalk(popup_playerlisting);
	if(talk)
		talk->updatePlayerListing();
}

/* See the room code this was copied from.  Its weird how we do all this... */
void FriendsListDialog::slot_playersDoubleClickedFriends(const QModelIndex & index)
{
	PlayerListing * opponent = friendsListModel->playerListingFromIndex(index);
    Talk * talk = room->getTalk(opponent);
	if(talk)
		talk->updatePlayerListing();
}

void FriendsListDialog::slot_playersDoubleClickedWatches(const QModelIndex & index)
{
	PlayerListing * opponent = watchesListModel->playerListingFromIndex(index);
    Talk * talk = room->getTalk(opponent);
	if(talk)
		talk->updatePlayerListing();
}
