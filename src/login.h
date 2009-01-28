#include <QDialog>
#include "ui_login.h"
#include "defines.h"

class HostList;
class MainWindow;
class NetworkDispatch;

class LoginDialog : public QDialog, Ui::LoginDialog
{
	Q_OBJECT
	public:
		LoginDialog(const QString &, HostList * h, MainWindow * m);
	private slots:		//or can these be private?
		void slot_connect(void);
		void slot_cancel(void);
		void slot_editTextChanged(const QString &);
	private:
		ConnectionType serverStringToConnectionType(const QString & s);
		QString connectionTypeToServerString(const ConnectionType c);
		Ui::LoginDialog ui;
		ConnectionType connType;
		NetworkDispatch * netdispatch;
		HostList * hostlist;
		QString connectionName;
		MainWindow * mainwindow;
};
