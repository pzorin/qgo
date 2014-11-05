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


/*
* sgfparser.cpp
* This class deals with SGF input. It puts data into a text stream (for cilpboard exchange)
* The parsing delivers orders solely into the game tree - namely the 'tree' class
*/


#include "sgfparser.h"
#include "../defines.h"
#include "move.h"
#include "tree.h"
#include "matrix.h"
#include "gamedata.h"

#include <QMessageBox>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#define STR_OFFSET 2000
/* #define DEBUG_CODEC */

struct Position { int x, y; };
struct MoveNum {int n; };

/* FIXME consider a parser for other file types, what few there are.
 * but for instance the tygem protocols use .gibo format and that code
 * is super ugly.  We could make a better parser for it and put it in here
 * also allowing the reading of files.  This will become more useful if
 * we add in the stuff to retrieve game records from the server.
 * oro also has its own format I think and there's a few others.  There's
 * no reason why we shouldn't be able to handle them.
 * IGS also has its own format as probably does cyberoro */

class MyString
{
public:
	MyString()
	{
	}
	
	MyString(const QString &src)
	{
		Str = src;
		strLength = src.length();
	}

	virtual ~MyString()
	{
	}
	
	virtual const QChar at(uint i) const { return Str.at(i); }
	
	virtual uint next_nonspace (uint i) const
	{
		// ignore lower characters, too
		while (at(i).isSpace() || // == ' ' || at(i) == '\n' || at(i) == '\t' || 
			   (at(i) >= 'a' && at(i) <= 'z'))
			i++;
		return i;
	}

    virtual uint isProperty(const char *c, unsigned int index) const
    // Check if a property identified by the C string *c begins at index.
    // Return poition of '[' (always >0 assuming *c is not empty) if yes, zero if no.
    {
        if (index+strlen(c) >= strLength)
            return 0;

        while (*c != '\0')
        {
            if (*(c++) != Str.at(index++))
                return 0;
        }

        uint tmppos = next_nonspace(index);
        if (at(tmppos) == '[')
            return tmppos;
        else return 0;
    }
	
	virtual int find(const char *c, unsigned int index) const
	{
		if (index >= strLength)
			return -1;
		
		// Offset. Hope that is enough. TODO Long comments check?
		unsigned int l = index+STR_OFFSET<strLength ? index+STR_OFFSET : strLength,
			cl = strlen(c),
			i,
			found;
		
		do {
			found = i = 0;
			do {
				if (Str.data()[index+i] != c[i])			//calls QChar::unicode() which is slow
					break;
				found ++;
				if (found == cl)
					return index;
			} while (i++ < cl);
		} while (index++ < l-1);
		if (index == l)
			return -1;
		return index;
	}
	
	virtual int find(char c, unsigned int index) const
	{
		if (index >= strLength)
			return -1;

		// Offset. Hope that is enough. TODO Long comments check?
		unsigned int l = index+STR_OFFSET<strLength ? index+STR_OFFSET : strLength;
		
		while (Str.data()[index] != c && index++ < l-1) {};			//calls QChar::unicode() which is slow
		if (index == l)
			return -1;
		return index;
	}
	
	virtual unsigned int length() const
	{
		return strLength;
	}
	
	unsigned int strLength;
	
	char * getStr() const {return (char *)Str.toLatin1().constData();}
//private:
	QString Str;
};


SGFParser::SGFParser(Tree * _tree)
//: boardHandler(bh)
{
//	CHECK_PTR(boardHandler);
	stream = NULL;
	tree = _tree;
	readCodec = 0;
	loadedfromfile = false;
//	xmlParser = NULL;
}

SGFParser::~SGFParser()
{
//	delete xmlParser;
}

bool SGFParser::parse(const QString &fileName, const QString &/*filter*/)
{
	if (fileName.isNull() || fileName.isEmpty())
	{
		qWarning("No filename given!");
		return false;
	}
	
//	CHECK_PTR(boardHandler);
	
	QString toParse = loadFile(fileName);
	if (toParse.isNull() || toParse.isEmpty())
		return false;

	/*
	// Check for filter, if given
	if (!filter.isNull())
	{
		// XML
		if (filter == Board::tr("XML"))
		{
			// Init xmlparser if not yet done
			if (xmlParser == NULL)
				xmlParser = new XMLParser(boardHandler);
			xmlParser->parse(fileName);
			return true;
		}
	}
	*/
	if (!initGame(toParse, fileName))
		return corruptSgf();
	return doParse(toParse);
}
/*
// Called from clipboard SGF import
bool SGFParser::parseString(const QString &toParse)
{
	if (toParse.isNull() || toParse.isEmpty() || !initGame(toParse, NULL))
		return corruptSgf();
	return doParse(toParse);
}
*/

