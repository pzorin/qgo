/***************************************************************************
 *   Copyright (C) 2009- by The qGo Project                                *
 *                                                                         *
 *   This file is part of qGo.                                             *
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
#include "connectionwidget.h"
#include "ui_connectionwidget.h"

#include "defines.h"
#include "network/networkconnection.h"
#include "network/talk.h"
#include "network/gamedialog.h"
#include "listviews.h"
#include "playergamelistings.h"		//FIXME should be moved out
#include "network/consoledispatch.h"		//FIXME for channel chat
#include "messages.h"
#include "room.h"

#include "network/serverliststorage.h"


ConnectionWidget::ConnectionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConnectionWidget)
{
    ui->setupUi(this);
    connectionWidget = this; // FIXME: remove this global variable
    ui->changeServerButton->hide();
    connect(ui->changeServerButton, SIGNAL(clicked()), SLOT(slot_changeServer()));

    ui->createRoomButton->hide();
    connect(ui->createRoomButton, SIGNAL(clicked()), SLOT(slot_createRoom()));

    connection = 0;

    connect( ui->connectButton, SIGNAL(clicked()), SLOT( disconnectFromServer(void) ) );

    /* We need to integrate this room list with the new room code FIXME */
    connect(ui->roomComboBox,SIGNAL(currentIndexChanged( const QString &)), SLOT(slot_roomListClicked(const QString &)));
    connect(ui->channelComboBox,SIGNAL(currentIndexChanged( const QString &)), SLOT(slot_channelListClicked(const QString &)));

    //creates the connection code
    seekMenu = new QMenu();
    ui->seekToolButton->setMenu(seekMenu);

    connect(seekMenu,SIGNAL(triggered(QAction*)), SLOT(slot_seek(QAction*)));
    connect(ui->seekToolButton, SIGNAL( toggled(bool) ), SLOT( slot_seek(bool) ) );

    connect( ui->commandLineComboBox, SIGNAL( activated(const QString&) ), this, SLOT( slot_cmdactivated(const QString&) ) );

    // connecting the server tab buttons
    connect( ui->quietCheckBox, SIGNAL( clicked(bool) ), this, SLOT( slot_cbquiet() ) );
    connect( ui->openCheckBox, SIGNAL( clicked(bool) ), this, SLOT( slot_cbopen() ) );
    connect( ui->lookingCheckBox, SIGNAL( clicked(bool) ), this, SLOT( setLooking(bool) ) );

    // for saving server ip lists
    serverliststorage = new ServerListStorage();
}

ConnectionWidget::~ConnectionWidget()
{
    delete ui;
    delete serverliststorage;
    cleanupServerData();
}

void ConnectionWidget::cleanupServerData(void)
{
    //delete serverliststorage;	no header, done in mainwindow FIXME awkward
    qDeleteAll(roomList);
    qDeleteAll(channelList);
}
/*
 * return pressed in edit line -> command to send
 */

void ConnectionWidget::slot_cmdactivated(const QString &cmd)
{
    if(connection->consoleIsChat())
    {
        connection->sendMsg(0, cmd);
        ui->commandLineComboBox->clearEditText();
        return;
    }
    if (cmd.trimmed().isEmpty())
        return;
    if (cmd.mid(0,2).contains("ob"))
    {
        GameListing g;
        QString cmd2 = cmd;
        g.number = cmd2.replace(QRegExp("[\\S]+\\s"), "").toInt();
        if (g.number)
            connection->sendObserve(g);
    }
    else if (cmd.mid(0,4).contains("yell") || cmd.mid(0,1) == ";")
    {
        GameListing g;
        QString cmd2 = cmd;
        cmd2.replace(QRegExp("^[\\S]+\\s"), "");
        connection->getConsoleDispatch()->recvText(cmd2);	 //copy back since otherwise doesn't appear
        connection->sendConsoleText(cmd.toLatin1().constData());
        ui->commandLineComboBox->clearEditText();
    }
    else
    {
        connection->sendConsoleText(cmd.toLatin1().constData());
        ui->commandLineComboBox->clearEditText();
    }
}

void ConnectionWidget::setNetworkConnection(NetworkConnection * conn)
{
    if (connection != NULL)
        connection->disconnect(this);
    connection = conn;
    connectionEstablished = QDateTime::currentDateTime();
    if (connection != NULL)
        connect(connection,SIGNAL(ready()),SLOT(loadConnectionSettings()));
}

void ConnectionWidget::loadConnectionSettings(void)
{
    QSettings settings;
    const bool looking = settings.value("LOOKING_FOR_GAMES").toBool();
    ui->lookingCheckBox->setChecked(looking);

    const bool open = settings.value("OPEN_FOR_GAMES").toBool();
    ui->openCheckBox->setChecked(open);

    if (connection != NULL)
    {
        connection->sendToggle("looking", looking);
        connection->sendToggle("open", open);
    }
}


