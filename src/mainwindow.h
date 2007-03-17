/***************************************************************************
* Mainwindow.h
* headers for the main client window           *
 ***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "globals.h"
#include "sgfparser.h"
#include "boardwindow.h"

#include <QtGui>

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 );
	~MainWindow();

public slots:
	void slot_fileNewBoard();
	void slot_fileOpenBoard();
	void slot_computerNewBoard();
	void slot_displayFileHeader(const QModelIndex & topLeft, const QModelIndex & bottomRight );

protected:
	void closeEvent(QCloseEvent *e);
	void loadSettings();
	void saveSettings();

private:
	Ui::MainWindow ui;
	QDirModel *model;
	SGFParser * MW_SGFparser;
	QString SGFloaded, fileLoaded ;
	GameData * GameLoaded ;

	QList<BoardWindow *> boardList;
	void createGame(GameMode gameMode, GameData * gameData, bool myColorIsBlack = FALSE, bool myColorIsWhite = FALSE, QString fileLoaded = "");
};

#endif


