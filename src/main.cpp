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

#include "mainwindow.h"
#include "defines.h"


struct _preferences preferences;
MainWindow * mainwindow = 0;
ConnectionWidget * connectionWidget = 0;

QTranslator * translatorPtr;

void installTranslator(enum Language l)
{
    switch(l)
    {
        case German:
            translatorPtr->load("qgo_de",TRANSLATIONS_PATH);
            return;
        case French:
            translatorPtr->load("qgo_fr",TRANSLATIONS_PATH);
            break;
        case Italian:
            translatorPtr->load("qgo_it",TRANSLATIONS_PATH);
            break;
        case Danish:
            translatorPtr->load("qgo_dk",TRANSLATIONS_PATH);
            break;
        case Dutch:
            translatorPtr->load("qgo_nl",TRANSLATIONS_PATH);
            break;
        case Czech:
            translatorPtr->load("qgo_cz",TRANSLATIONS_PATH);
            break;
        case Chinese:
            translatorPtr->load("qgo_zh",TRANSLATIONS_PATH);
            break;
        case Portugese:
            translatorPtr->load("qgo_pt",TRANSLATIONS_PATH);
            break;
        case Polish:
            translatorPtr->load("qgo_it",TRANSLATIONS_PATH);
            break;
        case Russian:
            translatorPtr->load("qgo_ru",TRANSLATIONS_PATH);
            break;
        case Turkish:
            translatorPtr->load("qgo_tr",TRANSLATIONS_PATH);
            break;
        case None:
        default:
            QString locale = QLocale::system().name();
                translatorPtr->load(QString("qgo_") + locale,TRANSLATIONS_PATH);
            return;
    }
    QCoreApplication::instance()->installTranslator(translatorPtr);
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

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(application);
    QApplication * app = new QApplication(argc, argv);
	QTranslator translator;

    app->setOrganizationName("qGo");
    app->setApplicationName("qGo");

    QCommandLineParser parser;
    parser.process(*app);
    const QStringList args = parser.positionalArguments();
    translatorPtr = &translator;
	
	startqGo();
    mainwindow->show();

    QStringList::const_iterator filename;
    for ( filename = args.begin(); filename != args.end(); ++filename )
	{
        mainwindow->openSGF(*filename);
    }

	srand(time(NULL));

    return app->exec();
}