QString SGFParser::loadFile(const QString &fileName)
{
	qDebug("Trying to load file <%s>", fileName.toUtf8().constData());
	
	QFile file(fileName);
	
	if (!file.exists())
	{
        qDebug() << "Could not find file: " << fileName;
		return NULL;
	}
	
	if (!file.open(QIODevice::ReadOnly))
	{
        qDebug() << "Could not open file: " << fileName;
		return NULL;
	}
	
	QTextStream txt(&file);
	stream = &txt;
	if (!setCodec())
	{
        qDebug() << "Invalid text encoding given. Please check preferences!";
		return NULL;
	}
	
	QString toParse;
	int i = 10;
	while (!txt.atEnd() && i--)
		toParse.append(txt.readLine() + "\n");
	QString tmp="";
	if (!parseProperty(toParse, "CA", tmp))		//codec
		return NULL;
	if (!tmp.isEmpty())
	{
		if(!setCodec(tmp))
			return NULL;
		toParse.clear();
		txt.seek(0);
	}
	while (!txt.atEnd())
		toParse.append(txt.readLine() + "\n");
	readCodec = stream->codec();
	file.close();
#ifdef DEBUG_CODEC
	QMessageBox::information(0, "READING", toParse);
#endif
	loadedfromfile = true;		//no need to check the codec later
	return toParse;
}

bool SGFParser::setCodec(QString c)
{
	QTextCodec *codec = NULL;

	if(c == QString())
	{
		QSettings settings;

		// TODO : make sure we are getting the proper codec value from settings
		if (settings.contains("CODEC"))
			codec = QTextCodec::codecForName(settings.value("CODEC").toByteArray());
	}
	else
		codec = QTextCodec::codecForName(c.toLatin1().constData());
#ifdef DEBUG_CODEC
	QMessageBox::information(0, "LOCALE", codec);
#endif
/*
	//switch (static_cast<Codec>(settings.value("CODEC"))
	
	switch (QTextCodec::codecForName(settings.value("CODEC")))
	{
	case codecNone:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "NONE");
#endif
		break;
		
	case codecBig5:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "Big5");
#endif
#ifndef Q_OS_WIN
		codec = new QBig5Codec();
#endif
		break;

	case codecEucJP:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "EUC JP");
#endif
		codec = new QEucJpCodec();
		break;
		
	case codecJIS:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "JIS");
#endif
		codec = new QJisCodec();
		break;
		
	case codecSJIS:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "Shift JIS");
#endif
		codec = new QSjisCodec();
		break;
		
	case codecEucKr:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "EUC KR");
#endif
		codec = new QEucKrCodec();
		break;
		
	case codecGBK:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "GBK");
#endif
		codec = new QGbkCodec();
		break;
		
	case codecTscii:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "TSCII");
#endif
		codec = new QTsciiCodec();
		break;
		
	default:
		return false;
	}
*/	
	if ( stream != NULL)
	{
		if (codec != NULL)
			stream->setCodec(codec);
		else 
			stream->setCodec(QTextCodec::codecForLocale());
	}

	return true;
}

