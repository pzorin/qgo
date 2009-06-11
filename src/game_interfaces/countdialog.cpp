#include <QtGui>
#include "../network/messages.h"
#include "countdialog.h"
#include "board/boardwindow.h"
#include "../network/boarddispatch.h"

CountDialog::CountDialog(BoardWindow * parent, BoardDispatch * dis, unsigned int game_id) 
	: QDialog((QDialog *)parent), dispatch(dis), board(parent)
{
	oppAcceptsCount = 0;
	oppRejectsCount = 0;
	weAcceptCount = 0;
	weRejectCount = 0;
	
	acceptButton = new QPushButton(tr("&Accept"));
	//acceptButton->setDefault(true);
	rejectButton = new QPushButton(tr("&Reject"));
	
	if(!dispatch)
	{
		qDebug("CountDialog called without board dispatch");
		deleteLater();
		return;
	}
	result = new GameResult(board->getBoardHandler()->retrieveScore());
	GameData * gamedata = board->getGameData();
	if(result->winner_color == stoneWhite)
	{
		result->winner_name = gamedata->white_name;
		result->loser_name = gamedata->black_name;
	}
	else
	{
		result->winner_name = gamedata->black_name;
		result->loser_name = gamedata->white_name;
	}
	
	mainlabel = new QLabel(result->longMessage() + 
			      	"\nWhite has " + QString::number(gamedata->white_prisoners)
				 + " captures\nBlack has " + 
			         QString::number(gamedata->black_prisoners) + " captures");
	/* These needs to also post black and white captures and ultimate score
	 * FIXME */
	
	buttonBox = new QDialogButtonBox(Qt::Horizontal);
	buttonBox->addButton(acceptButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(rejectButton, QDialogButtonBox::ActionRole);
	
	connect(acceptButton, SIGNAL(clicked()), this, SLOT(slot_accept()));
	connect(rejectButton, SIGNAL(clicked()), this, SLOT(slot_reject()));
	
	QGridLayout * mainLayout = new QGridLayout;
	/*mainLayout->setSizeConstraint(QLayout::SetFixedSize);*/
	mainLayout->addWidget(mainlabel, 0, 0);
	mainLayout->addWidget(buttonBox, 1, 0);
	setLayout(mainLayout);

	
	setWindowTitle(QString::number(game_id) + tr(": Accept result?"));
	//setSizePolicy(QSizePolicy::Minimum);
	// FIXME size?
	resize(200, 100);
	//setMinimumSize(QSize(300, 150));
	show();
}

CountDialog::~CountDialog()
{
	dispatch->clearCountDialog();
	delete acceptButton;
	delete rejectButton;
	delete mainlabel;
	delete buttonBox;
	delete result;
}

void CountDialog::recvRejectCount()
{
	oppRejectsCount = 1;
	if(weRejectCount || weAcceptCount)
	{
		/* Regardless of what happens, I think
		 * we have to restart the game, get back into it
		 * this may require sending clear stone messages
		 * or some other messsage and it definitely
		 * requires leaving score mode and starting
		 * any timers that may have stopped in addition
		 * to possibly deleting "pass" moves from the tree
		 * FIXME */
		board->qgoboard->leaveScoreMode();
		deleteLater();
	}
}

void CountDialog::recvAcceptCount()
{
	oppAcceptsCount = 1;
	if(weRejectCount)
	{
		/* See above comment FIXME */
		board->qgoboard->leaveScoreMode();
		deleteLater();
	}
	else if(weAcceptCount)
	{
		/* This means we notify network connection
		 * to send any necessary game result messages FIXME */
		dispatch->recvResult(result);
		dispatch->sendResult(result);
		//board->qgoboard->setResult(*result);
		deleteLater();
	}
}

/* We're not going to delete the countdialog immediately
 * but instead let it keep track of things... I don't
 * really know why... I guess because this is a dialog
 * for counting and its light so why not have it do
 * something besides displaying info and a couple
 * buttons */
void CountDialog::slot_accept(void)
{
	weAcceptCount = 1;
	dispatch->sendAcceptCount();	
	hide();
	if(oppRejectsCount)
	{
		/* See above comment FIXME */
		board->qgoboard->leaveScoreMode();
		deleteLater();
	}
	else if(oppAcceptsCount)
	{
		/* See above comment FIXME */
		dispatch->recvResult(result);
		dispatch->sendResult(result);
		//board->qgoboard->setResult(*result);
		deleteLater();
	}
}

void CountDialog::slot_reject(void)
{
	weRejectCount = 1;
	dispatch->sendRejectCount();	
	hide();
	if(oppRejectsCount || oppAcceptsCount)
	{
		/* See above comment FIXME */
		board->qgoboard->leaveScoreMode();
		deleteLater();
	}
}
