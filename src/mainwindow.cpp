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


#include "stdio.h"
#include "defines.h"
#include "mainwindow.h"
#include "boardwindow.h"
#include "connectionwidget.h"
#include "login.h"
#include "host.h"
#include "sgfpreview.h"

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags )
	: QMainWindow( parent,  flags )
{
	qDebug( "Home Path : %s" ,QDir::homePath().toLatin1().constData());	
	qDebug( "Current Path : %s" ,QDir::currentPath ().toLatin1().constData());

    this->setAttribute( Qt::WA_DeleteOnClose );
	ui.setupUi(this);
	//hide by default
	setWindowTitle(QString(PACKAGE) + " " + QString(VERSION));
	initStatusBar();
	/* FIXME, really need a list of such things, 0s */

	// loads the settings
    hostlist = new HostList;
	loadSettings();
	// connecting the Go server tab buttons and signals

	// connecting the new game button
    connect( ui.actionOpen, SIGNAL(triggered()), SLOT(slot_fileOpenBoard()) );

	connect(ui.button_newGame,SIGNAL(pressed()),SLOT(slot_fileNewBoard()));

	connect(ui.button_newComputerGame,SIGNAL(pressed()),SLOT(slot_computerNewBoard()));


	connect( ui.computerPathButton, SIGNAL( clicked() ), this, SLOT( slot_getComputerPath() ) );
	connect(ui.LineEdit_computer, SIGNAL(textChanged (const QString &)), this, SLOT(slot_computerPathChanged(const QString &)));
	
	connect(ui.newFile_Handicap, SIGNAL(valueChanged(int)), this, SLOT(slot_newFile_HandicapChange(int)));
	connect(ui.newComputer_Handicap, SIGNAL(valueChanged(int)), this, SLOT(slot_newComputer_HandicapChange(int)));
	connect(ui.cancelButtonPrefs,SIGNAL(pressed()),SLOT(slot_cancelPressed()));
	connect(ui.cancelButtonServer,SIGNAL(pressed()),SLOT(slot_cancelPressed()));
	connect(ui.stackedWidget, SIGNAL(currentChanged ( int )), SLOT(slot_currentChanged(int )));

	//coneects the preference buttons
	connect( ui.gobanPathButton, SIGNAL( clicked() ), this, SLOT( slot_getGobanPath() ) );
	connect( ui.tablePathButton, SIGNAL( clicked() ), this, SLOT( slot_getTablePath() ) );
	connect(ui.comboBox_language, SIGNAL(currentIndexChanged ( int )), SLOT(slot_languageChanged(int )));

	//sound
	connectSound = 	SoundFactory::newSound(SOUND_PATH_PREFIX"static.wav");

    connect( ui.actionConnect, SIGNAL(triggered()), SLOT(openConnectDialog()) );
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	/* Close connection if open */
    if(ui.connectionWidget->closeConnection() < 0)
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

#ifdef FIXME
	/* We're not sure yet what to do with the status bar,
	 * how to divy it up */
#endif //FIXME
void MainWindow::initStatusBar()
{
//	statusBar = new QStatusBar(parent);
//	statusBar->resize(-1, 20);
	statusBar()->show();
//	statusBar()->setSizeGripEnabled(false);
	statusBar()->showMessage(tr("Ready."));  // Normal indicator

	// Standard Text instead of "message" cause WhatsThisButten overlaps
	statusMessage = new QLabel(statusBar());
	statusMessage->setAlignment(Qt::AlignCenter /*| SingleLine*/);
	statusMessage->setText("");
	statusBar()->addPermanentWidget(statusMessage/*, 0, true*/);  // Permanent indicator
/*
	// What's this
	statusWhatsThis = new QLabel(statusBar);
	statusWhatsThis->setAlignment(AlignCenter | SingleLine);
	statusWhatsThis->setText("WHATSTHIS");
	statusBar->addWidget(statusWhatsThis, 0, true);  // Permanent indicator
	QWhatsThis::whatsThisButton(statusWhatsThis);
*/
	// The users widget
	statusUsers = new QLabel(statusBar());
	statusUsers->setAlignment(Qt::AlignCenter /* | SingleLine*/);
	statusUsers->setText(" P: 0");// / 0 ");
	statusBar()->addPermanentWidget(statusUsers /*, 0, true*/);  // Permanent indicator
	statusUsers->setToolTip( tr("Current online players / watched players"));
	statusUsers->setWhatsThis( tr("Displays the number of current online players\nand the number of online players you are watching.\nA player you are watching has an entry in the 'watch player:' field."));

	// The games widget
	statusGames = new QLabel(statusBar());
	statusGames->setAlignment(Qt::AlignCenter /*| SingleLine*/);
	statusGames->setText(" G: 0");// / 0 ");
	statusBar()->addPermanentWidget(statusGames /*, 0, true*/);  // Permanent indicator
	statusGames->setToolTip( tr("Current online games / observed games + matches"));
	statusGames->setWhatsThis( tr("Displays the number of games currently played on this server and the number of games you are observing or playing"));

	// The server widget
	statusServer = new QLabel(statusBar());
	statusServer->setAlignment(Qt::AlignCenter /*| SingleLine*/);
	statusServer->setText(" OFFLINE ");
	statusBar()->addPermanentWidget(statusServer /*, 0, true*/);  // Permanent indicator
	statusServer->setToolTip( tr("Current server"));
	statusServer->setWhatsThis( tr("Displays the current server's name or OFFLINE if you are not connected to the internet."));
/*
	// The channel widget
	statusChannel = new QLabel(statusBar());
	statusChannel->setAlignment(Qt::AlignCenter | SingleLine);
	statusChannel->setText("");
	statusBar()->addWidget(statusChannel, 0, true);  // Permanent indicator
	QToolTip::add(statusChannel, tr("Current channels and users"));
	QWhatsThis::add(statusChannel, tr("Displays the current channels you are in and the number of users inthere.\nThe tooltip text contains the channels' title and users' names"));
*/
	// Online Time
	statusOnlineTime = new QLabel(statusBar());
	statusOnlineTime->setAlignment(Qt::AlignCenter);
	statusOnlineTime->setText(" 00:00 ");
	statusBar()->addPermanentWidget(statusOnlineTime /*, 0, true*/);  // Permanent indicator
	statusOnlineTime->setToolTip( tr("Online Time"));
	statusOnlineTime->setWhatsThis( tr("Displays the current online time.\n(A) -> auto answer\n(Hold) -> hold the line"));

}

void MainWindow::setGameCountStat(int count)
{
    statusGames->setText(QVariant(count).toString().prepend(" G: "));
}

void MainWindow::setPlayerCountStat(int count)
{
    statusUsers->setText(QVariant(count).toString().prepend(" P: "));
}

void MainWindow::saveHostList(void)
{
    QSettings settings;
    settings.beginWriteArray("HOSTS");
    for (int i = 0; i < hostlist->size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("server", hostlist->at(i)->host());
        settings.setValue("loginName", hostlist->at(i)->loginName());
        settings.setValue("password", hostlist->at(i)->password());
    }
    settings.endArray();
}

void MainWindow::loadHostList(QSettings *settings)
{
    hostlist->clear();
    Host *h;
    int size = settings->beginReadArray("HOSTS");
    for (int i = 0; i < size; ++i)
    {
        settings->setArrayIndex(i);
        h = new Host(settings->value("server").toString(),
            settings->value("loginName").toString(),
            settings->value("password").toString());
        hostlist->append(h);
    }
    settings->endArray();
}

/*
 * The 'New Game' button in 'sgf editor' tab has been pressed.
 */
void MainWindow::slot_fileNewBoard()
{
	
	GameData *gd = new GameData();
	gd->gameMode = modeNormal;

	gd->board_size = ui.newFile_Size->value();
	gd->handicap = ui.newFile_Handicap->value();
	gd->black_name = ui.newFile_BlackPlayer->text();
	gd->white_name = ui.newFile_WhitePlayer->text();
	gd->komi = ui.newFile_Komi->text().toFloat();

    addBoardWindow(new BoardWindow(gd, TRUE, TRUE));
}


void MainWindow::slot_newFile_HandicapChange(int a)
{
	if(a == 1)
		ui.newFile_Handicap->setValue(0);
}

void MainWindow::slot_newComputer_HandicapChange(int a)
{
	if(a == 1)
		ui.newComputer_Handicap->setValue(0);
}

void MainWindow::slot_fileOpenBoard()
{
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);
    SGFPreview *previewWidget = new SGFPreview(dialog);
    QGridLayout *layout = (QGridLayout*)dialog->layout();
    layout->addWidget(previewWidget, 1, 3);
    connect(dialog,SIGNAL(currentChanged(QString)),previewWidget,SLOT(setPath(QString)));
    connect(dialog,SIGNAL(accepted()),previewWidget,SLOT(openSGF()));
    dialog->setNameFilter("Smart Game Format (*.sgf *.SGF)");
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->show(); // Maybe exec()
}

/*
 * The 'New Game' button in 'Go Engine' tab has been pressed.
 */
void MainWindow::slot_computerNewBoard()
{
	
	GameData *gd = new GameData();
	
	gd->gameMode = modeComputer;
	gd->board_size = ui.newComputer_Size->text().toInt();
	gd->handicap = ui.newComputer_Handicap->text().toInt();
	gd->komi = ui.newComputer_Komi->text().toFloat();
	gd->oneColorGo = ui.OneColorGoCheckBox->isChecked();
	
	bool imBlack = (ui.computerPlaysWhite->isChecked());
	bool imWhite = (ui.computerPlaysBlack->isChecked());
	gd->black_name = (imBlack ? "Human" : "Computer");
	gd->white_name = (imWhite ? "Human" : "Computer");

	if (imBlack == imWhite)
	{
		QMessageBox::warning(this, PACKAGE, tr("*** Both players are the same ! ***"));
		delete gd; gd = NULL;
		return;
	}
    BoardWindow * bw = new BoardWindow(gd , imBlack, imWhite );
    if(bw)
    {
        addBoardWindow(bw);
    } else {
		delete gd; gd = NULL;
	}
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
    LoginDialog * logindialog = new LoginDialog(hostlist);
    logindialog->exec();
}