/*
 * slot_connect: emitted when connect button has toggled
 */
void ConnectionWidget::disconnectFromServer(void)
{
    if(!connection)
        return;
            /* There was definitely a crash from disconnecting
             * with a logindialog open because the logindialog kept
             * calling network connection code.  What doesn't make
             * sense is that the logindialog is modal, so its like
             * the server list allowed the hitting of the connect
             * button, breaking the modality of the logindialog and
             * allowing a crash here... FIXME But we should never
             * get here. */
            // For the time being just made the dialog modal
    if (closeConnection() == 0)
        this->setEnabled(false);
}

/* FIXME I feel like all the UI elements below that hide
 * should hide on destruction of connection, not on
 * reconnect to another service.  At the same time,
 * ORO reconnects as part of its session so that would
 * be a bit tricky.  We'll leave it here for now. */
void ConnectionWidget::setupButtons(void)
{
    unsigned long rs_flags = connection->getRoomStructureFlags();
    if(rs_flags & RS_NOROOMLIST)
    {
        ui->roomComboBox->hide();
        ui->roomsLabel->hide();
    }
    else if(rs_flags & RS_SHORTROOMLIST)
    {
        ui->roomComboBox->show();
        ui->roomsLabel->show();
    }
    else if(rs_flags & RS_LONGROOMLIST)
    {
        /* Here, FIXME, we'd want to replace
        * the menu with a button that brings
        * up a window with a tree list, to join
        * different rooms */
    }

    ui->changeServerButton->setVisible(connection->supportsServerChange());
    ui->createRoomButton->setVisible(connection->supportsCreateRoom());
    ui->refreshPlayersButton->setEnabled(connection->supportsRefreshListsButtons());
    ui->refreshGamesButton->setEnabled(connection->supportsRefreshListsButtons());

    if(connection->supportsSeek())
    {
        ui->seekToolButton->show();
        ui->handicapComboBox->show();
    }
    else
    {
        ui->seekToolButton->hide();
        ui->handicapComboBox->hide();
    }
    if(connection->supportsChannels())
    {
        ui->channelsLabel->show();
        ui->channelComboBox->show();
    }
    else
    {
        ui->channelsLabel->hide();
        ui->channelComboBox->hide();
    }

    /* FIXME Also need away message and maybe game/player refresh */
#ifdef FIXME
    // quiet mode? if yes do clear table before refresh
    gamesListSteadyUpdate = ! ui->quietCheckBox->isChecked();
    playerListSteadyUpdate = ! ui->quietCheckBox->isChecked();

    // enable extended user info features
    setColumnsForExtUserInfo();

            // check for messages
    if (youhavemsg)
        sendcommand("message", false);
    // init shouts
    //TODO		slot_talk("Shouts*", 0, true);

            // show current Server name in status bar
    statusServer->setText(" " + myAccount->svname + " ");
#endif //FIXME
}

/* Shouldn't be here. FIXME */
/* Maybe it should be here? */
void ConnectionWidget::onConnectionError(void)
{
    qDebug("onConnectionError");
    closeConnection(true);		//probably don't care about return here since connection is likely dead.
                    //could be a crash here though or maybe shouldn't be here at all doublecheck FIXME
    /* FIXME this can get stuck open if we get a connection error on connect, like the app doesn't quit when
     * the main window is closed */
    this->setEnabled(false);
}

int ConnectionWidget::closeConnection(bool error)
{
    if(connection)
    {
        if(!error && connection->checkForOpenBoards() < 0)
            return -1;
        NetworkConnection * c = connection;
        connection = 0;
        ui->connectButton->blockSignals(true);
        ui->connectButton->setChecked(false);	//doublecheck all this?
        ui->connectButton->blockSignals(false);
        delete c;
    }
    return 0;
}

/*
 * connection closed - triggered by parser signal
 */
void ConnectionWidget::slot_connexionClosed()
{
    qDebug("slot_connexionClosed");
    // no Timers in offline mode!

#ifdef FIXME
    /* We may need to set somet status somewhere or like the
     * OFFLINE text, but that needs to be somewhere else.
     * This is just a reminder. */
    // set to offline:
    myAccount->set_offline();
#endif //FIXME
    ui->connectButton->blockSignals(true);
    ui->connectButton->setChecked(false);
    ui->connectButton->blockSignals(false);
    seekMenu->clear();

#ifdef FIXME
    /* FIXME FIXME seeking */
    slot_cancelSeek();
#endif //FIXME


//	qDebug("slot_connclosed()");
//	qDebug(QString("%1 -> slot_connclosed()").arg(statusOnlineTime->text()));

}

