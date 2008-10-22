#include "room.h"
#include "listviews.h"
#include "network/roomdispatch.h"
#include "network/talkdispatch.h"
#include "network/messages.h"
#include "network/gamedialogdispatch.h"
#include "mainwindow.h"
#include "network/playergamelistings.h"

Room::Room()
{
	playerView = mainwindow->getUi()->playerView;
	gamesView = mainwindow->getUi()->gamesView;
	refreshPlayersButton = mainwindow->getUi()->pbRefreshPlayers;
	refreshGamesButton = mainwindow->getUi()->pbRefreshGames;
	whoBox1 = mainwindow->getUi()->whoBox1;
	whoBox2 = mainwindow->getUi()->whoBox2;
	whoOpenCheckBox = mainwindow->getUi()->whoOpenCheck;
	/* Normally the dispatch and the UI are created at the sametime
	* by the connection or dispatch code.  In this case,
	* the UI already exists and is being passed straight in here,
	* so we'll do things backwards and have this create the dispatch */
	/* UI MUST be created before dispatch.  FIXME
	* This is an ugly non-apparent dependency because
	* the dispatch registries depend on the UI's models */
	/* There are further strangenesses here because of the account_name
	* for making our player listing blue */
	setupUI();
	dispatch = new RoomDispatch(this);
	
	players = 0;
	games = 0;
}

void Room::setupUI(void)
{
	qDebug("Setting up Room UI");
	gamesListModel = new GamesListModel();

	/*ui.ListView_games->header()->setSortIndicatorShown ( FALSE );
	ui.ListView_games->hideColumn(12);
	ui.ListView_games->hideColumn(13);*/
	gamesView->setModel(gamesListModel);
	/* Justifications??? */
	/* No sort indicator??? */
	/* Qt 4.4.1 made sortIndicatorShown necesssary for sort behavior
	 * !!!! */
	gamesView->header()->setSortIndicatorShown ( true );
	gamesView->setSelectionBehavior(QAbstractItemView::SelectRows);
	/*ui.gamesView->setColumnWidth ( 0, 40 );	//35
	ui.gamesView->setColumnWidth ( 1, 100 );
	ui.gamesView->setColumnWidth ( 2, 48 );	//35
	ui.gamesView->setColumnWidth ( 3, 100 );
	ui.gamesView->setColumnWidth ( 4, 48 );	//35
	ui.gamesView->setColumnWidth ( 5, 45 );	//30
	ui.gamesView->setColumnWidth ( 6, 45 ); //25
	ui.gamesView->setColumnWidth ( 7, 38 ); //20
	ui.gamesView->setColumnWidth ( 8, 35 ); //30
	ui.gamesView->setColumnWidth ( 9, 43 ); //25
	ui.gamesView->setColumnWidth ( 10, 60 ); //20
	ui.gamesView->setColumnWidth ( 11, 55 ); //25
	*/
	gamesView->setColumnWidth ( 0, 35 );	//35
	gamesView->setColumnWidth ( 1, 100 );
	gamesView->setColumnWidth ( 2, 35 );	//35
	gamesView->setColumnWidth ( 3, 100 );
	gamesView->setColumnWidth ( 4, 35 );	//35
	gamesView->setColumnWidth ( 5, 30 );	//30
	gamesView->setColumnWidth ( 6, 30 ); //25
	gamesView->setColumnWidth ( 7, 20 ); //20
	gamesView->setColumnWidth ( 8, 30 ); //30
	gamesView->setColumnWidth ( 9, 35 ); //25
	gamesView->setColumnWidth ( 10, 35 ); //20
	gamesView->setColumnWidth ( 11, 30 ); //25
	//ui.gamesView->show();
	
	playerSortProxy = new PlayerSortProxy();
	playerListModel = new PlayerListModel();
	playerSortProxy->setSourceModel(playerListModel);
	//playerSortProxy->setDynamicSortFilter(true);
	
	playerView->setModel(playerSortProxy);
	playerView->header()->setSortIndicatorShown ( true );
	playerView->setColumnWidth ( 0, 40 );
	playerView->setColumnWidth ( 1, 100 );
	playerView->setColumnWidth ( 2, 40 );
	playerView->setColumnWidth ( 3, 40 );
	playerView->setColumnWidth ( 4, 40);
	playerView->setColumnWidth ( 5, 40 );
	playerView->setColumnWidth ( 6, 40 );
	playerView->setColumnWidth ( 7, 40 );
	playerView->setColumnWidth ( 8, 80 );
	/*ui.ListView_players->hideColumn(6);
	ui.ListView_players->hideColumn(7);
	ui.ListView_players->hideColumn(8);
	ui.ListView_players->hideColumn(9);
	ui.ListView_players->hideColumn(12);
	ui.ListView_players->setColumnWidth ( 0, 30 );
	ui.ListView_players->setColumnWidth ( 1, 100 );
	ui.ListView_players->setColumnWidth ( 2, 30 );
	ui.ListView_players->setColumnWidth ( 3, 30 );
	ui.ListView_players->setColumnWidth ( 4, 30);
	ui.ListView_players->setColumnWidth ( 5, 30 );
//	ui.ListView_players->setColumnWidth ( 6, 80 );
	ui.ListView_players->setColumnWidth ( 7, 80 );
//	ui.ListView_players->setColumnWidth ( 8, 30 );
//	ui.ListView_players->setColumnWidth ( 9, 25 );
	ui.ListView_players->setColumnWidth ( 10, 50 );
//	ui.ListView_players->setColumnWidth ( 11, 25 );*/

	/* Maybe also a "clicked" for something? */
	connect(playerView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playersDoubleClicked(const QModelIndex &)));
	connect(playerView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopup(const QPoint &)));
	connect(gamesView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_gamesDoubleClicked(const QModelIndex &)));
	connect(gamesView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showGamesPopup(const QPoint &)));
	connect(refreshPlayersButton, SIGNAL(pressed()), SLOT(slot_refreshPlayers()));
	connect(refreshGamesButton, SIGNAL(pressed()), SLOT(slot_refreshGames()));
	connect(whoBox1, SIGNAL(currentIndexChanged(int)), SLOT(slot_setRankSpreadView()));
	connect(whoBox2, SIGNAL(currentIndexChanged(int)), SLOT(slot_setRankSpreadView()));
	connect(whoOpenCheckBox, SIGNAL(stateChanged(int)), SLOT(slot_showOpen(int)));
}

