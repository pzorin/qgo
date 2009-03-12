#include <QDialog>
#include "ui_login.h"
#include "defines.h"

class HostList;
class MainWindow;
class NetworkConnection;
class QMessageBox;

class LoginDialog : public QDialog, Ui::LoginDialog
{
	Q_OBJECT
	public:
		LoginDialog(const QString &, HostList * h);
	private slots:		//or can these be private?
		void slot_connect(void);
		void slot_cancel(void);
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
		QMessageBox * connectingDialog;
		QPushButton * cancelConnecting;
};
