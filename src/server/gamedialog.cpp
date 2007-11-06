/*
 *   gamedialog.cpp
 */
//#include "setting.h"
#include "gamedialog.h"
//#include "newgame_gui.h"
//#include "config.h"
//#include "misc.h"
#include "defines.h"
//#include "komispinbox.h"


GameDialog::GameDialog()//QWidget* parent, const char* name, bool modal, WFlags fl)
	: QDialog() , Ui::GameDialog()
{
	ui.setupUi(this);

	setWindowTitle(tr("New Game"));

	have_suggestdata = false;
	gsname = GS_UNKNOWN;
//	komiSpin->setValue(55);
//	buttonOffer->setFocus();
	komi_request = false;
	is_nmatch = false;
  
	connect(ui.buttonCancel,SIGNAL(pressed()), SLOT(slot_cancel()));
	connect(ui.buttonDecline,SIGNAL(pressed()), SLOT(slot_decline()));
	connect(ui.buttonOffer,SIGNAL(clicked(bool)),SLOT( slot_offer(bool)));
	connect(ui.buttonStats,SIGNAL(pressed()), SLOT(slot_statsOpponent()));


//  boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
//  timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
//  byoTimeSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
//	cb_free->setChecked(true);
}

GameDialog::~GameDialog()
{

}

void GameDialog::closeEvent(QCloseEvent *)
{
	//TODO Check whether there is a match cancel command
	if (is_nmatch)
		emit signal_sendCommand("nmatch _cancel", false);

	QString opponent = ui.playerOpponentEdit->text();

//	emit signal_removeDialog(opponent);
	emit signal_removeDialog(this);

}


void GameDialog::slot_statsOpponent()
{
/* 	if (playerWhiteEdit->isReadOnly())
	{
		// ok, I am white
		emit signal_sendcommand("stats " + playerBlackEdit->text(), false);
	}
	else
	{
		// ok, I am black
		emit signal_sendcommand("stats " + playerWhiteEdit->text(), false);
	}
*/
	emit signal_sendCommand("stats " + ui.playerOpponentEdit->text(), false);


}


/*
void GameDialog::slot_opponentopen(const QString &opp)
{
	qDebug("#### GameDialog::slot_opponentopen()");
	if (playerOpponentEdit->text() != opp)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() != opp ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() != opp)
	    // not for me
	    return;

	QString me;
	QString opponent = playerOpponentEdit->text();;
	// send match command, send tell:

	// 24 *xxxx*: CLIENT: <qGo 1.9.12> match xxxx wants handicap 0, komi 0.5[, free]
	// this command is not part of server preferences "use Komi" and "auto negotiation"
	QString send = "tell " + opponent + " CLIENT: <qGo " + VERSION + "> match " +
		me + " wants handicap " + ui.handicapSpin->text() + ", komi " +
		komiSpin->text();
	if (ComboBox_free->currentText() == QString(tr("yes")))
		send += ", free";

	emit signal_sendcommand(send, false);
}
*/


/* This is used for offering and accepting */
void GameDialog::slot_offer(bool active)
{
	qDebug("#### GameDialog::slot_offer()");
	// active serves for more in the future...
	if (!active)
		return;

	// if both names are identical -> teaching game
	if (ui.playerOpponentEdit->text() == myName)
	{
		emit signal_sendCommand("teach " + ui.boardSizeSpin->text(), false);
		// prepare for future use...
		ui.buttonOffer->setDown(false);
		ui.buttonDecline->setDisabled(true);
		ui.buttonCancel->setEnabled(true);
		ui.buttonOffer->setText(tr("Offer"));

		emit accept();
		return;
	}

	// send match command
	QString color = " B ";
	if (ui.play_white_button->isChecked())
		color = " W ";
	else if (ui.play_nigiri_button->isChecked() && is_nmatch)
		color = " N ";
	//{
		if (is_nmatch)
		{
			//<nmatch yfh2test W 3 19 60 600 25 0 0 0>
			emit signal_sendCommand("nmatch " + 
						ui.playerOpponentEdit->text() + 
						color + //" W " + 
						ui.handicapSpin->text() + " " +
						ui.boardSizeSpin->text() + " " + 
						QString::number(ui.timeSpin->value() * 60) + " " +
						QString::number(ui.byoTimeSpin->value() * 60) + 
						" 25 0 0 0", true); // carefull : 25 stones hard coded : bad
			qDebug("nmatch...");
		}
		else 
		{
			qDebug("match...");
			emit signal_sendCommand("match " + ui.playerOpponentEdit->text() + color + ui.boardSizeSpin->text() + " " + ui.timeSpin->text() + " " + ui.byoTimeSpin->text(), false);
		}

	switch (gsname)
	{
		case NNGS:
		case CWS:
			ui.buttonDecline->setEnabled(true);
			ui.buttonCancel->setDisabled(true);
			break;

		default:
			// IGS etc. don't support a withdraw command
			ui.buttonDecline->setDisabled(true);
			ui.buttonCancel->setEnabled(true);
			break;
	}
}

