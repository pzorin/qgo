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


#include "gamedialog.h"
#include "audio.h"

#include "defines.h"
//#include "komispinbox.h"
#include "network/networkconnection.h"
#include "network/messages.h"
#include "network/gamedialogflags.h"
#include "network/playergamelistings.h"


GameDialog::GameDialog(NetworkConnection * conn, const PlayerListing * opp)
	: QDialog(), Ui::GameDialog(), connection(conn), opponent(opp)
{
	ui.setupUi(this);
    gameSound = new Sound("blip.wav");
	
	setWindowTitle(tr("New Game"));

	have_suggestdata = false;
//	komiSpin->setValue(55);
//	buttonOffer->setFocus();
	komi_request = false;
	is_nmatch = false;
	offered_and_unrefused = false;
  
	dialog_changed = 0;
	clearChangedFlags();
	
	current_match_request = new MatchRequest();
	
	/* FIXME, what about size 38 and larger boards ?? */
	ui.boardSizeSpin->setRange(1,19);
	ui.handicapSpin->setRange(1,9);
	
	we_are_challenger = false;
	
	/* Not sure what to do with these.  It would make sense to
	* just have a number instead of a time spin since no seconds are
	* allowed in two of the settings... but it would probably be better
	* to set something on the timeSpin so that it still has the :00
	* seconds reminder.  Unless we want to have "seconds" in the text. */
	
	ui.boardSizeSpin->setValue(connection->gd_verifyBoardSize(preferences.default_size));
	ui.komiSpin->setValue(preferences.default_komi);
	
	ui.timeSpin->setDisplayFormat("h:mm:ss");
	ui.BYTimeSpin->setDisplayFormat("h:mm:ss");
	ui.timeSpin->setTime(connection->gd_checkMainTime(canadian, qtimeFromSeconds(preferences.default_stonesmaintime)));
	ui.stonesTimeSpin->setTime(connection->gd_checkPeriodTime(canadian, QTime(0, preferences.default_stonestime/60, preferences.default_stonestime%60)));
	ui.stonesSpin->setValue(connection->gd_checkPeriods(canadian, preferences.default_stones));
	ui.BYTimeSpin->setTime(connection->gd_checkMainTime(byoyomi, QTime(0, preferences.default_byomaintime/60, preferences.default_byomaintime%60)));
	ui.BYPeriodTimeSpin->setTime(connection->gd_checkPeriodTime(byoyomi, QTime(0, preferences.default_byoperiodtime/60, preferences.default_byoperiodtime%60)));
	ui.BYPeriodsSpin->setValue(connection->gd_checkPeriods(byoyomi, preferences.default_byoperiods));
	ui.ASIATimeSpin->setTime(connection->gd_checkMainTime(tvasia, QTime(0, preferences.default_asiamaintime/60, preferences.default_asiamaintime%60)));
	ui.ASIAPeriodTimeSpin->setTime(connection->gd_checkPeriodTime(tvasia, QTime(0, preferences.default_asiaperiodtime/60, preferences.default_asiaperiodtime%60)));
	ui.ASIAPeriodsSpin->setValue(connection->gd_checkPeriods(tvasia, preferences.default_asiaperiods));
	
//	cb_free->setChecked(true);

	connect(ui.buttonCancel,SIGNAL(pressed()), SLOT(slot_cancel()));
	connect(ui.buttonDecline,SIGNAL(pressed()), SLOT(slot_decline()));
	connect(ui.buttonOffer,SIGNAL(clicked(bool)),SLOT( slot_offer(bool)));
	//connect(ui.buttonStats,SIGNAL(pressed()), SLOT(slot_statsOpponent()));

	// in order to check if anything changes
	connect(ui.play_black_button, SIGNAL(clicked(bool)), SLOT(slot_play_black_button()));
	connect(ui.play_white_button, SIGNAL(clicked(bool)), SLOT(slot_play_white_button()));
	connect(ui.play_nigiri_button, SIGNAL(clicked(bool)), SLOT(slot_play_nigiri_button()));
	
	connect(ui.ratedCB, SIGNAL(clicked(bool)), SLOT(ratedCB_changed(bool)));
	connect(ui.boardSizeSpin, SIGNAL(valueChanged(int)), SLOT(slot_boardSizeSpin(int)));
	connect(ui.handicapSpin, SIGNAL(valueChanged(int)), SLOT(slot_handicapSpin(int)));
	connect(ui.komiSpin, SIGNAL(valueChanged(int)), SLOT(slot_komiSpin(int)));
	
	connect(ui.timeTab, SIGNAL(currentChanged(int)), SLOT(slot_timeTab(int)));
	// Does below need seconds or really only minutes??!?!? FIXME
	connect(ui.timeSpin, SIGNAL(timeChanged(const QTime &)), SLOT(slot_timeSpin(const QTime &)));	//weird that this isn't QTimeEdit!!!
	connect(ui.stonesTimeSpin, SIGNAL(timeChanged(const QTime &)), SLOT(slot_stonesTimeSpin(const QTime &)));
	connect(ui.stonesSpin, SIGNAL(valueChanged(int)), SLOT(slot_stonesSpin(int)));
	connect(ui.BYTimeSpin, SIGNAL(timeChanged(const QTime &)), SLOT(slot_BYTimeSpin(const QTime &)));
	connect(ui.BYPeriodTimeSpin, SIGNAL(timeChanged(const QTime &)), SLOT(slot_BYPeriodTimeSpin(const QTime &)));
	connect(ui.BYPeriodsSpin, SIGNAL(valueChanged(int)), SLOT(slot_BYPeriodsSpin(int)));
	connect(ui.ASIATimeSpin, SIGNAL(timeChanged(const QTime &)), SLOT(slot_ASIATimeSpin(const QTime &)));
	connect(ui.ASIAPeriodTimeSpin, SIGNAL(timeChanged(const QTime &)), SLOT(slot_ASIAPeriodTimeSpin(const QTime &)));
	connect(ui.ASIAPeriodsSpin, SIGNAL(valueChanged(int)), SLOT(slot_ASIAPeriodsSpin(int)));
}

