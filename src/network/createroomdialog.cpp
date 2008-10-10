#include <QtGui>
#include "createroomdialog.h"
#include "networkconnection.h"

CreateRoomDialog::CreateRoomDialog(NetworkConnection * conn) : connection(conn)
{
	ui.setupUi(this);
	ui.roomTypeTab->removeTab(5);			//specialty versions
	ui.roomTypeTab->setTabEnabled(3, false);	//review
	ui.roomTypeTab->setTabEnabled(4, false);	//multi
	
	/*namelabel = new QLabel(tr("%1 %2").arg(name).arg(rank));
	dialoglabel = new QLabel(tr("wants to play a match..."));
	timelabel = new QLabel(tr("%1 seconds").arg(seconds));
	
	createButton = new QPushButton(tr("&Create"));
	createButton->setDefault(true);
	
	cancelButton = new QPushButton(tr("&Cancel"));
	
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(createButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(cancelButton, QDialogButtonBox::ActionRole);
	
	connect(createButton, SIGNAL(clicked()), this, SLOT(slot_create()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancel()));
	
	QGridLayout * mainLayout = new QGridLayout;
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(namelabel, 0, 0);
	mainLayout->addWidget(dialoglabel, 1, 0);
	mainLayout->addWidget(timelabel, 2, 0);
	mainLayout->addWidget(buttonBox, 3, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Create Room"));*/
	connect(ui.createButton, SIGNAL(clicked()), this, SLOT(slot_create()));
	connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancel()));
	
	connect(ui.privateCB, SIGNAL(clicked(bool)), this, SLOT(slot_privateCB(bool)));
	
	connect(ui.roomTypeTab, SIGNAL(currentChanged(int)), SLOT(slot_roomTypeTab(int)));
	
	connect(ui.opponentStrongerRB, SIGNAL(clicked()), this, SLOT(slot_opponentStrongerRB()));
	connect(ui.opponentEvenRB, SIGNAL(clicked()), this, SLOT(slot_opponentEvenRB()));
	connect(ui.opponentWeakerRB, SIGNAL(clicked()), this, SLOT(slot_opponentWeakerRB()));
	connect(ui.opponentAnyRB, SIGNAL(clicked()), this, SLOT(slot_opponentAnyRB()));
	
	connect(ui.timeQuickRB, SIGNAL(clicked()), this, SLOT(slot_timeQuickRB()));
	connect(ui.timeNormalRB, SIGNAL(clicked()), this, SLOT(slot_timeNormalRB()));
	connect(ui.timePonderousRB, SIGNAL(clicked()), this, SLOT(slot_timePonderousRB()));
	connect(ui.timeAnyRB, SIGNAL(clicked()), this, SLOT(slot_timeAnyRB()));
	
	connect(ui.oneOnOneRB, SIGNAL(clicked()), this, SLOT(slot_oneOnOneRB()));
	connect(ui.pairRB, SIGNAL(clicked()), this, SLOT(slot_pairRB()));
	ui.pairRB->setEnabled(false);
	
	
	connect(ui.teachingRB, SIGNAL(clicked()), this, SLOT(slot_teachingRB()));
	connect(ui.liveRB, SIGNAL(clicked()), this, SLOT(slot_liveRB()));
}

void CreateRoomDialog::slot_privateCB(bool checked)
{
	ui.passwordLabel->setEnabled(checked);
	ui.passwordEdit->setEnabled(checked);
	if(!checked)
		ui.passwordEdit->clear();
}
		
void CreateRoomDialog::slot_roomTypeTab(void)
{
}
void CreateRoomDialog::slot_opponentStrongerRB(void)
{
}
void CreateRoomDialog::slot_opponentEvenRB(void)
{
}
void CreateRoomDialog::slot_opponentWeakerRB(void)
{
}
void CreateRoomDialog::slot_opponentAnyRB(void)
{
}

void CreateRoomDialog::slot_timeQuickRB(void)
{
}
void CreateRoomDialog::slot_timeNormalRB(void)
{
}
void CreateRoomDialog::slot_timePonderousRB(void)
{
}
void CreateRoomDialog::slot_timeAnyRB(void)
{
}
void CreateRoomDialog::slot_oneOnOneRB(void)
{
}
void CreateRoomDialog::slot_pairRB(void)
{
}
void CreateRoomDialog::slot_teachingRB(void)
{
}
void CreateRoomDialog::slot_liveRB(void)
{
}

CreateRoomDialog::~CreateRoomDialog()
{
	
}

void CreateRoomDialog::slot_create(void)
{
	done(0);		//FIXME
	RoomCreate * room = new RoomCreate();
	/* FIXME This could be problematic if the index
	 * can change when we remove tabs, but that's
	 * a ways away right now.*/
	room->type = (RoomCreate::roomType)ui.roomTypeTab->currentIndex();
	switch(ui.roomTypeTab->currentIndex())
	{
		case RoomCreate::GAME:
			break;
		case RoomCreate::GOMOKU:
			break;
		case RoomCreate::CHAT:
			break;
		case RoomCreate::REVIEW:
			done(0);
			break;
		case RoomCreate::MULTI:
			done(0);
			break;
		case RoomCreate::VARIATION:
			done(0);
			break;
	}
	connection->sendCreateRoom(room);
	done(1);
}

void CreateRoomDialog::slot_cancel(void)
{
	done(0);
}