void GameDialog::slot_decline()
{
qDebug("#### GameDialog::slot_decline()");
	
	QString opponent = ui.playerOpponentEdit->text();//(playerWhiteEdit->isReadOnly() ? playerBlackEdit->text():playerWhiteEdit->text());

	if (ui.buttonOffer->isDown())
	{
		// match has been offered
		// !! there seem to be not "setOn" in the code (apart init, but this should not reach this code)
		emit signal_sendCommand("withdraw", false);
		ui.buttonOffer->setDown(false);
		ui.buttonOffer->setText(tr("Offer"));
	}

	emit signal_sendCommand("decline " + opponent, false);
	emit signal_removeDialog(this /*opponent*/);

}

/*
 * 'Cancel' button pressed
 */
void GameDialog::slot_cancel()
{
/*
	if (is_nmatch)
		emit signal_sendCommand("nmatch _cancel", false);

	QString opponent = ui.playerOpponentEdit->text();

//	emit signal_removeDialog(opponent);
	emit signal_removeDialog(this);
*/
	close();
}

//TODO : not used now
void GameDialog::slot_changed()
{
	qDebug("#### GameDialog::slot_changed()");
	if (ui.playerOpponentEdit->text() == myName)// playerBlackEdit->text())
	{
		ui.buttonOffer->setText(tr("Teaching"));
//		ComboBox_free->setEnabled(false);

		ui.byoTimeSpin->setEnabled(false);
		ui.timeSpin->setEnabled(false);
	}
	else
	{
		//ui.buttonOffer->setText(tr("Offer"));
//		ComboBox_free->setEnabled(true);

		ui.byoTimeSpin->setEnabled(true);
		ui.timeSpin->setEnabled(true);
	}

	// check for free game
	if (ui.playerOpponentRkEdit->text() == "NR" || myRk == "NR" || ui.playerOpponentRkEdit->text() == myRk)
	{
//		ComboBox_free->setCurrentItem(1);
	}
	else
	{
//		ComboBox_free->setEnabled(true);
	}
}
/*
void GameDialog::slot_matchCreate(const QString &nr, const QString &opponent)
{
	qDebug("#### GameDialog::slot_matchcreate()");
	if (ui.playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)
	{
		// current match has been created -> send settings
		assessType kt;
		// check if komi has been requested
*//*		if (myRk != "NR" && oppRk != "NR")
		{
			if (ComboBox_free->currentText() == QString(tr("yes")))
				kt = FREE;
			else
				kt = RATED;
		}
		else
*//*
			kt = noREQ;

		// send to qgoif
		emit signal_matchSettings(nr, ui.handicapSpin->text(), ui.komiSpin->text(), kt);

		// close dialog
		ui.buttonOffer->setDown(false);
		emit accept();

		return;
	}
}
*/
void GameDialog::slot_notOpen(const QString &opponent, int motive)
{
	if (motive == 0)
		ui.refusedLabel->setText(tr("%1 not open for matches").arg(opponent));
	else if (motive == 1) 
		ui.refusedLabel->setText(tr("%1 declined the match request").arg(opponent));
	else if (motive == 2) 
		ui.refusedLabel->setText(tr("%1 canceled the match request").arg(opponent));
	else if (motive == 3) 
		ui.refusedLabel->setText(tr("%1 already playing a game").arg(opponent));		

	qDebug("#### GameDialog::slot_notopen()");
	if (opponent.isEmpty())
	{
		// IGS: no player named -> check if offering && focus set
		if (ui.buttonOffer->isChecked())// && QWidget::hasFocus())
		{
			
//			ui.buttonOffer->setDown(false);
//			ui.buttonOffer->setText(tr("Offer"));
			ui.buttonOffer->setDisabled(true);
			ui.buttonDecline->setDisabled(true);
			ui.buttonCancel->setEnabled(true);
		}
	}
	else if (ui.playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	         playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)
	{

//		ui.buttonOffer->setDown(false);
//		ui.buttonOffer->setText(tr("Offer"));
		ui.buttonOffer->setDisabled(true);
		ui.buttonDecline->setDisabled(true);
		ui.buttonCancel->setEnabled(true);
//		reject();
	}
}

