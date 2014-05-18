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


#ifdef LATENCY
#include <sys/time.h>
#endif //LATENCY
#include "defines.h"
#include "networkconnection.h"
#include "consoledispatch.h"
#include "boarddispatch.h"
#include "talk.h"
#include "gamedialog.h"
#include "room.h"
#include "playergamelistings.h"
#include "connectionwidget.h"			//don't like so much
#include "matchnegotiationstate.h"

#define FRIENDWATCH_NOTIFY_DEFAULT	1

NetworkConnection::NetworkConnection(ConnectionCredentials credentials) :
default_room(0), console_dispatch(0), qsocket(0)
{
	firstonReadyCall = 1;
	mainwindowroom = 0;
	friendwatch_notify_default = FRIENDWATCH_NOTIFY_DEFAULT;

    connectingDialog = 0;
	match_negotiation_state = new MatchNegotiationState();

    connectionType = credentials.type;
    hostname = credentials.hostName;
    port = credentials.port;
    username = credentials.userName;
    password = credentials.password;

    ourListing = NULL;
}

bool NetworkConnection::openConnection(const QString & host, const unsigned short port, bool not_main_connection)
{	
	qsocket = new QTcpSocket();	//try with no parent passed for now
	if(!qsocket)
		return 0;
	//connect signals
	
	//connect(qsocket, SIGNAL(hostFound()), SLOT(OnHostFound()));
	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(disconnected ()), SLOT(OnConnectionClosed()));
//	connect(qsocket, SIGNAL(delayedCloseFinished()), SLOT(OnDelayedCloseFinish()));
//	connect(qsocket, SIGNAL(bytesWritten(qint64)), SLOT(OnBytesWritten(qint64)));
    connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));

	if(qsocket->state() != QTcpSocket::UnconnectedState)
	{
		qDebug("Called openConnection while in state %d", qsocket->isValid());
		return 0;
	}
	//remove asserts later
	Q_ASSERT(host != 0);
	Q_ASSERT(port != 0);

	if(!not_main_connection)
		drawPleaseWait();

	qDebug("Connecting to %s %d...\n", host.toLatin1().constData(), port);
	// assume info.host is clean
	qsocket->connectToHost(host, (quint16) port);
	
	/* If dispatch does not have a UI, the thing that sets the UI
	 * will setupRoomAndConsole */
	/* Tricky now without dispatches... who sets up the UI?
	 * there's always a mainwindow... but maybe things aren't setup? */
	
	/* connectionInfo as a message with those pointers is probably a bad idea */
	return (qsocket->state() != QTcpSocket::UnconnectedState);
}

int NetworkConnection::write(const char * packet, unsigned int size)
{
    return qsocket->write(packet, size);
}

void NetworkConnection::writeZeroPaddedString(char * dst, const QString & src, int size)
{
	int i;
	Q_ASSERT(src.length() <= size);
	for(i = 0; i < src.length(); i++)
		dst[i] = src.toLatin1().constData()[i];
	for(i = src.length(); i < size; i++)
		dst[i] = 0x00;
}

int NetworkConnection::checkForOpenBoards(void)
{
	BoardDispatch * boarddispatch;
    QMap<unsigned int, class BoardDispatch *>::iterator i;
    for(i = boardDispatchMap.begin(); i != boardDispatchMap.end(); i++)
	{
        boarddispatch = i.value();
		if(!boarddispatch->canClose())
			return -1;
	}
	return 0;
}

void NetworkConnection::closeConnection(bool send_disconnect)
{
	if(!qsocket)		//when can this happen?  this function shouldn't be
				//called if we get here!!!
		return;
	
	/* FIXME We also need to close any open dispatches,
	* boards etc., for instance if there was an error.
	* Clearing lists and such would be good to.
	* there's a MainWindow::connexionClosed that does
	* good stuff we should move into somewhere
	* nearby., also what about onError?*/
    if(connectingDialog)
    {
        connectingDialog->deleteLater();
        connectingDialog = 0;
    }
	if(qsocket->state() != QTcpSocket::UnconnectedState)
	{
		if(send_disconnect)
		{
			if(console_dispatch)
				console_dispatch->recvText("Disconnecting...\n");
			qDebug("Disconnecting...");
			sendDisconnect();		//legit?	
		}
		// Close it.
		qsocket->close();
	
		// Closing succeeded, return message
		if (qsocket->state() == QTcpSocket::UnconnectedState)
		{
			//authState = LOGIN;
			//sendTextToApp("Connection closed.\n");
		}
	}
	// Not yet closed? Data will be written and then slot OnDelayClosedFinish() called
	
	//delete qsocket;
	qsocket->deleteLater();		//for safety
	qsocket = 0;
	onClose();
	
	return;
}

