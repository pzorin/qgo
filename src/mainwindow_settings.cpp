/***************************************************************************
 *   Copyright (C) 2006 by EB   *
 *      *
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

#include "mainwindow_settings.h"
#include "mainwindow.h"

/*
 *   Host - Class to save Host info
 */

/* FIXME, there's a cleaner way to define such a simple data structure */
Host::Host(const QString &host, const QString &login, const QString &pass)
{
	h = host;
	lg = login;
	pw = pass;
}

/*
 *   List to help keeping things sorted
 */

int HostList::compareItems(Host* d1, Host* d2)
{
	Host *s1 = static_cast<Host*>(d1);
	Host *s2 = static_cast<Host*>(d2);

	Q_CHECK_PTR(s1);
	Q_CHECK_PTR(s2);

	if (s1 > s2)
		return 1;
	else if (s1 < s2)
		return -1;
	else
		// s1 == s2;
		return 0;
}

#ifdef FIXME
/*
 *   account & caption
 */

Account::Account(QWidget* parent)
{
	// init
	this->parent = parent;
	standard = PACKAGE + QString("V") + VERSION;

	set_offline();
	//games.clear();	//unnecessary
}

Account::~Account()
{

}

// set caption
void Account::set_caption()
{
	if ((gsName == GS_UNKNOWN) ||  acc_name.isEmpty() ||  acc_name.isNull())
	{
		// server unknown or no account name
		// -> standard caption
		parent->setWindowTitle(standard);
	}
	else
	{
		if (status == GUEST)
			parent->setWindowTitle(svname + " - " + acc_name + " (guest)");
		else
			parent->setWindowTitle(svname + " - " + acc_name);
	}
}

// set go server name
void Account::set_gsname(GSName gs)
{
	gsName = gs;

	// now we know which server
	switch (gsName)
	{
		case IGS: svname.sprintf("IGS");
			break;

		case NNGS: svname.sprintf("NNGS");
			break;

		case LGS: svname.sprintf("LGS");
			break;

		case WING: svname.sprintf("WING");
			break;

		case CTN: svname.sprintf("CTN");
			break;

		case CWS: svname.sprintf("CWS");
			break;

		default: svname.sprintf("unknown Server");
			break;
	}

	// set account name
	if ( acc_name.isEmpty() ||  acc_name.isNull())
	{
		// acc_name should be set...
		acc_name.sprintf("Lulu");
		qWarning("set_gsname() - acc_name not found!");
	}
	
	if (status == OFFLINE)
		status = (enum Status) REGISTERED;
}

// set account name
void Account::set_accname(QString &name)
{
	acc_name = name;
}

// set status
void Account::set_status(Status s)
{
	status = s;
}

// set to offline mode
void Account::set_offline()
{
	gsName = GS_UNKNOWN;
	svname = (QString) NULL;
	acc_name = (QString) NULL;
	status = OFFLINE;

	set_caption();

	num_players = 0;
	num_watchedplayers = 0;
	num_observedgames = 0;
	num_games = 0;
}

// get some variables
Status Account::get_status()
{
	return status;
}

GSName Account::get_gsname()
{
	return gsName;
}

Game * Account::getGame(int game_number)
{
	std::map<int,Game *>::iterator iter = games.find(game_number);
	if(iter != games.end())
		return games[game_number];
	else
		return NULL;
}

void Account::removeGame(int game_number)
{
	Game * g;
	if(games.find(game_number) != games.end())
	{
		qDebug("game_number %d found", game_number);
		g = games[game_number];
		delete g;
		games.erase(game_number);
	}
	else
		qDebug("game_number %d not found", game_number);
}

void Account::addGame(int game_number, Game * game)
{
	qDebug("Adding game_number %d: %p", game_number, game);
	games[game_number] = game;
}


void PlayerTableItem::set_nmatchSettings(Player *p)
{
	nmatch = p->nmatch;

	nmatch_black = 		p->nmatch_black ;
	nmatch_white = 		p->nmatch_white;
	nmatch_nigiri = 	p->nmatch_nigiri ;
	nmatch_handicapMin = 	p->nmatch_handicapMin;
	nmatch_handicapMax  = 	p->nmatch_handicapMax;
	nmatch_timeMin = 	p->nmatch_timeMin;
	nmatch_timeMax  = 	p->nmatch_timeMax;
	nmatch_BYMin = 		p->nmatch_BYMin;
	nmatch_BYMax = 		p->nmatch_BYMax;
	nmatch_stonesMin = 	p->nmatch_stonesMin;
	nmatch_stonesMax = 	p->nmatch_stonesMax;
	nmatch_KoryoMin = 	p->nmatch_KoryoMin;
	nmatch_KoryoMax = 	p->nmatch_KoryoMax ;

	nmatch_settings =  !(p->nmatch_settings == "No match conditions");

}
#endif //FIXME

