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

#include "ui_mainwindow.h"
#include "defines.h"
#include "audio.h"

#include <QtGui>

class HostList;
class RoomListing;
class LoginDialog;
class NetworkConnection;
class Talk;
class GameDialog;
class SGFParser;
class BoardWindow;
class GameData;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 );
	~MainWindow();

	bool loadSGF(QString);

	void addBoardWindow(BoardWindow *);
	int checkForOpenBoards(void);

public slots:
    void removeBoardWindow(QObject *);

	void slot_expanded(const QModelIndex & i);
	// sfg slots
	void slot_fileNewBoard();
	void slot_fileOpenBoard();
	void slot_fileOpenBoard(const QModelIndex &);
	void slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	
	// go engine slots
	void slot_computerNewBoard();
	void slot_loadComputerFile(const QModelIndex & topLeft, const QModelIndex & bottomRight );
	void slot_getComputerPath();
	void slot_computerPathChanged(const QString &);

	//preferences tabs slots
	void slot_cancelPressed();
	void slot_currentChanged(int );
	void slot_getGobanPath();
	void slot_getTablePath();
	void slot_newFile_HandicapChange(int);
	void slot_newComputer_HandicapChange(int);
    void slot_languageChanged(int);

    void setGameCountStat(int);
    void setPlayerCountStat(int);

    void openConnectDialog(void);

protected:
	void closeEvent(QCloseEvent *e);
	void loadSettings();
	void saveSettings();

private:
    Ui::MainWindow ui;
    QLabel *statusMessage, *statusUsers, *statusGames, *statusServer,*statusOnlineTime;

	bool selectFile(const QModelIndex &);
	
	QDirModel *model;
	SGFParser * MW_SGFparser;
	QString SGFloaded, fileLoaded;
    GameData * GameLoaded; // Should be in a separate loader class
	Sound *connectSound, *gameSound;

	void initStatusBar();
	void displayGame(DisplayBoard *);

    QList<BoardWindow *> boardWindowList;
	QString currentWorkingDir;

    void saveHostList(void);
    void loadHostList(QSettings * settings);
    HostList * hostlist;
};

#endif