void ConnectionWidget::slot_changeServer(void)
{
    if(connection)
        connection->changeServer();
}

void ConnectionWidget::slot_createRoom(void)
{
    QMessageBox::information(this, tr("Not available"), tr("This feature will be in a later version"));
#ifdef FIXME
    connection->createRoom();
#endif //FIXME
}

// used for singleShot actions
void ConnectionWidget::set_tn_ready()
{
    tn_ready = true;
    tn_wait_for_tn_ready = false;
    //sendTextFromApp(0);
}

// tell, say, kibitz...
void ConnectionWidget::slot_message(QString txt, QColor c)
{
    ui->MultiLineEdit2->setTextColor(c);
    if(txt.length() - 2 >= 0 &&
          txt[txt.length() - 1] == '\n' && txt[txt.length() - 2] == '\n')
        txt.truncate(txt.length() - 1);
    // Scroll at bottom of text, set cursor to end of line
    if (! (ui->MultiLineEdit2->toPlainText().endsWith('\n') && txt == "\n"))
        ui->MultiLineEdit2->append(txt);

    ui->MultiLineEdit2->setTextColor(Qt::black);
}

/* We're going to leave this as is until we figure out what other
 * protocols might use for it. But obviously its necessary even
 * for the other text protocols.*/
// set session parameter
void ConnectionWidget::set_sessionparameter(QString par, bool val)
{
    QString value;
    if(!connection)
        return;


    //connection->sendText("toggle " + par + value);
    connection->sendToggle(par, val);
    /*switch(myAccount->get_gsname())
    {
        // only toggling...
        case IGS:
            connection->sendText("toggle " + par + value);
            break;

        default:
            connection->sendText("set " + par + value);
            break;
    }*/
}

void ConnectionWidget::slot_roomListClicked(const QString& text)
{
    if(!connection)
        return;
    for(int i=0; i < roomList.length(); i++)
    {
        if(roomList[i]->name == text)
        {
            connection->sendJoinRoom(*(roomList[i]));
            connection->getDefaultRoom()->slot_refreshPlayers();
            connection->getDefaultRoom()->slot_refreshGames();
            return;
        }
    }
    qDebug("Can't find room %s", text.toLatin1().constData());
}

void ConnectionWidget::slot_channelListClicked(const QString& text)
{
    if(!connection)
        return;
    if(text == "No Channel"/* || text == "Leave"*/)
    {

        return;
    }
    if(text == "Create")
    {

        return;
    }
    for(int i=0; i < channelList.length(); i++)
    {
        if(text.contains(QString::number(channelList[i]->number)))
        {
            connection->sendJoinChannel(*(channelList[i]));
            return;
        }
    }
    qDebug("Can't find channel %s", text.toLatin1().constData());
}
#ifdef FIXME





/* Same thing as the slot above that we can probably remove
 * but just in case */
// prepare tables (clear, ...)
void ConnectionWidget::prepareTables(InfoType cmd)
{
    switch (cmd)
    {
        case WHO: // delete player table
        {
//			QListViewItemIterator lv(ListView_players);
//			for (QListViewItem *lvi; (lvi = lv.current());)
//			{
//				lv++;
//				delete lvi;
//			}
#ifdef FIXME
            ui->ListView_players->clear();
#endif //FIXME

            // set number of players to 0
            myAccount->num_players = 0;
            myAccount->num_watchedplayers = 0;
            // use this for fast filling
            playerListEmpty = true;
            statusUsers->setText(" P: 0");// / 0 ");
            break;
        }

        case GAMES: // delete games table
        {
//			QListViewItemIterator lv(ListView_games);
//			for (QListViewItem *lvi; (lvi = lv.current());)
//			{
//				lv++;
//				delete lvi;
//			}
#ifdef FIXME
            ui->ListView_games->clear();
#endif //FIXME

            // set number of games to 0
            myAccount->num_games = 0;
            statusGames->setText(" G: 0");         //(" G: 0 / 0 ");
            break;
        }

        case CHANNELS:
        {
            // delete channel info
//			channellist.clear();
//			statusChannel->setText("");

            // delete tooltips too
//			QToolTip::remove(statusChannel);
//			QToolTip::add(statusChannel, tr("Current channels and users"));

            //We prepare the rooms list as well here
            while (ui->roomComboBox->count() > 1)
                    ui->roomComboBox->removeItem(1);

            break;
        }

        default: // unknown command???
            qWarning("unknown Command in 'prepare_tables()'");
            break;

    }
}
#endif //FIXME


#ifdef FIXME
/* Below, again, could be helpful and such column changes DO need to be
 * done, but they're specific to the connection */
/*
 * sets the columns for specific IGS infos
 */
