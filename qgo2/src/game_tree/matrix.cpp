/*
* matrix.cpp
*/

#include "matrix.h"
//#include <stdlib.h>
//#ifndef NO_DEBUG
#include <iostream>
//#endif

Matrix::Matrix(int s)
: size(s)
{
	Q_ASSERT(size > 0 && size <= 36);
	
	init();
}

Matrix::Matrix(const Matrix &m)
{
	size = m.getSize();
	Q_ASSERT(size > 0 && size <= 36);
	
	init();
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			matrix[i][j] = m.at(i, j);
}

Matrix::~Matrix()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
		delete [] matrix[i];
	delete [] matrix;
	
	delete markTexts;
}

void Matrix::init()
{
	matrix = new short*[size];
	Q_CHECK_PTR(matrix);
	
	for (int i=0; i<size; i++)
	{
		matrix[i] = new short[size];
		Q_CHECK_PTR(matrix[i]);
		
		for (int j=0; j<size; j++)
			matrix[i][j] = stoneNone;
	}
	
	markTexts = NULL;
}

void Matrix::initMarkTexts()
{
	markTexts = new QStringList();
	Q_CHECK_PTR(markTexts);
}

void Matrix::clear()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			matrix[i][j] = stoneNone;
}

#ifndef NO_DEBUG
void Matrix::debug() const
{
	Q_ASSERT(size > 0 && size <= 36);
	
	int i, j;
	
	std::cout << "\n  ";
	for (i=0; i<size; i++)
		std::cout << (i+1)%10 << " ";
	std::cout << std::endl;
	
	for (i=0; i<size; i++)
	{
		std::cout << (i+1)%10 << " ";
		for (j=0; j<size; j++)
		{
#if 1
			switch (abs(matrix[j][i]))
			{
			case stoneNone:
			case stoneErase: std::cout << ". "; break;
			case stoneBlack: std::cout << "B "; break;
			case stoneWhite: std::cout << "W "; break;
			case markSquare*10: std::cout << "[ "; break;
			case markCircle*10: std::cout << "O "; break;
			case markTriangle*10: std::cout << "T "; break;
			case markCross*10: std::cout << "X "; break;
			case markText*10: std::cout << "A "; break;
			case markNumber*10: std::cout << "1 "; break;
			case markSquare*10+stoneBlack: std::cout << "S "; break;
			case markCircle*10+stoneBlack: std::cout << "C "; break;
			case markTriangle*10+stoneBlack: std::cout << "D "; break;
			case markCross*10+stoneBlack: std::cout << "R "; break;
			case markText*10+stoneBlack: std::cout << "A "; break;
			case markNumber*10+stoneBlack: std::cout << "N "; break;
			case markSquare*10+stoneWhite: std::cout << "s "; break;
			case markCircle*10+stoneWhite: std::cout << "c "; break;
			case markTriangle*10+stoneWhite: std::cout << "d "; break;
			case markCross*10+stoneWhite: std::cout << "r "; break;
			case markText*10+stoneWhite: std::cout << "a "; break;
			case markNumber*10+stoneWhite: std::cout << "n "; break;
			default: std::cout << "? ";
			}
#else
			std::cout << matrix[j][i] << " ";
#endif
		}
		std::cout << (i+1)%10 << std::endl;
	}
	
	std::cout << "  ";
	for (i=0; i<size; i++)
		std::cout << (i+1)%10 << " ";
	std::cout << std::endl;
	
	if (markTexts != NULL && !markTexts->isEmpty())
	{
		std::cout << markTexts->count() << " mark texts in the storage.\n";
		
		for (QStringList::Iterator it=markTexts->begin(); it != markTexts->end(); ++it)
			std::cout << it->toLatin1().constData() << std::endl;

	}
}
#endif

void Matrix::insertStone(int x, int y, StoneColor c, GameMode mode)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] = abs(matrix[x-1][y-1] / 10 * 10) + c;
	if (mode == modeEdit)
		matrix[x-1][y-1] *= -1;
}

void Matrix::removeStone(int x, int y)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] = abs(matrix[x-1][y-1] / 10 * 10);
}

void Matrix::eraseStone(int x, int y)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] = (abs(matrix[x-1][y-1] / 10 * 10) + stoneErase) * -1;
}

short Matrix::at(int x, int y) const
{	
	Q_ASSERT(x >= 0 && x < size &&
		y >= 0 && y < size);
	
	return matrix[x][y];
}

void Matrix::set(int x, int y, int n)
{
	Q_ASSERT(x >= 0 && x < size &&
		y >= 0 && y < size);
	
	matrix[x][y] = n;
}

