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


#include <QtWidgets>
#include "login.h"
#include "host.h"
#include "network/igsconnection.h"
#include "network/wing.h"
#include "network/lgs.h"
#include "network/cyberoroconnection.h"
#include "network/tygemconnection.h"
#include "network/eweiqiconnection.h"
#include "network/tomconnection.h"
#include "connectionwidget.h"

const char * connectionTypeString[] = {"IGS", "WING", "LGS", "CyberORO", "Tygem", "eWeiQi", "Tom",
                                       "Default", "None", "Unknown",
                                       "NNGS", "CTN", "CWS",};

enum CredentialTableColumn { CRED_PROTOCOL, CRED_HOSTNAME, CRED_PORT, CRED_USERNAME, CRED_PASSWORD, CRED_NUMITEMS };

CredentialTableModel::CredentialTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    editable = true;
    loadCredentials();
}

CredentialTableModel::~CredentialTableModel() {}

int CredentialTableModel::rowCount(const QModelIndex &) const
{
    return credentials.size();
}

int CredentialTableModel::columnCount(const QModelIndex &) const
{
    return CRED_NUMITEMS;
}

QModelIndex CredentialTableModel::index ( int row, int column, const QModelIndex &) const
{
    return createIndex(row,column);
}

QVariant CredentialTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    if ((role != Qt::DisplayRole) && (role != Qt::EditRole))
        return QVariant();
    switch (index.column())
    {
    case CRED_PROTOCOL:
        if (role == Qt::DisplayRole)
            return QString(connectionTypeString[int(credentials[index.row()].type)]);
        else
            return QVariant(int(credentials[index.row()].type));
    case CRED_HOSTNAME:
        return credentials[index.row()].hostName;
    case CRED_PORT:
        return QVariant(credentials[index.row()].port);
    case CRED_USERNAME:
        return credentials[index.row()].userName;
    case CRED_PASSWORD:
        if ((role == Qt::DisplayRole) && ( ! credentials[index.row()].password.isEmpty() ))
            return QString::fromUtf8( "\u2022\u2022\u2022\u2022\u2022" );
        else
            return credentials[index.row()].password;
    default:
        return QVariant();
    }
}

QVariant CredentialTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch(section)
        {
        case CRED_PROTOCOL:
            return QVariant("Protocol");
        case CRED_HOSTNAME:
            return QVariant("Hostname");
        case CRED_PORT:
            return QVariant("Port");
        case CRED_USERNAME:
            return QVariant("Username");
        case CRED_PASSWORD:
            return QVariant("Password");
        }
    return QVariant();
}

Qt::ItemFlags CredentialTableModel::flags(const QModelIndex &) const
{
    if (editable)
        return Qt::ItemIsEnabled | Qt::ItemIsEditable;
    else
        return Qt::ItemIsEnabled;
}

bool CredentialTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    int row = index.row();
    int column = index.column();
    if ((row < 0) || (row >= credentials.size()) || (role != Qt::EditRole))
        return false;

    bool ok;
    int val;
    switch (column)
    {
    case CRED_PROTOCOL:
        val = value.toInt(& ok);
        if ((!ok) || (val<0) || (val >= int(TypeDEFAULT)))
            return false;
        credentials[row].type = ConnectionType(val);
        break;
    case CRED_HOSTNAME:
        credentials[row].hostName = value.toString();
        break;
    case CRED_PORT:
        val = value.toInt(& ok);
        if ((!ok) || (val<0) || (val > 0xFFFF))
            return false;
        credentials[row].port = (qint16)val;
        break;
    case CRED_USERNAME:
        credentials[row].userName = value.toString();
        break;
    case CRED_PASSWORD:
        credentials[row].password = value.toString();
        break;
    default:
        return false;
    }
    emit dataChanged(index,index);
    return true;
}

bool CredentialTableModel::removeRows(int row, int count, const QModelIndex &)
{
    beginRemoveRows(QModelIndex(),row,row+count-1);
    for(;count>0;count--)
        credentials.removeAt(row);
    endRemoveRows();
    return true;
}


void CredentialTableModel::addCredential(ConnectionCredentials cred)
{
    int row = credentials.size();
    beginInsertRows(QModelIndex(),row,row);
    credentials.append(cred);
    endInsertRows();
}