void ConnectionWidget::setColumnsForExtUserInfo()
{

    if (/* !extUserInfo || */(myAccount->get_gsname() != IGS) )
    {
        // set player table's columns to 'who' mode
#ifdef FIXME
        /* We'll have to test this on other servers?
         * Actually the columns should be there or not
         * but we just might leave them blank or "-"*/
        ui->ListView_players->setColumnHidden(11,TRUE);
        ui->ListView_players->setColumnHidden(10,TRUE);
        ui->ListView_players->setColumnHidden(9,TRUE);
        ui->ListView_players->setColumnHidden(8,TRUE);
        ui->ListView_players->setColumnHidden(7,TRUE);
#endif //FIXME
    }
    else// if ( ui->ListView_players->columns()  < 9 )
    {
        // set player table's columns to 'user' mode
        // first: remove invisible column
//		ui->ListView_players->removeColumn(7);
        // second: add new columns
//		ui->ListView_players->addColumn(tr("Info"));
//		ui->ListView_players->addColumn(tr("Won"));
//		ui->ListView_players->addColumn(tr("Lost"));
//		ui->ListView_players->addColumn(tr("Country"));
//		ui->ListView_players->addColumn(tr("Match prefs"));
//		ui->ListView_players->setColumnAlignment(7, Qt::AlignRight);
//		ui->ListView_players->setColumnAlignment(8, AlignRight);
//		ui->ListView_players->setColumnAlignment(9, AlignRight);
#ifdef FIXME
        ui->ListView_players->setColumnHidden(11,FALSE);
        ui->ListView_players->setColumnHidden(10,FALSE);
#endif //FIXME
//		ui->ListView_players->setColumnHidden(9,FALSE);
//		ui->ListView_players->setColumnHidden(8,FALSE);
//		ui->ListView_players->setColumnHidden(7,FALSE);
    }
}

/*
 * server name found by parser
 */
void ConnectionWidget::slot_svname(GSName &gs)
{
    // save local at 'gsname'
    // and change caption
    myAccount->set_gsname(gs);
    myAccount->set_caption();
}


/*
 * account name found by parser
 */
void ConnectionWidget::slot_accname(QString &name)
{
    // save local at 'gsname'
    // and change caption
    myAccount->set_accname(name);
    myAccount->set_caption();
}

#endif //FIXME

void ConnectionWidget::recvSeekCondition(class SeekCondition * s)
{
    if(!s)
        seekMenu->clear();
    else
    {
        QString time_condition;

        time_condition = QString::number((s->maintime / 60)) + " min + " + QString::number(int(s->periodtime / 60)) + " min / " + QString::number(s->periods) + " stones";

        QAction *act = seekMenu->addAction(time_condition);// , this, SLOT(slot_seek(QAction*)));//, 0, a.toInt());
        act->setData(s->number);
        delete s;
    }
}

void ConnectionWidget::recvSeekCancel(void)
{
    ui->seekToolButton->setChecked(FALSE);
    ui->seekToolButton->setMenu(seekMenu);
//	ui->seekToolButton->setPopupDelay(1);
    ui->seekToolButton->setIcon(QIcon(":/ressources/pics/not_seeking.png"));
    killTimer(seekButtonTimer);
    seekButtonTimer = 0;

}

/* FIXME This is for IGS, needs to be added it */
void ConnectionWidget::recvSeekPlayer(QString /*player*/, QString /*condition*/)
{
#ifdef FIXME
    QString Rk;

    QTreeWidgetItemIterator lv(ui->ListView_players);
    QTreeWidgetItem *lvi;
    for (; (*lv); lv++)
    {
        lvi = *lv;
        // compare names
        if (lvi->text(1) == player)
        {
            Rk = lvi->text(2);
            break;
        }
    }


//	QString Rk = getPlayerRk(player);
    QString hop = Rk.right(1);
    if ((Rk.right(1) != "?") && (Rk.right(1) != "+"))
        Rk.append(" ");

    slot_message(player.leftJustified(15,' ',true) + Rk.rightJustified(5) +  " : " + condition, Qt::darkRed);
#endif //FIXME
}

/*
 * the 'seek' button was pressed
 */
void ConnectionWidget::slot_seek(bool b)
{
//qDebug("seek button pressed : status %i", (int)b);

    //if the button was just pressed on, we have already used the popup menu : nothing to do
    if (b)
        return;
    if(connection)
        connection->sendSeekCancel();
}

/*
 * seek button : menu entry (time conditions) selected
 */
