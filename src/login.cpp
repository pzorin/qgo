#include <QtGui>
#include "login.h"
#include "mainwindow.h"
#include "mainwindow_settings.h"		//for hostlist
#include "network/networkdispatch.h"

LoginDialog::LoginDialog(const QString & s, HostList * h, MainWindow * m)
{
	ui.setupUi(this);
	connectionName = s;
	mainwindow = m;		//awkward, but otherwise can connect faster than it gets set
	
	connType = serverStringToConnectionType(connectionName);
	setWindowTitle(connectionName);
	
	connect(ui.connectPB, SIGNAL(clicked()), this, SLOT(slot_connect()));
	connect(ui.cancelPB, SIGNAL(clicked()), this, SLOT(slot_cancel()));
	
	hostlist = h;
	int firstloginitem = 1;
	for(HostList::iterator hi = hostlist->begin(); hi != hostlist->end(); hi++)
	{
		if((*hi)->host() == connectionName)
		{
			ui.loginEdit->addItem((*hi)->loginName());
			if(firstloginitem)
			{
				if((*hi)->password() != QString())
				{
					ui.passwordEdit->setText((*hi)->password());
					ui.savepasswordCB->setChecked(true);
				}
				firstloginitem = 0;
			}
		}
	}
	connect(ui.loginEdit, SIGNAL(editTextChanged(const QString &)), this, SLOT(slot_editTextChanged(const QString &)));
}

void LoginDialog::slot_connect(void)
{
	if(ui.loginEdit->currentText() == QString())
	{
		QMessageBox::information(this, tr("Empty Login"), tr("You must enter a username"));
		return;
	}
	netdispatch = new NetworkDispatch(connType, ui.loginEdit->currentText(), ui.passwordEdit->text());	
	/* Its awkward to do this here FIXME, just make sure that, for instance
	 * the connection has the mainwindow set within the network dispatch so its
	 * not passing information into nothing */
	mainwindow->setNetworkDispatch(netdispatch);
	netdispatch->setMainWindow(mainwindow);
	/* We need to wait here to get authorization confirm, no errors,
	   maybe popup either "please wait" dialog, which we'd annoyingly
	   have to handle and then close which might require a separate
	   dialog class, or just have the connect button grayed out or down.*/
	int connectionStatus;
	while((connectionStatus = netdispatch->checkForErrors()) == ND_WAITING)
		QApplication::processEvents(QEventLoop::AllEvents, 300);
	
	if(connectionStatus == ND_BADPASSWORD)
	{
		QMessageBox::information(this, tr("Bad Password"), tr("Invalid Password"));
		return;	
	}
	else if(connectionStatus == ND_BADLOGIN)
	{
		QMessageBox::information(this, tr("Bad Login"), tr("Invalid Login"));
		return;	
	}
	else if(connectionStatus == ND_BADCONNECTION)
	{
		QMessageBox::information(this, tr("Can't connect"), tr("Can't connect to host!"));
		return;	
	}
	else if(connectionStatus == ND_PROTOCOL_ERROR)
	{
		QMessageBox::information(this, tr("Protocol Error"), tr("Notify Developer!"));
		return;	
	}
	else if(connectionStatus == ND_CONNECTED)
	{
		bool foundlogin = false;
		for(HostList::iterator hi = hostlist->begin(); hi != hostlist->end(); hi++)
		{
			if((*hi)->host() == connectionName)
			{
				if((*hi)->loginName() == ui.loginEdit->currentText())
				{
					foundlogin = true;
					if(ui.savepasswordCB->isChecked())
					{
						if((*hi)->password() == ui.passwordEdit->text())
							break;
						else
						{
							delete (*hi);
							hostlist->erase(hi);
							foundlogin = false;
							break;
						}
					}
					else
					{
						if((*hi)->password() == QString())
							break;
						else
						{
							delete (*hi);
							hostlist->erase(hi);
							foundlogin = false;
							break;
						}
					}
					break;
				}
			}
		}
		if(!foundlogin)
		{
			Host * h = new Host(connectionName, ui.loginEdit->currentText(), 
					     (ui.savepasswordCB->isChecked() ? ui.passwordEdit->text() : QString()));
			hostlist->append(h);
		}
	}
	else if(connectionStatus == ND_USERCANCELED)
	{
		/* I think login is now responsible for mediating that
		 * first connection netdispatch, calls mainwindow
		 * which will close the connection.  FIXME, responsibilities
		 * are not clear even if they work out. */
		netdispatch->onError();
	}
	done(1);
}

void LoginDialog::slot_cancel(void)
{
	done(0);
}

void LoginDialog::slot_editTextChanged(const QString & text)
{
	/* This is a little clumsy but its a short list, probably less than 20 */
	for(HostList::iterator hi = hostlist->begin(); hi != hostlist->end(); hi++)
	{
		if((*hi)->host() == connectionName)
		{
			if((*hi)->loginName() == text)
			{
				if((*hi)->password() != QString())
				{
					ui.passwordEdit->setText((*hi)->password());
					ui.savepasswordCB->setChecked(true);
				}
				else
				{
					ui.passwordEdit->setText(QString());
					ui.savepasswordCB->setChecked(false);
				}
				return;
			}
		}
	}
}

ConnectionType LoginDialog::serverStringToConnectionType(const QString & s)
{
	if(s == "IGS")
		return TypeIGS;
	else if(s == "WING")
		return TypeWING;
	else if(s == "LGS")
		return TypeLGS;
	else if(s == "CyberORO")
		return TypeORO;
	else if(s == "Tygem")
		return TypeTYGEM;
	else if(s == "eWeiQi")
		return TypeEWEIQI;
	else if(s == "Tom")
		return TypeTOM;
	else
		return TypeUNKNOWN;
}

QString LoginDialog::connectionTypeToServerString(const ConnectionType c)
{
	switch(c)
	{
		case TypeIGS:
			return "IGS";
			break;
		case TypeWING:
			return "WING";
			break;
		case TypeLGS:
			return "LGS";
			break;
		case TypeORO:
			return "CyberORO";
			break;
		case TypeTYGEM:
			return "Tygem";
			break;
		case TypeEWEIQI:
			return "eWeiQi";
			break;
		case TypeTOM:
			return "Tom";
			break;
		default:
			return "Unknown";
			break;
	}
}