void NetworkConnection::onClose(void)
{
	/* This is from old netdispatch, fix me */
	//RoomDispatch * room;
	/* This needs to close all open dispatches */
	/*room = connection->getDefaultRoomDispatch();
	if(room)
	{
	connection->setDefaultRoomDispatch(0);
	delete room;
		//FIXME mainwindowroom = 0;
}*/
	tearDownRoomAndConsole();
}

NetworkConnection::~NetworkConnection()
{
	//feels a little sparse so far... this should probably be super close FIXME
	//qDebug("Destroying connection\n");
	//closeConnection();			//specific impl already calls this
	/* Not sure where to delete qsocket.  Possible OnDelayClosedFinish() thing. */
	//delete qsocket;
	//In case these still exist
	//probably unnecessary?
	delete match_negotiation_state;
}

void NetworkConnection::setConsoleDispatch(class ConsoleDispatch * c)
{
	console_dispatch = c;
}

QTime NetworkConnection::gd_checkMainTime(TimeSystem s, const QTime & t)
{
	if(s == canadian)
	{
//		int seconds = (t.minute() * 60) + t.second();
		if(t.second())
		{
//			seconds = (t.minute() * 60);
			return QTime(0, t.minute(), 0);
		}
	}
	return t;
}

PlayerListing * NetworkConnection::getPlayerListingFromFriendWatchListing(FriendWatchListing & f)
{
	if(!default_room) 
		return NULL;
	else
	{
		if(playerTrackingByID())
		{
			if(f.id == 0)
			{
				PlayerListing * p = default_room->getPlayerListing(f.name);
                f.id = p->id;
				return p;
			}
			else
				return default_room->getPlayerListing(f.id);
		}
		else
			return default_room->getPlayerListing(f.name);
	}
}

/* This will often be overridden by the specific connection but
 * otherwise this will handle local lists.  There's also the
 * possibility of a sync option? But that's almost useless */
/* As far as I can tell IGS has no support for on server lists,
 * so that will be the first add */
/* Note that these can't be called if the listing already has
 * the bit set and server side friends lists will have a
 * separate interface */
void NetworkConnection::addFriend(PlayerListing * player)
{
    if(player->friendWatchType == PlayerListing::blocked)
		removeBlock(player);
    else if(player->friendWatchType == PlayerListing::watched)
		removeWatch(player);
    player->friendWatchType = PlayerListing::friended;
	//FIXME presumably they're not already on the list because
	//the popup checked that in constructing the popup menu but...
    friendedList.push_back(new FriendWatchListing(player->name, friendwatch_notify_default));
    emit playerListingReceived(player);
}

void NetworkConnection::removeFriend(PlayerListing * player)
{
    player->friendWatchType = PlayerListing::none;
	std::vector<FriendWatchListing * >::iterator i;
	for(i = friendedList.begin(); i != friendedList.end(); i++)
	{
        if((*i)->name == player->name)
		{
			delete *i;
			friendedList.erase(i);
			break;
		}
	}
    emit playerListingReceived(player);
}

void NetworkConnection::addWatch(PlayerListing * player)
{
    if(player->friendWatchType == PlayerListing::friended)
		removeFriend(player);
    else if(player->friendWatchType == PlayerListing::blocked)
		removeBlock(player);
    player->friendWatchType = PlayerListing::watched;
    watchedList.push_back(new FriendWatchListing(player->name, friendwatch_notify_default));
    emit playerListingReceived(player);
}

void NetworkConnection::removeWatch(PlayerListing * player)
{
    player->friendWatchType = PlayerListing::none;
	std::vector<FriendWatchListing * >::iterator i;
	for(i = watchedList.begin(); i != watchedList.end(); i++)
	{
        if((*i)->name == player->name)
		{
			delete *i;
			watchedList.erase(i);
			break;
		}
	}
    emit playerListingReceived(player);
}

