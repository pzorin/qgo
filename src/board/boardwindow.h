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
//#include "define.h"
//#include "move.h"

//#include <QtGui>

class BoardHandler;

class BoardWindow : public QMainWindow, public Ui::BoardWindow
{
	Q_OBJECT

public:
	BoardWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 , int size =19);
	BoardWindow( QWidget *parent = 0 , Qt::WindowFlags flags = 0 ,  GameData *gamedata = 0 , GameMode gamemode = modeNormal , bool iAmBlack = TRUE, bool iAmWhite = TRUE);
	~BoardWindow();

	bool loadSGF(const QString fileName, const QString SGFLoaded, bool fastLoad = false); //TODO get rid of fastload

	int getBoardSize() 			{return boardSize;}
	Board *getBoard() 			{return ui.board;}
	Tree *getTree() 			{return tree;}
	InterfaceHandler *getInterfaceHandler() {return interfaceHandler;}
	Ui::BoardWindow getUi() 		{return ui;}
	GameMode getGameMode() 			{return gameMode; } 
	GamePhase getGamePhase() 		{return gamePhase;}
	void setGamePhase(GamePhase gp)		{gamePhase =gp;}
	bool getMyColorIsBlack()		{return myColorIsBlack;}
	bool getMyColorIsWhite()		{return myColorIsWhite;}

protected:
	void closeEvent(QCloseEvent *e);

	

public slots:
	void slotNthMove() ;
/*
	void slotNavBackward();
	void slotNavForward();
	void slotNavFirst();
	void slotNavLast();
	void slotNavPrevComment();
	void slotNavNextComment();
	void slotNavPrevVar();
	void slotNavNextVar();	
	void slotNavStartVar();
	void slotNavMainBranch();
	void slotNavNextBranch();
	void slotNavIntersection();
*/
private:
	void init();

	Ui::BoardWindow ui;
	Tree *tree;
	int boardSize;
	BoardHandler *boardHandler;
	InterfaceHandler *interfaceHandler;
	GameData *gameData;
	GameMode gameMode;
	GamePhase gamePhase;
	bool myColorIsBlack , myColorIsWhite ;
	
};

#endif
