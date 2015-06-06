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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "defines.h"

#include <QtWidgets>

class RoomListing;
class LoginDialog;
class NetworkConnection;
class GameDialog;
class BoardWindow;
class GameData;
class Sound;
namespace Ui {
class MainWindow;
}

class Engine
{
public:
    Engine(QString p, QString a) :
        path(p), arguments(a) {}
    QString path;
    QString arguments;
};

class EngineTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum EngineTableColumn { ENGINE_DEFAULT, ENGINE_PATH, ENGINE_ARGUMENTS, ENGINE_NUMITEMS };

    EngineTableModel(QObject * parent = 0);
    ~EngineTableModel();
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QVariant data(const QModelIndex & index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    bool removeRows(int row, int count, const QModelIndex &parent);

    void addEngine(Engine e);
    void loadEngines(void);
    void saveEngines(void);
public:
    QList <Engine> engines;
    int selected_engine;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 );
	~MainWindow();

	void addBoardWindow(BoardWindow *);
	int checkForOpenBoards(void);

public slots:
    void removeBoardWindow(QObject *);

    void slot_fileNew();
    void slot_fileOpen();
    void openSGF(QString path);

    //preferences tabs slots
	void slot_cancelPressed();
	void slot_currentChanged(int );
	void slot_getGobanPath();
    void slot_getTablePath();
    void slot_languageChanged(int);

    void openConnectDialog(void);

    void addEngine(void);
    void removeEngine(void);

protected:
	void closeEvent(QCloseEvent *e);
	void loadSettings();
	void saveSettings();

private:
    Ui::MainWindow * ui;
	Sound *connectSound, *gameSound;

    QList<BoardWindow *> boardWindowList;
	QString currentWorkingDir;

    LoginDialog * logindialog;
    EngineTableModel * engineTableModel;
};

#endif
