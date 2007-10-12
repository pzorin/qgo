/***************************************************************************
 *
 * This class handles the time information display on the board window
 *
 ***************************************************************************/


#include "clockdisplay.h"
#include "boardwindow.h"
#include "audio.h"

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

	QSettings settings;
	playWarningSound = settings.value("BYO_SOUND_WARNING").toBool();
	warningSecs = settings.value("BYO_SEC_WARNING").toInt();

	warningSound = 	SoundFactory::newSound( "/usr/share/qgo2/sounds/timer.wav" );
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



/*
 * activates the byo-time warnings
 */
void ClockDisplay::warning(bool black)
{
	static bool colorToggle = false;

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
	}

}
