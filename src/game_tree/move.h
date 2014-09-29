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


#ifndef MOVE_H
#define MOVE_H

#include "defines.h"

#include <QtCore>

class Matrix;

class Move
{
public:
	Move(int board_size);
	Move(StoneColor c, int mx, int my, int n,  GamePhase phase, const Matrix &mat, bool clearAllMarks=false, const QString &s=NULL);
	Move(StoneColor c, int mx, int my, int n,  GamePhase phase, const QString &s=NULL);
    Move(Move *_parent, StoneColor _c, int _x, int _y);
	~Move();

	int getX() const 		{ return x; }
	int getY() const 		{ return y; }
	void setX(int n)		 { x = n; }
	void setY(int n) 		{ y = n; }
	StoneColor getColor() const 	{ return stoneColor; }
	void setColor(StoneColor c) 	{ stoneColor = c; }
	int getCapturesBlack() const 	{ return capturesBlack; }
	int getCapturesWhite() const 	{ return capturesWhite; }
    Matrix* getMatrix() 		{ return matrix; }
    void setMoveNumber(int n) 	{ moveNum = n; }
	int getMoveNumber() const 	{ return moveNum; }
	GamePhase getGamePhase() const 	{ return gamePhase; }
	void setGamePhase(GamePhase p) 	{ gamePhase = p; }
	QString &getNodeName() 		{ return nodeName; }
	void setNodeName(const QString &s) { nodeName = s; }
	QString &getComment() 		{ return comment; }
	void setComment(const QString &s) {
		comment = s;
		comment.squeeze();
	}
	QString &getUnknownProperty() 	{ return unknownProperty; }
	void setUnknownProperty(const QString &s) { unknownProperty = s; }
	const QString saveMove(bool isRoot);
	bool isTerritoryMarked() const 	{ return terrMarked; }
	void setTerritoryMarked(bool b=true) { terrMarked = b; }
	//void insertFastLoadMark(int x, int y, MarkType markType, const QString &txt=0);
	bool isScored() const { return scored; }
	void setScore(float b, float w) { scored = true; scoreBlack = b; scoreWhite = w; }
	void setScored(bool b=true) { scored = b; }
	float getScoreBlack() const { return scoreBlack; }
	float getScoreWhite() const { return scoreWhite; }
	void setOpenMoves(int mv) { openMoves = mv; }
	int getOpenMoves() { return openMoves; }
	void setTimeLeft(float time) { timeLeft = time; }
	float getTimeLeft() { return timeLeft; }
	void setTimeinfo(bool ti) { timeinfo = ti; }
	bool getTimeinfo() { return timeinfo; }
	// PL[] info: show if stone color keeps equal
	void clearPLinfo() { PLinfo = false; }
	void setPLinfo(StoneColor sc) { PLinfo = true; PLnextMove = sc; }
	bool getPLinfo() { return PLinfo; }
	StoneColor getPLnextMove() { return PLnextMove; }
	void setHandicapMove(bool b)		{ handicapMove = b; }
	bool isHandicapMove()			{return handicapMove ;}
	void setNodeIndex(int i)		{nodeIndex = i;}
	int getNodeIndex()			{return nodeIndex;}
	void addBrother(Move * b); 
	
	Move *brother, *son, *parent, *marker;
	bool checked;
	//QMap<int,FastLoadMark> *fastLoadMarkDict;
	bool isPassMove();

	int getNumBrothers();
	int getNumSons();
	bool hasParent(); 
	bool hasPrevBrother(); 
	bool hasNextBrother();
    bool checkMoveIsValid(StoneColor c, int x, int y);

    Move *hasSon(StoneColor c, int x, int y);
    Move *makeMove(StoneColor c, int x, int y, bool force = false);
    Move *makePass();

    StoneColor whoIsOnTurn();

private:
	StoneColor stoneColor, PLnextMove;
	int x, y, moveNum, capturesBlack, capturesWhite, openMoves , nodeIndex;
	float scoreBlack, scoreWhite;
	float timeLeft;
	Matrix *matrix;
	GamePhase gamePhase;
	QString comment;
	QString unknownProperty;
	QString nodeName;
	bool terrMarked, scored, timeinfo, PLinfo;
	bool handicapMove;
};

#endif
