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

	QCoreApplication::setOrganizationName("qGo");
	QCoreApplication::setApplicationName("qGo");

	appPtr = &app;
	translatorPtr = &translator;
	
	startqGo();

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

	srand(time(NULL));

	return app.exec();
}

void installTranslator(enum Language l)
{
	switch(l)
	{
		case German:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_de");	
			return;
		case French:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_fr");
			break;
		case Italian:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_it");	
			break;
		case Danish:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_dk");	
			break;
		case Dutch:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_nl");	
			break;
		case Czech:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_cz");	
			break;
		case Chinese:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_zh");	
			break;
		case Portugese:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_pt");	
			break;
		case Polish:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_it");	
			break;
		case Russian:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_ru");	
			break;
		case Turkish:
			translatorPtr->load(TRANSLATIONS_PATH_PREFIX"qgo_tr");
			break;
		case None:
		default:
			QString locale = QLocale::system().name();
     			translatorPtr->load(QString("qgo_") + locale);
			return;
	}
	appPtr->installTranslator(translatorPtr);
}

void startqGo(void)
{
	QSettings settings;
	bool restarting = false;

	installTranslator((enum Language)settings.value("LANGUAGE").toInt());		//temporary place for this
	if(mainwindow)
	{
		restarting = true;
		mainwindow->deleteLater();
	}
	mainwindow = new MainWindow(0,0);
	if(restarting)
		mainwindow->show();

	QVariant main_window_size_x = settings.value("MAIN_WINDOW_SIZE_X");
	if(main_window_size_x != QVariant())
	{
		mainwindow->resize(main_window_size_x.toInt(), settings.value("MAIN_WINDOW_SIZE_Y").toInt());
		mainwindow->move(settings.value("MAIN_WINDOW_POS_X").toInt(), settings.value("MAIN_WINDOW_POS_Y").toInt());
	}
}