void NetworkConnection::addBlock(PlayerListing * player)
{
    if(player->friendWatchType == PlayerListing::friended)
		removeFriend(player);
    else if(player->friendWatchType == PlayerListing::watched)
		removeWatch(player);
    player->friendWatchType = PlayerListing::blocked;
    blockedList.push_back(new FriendWatchListing(player->name));
}

void NetworkConnection::removeBlock(PlayerListing * player)
{
    player->friendWatchType = PlayerListing::none;
	std::vector<FriendWatchListing * >::iterator i;
	for(i = blockedList.begin(); i != blockedList.end(); i++)
	{
        if((*i)->name == player->name)
		{
			delete *i;
			blockedList.erase(i);
			break;
		}
	}
}

/* This function checks the local lists for a name and sets
 * appropriate flags.  We may also have it do notifications.
 * Other protocol types can override it to, for instance
 * do nothing or just notify, assuming the flags are
 * set when the player is received, rather than looking them
 * up */
/* Another issue: recvPlayerListing is used to change
 * the status of a player, as in which room they're in
 * we don't want it to say "signed on" every time with
 * that.  So we probably need to have an online flag
 * on the friends listing to see if we've already flagged
 * them FIXME */
void NetworkConnection::getAndSetFriendWatchType(PlayerListing * player)
{
	std::vector<FriendWatchListing * >::iterator i;
	
	for(i = friendedList.begin(); i != friendedList.end(); i++)
	{
        if((*i)->name == player->name)
		{
			if(!(*i)->online)
			{
				(*i)->online = true;
                player->friendWatchType = PlayerListing::friended;
				/* We may want to put this somewhere else or have it be dialog
				 * with options, like talk or match.  We might also want
			 	 * to block all notifies while one is in a game FIXME 
			 	 * Also, these messages should be nonblocking !?!?!!!!! FIXME*/
			 	/* And actually, its a big enough deal, i.e., we might want console
			 	 * messages, game blocks, etc., that it makes sense to have some separate
			 	 * class for it that handles the notifications... a notification class... */
#ifdef FIXME
				if((*i)->notify)
					QMessageBox::information(0, tr("Signed on"), tr("%1 has signed on").arg(player.name));
#endif //FIXME
			}
            else if(!player->online)
			{
				(*i)->online = false;
				//they are disconnecting
			}
			return;
		}
	}
	for(i = watchedList.begin(); i != watchedList.end(); i++)
	{
        if((*i)->name == player->name)
		{
            player->friendWatchType = PlayerListing::watched;
			return;
		}
	}
	for(i = blockedList.begin(); i != blockedList.end(); i++)
	{
        if((*i)->name == player->name)
		{
            player->friendWatchType = PlayerListing::blocked;
			return;
		}
	}
    player->friendWatchType = PlayerListing::none;
	return;
}

void NetworkConnection::checkGameWatched(const GameListing *game)
{
	std::vector<FriendWatchListing * >::iterator i;
	for(i = watchedList.begin(); i != watchedList.end(); i++)
	{
        if((*i)->name == game->white_name() || (*i)->name == game->black_name())
		{
			//notifies too often!!! FIXME
			if((*i)->notify)
                QMessageBox::information(0, tr("Match Started!"), tr("Match has started between %1 and %2").arg(game->white_name()).arg(game->black_name()));
			return;
		}
	}
}

void NetworkConnection::drawPleaseWait(void)
{
    QPushButton * cancelConnecting;

    connectingDialog = new QMessageBox(QMessageBox::NoIcon, tr("Please wait"), tr("Connecting..."));
    //connectingDialog->setWindowTitle();
    //connectingDialog->setText();
    cancelConnecting = connectingDialog->addButton(QMessageBox::Cancel);
    connect(cancelConnecting, SIGNAL(clicked()), this, SLOT(slot_cancelConnecting()));
    connectingDialog->show();
    connectingDialog->setMinimumSize(180, 100);
}

/* Slots */
void NetworkConnection::slot_cancelConnecting(void)
{
	userCanceled();
    connectingDialog->deleteLater();
    connectingDialog = 0;
}

