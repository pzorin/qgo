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


#ifndef TALK_H
#define TALK_H

#include "ui_talk_gui.h"
#include <QtWidgets>

class NetworkConnection;
class PlayerListing;
class Room;

class Talk : public QDialog, public Ui::TalkGui
{
Q_OBJECT

public:
    Talk(NetworkConnection * conn, PlayerListing *player, Room * r);
    virtual ~Talk();
    QString get_name() const;
    bool getConversationOpened(void) const { return conversationOpened; }
    void setConversationOpened(bool c) { conversationOpened = c; }
    void recvTalk(QString text);
public slots:
	void slot_returnPressed();
	void slot_pbRelTab();
	void slot_match();
    void updatePlayerListing();

private:
	void closeEvent(QCloseEvent *e);
    Ui::TalkGui ui;
	NetworkConnection * connection;
    PlayerListing * opponent;
	bool conversationOpened;
    Room * room;
};

#endif
