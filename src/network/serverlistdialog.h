#include <vector>
#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QDialogButtonBox;

struct ServerItem
{
	char ipaddress[16];
	QString name;
};

class ServerListDialog : public QDialog
{
	Q_OBJECT
	public:
		ServerListDialog(std::vector <ServerItem *> serverlist, int current = -1);
		~ServerListDialog();
	public slots:
		void slot_listDoubleClicked(QTreeWidgetItem *, int);
		void slot_connect();
		void slot_cancel();
	private:
		QTreeWidget * constructTreeWidget(std::vector<ServerItem *> serverlist);

		int current_server;
		QTreeWidget * serverListView;	
		QPushButton * connectButton;
		QPushButton * cancelButton;	
		QDialogButtonBox * buttonBox;
};
