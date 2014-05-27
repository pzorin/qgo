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
	setTimeSettings(s, _maintime, _periodtime, _periods);

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

    warningSound = 	new Sound("timer.wav");
}

void ClockDisplay::setTimeSettings(TimeSystem s, int m, int p, int o)
{
	timeSystem = s;		//defaults to canadian
	maintime = m;
	periodtime = p;
	periods = o;
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
	if(wtime != 0 || wstones_periods != -1)
	{
		w_time = wtime;
		w_stones_periods = wstones_periods; 
	}
	if(btime != 0 || bstones_periods != -1)
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
	if (black)
	{
		b_time--;
		if(b_time == 0 && b_stones_periods != 0)
		{
			/* FIXME, maybe we should have one clock and subclass
			 * the time system for different clocks??*/
			if(timeSystem == byoyomi || (timeSystem == tvasia && b_stones_periods != -1))
			{
				//printf("b by: %d %d %d %d\n", b_time, b_stones_periods, periodtime, periods);
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
			/* IGS gets here a lot due to lag.  I.e., canadian time steps down to 0 with stones
			 * still remainaining to be played, then opponent plays and turns out they have five
			 * seconds left or something.  Would be neat to adjust for lag or something. FIXME */
		}
		/* FIXME
		 * major thing, we need to get time to switch
		 * over into byoyomi time on IGS 
		 * I think the issue was that IGS sends a byoyomi entering
		 * message... */
	}
	else
	{
		w_time--;
		if(w_time == 0 && w_stones_periods != 0)
		{
			if(timeSystem == byoyomi || (timeSystem == tvasia && w_stones_periods != -1))
			{
				//printf("w by: %d %d %d %d\n", w_time, w_stones_periods, periodtime, periods);
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
		bt = QTime(0,0).addSecs(abs(b_time)).toString("h:mm:ss") ;
		wt = QTime(0,0).addSecs(abs(w_time)).toString("h:mm:ss") ;
	}
	else
	{
		bt = QTime(0,0).addSecs(abs(b_time)).toString("m:ss") ;
		wt = QTime(0,0).addSecs(abs(w_time)).toString("m:ss") ;
	}
	
	switch(timeSystem)
    {
    case canadian:
		if (b_stones_periods > -1)	//instead of != -1, safer for WING
            bt.append(" / ").append(QString::number(b_stones_periods));

		if (w_stones_periods > -1)
            wt.append(" / ").append(QString::number(w_stones_periods));
        break;
    default:
    case byoyomi:
    case tvasia:
			if (b_stones_periods > -1)	//instead of != -1, safer for WING
                bt.append(" / ").append(QString::number(b_stones_periods));

			if (w_stones_periods > -1)
                wt.append(" / ").append(QString::number(w_stones_periods));
        break;
	}
    boardwindow->setTimeBlack(bt);
    boardwindow->setTimeWhite(wt);
}

/*
 * activates the byo-time warnings
 */
bool ClockDisplay::warning(bool black)
{
    if (black)
	{
		if ((b_time > 0) &&  (b_time < warningSecs))
		{
            boardwindow->warnTimeBlack(TimeLow);
            if (playWarningSound)
                warningSound->play();
        }
		else if(b_time == 0 && b_stones_periods == 0)	//FIXME for tvasia
		{
            boardwindow->warnTimeBlack(TimeExpired);
			return false;	//out of time
		}
        else
            boardwindow->warnTimeBlack(TimeOK);
	}
	else
	{
		if ((w_time > 0) &&  (w_time < warningSecs))
		{
            boardwindow->warnTimeWhite(TimeLow);
			if (playWarningSound)
				warningSound->play();
        }
		else if(w_time == 0 && w_stones_periods == 0)
		{
            boardwindow->warnTimeWhite(TimeExpired);
			return false;	//out of time
		}
        else
            boardwindow->warnTimeWhite(TimeOK);
	}
	return true;	//still good
}
