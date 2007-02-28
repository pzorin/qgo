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

class BoardHandler
{
public:
	BoardHandler(BoardWindow *bw, Tree *tree =NULL, int size=19);
	~BoardHandler();

	void clearData();
	bool markedDead;
	
	// Navigation
	bool nextMove(bool autoplay=false);
	void previousMove();
	void gotoFirstMove();
	void gotoLastMove();
	void nextComment();
	void previousComment();
	void nextVariation();
	void previousVariation();
	void gotoVarStart();
	void gotoMainBranch();
	void gotoNextBranch();
	void navIntersection();

	void updateMove(Move *m=0, bool ignore_update = false);
	bool updateAll(Matrix *m, bool toDraw=true);
	bool loadSGF(const QString &fileName, const QString &filter=0, bool fastLoad=false);

private:
	Board *board;
	Tree *tree;
	BoardWindow * boardwindow;

	int currentMove, capturesBlack, capturesWhite, caps_black, caps_white;
	Move *lastValidMove;
	GameMode gameMode;
	int boardSize;

	void updateVariationGhosts(Move *m);

};

#endif
