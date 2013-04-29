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
#include "undoprompt.h"

UndoPrompt::UndoPrompt(const QString * _name, bool multiple, int _moves) : name(_name), moves(_moves)
{
	movesSpin = 0;
	
	if(name)
	{
		if(!multiple)
			mainlabel = new QLabel(tr("%1 wants to undo the last move").arg(*name));
		else
			mainlabel = new QLabel(tr("%1 wants to undo to move %2").arg(*name).arg(moves));
	}	
	else
	{
		if(!multiple)
			mainlabel = new QLabel(tr("Undo the last move?").arg(*name));
		else
		{
			mainlabel = new QLabel(tr("Undo to what move?"));
			movesSpin = new QSpinBox();
			movesSpin->setValue(moves);	// really shouldn't this be moves -1?
			movesSpin->setRange(0, moves);
		}
	}
	
	if(!movesSpin)
	{
		acceptButton = new QPushButton(tr("&Accept"));
		acceptButton->setDefault(true);
	
		declineButton = new QPushButton(tr("&Decline"));
	}
	else
	{
		acceptButton = new QPushButton(tr("&Request"));
		acceptButton->setDefault(true);
	
		declineButton = new QPushButton(tr("&Cancel"));
	}
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(acceptButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(declineButton, QDialogButtonBox::ActionRole);
	
	connect(acceptButton, SIGNAL(clicked()), this, SLOT(slot_accept()));
	connect(declineButton, SIGNAL(clicked()), this, SLOT(slot_decline()));
	
	QGridLayout * mainLayout = new QGridLayout;
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(mainlabel, 0, 0);
	if(movesSpin)
		mainLayout->addWidget(movesSpin, 1, 0);
	mainLayout->addWidget(buttonBox, (movesSpin ? 2 : 1), 0);
	setLayout(mainLayout);
	
	if(name)
		setWindowTitle(tr("Undo requested"));
	else
		setWindowTitle(tr("Request undo?"));
	
	
	// we might want to set modality here
	//startTimer(1000);
}

void UndoPrompt::timerEvent(QTimerEvent*)
{
	// in case we need this?
}

UndoPrompt::~UndoPrompt()
{
	delete acceptButton;
	delete declineButton;
	delete mainlabel;
	delete movesSpin;
	delete buttonBox;
}

/* I'm assuming here that a close by time returns 0 */
void UndoPrompt::slot_accept(void)
{
	if(!movesSpin)
		done(moves);
	else
		done(movesSpin->value());
}

void UndoPrompt::slot_decline(void)
{
	done(-1);
}