void NetworkConnection::OnConnected()
{
}

void NetworkConnection::onAuthenticationNegotiated(void)
{
	setupRoomAndConsole();
	console_dispatch->recvText(QString("Connected"));	//peerAddress returns 0 for some reason FIXME
	//console_dispatch->recvText(QString("Connected to ") + qsocket->peerAddress().toString() + " " +  QString::number(qsocket->peerPort()));
}

void NetworkConnection::onReady(void)
{
    emit ready();
}

void NetworkConnection::OnReadyRead()
{
    handlePendingData();
}

void NetworkConnection::OnConnectionClosed() 
{
	/* Without networkdispatch, this now needs to do something
	 * except FIXME only if there's actually been an error.  like
	 * if we change servers and get disconnected, versus if we
	 * disconnect ourself.*/

	// read last data that could be in the buffer WHY??
	//OnReadyRead();
	//authState = LOGIN;
	//sendTextToApp("Connection closed by foreign host.\n");
	/* I'm not yet sure what the best procedure is, but
	 * presumably this handler occurs when we disconnect
	 * or when there's an error.  So we'll have it delete
	 * the main room which can then, upon deletion, notify
	 * the mainwindow_server code to delete the netdispatch
	 * which we'll delete the network connection... if
	 * there is one */
	//if(default_room_dispatch)
	//	delete default_room_dispatch;
}

/*
// Connection was closed from application, but delayed
void IGSConnection::OnDelayedCloseFinish()
{
	qDebug("DELAY CLOSED FINISHED");
	
	authState = LOGIN;
	sendTextToApp("Connection closed.\n");
}
*/

void NetworkConnection::setState(ConnectionState newState)
{
    connectionState = newState;
    if((newState == CONNECTED) && connectingDialog)
    {
        connectingDialog->deleteLater();
        connectingDialog = NULL;
    }
    emit stateChanged(newState);
}

void NetworkConnection::OnError(QAbstractSocket::SocketError i)
{
	/* FIXME These should pop up information boxes as
	 * well as somehow prevent the even attempted sending
	 * of other msgs? perhaps? like disconnect msgs?*/
    qDebug() << "NetworkConnection::OnError() : TCP socket error: " << qsocket->errorString();
    if(console_dispatch)
        console_dispatch->recvText("TCP socket error: " + qsocket->errorString());
	switch (i)
	{
        case QTcpSocket::ConnectionRefusedError:
            setState(CONN_REFUSED);
			break;
        case QTcpSocket::HostNotFoundError:
            setState(HOST_NOT_FOUND);
			break;
        case QTcpSocket::SocketTimeoutError:
            setState(SOCK_TIMEOUT);
			break;
        case QTcpSocket::RemoteHostClosedError:
            setState(PROTOCOL_ERROR);
			break;
        default:
            setState(UNKNOWN_ERROR);
			break;
	}
	
    if(connectingDialog)
    {
        connectingDialog->deleteLater();
        connectingDialog = 0;
    }
    //sendTextToApp("ERROR - Connection closed.\n"+ qsocket->errorString() );
    //OnReadyRead();
	/* We need to toggle the connection flag, close things up, etc.. */
	
	/* FIXME, we need to also notify the board dispatches, etc., not just ignore it, it should close everything and prevent
	 * those board window, do you want to resign messages */
    if(connectionWidget)	//mainwindow can ignore if loginDialog is open
        connectionWidget->onConnectionError();
}

void NetworkConnection::setupRoomAndConsole(void)
{
    mainwindowroom = new Room(this);
    mainwindowroom->setConnection(this);
    /* Two calls are needed beacause Room has a two step
     * initialization process that should be merged
     * in the constructor FIXME */
	setDefaultRoom(mainwindowroom);
	
	console_dispatch = new ConsoleDispatch(this);
	setConsoleDispatch(console_dispatch);	
	
	loadfriendswatches();
}

