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


#ifndef SGFPARSER_H
#define SGFPARSER_H

#include "../defines.h"
#include "tree.h"

#include <QtCore>

class GameData;

class SGFParser
{
public:
	SGFParser(Tree *tree);
	~SGFParser();
	QString loadFile(const QString &fileName);
	GameData * initGame(const QString &toParse, const QString &fileName);

	bool parse(const QString &fileName, const QString &filter=0);

	bool doParse(const QString &toParseStr);
	bool doWrite(const QString &fileName, Tree *tree, GameData *gameData);
	bool exportSGFtoClipB(QString *str, Tree *tree, GameData *gameData);

protected:
	int minPos(int n1, int n2, int n3);
	bool corruptSgf(int where=0, QString reason=QString::null);

	bool parseProperty(const QString &toParse, const QString &prop, QString &result);

	void traverse(Move *t, GameData *gameData);
	void writeGameHeader(GameData *gameData);
	bool writeStream(Tree *tree, GameData *gameData);

private:
	bool setCodec(QString c = QString());

	QTextStream *stream;
	QTextCodec * readCodec;
    bool isRoot;
	Tree *tree;
	bool loadedfromfile;
};

#endif