bool SGFParser::doParse(const QString &toParseStr)
{
	if (toParseStr.isNull() || toParseStr.isEmpty())
	{
		qWarning("Failed loading from file. Is it empty?");
		return false;
	}
	QString tmp;

	if(!loadedfromfile)
	{
		/* This bit of ugliness is because sgfs are used to duplicate boards as well
		 * as load from file FIXME */
		parseProperty(toParseStr, "CA", tmp);		//codec
		if (!tmp.isEmpty())
			readCodec = QTextCodec::codecForName(tmp.toLatin1().constData());
	}
	
	const MyString *toParse = NULL;

//////TODO	if (static_cast<Codec>(setting->readIntEntry("CODEC")) == codecNone)
/////////		toParse = new MySimpleString(toParseStr);
///////	else
		toParse = new MyString(toParseStr);

	Q_CHECK_PTR(toParse);
	
	int 	pos = 0,
		posVarBegin = 0,
		posVarEnd = 0,
		posNode = 0,
		moves = 0,
		i, x=-1, y=-1;

	int 	a_offset = QChar::fromLatin1('a').unicode() - 1 ;

	unsigned int pointer = 0,
		strLength = toParse->length();
	bool black = true,
		setup = false,
        old_label = false;
	isRoot = true;
	bool remember_root;
	QString unknownProperty;
	State state;
	MarkType markType;
	QString moveStr, commentStr;
	Position *position;
	MoveNum *moveNum;
	QStack<Move*> stack;
	QStack<MoveNum*> movesStack;
	/* FIXME toRemove, et., al., appears unused Remove it */
	QStack<Position*> toRemove;
/*
////TODO	stack.setAutoDelete(false);
	movesStack.setAutoDelete(true);
	toRemove.setAutoDelete(true);
*/
	// Initialises the tree with board size
	parseProperty(toParseStr, "SZ", tmp);
//	Tree *tree = new Tree(tmp.isEmpty() ? 19 : tmp.toInt()) ;// boardHandler->getTree();
	
	state = stateVarBegin;
	
	bool cancel = false;
    //FIXME abort does nothing!!
	
	// qDebug("File length = %d", strLength);
	
    tree->setLoadingSGF(true);
	
	QString sss="";
    do {
		posVarBegin = toParse->find('(', pointer);
		posVarEnd = toParse->find(')', pointer);
		posNode = toParse->find(';', pointer);
		
		pos = minPos(posVarBegin, posVarEnd, posNode);

		// Switch states

		// Node -> VarEnd
		if (state == stateNode && pos == posVarEnd)
			state = stateVarEnd;
		
		// Node -> VarBegin
		if (state == stateNode && pos == posVarBegin)
			state = stateVarBegin;
		
		// VarBegin -> Node
		else if (state == stateVarBegin && pos == posNode)
			state = stateNode;
		
		// VarEnd -> VarBegin
		else if (state == stateVarEnd && pos == posVarBegin)
			state = stateVarBegin;
		
		// qDebug("State after switch = %d", state);
		
		// Do the work
		switch (state)
		{
		case stateVarBegin:
			if (pos != posVarBegin)
			{
				delete toParse;
				return corruptSgf(pos);
			}
			
			// qDebug("Var BEGIN at %d, moves = %d", pos, moves);
			
			stack.push(tree->getCurrent());
			moveNum = new MoveNum;
			moveNum->n = moves;
			movesStack.push(moveNum);
			pointer = pos + 1;
			break;
			
		case stateVarEnd:
			if (pos != posVarEnd)
			{
				delete toParse;
				return corruptSgf(pos);
			}
			
			// qDebug("VAR END");
			
			if (!movesStack.isEmpty() && !stack.isEmpty())
			{
				Move *m = stack.pop();
				Q_CHECK_PTR(m);
				x = movesStack.pop()->n;
				
				// qDebug("Var END at %d, moves = %d, moves from stack = %d", pos, moves, x);
				
				for (i=moves; i > x; i--)
				{
					position = toRemove.pop();
					if (position == NULL)
						continue;
///////////////////			boardHandler->getStoneHandler()->removeStone(position->x, position->y);
//					tree->removeStone(position->x, position->y);
					// qDebug("Removing %d %d from stoneHandler.", position->x, position->y);
				}
				
				moves = x;
 							
				
				tree->setCurrent(m);
			}
			pointer = pos + 1;
			break;
			
		case stateNode:
			if (pos != posNode)
			{
				delete toParse;
				return corruptSgf(pos);
			}
			
			// qDebug("Node at %d", pos);
			commentStr = QString();
			setup = false;
			markType = markNone;
			
			// Create empty node
			remember_root = isRoot;
			if (!isRoot)
			{
/////////////////////		boardHandler->createMoveSGF();
//				qDebug("############### Before creating move ####################");
//				qDebug(toParse->Str.toLatin1().constData());
				//tree->createMoveSGF();
				/* This does happen, why??? FIXME */
//				qDebug("###############                      ####################");
//				qDebug(toParse->Str.toLatin1().constData());
//				qDebug("############### After creating move ####################");
				unknownProperty = QString();
#ifdef FIXME	//why is this a warning? this happens on loading a file with time info
				if (tree->getCurrent()->getTimeinfo())
				qWarning("*** Timeinfo set !!!!");
#endif //FIXME
				//tree->getCurrent()->setTimeinfo(false);
			}
			else
				isRoot = false;
						
			Property prop;
			pos ++;

			do {
				uint tmppos=0;
				pos = toParse->next_nonspace (pos);
				
                if ((tmppos = toParse->isProperty("B",pos)))
				{
					prop = moveBlack;
					pos = tmppos;
					black = true;
				}
                else if ((tmppos = toParse->isProperty("W",pos)))
				{
					prop = moveWhite;
					pos = tmppos;
					black = false;
				}
                else if ((tmppos = toParse->isProperty("N",pos)))
				{
					prop = nodeName;
					pos = tmppos;
				}
                else if ((tmppos = toParse->isProperty("AB",pos)))
				{
					prop = editBlack;
					pos = tmppos;
					setup = true;
					black = true;
				}
                else if ((tmppos = toParse->isProperty("AW",pos)))
				{
					prop = editWhite;
					pos = tmppos;
					setup = true;
					black = false;
				}
                else if ((tmppos = toParse->isProperty("AE",pos)))
				{
					prop = editErase;
					pos = tmppos;
					setup = true;
				}
                else if ((tmppos = toParse->isProperty("TR",pos)))
				{
					prop = editMark;
					markType = markTriangle;
					pos = tmppos;
				}
                else if ((tmppos = toParse->isProperty("CR",pos)))
				{
					prop = editMark;
					markType = markCircle;
					pos = tmppos;
				}
                else if ((tmppos = toParse->isProperty("SQ",pos)))
				{
					prop = editMark;
					markType = markSquare;
					pos = tmppos;
				}
                else if ((tmppos = toParse->isProperty("MA",pos)))
				{
					prop = editMark;
					markType = markCross;
					pos = tmppos;
				}
				// old definition
                else if ((tmppos = toParse->isProperty("M",pos)))
				{
					prop = editMark;
					markType = markCross;
					pos = tmppos;
				}
                else if ((tmppos = toParse->isProperty("LB",pos)))
				{
					prop = editMark;
					markType = markText;
					pos = tmppos;
					old_label = false;
				}
				// Added old L property. This is not SGF4, but many files contain this tag.
                else if ((tmppos = toParse->isProperty("L",pos)))
				{
					prop = editMark;
					markType = markText;
					pos = tmppos;
					old_label = true;
				}
                else if ((tmppos = toParse->isProperty("C",pos)))
				{
					prop = comment;
					pos = tmppos;
				}
                else if ((tmppos = toParse->isProperty("TB",pos)))
				{
					prop = editMark;
					markType = markTerrBlack;
					pos = tmppos;
					black = true;
				}
                else if ((tmppos = toParse->isProperty("TW",pos)))
				{
					prop = editMark;
					markType = markTerrWhite;
					pos = tmppos;
					black = false;
				}
                else if ((tmppos = toParse->isProperty("BL",pos)))
				{
					prop = timeLeft;
					pos = tmppos;
					black = true;
				}
                else if ((tmppos = toParse->isProperty("WL",pos)))
				{
					prop = timeLeft;
					pos = tmppos;
					black = false;
				}
                else if ((tmppos = toParse->isProperty("OB",pos)))
				{
					prop = openMoves;
					pos = tmppos;
					black = true;
				}
                else if ((tmppos = toParse->isProperty("OW",pos)))
				{
					prop = openMoves;
					pos = tmppos;
					black = false;
				}
                else if ((tmppos = toParse->isProperty("PL",pos)))
				{
					prop = nextMove;
					pos = tmppos;
				}
                    else if ((tmppos = toParse->isProperty("RG",pos)))
				{
					prop = unknownProp;
					pos = tmppos;
          				setup = true;
				}
				// Empty node
				else if (toParse->at(pos) == ';' || toParse->at(pos) == '(' || toParse->at(pos) == ')')
				{
					qDebug("Found empty node at %d", pos);
					while (toParse->at(pos).isSpace())
						pos++;
					continue;
				}
				else
				{
					// handle like comment
					prop = unknownProp;
					pos = toParse->next_nonspace (pos);
					//qDebug("SGF: next nonspace (1st):" + QString(toParse->at(pos)) + QString(toParse->at(pos+1)) + QString(toParse->at(pos+2)));
				}
				
				//qDebug("Start do loop : FOUND PROP %d, pos at %d now", prop, pos);
				//qDebug(toParse->getStr());			//causes crash
				// Next is one or more '[xx]'.
				// Only one in a move property, several in a setup propery
				do {
					if (toParse->at(pos) != '[' && prop != unknownProp)
					{
						delete toParse;
						return corruptSgf(pos);
					}
					
					// Empty type
					if (toParse->at(pos+1) == ']')
					{
						// CGoban stores pass as 'B[]' or 'W[]'
						if (prop == moveBlack || prop == moveWhite)
						{
							tree->doPass(true);
							
							// Remember this move for later, to remove from the matrix.
							position = new Position;
							position->x = x;
							position->y = y;
							toRemove.push(position);
							moves ++;
						}
						
						pos += 2;
						continue;
					}
					
					switch (prop)
					{
					case moveBlack:
					case moveWhite:
						// rare case: root contains move or placed stone:
						if (remember_root)
						{
							qDebug("root contains stone -> node created");
							/* Something is screwy here, inconsistencies
							 * in the way SGF's are treated. Like the below:
							 * the whole point of "remember_root", FIXME*/
							tree->addEmptyMove();
							isRoot = false;
							unknownProperty = QString();
#ifdef FIXME	//why is this a warning?
							if (tree->getCurrent()->getTimeinfo())
								qWarning("*** Timeinfo set (2)!!!!");
#endif //FIXME
							//tree->getCurrent()->setTimeinfo(false);
						}
					case editBlack:
					case editWhite:
					case editErase:
					{
						x = toParse->at(pos+1).unicode() - a_offset ;// - 'a';// + 1;
						y = toParse->at(pos+2).unicode() - a_offset ; //- 'a' + 1;

						int x1, y1;
						bool compressed_list;

						// check for compressed lists
						if (toParse->at(pos+3) == ':')
						{
							x1 = toParse->at(pos+4).unicode() -a_offset;// - 'a' + 1;
							y1 = toParse->at(pos+5).unicode() -a_offset;// - 'a' + 1;
							compressed_list = true;
						}
						else
						{
							x1 = x;
							y1 = y;
							compressed_list = false;
						}
/*								
*						TODO Do we nned this when the tree is created from file ?
*						boardHandler->setModeSGF(setup || compressed_list ? modeEdit : modeNormal);
*/
						
						int i, j;
						for (i = x; i <= x1; i++)
							for (j = y; j <= y1; j++)
                            {
                                if (prop == editErase)
								{
									tree->addStoneToCurrentMove(stoneErase, i, j);
								}
								else
								{
									if(setup)
									{
                                        if ((!remember_root) && (stack.top() == tree->getCurrent()))
                                            tree->addEmptyMove(); //if this is first in branch we need to add an empty move

										tree->addStoneToCurrentMove(black ? stoneBlack : stoneWhite, i, j);
									}
									else
                                    {
                                        Move *result = tree->getCurrent()->makeMove(black ? stoneBlack : stoneWhite, i, j);
                                        if (result)
                                            tree->setCurrent(result);
                                    }
								}
								// tree->getCurrent()->getMatrix()->debug();
								//qDebug("ADDING MOVE %s %d/%d", black?"B":"W", x, y);
								
								// Remember this move for later, to remove from the matrix.
								position = new Position;
								position->x = i;
								position->y = j;
								toRemove.push(position);
								moves ++;
							}
												
						if (compressed_list)
							// Advance pos by 7
							pos += 7;
						else
							// Advance pos by 4
							pos += 4;
						break;
					}
						
					case nodeName:
					{
						commentStr = QString();
						bool skip = false;
						
						while (toParse->at(++pos) != ']')
						{
							if (static_cast<unsigned int>(pos) > strLength-1)
							{
								qDebug("SGF: Nodename string ended immediately");
								delete toParse;
								return corruptSgf(pos, "SGF: Nodename string ended immediately");
							}

							// white spaces
							if (toParse->at(pos) == '\\')
							{
								while (toParse->at(pos+1).isSpace() &&
									static_cast<unsigned int>(pos) < strLength-2)
									pos++;
								if (toParse->at(pos).isSpace())
									pos++;

								// case: "../<cr><lf>]"
								if (toParse->at(pos) == ']')
								{
									pos--;
									skip = true;
								}
							}

							// escaped chars: '\', ']', ':'
							if (!(toParse->at(pos) == '\\' &&
								(toParse->at(pos+1) == ']' ||
								 toParse->at(pos+1) == '\\' ||
								 toParse->at(pos+1) == ':')) &&
								 !skip &&
								 // no formatting
								!(toParse->at(pos) == '\n') &&
								!(toParse->at(pos) == '\r'))
								commentStr.append(toParse->at(pos));
						}
						
					 	//qDebug("Node name read: %s", commentStr.toLatin1().constData());
						if (!commentStr.isEmpty())
							// add comment; skip 'C[]'
							tree->getCurrent()->setNodeName(commentStr);
						pos++;
						break;
					}

					case comment:
					{
						commentStr = QString();
						bool skip = false;
						
						while (toParse->at(++pos) != ']' ||
							(toParse->at(pos-1) == '\\' && toParse->at(pos) == ']'))
						{
							if (static_cast<unsigned int>(pos) > strLength-1)
							{
								qDebug("SGF: Comment string ended immediately");
								delete toParse;
								return corruptSgf(pos, "SGF: Comment string ended immediately");
							}

							// white spaces
							if (toParse->at(pos) == '\\')
							{
								while (toParse->at(pos+1).isSpace() &&
									static_cast<unsigned int>(pos) < strLength-2)
									pos++;
								if (toParse->at(pos).isSpace())
									pos++;

								// case: "../<cr><lf>]"
								if (toParse->at(pos) == ']')
								{
									pos--;
									skip = true;
								}
							}

							// escaped chars: '\', ']', ':'
							if (!(toParse->at(pos) == '\\' &&
								(toParse->at(pos+1) == ']' ||
								 toParse->at(pos+1) == '\\' ||
								 toParse->at(pos+1) == ':')) &&
								 !skip)
								commentStr.append(toParse->at(pos));
						}

						//qDebug("Comment read: %s", commentStr.toLatin1().constData());
						if (!commentStr.isEmpty())
						{
							// add comment; skip 'C[]'
							if(readCodec)
								tree->getCurrent()->setComment(readCodec->toUnicode(commentStr.toLatin1().constData()));
							else
								tree->getCurrent()->setComment(commentStr.toLatin1().constData());
						}
						pos ++;
						break;
					}

					case unknownProp:
					{
						// skip if property is known anyway
						bool skip = false;

						// save correct property name (or p.n. + '[')
						commentStr = QString(toParse->at(pos));
						commentStr += toParse->at(tmppos = toParse->next_nonspace (pos + 1));
						pos = tmppos;

						// check if it's really necessary to hold properties
						// maybe they are handled at another position
						if (commentStr == "WR" ||
							commentStr == "BR" ||
							commentStr == "PW" ||
							commentStr == "PB" ||
							commentStr == "SZ" ||
							commentStr == "KM" ||
							commentStr == "HA" ||
							commentStr == "RE" ||
							commentStr == "DT" ||
							commentStr == "PC" ||
							commentStr == "CP" ||
							commentStr == "GN" ||
							commentStr == "OT" ||
							commentStr == "TM" ||
							// now: general options
							commentStr == "GM" ||
							commentStr == "ST" ||
							commentStr == "AP" ||
							commentStr == "FF")
						{
							skip = true;
						}
						sss= toParse->at(pos);
						while (toParse->at(++pos) != ']' ||
							(toParse->at(pos-1) == '\\' && toParse->at(pos) == ']'))
						{
							if (static_cast<unsigned int>(pos) > strLength-1)
							{
								qDebug("SGF: Unknown property ended immediately");
								delete toParse;
								return corruptSgf(pos, "SGF: Unknown property ended immediately");
							}
              						sss= toParse->at(pos);
							if (!skip)
								commentStr.append(toParse->at(pos));
						}

						if (!skip)
							commentStr.append("]");

						// qDebug("Comment read: %s", commentStr.latin1());
						if ((!commentStr.isEmpty()) && (!skip))
						{
							// cumulate unknown properties; skip empty property 'XZ[]'
							unknownProperty += commentStr;
							tree->getCurrent()->setUnknownProperty(unknownProperty);
						}
						pos ++;
            					sss= toParse->at(pos);
						break;
					}

					case editMark:
						// set moveStr for increment labels of old 'L[]' property
						moveStr = "A";
						while (toParse->at(pos) == '[' &&
							static_cast<unsigned int>(pos) < strLength)
						{
							x = toParse->at(pos+1).unicode() -a_offset;// - 'a' + 1;
							y = toParse->at(pos+2).unicode() -a_offset;// - 'a' + 1;
							// qDebug("MARK: %d at %d/%d", markType, x, y);
							pos += 3;
							
							// 'LB' property? Then we need to get the text
							if (markType == markText && !old_label)
							{
								if (toParse->at(pos) != ':')
								{
									delete toParse;
									return corruptSgf(pos);
								}
								moveStr = "";
								while (toParse->at(++pos) != ']' &&
									static_cast<unsigned int>(pos) < strLength)
									moveStr.append(toParse->at(pos));
								// qDebug("LB TEXT = %s", moveStr.latin1());
								// It might me a number mark?
								bool check = false;
								moveStr.toInt(&check);  // Try to convert to Integer
								// treat integers as characters...
								check = false;
								
								if (check)
									tree->getCurrent()->getMatrix()->
									insertMark(x, y, markNumber);  // Worked, its a number
								else
									tree->getCurrent()->getMatrix()->
									insertMark(x, y, markType);    // Nope, its a letter
								tree->getCurrent()->getMatrix()->
										setMarkText(x, y, moveStr);
							
								/*else	//fastload
								{
									if (check)  // Number
										tree->getCurrent()->insertFastLoadMark(x, y, markNumber);
									else        // Text
										tree->getCurrent()->insertFastLoadMark(x, y, markType, moveStr);
								}*/
							}
							else
							{
								int x1, y1;
								bool compressed_list;

								// check for compressed lists
								if (toParse->at(pos) == ':')
								{
									x1 = toParse->at(pos+1).unicode() -a_offset;// - 'a' + 1;
									y1 = toParse->at(pos+2).unicode() -a_offset;// - 'a' + 1;
									compressed_list = true;
								}
								else
								{
									x1 = x;
									y1 = y;
									compressed_list = false;
								}
								
								int i, j;
								for (i = x; i <= x1; i++)
									for (j = y; j <= y1; j++)
									{
										tree->getCurrent()->getMatrix()->insertMark(i, j, markType);
										//else	//fastload
										//	tree->getCurrent()->insertFastLoadMark(i, j, markType);

										// auto increment for old property 'L'
										if (old_label)
										{
											tree->getCurrent()->getMatrix()->
												setMarkText(x, y, moveStr);
											QChar c1 = moveStr[0];
											if (c1 == 'Z')
												moveStr = QString("a");
											else
												moveStr = c1.unicode() + 1;
										}
									}

//								new_node = false;

								if (compressed_list)
									// Advance pos by 3
									pos += 3;

								if((markType == markTerrWhite || markType == markTerrBlack) && !tree->getCurrent()->isTerritoryMarked())
									tree->getCurrent()->setTerritoryMarked();
							}

							//old_label = false;
							pos ++;
							while (toParse->at(pos).isSpace()) pos++;
						}
						break;

					case openMoves:
					{
						QString tmp_mv;
						while (toParse->at(++pos) != ']')
							tmp_mv += toParse->at(pos);
						tree->getCurrent()->setOpenMoves(tmp_mv.toInt());
						pos++;

						if (!tree->getCurrent()->getTimeinfo())
						{
							tree->getCurrent()->setTimeinfo(true);
							tree->getCurrent()->setTimeLeft(0);
						}
						break;
					}

					case timeLeft:
					{
						QString tmp_mv;
						while (toParse->at(++pos) != ']')
							tmp_mv += toParse->at(pos);
						tree->getCurrent()->setTimeLeft(tmp_mv.toFloat());
						pos++;

						if (!tree->getCurrent()->getTimeinfo())
						{
							tree->getCurrent()->setTimeinfo(true);
							tree->getCurrent()->setOpenMoves(0);
						}
						break;
					}

					case nextMove:
						if (toParse->at(++pos) == 'W')
							tree->getCurrent()->setPLinfo(stoneWhite);
						else if (toParse->at(pos) == 'B')
							tree->getCurrent()->setPLinfo(stoneBlack);

						pos += 2;
						break;

					default:
						break;
				}
		
				while (toParse->at(pos).isSpace())
			    		pos++;
        	
				sss= toParse->at(pos);

			} while (setup && toParse->at(pos) == '[');
			
//			tree->getCurrent()->getMatrix()->debug();
//			qDebug("end do loop");
//			qDebug(toParse->getStr());
			
			while (toParse->at(pos).isSpace())
				pos++;

		} while (toParse->at(pos) != ';' && toParse->at(pos) != '(' && toParse->at(pos) != ')' &&    static_cast<unsigned int>(pos) < strLength);
		
		// Advance pointer
		pointer = pos;
	
		break;
	
	default:
		delete toParse;
		return corruptSgf(pointer);
	}
	
	} while (pointer < strLength && pos >= 0);

	tree->setLoadingSGF(false);
	
	delete toParse;
	return !cancel;
}

