#include "networkdispatch.h"
#include "connectiontypes.h"
#include "igsconnection.h"
#include "wing.h"
#include "lgs.h"
#include "cyberoroconnection.h"
#include "tygemconnection.h"
#include "eweiqiconnection.h"
#include "tomconnection.h"
#include "consoledispatch.h"
#include "roomdispatch.h"
#include "gamedialogdispatch.h"
#include "talkdispatch.h"
#include "room.h"
#include "../mainwindow.h"

NetworkDispatch::NetworkDispatch(ConnectionType connType, QString username, QString password)
{
	mainwindow = 0;
	mainwindowroom = 0;
	consoledispatch = 0;
	isSubDispatch = false;	//only main dispatch deletes connection
	switch(connType)	
	{
		case TypeIGS:
			connection = new IGSConnection(this, username, password);
			break;
		case TypeORO:
			connection = new CyberOroConnection(this, username, password);
			break;
		case TypeWING:
			connection = new WingConnection(this, username, password);
			break;
		case TypeLGS:
			connection = new LGSConnection(this, username, password);
			break;
		case TypeTYGEM:
			connection = new TygemConnection(this, username, password);
			break;
		case TypeEWEIQI:
			connection = new EWeiQiConnection(this, username, password);
			break;
		case TypeTOM:
			connection = new TomConnection(this, username, password);
			break;
		default:
			qDebug("Bad connection Type");
			// ERROR handling???
			break;
	}
}

/* Should be more complex error checking here,
 * maybe we even wait until socket is complete
 * or return a "waiting" error? At the very
 * least this will ensure the connection
 * exists everywhere.
 * Well, we made it more complex, but now
 * name should probably be changed. */
int NetworkDispatch::checkForErrors(void)
{
	if(!connection)
		return ND_BADCONNECTION;
	return connection->getConnectionState();
}

void NetworkDispatch::setMainWindow(MainWindow * mw)
{
	mainwindow = mw;
	
	setupRoomAndConsole();
}

void NetworkDispatch::setupRoomAndConsole(void)
{
	qDebug("setupRoomAndConsole");
	mainwindowroom = new Room();
	setDefaultRoomDispatch(mainwindowroom->getDispatch());
	
	consoledispatch = new ConsoleDispatch(mainwindow);
	setConsoleDispatch(consoledispatch);	
}

void NetworkDispatch::sendText(const char * text)
{
	qDebug("send text: %s\n", text);
	connection->sendText(text);
}

void NetworkDispatch::sendText(QString text)
{
	qDebug("send text: %s\n", text.toLatin1().constData());
	connection->sendText(text);
}

void NetworkDispatch::sendConsoleText(const char * text)
{
	if(consoledispatch) 
		consoledispatch->sendText(text);
}

void NetworkDispatch::sendToggle(const QString & param, bool val)
{
	connection->sendToggle(param, val);
}

/* Below should maybe not be here, i.e., should be specifically
 * implemented by dispatch subclass.  This is just to test, which is silly */
void NetworkDispatch::recvText(const char * text)
{
	qDebug("recv text: %s\n", text);
}

void NetworkDispatch::recvText(QString /*text*/)
{
	/* FIXME This is apparently unused !!! */
	qDebug("FIXME %d", __LINE__);
	//qDebug(text);
}

void NetworkDispatch::recvMsg(const QString & player, const QString & text)
{
	RoomDispatch * room;
	/* For right now, we'll pass this to the default room
	 * dispatch, just because its closer to the mainwindow code
	 * but I'm not sure it really belongs there, it might be
	 * better here.  After all, anyone on a server could
	 * contact you.  It might be best to talk the slot_talk
	 * out of the mainwindow code somehow and then we could
	 * link that up with this. But for now: */
	room = connection->getDefaultRoomDispatch();
	room->recvMsg(player, text);
}

void NetworkDispatch::setConsoleDispatch(class ConsoleDispatch * dispatch)
{
	connection->setConsoleDispatch(dispatch);
}

