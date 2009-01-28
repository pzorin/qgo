#ifndef NETWORKDISPATCH_H
#define NETWORKDISPATCH_H
#include "networkconnection.h"
#include "messages.h"

class NetworkConnection;
class RoomDispatch;
class ConsoleDispatch;
class GameDialogDispatch;
class TalkDispatch;
class MainWindow;

class Room;

class NetworkDispatch
{
	public:
		NetworkDispatch(): connection(0), isSubDispatch(true){};
		NetworkDispatch(ConnectionType connType, QString, QString);
		virtual ~NetworkDispatch();
		int checkForErrors(void);
		void onError(void);
		void setupRoomAndConsole(void);
		void setMainWindow(MainWindow * mw);
		bool hasUI(void) { if(mainwindow) return true; return false; };
		void setConsoleDispatch(ConsoleDispatch * dispatch);
		void setDefaultRoomDispatch(RoomDispatch * roomdispatch);
		GameDialogDispatch * getGameDialogDispatch(const PlayerListing & opponent);
		GameDialogDispatch * _getGameDialogDispatch(const PlayerListing & opponent);
		TalkDispatch * getTalkDispatch(const PlayerListing & opponent);
		virtual void setConnection(NetworkConnection * c) { connection = c; };
		virtual void sendText(QString text);
		virtual void sendText(const char * text);
		void sendConsoleText(const char * text);
		void sendToggle(const QString & param, bool val);	//FIXME?? okay?
		virtual void recvText(const char * text);
		virtual void recvText(QString text);
		virtual void recvMsg(const QString & player, const QString & text);
		virtual void onClose(void);
		QString getUsername(void) { return connection->getUsername(); };
		unsigned int rankToScore(QString rank) { return connection->rankToScore(rank); };
		unsigned long getGameDialogFlags(void) { return connection->getGameDialogFlags(); };
		unsigned long getRoomStructureFlags(void) { return connection->getRoomStructureFlags(); };
		bool supportsServerChange(void) { if(connection) return connection->supportsServerChange(); else return false; };
		bool supportsSeek(void) { if(connection) return connection->supportsSeek(); else return false; };
		bool supportsChannels(void) { if(connection) return connection->supportsChannels(); else return false; };
		bool supportsCreateRoom(void) { if(connection) return connection->supportsCreateRoom(); else return false; };
		void changeServer(void) { if(connection) connection->changeServer(); };
		void createRoom(void) { if(connection) connection->createRoom(); };
		void setKeepAlive(int s) { if(connection) connection->setKeepAlive(s); };
		void recvRoomListing(class RoomListing *);
		void sendJoinRoom(const RoomListing &r , const char * p = 0) { connection->sendJoinRoom(r, p); };
		void recvSeekCondition(class SeekCondition * s);
		void recvSeekCancel(void);
		void recvSeekPlayer(QString player, QString condition);
		void sendSeek(class SeekCondition * s) { if(connection) connection->sendSeek(s); };
		void sendSeekCancel(void) { if(connection) connection->sendSeekCancel(); };
	protected:
		NetworkConnection * connection;
	private:	
		bool isSubDispatch;
		MainWindow * mainwindow;
		Room * mainwindowroom;
		ConsoleDispatch * consoledispatch;
};
#endif //NETWORKDISPATCH_H
