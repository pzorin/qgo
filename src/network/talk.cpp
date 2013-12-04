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


#include "talk.h"
#include "networkconnection.h"
#include "gamedialog.h"
#include "messages.h"
#include "playergamelistings.h"
#include "connectionwidget.h"
#include "room.h"
/* I wonder if we could somehow generalize this class to handle
 * all message/windows even if they're part of larger windows.
 * We could even do it for the console, specifying a special
 * "opponent" and for the room, with the room name as the
 * opponent.  And then it could handle all the sendText
 * messages that would otherwise be spread around all
 * the dispatches, cluttering up their otherwise
 * specific calls.*/

Talk::Talk(NetworkConnection * conn, PlayerListing *player, Room * r) : TalkGui(), connection(conn), opponent(player), room(r)
{
    qDebug("Creating Talk for %s", opponent->name.toLatin1().constData());
	ui.setupUi(this);
    opponent->dialog_opened = true;
	conversationOpened = false;

/*
	// do not add a button for shouts* or channels tab
	if ( (name.find('*') != -1) || (!isplayer))
	{
		delete pb_releaseTalkTab;
		delete pb_match;
		delete stats_layout;
	}
*/
	connect (ui.pb_releaseTalkTab, SIGNAL(pressed()),SLOT(slot_pbRelTab()));
	connect (ui.pb_match, SIGNAL(pressed()),SLOT(slot_match()));
    connect(ui.LineEdit1, SIGNAL(returnPressed()), SLOT(slot_returnPressed()));
}

Talk::~Talk()
{
	qDebug("Talk destroyed");
    opponent->dialog_opened = false;
}

void Talk::closeEvent(QCloseEvent *)
{
    if(room)
        room->closeTalk(opponent);
}

QString Talk::get_name() const
{ return opponent->name; }

// release current Tab
void Talk::slot_pbRelTab()
{
    room->closeTalk(opponent);
}

void Talk::slot_returnPressed()
{
	// read tab
	QString txt = ui.LineEdit1->text();
	connection->sendMsg(opponent, txt);
    ui.MultiLineEdit1->append(connection->getUsername() + ": " + txt);
	ui.LineEdit1->clear();
}

void Talk::slot_match()
{
	connection->sendMatchInvite(opponent);
}

void Talk::recvTalk(QString text)
{
    ui.MultiLineEdit1->append(opponent->name + ": " + text);
}

void Talk::updatePlayerListing(void)
{
    if (opponent == NULL)
        return;

    ui.stats_rating->setText(opponent->rank);
    ui.stats_info->setText(opponent->info);
    ui.stats_default->setText(opponent->extInfo);
    ui.stats_wins->setText(QString::number(opponent->wins) + " /");
    ui.stats_loss->setText(QString::number(opponent->losses) );
    ui.stats_country->setText(opponent->country);
    ui.stats_playing->setText(QString::number(opponent->playing));
            //ui.stats_rated->setText(p->rated);
    ui.stats_address->setText(opponent->email_address);

            // stored either idle time or last access
    ui.stats_idle->setText(opponent->idletime);
    if (!opponent->idletime.isEmpty())
        ui.Label_Idle->setText(opponent->idletime.at(0).isDigit() ? "Idle :": "Last log :");
}
