/***************************************************************************
 *   Copyright (C) 2006 by Emmanuel Béranger                               *
 *   yfh2@hotmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "stdio.h"
#include "globals.h"
#include "mainwindow.h"
#include "boardwindow.h"
#include "sgfparser.h"
#include "tree.h"

#include <QtGui>

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags )
	: QMainWindow( parent,  flags )
{
	qDebug( "Home Path : %s" ,QDir::homePath().toLatin1().constData());	
	qDebug( "Current Path : %s" ,QDir::currentPath ().toLatin1().constData());

	ui.setupUi(this);

	SGFloaded = "";
/*
* settings example
*/
	ui.comboBox_language->setCurrentIndex (settings.value("language").toInt());

/*
* filling the file view
*/
	QStringList filters = (QStringList() << "*.sgf" << "*.SGF");
	model = new QDirModel(filters,  QDir::AllEntries | QDir::AllDirs , QDir::Name,0);

	ui.dirView_1->setModel(model);

	ui.dirView_1->hideColumn(1);
	ui.dirView_1->hideColumn(2);
	ui.dirView_1->setColumnWidth(0,250); 
	ui.dirView_1->setCurrentIndex(model->index( QDir::homePath () ));

/*
* connecting the new game button
*/
	connect(ui.button_newGame,SIGNAL(pressed()),SLOT(slot_fileNewBoard()));
	connect(ui.button_loadGame,SIGNAL(pressed()),SLOT(slot_fileOpenBoard()));

	connect(ui.dirView_1->selectionModel(),  
		SIGNAL(currentChanged ( const QModelIndex & , const QModelIndex &  )),
		this,
		SLOT(slot_displayFileHeader(const QModelIndex & , const QModelIndex &  )));

/*
 * Creates the SGF parser for displaying the file infos
 */
	MW_SGFparser = new SGFParser(NULL);

}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *)
{
	settings.setValue("language",ui.comboBox_language->currentIndex ());
}


/* 
 * Loads the file header data from the item selected in the directory display
 */
void MainWindow::slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/ )
{

	ui.File_WhitePlayer->setText("");
	ui.File_BlackPlayer->setText("");
	ui.File_Date->setText("");
	ui.File_Handicap->setText("");
	ui.File_Result->setText("");
	ui.File_Komi->setText("");
	ui.File_Size->setText("");

	QVariant v = topLeft.data(QDirModel::FilePathRole);
	//qDebug( "Selected item : %s" ,v.toString().toLatin1().constData());

	if (model->isDir(topLeft))
	{
		ui.button_loadGame->setDisabled(true);
		return ;
	}
	//qDebug( "Selected file : %s \n" ,model->filePath(topLeft).toLatin1().constData());

	fileLoaded = model->filePath(topLeft).toLatin1().constData();
	SGFloaded = MW_SGFparser->loadFile(fileLoaded);
	
	if (SGFloaded == NULL)
	{
		ui.button_loadGame->setDisabled(true);
		return ;
	}

	ui.button_loadGame->setEnabled(true);
	
	GameLoaded = MW_SGFparser-> initGame(SGFloaded, fileLoaded);
	
	if (GameLoaded)
	{
		QString komi;
		komi.setNum(GameLoaded->komi);	

		ui.File_WhitePlayer->setText(GameLoaded->playerWhite);
		ui.File_BlackPlayer->setText(GameLoaded->playerBlack);
		ui.File_Date->setText(GameLoaded->date);
		ui.File_Handicap->setText(QString::QString(GameLoaded->handicap));
		ui.File_Result->setText(GameLoaded->result);
		ui.File_Komi->setText(komi);
		ui.File_Size->setText(QString::QString(GameLoaded->size));
	}	

}

void MainWindow::slot_fileNewBoard()
{
	BoardWindow *b = new BoardWindow(this,0,19);
	b->show();
}

void MainWindow::slot_fileOpenBoard()
{
	BoardWindow *b = new BoardWindow(this,0,19);//TODO see how to pass the correct size to the board
	b->show();

 	if(!b->loadSGF(fileLoaded))
		delete b;

	//TODO decide what to do with the game info ...
}
