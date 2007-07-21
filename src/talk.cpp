
/*
 *   Talk - Class to handle  Talk Dialog Windows
 */

#include "talk.h"


int Talk::counter = 0;

Talk::Talk(const QString &playername, QWidget */*  parent*/ , bool /* isplayer*/)
	: TalkGui()
{
	ui.setupUi(this);

	name = playername;

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
}

// release current Tab
void Talk::slot_pbRelTab()
{
	emit signal_pbRelOneTab( this);	
}

void Talk::slot_returnPressed()
{
	// read tab
	QString txt = ui.LineEdit1->text();
	emit signal_talkTo(name, txt);
	ui.LineEdit1->clear();
}

void Talk::slot_match()
{
	QString txt= name+ " " + ui.stats_rating->text();
	emit signal_matchRequest(txt,true);
}



// write to txt field in dialog
// if null string -> check edit field
void Talk::write(const QString &text) const
{
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

void Talk::setTalkWindowColor(QPalette pal)
{
	ui.MultiLineEdit1->setPalette(pal);
	ui.LineEdit1->setPalette(pal);
}