/*
 * a cancel button has been pressed on the preference pages  
 */
void MainWindow::slot_cancelPressed()
{
	loadSettings();
}

/*
 * a page has been left. If it's a preference or server stting page, we check the settings
 */
void MainWindow::slot_currentChanged(int i)
{
	static int former=-1;
	QSettings settings;
	bool resend = FALSE;

	if ((former == 3) || (former == 4))
	{
		//Checks wether the nmatch parameters have been modified, in order to send a new nmatchrange command
		resend=((settings.value("NMATCH_BLACK").toBool() != ui.checkBox_Nmatch_Black->isChecked()) || 
			(settings.value("NMATCH_WHITE").toBool() != ui.checkBox_Nmatch_White->isChecked()) ||
			(settings.value("NMATCH_NIGIRI").toBool() != ui.checkBox_Nmatch_Nigiri->isChecked()) ||
			(settings.value("NMATCH_MAIN_TIME").toInt() != ui.timeSpin_Nmatch->value()) ||
			(settings.value("NMATCH_BYO_TIME").toInt() != ui.BYSpin_Nmatch->value()) ||
			(settings.value("NMATCH_HANDICAP").toInt() != ui.HandicapSpin_Nmatch->value()) ||
			(settings.value("DEFAULT_SIZE").toInt() != ui.boardSizeSpin->value()) ||
			(settings.value("DEFAULT_TIME").toInt() != ui.timeSpin->value()) ||
			(settings.value("DEFAULT_BY").toInt() != ui.BYSpin->value()) );

		saveSettings();
#ifdef FIXME
		if (resend)
			sendNmatchParameters();
#endif //FIXME
	}
	if(i == 1 || i == 2)
	{
		//refresh file system model
		/* Apparently unnecessary and causes large slowdown in windows 
		 * EXCEPT, it might be nice to be able to refresh the file
		 * view somehow.  Need to figure out why its slow in windows.  FIXME*/
		//model->refresh();
	}

	former = i;
	
}




/*
 * saves the parameters on the 2 lats tabs into the QSettings 
 */