// if opponent has requestet for handicap, komi and/or free game
void GameDialog::slot_komiRequest(const QString &opponent, int h, int k, bool /*free*/)
{
	qDebug("#### GameDialog::slot_komirequest()");
	if (ui.playerOpponentEdit->text() == opponent)
	{

		ui.handicapSpin->setValue(h);

		komi_request = true; //the komi checkbox has been replaced by
		ui.komiSpin->setValue(k);

//		if (free)
//			ComboBox_free->setCurrentItem(1);
//		else
//			ComboBox_free->setCurrentItem(0);

//		ui.buttonOffer->setText(tr("Accept"));
		ui.buttonOffer->setDown(false);
		ui.buttonCancel->setDisabled(true);
	}
	else
		ui.buttonCancel->setEnabled(true);
}


void GameDialog::slot_dispute(const QString &opponent, const QString &line)
{
	QString val;

	QPalette p(QApplication::palette());
	p.setColor(  QPalette::Base , QColor::QColor("cyan"));

	if (ui.playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)

	{
//		val = element(line,1," ");
		val = line.section(" ",1,1);
		if (ui.handicapSpin->text().toInt() != val.toInt())
		{
			ui.handicapSpin->setValue((val.toInt() == 0 ? 1 : val.toInt()));
//			ui.handicapSpin->setPaletteBackgroundColor(QColor::QColor("cyan"));
			ui.handicapSpin->setPalette(p);
		}
		else
			ui.handicapSpin->setPalette(QApplication::palette());

		val = line.section(" ",2,2);
		if (ui.boardSizeSpin->value() != val.toInt())
		{
			ui.boardSizeSpin->setValue(val.toInt());
			ui.boardSizeSpin->setPalette(p);
		}
		else
			ui.boardSizeSpin->setPalette(QApplication::palette());

		val = line.section(" ",3,3);
		if (ui.timeSpin->value() != val.toInt()/60)
		{
			ui.timeSpin->setValue(val.toInt()/60);
			ui.timeSpin->setPalette(p);
		}
		else
			ui.timeSpin->setPalette(QApplication::palette());

		val = line.section(" ",4,4);
		if (ui.byoTimeSpin->value() != val.toInt()/60)
		{
			ui.byoTimeSpin->setValue(val.toInt()/60);
			ui.byoTimeSpin->setPalette(p);
		}
		else
			ui.byoTimeSpin->setPalette(QApplication::palette());	

		val = line.section(" ",0,0);
		if ( !(ui.play_nigiri_button->isChecked()) && (val == "N"))
		{
			ui.play_nigiri_button->setPalette(p);
			play_white_button->setPalette(p);
			ui.play_black_button->setPalette(p);
			ui.play_nigiri_button->setChecked(true);

		}
		else if ( (ui.play_black_button->isChecked()) && (val == "B"))
		{
			ui.play_nigiri_button->setPalette(p);
			ui.play_white_button->setPalette(p);
			ui.play_black_button->setPalette(p);
			ui.play_white_button->setChecked(true);

		}
		else if ( (ui.play_white_button->isChecked()) && (val == "W"))
		{
			ui.play_nigiri_button->setPalette(p);
			ui.play_white_button->setPalette(p);
			ui.play_black_button->setPalette(p);
			ui.play_black_button->setChecked(true);

		}
		else
		{
			ui.play_nigiri_button->setPalette(QApplication::palette());
			ui.play_white_button->setPalette(QApplication::palette());
			ui.play_black_button->setPalette(QApplication::palette());
		}

//	ui.buttonOffer->setText(tr("Accept"));
	ui.buttonOffer->setChecked(false);
	ui.buttonDecline->setEnabled(true);
	
	}
}