Room::~Room()
{
	delete dispatch;
	
	/* FIXME: Should probably be part of something else this P G stuff */
	mainwindow->statusUsers->setText(" P: 0");
	mainwindow->statusGames->setText(" G: 0");
	//mainwindow->ui.changeServerPB->hide();	//done in mainwindow_server
	//mainwindow->ui.createRoomPB->hide();
	
	/* If this was a stand alone room, we'd also destroy the UI
	 * here, I just want to clear the lists */
	qDebug("Deconstructing room");
	playerView->setModel(0);
	gamesView->setModel(0);
	delete playerListModel;
	delete playerSortProxy;
	delete gamesListModel;
}

void Room::onError(void)
{
	/* We're going to call this straight from the network dispatch
	 * until we work out any room UI issues */
	//mainwindow->onConnectionError();
}

void Room::onConnectionAssignment(QString username)
{
	playerListModel->setAccountName(username);	
	// FIXME, what about observerListModels when they come up on boards?
	unsigned long flags = dispatch->getPlayerListColumns();
	if(flags & PL_NOWINSLOSSES)
	{
		playerView->hideColumn(5);
		playerView->hideColumn(6);
	}
	if(flags & PL_NOMATCHPREFS)
	{
		playerView->hideColumn(9);	
	}
}

void Room::updateRoomStats(void)
{
	players = playerListModel->rowCount(QModelIndex());
	games = gamesListModel->rowCount(QModelIndex());
	
	//qDebug("%d %d", players, games);
	mainwindow->statusUsers->setText(" P: " + QString::number(players));
	mainwindow->statusGames->setText(" G: " + QString::number(games));
}

//stats
void Room::slot_playersDoubleClicked(const QModelIndex & index)
{
	QModelIndex translated = playerSortProxy->mapToSource(index);
	const PlayerListing * opponent = playerListModel->playerListingFromIndex(translated);
	dispatch->sendStatsRequest(*opponent);
}

void Room::slot_showPopup(const QPoint & iPoint)
{
	popup_item = playerView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
    		QMenu menu(playerView);
    		menu.addAction(tr("Match"), this, SLOT(slot_popupMatch()));
		menu.addAction(tr("Talk"), this, SLOT(slot_popupTalk()));
    		menu.exec(playerView->mapToGlobal(iPoint));
	}
}

void Room::slot_showGamesPopup(const QPoint & iPoint)
{
	if(!dispatch)
		return;
	if(!dispatch->supportsObserveOutside())
		return;
	popup_item = gamesView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
		/* Do not give options on rooms without games */
		const GameListing * g = gamesListModel->gameListingFromIndex(popup_item);
		if(g->isRoomOnly)
			return;
		QMenu menu(gamesView);
		if(preferences.observe_outside_on_doubleclick)
		{
			menu.addAction(tr("Observe Outside"), this, SLOT(slot_popupObserveOutside()));
			menu.addAction(tr("Join and Observe"), this, SLOT(slot_popupJoinObserve()));
		}
		else
		{
			menu.addAction(tr("Join and Observe"), this, SLOT(slot_popupJoinObserve()));
			menu.addAction(tr("Observe Outside"), this, SLOT(slot_popupObserveOutside()));
		}
		
		menu.exec(gamesView->mapToGlobal(iPoint));
	}
}