bool SGFParser::corruptSgf(int where, QString reason)
{
	QMessageBox::warning(0, PACKAGE, QObject::tr("Corrupt SGF file at position") + " " +
			     QString::number(where) + "\n\n" +
			     (reason.isNull() || reason.isEmpty() ? QString("") : reason));
	tree->setLoadingSGF(false);
	return false;
}


int SGFParser::minPos(int n1, int n2, int n3)
{
	int min;
	
	if (n1 != -1)
		min = n1;
	else if (n2 != -1)
		min = n2;
	else
		min = n3;
	
	if (n1 < min && n1 != -1)
		min = n1;
	
	if (n2 < min && n2 != -1)
		min = n2;
	
	if (n3 < min && n3 != -1)
		min = n3;
	
	return min;
}

// Return false: corrupt sgf, true: sgf okay. result = 0 when property not found
bool SGFParser::parseProperty(const QString &toParse, const QString &prop, QString &result)
{
	int pos, strLength=toParse.length();
	result = "";
	
	pos = toParse.indexOf(prop+"[");
	if (pos == -1)
		return true;
	
	pos += 2;
	if (toParse[pos] != '[')
		return  corruptSgf(pos);
	while (toParse[++pos] != ']' && pos < strLength)
		result.append(toParse[pos]);
	if (pos > strLength)
		return  corruptSgf(pos);
	
	return true;
}

