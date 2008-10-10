#include <vector>
#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QDialogButtonBox;

struct ServerItem
{
	char ipaddress[16];
	char name[18];
};

class ServerListDialog : public QDialog
{
	Q_OBJECT
	public:
		ServerListDialog(std::vector <ServerItem *> serverlist);
		~ServerListDialog();
	public slots:
		void slot_listDoubleClicked(QTreeWidgetItem *, int);
		void slot_connect();
		void slot_cancel();
	private:
		QTreeWidget * constructTreeWidget(std::vector<ServerItem *> serverlist);

		
		QTreeWidget * serverListView;	
		QPushButton * connectButton;
		QPushButton * cancelButton;	
		QDialogButtonBox * buttonBox;
};
