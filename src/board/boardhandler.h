/*
 * boardhandler.h
 */

#ifndef BOARDHANDLER_H
#define BOARDHANDLER_H

#include "../defines.h"
#include "tree.h"
#include "sgfparser.h"
#include "board.h"
#include "boardwindow.h"

class BoardWindow;

class BoardHandler : public QObject
{
	Q_OBJECT
public:
	BoardHandler(BoardWindow *bw, Tree *tree, int * board_size);
	~BoardHandler();

	void clearData();
	bool markedDead;
	void countScore();
	void countMarked(void);
	class GameResult retrieveScore(void);
	void exitScore();
	void updateMove(Move *m=0, bool ignore_update = false);
	bool updateAll(Move *move, bool toDraw=true);
	void updateCursor(StoneColor c=stoneNone);
	void gotoMove(Move *m);
//	bool loadSGF(const QString fileName, const QString SGFLoaded=0, bool fastLoad=false);

	void findMoveByPos(int x,int  y);

public slots:
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
	void slotNthMove(int n);

	void slotWheelEvent(QWheelEvent *e);

private:
	Board *board;
	Tree *tree;
	BoardWindow * boardwindow;

	bool updateAll_updateAll;
	int capturesBlack, capturesWhite, caps_black, caps_white;
	int terrBlack, terrWhite;
	Move *lastValidMove;
//	GameMode gameMode;
	int * boardSize;
	
	void updateVariationGhosts(Move *m);
	bool navIntersectionStatus;

	QTime wheelTime;

};

#endif