void NetworkConnection::tearDownRoomAndConsole(void)
{
    savefriendswatches();
    QMap<unsigned int, class BoardDispatch *>::iterator i;
    for(i = boardDispatchMap.begin(); i != boardDispatchMap.end(); i++)
    {
        (i.value())->setConnection(NULL);
    }
    boardDispatchMap.clear();
    gameDialogMap.clear(); // FIXME probably unnecessary
	
	if(mainwindowroom)
	{
		delete mainwindowroom;
		mainwindowroom = 0;
	}
	if(console_dispatch)
	{
		delete console_dispatch;
		console_dispatch = 0;
	}
}

/* Edit friends/watches list window is created when needed.
 * but we might consider having it always existing and then
 * show hide it?  That's ugly though.  But like these functions
 * should probably be part of a friendsfan class instead of
 * the network connection awkward but perhaps minor FIXME */
void NetworkConnection::loadfriendswatches(void)
{
	QSettings settings;
	int size, i;
	if(!supportsFriendList())
	{
		size = settings.beginReadArray("FRIENDEDLIST");
		for (i = 0; i < size; ++i) 
		{
			settings.setArrayIndex(i);
			friendedList.push_back(new FriendWatchListing(
							settings.value("name").toString(),
							settings.value("notify").toBool()));
		}
 		settings.endArray();
	}
	if(!supportsWatchList())
	{
		size = settings.beginReadArray("WATCHEDLIST");
		for (i = 0; i < size; ++i) 
		{
			settings.setArrayIndex(i);
			watchedList.push_back(new FriendWatchListing(
							settings.value("name").toString(),
							settings.value("notify").toBool()));
		}
 		settings.endArray();
	}
	if(!supportsBlockList())
	{
		size = settings.beginReadArray("BLOCKEDLIST");
		for (i = 0; i < size; ++i) 
		{
			settings.setArrayIndex(i);
			blockedList.push_back(new FriendWatchListing(
							settings.value("name").toString()));
		}
 		settings.endArray();
	}
}

void NetworkConnection::savefriendswatches(void)
{
	QSettings settings;
	int index;
	if(!supportsFriendList())
	{
		settings.beginWriteArray("FRIENDEDLIST");
		std::vector<FriendWatchListing * >::iterator i;
		index = 0;
		for (i = friendedList.begin(); i != friendedList.end(); i++) 
		{
			settings.setArrayIndex(index);
			settings.setValue("name", (*i)->name);
			settings.setValue("notify", (*i)->notify);
			delete (*i);
			index++;
		}
		settings.endArray();
	}
	if(!supportsWatchList())
	{
		settings.beginWriteArray("WATCHEDLIST");
		std::vector<FriendWatchListing * >::iterator i;
		index = 0;
		for (i = watchedList.begin(); i != watchedList.end(); i++) 
		{
			settings.setArrayIndex(index);
			settings.setValue("name", (*i)->name);
			settings.setValue("notify", (*i)->notify);
			delete (*i);
			index++;
		}
		settings.endArray();	
	}
	if(!supportsBlockList())
	{
		settings.beginWriteArray("BLOCKEDLIST");
		std::vector<FriendWatchListing * >::iterator i;
		index = 0;
		for (i = blockedList.begin(); i != blockedList.end(); i++) 
		{
			settings.setArrayIndex(index);
			settings.setValue("name", (*i)->name);
			delete (*i);
			index++;
		}
		settings.endArray();
	}
}

/* Problem if messages come out of order */
void NetworkConnection::latencyOnSend(void)
{
#ifdef LATENCY
	gettimeofday(&latencyLast, NULL);
#endif //LATENCY
}

void NetworkConnection::latencyOnRecv(void)
{
#ifdef LATENCY
	unsigned long ms;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec - latencyLast.tv_sec) * 1000;
	ms += (tv.tv_usec - latencyLast.tv_usec);
	latencyAverage += ms;
	latencyAverage /= 2;
#endif //LATENCY
}

void NetworkConnection::sendConsoleText(const char * text)
{
	//FIXME issue
	if(console_dispatch) 
		console_dispatch->sendText(text);
}

/* I was thinking about breakng up the seeks and rooms by room dispatch, etc.
 * originally, but if we're getting rid of the dispatches and we haven't
 * yet seen anymore interesting use for a room dispatch then, we'll just
 * let the network connection handle it for now. */
 /* Also, I'm not in a hurry to have the sub connections calling mainwindow
  * functions, but it is global and it is awkward to call recvRoomListing within
  * the subconnection FIXME */
