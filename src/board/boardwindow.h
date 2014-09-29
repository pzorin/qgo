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


#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include "gamedata.h"
#include <QMainWindow>

class BoardDispatch;
class qGoBoard;
class ClockDisplay;
class Tree;
class QLabel;
class QButtonGroup;
class Board;
class Move;
namespace Ui {
class BoardWindow;
}

enum TimeWarnState { TimeOK, TimeLow, TimeExpired };

class BoardWindow : public QMainWindow
{
	Q_OBJECT

    friend class BoardDispatch;
    friend class CountDialog;

public:
    BoardWindow(GameData *gamedata = 0 , bool iAmBlack = true, bool iAmWhite = true, class BoardDispatch * _dispatch = 0);
	~BoardWindow();
	
	void init();
    bool doSave(QString fileName, bool force);
	QString getCandidateFileName();

    int getBoardSize() const {return boardSize;}
    Tree *getTree() 			{return tree;}
	GameMode getGameMode() 			{return gameData->gameMode; } 
	GamePhase getGamePhase() 		{return gamePhase;}
	bool getMyColorIsBlack()		{return myColorIsBlack;}
	bool getMyColorIsWhite()		{return myColorIsWhite;}
	GameData *getGameData() 		{return gameData;}
	int getId()				{return gameData->number;}
	MarkType getEditMark()			{return editMark;}
    void setGamePhase(GamePhase gp);
	void gameDataChanged(void);	
    ClockDisplay *getClockDisplay()		{return clockDisplay;}
    void swapColors(bool noswap = false);
	void saveRecordToGameData(void);
	
    bool okayToQuit(void);

    BoardDispatch * getBoardDispatch(void) { return dispatch; }
	void setBoardDispatch(BoardDispatch * d);

    void clearData();
    void setMoveData(int n, bool black, int brothers, int sons, bool hasParent,
        bool hasPrev, bool hasNext, int lastX=-1, int lastY=-1);

    void setUndoEnabled(bool state);
    void setDrawEnabled(bool state);
    void setCountEnabled(bool state);
    void setDoneEnabled(bool state);
    void setAdjournEnabled(bool state);
    void setResignEnabled(bool state);
    void setObserverModel(QAbstractItemModel * model);

    void setTimeBlack(QString time);
    void setTimeWhite(QString time);
    void warnTimeBlack(TimeWarnState state);
    void warnTimeWhite(TimeWarnState state);
    void displayComment(QString comment);

protected:
	void closeEvent(QCloseEvent *e);
	void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *e);

signals:
	void signal_boardClosed(int);
	void signal_duplicate( GameData *, const QString&, int);

public slots:
	void slotEditButtonPressed( int id );
	void slotShowCoords(bool toggle);
	void slotGameInfo(bool toggle);
	void slotSound(bool toggle);
	bool slotFileSave();
	bool slotFileSaveAs();
	void slotEditDelete();
	void slotExportSGFtoClipB();
	void slotExportPicClipB();
	void slotExportPic();
	void slotDuplicate();
    void slot_addtime_menu(QAction *);
    void setComputerBlack(bool val);
    void setComputerWhite(bool val);
    void requestComputerMove();
    void setBlackName(QString name);
    void setWhiteName(QString name);
    void switchToEditMode();
    void switchToLocalMode();
    void slotUpdateComment();
    void slotNavIntersection();
    void slotWheelEvent(QWheelEvent *e);
    void slotGetScore(int terrBlack, int captBlack, int deadWhite, int terrWhite, int captWhite, int deadBlack);
    void updateMove(Move *m);
    void slotSendComment(void);

private:
    void updateCaption();
    void setMode(GameMode gameMode);
    void setSliderMax(int n);
    void checkHideToolbar(int h);
    void updateButtons(StoneColor lastMoveColor=stoneNone);
    void updateCursor(StoneColor c=stoneNone);

    qGoBoard *qgoboard;
    QLabel *moveNumLabel,*komiLabel,*buyoyomiLabel,*handicapLabel,*freeratedLabel;

    Ui::BoardWindow * ui;
	QMenu * addtime_menu;
	Tree *tree;
    const int boardSize;		//the true boardsize
    class BoardDispatch * dispatch;		//may not be the best place!!!

	GameData *gameData;
	GamePhase gamePhase;
	bool myColorIsBlack , myColorIsWhite ;
	bool localStoneSound ;
	
	QButtonGroup *editButtons;
	MarkType editMark;
	ClockDisplay *clockDisplay;
    QTime wheelTime;
};

#endif