void Room::slot_popupMatch(void)
{
	QModelIndex translated = playerSortProxy->mapToSource(popup_item);
	const PlayerListing * listing = playerListModel->playerListingFromIndex(translated);
	
	dispatch->sendMatchInvite(*listing);
}

void Room::slot_popupTalk(void)
{
	slot_playersDoubleClicked(popup_item);
}

void Room::slot_popupObserveOutside(void)
{
	const GameListing * g = gamesListModel->gameListingFromIndex(popup_item);
	dispatch->sendObserveOutside(*g);
}

void Room::slot_popupJoinObserve(void)
{
	const GameListing * g = gamesListModel->gameListingFromIndex(popup_item);
	dispatch->sendObserve(*g);
}

//observe
void Room::slot_gamesDoubleClicked(const QModelIndex & index)
{
	const GameListing * g = gamesListModel->gameListingFromIndex(index);
	if(preferences.observe_outside_on_doubleclick &&
		  dispatch->supportsObserveOutside() && !g->isRoomOnly)
		dispatch->sendObserveOutside(*g);
	else
		dispatch->sendObserve(*g);
}

void Room::slot_refreshPlayers(void)
{
	clearPlayerList();
	dispatch->sendPlayersRequest();
}

void Room::clearPlayerList(void)
{
	playerListModel->clearList();
}

void Room::clearGamesList(void)
{
	gamesListModel->clearList();
}

void Room::slot_refreshGames(void)
{
	/* First clear list.  There doesn't
	* seem to be a good way to update it
	* or update everything.  Since we're
	* getting all info again anyway... 
	* same with players and perhaps 
	* observers as well. */
	clearGamesList();
	dispatch->sendGamesRequest();
}

/* This code was taken from mainwindow_server.cpp
 * I guess it figures out which is min and which
 * is max?? */
void Room::slot_setRankSpreadView(void)
{
	QString rkMin, rkMax;
	QString r1,r2;

	if ((whoBox1->currentIndex() == 0) && (whoBox2->currentIndex() == 0))
	{
		rkMin = "NR";
		rkMax = "9p";
	}

	else if ( ((whoBox1->currentIndex() == 0) && (whoBox2->currentIndex() == 1)) ||
		((whoBox1->currentIndex() == 1) && (whoBox2->currentIndex() == 0))  ||
		((whoBox1->currentIndex() == 1) && (whoBox2->currentIndex() ==1)) )
	{
		rkMin = "1p";
		rkMax = "9p";
	}	

	else if ((whoBox1->currentIndex() == 0) && (whoBox2->currentIndex() > 1))
	{
		rkMin = whoBox2->currentText();
		rkMax = whoBox2->currentText();
	}	


	else if ((whoBox1->currentIndex() > 1) && (whoBox2->currentIndex() == 0))
	{
		rkMin = whoBox1->currentText();
		rkMax = whoBox1->currentText();
	}	

	else if ((whoBox1->currentIndex() > 1) && (whoBox2->currentIndex() == 1))
	{
		rkMin = whoBox1->currentText();
		rkMax = "9p";
	}	

	else if ((whoBox1->currentIndex() == 1) && (whoBox2->currentIndex() > 1))
	{
		rkMin = whoBox2->currentText();
		rkMax = "9p";
	}


	else if ((whoBox2->currentIndex() >= whoBox1->currentIndex() ))
	{
		rkMin = whoBox2->currentText();
		rkMax = whoBox1->currentText();
	} 
	else 
	{
		rkMin = whoBox1->currentText();
		rkMax = whoBox2->currentText();
	} 

	playerSortProxy->setFilter(dispatch->rankToScore(rkMin), dispatch->rankToScore(rkMax));
	qDebug( "rank spread : %s - %s" , rkMin.toLatin1().constData() , rkMax.toLatin1().constData());
}

void Room::slot_showOpen(int state)
{
	playerSortProxy->setFilter(state);
}

/* These two may want to check that they are the mainwindowroom or the
 * default room or whatever.  But its not necessary to have a whole
 * other subclass for mainwindowrooms because they both need some
 * kind of ui.  It would be more effective to do something with the
 * ui classes */
void Room::recvExtPlayerListing(class PlayerListing * player)
{
	mainwindow->slot_statsPlayer(player);
}

void Room::talkOpened(TalkDispatch * d)
{
	Talk * dlg = d->getDlg();
	mainwindow->talkOpened(dlg);
}

void Room::recvToggle(int type, bool val)
{
	mainwindow->slot_checkbox(type, val);
}

