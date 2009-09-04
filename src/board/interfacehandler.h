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


#ifndef INTERFACEHANDLER_H
#define INTERFACEHANDLER_H

#include "defines.h"

class BoardWindow;
class GameData;

class InterfaceHandler
{
public:
	InterfaceHandler( BoardWindow * bw);
	~InterfaceHandler();
	void clearData();
	void toggleMode(GameMode gameMode);
//	void setEditMode();
//	void setMarkType(int m);
	void setMoveData(int n, bool black, int brothers, int sons, bool hasParent,
		bool hasPrev, bool hasNext, int lastX=-1, int lastY=-1);
	void updateCaption(GameData *gd);
	void displayComment(const QString &c);
	void setSliderMax(int n);
	void setCaptures(float black, float white);
	void clearComment();
//	void toggleToolbarButtons(bool state);
	void setScore(int terrB, int capB, int terrW, int capW, float komi=0);
/*	QString getComment();
	QString getComment2();
	void toggleSidebar(bool toggle);
	QString getTextLabelInput(QWidget *parent, const QString &oldText);
	void showEditGroup();
	void toggleMarks();
	const QString getStatusMarkText(MarkType t);
	void restoreToolbarButtons();

	void setClipboard(bool b);
	

*/
	BoardWindow *boardwindow;
	bool scored_flag;

private :
	QWidget *tabPlay, *tabEdit;

};

#endif
