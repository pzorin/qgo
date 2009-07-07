/***************************************************************************
 *   Copyright (C) 2006 by Emmanuel Béranger   *
 *   yfh2@hotmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <QApplication>
#include <QtGui>

#include "mainwindow.h"
#include "defines.h"

struct _preferences preferences;
MainWindow * mainwindow = 0;
QApplication * appPtr;
QTranslator * translatorPtr;
void installTranslator(enum Language);

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(application);
	QApplication app(argc, argv);
	QTranslator translator;
	QString *sgf_file = NULL;

	QCoreApplication::setOrganizationName("qgo2");
	
	appPtr = &app;
	translatorPtr = &translator;
	QSettings settings;
	installTranslator((enum Language)settings.value("LANGUAGE").toInt());		//temporary place for this
	
	mainwindow = new MainWindow(0,0);

	if ( argc > 1 )
	{
		sgf_file = new QString( argv[1] );
	}

	if ( sgf_file )
	{
		mainwindow->loadSgfFile( *sgf_file );
		delete sgf_file;
		//mainwindow->hide();
	}
	else
	{
		mainwindow->show();
	}

	return app.exec();
}

void installTranslator(enum Language l)
{
	//QSettings settings;
	
	switch(l)
	{
		case German:
			return;
		case French:
			translatorPtr->load("qgo2_fr");			//path?!?
			break;
		case Turkish:
			translatorPtr->load("qgo2_tr");			//path?!?
			break;
		case None:
		default:
			return;
	}
	appPtr->installTranslator(translatorPtr);
}
