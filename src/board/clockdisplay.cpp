/***************************************************************************
 *
 * This class handles the time information display on the board window
 *
 ***************************************************************************/

/* All the clock display stuff should be stuffed in here and taken out from
 * all the redundant places elsewhere */
/* It annoys me that clock can display 3:56 / 0 after a player has moved.  It does
 * this in observe because server hasn't sent the new time, so we don't display it.
 * But that's a nonsencical reading and we should check it here and update it
 * to the full time left. */


#include "clockdisplay.h"
#include "boardwindow.h"
#include "audio.h"
#include "network/messages.h" 		//for the TimeRecord FIXME

ClockDisplay::ClockDisplay(BoardWindow *bw, TimeSystem s, int _maintime, int _periods, int _periodtime) : QObject(bw)
{
	boardwindow = bw;
	timeSystem = s;		//canadian
	maintime = _maintime;
	periods = _periods;
	periodtime = _periodtime;
	//outOfMainTime = false;
	
	pb_timeBlack = boardwindow->getUi().pb_timeBlack;
	pb_timeWhite = boardwindow->getUi().pb_timeWhite;
#ifdef Q_WS_WIN
	/* Otherwise windows XP style makes time buttons ugly white on white.
	 * Note, this may also be an issue on mac?  and it could interfere
	 * with blinking warning. */
	pb_timeBlack->setStyleSheet("background-color: black; color: white");
	pb_timeWhite->setStyleSheet("background-color: black; color: white");
#endif //Q_WS_WIN
	if(maintime == 0)
	{
		b_time = periodtime;
		w_time = periodtime;
		b_stones_periods = periods;
		w_stones_periods = periods;
	}
	else
	{
		b_time = maintime;
		w_time = maintime;
		b_stones_periods = -1; 
		w_stones_periods = -1; 
	}
	QSettings settings;
	playWarningSound = settings.value("BYO_SOUND_WARNING").toBool();
	warningSecs = settings.value("BYO_SEC_WARNING").toInt();

	warningSound = 	SoundFactory::newSound( "/usr/share/qgo2/sounds/timer.wav" );
}

/*
 * This takes alls values coming from the time information and updates the clocks
 */
 /* FIXME, change to take TimeRecords ?? or not*/
void ClockDisplay::setTimeInfo(int btime, int bstones_periods, int wtime, int wstones_periods)
{
	/* FIXME DOUBLECHECK if this is allowed.  We want to be able
	 * to pass empty records but, this may not be the way to
	 * do it */
	if(wtime != 0)
	{
		w_time = wtime;
		w_stones_periods = wstones_periods; 
	}
	if(btime != 0)
	{ 
		b_time = btime;
		b_stones_periods = bstones_periods;
	}
	//printf("wb %d %d %d %d\n", w_time, b_time, w_stones_periods, b_stones_periods);
	updateTimers();
}

/*
 * decrease the timer info according to the step, and updates the clocks
 */
void ClockDisplay::setTimeStep(bool black)
{
	/* Why would this ever be anything besides -1?? */
	if (black)
	{
		b_time--;
		if(b_time == 0 && b_stones_periods != 0)
		{
			/* FIXME, maybe we should have one clock and subclass
			 * the time system for different clocks?? Isn't that
			 * more c++.  Why do you never just do these things
			 * instead of adding a note?  I guess I want to be
			 * doing other things... */
			/* Also, FIXME, is this too much garbage for a timer
			 * routine? Further, I feel like I've been adding
			 * stuff to this without any thought to efficiency.
			 * Laziness... but so instead of this cumulative
			 * bullshit, this should be made much more efficient,
			 * combined and such... FIXME*/
			if(timeSystem == byoyomi || (timeSystem == tvasia && b_stones_periods != -1))
			{
				printf("b by: %d %d %d %d\n", b_time, b_stones_periods, periodtime, periods);
				if(b_stones_periods > -1 || timeSystem == tvasia)
					b_stones_periods--;
				else
					b_stones_periods = periods;
				if(b_stones_periods != 0)
					b_time = periodtime;
			}
			else if(b_stones_periods == -1)
			{
				if(timeSystem == canadian)
				{
					b_time = periodtime;
					b_stones_periods = periods;
				}
				else if(timeSystem == tvasia)
				{
					b_time = periodtime;
					b_stones_periods = periods - 1;
				}
			}
			else
				printf("other timesystem\n");
		}
	}
	else
	{
		w_time--;
		if(w_time == 0 && w_stones_periods != 0)
		{
			if(timeSystem == byoyomi || (timeSystem == tvasia && w_stones_periods != -1))
			{
				printf("w by: %d %d %d %d\n", w_time, w_stones_periods, periodtime, periods);
				if(w_stones_periods > -1 || timeSystem == tvasia)
					w_stones_periods--;
				else
					w_stones_periods = periods;
				if(w_stones_periods != 0)
					w_time = periodtime;
			}
			else if(w_stones_periods == -1)
			{
				if(timeSystem == canadian)
				{
					w_time = periodtime;
					w_stones_periods = periods;
				}
				else if(timeSystem == tvasia)
				{
					w_time = periodtime;
					w_stones_periods = periods - 1;
				}
			}
			else
				printf("other timesystem\n");
		}
	}
	
	updateTimers();
}