void NetworkConnection::recvRoomListing(class RoomListing * r)
{ 
    if(connectionWidget)
        connectionWidget->recvRoomListing(*r, true);
	else
		delete r;
}

void NetworkConnection::recvChannelListing(class ChannelListing * r)
{ 
    if(connectionWidget)
        connectionWidget->recvChannelListing(*r, true);
	else
		delete r;
}

void NetworkConnection::changeChannel(const QString & s)
{
    if(connectionWidget)
        connectionWidget->changeChannel(s);
}
				
void NetworkConnection::recvSeekCondition(class SeekCondition * s)
{
    if(connectionWidget)
        connectionWidget->recvSeekCondition(s);
	else
		delete s;
}

void NetworkConnection::recvSeekCancel(void)
{
    if(connectionWidget)
        connectionWidget->recvSeekCancel();
}

void NetworkConnection::recvSeekPlayer(QString player, QString condition)
{
    if(connectionWidget)
        connectionWidget->recvSeekPlayer(player, condition);
}

const PlayerListing * NetworkConnection::getOurListing(void)
{
    if (ourListing == NULL)
        ourListing = getDefaultRoom()->getPlayerListing(getUsername());
    return ourListing;
}

/* FIXME These are really more like netdispatch type functions, but the registries
 * are on the connection right now.  We might want to move them though at
 * some point if that does seem to make more sense in terms of the namespace. */
BoardDispatch * NetworkConnection::getBoardDispatch(unsigned int game_id)
{
    QMap <unsigned int, BoardDispatch *>::const_iterator i = boardDispatchMap.find(game_id);
    if(i == boardDispatchMap.end())
    {
        // Create if it does not exist
        BoardDispatch * newBoardDispatch = this->getDefaultRoom()->getNewBoardDispatch(game_id);
        boardDispatchMap.insert(game_id, newBoardDispatch);
        return newBoardDispatch;
    }
    else
        return i.value();
}

BoardDispatch * NetworkConnection::getIfBoardDispatch(unsigned int game_id)
{
    QMap <unsigned int, BoardDispatch *>::const_iterator i = boardDispatchMap.find(game_id);
    if(i == boardDispatchMap.end())
        return NULL;
    else
        return i.value();
}

void NetworkConnection::closeBoardDispatch(unsigned int game_id)
{
    QMap <unsigned int, BoardDispatch *>::iterator i = boardDispatchMap.find(game_id);
    if(i == boardDispatchMap.end())
        return;

    delete i.value();
    boardDispatchMap.erase(i);
}

int NetworkConnection::getBoardDispatches(void)
{
    return boardDispatchMap.count();
}

GameDialog * NetworkConnection::getGameDialog(const PlayerListing * opponent)
{
    QMap <const PlayerListing *, GameDialog *>::const_iterator i = gameDialogMap.find(opponent);
    if(i == gameDialogMap.end())
    {
        // Create if it does not exist
        GameDialog * newDialog = new GameDialog(this, opponent);
        gameDialogMap.insert(opponent, newDialog);
        return newDialog;
    }
    else
        return i.value();
}

GameDialog * NetworkConnection::getIfGameDialog(const PlayerListing * opponent)
{
    QMap <const PlayerListing *, GameDialog *>::const_iterator i = gameDialogMap.find(opponent);
    if(i == gameDialogMap.end())
        return NULL;
    else
        return i.value();
}

void NetworkConnection::closeGameDialog(const PlayerListing * opponent)
{
    QMap <const PlayerListing *, GameDialog *>::iterator i = gameDialogMap.find(opponent);
    if(i == gameDialogMap.end())
        return;

    i.value()->deleteLater();
    gameDialogMap.erase(i);
}

MatchRequest * NetworkConnection::getAndCloseGameDialog(const PlayerListing * opponent)
{
	GameDialog * gd = getIfGameDialog(opponent);
	MatchRequest * new_mr = 0;
	if(gd)
	{
		MatchRequest * mr = gd->getMatchRequest();
		new_mr = new MatchRequest(*mr);
		closeGameDialog(opponent);
	}
	else
        qDebug("Couldn't find gamedialog for opponent: %s", opponent->name.toLatin1().constData());
	return new_mr;
}
