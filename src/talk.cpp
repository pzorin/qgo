
/*
 *   Talk - Class to handle  Talk Dialog Windows
 */

#include "talk.h"
#include "network/talkdispatch.h"
#include "network/gamedialogdispatch.h"
#include "network/messages.h"
#include "network/playergamelistings.h"
/* I wonder if we could somehow generalize this class to handle
 * all message/windows even if they're part of larger windows.
 * We could even do it for the console, specifying a special
 * "opponent" and for the room, with the room name as the
 * opponent.  And then it could handle all the sendText
 * messages that would otherwise be spread around all
 * the dispatches, cluttering up their otherwise
 * specific calls.*/


int Talk::counter = 0;

Talk::Talk(TalkDispatch * dis, const PlayerListing & player) : TalkGui(), dispatch(dis), opponent(player)
{
	qDebug("Creating Talk for %s", opponent.name.toLatin1().constData());
	ui.setupUi(this);

	// create a new tab
	QString s = "MultiLineEdit1_" + QString::number(++counter);
	ui.MultiLineEdit1->setObjectName(s.toAscii()) ;
  
//	MultiLineEdit1->setCurrentFont(setting->fontComments); 

	s = "LineEdit1_" + QString::number(++counter);
	ui.LineEdit1->setObjectName(s.toAscii());
//	LineEdit1->setFont(setting->fontComments);
/*
	// do not add a button for shouts* or channels tab
	if ( (name.find('*') != -1) || (!isplayer))
	{

		delete pb_releaseTalkTab;
		delete pb_match;
		delete stats_layout;
	}
*/
	connect (ui.pb_releaseTalkTab, SIGNAL(pressed()),SLOT(slot_pbRelTab()));
	connect (ui.pb_match, SIGNAL(pressed()),SLOT(slot_match()));
}

Talk::~Talk()
{
	if(dispatch)
		dispatch->closeDispatchFromDialog();
}

QString Talk::get_name() const
{ return opponent.name; }

// release current Tab
void Talk::slot_pbRelTab()
{
	emit signal_pbRelOneTab( this);	
}

void Talk::slot_returnPressed()
{
	// read tab
	QString txt = ui.LineEdit1->text();
	dispatch->sendTalk(txt);
	QString our_name = dispatch->getUsername();
	ui.MultiLineEdit1->append(our_name + ": " + txt);
	ui.LineEdit1->clear();
}

void Talk::slot_match()
{
	dispatch->sendMatchInvite(opponent);
}

// write to txt field in dialog
// if null string -> check edit field
void Talk::write(const QString &text) const
{
	/* FIXME what is this for??? */
	qDebug("Talk::write");
	QString txt;

	// check which text to display
	if (!text.isEmpty())
		// ok, text given
		txt = text;

	else if (!ui.LineEdit1->text().isEmpty())
	{
		// take txt of edit field
		txt = ui.LineEdit1->text();
		ui.LineEdit1->clear();
	}
	else
	{
		// no text found...
		return;
	}

	// Scroll at bottom of text, set cursor to end of line
	ui.MultiLineEdit1->append(txt); 
}

/* FIXME We apparently don't need the below since
 * we have a reference but at the same time, we do
 * need to update things for it
 * We could just call this update and have it use the reference.*/
void Talk::updatePlayerListing(void)
{
	ui.stats_rating->setText(opponent.rank);
	ui.stats_info->setText(opponent.info);
	ui.stats_default->setText(opponent.extInfo);
	ui.stats_wins->setText(QString::number(opponent.wins) + " /");
	ui.stats_loss->setText(QString::number(opponent.losses) );
	ui.stats_country->setText(opponent.country);
	ui.stats_playing->setText(QString::number(opponent.playing));
			//ui.stats_rated->setText(opponent.rated);
	ui.stats_address->setText(opponent.email_address);
			
			// stored either idle time or last access	 
	ui.stats_idle->setText(opponent.idletime);
	if (!opponent.idletime.isEmpty())
		ui.Label_Idle->setText(opponent.idletime.at(0).isDigit() ? "Idle :": "Last log :");
}

void Talk::setTalkWindowColor(QPalette pal)
{
	ui.MultiLineEdit1->setPalette(pal);
	ui.LineEdit1->setPalette(pal);
}