void MainWindow::saveSettings()
{
	QSettings settings;

	settings.setValue("LANGUAGE",ui.comboBox_language->currentIndex ());
//	settings.setValue("COMPUTER_PATH", ui.LineEdit_computer->text());
	settings.setValue("COMPUTER_PLAYS_WHITE", ui.computerPlaysWhite->isChecked());
	settings.setValue("COMPUTER_HANDICAP", ui.newComputer_Handicap->text().toInt());
	settings.setValue("COMPUTER_KOMI", ui.newComputer_Komi->text().toInt());
	settings.setValue("SKIN", ui.LineEdit_goban->text()); 
	settings.setValue("SKIN_TABLE", ui.LineEdit_table->text()); 

	settings.setValue("TIMER_INTERVAL", ui.timerComboBox->currentIndex());

	int i = 0;
	if ( ui.radioButtonStones_2D->isChecked())
		i=1;
	else if ( ui.radioButtonStones_3D->isChecked())
		i=2;
	settings.setValue("STONES_LOOK", i);
	
	if ( ui.terrCrossRB->isChecked())
		i=0;
	else
		i=1;
	settings.setValue("TERR_STONE_MARK", i);
	
	i = 0;
	if ( ui.radioButton_noSound->isChecked())
		i=1;
	else if ( ui.radioButton_myGamesSound->isChecked())
		i=2;
	settings.setValue("SOUND", i);
	
	if ( ui.komarkerCB->isChecked())
		i=1;
	else
		i=0;
	settings.setValue("KOMARKER", i);
	if ( ui.numberCurrentMoveCB->isChecked())
		i=1;
	else
		i=0;
	settings.setValue("NUMBER_CURRENT_MOVE", i);
	
	if ( ui.observeOutsideCB->isChecked())
		i=1;
	else
		i=0;
	settings.setValue("OBSERVEOUTSIDE", i);
	
	//saves hosts list
	settings.beginWriteArray("HOSTS");
	for (int i = 0; i < hostlist.size(); ++i) 
	{
		settings.setArrayIndex(i);
		settings.setValue("server", hostlist.at(i)->host());
		settings.setValue("loginName", hostlist.at(i)->loginName());
		settings.setValue("password", hostlist.at(i)->password());
	}
	settings.endArray();

	settings.setValue("ACCOUNT", ui.cb_connect->currentIndex());

	//server games default values
	settings.setValue("DEFAULT_KOMI",ui.komiSpinDefault->value() );
	settings.setValue("DEFAULT_SIZE",ui.boardSizeSpin->value() );
	settings.setValue("DEFAULT_TIME",ui.timeSpin->value() );
	settings.setValue("DEFAULT_BY",ui.BYSpin->value() );

	settings.setValue("NMATCH_BLACK", ui.checkBox_Nmatch_Black->isChecked());
	settings.setValue("NMATCH_WHITE", ui.checkBox_Nmatch_White->isChecked());
	settings.setValue("NMATCH_NIGIRI",ui.checkBox_Nmatch_Nigiri->isChecked());
	settings.setValue("NMATCH_MAIN_TIME", ui.timeSpin_Nmatch->value());
	settings.setValue("NMATCH_BYO_TIME", ui.BYSpin_Nmatch->value());
	settings.setValue("NMATCH_HANDICAP", ui.HandicapSpin_Nmatch->value());

	settings.setValue("AUTOSAVE", ui.CheckBox_autoSave->isChecked());
	settings.setValue("AUTOSAVE_PLAYED", ui.CheckBox_autoSave_Played->isChecked());


	//server byo yomi warning
	settings.setValue("BYO_SOUND_WARNING", ui.ByoSoundWarning->isChecked());
	settings.setValue("BYO_SEC_WARNING",ui.ByoSecWarning->value());

	//SGF edition tab default values
	settings.setValue("EDIT_SIZE",ui.newFile_Size->value());
	settings.setValue("EDIT_HANDICAP",ui.newFile_Handicap->value());
	settings.setValue("EDIT_KOMI",ui.newFile_Komi->text());

	//qDebug("password: %s\n", hostlist.at(0)->password().toLatin1().constData());	

	preferences.fill();
	preferences.save();	//FIXME, save is only for match setting defaults right?
}



/*
 * loads the parameters from the QSettings into the 2 lats tabs
 */
