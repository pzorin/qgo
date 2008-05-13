/***************************************************************************
 *   Copyright (C) 2006 by EB   *
 *      *
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

#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include "ui_boardwindow.h"
#include "boardhandler.h"
#include "interfacehandler.h"
#include "tree.h"
#include "qgoboard.h"
#include "clockdisplay.h"

class BoardHandler;
class qGoBoard;
class ClockDisplay;

class BoardWindow : public QMainWindow, public Ui::BoardWindow
{
	Q_OBJECT

public:
	//BoardWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 , int size = 19); // jm 071106: two standard constructors are dangerous!
	BoardWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 ,  GameData *gamedata = 0 , GameMode gamemode = modeNormal , bool iAmBlack = TRUE, bool iAmWhite = TRUE);
	~BoardWindow();
	
	void init();
	bool loadSGF(const QString fileName, const QString SGFLoaded=0, bool fastLoad = false); //TODO get rid of fastload
	bool doSave(QString fileName, bool force);
	QString getCandidateFileName();

	int getBoardSize() 			{return boardSize;}
	Board *getBoard() 			{return ui.board;}
	Tree *getTree() 			{return tree;}
	InterfaceHandler *getInterfaceHandler() {return interfaceHandler;}
	BoardHandler *getBoardHandler() 	{return boardHandler;}
	Ui::BoardWindow getUi() 		{return ui;}
	GameMode getGameMode() 			{return gameMode; } 
	GamePhase getGamePhase() 		{return gamePhase;}
	bool getMyColorIsBlack()		{return myColorIsBlack;}
	bool getMyColorIsWhite()		{return myColorIsWhite;}
	GameData *getGameData() 		{return gameData;}
	int getId()				{return gameData->gameNumber;}
	MarkType getEditMark()			{return editMark;}
	void setBoardSize(int sz)		{boardSize=sz;}
	void setGamePhase(GamePhase gp)		{gamePhase = gp;}
	void setTree(Tree *t)			{tree=t;}
	void setGameData(GameData *gd);	
	QString get_wplayer()			{return gameData->playerWhite;}
	QString get_bplayer()			{return gameData->playerBlack;}
	ClockDisplay *getClockDisplay()		{return clockDisplay;}

	qGoBoard *qgoboard;

protected:
	void closeEvent(QCloseEvent *e);

signals:
	void signal_boardClosed(int);
	void signal_duplicate( GameData *, const QString&, int);

public slots:
	void slotEditButtonPressed( int id );
	void slotViewCoords(bool toggle);
	void slotGameInfo(bool toggle);
	void slotSound(bool toggle);
	bool slotFileSave();
	bool slotFileSaveAs();
	void slotEditDelete();
	void slotExportSGFtoClipB();
	void slotExportPicClipB();
	void slotExportPic();
	void slotDuplicate();
	void keyPressEvent(QKeyEvent *e);

private:

	Ui::BoardWindow ui;
	Tree *tree;
	int boardSize;
	BoardHandler *boardHandler;
	InterfaceHandler *interfaceHandler;
//	qGoBoard *qgoboard;

	GameData *gameData;
	GameMode gameMode;
	GamePhase gamePhase;
	bool myColorIsBlack , myColorIsWhite ;
	bool localStoneSound ;
	
	QButtonGroup *editButtons;
	MarkType editMark;
	ClockDisplay *clockDisplay;
};

#endif
