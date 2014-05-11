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


#include "gameinfo.h"
#include "boardwindow.h"
#include <QPushButton>

GameInfo::GameInfo(BoardWindow * bw) : GameinfoDialog(), boardwindow(bw)
{
	ui.setupUi(this);

	ui.whiteName->setText( boardwindow->getGameData()->white_name );
	ui.blackName->setText( boardwindow->getGameData()->black_name );
	ui.whiteRank->setText( boardwindow->getGameData()->white_rank );
	ui.blackRank->setText( boardwindow->getGameData()->black_rank );
	ui.komi->setText( QString::number(boardwindow->getGameData()->komi ));
	ui.handicap->setText( QString::number(boardwindow->getGameData()->handicap ));
	ui.result->setText( boardwindow->getGameData()->result );
	ui.gameName->setText( boardwindow->getGameData()->gameName );
	ui.date->setText( boardwindow->getGameData()->date );
	ui.playedAt->setText( boardwindow->getGameData()->place );
	ui.copyright->setText( boardwindow->getGameData()->copyright );
	
    if(boardwindow->getGameData()->gameMode != modeEdit)
	{
		ui.whiteName->setReadOnly( 1 );
		ui.blackName->setReadOnly( 1 );
		ui.whiteRank->setReadOnly( 1 );
		ui.blackRank->setReadOnly( 1 );
		ui.komi->setReadOnly(1);
		ui.handicap->setReadOnly(1);
		ui.result->setReadOnly(1);
		ui.gameName->setReadOnly(1);
		ui.date->setReadOnly(1);
		ui.playedAt->setReadOnly(1);
		ui.copyright->setReadOnly(1);
	}
	else
	{
		connect(ui.whiteName, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.blackName, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.whiteRank, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.blackRank, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.komi, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.handicap, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.result, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.gameName, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.date, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.playedAt, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
		connect(ui.copyright, SIGNAL(textEdited(const QString &)), this, SLOT(slotGameInfoTextEdited(const QString &)));
	}
	
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(slotGameInfoButton(QAbstractButton *)));
	show();
}

GameInfo::~GameInfo()
{
}

void GameInfo::slotGameInfoButton(QAbstractButton * button)
{
	if(button->text() == "Save")
	{
		boardwindow->getGameData()->white_name = ui.whiteName->text();
		boardwindow->getGameData()->black_name = ui.blackName->text();
		boardwindow->getGameData()->white_rank = ui.whiteRank->text();
		boardwindow->getGameData()->black_rank = ui.blackRank->text();
		sscanf(ui.komi->text().toLatin1().constData(), "%f", &(boardwindow->getGameData()->komi));
		sscanf(ui.handicap->text().toLatin1().constData(), "%d", &(boardwindow->getGameData()->handicap));
		boardwindow->getGameData()->result = ui.result->text();
		boardwindow->getGameData()->gameName = ui.gameName->text();
		boardwindow->getGameData()->date = ui.date->text();
		boardwindow->getGameData()->place = ui.playedAt->text();
		boardwindow->getGameData()->copyright = ui.copyright->text();
		boardwindow->gameDataChanged();
	}
	this->deleteLater();
}

void GameInfo::slotGameInfoTextEdited(const QString &)
{
	bool changed = false;

	if(ui.whiteName->text() != boardwindow->getGameData()->white_name)
		changed = true;
	else if(ui.blackName->text() != boardwindow->getGameData()->black_name)
		changed = true;
	else if(ui.whiteRank->text() != boardwindow->getGameData()->white_rank)
		changed = true;
	else if(ui.blackRank->text() != boardwindow->getGameData()->black_rank)
		changed = true;
	else if(QString::number(boardwindow->getGameData()->komi) != ui.result->text())
		changed = true;
	else if(QString::number(boardwindow->getGameData()->handicap) != ui.result->text())
		changed = true;
	else if(boardwindow->getGameData()->result != ui.result->text())
		changed = true;
	else if(boardwindow->getGameData()->gameName != ui.gameName->text())
		changed = true;
	else if(boardwindow->getGameData()->date != ui.date->text())
		changed = true;
	else if(boardwindow->getGameData()->place != ui.playedAt->text())
		changed = true;
	else if(boardwindow->getGameData()->copyright != ui.copyright->text())
		changed = true;

	QPushButton * button = 0;
	if(changed)
	{
		button = ui.buttonBox->button(QDialogButtonBox::Ok);
		if(button)
		{
			ui.buttonBox->removeButton(button);
			ui.buttonBox->addButton(QDialogButtonBox::Save);
		}
	}	
	else
	{
		button = ui.buttonBox->button(QDialogButtonBox::Save);
		if(button)
		{
			ui.buttonBox->removeButton(button);
			ui.buttonBox->addButton(QDialogButtonBox::Ok);
		}
	}
}
