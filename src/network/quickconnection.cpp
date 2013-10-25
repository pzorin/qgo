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


#include "quickconnection.h"
#include "networkconnection.h"

QuickConnection::QuickConnection(QString hostname, qint16 port, void * m, NetworkConnection * c, QuickConnectionType t) :
msginfo(m), connection(c), type(t)
{
	qsocket = new QTcpSocket();	//try with no parent passed for now
	if(!qsocket)
	{
		success = 0;
		return;
	}
	
	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));
	
    qDebug() << QString("QuickConnection::QuickConnection : Connecting to") << hostname << ":" << port;
	
    qsocket->connectToHost(hostname, port);
	
	success = !(qsocket->state() != QTcpSocket::UnconnectedState);
}

QuickConnection::QuickConnection(QTcpSocket * q, void * m, NetworkConnection * c, QuickConnectionType t) :
qsocket(q), msginfo(m), connection(c), type(t)
{
	/* FIXME Can we even do this?  Switch signals while
	 * they're running? */
	disconnect(qsocket, SIGNAL(hostFound()), connection, 0);
	disconnect(qsocket, SIGNAL(connected()), connection, 0);
	disconnect(qsocket, SIGNAL(readyRead()), connection, 0);
	disconnect(qsocket, SIGNAL(disconnected ()), connection, 0);
	disconnect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), connection, 0);

	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));
	
	success = !(qsocket->state() != QTcpSocket::UnconnectedState);
	if(success != 0)
	{
		qDebug("QC passed bad connection");
		return;
	}
	OnConnected();
}

int QuickConnection::checkSuccess(void)
{
	return success;
}

void QuickConnection::OnConnected(void)
{
	int size;
	char * packet = 0;
	//qDebug("QC: OnConnected()");
	switch(type)
	{
		case sendRequestAccountInfo:
			packet = connection->sendRequestAccountInfo(&size, msginfo);
			break;
		case sendAddFriend:
			packet = connection->sendAddFriend(&size, msginfo);
			break;
		case sendRemoveFriend:
			packet = connection->sendRemoveFriend(&size, msginfo);
			break;
		case sendAddBlock:
			packet = connection->sendAddFriend(&size, msginfo);
			break;
		case sendRemoveBlock:
			packet = connection->sendRemoveFriend(&size, msginfo);
			break;
	}
	if (qsocket->write(packet, size) < 0)
		qDebug("QuickConnection write failed");
	delete[] packet;
}

void QuickConnection::OnReadyRead(void)
{
	int bytes = qsocket->bytesAvailable();
	if(bytes > 1)
	{
		char * packet = new char[bytes];
		qsocket->read(packet, bytes);
		switch(type)
		{
			case sendRequestAccountInfo:
				connection->handleAccountInfoMsg(bytes, packet);
				break;
			case sendAddFriend:
			case sendRemoveFriend:
			case sendAddBlock:
			case sendRemoveBlock:
				connection->recvFriendResponse(bytes, packet);
				break;
		}
		delete[] packet;
	}
}

void QuickConnection::OnError(QAbstractSocket::SocketError err)
{
	switch(err)
	{
        case QTcpSocket::ConnectionRefusedError: qDebug("QuickConnection::OnError : connection refused");
			break;
        case QTcpSocket::HostNotFoundError: qDebug("QuickConnection::OnError : host not found");
			break;
        case QTcpSocket::SocketTimeoutError: qDebug("QuickConnection::OnError : socket time out");
			break;
        case QTcpSocket::RemoteHostClosedError: qDebug("QuickConnection::OnError : connection closed by host");
			break;
        default: qDebug("QuickConnection::OnError : unknown error");
			break;
	}
}

QuickConnection::~QuickConnection()
{
	if(qsocket)
	{
		qsocket->close();
		qsocket->deleteLater();
	}
}
