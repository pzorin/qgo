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


#include <QDialog>

class QLabel;
class QDialogButtonBox;
class GameResult;
class BoardDispatch;
class BoardWindow;

class CountDialog : public QDialog
{
	Q_OBJECT
	public:
		CountDialog(BoardWindow * parent, BoardDispatch * dis, unsigned int game_id);
		~CountDialog();
		//virtual QSize minimumSize(void) const { return QSize(300, 150); };
		void recvRejectCount(void);
		void recvAcceptCount(void);
	public slots:
		void slot_accept(void);
		void slot_reject(void);
	private:
		QLabel * mainlabel;
		QPushButton * acceptButton;
		QPushButton * rejectButton;
		QDialogButtonBox * buttonBox;
		BoardDispatch * dispatch;
		BoardWindow * board;
		GameResult * result;
		bool oppAcceptsCount;
		bool oppRejectsCount;
		bool weAcceptCount;
		bool weRejectCount;
};