void CredentialTableModel::loadCredentials(void)
{
    QSettings settings;
    int size = settings.beginReadArray("CREDENTIALS");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        int j = 0;
        for (;j < int(TypeDEFAULT); j++)
        {
            if (settings.value("type").toString() == QString(connectionTypeString[j]))
                break;
        }
        if (j == int(TypeDEFAULT))
            break;
        // We should probably do some sanity checks here
        // although this is all user-changeable in the GUI, so bad values should be handled gracefully
        credentials.append(ConnectionCredentials((ConnectionType)(j),
                                                 settings.value("host").toString(),
                                                 (qint16)(settings.value("port").toInt()),
                                                 settings.value("user").toString(),
                                                 settings.value("password").toString()));
    }
    settings.endArray();
}

void CredentialTableModel::saveCredentials(void)
{
    QSettings settings;
    settings.beginWriteArray("CREDENTIALS");
    for (int i = 0; i < credentials.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("type", connectionTypeString[credentials.at(i).type]);
        settings.setValue("host", credentials.at(i).hostName);
        settings.setValue("port", credentials.at(i).port);
        settings.setValue("user", credentials.at(i).userName);
        settings.setValue("password", credentials.at(i).password);
    }
    settings.endArray();

}

ComboBoxDelegate::ComboBoxDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex &/* index */) const
 {
     QComboBox *editor = new QComboBox(parent);
     for (int i=0; i <= (int)TypeDEFAULT; i++)
         editor->addItem(connectionTypeString[i]);
     return editor;
 }

void ComboBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
 {
     int value = index.model()->data(index, Qt::EditRole).toInt();

     QComboBox *comboBox = static_cast<QComboBox*>(editor);
     comboBox->setCurrentIndex(value);
 }

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
 {
     QComboBox *comboBox = static_cast<QComboBox*>(editor);

     model->setData(index, comboBox->currentIndex(), Qt::EditRole);
 }

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
    newCredMenu = new QMenu("New credentials...",this);
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("IGS"),ConnectionCredentials(TypeIGS,"igs.joyjoy.net", 7777,"",""),(QObject*)this)));
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("WING"),ConnectionCredentials(TypeWING,"wing.gr.jp", 1515,"",""),(QObject*)this)));
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("LGS"),ConnectionCredentials(TypeLGS,"lgs.taiwango.net", 9696,"",""),(QObject*)this)));
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("CyberORO"),ConnectionCredentials(TypeORO,"211.38.95.221", 7447,"",""),(QObject*)this)));
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("Tygem"),ConnectionCredentials(TypeTYGEM,"121.189.9.52", 80,"",""),(QObject*)this)));
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("eWeiQi"),ConnectionCredentials(TypeEWEIQI,"121.189.9.52", 80,"",""),(QObject*)this)));
    newCredMenu->addAction((QAction*)(new NewCredentialsAction(QString("Tom"),ConnectionCredentials(TypeTOM,"61.135.158.147", 80,"",""),(QObject*)this)));
    credModel = new CredentialTableModel(this);
    slot_toggleEditable(); // Sets "editable" to false
    ui.credentialView->setModel((QAbstractTableModel*)credModel);
    ui.credentialView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.credentialView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.credentialView->setItemDelegateForColumn(0,new ComboBoxDelegate(this));
    ui.credentialView->setColumnWidth(CRED_PROTOCOL,80);
    ui.credentialView->setColumnWidth(CRED_PORT,50);
    connect(ui.editButton, SIGNAL(clicked()), this, SLOT(slot_toggleEditable()));
    connect(ui.credentialView, SIGNAL(activated(QModelIndex)), this, SLOT(slot_connect(QModelIndex)));
    connect(newCredMenu, SIGNAL(triggered(QAction *)), this, SLOT(slot_newCredential(QAction *)));
    ui.credentialView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.credentialView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopup(const QPoint &)));
}

LoginDialog::~LoginDialog()
{
    delete newCredMenu;
    delete credModel;
}

