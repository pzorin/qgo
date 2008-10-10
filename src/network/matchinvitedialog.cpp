#include <QtGui>
#include "matchinvitedialog.h"

MatchInviteDialog::MatchInviteDialog(QString name, QString rank)
{
	seconds = 20;		//does it?
	
	/* FIXME, we need to center this text, make the font larger, or
	 * bold, etc.. */
	namelabel = new QLabel(tr("%1 %2").arg(name).arg(rank));
	dialoglabel = new QLabel(tr("wants to play a match..."));
	timelabel = new QLabel(tr("%1 seconds").arg(seconds));
	
	acceptButton = new QPushButton(tr("&Accept"));
	acceptButton->setDefault(true);
	
	declineButton = new QPushButton(tr("&Decline"));
	
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(acceptButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(declineButton, QDialogButtonBox::ActionRole);
	
	connect(acceptButton, SIGNAL(clicked()), this, SLOT(slot_accept()));
	connect(declineButton, SIGNAL(clicked()), this, SLOT(slot_decline()));
	
	QGridLayout * mainLayout = new QGridLayout;
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(namelabel, 0, 0);
	mainLayout->addWidget(dialoglabel, 1, 0);
	mainLayout->addWidget(timelabel, 2, 0);
	mainLayout->addWidget(buttonBox, 3, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Match Invite!"));
	
	startTimer(1000);
}

void MatchInviteDialog::timerEvent(QTimerEvent*)
{
	seconds--;
	if(seconds == -1)
		close();	//does this return 0?
	timelabel->setText(tr("%1 seconds").arg(seconds));
}

MatchInviteDialog::~MatchInviteDialog()
{
	delete acceptButton;
	delete declineButton;
	delete buttonBox;
}

/* I'm assuming here that a close by time returns 0 */
void MatchInviteDialog::slot_accept(void)
{
	done(1);
}

void MatchInviteDialog::slot_decline(void)
{
	done(-1);
}
