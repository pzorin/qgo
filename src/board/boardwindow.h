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
#include "qgoboard.h"
#include "clockdisplay.h"
#include "gamedata.h"

class BoardHandler;
class qGoBoard;
class ClockDisplay;
class Tree;

class BoardWindow : public QMainWindow, public Ui::BoardWindow
{
	Q_OBJECT

public:
	//BoardWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 ,  GameData *gamedata = 0 , GameMode gamemode = modeNormal , bool iAmBlack = TRUE, bool iAmWhite = TRUE, class BoardDispatch * _dispatch = 0);
	BoardWindow(GameData *gamedata = 0 , bool iAmBlack = TRUE, bool iAmWhite = TRUE, class BoardDispatch * _dispatch = 0);
	~BoardWindow();
	
	void init();
	bool loadSGF(const QString fileName, const QString SGFLoaded=0);
	bool doSave(QString fileName, bool force);
	QString getCandidateFileName();

	int getBoardSize() 			{return boardSize;}
	Board *getBoard() 			{return ui.board;}
	Tree *getTree() 			{return tree;}
	InterfaceHandler *getInterfaceHandler() {return interfaceHandler;}
	BoardHandler *getBoardHandler() 	{return boardHandler;}
	Ui::BoardWindow * getUi() 		{return &ui;}
	GameMode getGameMode() 			{return gameData->gameMode; } 
	GamePhase getGamePhase() 		{return gamePhase;}
	bool getMyColorIsBlack()		{return myColorIsBlack;}
	bool getMyColorIsWhite()		{return myColorIsWhite;}
	GameData *getGameData() 		{return gameData;}
	int getId()				{return gameData->number;}
	MarkType getEditMark()			{return editMark;}
	void setBoardSize(int sz)		{boardSize=sz;}
	void setGamePhase(GamePhase gp);
	void setTree(Tree *t)			{tree=t;}
	void gameDataChanged(void);
	void setGameData(GameData *gd);	
	QString get_wplayer()			{return gameData->white_name;}
	QString get_bplayer()			{return gameData->black_name;}
	ClockDisplay *getClockDisplay()		{return clockDisplay;}
	void swapColors(bool noswap = false);
	
	bool okayToQuit(void);	

	qGoBoard *qgoboard;
	//virtual QSize sizeHint() const;
	/* boarddispatch should be stored in the interface, NOT here
	 * in boardwindow, but we'll move it once we see where its
	 * going to be everywhere */
	class BoardDispatch * getBoardDispatch(void) { return dispatch; };
	void setBoardDispatch(BoardDispatch * d);

protected:
	void closeEvent(QCloseEvent *e);
	void resizeEvent(QResizeEvent *e);

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
	void slotToggleInsertStones(bool val);
	

private:
	void setupUI(void);
	void setupBoardUI(void);

	Ui::BoardWindow ui;
	Tree *tree;
	int boardSize;		//the true boardsize
	BoardHandler *boardHandler;
	InterfaceHandler *interfaceHandler;
//	qGoBoard *qgoboard;
	class BoardDispatch * dispatch;		//may not be the best place!!!

	GameData *gameData;
	GamePhase gamePhase;
	bool myColorIsBlack , myColorIsWhite ;
	bool localStoneSound ;
	
	QButtonGroup *editButtons;
	MarkType editMark;
	ClockDisplay *clockDisplay;

};

#endif
