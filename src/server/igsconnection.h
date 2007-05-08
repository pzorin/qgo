/*

 * igsconnection.h

 */

#ifndef IGSCONNECTION_H
#define IGSCONNECTION_H

//#include "igsinterface.h"

#include <QtCore>
#include <QtNetwork>
//class QTextCodec;

#define MAX_LINESIZE 512

class IGSConnection : public QObject //, public IGSInterface
{
	Q_OBJECT

public:
	IGSConnection();
	/*virtual*/ ~IGSConnection();

	// Implementation of IGSInterface virtual functions
	/*virtual*/ bool isConnected();
	/*virtual*/ bool openConnection(const char *host, unsigned port, const char *user=0, const char *pass=0);

	/*virtual*/ bool closeConnection();
	/*virtual*/ void sendTextToHost(QString txt, bool ignoreCodec=false);
	
	/*virtual*/ const char* getUsername();

	/*virtual*/ void setTextCodec(QString codec);


signals:
	// for statistics reason
	void signal_setBytesIn(int);
	void signal_setBytesOut(int);
	void signal_textReceived (const QString&);

protected:
	/*virtual*/ bool checkPrompt();
//	void convertBlockToLines();
	
	void sendTextToApp(QString txt);

private slots:
	void OnHostFound();
	void OnConnected();
	void OnReadyRead();
	void OnConnectionClosed();
//	void OnDelayedCloseFinish();
//	void OnBytesWritten(int);
	void OnError(QAbstractSocket::SocketError);

private:
	QTcpSocket *qsocket;
	QTextCodec *textCodec;

	//struct USERINFO {
	QString username;
	QString password;
//	QString host, /*loginName, password, */codec;
	
	//}  userInfo;

	enum {
		LOGIN,	// parse will search for login prompt
		PASSWORD,	// parse will search for password prompt
		SESSION,	// logged in
		AUTH_FAILED	// wrong user/pass  
	} authState;

	QString bufferLineRest;
};

#endif