void GameDialog::slot_play_black_button(void)
{
	if(current_match_request->color_request == MatchRequest::BLACK)
	{
		dialog_changed--;
		color_request_changed = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else if(!color_request_changed)
	{
		dialog_changed++;
		color_request_changed = true;
		ui.buttonOffer->setText(tr("Offer"));
	}
	qDebug("play black: %d", dialog_changed);
	if(flags & GDF_NIGIRI_EVEN)
		ui.handicapSpin->setEnabled(true);
}

void GameDialog::slot_play_white_button(void)
{
	if(current_match_request->color_request == MatchRequest::WHITE)
	{
		dialog_changed--;
		color_request_changed = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else
	{
		dialog_changed++;
		color_request_changed = true;
		ui.buttonOffer->setText(tr("Offer"));
	}
	qDebug("play white: %d", dialog_changed);
	if(flags & GDF_NIGIRI_EVEN)
		ui.handicapSpin->setEnabled(true);
}

void GameDialog::slot_play_nigiri_button(void)
{
	if(current_match_request->color_request == MatchRequest::NIGIRI)
	{
		dialog_changed--;
		color_request_changed = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else
	{
		dialog_changed++;
		color_request_changed = true;
		ui.buttonOffer->setText(tr("Offer"));
	}
	qDebug("play nigiri: %d", dialog_changed);	
	if(flags & GDF_NIGIRI_EVEN)
	{
		ui.handicapSpin->setValue(0);
		ui.handicapSpin->setEnabled(false);
	}
}

void GameDialog::ratedCB_changed(bool checked)
{	
	if((current_match_request->free_rated == RATED) == checked)	//must have changed to get here
	{
		dialog_changed--;
		ratedchanged = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else if(!ratedchanged)
	{
		dialog_changed++;
		ui.buttonOffer->setText(tr("Offer"));
		ratedchanged = true;
	}
	
	if(flags & GDF_RATED_SIZE_FIXED)
	{
		if(checked)
		{
			qDebug("set disabled boardsize");
			ui.boardSizeSpin->setValue(19);
			ui.boardSizeSpin->setEnabled(false);
		}
		else if(!(flags & GDF_ONLY_DISPUTE_TIME))
			ui.boardSizeSpin->setEnabled(true);
	}
	if(flags & GDF_RATED_HANDICAP_FIXED)
	{
		if(checked)
		{
			// We need to actually calculate the proper handicap
			// FIXME
			// with tygem its either 0 or 1 !!, anything else
			// is automatically friendly
			//ui.boardSizeSpin->setValue(19);
			//ui.boardSizeSpin->setEnabled(false);
			//for now
			ui.handicapSpin->setEnabled(false);
		}
		else if(!(flags & GDF_ONLY_DISPUTE_TIME))
			ui.handicapSpin->setEnabled(true);
	}
	if(flags & GDF_RATED_NO_HANDICAP)
	{
		if(checked)
		{
            const PlayerListing * us = connection->getOurListing();
            getProperKomiHandicap(us->rank, opponent->rank, &(current_match_request->komi), &(current_match_request->handicap));
			ui.handicapSpin->setValue(current_match_request->handicap);
			ui.komiSpin->setValue((int)(current_match_request->komi - 0.5));	//.5 is added by dialog ui
			ui.komiSpin->setEnabled(false);
			ui.handicapSpin->setEnabled(false);
		}
		else if(!(flags & GDF_ONLY_DISPUTE_TIME))
		{
			ui.komiSpin->setEnabled(true);
			ui.handicapSpin->setEnabled(true);
		}
	}
}

void GameDialog::slot_boardSizeSpin(int value)
{
	int bs = connection->gd_verifyBoardSize(value);
	if(bs != value)
	{
		value = bs;
		ui.boardSizeSpin->setValue(value);
	}
	if((int)current_match_request->board_size == value)	//must have changed to get here
	{
		dialog_changed--;
		boardSizechanged = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else if(!boardSizechanged)
	{
		dialog_changed++;
		ui.buttonOffer->setText(tr("Offer"));
		boardSizechanged = true;
	}
}

/* Be careful!! there's a 1/0 issue here FIXME */
void GameDialog::slot_handicapSpin(int value)
{
	if((int)current_match_request->handicap == value)	//must have changed to get here
	{
		dialog_changed--;
		handicapchanged = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else if(!handicapchanged)
	{
		dialog_changed++;
		ui.buttonOffer->setText(tr("Offer"));
		handicapchanged = true;
	}
	if(flags & GDF_KOMI_FIXED6)
	{
		if(value == 0)
			ui.komiSpin->setValue(6);
		else
			ui.komiSpin->setValue(0);
		//already disabled
		//ui.komiSpin->setEnabled(false);
	}
	else
	{
		/* We do this anyway, its just not disabled so it can be changed after */
		if(value == 0)
			ui.komiSpin->setValue(6);
		else
			ui.komiSpin->setValue(0);
	}
}

void GameDialog::slot_komiSpin(int value)
{
	float komi = (float)value;
	if(komi < 0)
		komi -= 0.5;
	else
		komi += 0.5;
	if(current_match_request->komi == komi)	//must have changed to get here
	{
		dialog_changed--;
		komichanged = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else if(!komichanged)
	{
		dialog_changed++;
		ui.buttonOffer->setText(tr("Offer"));
		komichanged = true;
	}
}

void GameDialog::slot_timeTab(int value)
{
	TimeSystem t;
	switch(value)
	{
		case 0:
			t = canadian;
			break;
		case 1:
			t = byoyomi;
			break;
		case 2:
			t = tvasia;
			break;
		default:
			t = none;
			break;
	}
	if(current_match_request->timeSystem == t)	//must have changed to get here
	{
		dialog_changed--;
		ttchanged = false;
		if(dialog_changed == 0 && !current_match_request->first_offer)
			ui.buttonOffer->setText(tr("Accept"));
	}
	else if(!ttchanged)
	{
		dialog_changed++;
		ui.buttonOffer->setText(tr("Offer"));
		ttchanged = true;
	}
}

void GameDialog::slot_timeSpin(const QTime & v)
{
	//can't have seconds... ?
	//if(v.seconds() > 0)
	//	ui.timeSpin->setTime(QTime(v.minutes(), 0);
	QTime check = connection->gd_checkMainTime(canadian, v);
	if(check != v)
	{
		ui.timeSpin->blockSignals(true);
		ui.timeSpin->setTime(check);
		ui.timeSpin->blockSignals(false);
	}
	/*if(current_match_request->timeSystem == canadian)
	{*/
		int seconds = timeToSeconds(v);
		if(current_match_request->maintime == seconds)
		{
			dialog_changed--;
			timechanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!timechanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			timechanged = true;
		}
	//}
}

void GameDialog::slot_stonesTimeSpin(const QTime & v)
{
	QTime check = connection->gd_checkPeriodTime(canadian, v);
	if(check != v)
	{
		ui.stonesTimeSpin->blockSignals(true);
		ui.stonesTimeSpin->setTime(check);
		ui.stonesTimeSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == canadian)
	{
		int seconds = timeToSeconds(check);
		if(current_match_request->periodtime == seconds)
		{
			dialog_changed--;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
			stonesTimechanged = false;
		}
		else if(!stonesTimechanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			stonesTimechanged = true;
		}
	}
}

void GameDialog::slot_stonesSpin(int v)
{
	unsigned int check = connection->gd_checkPeriods(canadian, v);
	if(check != (unsigned)v)
	{
		ui.stonesSpin->blockSignals(true);
		ui.stonesSpin->setValue(check);
		ui.stonesSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == canadian)
	{
		if(current_match_request->stones_periods == v)
		{
			dialog_changed--;
			stoneschanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!stoneschanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			stoneschanged = true;
		}
	}
}

void GameDialog::slot_BYTimeSpin(const QTime & v)
{
	QTime check = connection->gd_checkMainTime(byoyomi, v);
	if(check != v)
	{
		ui.BYTimeSpin->blockSignals(true);
		ui.BYTimeSpin->setTime(check);
		ui.BYTimeSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == byoyomi)
	{
		int seconds = timeToSeconds(v);
		if(current_match_request->maintime == seconds)
		{
			dialog_changed--;
			timechanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!timechanged)
		{
			dialog_changed++;
			timechanged = true;
			ui.buttonOffer->setText(tr("Offer"));
		}
	}
}

void GameDialog::slot_BYPeriodTimeSpin(const QTime & v)
{
	QTime check = connection->gd_checkPeriodTime(byoyomi, v);
	if(check != v)
	{
		ui.BYPeriodTimeSpin->blockSignals(true);
		ui.BYPeriodTimeSpin->setTime(check);
		ui.BYPeriodTimeSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == byoyomi)
	{
		int seconds = timeToSeconds(v);
		if(current_match_request->periodtime == seconds)
		{
			dialog_changed--;
			BYPeriodTimechanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!BYPeriodTimechanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			BYPeriodTimechanged = true;
		}
	}
}

void GameDialog::slot_BYPeriodsSpin(int v)
{
	unsigned int check = connection->gd_checkPeriods(byoyomi, v);
	if(check != (unsigned)v)
	{
		ui.BYPeriodsSpin->blockSignals(true);
		ui.BYPeriodsSpin->setValue(check);
		ui.BYPeriodsSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == byoyomi)
	{
		if(current_match_request->stones_periods == v)
		{
			dialog_changed--;
			BYPeriodschanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!BYPeriodschanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			BYPeriodschanged = true;
		}
	}
}

void GameDialog::slot_ASIATimeSpin(const QTime & v)
{
	QTime check = connection->gd_checkMainTime(tvasia, v);
	if(check != v)
	{
		ui.ASIATimeSpin->blockSignals(true);
		ui.ASIATimeSpin->setTime(check);
		ui.ASIATimeSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == tvasia)
	{
		int seconds = timeToSeconds(v);
		if(current_match_request->maintime == seconds)
		{
			dialog_changed--;
			ASIATimechanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!ASIATimechanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			ASIATimechanged = true;
		}
	}
}

void GameDialog::slot_ASIAPeriodTimeSpin(const QTime & v)
{
	QTime check = connection->gd_checkPeriodTime(tvasia, v);
	if(check != v)
	{
		ui.ASIAPeriodTimeSpin->blockSignals(true);
		ui.ASIAPeriodTimeSpin->setTime(check);
		ui.ASIAPeriodTimeSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == tvasia)
	{
		int seconds = timeToSeconds(v);
		if(current_match_request->periodtime == seconds)
		{
			dialog_changed--;
			ASIAPeriodTimechanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!ASIAPeriodTimechanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			ASIAPeriodTimechanged = true;
		}
	}
}

void GameDialog::slot_ASIAPeriodsSpin(int v)
{
	unsigned int check = connection->gd_checkPeriods(tvasia, v);
	if(check != (unsigned)v)
	{
		ui.ASIAPeriodsSpin->blockSignals(true);
		ui.ASIAPeriodsSpin->setValue(check);
		ui.ASIAPeriodsSpin->blockSignals(false);
	}
	if(current_match_request->timeSystem == tvasia)
	{
		if(current_match_request->stones_periods == v)
		{
			dialog_changed--;
			ASIAPeriodschanged = false;
			if(dialog_changed == 0 && !current_match_request->first_offer)
				ui.buttonOffer->setText(tr("Accept"));
		}
		else if(!ASIAPeriodschanged)
		{
			dialog_changed++;
			ui.buttonOffer->setText(tr("Offer"));
			ASIAPeriodschanged = true;
		}
	}
}	

void GameDialog::clearChangedFlags(void)
{	
	if(dialog_changed != 10000)	//for offers
		dialog_changed = 0;
	color_request_changed = false;
	ratedchanged = false;
	boardSizechanged = false;
	handicapchanged = false;
	komichanged = false;
	ttchanged = false;
	timechanged = false;
	stonesTimechanged = false;
	stoneschanged = false;
	BYPeriodTimechanged = false;
	BYPeriodschanged = false;
	ASIATimechanged = false;
	ASIAPeriodTimechanged = false;
	ASIAPeriodschanged = false;
}

GameDialog::~GameDialog()
{
	qDebug("deconstructing GameDialog");
	delete current_match_request;
}

void GameDialog::closeEvent(QCloseEvent *)
{
	qDebug("GameDialog::closeEvent");
	if(connection)
        connection->closeGameDialog(opponent);
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
	// to clarify: active means the button is down and client is
	// waiting on server response to offer
	if (!active)
	{
		qDebug("Not sending game offer, already active");
		ui.buttonOffer->setChecked(true);	//since we just unchecked it
		return;
	}
	
	offered_and_unrefused = true;
	save_to_preferences();
	if(!dialog_changed)
	{
		if(current_match_request->first_offer)
			connection->sendMatchRequest(current_match_request);
		else
			connection->acceptMatchOffer(opponent, current_match_request);
		qDebug("Match request unchanged\n");
		return;
	}
	// if both names are identical -> teaching game
	if (ui.playerOpponentEdit->text() == myName)
	{
		/* FIXME I'm sure this teaching stuff is going to be buggy */
		emit signal_sendCommand("teach " + ui.boardSizeSpin->text(), false);
		// prepare for future use...
		//ui.buttonOffer->setDown(false);
		ui.buttonOffer->setChecked(false);
		ui.buttonDecline->setDisabled(true);
		ui.buttonCancel->setEnabled(true);
		ui.buttonOffer->setText(tr("Offer"));

		//emit accept();
		connection->acceptMatchOffer(opponent, current_match_request);
		return;
	}

	// send match command
	// below nigiri shouldn't be checked if not possible
	if (ui.play_white_button->isChecked())
	{
		current_match_request->color_request = MatchRequest::WHITE;
		//FIXME maybe check here if we're the challenger??? 
		//current_match_request->challenger_is_black = 0;
	}
	else if(ui.play_black_button->isChecked())
	{
		current_match_request->color_request = MatchRequest::BLACK;
		//current_match_request->challenger_is_black = 1;
	}
	else if (ui.play_nigiri_button->isChecked() /*&& is_nmatch*/)	//what is this nmatch stuff ??? FIXME
	{
		current_match_request->color_request = MatchRequest::NIGIRI;
		//current_match_request->challenger_is_black = 0;
	}
	if(ui.ratedCB->isChecked())
		current_match_request->free_rated = RATED;
	else
		current_match_request->free_rated = FREE;
	current_match_request->board_size = ui.boardSizeSpin->value();
	qDebug("bs value: %d", current_match_request->board_size);
	current_match_request->handicap = ui.handicapSpin->cleanText().toInt();
	current_match_request->komi = (float)ui.komiSpin->value();
	if(current_match_request->komi > 0)
		current_match_request->komi += 0.5;
	else
		current_match_request->komi -= 0.5;
	/* According to ui form, there's only two time fields and according to existing code
	 * and IGS parser code, there's only canadian time... so we'll go with that for now. 
	 * I am now certain that "nmatch" means byoyomi according to IGS... so why
	 * the comment below: "25 stones_periods hard coded: bad ??? FIXME */
	if(ui.timeTab->currentIndex() == 0)	//canadian or is it in IGS???
	{
		//FIXME, really its nmatch for IGS if we support nmatch and
		//opponent does as well
		//current_match_request->nmatch = false;
		current_match_request->nmatch = true;
		current_match_request->maintime = timeToSeconds(ui.timeSpin->time());
		current_match_request->periodtime = (ui.stonesTimeSpin->time().minute() * 60);
		current_match_request->periodtime += ui.stonesTimeSpin->time().second();
		current_match_request->stones_periods = ui.stonesSpin->value();
		current_match_request->timeSystem = canadian;
	}
	else if(ui.timeTab->currentIndex() == 1)	//byo yomi IGS nmatch
	{
		current_match_request->nmatch = true;
		current_match_request->maintime = timeToSeconds(ui.BYTimeSpin->time());
		if(!(flags & GDF_BY_CAN_MAIN_MIN))	//ugly FIXME
			current_match_request->maintime += ui.BYTimeSpin->time().second();
		current_match_request->periodtime = (ui.BYPeriodTimeSpin->time().minute() * 60);
		current_match_request->periodtime += ui.BYPeriodTimeSpin->time().second();
		current_match_request->stones_periods = ui.BYPeriodsSpin->value();
		current_match_request->timeSystem = byoyomi;
	}
	else if(ui.timeTab->currentIndex() == 2)	//tv asia, ORO
	{
		current_match_request->maintime = timeToSeconds(ui.ASIATimeSpin->time());
		/* We can have more than 60 seconds... like 300 FIXME */
		//current_match_request->maintime = (ui.ASIATimeSpin->time().minute() * 60);
		//current_match_request->maintime += ui.ASIATimeSpin->time().second();
		current_match_request->periodtime = (ui.ASIAPeriodTimeSpin->time().minute() * 60);
		current_match_request->periodtime += ui.ASIAPeriodTimeSpin->time().second();
		current_match_request->stones_periods = ui.ASIAPeriodsSpin->value();
		current_match_request->timeSystem = tvasia;
	}
		
	qDebug("Match request changed %d\n", dialog_changed);
	connection->sendMatchRequest(current_match_request);
	/* FIXME
	 * I'm pretty sure that we want to SetPalette on all the mutable ui
	 * elements to the app default when we hit offer. But we should
	 * double check this behavior, see if it makes sense.*/
	ui.boardSizeSpin->setPalette(QApplication::palette());
	ui.komiSpin->setPalette(QApplication::palette());
	ui.handicapSpin->setPalette(QApplication::palette());
	ui.timeSpin->setPalette(QApplication::palette());
	ui.stonesTimeSpin->setPalette(QApplication::palette());
	ui.stonesSpin->setPalette(QApplication::palette());
	ui.BYTimeSpin->setPalette(QApplication::palette());
	ui.BYPeriodTimeSpin->setPalette(QApplication::palette());
	ui.BYPeriodsSpin->setPalette(QApplication::palette());
	ui.ASIATimeSpin->setPalette(QApplication::palette());
	ui.ASIAPeriodTimeSpin->setPalette(QApplication::palette());
	ui.ASIAPeriodsSpin->setPalette(QApplication::palette());
}

void GameDialog::slot_decline()
{
	qDebug("#### GameDialog::slot_decline()");
	
	//QString opponent = ui.playerOpponentEdit->text();//(playerWhiteEdit->isReadOnly() ? playerBlackEdit->text():playerWhiteEdit->text());
	
#ifdef FIXME
	if (ui.buttonOffer->isDown())
	{
		// match has been offered
		// !! there seem to be not "setOn" in the code (apart init, but this should not reach this code)
		qDebug("GameDialog::slot_decline sends withdraw  (not IGS)");
		emit signal_sendCommand("withdraw", false);
		//ui.buttonOffer->setDown(false);
		ui.buttonOffer->setChecked(false);
		ui.buttonOffer->setText(tr("Offer"));
	}
#endif //FIXME
	/* FIXME this is also sending a lot of unnecessary declines in IGS.
	 * like maybe we only need to send an IGS decline if there's been an offer
	 * and a counter offer.  But what if you offer a game and then cancel
	 * the offer?  Maybe you can't...  we could check for the decline text
	 * on the button as well, but then what exactly is the flag for...
	 * confusing...*/
	if(offered_and_unrefused)	//FIXME decline button shouldn't be possible otherwise
		connection->declineMatchOffer(opponent);
	
	close();
}

void GameDialog::slot_cancel()
{
	if(offered_and_unrefused && ui.buttonDecline->isEnabled())
		connection->declineMatchOffer(opponent);
	else if(ui.buttonOffer->isEnabled() && ui.buttonOffer->text() == tr("Offer"))
		connection->cancelMatchOffer(opponent);
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

		ui.stonesTimeSpin->setEnabled(false);
		ui.timeSpin->setEnabled(false);
	}
	else
	{
		//ui.buttonOffer->setText(tr("Offer"));
//		ComboBox_free->setEnabled(true);

		ui.stonesTimeSpin->setEnabled(true);
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
	// FIXME
	//ui.buttonOffer->setDown(false);
	ui.buttonOffer->setChecked(false);
	ui.buttonOffer->setText(tr("Offer"));	//counter?
}

void GameDialog::recvRefuseMatch(int motive)
{
	if (motive == GD_REFUSE_NOTOPEN)
        ui.refusedLabel->setText(tr("%1 not open for matches").arg(opponent->name));
	else if (motive == GD_REFUSE_DECLINE) 
        ui.refusedLabel->setText(tr("%1 declined the match request").arg(opponent->name));
	else if (motive == GD_REFUSE_CANCEL) 
        ui.refusedLabel->setText(tr("%1 canceled the match request").arg(opponent->name));
	else if (motive == GD_REFUSE_INGAME) 
        ui.refusedLabel->setText(tr("%1 already playing a game").arg(opponent->name));
	else if(motive == GD_REFUSE_NODIRECT)
        ui.refusedLabel->setText(tr("%1 does not accept direct matches").arg(opponent->name));
	else if(motive == GD_OPP_NO_NMATCH)
        ui.refusedLabel->setText(tr("%1's client does not support nmatch").arg(opponent->name));
	else if(motive == GD_INVALID_PARAMETERS)
	{
		ui.refusedLabel->setText(tr("Invalid Parameters!"));
		ui.buttonOffer->setText(tr("Offer"));
		ui.buttonOffer->setChecked(false);
		ui.buttonDecline->setDisabled(true);
		return;
	}
	else if(motive == GD_RESET)
	{
		ui.buttonOffer->setText(tr("Offer"));
		ui.buttonOffer->setChecked(false);
		return;
	}
	qDebug("#### GameDialog::slot_notopen()");
    if (opponent->name.isEmpty())	//FIXME
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
    else if (ui.playerOpponentEdit->text() == opponent->name)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	         playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)
	{

//		ui.buttonOffer->setDown(false);
//		ui.buttonOffer->setText(tr("Offer"));
		ui.buttonOffer->setDisabled(true);
		ui.buttonDecline->setDisabled(true);
		ui.buttonCancel->setEnabled(true);
	}
	offered_and_unrefused = false;
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
		ui.buttonOffer->setChecked(false);
		ui.buttonCancel->setDisabled(true);
	}
	else
		ui.buttonCancel->setEnabled(true);
}

/* If there's an existing match request, we need to highlight the changes */
void GameDialog::recvRequest(MatchRequest * mr, unsigned long _flags)
{
	bool destroy_other_timetabs = false;
	bool this_is_offer = false;
	/* A little iffy here.  We might want to get the flags
	 * every request, but it also might be that we don't
	 * want to have them handy, so we just want to set them
	 * once the first time */
	if(_flags)
	{
		flags = _flags;
		if(!(flags & GDF_CANADIAN))
			ui.timeTab->setTabEnabled(0, false);
		if(!(flags & GDF_BYOYOMI))
			ui.timeTab->setTabEnabled(1, false);
		/* We remove ASIA because its the last tab.
		 * this is rather unfortunately, we need names
		 * for the tabs, FIXME, otherwise it screws
		 * the indexes up to delete the tabs */
		if(!(flags & GDF_TVASIA))
			ui.timeTab->removeTab(2);
		
		if(!(flags & GDF_FREE_RATED_CHOICE))
			ui.ratedCB->setEnabled(false);
		if((flags & GDF_RATED_NO_HANDICAP) && (!mr || mr->free_rated == RATED))
		{
            const PlayerListing * us = connection->getOurListing();
			float k;
			unsigned int h;
            getProperKomiHandicap(us->rank, opponent->rank, &k, &h);
			if(h > 1)
			{
				//mr->free_rated = FREE;	//necessary?? FIXME
				ui.ratedCB->setChecked(false);
				ui.ratedCB->setEnabled(false);
			}
		}
		if((flags & GDF_STONES25_FIXED) && !mr)
		{
			ui.stonesSpin->setValue(25);
			ui.stonesSpin->setEnabled(false);
		}
		
		/* We can actually ignore this with ORO, change
		 * and send whatever we want, but I feel like its
		 * cheap to the opponent who doesn't even see
		 * the changes */
		if((flags & GDF_ONLY_DISPUTE_TIME) && mr)
		{
			/* I don't like the color of this gray
			 * FIXME.  It looks more like an option
			 * that's removed rather than something
			 * that you're not allowed to change */
			ui.handicapSpin->setEnabled(false);
			ui.boardSizeSpin->setEnabled(false);
			ui.komiSpin->setEnabled(false);
			ui.play_black_button->setEnabled(false);
			ui.play_white_button->setEnabled(false);
			ui.play_nigiri_button->setEnabled(false);
			ui.ratedCB->setEnabled(false);
			
			destroy_other_timetabs = true;
		}
		/* As far as I know, no protocols allow time systems
		* with main time in seconds except TVASIA and there's
		* also ranges on these things.  We need flags for that
		* as well FIXME */
		/* Note that we still FIXME need a flag for things like
		 * 10 second increments and the like.
		 * Also, it doesn't like more than two mm, or two ss
		 * which means we need hours I guess and I wonder if
		 * that's clear and then maybe we need a "seconds"
		 * on the label. FIXME.*/
		if(flags & GDF_BY_CAN_MAIN_MIN)
		{
			//ui.timeSpin->setDisplayFormat("mmm");
			//ui.BYTimeSpin->setDisplayFormat("mmm");
		}
		//ui.ASIATimeSpin->setDisplayFormat("sss");
		if((flags & GDF_KOMI_FIXED6) && !mr)
		{
			if(ui.handicapSpin->value() == 0)
				ui.komiSpin->setValue(6);
			else
				ui.komiSpin->setValue(0);
			ui.komiSpin->setEnabled(false);
		}
		if(mr)
		{
#ifdef FIXME
			if(mr->timeSystem == canadian)
			{
				//is min 0 or is it the current ? FIXME
				if(mr->nmatch_timeMax)
					ui.timeSpin->setTimeRange(qtimeFromSeconds(0), qtimeFromSeconds(mr->nmatch_timeMax));
					//ui.timeSpin->setTimeRange(qtimeFromSeconds(mr->maintime), qtimeFromSeconds(mr->nmatch_timeMax));
				if(mr->nmatch_BYMax)
					ui.stonesTimeSpin->setTimeRange(qtimeFromSeconds(0), qtimeFromSeconds(mr->nmatch_BYMax));
					//ui.stonesTimeSpin->setTimeRange(qtimeFromSeconds(mr->periodtime), qtimeFromSeconds(mr->nmatch_BYMax));
			}
			else if(mr->timeSystem == byoyomi)
			{
				if(mr->nmatch_timeMax)
					ui.BYTimeSpin->setTimeRange(QTime(0, mr->maintime/60, 0), QTime(0, mr->nmatch_timeMax/60, 0));
				if(mr->nmatch_BYMax)
					ui.BYPeriodTimeSpin->setTimeRange(QTime(0, mr->periodtime/60, 0), QTime(0, mr->nmatch_BYMax/60, 0));
			}
			if(mr->nmatch_handicapMax)
				ui.handicapSpin->setRange(mr->handicap, mr->nmatch_handicapMax);	
#endif //FIXME
			//not necessarily: !!! FIXME
			//we_are_challenger = true;
			//this_is_offer = true;
		}
	}
	if(!mr)
	{
		/* FIXME FIXME FIXME
		 * if this is the open of the dialog,
		 * we need to fill out the current_match_request,
		 * otherwise it gets sent as uninitialized garbage!!
		 * at least sometimes */
		//load defaults
		//mr->time = preferences.default_maintime;
		//mr->board_size = preferences.default_boardsize;
		//etc...
		// FIXME
		mr = new MatchRequest();
        const PlayerListing * us = connection->getOurListing();
        mr->opponent = opponent->name;
        mr->opponent_id = opponent->id;
        mr->their_rank = opponent->rank;
        mr->our_name = us->name;
        mr->our_rank = us->rank;	//us.rank sounds like bad grammar
		mr->timeSystem = canadian;
		mr->maintime = 600;
		mr->periodtime = 600;
		mr->stones_periods = 25;
        if(getProperKomiHandicap(us->rank, opponent->rank, &(mr->komi), &(mr->handicap)))
		{
			//challenger is black is a mystery byte
			//I'm thinking it might be whether the request is
			// modifiable, or if our only answer is to accept
			// or decline, okay, nevermind, its not modification...
			// FIXME
			mr->challenger_is_black = 1;
			mr->color_request = MatchRequest::BLACK;
		}
		else
		{
			mr->challenger_is_black = 1;
			mr->color_request = MatchRequest::WHITE;
		}
		if(mr->handicap == 1 && !(flags & GDF_HANDICAP1))
			mr->handicap = 0; 
		we_are_challenger = true;
		this_is_offer = true;
		/* FIXME, a lot of stuff to fill in here, defaults and the
		 * like to make it kosher across the board */
		mr->flags = 0xf0;	//FIXME for now
		mr->number = connection->getRoomNumber();
		mr->opponent_is_challenger = false;
		mr->first_offer = true;
		//mr->free_rated = FREE;
		mr->free_rated = RATED;		/* its more typically rated by default, but we should get this from somewhere FIXME */
		mr->nmatch = true;	//igs
		current_match_request->nmatch = mr->nmatch;	//awkward, but not set anywhere else
		dialog_changed = 10000;	//so it can't be anything but "Offer"
	}
	else
	{
		if(mr->first_offer)
		{
			this_is_offer = true;
			dialog_changed = 10000;	//so it can't be anything but "Offer"
			//FIXME should be in one place, also, this FORCES us
			//to call recvRequest after getGameDialog
			//the whole thing is awkward and now that we have most
			//of the protocols figured out it should be cleaned up
			//also, this may not be necessary, it didn't fix the problem
			//FIXME
		}
		else
			offered_and_unrefused = true;
	}
	/* I think above is right... so if its a default that we haven't offered yet,
	 * then offered_and_unrefused is left off, but if its from them, then we
	 * can send decline.  We also send a decline after we offer which is
	 * a little weird... I mean either they accept it and there's a game, or
	 * they change it and then we'd alter offered_and_unrefused here.
	 * probably FIXME, I just wonder why its like that in the first place */
	
	QPalette p(QApplication::palette());
	p.setColor(  QPalette::Base , QColor("cyan"));
	
	qDebug("GameDialog::recvRequest");
	/* If there's no existing request, then nothing should have changed
	 * opponent can't change, so that's the check. */
	if(current_match_request->opponent != mr->opponent)
		*current_match_request = *mr;
	
	
	set_oppRk(mr->their_rank);
		
	set_myRk(mr->our_rank);

	ui.playerOpponentEdit->setText(mr->opponent);		
	ui.playerOpponentEdit->setReadOnly(true);		
	ui.playerOpponentRkEdit->setText(mr->their_rank);
	set_myName(mr->our_name);

	if(mr->free_rated != current_match_request->free_rated)
		ui.ratedCB->setPalette(p);
	if(mr->free_rated == RATED)
		ui.ratedCB->setChecked(true);
	else
		ui.ratedCB->setChecked(false);
	ratedCB_changed((mr->free_rated == RATED));
	
	/* These may have rated fixed issues as well */
	switch(mr->color_request)
	{
		case MatchRequest::BLACK:
			ui.play_black_button->setChecked(true);
			if(mr->color_request != current_match_request->color_request)
				ui.play_black_button->setPalette(p);
			slot_play_black_button();
			break;
		case MatchRequest::WHITE:
			ui.play_white_button->setChecked(true);	
			if(mr->color_request != current_match_request->color_request)
				ui.play_white_button->setPalette(p);
			slot_play_white_button();
			break;
		case MatchRequest::NIGIRI:
			ui.play_nigiri_button->setChecked(true);
			if(mr->color_request != current_match_request->color_request)
				ui.play_nigiri_button->setPalette(p);
			slot_play_nigiri_button();
			break;
	}
	
	if(mr->komi != current_match_request->komi)
			ui.komiSpin->setPalette(p);
	ui.komiSpin->setValue((int)mr->komi);
	
	
	if(mr->board_size != current_match_request->board_size)
		ui.boardSizeSpin->setPalette(p);
	ui.boardSizeSpin->setValue(mr->board_size);
	
	if(mr->handicap != current_match_request->handicap)
		ui.handicapSpin->setPalette(p);
	ui.handicapSpin->setValue(mr->handicap);
	
	if (/*mr->nmatch || */mr->timeSystem == byoyomi)
	{
		qDebug("nmatch");
		qDebug("%d %d %d", mr->maintime, mr->periodtime, mr->stones_periods);
		//specific behavior here : IGS nmatch not totally supported
		// disputes are hardly supported
		set_is_nmatch(true);	//FIXME, change to is BY setting
		//mr->nmatch = true;
		if(mr->maintime != current_match_request->maintime)
			ui.BYTimeSpin->setPalette(p);
		ui.BYTimeSpin->setTime(QTime(mr->maintime/3600, (mr->maintime % 3600)/60, mr->maintime%60));
		
		if(mr->periodtime != current_match_request->periodtime)
			ui.BYPeriodTimeSpin->setPalette(p);
		ui.BYPeriodTimeSpin->setTime(QTime(0, mr->periodtime/60, mr->periodtime%60));
		
		ui.BYPeriodsSpin->setRange(0,99);
		if(mr->stones_periods != current_match_request->stones_periods)
			ui.BYPeriodsSpin->setPalette(p);
		ui.BYPeriodsSpin->setValue(mr->stones_periods);
		
		//ui.BY_label->setText(tr(" Byo Time : (") + mr->stones_periods+ tr(" stones_periods)"));
		// necessary?  these enables/disables?
		//ui.handicapSpin->setEnabled(true);
		//ui.play_nigiri_button->setEnabled(true);
		ui.timeTab->setCurrentIndex(1);
		if(current_match_request->timeSystem != byoyomi)
			ui.timeTab->setPalette(p);
		if(destroy_other_timetabs && ui.timeTab->count() > 1)
		{
			ui.timeTab->setTabEnabled(0, false);
			ui.timeTab->setTabEnabled(2, false);	
		}
	}
	else if (mr->timeSystem == tvasia)
	{
		/* As I understand it, tv asia time is just
		 * byo yomi with a maintime in seconds... if I'm wrong, if its
		 * something else... and I really need to check this in an
		 * actual ORO on ORO game... */
		if(mr->maintime != current_match_request->maintime)
			ui.ASIATimeSpin->setPalette(p);
		
		ui.ASIATimeSpin->setTime(QTime(0, mr->maintime/60, mr->maintime%60));
		
		if(mr->periodtime != current_match_request->periodtime)
			ui.ASIAPeriodTimeSpin->setPalette(p);
		ui.ASIAPeriodTimeSpin->setTime(QTime(0, mr->periodtime/60, mr->periodtime%60));
		
		ui.ASIAPeriodsSpin->setRange(0,99);
		if(mr->stones_periods != current_match_request->stones_periods)
			ui.ASIAPeriodsSpin->setPalette(p);
		ui.ASIAPeriodsSpin->setValue(mr->stones_periods);
		
		ui.timeTab->setCurrentIndex(2);
		if(current_match_request->timeSystem != tvasia)
			ui.timeTab->setPalette(p);
		if(destroy_other_timetabs && ui.timeTab->count() > 1)
		{
			ui.timeTab->setTabEnabled(0, false);
			ui.timeTab->setTabEnabled(1, false);
		}
	}
	else if(/* !mr->nmatch || */mr->timeSystem == canadian)
	{
		qDebug("no nmatch");
		set_is_nmatch(false);
		
		ui.timeSpin->setTime(qtimeFromSeconds(mr->maintime));
		if(mr->maintime != current_match_request->maintime)
			ui.timeSpin->setPalette(p);
		
		ui.stonesTimeSpin->setTime(qtimeFromSeconds(mr->periodtime));
		if(mr->periodtime != current_match_request->periodtime)
			ui.stonesTimeSpin->setPalette(p);
		
		ui.stonesSpin->setValue(mr->stones_periods);
		if(mr->stones_periods != current_match_request->stones_periods)
			ui.stonesSpin->setPalette(p);
		
		// necessary?  these enables/disables?
		//ui.handicapSpin->setEnabled(false);
		//ui.play_nigiri_button->setEnabled(false);
		
		if(mr->board_size != current_match_request->board_size)
			ui.boardSizeSpin->setPalette(p);
		ui.boardSizeSpin->setValue(mr->board_size);

		ui.timeTab->setCurrentIndex(0);
		if(current_match_request->timeSystem != canadian)
			ui.timeTab->setPalette(p);
		if(destroy_other_timetabs && ui.timeTab->count() > 1)
		{
			ui.timeTab->setTabEnabled(1, false);
			ui.timeTab->setTabEnabled(2, false);
		}
	}
	
	
	qDebug("set-up accept");
	if(this_is_offer)
	{
		ui.buttonDecline->setEnabled(false);
		ui.buttonOffer->setChecked(false);
		ui.buttonOffer->setText(tr("Offer"));
		ui.buttonCancel->setEnabled(true);
	}
	else
	{
		ui.buttonDecline->setEnabled(true);
		ui.buttonOffer->setChecked(false);
		ui.buttonOffer->setText(tr("Accept"));
		ui.buttonCancel->setDisabled(true);
	}
	
	show();
//	dlg->setWindowState(Qt::WindowActive);
	raise();

	if(!this_is_offer)
		gameSound->play();
	// store this as the new offer
	*current_match_request = *mr;
	clearChangedFlags();
}

void GameDialog::save_to_preferences(void)
{
	preferences.default_size = ui.boardSizeSpin->value();
	//preferences.default_komi = ui.komiSpin->value();
	
	preferences.default_stonesmaintime = timeToSeconds(ui.timeSpin->time());
	preferences.default_stonestime = (ui.stonesTimeSpin->time().minute() * 60) + ui.stonesTimeSpin->time().second();
	preferences.default_stones = ui.stonesSpin->value();
	preferences.default_byomaintime = (ui.BYTimeSpin->time().minute() * 60) + ui.BYTimeSpin->time().second();
	preferences.default_byoperiodtime = (ui.BYPeriodTimeSpin->time().minute() * 60) + ui.BYPeriodTimeSpin->time().second();
	preferences.default_byoperiods = ui.BYPeriodsSpin->value();
	preferences.default_asiamaintime = (ui.ASIATimeSpin->time().minute() * 60) + ui.ASIATimeSpin->time().second();
	preferences.default_asiaperiodtime = (ui.ASIAPeriodTimeSpin->time().minute() * 60) + ui.ASIAPeriodTimeSpin->time().second();
	preferences.default_asiaperiods = ui.ASIAPeriodsSpin->value();
}


MatchRequest * GameDialog::getMatchRequest(void)
{
	return current_match_request;
}

QTime GameDialog::qtimeFromSeconds(int seconds)
{
	if(seconds == 0)
		return QTime(0, 0, 0);
	int minutes = seconds / 60;
	int hours;
	seconds %= 60;
	if(minutes == 0)
		hours = 0;
	else
	{
		hours = minutes / 60;
		minutes %= 60;
	}
	return QTime(hours, minutes, seconds);	
}

unsigned int GameDialog::timeToSeconds(QString time)
{
	QRegExp re = QRegExp("([0-9]{1,3}):([0-9]{1,2})");
	int min, sec;
	
	if(re.indexIn(time) >= 0)
	{
		min = re.cap(1).toInt();
		sec = re.cap(2).toInt();	
	}
	else
	{
		qDebug("Bad time string");
		return 0xffff;
	}
	
	return (60 * min) + sec;
}

unsigned int GameDialog::timeToSeconds(const QTime & t)
{
	return ((t.hour() * 3600) + (t.minute() * 60) + t.second());	
}

//we just assume that white has the higher rank
/* We're having this return 1 handicap if there's a one stone
 * difference, but this only works because this is only called
 * from the one place... then we'll use the GDF_HANDICAP1 flag
 * to determine if that's set */
bool GameDialog::getProperKomiHandicap(QString rankA, QString rankB, float * komi, unsigned int * handicap)
{
	/* Check for k, d, or p (as d), calc difference, */
	QString buffer = rankA;
	buffer.replace(QRegExp("[pdk+?]"), "");
	int ordinalA = buffer.toInt();
	buffer = rankB;
	buffer.replace(QRegExp("[pdk+?]"), "");
	int ordinalB = buffer.toInt();
	int difference;
	bool A_lowerthan_B = false;
	
	if((rankA.contains("k") && (rankB.contains("d") || rankB.contains("p")))
	|| (rankB.contains("k") && (rankA.contains("d") || rankA.contains("p"))))
	{
		difference = ordinalB + ordinalA - 1;
		*komi = 0.0;
		if(difference == 1)
			*handicap = 1;	
		else if(difference > 9)
			*handicap = 9;
		else
			*handicap = difference;
		if(rankA.contains("k"))
			A_lowerthan_B = true;
		else
			A_lowerthan_B = false;
		
	}
	else
	{
		if(ordinalA == ordinalB)
		{
			*komi = preferences.default_komi;
			*handicap = 0;
		}
		else
		{
			difference = abs(ordinalA - ordinalB);
			*komi = 0.0;
			if(difference == 1)
				*handicap = 1;	
			else if(difference > 9)
				*handicap = 9;
			else
				*handicap = difference;
			if(ordinalA > ordinalB && rankA.contains("k"))
				A_lowerthan_B = true;
			else if(ordinalA < ordinalB && rankA.contains("k"))
				A_lowerthan_B = false;
			else if(ordinalA > ordinalB)
				A_lowerthan_B = false;
			else if(ordinalA < ordinalB)
				A_lowerthan_B = true;
		}	
	}
	return A_lowerthan_B;
}
