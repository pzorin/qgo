/***************************************************************************
 *   Copyright (C) 2009 by the qGo project                                 *
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


#include "defines.h"
#include "mainwindow.h"
#include "boardwindow.h"
#include "connectionwidget.h"
#include "login.h"
#include "sgfpreview.h"
#include "audio.h"
#include "sgfparser.h"
#include "newgamedialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags )
    : QMainWindow( parent,  flags ), ui(new Ui::MainWindow)
{
	qDebug( "Home Path : %s" ,QDir::homePath().toLatin1().constData());	
	qDebug( "Current Path : %s" ,QDir::currentPath ().toLatin1().constData());

    ui->setupUi(this);
    this->setAttribute( Qt::WA_DeleteOnClose );
    //hide by default
	setWindowTitle(QString(PACKAGE) + " " + QString(VERSION));

	// loads the settings
    loadSettings();
	// connecting the Go server tab buttons and signals

	// connecting the new game button
    connect( ui->actionOpen, SIGNAL(triggered()), SLOT(slot_fileOpen()) );
    connect(ui->actionNew,SIGNAL(triggered()),SLOT(slot_fileNew()));

    connect(ui->cancelButtonPrefs,SIGNAL(pressed()),SLOT(slot_cancelPressed()));
    connect(ui->cancelButtonServer,SIGNAL(pressed()),SLOT(slot_cancelPressed()));
    connect(ui->stackedWidget, SIGNAL(currentChanged ( int )), SLOT(slot_currentChanged(int )));

	//coneects the preference buttons
    connect( ui->gobanPathButton, SIGNAL( clicked() ), this, SLOT( slot_getGobanPath() ) );
    connect( ui->tablePathButton, SIGNAL( clicked() ), this, SLOT( slot_getTablePath() ) );
    connect(ui->comboBox_language, SIGNAL(currentIndexChanged ( int )), SLOT(slot_languageChanged(int )));

	//sound
    connectSound = 	new Sound("static.wav");

    logindialog = new LoginDialog(this);

    connect( ui->actionConnect, SIGNAL(triggered()), SLOT(openConnectDialog()) );

    engineTableModel = new EngineTableModel(this);
    ui->engineTableView->setModel(engineTableModel);
    ui->engineTableView->setColumnWidth ( EngineTableModel::ENGINE_DEFAULT, 60 );
    ui->engineTableView->setColumnWidth ( EngineTableModel::ENGINE_PATH, 300 );
    connect(ui->engineAddButton, &QPushButton::pressed, this, &MainWindow::addEngine);
    connect(ui->engineRemoveButton, &QPushButton::pressed, this, &MainWindow::removeEngine);
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	/* Close connection if open */
    if(ui->connectionWidget->closeConnection() < 0)
	{
		e->ignore();
		return;
	}
	if(checkForOpenBoards() < 0)
	{
		e->ignore();
		return;
	}
	saveSettings();
}


void MainWindow::slot_fileOpen()
{
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);
    SGFPreview *previewWidget = new SGFPreview(dialog);
    QGridLayout *layout = (QGridLayout*)dialog->layout();
    layout->addWidget(previewWidget, 1, 3);
    connect(dialog,SIGNAL(currentChanged(QString)),previewWidget,SLOT(setPath(QString)));
    connect(dialog,SIGNAL(fileSelected(QString)),this,SLOT(openSGF(QString)));
    dialog->setNameFilter("Smart Game Format (*.sgf *.SGF)");
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->exec();
    delete dialog;
}

void MainWindow::openSGF(QString path)
{
    SGFParser * MW_SGFparser = new SGFParser(NULL);
    QString SGFloaded = MW_SGFparser->loadFile(path);
    if (SGFloaded == NULL)
        return;

    GameData * GameLoaded = MW_SGFparser->initGame(SGFloaded, path);
    if (GameLoaded == NULL)
        return;

    GameLoaded->gameMode = modeLocal;
    this->addBoardWindow(new BoardWindow(GameLoaded, true, true));
}

/*
 * The 'New Game' button in 'Go Engine' tab has been pressed.
 */
void MainWindow::slot_fileNew()
{
    NewGameDialog * newGameDialog = new NewGameDialog(this);
    newGameDialog->exec();
    delete newGameDialog;
}

void MainWindow::addBoardWindow(BoardWindow * bw)
{
    boardWindowList.append(bw);
    connect(bw,SIGNAL(destroyed(QObject*)),SLOT(removeBoardWindow(QObject*)));
}

void MainWindow::removeBoardWindow(QObject *bw)
{
    for(int i = 0; i < boardWindowList.length(); i++)
	{
        if(boardWindowList[i] == (BoardWindow*)bw)
		{
            boardWindowList.removeAt(i);
			return;
		}
	}
}

int MainWindow::checkForOpenBoards(void)
{
    for(int i = 0; i < boardWindowList.length(); i++)
	{
        if(!(boardWindowList[i]->okayToQuit()))
			return -1;
	}
	//close all open, since boardwindow no longer has parent because of windows task bar issue:
    qDeleteAll(boardWindowList);
    boardWindowList.clear();
	return 0;
}

void MainWindow::openConnectDialog(void)
{
    /* The login dialog is responsible for creating the connection
     *and notifying the connectionWidget about it. */
    if(connectionWidget->isConnected())
        return;
    logindialog->exec();
}