void Matrix::insertMark(int x, int y, MarkType t)
{
	//Q_ASSERT(x > 0 && x <= size && y > 0 && y <= size);
	if (!(x > 0 && x <= size && y > 0 && y <= size))
		return;

	matrix[x-1][y-1] = (abs(matrix[x-1][y-1]) + 10*t) * (matrix[x-1][y-1] < 0 ? -1 : 1);
}

void Matrix::removeMark(int x, int y)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] %= 10;
	
	if (markTexts != NULL && !markTexts->isEmpty())
	{
		QStringList::Iterator it = getMarkTextIterator(x, y);
		if (*it != NULL)
			markTexts->erase(it);
	}
}

void Matrix::clearAllMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			matrix[i][j] %= 10;
		
		if (markTexts != NULL)
		{
			delete markTexts;
			markTexts = NULL;
		}
}

void Matrix::clearTerritoryMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	int data;
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			if ((data = abs(matrix[i][j] / 10)) == markTerrBlack ||
				data == markTerrWhite)
				matrix[i][j] %= 10;
}

void Matrix::absMatrix()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
	{
		for (int j=0; j<size; j++)
		{
			matrix[i][j] = abs(matrix[i][j]);
			if (matrix[i][j] == stoneErase)
				matrix[i][j] = stoneNone;
			if (matrix[i][j] % 10 == stoneErase)
				matrix[i][j] = matrix[i][j] / 10 * 10;
		}
	}
}

void Matrix::setMarkText(int x, int y, const QString &txt)
{
	Q_ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	// We only create the markTexts list if we really need it.
	if (markTexts == NULL)
		initMarkTexts();
	
	QStringList::Iterator it = getMarkTextIterator(x, y);
	if (it != markTexts->end())  // Mark already exists at this position, remove old text.
		markTexts->erase(it);
	
	QString tmp = QString::number(coordsToKey(x, y)) + "#" + txt;
	markTexts->append(tmp);
}

QStringList::Iterator Matrix::getMarkTextIterator(int x, int y)
{
	if (markTexts == NULL)
		return NULL;
	
	QString s, tmp;
	int pos, tmpX, tmpY, counter=0;
	bool check = false;
	long key;
	QStringList::Iterator it;
	
	for (it=markTexts->begin(); it != markTexts->end(); ++it)
	{
		s = (QString)(*it);
		
		// Get the splitting '#', everything left of it is our key.
		pos = s.indexOf('#');
		if (pos == -1)  // Whoops
		{
			qWarning("   *** Corrupt text marks in matrix! ***");
			continue;
		}
		
		// Transform key to coordinates
		tmp = s.left(pos);
		key = tmp.toLong(&check);
		if (!check)
		{
			qWarning("   *** Corrupt text marks in matrix! ***");
			continue;
		}
		keyToCoords(key, tmpX, tmpY);
		
		// This is our hit?
		if (tmpX == x && tmpY == y)
			return it;
		
		// Nope, wasn't
		counter++;
	}
	return markTexts->end();
}

const QString Matrix::getMarkText(int x, int y)
{
	// We didn't store any texts in this matrix.
	if (markTexts == NULL || markTexts->isEmpty())
		return NULL;
	
	QStringList::Iterator it = getMarkTextIterator(x, y);
	if (*it == NULL)  // Nope, this entry does not exist.
		return NULL;
	
	QString s = (QString)(*it);
	s = s.right(s.length() - s.indexOf('#') - 1);
	
	return s;
}

const QString Matrix::saveMarks()
{
	Q_ASSERT(size > 0 && size <= 36);
	
	QString txt, sSQ = "", sCR = "", sTR = "", sMA = "", sLB = "", sTB = "", sTW = "";
	int i, j, colw = 0, colb = 0;
	
	for (i=0; i<size; i++)
	{
		for (j=0; j<size; j++)
		{
			switch (abs(matrix[i][j] / 10))
			{
			case markSquare:
				if (sSQ.isEmpty())
					sSQ += "SQ";
				sSQ += "[" + coordsToString(i, j) + "]";
				break;
			case markCircle:
				if (sCR.isEmpty())
					sCR += "CR";
				sCR += "[" + coordsToString(i, j) + "]";
				break;
			case markTriangle:
				if (sTR.isEmpty())
					sTR += "TR";
				sTR += "[" + coordsToString(i, j) + "]";
				break;
			case markCross:
				if (sMA.isEmpty())
					sMA += "MA";
				sMA += "[" + coordsToString(i, j) + "]";
				break;
			case markText: 
			case markNumber:
				if (sLB.isEmpty())
					sLB += "LB";
				sLB += "[" + coordsToString(i, j);
				sLB += ":";
				txt = getMarkText(i+1, j+1);
				if (txt.isNull() || txt.isEmpty())
					sLB += "?";  // Whoops
				else
					sLB += txt;
				sLB += "]";
				break;
			case markTerrBlack:
				if (sTB.isEmpty())
				{
					sTB += "\nTB";
					colb = 0;
				}
				sTB += "[" + coordsToString(i, j) + "]";
				if (++colb % 15 == 0)
					sTB += "\n  ";
				break;
			case markTerrWhite:
				if (sTW.isEmpty())
				{
					sTW += "\nTW";
					colw = 0;
				}
				sTW += "[" + coordsToString(i, j) + "]";
				if (++colw % 15 == 0)
					sTW += "\n  ";
				break;
			default: continue;
			}
		}
	}
	
	return sSQ + sCR + sTR + sMA + sLB + sTB + sTW;
}

