#include <QtGui>
#include "serverlistdialog.h"

ServerListDialog::ServerListDialog(std::vector <ServerItem *> serverlist, int current)
{
	current_server = current;
	serverListView = constructTreeWidget(serverlist);
	
	connectButton = new QPushButton(tr("Connect"));
	connectButton->setDefault(true);
	
	cancelButton = new QPushButton(tr("&Cancel"));
	
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(connectButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(cancelButton, QDialogButtonBox::ActionRole);
	
	connect(serverListView, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slot_listDoubleClicked(QTreeWidgetItem *, int)));
	
	connect(connectButton, SIGNAL(clicked()), this, SLOT(slot_connect()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancel()));
	
	QGridLayout * mainLayout = new QGridLayout;
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(serverListView, 0, 0);
	mainLayout->addWidget(buttonBox, 1, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Choose server..."));
}

ServerListDialog::~ServerListDialog()
{
	delete serverListView;
	delete connectButton;
	delete cancelButton;
	delete buttonBox;
}

QTreeWidget * ServerListDialog::constructTreeWidget(std::vector<ServerItem *> serverlist)
{
	QTreeWidget * treeWidget = new QTreeWidget(this);
	QTreeWidgetItem * i;
	QStringList stringList;
	int index = 0;
	
	treeWidget->setRootIsDecorated(false);
	treeWidget->setColumnCount(2);
	stringList << "Name" << "IP Address";
	treeWidget->setHeaderLabels(stringList);
	stringList.clear();
	
	std::vector<ServerItem *>::iterator it;
	for(it = serverlist.begin(); it != serverlist.end(); it++)
	{
		stringList << (*it)->name << (*it)->ipaddress;
		i = new QTreeWidgetItem(treeWidget, stringList);
		if(index == current_server)
			i->setDisabled(true);
		if(it == serverlist.begin())	//ugly but its a short list
			treeWidget->setCurrentItem(i);
		stringList.clear();
		index++;
	}
	return treeWidget;
}

void ServerListDialog::slot_listDoubleClicked(QTreeWidgetItem * i, int)
{
	int index = serverListView->indexOfTopLevelItem(i);
	if(index < 0 || index == current_server)
		return;
	done(index);
}

void ServerListDialog::slot_connect(void)
{
	QTreeWidgetItem * i = serverListView->currentItem();
	if(!i)
	{
		//FIXME error message necessary?
		return;
	}
	int index = serverListView->indexOfTopLevelItem(i);
	done(index);
}

void ServerListDialog::slot_cancel(void)
{
	qDebug("Cancel");
	deleteLater();
	done(-1);
}