void ConnectionWidget::slot_seek(QAction *act)
{
    ui->seekToolButton->setChecked(true);
    ui->seekToolButton->setMenu(NULL);

    //childish, but we want to have an animated icon there
    seekButtonTimer = startTimer(200);

    SeekCondition * s = new SeekCondition();
    s->number = act->data().toInt();
    qDebug("preparing to send seek");
    switch (ui->handicapComboBox->currentIndex())
    {
        case 0 :
            s->strength_wished = "1 1 0";
            break ;

        case 1 :
            s->strength_wished = "2 2 0";
            break ;

        case 2 :
            s->strength_wished = "5 5 0";
            break ;

        case 3 :
            s->strength_wished = "9 9 0";
            break ;

        case 4 :
            s->strength_wished = "0 9 0";
            break ;

        case 5 :
            s->strength_wished = "9 0 0";
            break ;
    }
    connection->sendSeek(s);
    delete s;
}

/* FIXME FIXME FIXME
 * Same thing here, room changes, as in channels, should be fixed.. but I'm
 * thinking, no console and tabbed chat rooms FIXME FIXME */
/*
 * room string received after 'room' command
 */
void ConnectionWidget::recvRoomListing(const RoomListing & room, bool b)
{
    unsigned long rf = connection->getRoomStructureFlags();
    /* FIXME, either way, we should keep a list of RoomListings
     * some where */
    //qDebug("Recv room listing %d %s", room.number, room.name.toLatin1().constData());
    for(int i=0; i < roomList.length(); i++)
    {
        if(roomList[i]->name == room.name)
        {
            if(!b)
            {
                //room has been removed
                roomList.removeAt(i);
                if(rf & RS_SHORTROOMLIST)
                {
                    //remove item from list
                }
                else if(rf & RS_LONGROOMLIST)
                {

                }
            }
            return;
        }
    }
    if(!b)
        return;		//removed room not found
    roomList.append(&room);

    if(rf & RS_SHORTROOMLIST)
    {
        //do we already have the same room number in list ?
        //if (ui->roomComboBox->findText(room.name.left(3), Qt::MatchStartsWith )!= -1)
        //	return;
        //so far, we just create the room if it is open
        //if(b)
        ui->roomComboBox->addItem(room.name);
    }
    else if(rf & RS_LONGROOMLIST)
    {
        //FIXME update that window tree view we would add
    }
}

void ConnectionWidget::recvChannelListing(const ChannelListing & channel, bool b)
{
    for(int i=0; i < channelList.length(); i++)
    {
        if(channelList[i]->name.contains(QString::number(channel.number)))
        {
            if(!b)
            {
                //channel has been removed
                channelList.removeAt(i);
            }
            return;
        }
    }
    if(!b)
        return;		//removed channel not found
    channelList.append(&channel);
    if(channel.number == 0)
        ui->channelComboBox->addItem(channel.name);
    else
        ui->channelComboBox->addItem(QString::number(channel.number) + " " + channel.name);
}

void ConnectionWidget::changeChannel(const QString & s)
{
    for(int i = 0; i < ui->channelComboBox->count(); i++)
    {
        if(ui->channelComboBox->itemText(i).contains(s))
        {
            ui->channelComboBox->setCurrentIndex(i);
            return;
        }
    }
}

// convert rk to key for sorting;
// if integer is true, then result can be used to calculate handicap
// this is for sort list
QString ConnectionWidget::rkToKey(QString txt, bool integer)
{
    QString rk = txt;
    QString keyStr;

    if (integer)
    {
        // NR
        if (rk == "NR")
            return "0000";

        // BC ( IGS new rating stops at 23 k = BC )
        if (rk == "BC")
            return "0800";

        // get number
        QString buffer = rk;
        buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
        bool ok;
        int pt = buffer.toInt(&ok);
        if (!ok)
            return "0000";

        // check for k,d,p
        if (rk.indexOf("k") != -1)
        {
            pt = (31 - pt)*100 ;//+ ( (rk.find("+") != -1) ? 10:0) ;
        }
        else if (rk.indexOf("d") != -1)
        {
            /* Former code replaced
            // 7d == 1p
            if (pt > 7)
                pt = 3670 + (pt - 6)*30 + ((rk.find("+") != -1) ? 10:0);
            else
                pt = 3000 + pt*100 + ((rk.find("+") != -1) ? 10:0);
            */

            /* New formula */
            pt = 3000 + pt*100 ;//+ ((rk.find("+") != -1) ? 10:0);
        }
        else if (rk.indexOf("p") != -1)
        {

            /* Former code replaced
            // 7d == 1p
            pt = 3670 + pt*30;
            */
            /* New formula : still 7d ~ 1p */
            pt = 3600 + pt*100 ;//+ ((rk.find("+") != -1) ? 10:0);

        }
        else
            return "0000";

        buffer = QString::number(pt).rightJustified(4, '0');
        return buffer;
    }
    else
    {
        // NR
        if (rk == "NR" || rk == "BC")
            return "nr";

        // check for k,d,p
        if (rk.indexOf("k") != -1)
            keyStr = "c";
        else if (rk.indexOf("d") != -1)
            keyStr = "b";
        else if (rk.indexOf("p") != -1)
            keyStr = "a";
        else
            keyStr = "z";

        // get number
        QString buffer = rk;
        buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
        if (buffer.length() < 2)
        {
            keyStr += "0";

            // reverse sort order for dan/pro players
            if ((keyStr == "a0") || (keyStr == "b0"))
            {
                int i = buffer.toInt();
                i = 10 - i;
                buffer = QString::number(i);
            }
        }

        return keyStr + buffer;
    }
}

