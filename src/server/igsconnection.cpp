 /* 
 * igsconnection.cpp
 * 	
 * insprired by qigs
 *
 * authors: Peter Strempel & Tom Le Duc & Johannes Mesa
 *
 * this code is still in experimentational phase
 */


#include "igsconnection.h"

#include <QtNetwork>

IGSConnection::IGSConnection() //: IGSInterface()
{ 
	authState = LOGIN;

	qsocket = new QTcpSocket(this);
	Q_CHECK_PTR(qsocket);

	textCodec = QTextCodec::codecForLocale();//0;

	username = "";
	password = "";
	
	connect(qsocket, SIGNAL(hostFound()), SLOT(OnHostFound()));
	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(disconnected ()), SLOT(OnConnectionClosed()));
//	connect(qsocket, SIGNAL(delayedCloseFinished()), SLOT(OnDelayedCloseFinish()));
//	connect(qsocket, SIGNAL(bytesWritten(qint64)), SLOT(OnBytesWritten(qint64)));
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));

}


IGSConnection::~IGSConnection()
{
//	delete qsocket;
}

// maybe callback should be callback(void)...,
// any data can be transferred after signalling with normal functions
void IGSConnection::sendTextToApp(QString txt)
{
//	if (callbackFP != NULL)
//		(*callbackFP)(txt);

	emit signal_textReceived ( txt);
}

/*
 * check for 'Login:' (and Password)
 */
bool IGSConnection::checkPrompt()
{
	// prompt can be login prompt or usermode prompt
		
	if (bufferLineRest.length() < 4)
	{
		qDebug("IGSConnection::checkPrompt called with string of size %i", bufferLineRest.length());

		if (authState == PASSWORD)
		{
			int b = qsocket->bytesAvailable();
			if (!b)
			{
				qsocket->waitForReadyRead(500);
				b = qsocket->bytesAvailable();
				if (!b)
					return false;
			}
			char * c = new char[b +1];
			qsocket->read(c, b);
//			while (b-- > 0)	
//				if ( qsocket->getChar ( c ))
					bufferLineRest.append(c);// += qsocket->getch();
		}
		else
			return false; 
	}

	switch (authState)
	{
		case LOGIN:
			if (bufferLineRest.indexOf("Login:") != -1)
			{
				qDebug("Login: found\n");
				
				// tell application what to send
				sendTextToApp(bufferLineRest);
				if (!username.isEmpty())
				{
					sendTextToApp("...sending: {" + QString(username) + "}");
				
					sendTextToHost(username, true);// + '\n');       // CAREFUL : THIS SOMETIMES CHANGES on IGS
				}

				// next state
				if (!password.isEmpty())
					authState = PASSWORD;
				else
					authState = SESSION;
				return true;
			}
			break;

		case PASSWORD:
			if ((bufferLineRest.indexOf("Password:") != -1) || (bufferLineRest.indexOf("1 1") != -1))
			{
				qDebug("Password or 1 1: found , strlen = %d\n", bufferLineRest.length());
				sendTextToApp(tr("...send password"));
				sendTextToHost(password, true);

				// next state
				authState = SESSION;
				return true;
			}
			else if (bufferLineRest.indexOf("guest account") != -1)
			{
				authState = SESSION;
				return true;
			}
			break;

		default: 
			break;
	}
	
	return false;
}


/*
 * Slots
 */
void IGSConnection::OnHostFound()
{
	qDebug("IGSConnection::OnHostFound()");
}

void IGSConnection::OnConnected()
{
	qDebug("IGSConnection::OnConnected() \n");

	sendTextToApp("Connected to " + qsocket->peerAddress().toString() + " " +
		  QString::number(qsocket->peerPort()));
}

// We got data to read
void IGSConnection::OnReadyRead()
{
	int size;
//	qDebug("OnReadyRead....");
	while (qsocket->canReadLine())
	{
		size = qsocket->bytesAvailable() + 1;
		char *c = new char[size];
		/*int bytes =*/ qsocket->readLine(c, size);
		QString x = textCodec->toUnicode(c);
		delete[] c;
		if (x.isEmpty())
		{
			qDebug("READ:NULL");
			return;
		}

		// some statistics
//		emit signal_setBytesIn(x.length());

		x.truncate(x.length()-2);

		sendTextToApp(x);

		if (authState == PASSWORD)
		{
			bufferLineRest = x;
			checkPrompt();
			qDebug("PASSWORD***");
		}
	}
	
//	char *cc ;
	size = qsocket->bytesAvailable() ;
	char *cc = new char[size +1];
	if (authState == LOGIN && size > 0)//qsocket->bytesAvailable() == 7)
	{
//		QString y = "";
		qDebug("looking for \'Login:\'");	
//		char *cc;
//		while (qsocket->bytesAvailable())
//		{
//			if (qsocket->getChar( cc ))
//				y.append(cc);// += qsocket->getch();
//		}
		qsocket->read(cc, size);
		QString y = QString(cc);		

		if (!y.isEmpty())
		{
			qDebug("Collected: %s" ,y.toLatin1().constData());
			bufferLineRest = y;
			checkPrompt();
		}
	}

	
//	if (authState == LOGIN && qsocket->bytesAvailable() == 7)



//	convertBlockToLines();
}

