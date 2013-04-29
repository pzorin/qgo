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


#include <QtWidgets>
#include "createroomdialog.h"
#include "networkconnection.h"
 
CreateRoomDialog::CreateRoomDialog(NetworkConnection * conn) : connection(conn)
{
    setupUi(this);
    roomTypeTab->removeTab(5);			//specialty versions
    roomTypeTab->setTabEnabled(3, false);	//review
    roomTypeTab->setTabEnabled(4, false);	//multi
	
	/*namelabel = new QLabel(tr("%1 %2").arg(name).arg(rank));
	dialoglabel = new QLabel(tr("wants to play a match..."));
	timelabel = new QLabel(tr("%1 seconds").arg(seconds));
	
	createButton = new QPushButton(tr("&Create"));
	createButton->setDefault(true);
	
	cancelButton = new QPushButton(tr("&Cancel"));
	
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(createButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(cancelButton, QDialogButtonBox::ActionRole);
	
	connect(createButton, SIGNAL(clicked()), this, SLOT(slot_create()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancel()));
	
	QGridLayout * mainLayout = new QGridLayout;
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(namelabel, 0, 0);
	mainLayout->addWidget(dialoglabel, 1, 0);
	mainLayout->addWidget(timelabel, 2, 0);
	mainLayout->addWidget(buttonBox, 3, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Create Room"));*/
	
    connect(createButton, SIGNAL(clicked()), this, SLOT(slot_create()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancel()));
	
    connect(privateCB, SIGNAL(clicked(bool)), this, SLOT(slot_privateCB(bool)));
	
    connect(roomTypeTab, SIGNAL(currentChanged(int)), SLOT(slot_roomTypeTab(int)));
	
    connect(opponentStrongerRB, SIGNAL(clicked()), this, SLOT(slot_opponentStrongerRB()));
    connect(opponentEvenRB, SIGNAL(clicked()), this, SLOT(slot_opponentEvenRB()));
    connect(opponentWeakerRB, SIGNAL(clicked()), this, SLOT(slot_opponentWeakerRB()));
    connect(opponentAnyRB, SIGNAL(clicked()), this, SLOT(slot_opponentAnyRB()));
	
    connect(timeQuickRB, SIGNAL(clicked()), this, SLOT(slot_timeQuickRB()));
    connect(timeNormalRB, SIGNAL(clicked()), this, SLOT(slot_timeNormalRB()));
    connect(timePonderousRB, SIGNAL(clicked()), this, SLOT(slot_timePonderousRB()));
    connect(timeAnyRB, SIGNAL(clicked()), this, SLOT(slot_timeAnyRB()));
	
    connect(oneOnOneRB, SIGNAL(clicked()), this, SLOT(slot_oneOnOneRB()));
    connect(pairRB, SIGNAL(clicked()), this, SLOT(slot_pairRB()));
    pairRB->setEnabled(false);
	
	
    connect(teachingRB, SIGNAL(clicked()), this, SLOT(slot_teachingRB()));
    connect(liveRB, SIGNAL(clicked()), this, SLOT(slot_liveRB()));
}

void CreateRoomDialog::slot_privateCB(bool checked)
{
	checked = false; //for the warning
    passwordLabel->setEnabled(checked);
    passwordEdit->setEnabled(checked);
	if(!checked)
        passwordEdit->clear();
}
		
void CreateRoomDialog::slot_roomTypeTab(void)
{
}
void CreateRoomDialog::slot_opponentStrongerRB(void)
{
}
void CreateRoomDialog::slot_opponentEvenRB(void)
{
}
void CreateRoomDialog::slot_opponentWeakerRB(void)
{
}
void CreateRoomDialog::slot_opponentAnyRB(void)
{
}

void CreateRoomDialog::slot_timeQuickRB(void)
{
}
void CreateRoomDialog::slot_timeNormalRB(void)
{
}
void CreateRoomDialog::slot_timePonderousRB(void)
{
}
void CreateRoomDialog::slot_timeAnyRB(void)
{
}
void CreateRoomDialog::slot_oneOnOneRB(void)
{
}
void CreateRoomDialog::slot_pairRB(void)
{
}
void CreateRoomDialog::slot_teachingRB(void)
{
}
void CreateRoomDialog::slot_liveRB(void)
{
}

CreateRoomDialog::~CreateRoomDialog()
{
	
}

void CreateRoomDialog::slot_create(void)
{
	RoomCreate * room = new RoomCreate();
	/* FIXME This could be problematic if the index
	 * can change when we remove tabs, but that's
	 * a ways away right now.*/
    room->type = (RoomCreate::roomType)roomTypeTab->currentIndex();
    switch(roomTypeTab->currentIndex())
	{
		case RoomCreate::GAME:
			break;
		case RoomCreate::GOMOKU:
			break;
		case RoomCreate::CHAT:
			break;
		case RoomCreate::REVIEW:
			done(0);
			break;
		case RoomCreate::MULTI:
			done(0);
			break;
		case RoomCreate::VARIATION:
			done(0);
			break;
	}
	connection->sendCreateRoom(room);
	done(1);
}

void CreateRoomDialog::slot_cancel(void)
{
	done(0);
}
