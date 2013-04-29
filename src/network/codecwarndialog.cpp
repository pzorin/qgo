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
#include "codecwarndialog.h"
#include "../defines.h"

CodecWarnDialog::CodecWarnDialog(const char * encoding)
{
	if(!preferences.warn_about_codecs)
	{
		deleteLater();
		done(0);
	}
	/* FIXME okayButton should be on lower right and window should be slightly
	 * larger, more warning like, maybe with padding around text, some kind of
	 * layout. */
	okayButton = new QPushButton(tr("Okay"));
	okayButton->setMaximumWidth(80);
	okayButton->setDefault(true);
	
	dontwarnCB = new QCheckBox(tr("Don't warn me again"));
	
	textLabel = new QLabel(tr("Can't find font codec \"%1\"\nUsing default").arg(encoding));
	
	connect(okayButton, SIGNAL(clicked()), this, SLOT(slot_okay()));
	
	QGridLayout * mainLayout = new QGridLayout;
	//mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(textLabel, 0, 0);
	mainLayout->addWidget(dontwarnCB, 1, 0);
	mainLayout->addWidget(okayButton, 2, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Missing Codec!"));
	show();
}

CodecWarnDialog::~CodecWarnDialog()
{
	if(dontwarnCB->isChecked())
		preferences.warn_about_codecs = 0;
	delete okayButton;
	delete dontwarnCB;
}

void CodecWarnDialog::slot_okay(void)
{
	/* Why does this need to be here as well as above? FIXME,
	 * why doesn't it suffice solely in deconstructor */
	if(dontwarnCB->isChecked())
		preferences.warn_about_codecs = 0;
	deleteLater();
	done(0);
}
