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
#include "matchinvitedialog.h"

MatchInviteDialog::MatchInviteDialog(QString name, QString rank, bool canRefuseFuture)
{
	int horiz_pos = 0;
	seconds = 20;		//does it?
	
	/* FIXME, we need to center this text, make the font larger, or
	 * bold, etc.. */
	namelabel = new QLabel(tr("%1 %2").arg(name).arg(rank));
	dialoglabel = new QLabel(tr("wants to play a match..."));
	timelabel = new QLabel(tr("%1 seconds").arg(seconds));
	
	acceptButton = new QPushButton(tr("&Accept"));
	acceptButton->setDefault(true);
	
	declineButton = new QPushButton(tr("&Decline"));
	if(canRefuseFuture)
		refuseFutureCB = new QCheckBox(tr("Refuse Invites from %1").arg(name));
	else
		refuseFutureCB = 0;
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(acceptButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(declineButton, QDialogButtonBox::ActionRole);
	
	connect(acceptButton, SIGNAL(clicked()), this, SLOT(slot_accept()));
	connect(declineButton, SIGNAL(clicked()), this, SLOT(slot_decline()));
	if(refuseFutureCB)
		connect(refuseFutureCB, SIGNAL(clicked(bool)), this, SLOT(slot_refuseFutureCB(bool)));
	
	QGridLayout * mainLayout = new QGridLayout;
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(namelabel, horiz_pos++, 0);
	mainLayout->addWidget(dialoglabel, horiz_pos++, 0);
	mainLayout->addWidget(timelabel, horiz_pos++, 0);
	if(refuseFutureCB)
		mainLayout->addWidget(refuseFutureCB, horiz_pos++, 0);
	mainLayout->addWidget(buttonBox, horiz_pos++, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Match Invite!"));
	
	startTimer(1000);
}

void MatchInviteDialog::timerEvent(QTimerEvent*)
{
	seconds--;
	if(seconds == -1)
		done(-1);	//does this return 0?
	timelabel->setText(tr("%1 seconds").arg(seconds));
}

MatchInviteDialog::~MatchInviteDialog()
{
	delete acceptButton;
	delete declineButton;
	delete refuseFutureCB;
	delete buttonBox;
}

/* I'm assuming here that a close by time returns 0 */
void MatchInviteDialog::slot_accept(void)
{
	done(1);
}

void MatchInviteDialog::slot_decline(void)
{
	if(refuseFutureCB && refuseFutureCB->isChecked())
		done(-2);
	else
		done(-1);
}

void MatchInviteDialog::slot_refuseFutureCB(bool b)
{
	if(b)
	{
		acceptButton->setEnabled(false);
	}
	else
		acceptButton->setEnabled(true);
}