/*
 * Connection was closed from host
 */
void IGSConnection::OnConnectionClosed() 
{
	qDebug("CONNECTION CLOSED BY FOREIGN HOST");

	// read last data that could be in the buffer
	OnReadyRead();
	authState = LOGIN;
	sendTextToApp("Connection closed by foreign host.\n");
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

void IGSConnection::OnError(QAbstractSocket::SocketError )
{
/*	switch (i)
	{
		case QTcpSocket::ConnectionRefusedError: qDebug("ERROR: connection refused...");
			break;
		case QTcpSocket::HostNotFoundError: qDebug("ERROR: host not found...");
			break;
		case QTcpSocket::SocketTimeoutError: qDebug("ERROR: socket time out ...");
			break;
		case QTcpSocket::RemoteHostClosedError: qDebug("ERROR: connection closed by host ...");
			break;
		default: qDebug("ERROR: unknown Error...");
			break;
	}
*/
	sendTextToApp("ERROR - Connection closed.\n"+ qsocket->errorString() );
}


/*
 * Functions called from the application
 */
bool IGSConnection::isConnected()
{
	qDebug("IGSConnection::isConnected()");
	return qsocket->state() == QTcpSocket::ConnectedState;
}

void IGSConnection::sendTextToHost(QString txt, bool ignoreCodec)
{
	/*
	*	This is intended because for weird reasons, if 'username' s given
	*	with a codec, IGS might reject it and ban the IP (!!!) for several hours.
	*	This seems to concern only windows users.
	*	Therefore, we pretend to ignore the codec when passing username or password
	*/

	if (ignoreCodec)
	{

        	int len;
		const char * txt2 = txt.toLatin1();

        	if ((len = qsocket->write(txt2, strlen(txt2) * sizeof(char))) != -1)
			qsocket->write("\r\n", 2);
		else
			qWarning("*** failed sending to host: %s", txt2);
	}

	else 
	{
		QByteArray raw = textCodec->fromUnicode(txt);

		if (qsocket->write(raw.data(), raw.size() ) != -1)
			qsocket->write("\r\n", 2);
		else
			qWarning("*** failed sending to host: %s", txt.toLatin1().constData());
	}
}


void IGSConnection::setTextCodec(QString codec)
{
	textCodec = QTextCodec::codecForName(codec.toLatin1());
	if(!textCodec)
		textCodec = QTextCodec::codecForLocale();
	Q_CHECK_PTR(textCodec);
}

/*
 * called by the main window when the connection button is pressed down
 */
bool IGSConnection::openConnection(const char *host, unsigned int port, const char *user, const char *pass)
{
	Q_ASSERT(qsocket);
	if (qsocket->state() != QTcpSocket::UnconnectedState ) 
	{
		qDebug("Called IGSConnection::openConnection while in state %d", qsocket->isValid());
		return false;
	}

	//textCodec = QTextCodec::codecForName(codec);
	//if(!textCodec)
	//	textCodec = QTextCodec::codecForLocale();
	//CHECK_PTR(textCodec);

	username = user;
	password = pass;
	
	qDebug("Connecting to %s %d as [%s], [%s]...", host, port, username.toLatin1().constData(), (!password.isEmpty() ? "***" : "NULL"));
	sendTextToApp(tr("Trying to connect to %1 %2").arg(host,port));

	Q_ASSERT(host != 0);
	Q_ASSERT(port != 0);
	int len = qstrlen(host);
	if ((len > 0) && (len < 200)) // otherwise host points to junk
		qsocket->connectToHost(host, (quint16) port);

	return qsocket->state() != QTcpSocket::UnconnectedState;
}

/*
 *  called by the main window when the connection button is pressed up
 */
bool IGSConnection::closeConnection()
{
	// We have no connection?
	if (qsocket->state() == QTcpSocket::UnconnectedState)
		return false;

	qDebug("Disconnecting...");

	// Close it.
	qsocket->close();

	// Closing succeeded, return message
	if (qsocket->state() == QTcpSocket::UnconnectedState)
	{
		authState = LOGIN;
		sendTextToApp("Connection closed.\n");
	}

	// Not yet closed? Data will be written and then slot OnDelayClosedFinish() called
	return true;
}

const char* IGSConnection::getUsername ()
{
	return (const char*) username.toLatin1().constData() ;
}