void MainWindow::loadSettings()
{
	QSettings settings;
	QVariant var;
	
	ui.comboBox_language->setCurrentIndex (settings.value("LANGUAGE").toInt());
	ui.LineEdit_computer->setText(settings.value("COMPUTER_PATH").toString());
	if(settings.value("COMPUTER_PLAYS_WHITE").toBool())
		ui.computerPlaysWhite->setChecked(TRUE);
	else
		ui.computerPlaysBlack->setChecked(TRUE);
	ui.newComputer_Handicap->setValue(settings.value("COMPUTER_HANDICAP").toInt());
	/* Why is this komi text and other default komi is value?  FIXME,
	 * Spin versus LineEdit, but inconsistent */
	if((var = settings.value("COMPUTER_KOMI")) == QVariant())
		var = 5.5;
	ui.newComputer_Komi->setText(var.toString());
	
	ui.radioButtonStones_real->setChecked(TRUE);
	ui.radioButtonStones_2D->setChecked((settings.value("STONES_LOOK")==1));
	ui.radioButtonStones_3D->setChecked((settings.value("STONES_LOOK")==2));

	ui.radioButton_allGameSound->setChecked(TRUE);
	ui.radioButton_noSound->setChecked((settings.value("SOUND")==1));
	ui.radioButton_myGamesSound->setChecked((settings.value("SOUND")==2));

	ui.LineEdit_goban->setText(settings.value("SKIN").toString());
	ui.LineEdit_table->setText(settings.value("SKIN_TABLE").toString());

	ui.timerComboBox->setCurrentIndex(settings.value("TIMER_INTERVAL").toInt());
	ui.komarkerCB->setChecked((settings.value("KOMARKER") == 1));
	ui.numberCurrentMoveCB->setChecked((settings.value("NUMBER_CURRENT_MOVE") == 1));
	if(settings.value("TERR_STONE_MARK").toBool())
		ui.terrStoneRB->setChecked(true);
	else
		ui.terrCrossRB->setChecked(true);
	
	ui.observeOutsideCB->setChecked((settings.value("OBSERVEOUTSIDE") == 1));
	
	//server list
	hostlist.clear();
	//ui.cb_connect->clear();
	Host *h;
	int size = settings.beginReadArray("HOSTS");
	for (int i = 0; i < size; ++i) 
	{
		settings.setArrayIndex(i);
		h = new Host(settings.value("server").toString(),
				settings.value("loginName").toString(),
				settings.value("password").toString());
		hostlist.append(h);
	}
 	settings.endArray();

	ui.cb_connect->setCurrentIndex(settings.value("ACCOUNT").toInt());


	//server games default values
	if((var = settings.value("DEFAULT_KOMI")) == QVariant())
		var = 5.5;
	ui.komiSpinDefault->setValue(var.toInt());
	if((var = settings.value("DEFAULT_SIZE")) == QVariant())
		var = 19;
	ui.boardSizeSpin->setValue(var.toInt());
	ui.timeSpin->setValue(settings.value("DEFAULT_TIME").toInt());
	ui.BYSpin->setValue(settings.value("DEFAULT_BY").toInt());

	ui.checkBox_Nmatch_Black->setChecked(settings.value("NMATCH_BLACK", QVariant(TRUE)).toBool());
	ui.checkBox_Nmatch_White->setChecked(settings.value("NMATCH_WHITE", QVariant(TRUE)).toBool());
	ui.checkBox_Nmatch_Nigiri->setChecked(settings.value("NMATCH_NIGIRI", QVariant(TRUE)).toBool());
	ui.HandicapSpin_Nmatch->setValue(settings.value("NMATCH_HANDICAP", QVariant(8)).toInt());	
	ui.timeSpin_Nmatch->setValue(settings.value("NMATCH_MAIN_TIME", QVariant(99)).toInt());
	ui.BYSpin_Nmatch->setValue(settings.value("NMATCH_BYO_TIME", QVariant(60)).toInt());

	ui.CheckBox_autoSave->setChecked(settings.value("AUTOSAVE").toBool());
	ui.CheckBox_autoSave_Played->setChecked(settings.value("AUTOSAVE_PLAYED").toBool());

	//server byo yomi warning
	ui.ByoSoundWarning->setChecked(settings.value("BYO_SOUND_WARNING").toBool());
	ui.ByoSecWarning->setValue(settings.value("BYO_SEC_WARNING").toInt());
	
	
	//SGF edition tab default values
	if((var = settings.value("EDIT_SIZE")) == QVariant())
		var = 19;
	ui.newFile_Size->setValue(var.toInt());
	ui.newFile_Handicap->setValue(settings.value("EDIT_HANDICAP").toInt());
	if((var = settings.value("EDIT_KOMI")) == QVariant())
		var = 5.5;
	ui.newFile_Komi->setText(var.toString());

	preferences.fill();


}

/*
 * The 'get engine' button has been pressed on the Go engine tab
 */
void MainWindow::slot_getComputerPath()
{
	QString fileName(QFileDialog::getOpenFileName(this, tr("Go engine"), "",
		tr("All Files (*)")));
	if (fileName.isEmpty())
		return;

  	ui.LineEdit_computer->setText(fileName);
}

/*
 * The 'get goban path' button has been pressed on the preferences tab
 */
void MainWindow::slot_getGobanPath()
{
	QString fileName(QFileDialog::getOpenFileName(this, tr("Goban picture"), "",
		tr("All Files (*)")));
	if (fileName.isEmpty())
		return;

  	ui.LineEdit_goban->setText(fileName);
}

/*
 * The 'get table path' button has been pressed on the preferences tab
 */
void MainWindow::slot_getTablePath()
{
	QString fileName(QFileDialog::getOpenFileName(this, tr("Table picture"), "",
		tr("All Files (*)")));
	if (fileName.isEmpty())
		return;

  	ui.LineEdit_table->setText(fileName);
}


/*
 * The engine path has been modified on the Go engine tab
 */
void MainWindow::slot_computerPathChanged(const QString & text)
{
	QSettings settings;

	settings.setValue("COMPUTER_PATH", text);
}



