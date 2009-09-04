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


#include <QToolBar>

class QAction;
class CyberOroConnection;
class QPushButton;

class SetPhrasePalette : public QToolBar
{
	Q_OBJECT
	public:
		SetPhrasePalette(CyberOroConnection *);
		~SetPhrasePalette();
	public slots:
		void slot_triggered(QAction *);
	private:
		QPushButton * button0;
		QPushButton * button1;
		QPushButton * button2;
		QPushButton * button3;
		QPushButton * button4;
		QPushButton * button5;
		QPushButton * button6;
		QPushButton * button7;
		QPushButton * button8;
		QPushButton * button9;
		std::vector<QAction *> actions;
		CyberOroConnection * connection;
};
