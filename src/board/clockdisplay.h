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


#ifndef CLOCK_H
#define CLOCK_H

#include "defines.h"
#include <QObject>

class BoardWindow;
class Sound;

class ClockDisplay : public QObject
{
	Q_OBJECT

public:
	ClockDisplay(BoardWindow *bw, TimeSystem s, int _maintime, int _period, int _periodtime);
	~ClockDisplay() {}
	void setTime( bool black, int secs);
	void setTimeStep(bool black);
	void rerackTime(bool black);
	void makeMove(bool black);
	class TimeRecord getTimeRecord(bool black);
	void setTimeSettings(TimeSystem s, int m, int p, int o);
	void setTimeInfo(int btime, int bstones_periods, int wtime, int wstones_periods);
	void updateTimers();
	bool warning(bool black);
private :
	BoardWindow *boardwindow;
	TimeSystem timeSystem;
	int w_time, b_time;
	int w_stones_periods, b_stones_periods;
	int maintime, periods, periodtime;
    Sound *warningSound;
	int warningSecs;
	bool playWarningSound;
	bool outOfMainTime;
	bool last_black;
};

#endif
