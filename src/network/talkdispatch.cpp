#include "talkdispatch.h"
#include "roomdispatch.h"
#include "../talk.h"
#include "playergamelistings.h"

TalkDispatch::TalkDispatch(NetworkConnection * c, const PlayerListing & opp) : opponent(opp)
{
	connection = c;
	dlg = new Talk(this, opponent); 
	room = 0;
}

TalkDispatch::TalkDispatch(NetworkConnection * c, const PlayerListing & opp, RoomDispatch * r) : opponent(opp)
{
	connection = c;
	//dlg = 0;
	dlg = new Talk(this, opponent); 
	r->talkOpened(this);
	room = r;
	/* Request stats */
	connection->sendStatsRequest(opponent);
}

TalkDispatch::~TalkDispatch()
{
	if(dlg)
	{
		dlg->setDispatch(0);
		delete dlg;
	}
}

void TalkDispatch::closeDispatchFromDialog(void)
{
	dlg = 0;
	connection->closeTalkDispatch(opponent);
}

void TalkDispatch::sendTalk(QString text)
{
	connection->sendMsg(opponent, text);
}

void TalkDispatch::recvTalk(QString text)
{
	/* This will initially have double opponent names
	 * but I think we want to do it this way... maybe
	 * so that the connection doesn't have to put the
	 * name in the message if its not there... but
	 * we might always want to do that? FIXME */
	dlg->write(opponent.name + ": " + text);
}

void TalkDispatch::updatePlayerListing(void)
{
	dlg->updatePlayerListing();
}

Talk * TalkDispatch::getDlg(void)
{
	return dlg;
}
