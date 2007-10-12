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


#ifndef CLOCK_H
#define CLOCK_H

#include "boardwindow.h"
#include "audio.h"

class ClockDisplay : public QObject
{
	Q_OBJECT

public:
	ClockDisplay(BoardWindow *bw);
	~ClockDisplay() {}
	void setTime( bool black, int secs);
	void setTimeStep( bool black, int sec = -1);
	void setTimeInfo(int btime, int bstones, int bperiods, int wtime, int wstones, int wperiods);
	void updateTimers();
	void warning(bool black);
	
private :
	BoardWindow *boardwindow;
	TimeSystem timeSystem;
	int w_time, b_time;
	int w_stones, b_stones;
	int w_periods, b_periods;
	
	QPushButton *pb_timeBlack, * pb_timeWhite;
	Sound *warningSound;
	int warningSecs;
	bool playWarningSound;
};

#endif
