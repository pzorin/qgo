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


#include "consoledispatch.h"
#include "../mainwindow.h"
#include "networkconnection.h"
#include "connectionwidget.h"
/* Probably want to just remove this file FIXME*/

/* All consoles need somewhere to write to... so... MainWindow */
ConsoleDispatch::ConsoleDispatch(NetworkConnection * conn)
{
	connection = conn;
}

ConsoleDispatch::~ConsoleDispatch()
{
	connection->setConsoleDispatch(0);
}

void ConsoleDispatch::recvText(const char * text)
{
    connectionWidget->slot_message(text);
}

void ConsoleDispatch::recvText(QString text)
{
    connectionWidget->slot_message(text);
}

/* This is going to be a problem.  The \r\n is for IGS, but if
 * want to use it as chat... FIXME */
void ConsoleDispatch::sendText(const char * text)
{
	if(connection)
		connection->sendText(QString(text) + "\r\n");
	else
		qDebug("No connection set for console!\n");
}
