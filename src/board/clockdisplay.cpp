/***************************************************************************
 *
 * This class handles the time information display on the board window
 *
 ***************************************************************************/


#include "clockdisplay.h"
#include "boardwindow.h"


ClockDisplay::ClockDisplay(BoardWindow *bw) : QObject(bw)
{
	boardwindow = bw;
	timeSystem = canadian;

	pb_timeBlack = boardwindow->getUi().pb_timeBlack;
	pb_timeWhite = boardwindow->getUi().pb_timeWhite;
	
	b_time=0;
	w_time=0;
	b_stones = -1; 
	w_stones = -1; 
	b_periods = 0; 
	w_periods = 0;	
}

/*
 * This takes alls values coming from the time information and updates the clocks
 */
void ClockDisplay::setTimeInfo(int btime, int bstones, int bperiods, int wtime, int wstones, int wperiods)
{
	w_time = wtime;
	b_time = btime;
	b_stones = bstones; 
	w_stones = wstones; 
	b_periods = bperiods; 
	w_periods = wperiods; 

	updateTimers();
}

/*
 * decrease the timer info according to the step, and updates the clocks
 */
void ClockDisplay::setTimeStep(bool black, int secs)
{
	if (black)
		b_time += secs;
	else
		w_time += secs;
	
	updateTimers();
}



/*
 * updates the clocks display
 */
void ClockDisplay::updateTimers()
{

	QString bt = QTime::QTime(0,0).addSecs(abs(b_time)).toString("m:ss") ;
	QString wt = QTime::QTime(0,0).addSecs(abs(w_time)).toString("m:ss") ;

	if (b_time < 0)
		bt.prepend("-");
	if (w_time < 0)
		wt.prepend("-");	

	switch(timeSystem)
	{
		case canadian:
		{

		if (b_stones != -1)
			pb_timeBlack->setText(bt + " / " + QString::number(b_stones));
		else
			pb_timeBlack->setText(bt);


		if (w_stones != -1)
			pb_timeWhite->setText(wt + " / " + QString::number(w_stones));
		else
			pb_timeWhite->setText(wt);
		}

		default:
			break;
	}


}
