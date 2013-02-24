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


#include <QDialog>
#include "ui_login.h"
#include "defines.h"

class HostList;
class NetworkConnection;
class QMessageBox;

class LoginDialog : public QDialog, Ui::LoginDialog
{
	Q_OBJECT
	public:
		LoginDialog(const QString &, HostList * h);
	private slots:		//or can these be private?
		void slot_cancel(void);
		void slot_connect(void);
		void slot_editTextChanged(const QString &);
	private:
		ConnectionType serverStringToConnectionType(const QString & s);
		NetworkConnection * newConnection(ConnectionType connType, QString username, QString password);
		QString connectionTypeToServerString(const ConnectionType c);
		Ui::LoginDialog ui;
		ConnectionType connType;
		NetworkConnection * connection;
		HostList * hostlist;
		QString connectionName;
		bool serverlistdialog_open;
};