/* FIXME If nothing uses this, we should remove it */
 // handle chat boxes in a list
void ConnectionWidget::slot_talk(const QString &name, const QString &text, bool /*isplayer*/)
{
    static Talk *dlg;
    QString txt;
    qDebug("slot_talk\n");
    return;
//	bool bonus = false;
//	bool autoAnswer = true;

//	if (text && text != "@@@")
        // text given or player logged in
        txt = text;
//	else if (text == "@@@" && isplayer)
//	{
        // player logged out -> iplayer == false
//		txt = tr("USER NOT LOGGED IN.");
//		autoAnswer = false;
//	}
//	else
//	{
//		txt = "";
//		autoAnswer = false;
//	}

    // dialog recently used?
    if (dlg && dlg->get_name() == name)
        dlg->write(txt);
    else if (!name.isEmpty() && name != tr("msg*"))
    {
        // seek dialog
        dlg = 0;
//		dlg = talkList.first();
//		while (dlg != NULL && dlg->get_name() != name)
//			dlg = talkList.next();
        for (int i=0; i<talkList.count(); i++)
        {
//			dlg = talkList.at(i);
            if (talkList.at(i)->get_name() == name)
                dlg = talkList.at(i);
//				break;
        }
        // not found -> create new dialog
        if (!dlg)
        {
            //dlg = new Talk(name, 0, isplayer);
            talkList.insert(0, dlg);
//			dlg = talkList.current();
            connect(dlg, SIGNAL(signal_talkTo(QString&, QString&)), this, SLOT(slot_talkTo(QString&, QString&)));
            //connect(dlg, SIGNAL(signal_matchRequest(const QString&)), this, SLOT(slot_matchRequest(const QString&)));

            // make new multiline field
            ui->talkTabs->addTab(dlg->get_tabWidget(), dlg->get_name());

            if (name != tr("Shouts*"))
                ui->talkTabs->setCurrentWidget(dlg->get_tabWidget());

            dlg->pageActive = true;
            connect(dlg->get_le(), SIGNAL(returnPressed()), dlg, SLOT(slot_returnPressed()));
            connect(dlg, SIGNAL(signal_pbRelOneTab(QWidget*)), this, SLOT(slot_pbRelOneTab(QWidget*)));

//			QPalette pal = dlg->get_mle()->palette();
//			pal.setColor(QColorGroup::Base, setting->colorBackground);
//			dlg->get_mle()->setPalette(pal);
//			dlg->get_le()->setPalette(pal);

            //if (!name.isEmpty() && name != tr("Shouts*") && currentCommand->get_txt() !="stats")
#ifdef FIXME
            if (!name.isEmpty() && isplayer)
                slot_sendCommand("stats " + name, false);    // automatically request stats
#endif //FIXME
        }

        Q_CHECK_PTR(dlg);
        dlg->write(txt);

        // play sound on new created dialog
//		bonus = true;
    }
/*
    // check if it was a channel message
    if (autoAnswer &= (isplayer && autoAwayMessage && !name.contains('*') && text[0] == '>'))
    {
        // send when qGo is NOT the active application - TO DO
        sendcommand("tell " + name + " [I'm away right now]");
    }
*/
    if (!dlg->pageActive)
    {
        ui->talkTabs->addTab(dlg->get_tabWidget(), dlg->get_name());
        dlg->pageActive = true;
        ui->talkTabs->setCurrentWidget(dlg->get_tabWidget());
    }
/*
    // play a sound - not for shouts
    if ((text[0] == '>' && bonus || !dlg->get_le()->hasFocus()) && !name.contains('*'))
    {
        qgoif->get_qgo()->playTalkSound();

        // set cursor to last line
        //dlg->get_mle()->setCursorPosition(dlg->get_mle()->lines(), 999); //eb16
        dlg->get_mle()->append("");                                        //eb16
        //dlg->get_mle()->removeParagraph(dlg->get_mle()->paragraphs()-2);   //eb16

        // write time stamp
        MultiLineEdit3->append(statusOnlineTime->text() + " " + name + (autoAnswer ? " (A)" : ""));
    }
    else if (name == tr("msg*"))
    {
        qgoif->get_qgo()->playTalkSound();

        // set cursor to last line
//		dlg->get_mle()->setCursorPosition(dlg->get_mle()->numLines(), 999); //eb16
        dlg->get_mle()->append(""); //eb16
//		dlg->get_mle()->removeParagraph(dlg->get_mle()->paragraphs()-2);   //eb16

        // write time stamp
        MultiLineEdit3->append(tr("Message") + ": " + text);
    }
*/
}

