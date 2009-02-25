#include "consoledispatch.h"
#include "../mainwindow.h"
#include "networkconnection.h"
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
	mainwindow->slot_message(text);	
}

void ConsoleDispatch::recvText(QString text)
{
	mainwindow->slot_message(text);	
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
