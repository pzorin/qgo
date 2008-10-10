#include "gamedialogdispatch.h"
#include "../gamedialog.h"

GameDialogDispatch::GameDialogDispatch(NetworkConnection * c, const PlayerListing & opp) : opponent(opp)
{
	connection = c;
	dlg = 0;
}

GameDialogDispatch::~GameDialogDispatch()
{
	qDebug("Deconstructing GameDialogDispatch");
	if(dlg)
	{
		dlg->setDispatch(0);	//so that the dialog won't try to delete us
		delete dlg;
	}
}

void GameDialogDispatch::closeDispatchFromDialog(void)
{
	qDebug("closing dispatch from dialog");
	dlg = 0;
	connection->closeGameDialogDispatch(opponent);	//this deletes this, little counterintuitive
}

/* As with sendRequest, we may already have a dialog open */
void GameDialogDispatch::recvRequest(MatchRequest * mr, unsigned long flags)
{
	if(!dlg)
		dlg = new GameDialog(this);
	dlg->recvRequest(mr, flags);
}

void GameDialogDispatch::recvRefuseMatch(int motive)
{
	if(dlg)
		dlg->recvRefuseMatch( motive);
}

/* Used for both changing open match dialogs as well as
 * offering new.*/
void GameDialogDispatch::sendRequest(MatchRequest * mr)
{
	connection->sendMatchRequest(mr);	
}

void GameDialogDispatch::declineOffer()
{
	connection->declineMatchOffer(opponent);
}

void GameDialogDispatch::cancelOffer()
{
	connection->cancelMatchOffer(opponent);
}

/* I think we can just use sendRequest instead of acceptOffer, with the
 * idea that both sides are checking to make sure nothing has changed */
void GameDialogDispatch::acceptOffer(MatchRequest * mr)
{
	connection->acceptMatchOffer(opponent, mr);
}

MatchRequest * GameDialogDispatch::getMatchRequest(void)
{
	if(dlg)
		return dlg->getMatchRequest();
	else
		return 0;
}
