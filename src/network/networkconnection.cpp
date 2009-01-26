#include "networkconnection.h"
#include "consoledispatch.h"
#include "roomdispatch.h"
#include "boarddispatch.h"
#include "gamedialogdispatch.h"
#include "talkdispatch.h"
#include "dispatchregistries.h"
#include "playergamelistings.h"

NetworkConnection::NetworkConnection() :
dispatch(0), default_room_dispatch(0), console_dispatch(0), qsocket(0)
{
	protocol_save_int = -1;
	firstonReadyCall = 1;
}

bool NetworkConnection::openConnection(const class ConnectionInfo & info)
{
	boardDispatchRegistry = new BoardDispatchRegistry(this);
	gameDialogDispatchRegistry = new GameDialogDispatchRegistry(this);
	talkDispatchRegistry = new TalkDispatchRegistry(this);
	
	qsocket = new QTcpSocket();	//try with no parent passed for now
	if(!qsocket)
		return 0;
	//connect signals
	
	connect(qsocket, SIGNAL(hostFound()), SLOT(OnHostFound()));
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
	Q_ASSERT(info.host != 0);
	Q_ASSERT(info.port != 0);
	qDebug("Connecting to %s %d...\n", info.host, info.port);
	// assume info.host is clean
	qsocket->connectToHost(info.host, (quint16) info.port);
	connectionInfo = new ConnectionInfo(info);
	
	/* If dispatch does not have a UI, the thing that sets the UI
	 * will setupRoomAndConsole */
	if(dispatch->hasUI())
		dispatch->setupRoomAndConsole();
	/* connectionInfo as a message with those pointers is probably a bad idea */
	return (qsocket->state() != QTcpSocket::UnconnectedState);
}

int NetworkConnection::write(const char * packet, unsigned int size)
{
	if(readyToWrite())
	{
		int len;	
		
       		if ((len = qsocket->write(packet, size)) < 0)
			return len;
	}
	else
		send_buffer.write((unsigned char *)packet, size);
	return 0;
}

/* This is only called by network connection subclasses that
 * want to do things this way. *cough* IGS */
void NetworkConnection::writeFromBuffer(void)
{
	int len, size;
	unsigned char packet[256];	//no commands are more than 255, right now
	while(readyToWrite())
	{
		size = send_buffer.canReadLine();
		if(size > 255)
		{
			//FIXME qWarning, better error?
			qDebug("send buffer line too large !!!!");
			exit(0);
		}
		else if(size == 0)
		{
			setReadyToWrite();
			return;
		}
		send_buffer.readLine(static_cast<unsigned char *>(packet), 255);	
		//size presumably == return value... 
		if ((len = qsocket->write(reinterpret_cast<const char *>(packet), size)) < 0)
			qDebug("Write error!!");	//qWarning FIXME
	}
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
	delete boardDispatchRegistry;
	delete gameDialogDispatchRegistry;
	delete talkDispatchRegistry;
	
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
	dispatch->onClose();
	//dispatch = 0;			//don't set this here, screws up ORO reconnect
	return;
}

NetworkConnection::~NetworkConnection()
{
	/* Dispatch creates the connection so we definitely don't want to delete
	 * it from here */
	//if(dispatch)
	//	delete dispatch;
	delete connectionInfo;
	qDebug("Destroying connection\n");
	//closeConnection();			//specific impl already calls this
	/* Not sure where to delete qsocket.  Possible OnDelayClosedFinish() thing. */
	//delete qsocket;
}

void NetworkConnection::setConsoleDispatch(class ConsoleDispatch * c)
{
	console_dispatch = c;
	if(c)
		console_dispatch->setConnection(this);
}

/* Slots */
void NetworkConnection::OnHostFound()
{
	qDebug("OnHostFound()");
}

void NetworkConnection::OnConnected()
{
	qDebug("OnConnected()");
	/* Invalid read of size 1 here FIXME 
	 * also prints garbage... */
	if(console_dispatch)
		console_dispatch->recvText(QString("Connected to ") + QString(connectionInfo->host) + " " + QString::number(connectionInfo->port));
	
	//sendTextToApp("Connected to " + qsocket->peerAddress().toString() + " " +
	//	  QString::number(qsocket->peerPort()));
}

void NetworkConnection::OnReadyRead()
{
	int bytes = qsocket->bytesAvailable();
	if(bytes > 1)
	{
		unsigned char * packet = new unsigned char[bytes];
		/* If that last byte is a newline... */
		qsocket->read((char *)packet, bytes);
		pending.write(packet, bytes);
		delete[] packet;
		handlePendingData(&pending);
	}
}