/* I think we still use this, it needs to move moved to somewhere
 * or kept here but... reconciled with new code or something FIXME */
void ConnectionWidget::talkOpened(Talk * d)
{
    talkList.insert(0, d);
    // make new multiline field
    ui->talkTabs->addTab(d->get_tabWidget(), d->get_name());

    ui->talkTabs->setCurrentWidget(d->get_tabWidget());

    d->pageActive = true;
    connect(d->get_le(), SIGNAL(returnPressed()), d, SLOT(slot_returnPressed()));
    connect(d, SIGNAL(signal_pbRelOneTab(QWidget*)), this, SLOT(slot_pbRelOneTab(QWidget*)));

    if (!d->pageActive)
    {
        ui->talkTabs->addTab(d->get_tabWidget(), d->get_name());
        d->pageActive = true;
        ui->talkTabs->setCurrentWidget(d->get_tabWidget());
    }
}

void ConnectionWidget::talkRecv(Talk * d)
{
    /* FIXME This needs to set this dialog as active
     * or have its name blink or turn red or something */
    qDebug("MW::talkRecv");
    if (!d->pageActive)
    {
        ui->talkTabs->addTab(d->get_tabWidget(), d->get_name());
        d->pageActive = true;
        ui->talkTabs->setCurrentWidget(d->get_tabWidget());
    }
}

/*
 * close button pressed on talk tab
 */
void ConnectionWidget::slot_pbRelOneTab(QWidget *w)
{
    // seek dialog
    Talk *dlg = talkList.at(0);
    int i= talkList.indexOf((Talk*)w);

    if ( i != -1)
    {
        if (w != ui->talkTabs->currentWidget())
            //we have a problem !
            return;

//		dlg = talkList.takeAt(i);
        ui->talkTabs->removeTab(ui->talkTabs->currentIndex()) ;
        dlg->pageActive = false;

        connection->closeTalk(&(dlg->get_opponent()));
    }

}

/*
 * 'stats' information has been received by the parser
 */
void ConnectionWidget::slot_statsPlayer(PlayerListing *p)
{
    Talk *dlg = 0;
    QString txt = "";
    /* FIXME !!! */
    slot_talk( p->name, txt, true);
    if (!p->name.isEmpty())
    {
        // seek dialog
        for (int i=0; i<talkList.count(); i++)
        {
            if (talkList.at(i)->get_name() == p->name)
                dlg = talkList.at(i);
        }
        //  found
        if (dlg)
        {
            dlg->getUi().stats_rating->setText(p->rank);
            dlg->getUi().stats_info->setText(p->info);
            dlg->getUi().stats_default->setText(p->extInfo);
            dlg->getUi().stats_wins->setText(QString::number(p->wins) + " /");
            dlg->getUi().stats_loss->setText(QString::number(p->losses) );
            dlg->getUi().stats_country->setText(p->country);
            dlg->getUi().stats_playing->setText(QString::number(p->playing));
            //dlg->getUi().stats_rated->setText(p->rated);
            dlg->getUi().stats_address->setText(p->email_address);

            // stored either idle time or last access
            dlg->getUi().stats_idle->setText(p->idletime);
            if (!p->idletime.isEmpty())
                dlg->getUi().Label_Idle->setText(p->idletime.at(0).isDigit() ? "Idle :": "Last log :");


        }
    }
}

/*
 * set checkbox status because of server info or because menu / checkbok toggled
 */
void ConnectionWidget::slot_checkbox(int nr, bool val)
{
//	QCheckBox::ToggleState v = (val ?

    // set checkbox (nr) to val
    switch (nr)
    {
        // open
        case 0:
            //toolOpen->setOn(val);
            ui->openCheckBox->setChecked(val);
            break;

        // looking
        case 1:
            //toolLooking->setOn(val);
            ui->lookingCheckBox->setChecked(val);
            break;

        // quiet
        case 2:
            //toolQuiet->setOn(val);
            ui->quietCheckBox->setChecked(val);
            break;

        default:
            qWarning("checkbox doesn't exist");
            break;
    }
}

/*
 * checkbox looking cklicked
 */
void ConnectionWidget::setLooking(bool val)
{
    set_sessionparameter("looking", val);

    if (val)
        // if looking then set open
        set_sessionparameter("open", true);
}

