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
class GameResult;
class BoardDispatch;

class ResultDialog : public QDialog
{
	Q_OBJECT
	public:
		ResultDialog(QWidget * parent, BoardDispatch * dis, unsigned int game_id, GameResult * gr);
		~ResultDialog();
		void recvRematchRequest(void);
		virtual void timerEvent(QTimerEvent*);
		//virtual QSize minimumSize(void) const { return QSize(300, 150); };
	public slots:
		void slot_okay(void);
	private:
		void closeEvent(QCloseEvent *);

		QLabel * mainlabel;
		QPushButton * okayButton;
		BoardDispatch * dispatch;
		int seconds;
		bool accepting;
};