void NetworkDispatch::setDefaultRoomDispatch(class RoomDispatch * roomdispatch)
{
	/* Its unfortunate that we set the connection here but
	 * might be necessary just for the default rooms.  The others
	 * can be set on room creation time */
	if(roomdispatch)
		roomdispatch->setConnection(connection);
	connection->setDefaultRoomDispatch(roomdispatch);
}

/* This is a GIANT Reason why the registries should be on the dispatch */
/* This needs to create a new game dialog.  Its called
 * from the connection which registers it */
GameDialogDispatch * NetworkDispatch::_getGameDialogDispatch(const PlayerListing & opponent)
{
	/* Its way too easy to call this from someplace like parser for instance,
	 * accidentally: connection->getGDD(), dispatch->getGDD();
	 * thus bypassing the gdd registry and potentially wreaking havoc. FIXME */
	return new GameDialogDispatch(connection, opponent);
}

GameDialogDispatch * NetworkDispatch::getGameDialogDispatch(const PlayerListing & opponent)
{
	return connection->getGameDialogDispatch(opponent);
}

TalkDispatch * NetworkDispatch::getTalkDispatch(const PlayerListing & opponent)
{
	/* I don't want the talk windows to be slaved to a room, 
	 * but at the same time, currently its done in the mainwindow,
	 * so we'll have two different functions with a possible
	 * room dispatch that can be notified by the talkdispatch */
	RoomDispatch * room = connection->getDefaultRoomDispatch();
	if(room)
		return new TalkDispatch(connection, opponent, room);
	else
		return new TalkDispatch(connection, opponent);
}

void NetworkDispatch::recvRoomListing(class RoomListing * r)
{
	if(!mainwindow)
		return;
	mainwindow->recvRoomListing(*r, true);
}

void NetworkDispatch::recvSeekCondition(class SeekCondition * s)
{
	if(mainwindow)
		mainwindow->recvSeekCondition(s);
	else
		delete s;
}

void NetworkDispatch::recvSeekCancel(void)
{
	if(mainwindow)
		mainwindow->recvSeekCancel();
}

void NetworkDispatch::recvSeekPlayer(QString player, QString condition)
{
	if(mainwindow)
		mainwindow->recvSeekPlayer(player, condition);
}

// this should probably also be able to handle errors
void NetworkDispatch::onClose(void)
{
	//RoomDispatch * room;
	ConsoleDispatch * console;
	/* This needs to close all open dispatches */
	/*room = connection->getDefaultRoomDispatch();
	if(room)
	{
		connection->setDefaultRoomDispatch(0);
		delete room;
		//FIXME mainwindowroom = 0;
	}*/
	if(mainwindowroom)
	{
		delete mainwindowroom;
		mainwindowroom = 0;
	}
	console = connection->getConsoleDispatch();
	if(console)
	{
		connection->setConsoleDispatch(0);
		delete console;
		//FIXME consoledispatch = 0;
	}
}

/* We also should probably check and delete any talk, gamedialog or
 * board dispatches that are still open FIXME.  Maybe this should
 * be part of the registry deletion code.*/
NetworkDispatch::~NetworkDispatch()
{
	//this could be a subdispatch so be careful what we delete
	if(isSubDispatch)
		return;
	qDebug("Deconstructing true network dispatch");
	if(connection)
	{
		/* We can't call onClose because "this" might
		 * already be gone?!? FIXME */
		//onClose();
		//destructor closes
		//connection->closeConnection();	
		delete connection;
	}
}

void NetworkDispatch::onError(void)
{
	/* Right now, we just need to notify the mainwindowroom, but I guess
	 * it would be okay to notify all rooms, eventually, each registered room 
	 * FIXME */
	qDebug("ND::onError");
	//RoomDispatch * room = connection->getDefaultRoomDispatch();
	//if(room)
	//	room->onError();
	if(mainwindowroom)
		mainwindowroom->onError();
	/* We could also have the rooms deleted here. */
	//delete this;
	if(mainwindow)
		mainwindow->onConnectionError();
}