/*
 * checkbox open clicked
 */
void ConnectionWidget::slot_cbopen()
{
    bool val = ui->openCheckBox->isChecked();
    set_sessionparameter("open", val);
    qDebug("Setting session open: %d", val);
    if (!val)
        // if not open then set close
        set_sessionparameter("looking", false);
    /* See above comment. */
    /*QSettings settings;
    settings.setValue("OPEN_FOR_GAMES", val);
    if(!val)
        settings.setValue("LOOKING_FOR_GAMES", false);*/
}

/*
 * checkbox quiet clicked
 */
void ConnectionWidget::slot_cbquiet()
{
    bool val = ui->quietCheckBox->isChecked();
    set_sessionparameter("quiet", val);

    /* FIXME periodic list refreshes may not be necessary, set quiet true does a lot */
    /*if (val)
    {
        if(connection)
            connection->periodicListRefreshes(false);
        // if 'quiet' button is once set to true the list is not reliable any longer
    }
    else
    {
        if(connection)
            connection->periodicListRefreshes(true);
    }*/
}

void ConnectionWidget::slot_alternateListColorsCB(bool b)
{
    ui->playerView->setAlternatingRowColors(b);
    ui->gamesView->setAlternatingRowColors(b);
}

void ConnectionWidget::timerEvent(QTimerEvent* e)
{
    // some variables for forcing send buffer and keep line established
    static int counter = 899;
    static int holdTheLine = true;
    static int tnwait = 0;
    static int imagecounter = 0;

    //qDebug( "timer event, id %d", e->timerId() );

    if (e->timerId() == seekButtonTimer)
    {
        imagecounter = (imagecounter+1) % 4;
        QString ic = ":/ressources/pics/seeking" + QString::number(imagecounter + 1) +".png";
        ui->seekToolButton->setIcon(QIcon(ic));
        return;
    }
    // else its the mainServerTimer
    if (tn_ready)
    {
        // case: ready to send
        tnwait = 0;
    }
    else if (tnwait < 2)
    {
        //qDebug(QString("%1: HoldTheLine SET: something has been sent").arg(statusOnlineTime->text()));
        // case: not ready to send, but maybe waiting for READY message
        tnwait++;
        // something was sent, so reset counter
        counter = 899;//resetCounter();
        holdTheLine = true;
        autoAwayMessage = false;
    }

    if (counter % 300 == 0)
    {
        // 5 mins away -> set auto answering
        autoAwayMessage = true;
        if (counter == 0)
        {
            // send "ayt" every half hour anyway
            // new: reset timer after one hour of idle state
            //      -> if not observing a game!
            //qDebug(QString("%1 -> HoldTheLine: status = %2").arg(statusOnlineTime->text()).arg(holdTheLine));
#ifdef FIXME
            if (holdTheLine)
                sendcommand("ayt", false);
#endif //FIXME
            // 12*5 min
            counter = 899;//	resetCounter();
#ifdef FIXME
            if (myAccount->num_observedgames == 0)
            {
                // nothing observing
                holdTheLine = false;
//qDebug(QString("%1: HoldTheLine END!").arg(statusOnlineTime->text()));
            }
            else
            {
//qDebug(QString("%1: HoldTheLine LENGTHENED: observing game...").arg(statusOnlineTime->text()));
            }
#endif //FIXME
        }
#ifdef FIXME
        else if (myAccount->get_gsname() == IGS && holdTheLine)
        {
            sendcommand("gamelist", false);
            qDebug(QString("%1 -> gamelist").arg(statusOnlineTime->text()).toLatin1().constData());
        }
#endif //FIXME
    }

    counter--;

    // display online time. FIXME: maybe update every minute
    /*
    QTime tempTime(0,0,0,0);
    tempTime.addSecs(connectionEstablished.secsTo(QDateTime::currentDateTime()));
    tempTime.toString("HH:mm"); */
    //statusOnlineTime->setText(pre + min_ + ":" + sec_ + " ");

    // some statistics
//	QToolTip::remove(statusServer);
//	QToolTip::add(statusServer, tr("Current server") + "\n" +
//		tr("Bytes in:") + " " + QString::number(bytesIn) + "\n" +
//		tr("Bytes out:") + " " + QString::number(bytesOut));
//	LineEdit_bytesIn->setText(QString::number(bytesIn));
//	LineEdit_bytesOut->setText(QString::number(bytesOut));
}

/*
 * The parser has sent a message to be put in the messages box
 */
void ConnectionWidget::slot_msgBox(const QString& msg)
{
    qDebug("slot_msgBox\n");
    ui->talkTabs->setCurrentIndex(1);
    ui->msgTextEdit->append(msg);
}

bool ConnectionWidget::isConnected(void)
{
    return (connection != NULL);
}