const QString Matrix::saveEditedMoves(Matrix *parent)
{
	Q_ASSERT(size > 0 && size <= 36);
	
	QString sAB="", sAW="", sAE="";
	int i, j;
	
	for (i=0; i<size; i++)
	{
		for (j=0; j<size; j++)
		{
			switch (matrix[i][j] % 10)
			{
			case stoneBlack * -1:
				if (parent != NULL &&
					parent->at(i, j) == stoneBlack)
					break;
				if (sAB.isEmpty())
					sAB += "AB";
				sAB += "[" + coordsToString(i, j) + "]";
				break;
				
			case stoneWhite * -1:
				if (parent != NULL &&
					parent->at(i, j) == stoneWhite)
					break;
				if (sAW.isEmpty())
					sAW += "AW";
				sAW += "[" + coordsToString(i, j) + "]";
				break;
				
			case stoneErase * -1:
				if (parent != NULL &&
					(parent->at(i, j) == stoneNone ||
					parent->at(i, j) == stoneErase))
					break;
				if (sAE.isEmpty())
					sAE += "AE";
				sAE += "[" + coordsToString(i, j) + "]";
				break;
			}
		}
	}
	
	return sAB + sAW + sAE;
}

const QString Matrix::printMe(ASCII_Import *charset)
{
	Q_ASSERT(size > 0 && size <= 36);
	
#if 0
	qDebug("BLACK STONE CHAR %c\n"
		"WHITE STONE CHAR %c\n"
		"STAR POINT  CHAR %c\n"
		"EMPTY POINT CHAR %c\n",
		charset->blackStone,
		charset->whiteStone,
		charset->starPoint,
		charset->emptyPoint);
#endif
	
	int i, j;
	QString str;
	
	str = "\n    ";
	for (i=0; i<size; i++)
		str += QString(QChar(static_cast<const char>('A' + (i<8?i:i+1)))) + " ";
	str += "\n   +";
	for (i=0; i<size-1; i++)
		str += "--";
	str += "-+\n";
	
	for (i=0; i<size; i++)
	{
		if (size-i < 10) str += " ";
		str += QString::number(size-i) + " |";
		for (j=0; j<size; j++)
		{
			switch (abs(matrix[j][i] % 10))
			{
			case stoneBlack: str += QChar(charset->blackStone); str += " "; break;
			case stoneWhite: str += QChar(charset->whiteStone); str += " "; break;
			default:
				// Check for starpoints
				if (size > 9)  // 10x10 or larger
				{
					if ((i == 3 && j == 3) ||
						(i == size-4 && j == 3) ||
						(i == 3 && j == size-4) ||
						(i == size-4 && j == size-4) ||
						(i == (size+1)/2 - 1 && j == 3) ||
						(i == (size+1)/2 - 1 && j == size-4) ||
						(i == 3 && j == (size+1)/2 - 1) ||
						(i == size-4 && j == (size+1)/2 - 1) ||
						(i == (size+1)/2 - 1 && j == (size+1)/2 - 1))
					{
						str += QChar(charset->starPoint);
						str += " ";
						break;
					}
				}
				else  // 9x9 or smaller
				{
					if ((i == 2 && j == 2) ||
						(i == 2 && j == size-3) ||
						(i == size-3 && j == 2) ||
						(i == size-3 && j == size-3))
					{
						str += QChar(charset->starPoint);
						str += " ";
						break;
					}
				}
				
				str += QChar(charset->emptyPoint);
				str += " ";
				break;
			}
		}
		str = str.left(str.length()-1);
		str += "| ";
		if (size-i < 10) str += " ";
		str += QString::number(size-i) + "\n";
	}
	
	str += "   +";
	for (i=0; i<size-1; i++)
		str += "--";
	str += "-+\n    ";
	for (i=0; i<size; i++)
		str += QString(QChar(static_cast<const char>('A' + (i<8?i:i+1)))) + " ";
	str += "\n";
	
	return str;
}