GameData * SGFParser::initGame(const QString &toParse, const QString &fileName)
{
	QString tmp="";
	GameData *gameData = new GameData;

	if (!parseProperty(toParse, "CA", tmp))		//codec
		return NULL;
	if (!tmp.isEmpty())
		gameData->codec = tmp;
	else
		gameData->codec = QString();
	//probably should either be Latin1 or some default codec from somewhere FIXME
		
	// White player name
	if (!parseProperty(toParse, "PW", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->white_name = tmp;
	else
		gameData->white_name = "White";

	// White player rank
	if (!parseProperty(toParse, "WR", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->white_rank = tmp;
	else
		gameData->white_rank = "";

	// Black player name
	if (!parseProperty(toParse, "PB", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->black_name = tmp;
	else
		gameData->black_name = "Black";
	
	// Black player rank
	if (!parseProperty(toParse, "BR", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->black_rank = tmp;
	else
		gameData->black_rank = "";
	
	// Board size
	if (!parseProperty(toParse, "SZ", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->board_size = tmp.toInt();
	else
		gameData->board_size = 19;
	
	// Komi
	if (!parseProperty(toParse, "KM", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->komi = tmp.toFloat();
	else
		gameData->komi = 0.0;
	
	// Handicap
	if (!parseProperty(toParse, "HA", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->handicap = tmp.toInt();
	else
		gameData->handicap = 0;
	
	// Result
	if (!parseProperty(toParse, "RE", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->result = tmp;
	else
		gameData->result = "";
	
	// Date
	if (!parseProperty(toParse, "DT", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->date = tmp;
	else
		gameData->date = "";
	
	// Place
	if (!parseProperty(toParse, "PC", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->place = tmp;
	else
		gameData->place = "";
	
	// Copyright
	if (!parseProperty(toParse, "CP", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->copyright = tmp;
	else
		gameData->copyright = "";
	
	// Game Name
	if (!parseProperty(toParse, "GN", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->gameName = tmp;
	else
		gameData->gameName = "";

	// Comments style
	if (!parseProperty(toParse, "ST", tmp))
        return NULL;
	if (!tmp.isEmpty())
		gameData->style = tmp.toInt();
	else
		gameData->style = 1;

	// Timelimit
	if (!parseProperty(toParse, "TM", tmp))
        return NULL;
	if (!tmp.isEmpty())
	{
		gameData->timelimit = tmp.toInt();
		gameData->timeSystem = absolute;
	}
	else
	{
		gameData->timelimit = 0;
		gameData->timeSystem = none;
	}

	// Overtime == time system
	if (!parseProperty(toParse, "OT", tmp))
        return NULL;
	if (!tmp.isEmpty())
	{
		gameData->overtime = tmp;
		if (tmp.contains("byo-yomi"))
		{
			// type: OT[5x30 byo-yomi]
			gameData->timeSystem = byoyomi;
			int pos1, pos2;
			if ((pos1 = tmp.indexOf("x")) != -1)
			{
				pos2 = tmp.indexOf("byo");
				QString time = tmp.mid(pos1+1, pos2-pos1-1);
				/*gameData->byoTime = time.toInt();
				gameData->byoPeriods = tmp.left(pos1).toInt();
				gameData->byoStones = 0;*/
				//FIXME okay?
				gameData->periodtime = time.toInt();
				gameData->stones_periods = tmp.left(pos1).toInt();
#ifdef FIXME
				qDebug(QString("byoyomi time system: %1 Periods at %2 seconds").arg(gameData->stones_periods).arg(gameData->periodtime).toLatin1().constData());
#endif //FIXME
			}
		}
		else if (tmp.contains(":"))
		{
			// type: OT[3:30] = byo-yomi?;
			int pos1;
			gameData->timeSystem = byoyomi;
			if ((pos1 = tmp.indexOf(":")) != -1)
			{
				QString time = tmp.left(pos1);
				int t = time.toInt()*60 + tmp.right(tmp.length() - pos1 - 1).toInt();
				gameData->periodtime = 30;
				gameData->stones_periods = t/gameData->periodtime;
				//gameData->byoStones = 0;

////////////////////FIXME	qDebug(QString("byoyomi time system: %1 Periods at %2 seconds").arg(gameData->byoPeriods).arg(gameData->byoTime));
			}
		}
		else if (tmp.contains("Canadian"))
		{
			// type: OT[25/300 Canadian]
			gameData->timeSystem = canadian;
			int pos1, pos2;
			if ((pos1 = tmp.indexOf("/")) != -1)
			{
				pos2 = tmp.indexOf("Can");
				QString time = tmp.mid(pos1+1, pos2-pos1-1);
				gameData->periodtime = time.toInt();
				gameData->stones_periods = tmp.left(pos1).toInt();

///////////////////////				qDebug(QString("Canadian time system: %1 seconds at %2 stones").arg(gameData->byoTime).arg(gameData->byoStones));
			}
		}

		// simple check
		if (gameData->stones_periods < 0)
			gameData->stones_periods = 0;
		if (gameData->periodtime <= 0)
		{
			gameData->periodtime = 0;
//			gameData->timeSystem = none;
		}
	}
	else
	{
		gameData->overtime = "";
//		gameData->timeSystem = none;
		gameData->stones_periods = 0;
	}

	// Game number
	gameData->number = 0;

	gameData->fileName = fileName;
	
//	boardHandler->board->initGame(gameData, true);  // Set sgf flag
	
	return gameData ;
}

/*
 * takes a game tree, and puts it in SGF format
 */
bool SGFParser::exportSGFtoClipB(QString *str, Tree *tree, GameData *gd)
{
	Q_CHECK_PTR(tree);
	
	if (stream != NULL)
		delete stream;
	stream = new QTextStream(str, QIODevice::WriteOnly);
	
	bool res = writeStream(tree,gd);
	
	delete stream;
	stream = NULL;
	return res;
}

/*
 * Opens a file for saving SGF
 */
bool SGFParser::doWrite(const QString &fileName, Tree *tree, GameData *gameData)
{
	Q_CHECK_PTR(tree);
	
	QFile file(fileName);
	
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(0, PACKAGE, QObject::tr("Could not open file:") + " " + fileName);
		return false;
	}
	
	if (stream != NULL)
		delete stream;
	stream = new QTextStream(&file);
	
	bool res = writeStream(tree, gameData );
	
	file.flush();
	file.close();
	delete stream;
	stream = NULL;
	return res;
}

/*
 * Writes the SGF code for whot 'tree' into the 'stream'
 */
bool SGFParser::writeStream(Tree *tree, GameData *gameData)
{
	Q_CHECK_PTR(stream);
	if (!setCodec(gameData->codec))
	{
		QMessageBox::critical(0, PACKAGE, QObject::tr("Invalid text encoding given. Please check preferences!"));
		delete stream;
		return false;
	}
	
	Move *root = tree->getRoot();
	if (root == NULL)
		return false;
	
//	GameData *gameData = boardHandler->getGameData();
	
	// Traverse the tree recursive in pre-order
	isRoot = true;
	traverse(root, gameData);
	
	return true;
}

/*
 * Writes the SGF header data from gameData into the 'stream'
 */
void SGFParser::writeGameHeader(GameData *gameData)
{
	// Assemble data for root node
	*stream << ";GM[1]FF[4]"						// We play Go, we use FF 4
		<< "AP[" << PACKAGE << ":" << VERSION << "]";		// Application name : Version
	if (gameData->style >= 0 && gameData->style <= 4)
		*stream << "ST[" << gameData->style << "]";
	else
		*stream << "ST[1]";						// We show vars of current node
	if(gameData->codec != QString())
		*stream << "CA[" << gameData->codec << "]";
	if (gameData->gameName.isEmpty())					// Skip game name if empty
		*stream << endl;
	else
		*stream << "GN[" << gameData->gameName << "]"		// Game Name
		<< endl;
	*stream << "SZ[" << gameData->board_size << "]"				// Board size
		<< "HA[" << gameData->handicap << "]"			// Handicap
		<< "KM[" << gameData->komi << "]";				// Komi
//		<< endl;
	
	if (gameData->timelimit != 0)
		*stream << "TM[" << gameData->timelimit << "]";		// Timelimit
	
	if (!gameData->overtime.isEmpty())
		*stream << "OT[" << gameData->overtime << "]" << endl;		// Overtime
	
	if (!gameData->white_name.isEmpty())
		*stream << "PW[" << gameData->white_name << "]";  // White name
	
	if (!gameData->white_rank.isEmpty())
		*stream << "WR[" << gameData->white_rank << "]";    // White rank
	
	if (!gameData->black_name.isEmpty())
		*stream << "PB[" << gameData->black_name << "]";  // Black name
	
	if (!gameData->black_rank.isEmpty())
		*stream << "BR[" << gameData->black_rank << "]";    // Black rank
	
	if (!gameData->result.isEmpty())
		*stream << "RE[" << gameData->result << "]";       // Result
	
	if (!gameData->date.isEmpty())
		*stream << "DT[" << gameData->date << "]";         // Date
	
	if (!gameData->place.isEmpty())
		*stream << "PC[" << gameData->place << "]";         // Place
	
	if (!gameData->copyright.isEmpty())
		*stream << "CP[" << gameData->copyright << "]";    // Copyright
	
	*stream << endl;
}

/*
 * traverses the tree from move 't', and stores SGF code into 'stream'
 */
void SGFParser::traverse(Move *t, GameData *gameData)
{
	QString txt;
	*stream << "(";
	int col = -1, cnt = 6;


	do {
		if (isRoot)
		{
			writeGameHeader(gameData);
			txt = t->saveMove(isRoot).toUtf8();		//utf8 might only be necessary if gameData->codec is set, but should be okay either way
			*stream << txt;
			isRoot = false;
		}
		else
		{
			// do some formatting: add single B[]/W[] properties to one line, max: 10
			// if more properties, e.g. B[]PL[]C] -> new line
			txt = t->saveMove(false).toUtf8();
			if(txt == ";")		//don't save empty nodes
				txt = "";
			int cnt_old = cnt;
			cnt = txt.length();
			if (col % 10 == 0 || (col == 1 && cnt != 6) || cnt_old != 6 || col == -1)
			{
				*stream << endl;
				col = 0;
			}
			*stream << txt;
			col++;
		}
	
		Move *tmp = t->son;
		if (tmp != NULL && tmp->brother != NULL)
		{
			do {
				*stream << endl;
				traverse(tmp, NULL);
			} while ((tmp = tmp->brother) != NULL);
			break;
		}
	} while ((t = t->son) != NULL);
	*stream << endl << ")";
}