void NetworkConnection::OnConnectionClosed() 
{
	qDebug("OnConnectionClosed");

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
/*
void IGSConnection::OnBytesWritten(qint64 nbytes)
{
//	qDebug("%d BYTES WRITTEN", nbytes);
//	emit signal_setBytesOut(nbytes);
}
*/

void NetworkConnection::OnError(QAbstractSocket::SocketError i)
{
	/* FIXME These should pop up information boxes as
	 * well as somehow prevent the even attempted sending
	 * of other msgs? perhaps? like disconnect msgs?*/
	switch (i)
	{
		case QTcpSocket::ConnectionRefusedError: qDebug("ERROR: connection refused...");
			if(console_dispatch)
				console_dispatch->recvText("Error: Connection refused!");
			break;
		case QTcpSocket::HostNotFoundError: qDebug("ERROR: host not found...");
			if(console_dispatch)
				console_dispatch->recvText("Error: Host not found!");
			break;
		case QTcpSocket::SocketTimeoutError: qDebug("ERROR: socket time out ...");
			if(console_dispatch)
				console_dispatch->recvText("Error: Socket time out!");
			break;
		case QTcpSocket::RemoteHostClosedError: qDebug("ERROR: connection closed by host ...");
			if(console_dispatch)
				console_dispatch->recvText("Error: Connection closed by host!");
			break;
		default: qDebug("ERROR: unknown Error...");
			if(console_dispatch)
				console_dispatch->recvText("Error: Unknown socket error!");
			break;
	}

	//sendTextToApp("ERROR - Connection closed.\n"+ qsocket->errorString() );
	qDebug("Socket Error\n");
	//OnReadyRead();
	/* We need to toggle the connection flag, close things up, etc.. */
	
	/* This order is a little iffy.  But the main network dispatch, we're
	 * thinking its best to close that last and that the network connection
	 * can close the other dispatches of which it is aware */
	/* Actually, I think the boarddispatches should be killed by
	 * the boardDispatchRegistry end... which they are... the problem
	 * is setting the connection to 0.*/
	if(dispatch)
		dispatch->onError();
}

void NetworkConnection::recvRoomListing(class RoomListing * r)
{ 
	if(dispatch) 
		dispatch->recvRoomListing(r);
	else
		delete r;
}
				
void NetworkConnection::recvSeekCondition(class SeekCondition * s)
{
	if(dispatch)
		dispatch->recvSeekCondition(s);
	else
		delete s;
}

void NetworkConnection::recvSeekCancel(void)
{
	if(dispatch)
		dispatch->recvSeekCancel();
}

void NetworkConnection::recvSeekPlayer(QString player, QString condition)
{
	if(dispatch)
		dispatch->recvSeekPlayer(player, condition);
}

/* FIXME These are really more like netdispatch type functions, but the registries
 * are on the connection right now.  We might want to move them though at
 * some point if that does seem to make more sense in terms of the namespace. */
BoardDispatch * NetworkConnection::getBoardDispatch(unsigned int game_id)
{
	return boardDispatchRegistry->getEntry(game_id);
}

BoardDispatch * NetworkConnection::getIfBoardDispatch(unsigned int game_id)
{
	return boardDispatchRegistry->getIfEntry(game_id);
}

void NetworkConnection::closeBoardDispatch(unsigned int game_id)
{
	boardDispatchRegistry->deleteEntry(game_id);
}

GameDialogDispatch * NetworkConnection::getGameDialogDispatch(const PlayerListing & opponent)
{
	return gameDialogDispatchRegistry->getEntry(&opponent);
}

GameDialogDispatch * NetworkConnection::getIfGameDialogDispatch(const PlayerListing & opponent)
{
	return gameDialogDispatchRegistry->getIfEntry(&opponent);
}

void NetworkConnection::closeGameDialogDispatch(const PlayerListing & opponent)
{
	gameDialogDispatchRegistry->deleteEntry(&opponent);
}

MatchRequest * NetworkConnection::getAndCloseGameDialogDispatch(const PlayerListing & opponent)
{
	GameDialogDispatch * gd = getIfGameDialogDispatch(opponent);
	MatchRequest * new_mr = 0;
	if(gd)
	{
		MatchRequest * mr = gd->getMatchRequest();
		new_mr = new MatchRequest(*mr);
		closeGameDialogDispatch(opponent);
	}
	else
		qDebug("Couldn't find gamedialog for opponent: %s", opponent.name.toLatin1().constData());
	return new_mr;
}

TalkDispatch * NetworkConnection::getTalkDispatch(const PlayerListing & opponent)
{
	return talkDispatchRegistry->getEntry(&opponent);
}

void NetworkConnection::closeTalkDispatch(const PlayerListing & opponent)
{
	qDebug("deleting %s\n", opponent.name.toLatin1().constData());
	talkDispatchRegistry->deleteEntry(&opponent);
}

BoardDispatch * BoardDispatchRegistry::getNewEntry(unsigned int game_id)
{
	return _c->getDefaultRoomDispatch()->getNewBoardDispatch(game_id);
}

void BoardDispatchRegistry::initEntry(BoardDispatch * boarddispatch)
{
	boarddispatch->setConnection(_c);
}

void BoardDispatchRegistry::onErase(BoardDispatch * boarddispatch)
{
	delete boarddispatch;
}

/* This is because there are several IGS protocol messages
 * that do not list the id of the board so it has to be looked
 * up from other information.
 * This may be the case with other messages we find later, but
 * this is a simple solution. 
 * Fairly simple.  It really hurt my feelings when I realized
 * that the registry template had this nice private registry
 * and I had to add this other function to complicate it.*/
std::map<unsigned int, BoardDispatch *> * BoardDispatchRegistry::getRegistryStorage(void)
{
	return getStorage();
}

GameDialogDispatch * GameDialogDispatchRegistry::getNewEntry(const PlayerListing * opponent)
{
	/* FIXME by moving the registries of dispatches to the dispatch?? */
	return _c->dispatch->_getGameDialogDispatch(*opponent);  //turn to ref FIXME?
}

void GameDialogDispatchRegistry::initEntry(GameDialogDispatch * dlg)
{
	dlg->setConnection(_c);
}

void GameDialogDispatchRegistry::onErase(GameDialogDispatch * dlg)
{
	delete dlg;
}

TalkDispatch * TalkDispatchRegistry::getNewEntry(const PlayerListing * opponent)
{
	return _c->dispatch->getTalkDispatch(*opponent);
}

void TalkDispatchRegistry::initEntry(TalkDispatch * talk)
{
	talk->setConnection(_c);
}
