/*
* move.cpp
*/

#include "move.h"
#include "matrix.h"

Move::Move(int board_size)
{
	brother = NULL;
	son = NULL;
	parent = NULL;
	marker = NULL;
	stoneColor = stoneNone;
	x = y = -1;
	gamePhase = phaseOngoing;
	moveNum = 0;
	terrMarked = false;
	capturesBlack = capturesWhite = 0;
	checked = true;
	fastLoadMarkDict = NULL;
	scored = false;
	scoreWhite = scoreBlack = 0;
	PLinfo = false;
	
	matrix = new Matrix(board_size);
}

Move::Move(StoneColor c, int mx, int my, int n, GamePhase phase, const Matrix &mat, const QString &s)
: stoneColor(c), x(mx), y(my), moveNum(n), gamePhase(phase), comment(s)
{
	brother = NULL;
	son = NULL;
	parent = NULL;
	marker = NULL;
	capturesBlack = capturesWhite = 0;
	terrMarked = false;
	checked = true;
	fastLoadMarkDict = NULL;
	scored = false;
	scoreWhite = scoreBlack = 0;
	PLinfo = false;
	
	matrix = new Matrix(mat);
	// Make all matrix values positive
	matrix->absMatrix();
}

Move::Move(StoneColor c, int mx, int my, int n, GamePhase phase, const QString &s)
: stoneColor(c), x(mx), y(my), moveNum(n), gamePhase(phase), comment(s)
{
	brother = NULL;
	son = NULL;
	parent = NULL;
	marker = NULL;
	capturesBlack = capturesWhite = 0;
	terrMarked = false;
	checked = false;
	matrix = NULL;
	fastLoadMarkDict = NULL;
	scored = false;
	scoreWhite = scoreBlack = 0;
	PLinfo = false;
}

Move::~Move()
{
	delete matrix;
	if (fastLoadMarkDict) //TODO make sure this is the proper way 
//		qDeleteAll(*fastLoadMarkDict);
		fastLoadMarkDict->clear();
	delete fastLoadMarkDict;
}

// We do not overwrite the operator == as well, as this is used to compare the
// pointers for faster operation.
bool Move::equals(Move *m)
{
	if (m == NULL)
		return false;
	
	if (x == m->getX() && y == m->getY() &&
		stoneColor == m->getColor() &&
		moveNum == m->getMoveNumber() &&
		gamePhase == m->getGamePhase())
		return true;
	
	return false;
}

const QString Move::saveMove(bool isRoot)
{
	QString str;
	
	if (!isRoot)
		str += ";"; //"\n;";
	
	if (x != -1 && y != -1 && gamePhase != phaseEdit)
	{
		// Write something like 'B[aa]'
		str += stoneColor == stoneBlack ? "B" : "W";
 		str += "[" + Matrix::coordsToString(x-1, y-1) + "]";
	}
	
	// Save edited moves
	str += matrix->saveEditedMoves(parent != NULL ? parent->getMatrix() : 0);
	
	// Save marks
	str += matrix->saveMarks();
	
	// Add nodename, if we have one
	if (!nodeName.isNull() && !nodeName.isEmpty())
	{
		// simpletext
		str += "N[";
		str += nodeName;
		str += "]";
	}

	// Add next move's color
	if (PLinfo)
	{
		if (PLnextMove == stoneBlack)
			str += "PL[B]";
		else
			str += "PL[W]";
	}

	// Add comment, if we have one
	if (!comment.isNull() && !comment.isEmpty())
	{
		// text
		QString tmp = comment;
		int pos = 0;
		while ((  (pos = tmp.indexOf("]", pos)) != -1) && (static_cast<signed int>(pos) < tmp.length()))
		{
			tmp.replace(pos, 1, "\\]");
			pos += 2;
		}
		str += "C[";
		str += tmp;
		str += "]";
	}

	// time info
	if (timeinfo && !isRoot && (int) timeLeft)
	{
		if (stoneColor == stoneBlack)
			str += "BL[";
		else
			str += "WL[";
		str += QString::number(timeLeft);
		str += "]";

		// open moves info
		if (openMoves > 0)
		{
			if (stoneColor == stoneBlack)
				str += "OB[";
			else
				str += "OW[";
			str += QString::number(openMoves);
			str += "]";
		}
	}

	// Add unknown properties, if we have some
	if (!unknownProperty.isNull() && !unknownProperty.isEmpty())
	{
		// complete property
		str += unknownProperty;
	}
	
	return str;
}

void Move::insertFastLoadMark(int x, int y, MarkType markType, const QString &txt)
{
	if (fastLoadMarkDict == NULL)
	{
		fastLoadMarkDict = new QMap<int, FastLoadMark>;
/////TODO		fastLoadMarkDict->setAutoDelete(TRUE);
	}
	
	FastLoadMark *flm = new FastLoadMark;
	flm->x = x;
	flm->y = y;
	flm->t = markType;
	flm->txt = txt;
	
	fastLoadMarkDict->insert(Matrix::coordsToKey(x, y), *flm);
}

bool Move::isPassMove()
{
	if ((x==20) && (y==20))
		return true;

	return false;
}

int Move::getNumBrothers()
{
	if (parent ==NULL)
		return 0;

	Move *tmp = parent->son;
	int counter = 0;
	
	while ((tmp = tmp->brother) != NULL)
		counter ++;
	
	return counter;
}

int Move::getNumSons()
{
	Move *tmp = son;
	
	if (tmp == NULL)
		return 0;
	
	int counter = 1;
	while ((tmp = tmp->brother) != NULL)
		counter ++;
	
	return counter;
}


bool Move::hasParent()
{
	return (parent != NULL);
}

bool Move::hasPrevBrother()
{

	if (parent == NULL)
		return false;
	else
		return (this != parent->son);
}

bool Move::hasNextBrother()
{
	return (brother != NULL);
}
