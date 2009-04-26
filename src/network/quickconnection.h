#ifndef QUICKCONNECTION_H
#define QUICKCONNECTION_H
#include <QtCore>
#include <QtNetwork>

class NetworkConnection;

class QuickConnection : public QObject
{
	Q_OBJECT
	public:
		enum QuickConnectionType { sendRequestAccountInfo, sendAddFriend, sendRemoveFriend };
		QuickConnection(char * host, unsigned short port, void * m, NetworkConnection * c, QuickConnectionType t);
		QuickConnection(QTcpSocket * q, void * m, NetworkConnection * c, QuickConnectionType t);
		int checkSuccess(void);
		~QuickConnection();
	public slots:
		void OnConnected(void);
		void OnReadyRead(void);
		void OnError(QAbstractSocket::SocketError err);
		
	private:
		QTcpSocket * qsocket;
		int success;
		void * msginfo;
		NetworkConnection * connection;
		QuickConnectionType type;
};
#endif //QUICKCONNECTION_H
