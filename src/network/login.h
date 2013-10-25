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
#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QAbstractTableModel>
#include <QAction>
#include <QMenu>
#include <QStyledItemDelegate>
#include "ui_login.h"
#include "defines.h"
#include "networkconnection.h"

class NetworkConnection;
class QMessageBox;

class CredentialTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    CredentialTableModel(QObject * parent = 0);
    ~CredentialTableModel();
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QVariant data(const QModelIndex & index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    bool removeRows(int row, int count, const QModelIndex &parent);

    void addCredential(ConnectionCredentials cred);
    void loadCredentials(void);
    void saveCredentials(void);
public:
    QList <ConnectionCredentials> credentials;
    bool editable;
};

class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ComboBoxDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class NewCredentialsAction : public QAction
{
    Q_OBJECT
public:
    NewCredentialsAction(const QString &text, ConnectionCredentials c, QObject * parent = 0) : QAction(text,parent), cred(c) {};
    ConnectionCredentials cred;
};

class LoginDialog : public QDialog, Ui::LoginDialog
{
	Q_OBJECT
	public:
        LoginDialog(QWidget *parent = 0);
        ~LoginDialog();
	private slots:		//or can these be private?
		void slot_cancel(void);
        void slot_connect(QModelIndex);
        void slot_newCredential(QAction *action);
        void slot_deleteCredential(void);
        void slot_receiveConnectionState(ConnectionState newState);
        void slot_showPopup(const QPoint & iPoint);
        void slot_toggleEditable(void);
    private:
		ConnectionType serverStringToConnectionType(const QString & s);
        NetworkConnection * newConnection(ConnectionCredentials cred);
        Ui::LoginDialog ui;
        NetworkConnection * connection;
        CredentialTableModel * credModel;
        QMenu * newCredMenu;
        QModelIndex popup_item;
};

#endif
