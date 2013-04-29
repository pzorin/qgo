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
#include "ui_createroomdialog.h"

class QLabel;
class QDialogButtonBox;
class NetworkConnection;

class CreateRoomDialog : public QDialog, public Ui::CreateRoomDialog
{
	Q_OBJECT
	public:
		CreateRoomDialog(NetworkConnection * conn);
		~CreateRoomDialog();
	public slots:
		void slot_create(void);
		void slot_cancel(void);
		
		void slot_privateCB(bool);
		
		void slot_roomTypeTab(void);
		void slot_opponentStrongerRB(void);
		void slot_opponentEvenRB(void);
		void slot_opponentWeakerRB(void);
		void slot_opponentAnyRB(void);

		void slot_timeQuickRB(void);
		void slot_timeNormalRB(void);
		void slot_timePonderousRB(void);
		void slot_timeAnyRB(void);
		void slot_oneOnOneRB(void);
		void slot_pairRB(void);
		void slot_teachingRB(void);
		void slot_liveRB(void);
	private:
		NetworkConnection * connection;
		//Ui::CreateRoomDialog ui;
};

class RoomCreate
{
public:
	unsigned char * title;
	unsigned char * password;
	enum roomType { GAME = 0, GOMOKU, CHAT, REVIEW, MULTI, VARIATION } type;	
	enum OpponentStrength { STRONGER = 0, EVEN, WEAKER, ANYBODY } opponentStrength;
	enum TimeLimit { QUICK = 0, NORMAL, PONDEROUS, ANYTIME } timeLimit;
	enum Topic { BADUK = 0, MOVIES, MUSIC, SPORTS, GAMES, MANGA, HUMOR, CURRENTAFFAIRS,
			LITERATURE, HOBBIES, TRAVEL, CLIMBING, ENTERTAINMENT, COMPUTING, INTERNET, MISC } topic;
	enum Age { ALL=0, _9, _10_19, _20_29, _30_39, _40_49, _50_59, _60_69, _70 } age;
	enum Location { KOREA = 0, CHINA, JAPAN, THAI, US, EUROPE, AUSTRALIA, FOREIGN, OTHER } location;
};