void LoginDialog::slot_connect(QModelIndex index)
{
    if(credModel->editable)
        return;

    int row = index.row();
    if ( credModel->credentials.at(row).userName.isEmpty() )
    {
        QMessageBox::information(this, tr("Empty Login"), tr("You must enter a username"));
        return;
    }

    this->setEnabled(false);
    /* Here a possibility to cancel the connection
     * while it is being created could be provided using
     * void NetworkConnection::slot_cancelConnecting(void) */

	//if(ui.connectPB->isDown())	//wth?  unreliable?

    connection = newConnection(credModel->credentials.at(row));
    connect(connection,SIGNAL(stateChanged(ConnectionState)),this,SLOT(slot_receiveConnectionState(ConnectionState)));
    connectionWidget->setNetworkConnection(connection);
}

void LoginDialog::slot_cancel(void)
{
	if(connection)
		connection->userCanceled();
    this->setEnabled(true);
    reject();
}

void LoginDialog::slot_toggleEditable(void)
{
    credModel->editable = !(credModel->editable);
    if (credModel->editable)
    {
        ui.editButton->setText("&Save");
        ui.editLabel->setText("Right click to edit");
    } else {
        ui.editButton->setText("&Edit");
        ui.editLabel->setText("");
        ui.editLabel->repaint();
        credModel->saveCredentials();
    }
}

void LoginDialog::slot_newCredential(QAction *action)
{
    credModel->addCredential(((NewCredentialsAction*)action)->cred);
}

void LoginDialog::slot_showPopup(const QPoint & iPoint)
{
    if (!credModel->editable)
        return;
    popup_item = ui.credentialView->indexAt(iPoint);
    QMenu menu(ui.credentialView);
    QAction * deleteAction = new QAction(tr("Delete"), 0);
    menu.addMenu(newCredMenu);
    if (popup_item != QModelIndex())
    {
        connect(deleteAction,SIGNAL(triggered()),this,SLOT(slot_deleteCredential()));
        menu.addSeparator();
        menu.addAction(deleteAction);
    }

    menu.exec(ui.credentialView->mapToGlobal(iPoint));
    delete deleteAction;
}

void LoginDialog::slot_deleteCredential(void)
{
    credModel->removeRow(popup_item.row());
}

void LoginDialog::slot_receiveConnectionState(ConnectionState newState)
{
    switch(newState)
    {
    case CONNECTED:
        connectionWidget->setupButtons();
        connectionWidget->setEnabled(true);
        this->setEnabled(true);
        this->accept();
        return;
    case AUTH_FAILED:
        QMessageBox::information(this, tr("Bad Login"), tr("Invalid Login"));
        break;
    case PASS_FAILED:
        QMessageBox::information(this, tr("Bad Password"), tr("Invalid Password"));
        break;
    case PROTOCOL_ERROR:
        QMessageBox::information(this, tr("Protocol Error"), tr("This may also result from wrong username or password"));
        break;
    case CANCELED:
        break;
    case ALREADY_LOGGED_IN:
        QMessageBox::information(this, tr("Already Logged In"), tr("Are you logged in somewhere else?"));
        break;
    case CONN_REFUSED:
        QMessageBox::information(this, tr("Connection Refused"), tr("Server may be down"));
        break;
    case HOST_NOT_FOUND:
    case SOCK_TIMEOUT:
    case UNKNOWN_ERROR:
        QMessageBox::information(this, tr("Can't connect"), tr("Can't connect to host!"));
        break;
    default:
        return; // waiting
    }
    // Arrive here only in case of connection error
    disconnect(connection,SIGNAL(stateChanged(ConnectionState)),this,SLOT(slot_receiveConnectionState(ConnectionState)));
    connectionWidget->setNetworkConnection(0);
    connection->deleteLater();
    connection = 0;
    this->setEnabled(true);
}

NetworkConnection * LoginDialog::newConnection(ConnectionCredentials cred)
{
    switch(cred.type)
	{
		case TypeIGS:
            return new IGSConnection(cred);
		case TypeORO:
            return new CyberOroConnection(cred);
		case TypeWING:
            return new WingConnection(cred);
		case TypeLGS:
            return new LGSConnection(cred);
		case TypeTYGEM:
            return new TygemConnection(cred);
		case TypeEWEIQI:
            return new EWeiQiConnection(cred);
		case TypeTOM:
            return new TomConnection(cred);
		default:
            qDebug("LoginDialog::newConnection : Bad connection Type");
			// ERROR handling???
			return 0;
	}
}
