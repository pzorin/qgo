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
#include "../network/messages.h"
#include "resultdialog.h"
#include "../network/boarddispatch.h"

ResultDialog::ResultDialog(QWidget * parent, BoardDispatch * dis, unsigned int game_id, GameResult * gr) : QDialog(parent), dispatch(dis)
{
	if(!dispatch)
	{
		qDebug("ResultDialog called without board dispatch");
		deleteLater();
		return;
	}
	if(!gr)
	{
		// This was created by the board dispatch in response
		// to a rematch request
		
		mainlabel = new QLabel(dispatch->getOpponentName() + tr(" requests rematch"));
		seconds = 20;
		okayButton = new QPushButton(tr("&Accept?"));
		accepting = true;
	}
	else
	{
		accepting = false;
		if(dispatch->supportsRematch())
		{
			seconds = 20;
			/* We make them stare at message for 20 seconds */
			okayButton = new QPushButton(tr("&Rematch?"));
			dispatch->setRematchDialog(this);
		}
		else
		{
			seconds = 0;
			okayButton = new QPushButton(tr("&Okay"));
			okayButton->setDefault(true);
		}
		mainlabel = new QLabel(gr->longMessage());
	}
	connect(okayButton, SIGNAL(clicked()), this, SLOT(slot_okay()));
	
	QGridLayout * mainLayout = new QGridLayout;
	/*mainLayout->setSizeConstraint(QLayout::SetFixedSize);*/
	mainLayout->addWidget(mainlabel, 0, 0);
	mainLayout->addWidget(okayButton, 1, 0);
	setLayout(mainLayout);

	
	setWindowTitle(tr("Game ") + QString::number(game_id));
	//setSizePolicy(QSizePolicy::Minimum);
	// FIXME size?
	resize(200, 100);
	//setMinimumSize(QSize(300, 150));
	if(dispatch->supportsRematch())
		startTimer(1000);
}

void ResultDialog::recvRematchRequest(void)
{
	mainlabel->setText(dispatch->getOpponentName() + tr(" requests rematch"));
	seconds = 20;
	okayButton->setText(tr("&Accept?"));
	accepting = true;
}

void ResultDialog::timerEvent(QTimerEvent*)
{
	// in case we need this?
	if(seconds == 1)
	{
		//what about sending a declineRematch?? if(accepting)?? 
		hide();
		deleteLater();
	}
	else
		seconds--;
}

void ResultDialog::closeEvent(QCloseEvent *)
{
	/*if(dispatch)
		dispatch->setRematchDialog(0);*/
}

ResultDialog::~ResultDialog()
{
	if(dispatch)		//here also in case no "closeEvent"
		dispatch->setRematchDialog(0);
	qDebug("deleting result dialog");
	delete okayButton;
	delete mainlabel;
}

/* I'm assuming here that a close by time returns 0 */
void ResultDialog::slot_okay(void)
{
	if(dispatch->supportsRematch())
	{
		if(accepting)
			dispatch->sendRematchAccept();
		else
			dispatch->sendRematchRequest();
		dispatch->setRematchDialog(0);
		qDebug("rematch slot okay");
		dispatch = 0;
	}
	hide();
	deleteLater();
	// or should this be just done()
}