void ClockDisplay::makeMove(bool black)
{
	if (black)
	{
		if(timeSystem == canadian && b_stones_periods != -1)
		{
			b_stones_periods--;
		}
	}
	else
	{
		if(timeSystem == canadian && w_stones_periods != -1)
		{
			w_stones_periods--;
		}
	}
	
	updateTimers();
}

void ClockDisplay::rerackTime(bool black)
{
	if (black)
	{
		if(timeSystem == canadian && b_stones_periods == 0)
		{
			b_time = periodtime;
			b_stones_periods = periods;
		}
		else if(timeSystem == byoyomi && b_stones_periods != -1)
		{
			b_time = periodtime;
			//b_stones_periods = periods;
		}
		else if(timeSystem == tvasia)
		{
			b_time = maintime;
			b_stones_periods = -1;
		}
	}
	else
	{
		if(timeSystem == canadian && w_stones_periods == 0)
		{
			w_time = periodtime;
			w_stones_periods = periods;
		}
		else if(timeSystem == byoyomi && w_stones_periods != -1)
		{
			w_time = periodtime;
			//w_stones_periods = periods;
		}
		else if(timeSystem == tvasia)
		{
			w_time = maintime;
			w_stones_periods = -1;
		}
	}
	
	updateTimers();	
}

TimeRecord ClockDisplay::getTimeRecord(bool black)
{
	if(black)
		return TimeRecord(b_time, b_stones_periods);
	else
		return TimeRecord(w_time, w_stones_periods);
}

/*
 * updates the clocks display
 */
void ClockDisplay::updateTimers()
{
	QString bt, wt;
	/* Negative stones are used to indicate main time period before byo yomi 
	 * not sure about the other two, maybe just 0 minutes */
	/* FIXME periods appear to be unused... which is fine I suppose but
	 * its not clear */
	/* The below has to be first because otherwise the abs() make
	 * the time continually jump back up to one */
	if (b_time < 0)
		b_time = 0;
		//bt.prepend("-");
	if (w_time < 0)
		w_time = 0;
		//wt.prepend("-");	
	
	if(b_time > 3600 || w_time > 3600)
	{
		bt = QTime::QTime(0,0).addSecs(abs(b_time)).toString("h:mm:ss") ;
		wt = QTime::QTime(0,0).addSecs(abs(w_time)).toString("h:mm:ss") ;
	}
	else
	{
		bt = QTime::QTime(0,0).addSecs(abs(b_time)).toString("m:ss") ;
		wt = QTime::QTime(0,0).addSecs(abs(w_time)).toString("m:ss") ;
	}
	
	switch(timeSystem)
	{
		case canadian:
		{

		if (b_stones_periods > -1)	//instead of != -1, safer for WING
			pb_timeBlack->setText(bt + " / " + QString::number(b_stones_periods));
		else
			pb_timeBlack->setText(bt);


		if (w_stones_periods > -1)
			pb_timeWhite->setText(wt + " / " + QString::number(w_stones_periods));
		else
			pb_timeWhite->setText(wt);
		}
			break;
		default:
		case byoyomi:
		case tvasia:
		{

			if (b_stones_periods > -1)	//instead of != -1, safer for WING
				pb_timeBlack->setText(bt + " / " + QString::number(b_stones_periods));
			else
				pb_timeBlack->setText(bt);


			if (w_stones_periods > -1)
				pb_timeWhite->setText(wt + " / " + QString::number(w_stones_periods));
			else
				pb_timeWhite->setText(wt);
		}
			break;
	}


}



/*
 * activates the byo-time warnings
 */
bool ClockDisplay::warning(bool black)
{
	static bool colorToggle = false;
	/* FIXME are premature reddenings of time an issue?  Do we even want
	 * the time turning red ?!? */
	if (black)
	{
		if ((b_time > 0) &&  (b_time < warningSecs))
		{
			colorToggle = !colorToggle;
			pb_timeBlack->setPalette( ( colorToggle ? QPalette(Qt::red) : QPalette(Qt::black) ));
			if (playWarningSound)
				warningSound->play();
		}
		else if ((colorToggle)&& (pb_timeBlack->palette().color(QPalette::Background) != Qt::black))
			pb_timeBlack->setPalette(QPalette(Qt::black));
		else if(b_time == 0 && b_stones_periods == 0)	//FIXME for tvasia
		{
			pb_timeBlack->setPalette(QPalette(Qt::red));
			return false;	//out of time
		}
		else if(pb_timeBlack->palette().color(QPalette::Background) != Qt::black)
			pb_timeBlack->setPalette(QPalette(Qt::black));	//in case premature time reddening
	}
	else
	{
		if ((w_time > 0) &&  (w_time < warningSecs))
		{
			colorToggle = !colorToggle;
			pb_timeWhite->setPalette( ( colorToggle ? QPalette(Qt::red) : QPalette(Qt::black) ));
			if (playWarningSound)
				warningSound->play();
		}
		else if ((colorToggle)&& (pb_timeWhite->palette().color(QPalette::Background) != Qt::black))
			pb_timeWhite->setPalette(QPalette(Qt::black));	
		else if(w_time == 0 && w_stones_periods == 0)
		{
			pb_timeWhite->setPalette(QPalette(Qt::red));
			return false;	//out of time
		}
		else if(pb_timeWhite->palette().color(QPalette::Background) != Qt::black)
			pb_timeWhite->setPalette(QPalette(Qt::black));	//in case premature time reddening
	
	}
	return true;	//still good
}
