/*
 *   qgo_interface.cpp -  interface to server
 */


#include "mainwindow.h"
#include "qgo_interface.h"


/*
 *	Playing or Observing
 */

qGoIF::qGoIF(QWidget *p) : QObject()
{

	parent = p;

	gsName = GS_UNKNOWN;
	boardlist = new QHash< int, BoardWindow*>;

	localBoardCounter = 10000;

	// init sound (all sounds!)
//	qgo->loadSound();
}

qGoIF::~qGoIF()
{
//	delete boardlist;
//	delete qgo;
}

/*
 * Creates a game board with all elements initialised from 
 * Gamedata and an sgf file 
 */
void qGoIF::createGame(GameMode _gameMode, GameData * _gameData, bool _myColorIsBlack , bool _myColorIsWhite )
{
	//local game ? set the game id accordingly
	if (_gameMode == modeNormal || _gameMode == modeComputer)
		_gameData->gameNumber = ++localBoardCounter;

	BoardWindow *b = new BoardWindow(parent, 0 , _gameData , _gameMode , _myColorIsBlack , _myColorIsWhite);
	
	b->show();
	
	//just in case. This should not happen but one never knows
	boardlist->remove(_gameData->gameNumber);

	boardlist->insert(_gameData->gameNumber, b);

	connect (b , SIGNAL(signal_boardClosed(int)) , SLOT(slot_boardClosed(int)));

//	if (_gameMode == modeObserve || _gameMode == modeMatch || _gameMode == modeTeach || _gameMode == modeReview)
//		connect(b->qgoboard, SIGNAL(signal_sendCommandFromBoard(const QString&, bool)), SLOT(slot_sendCommandFromInterface(const QString&, bool)));

}


/*
 * The board with game number 'i' has been closed
 * we remove it from the list.
 */
void qGoIF::slot_boardClosed(int n)
{

	BoardWindow *bw = getBoardWindow(n);

	if (boardlist->remove(n) == 0) 
		qDebug("Problem removing the board with Id :%d from the list - Id not found",n);

	// if this was an observed game, we have to replace by a null pointer in the 
	// list so that any incoming move is ignored before the server cancels sending moves
	if ( bw && bw->getGameMode()==modeObserve)
	{
		emit signal_sendCommandFromInterface("observe " + QString::number(n), FALSE);
		boardlist->insert(n, NULL);
	}
}



/*
 * a game information (move or time info) has been received and is sent by parser
 */
void qGoIF::slot_move(GameInfo* gi)
{
	int game_id = gi->nr.toInt();

	BoardWindow *bw = getBoardWindow(game_id);

	// If we do not have a board window with the game Id, create it
	// we must take care of a null game being in the list (observed game waiting to be closed)
	if ( bw == NULL && ! boardlist->contains(game_id) )
	{
		GameData *gd = new GameData();
		gd->gameNumber = gi->nr.toInt();
		gd->playerBlack = gi->bname;
		gd->playerWhite = gi->wname;
				
		createGame(modeObserve, gd, FALSE, FALSE );

		return ;
	}

		//if the initials commands (getting all moves) has not been sent, discard
	if (bw->getGamePhase() == phaseInit)
		return;

 	StoneColor  sc = stoneNone;
	QString     pt;
	QString     mv_nr;

	if (gi->mv_col == QString("B"))
		sc = stoneBlack;
	else if (gi->mv_col == QString("W"))
		sc = stoneWhite;
	else
		sc = stoneNone;
	mv_nr = gi->mv_nr;
	pt = gi->mv_pt;
	

	if (gi->mv_col == "T")
	{
		// set times
//		qgobrd->setTimerInfo(gi->btime, gi->bstones, gi->wtime, gi->wstones);
	}
	else if (gi->mv_col == "B" || gi->mv_col == "W")
	{
//		// set move if game is initialized
//		if (qgobrd->get_havegd())
//		{
//			// get all moves till now one times if kibitzing
//			if (qgobrd->get_Mode() == modeObserve && !qgobrd->get_sentmovescmd())
//			{
//				qgobrd->set_sentmovescmd(true);
//				emit signal_sendcommand("moves " + gi->nr, false);
//				emit signal_sendcommand("all " + gi->nr, false);
//			}
//			else
//			{
				// do normal move
				bw->qgoboard->set_move(sc, pt, mv_nr);
//			}
//		}
	}
}



/*
 * a game info has been received from parser (games list format)
 * This happens when observing a game, when 'games ##' command is issued
 */
void qGoIF::slot_gameInfo(Game *g)
{
	BoardWindow *bw = getBoardWindow(g->nr.toInt());

	if (!bw)
		return;

	GameData *gd = new GameData();

	gd->playerBlack = g->bname;
	gd->playerWhite = g->wname;
	gd->rankBlack = g->brank;
	gd->rankWhite = g->wrank;
//	QString status = g->Sz.simplifyWhiteSpace();
	gd->size = g->Sz.toInt();
	gd->handicap = g->H.toInt();
	gd->komi = g->K.toFloat();
	gd->gameNumber = g->nr.toInt();
	if (g->FR.contains("F"))
		gd->freegame = FREE;
	else if (g->FR.contains("T"))
		gd->freegame = TEACHING;
	else
		gd->freegame = RATED;
	if (gd->freegame != TEACHING)
	{
		gd->timeSystem = canadian;
		gd->byoTime = g->By.toInt()*60;
		gd->byoStones = 25;
//		if (wt_i > 0)
//			gd.timelimit = (((wt_i > bt_i ? wt_i : bt_i) / 60) + 1) * 60;
//		else
//			gd.timelimit = 60;

		if (gd->byoTime == 0)
			// change to absolute
			gd->timeSystem = absolute;
		else
			gd->overtime = "25/" + QString::number(gd->byoTime) + " Canadian";
	}
	
	else
	{
		gd->timeSystem = none;
		gd->byoTime = 0;
		gd->byoStones = 0;
		gd->timelimit = 0;
	}
	gd->style = 1;
	gd->oneColorGo = g->oneColorGo ;
  	
	switch (gsName)
	{
		case IGS :
			gd->place = "IGS";
			break ;
		case LGS :
			gd->place = "LGS";
			break ;
		case WING :
			gd->place = "WING";
			break ;
		case CWS :
			gd->place = "CWS";
			break;
		default :
			;
	}

	gd->date = QDate::currentDate().toString("dd MM yyyy") ;
	
//	setMode();
//	initGame();
//	setMode();
//	win->getInterfaceHandler()->toggleMode();
//	have_gameData = true;
	bw->qgoboard->set_havegd(TRUE);
	// needed for correct sound
	bw->qgoboard->set_statedMoveCount(g->mv.toInt());
	bw->setGameData(gd);
//	sound = false;
}


/*
// handle move info and distribute to different boards
bool qGoIF::parse_move(int src, GameInfo* gi, Game* g, QString txt)
{
#ifdef SHOW_MOVES_DLG
	static Talk *dlg;
#endif

	int         game_id = 0;
 	StoneColor  sc = stoneNone;
	QString     pt;
	QString     mv_nr;

	switch (src)
	{
		// regular move info (parser)
		case 0:
 			game_id = gi->nr.toInt();
			if (gi->mv_col == QString("B"))
				sc = stoneBlack;
			else if (gi->mv_col == QString("W"))
				sc = stoneWhite;
			else
				sc = stoneNone;
			mv_nr = gi->mv_nr;
			pt = gi->mv_pt;
			break;

		// adjourn/end/resume of a game (parser)
		case 1:
			// check if g->nr is a number
			if (g->nr == QString("@"))
			{
				qGoBoard *qb = boardlist->first();
				while (qb != NULL && qb->get_bplayer() != myName && qb->get_wplayer() != myName)
				{
					qb = boardlist->next();
				}

				if (qb)
					game_id = qb->get_id();
				else
				{
					qWarning("*** You are not playing a game to be adjourned!");
					return false;
				}
			}
			else
				game_id = g->nr.toInt();

			if (g->Sz == QString("@") || g->Sz == QString("@@"))
			{
				// look for adjourned games
				qGoBoard *qb = boardlist->first();
				while (qb != NULL)
				{
					if (qb->get_adj() && qb->get_id() == 10000 &&
					    qb->get_bplayer() == g->bname &&
					    qb->get_wplayer() == g->wname)
					{
						qDebug("ok, adjourned game found ...");
						if (g->bname != myName && g->wname != myName)
							// observe if I'm not playing
							emit signal_sendcommand("observe " + g->nr, false);
						else
							// ensure that my game is correct stated
							emit signal_sendcommand("games " + g->nr, false);
						qb->set_sentmovescmd(true);
						qb->set_id(g->nr.toInt());
						qb->setGameData();
						qb->setMode();
						qb->set_adj(false);
						qb->get_win()->getInterfaceHandler()->toggleMode();
						qb->set_runTimer();
						qb->set_sentmovescmd(true);
						emit signal_sendcommand("moves " + g->nr, false);
						emit signal_sendcommand("all " + g->nr, false);

						qb->send_kibitz(tr("Game continued as Game number %1").arg(g->nr));
						// show new game number;
						qb->get_win()->getBoard()->updateCaption();

						// renew refresh button
						qb->get_win()->getInterfaceHandler()->refreshButton->setText(QObject::tr("Refresh", "button label"));

						// increase number of observed games
						emit signal_addToObservationList(-2);

						return true;
					}

					qb = boardlist->next();
				}
			}

			// special case: your own game
			if (g->Sz == QString("@@"))// || g->bname == myName || g->wname == myName)
			{
				// set up new game
				if (g->bname == g->wname)
					// teaching game
					src = 14;
				else
					// match
					src = 13;
			}
			break;

		// game to observe
		case 2:
			game_id = txt.toInt();
			break;

		// match
		case 3:
			game_id = txt.toInt();
			break;

		// teaching game
		case 4: game_id = txt.toInt();
			break;

		// computer game
		case 6: game_id = ++localBoardCounter; //txt.toInt();
			qDebug(QString("computer game no. %1").arg(game_id));
			break;      

		// remove all boards! -> if connection is closed
		// but: set options for local actions
		case -1:
			qgobrd = boardlist->first();
			while (qgobrd)
			{
				// modeNormal;
				qgobrd->set_stopTimer();
				// enable Menus
//				qgobrd->get_win()->setOnlineMenu(false);

				// set board editable...
				qgobrd->set_Mode(1);
				// toggle score 2x
				qgobrd->get_win()->getInterfaceHandler()->toggleMode();
				qgobrd->get_win()->getInterfaceHandler()->toggleMode();

				boardlist->remove();
				qgobrd = boardlist->current();
			}

			// set number of observed games to 0
			emit signal_addToObservationList(0);
			return false;
			break;

		default:
			qWarning("*** qGoIF::parse_move(): unknown case !!! ***");
			return false;
			break;
	}

#ifdef SHOW_MOVES_DLG
	// dialog recently used?
	if (!dlg)
	{
		dlg = new Talk("SPIEL", parent, true);
		connect(dlg->get_dialog(), SIGNAL(signal_talkto(QString&, QString&)), parent, SLOT(slot_talkto(QString&, QString&)));

		CHECK_PTR(dlg);
		dlg->write("New Dialog created\n");
	}
#endif

	// board recently used?
	if (!qgobrd || qgobrd->get_id() != game_id)
	{
		// seek dialog
		qgobrd = boardlist->first();
		while (qgobrd != NULL && qgobrd->get_id() != game_id)
			qgobrd = boardlist->next();
		
		// not found -> create new dialog
		if (!qgobrd)
		{
			// setup only with mouseclick or automatic in case of matching !!!!
			// added case 0: 'observe' cmd
			if (src < 2) //(src == 1) //
			{
				return false;
			}

			boardlist->append(new qGoBoard(this, qgo));
			qgobrd = boardlist->current();
//			qgobrd->get_win()->setOnlineMenu(true);

			CHECK_PTR(qgobrd);

			// connect with board (MainWindow::CloseEvent())
			connect(qgobrd->get_win(),
				SIGNAL(signal_closeevent()),
				qgobrd,
				SLOT(slot_closeevent()));
			// connect with board (qGoBoard)

			//  board -> to kibitz or say
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_sendcomment(const QString&)),
				qgobrd,
				SLOT(slot_sendcomment(const QString&)));
			// board -> stones
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_addStone(enum StoneColor, int, int)),
				qgobrd,
				SLOT(slot_addStone(enum StoneColor, int, int)));
			// board-> stones (computer game)
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_Stone_Computer(enum StoneColor, int, int)),
				qgobrd,
				SLOT(slot_stoneComputer(enum StoneColor, int, int)));

			connect(qgobrd,
				SIGNAL(signal_2passes(const QString&,const QString& )),
				this,
				SLOT(slot_removestones(const QString&, const QString&)));
            
			connect(qgobrd,
				SIGNAL(signal_sendcommand(const QString&, bool)),
				this,
				SLOT(slot_sendcommand(const QString&, bool)));
			// board -> commands
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_pass()),
				qgobrd,
				SLOT(slot_doPass()));

			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_adjourn()),
				qgobrd,
				SLOT(slot_doAdjourn()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_undo()),
				qgobrd,
				SLOT(slot_doUndo()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_resign()),
				qgobrd,
				SLOT(slot_doResign()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_done()),
				qgobrd,
				SLOT(slot_doDone()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_refresh()),
				qgobrd,
				SLOT(slot_doRefresh()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_editBoardInNewWindow()),
				qgobrd->get_win(),
				SLOT(slot_editBoardInNewWindow()));
			// teach tools
			connect(qgobrd->get_win()->getMainWidget()->cb_opponent,
				SIGNAL(activated(const QString&)),
				qgobrd,
				SLOT(slot_ttOpponentSelected(const QString&)));
			connect(qgobrd->get_win()->getMainWidget()->pb_controls,
				SIGNAL(toggled(bool)),
				qgobrd,
				SLOT(slot_ttControls(bool)));
			connect(qgobrd->get_win()->getMainWidget()->pb_mark,
				SIGNAL(toggled(bool)),
				qgobrd,
				SLOT(slot_ttMark(bool)));

			if (src == 2)
			{
				qgobrd->set_sentmovescmd(true);
				emit signal_sendcommand("games " + txt, false);
				emit signal_sendcommand("moves " + txt, false);
				emit signal_sendcommand("all " + txt, false);
			}

			// for own games send "games" cmd to get full info
			if (src == 3 || src == 4 || src == 13 || src == 14 )//|| src == 0)
			{
				connect(qgobrd->get_win()->getInterfaceHandler()->normalTools->pb_timeBlack,
					SIGNAL(clicked()),
					qgobrd,
					SLOT(slot_addtimePauseB()));
				connect(qgobrd->get_win()->getInterfaceHandler()->normalTools->pb_timeWhite,
					SIGNAL(clicked()),
					qgobrd,
					SLOT(slot_addtimePauseW()));

				if (src == 13 || src == 14 )//|| src== 0)
				{
					// src changed from 1 to 3/4
					qgobrd->set_sentmovescmd(true);
					emit signal_sendcommand("games " + g->nr, false);
					emit signal_sendcommand("moves " + g->nr, false);
					emit signal_sendcommand("all " + g->nr, false);

					if (src == 13)
						src = 3;
					else
						src = 4;
				}
				else
					emit signal_sendcommand("games " + txt, false);

				if (src == 4)
				{
					// make teaching features visible
					qgobrd->get_win()->getMainWidget()->cb_opponent->setEnabled(true);
					qgobrd->get_win()->getMainWidget()->pb_controls->setDisabled(true);
					qgobrd->get_win()->getMainWidget()->pb_mark->setEnabled(true);
					qgobrd->ExtendedTeachingGame = true;
					qgobrd->IamTeacher = true;
					qgobrd->havePupil = false;
					qgobrd->mark_set = false;
				}
			}

			// set correct mode in qGo
			qgobrd->set_id(game_id);
			qgobrd->set_gsName(gsName);
			qgobrd->set_myName(myName);
			if ((game_id == 0) && (src != 6)) //SL added eb 12
				// set local board
				qgobrd->set_Mode(5);
			//else if (src==0)
			//	qgobrd->set_Mode(2); // special case when not triggered by the 'observe' command (trail, for instance)
			else
				qgobrd->set_Mode(src);

			// increase number of observed games
			emit signal_addToObservationList(-2);

			return true;
		}
	}

	switch (src)
	{
		// regular move info (parser)
		case 0:
#ifdef SHOW_MOVES_DLG
			dlg->write("Spiel " + gi->nr + gi->type + "\n");
#endif

			if (gi->mv_col == "T")
			{
#ifdef SHOW_MOVES_DLG
				dlg->write("W: " + gi->wname + " " + gi->wprisoners + " " + gi->wtime + " " + gi->wstones + "\n");
				dlg->write("B: " + gi->bname + " " + gi->bprisoners + " " + gi->btime + " " + gi->bstones + "\n");
#endif

				// set times
				qgobrd->setTimerInfo(gi->btime, gi->bstones, gi->wtime, gi->wstones);
			}
			else if (gi->mv_col == "B" || gi->mv_col == "W")
			{
#ifdef SHOW_MOVES_DLG
				dlg->write(gi->mv_col + " " + gi->mv_nr + " " + gi->mv_pt + "\n");
#endif

				// set move if game is initialized
				if (qgobrd->get_havegd())
				{
					// get all moves till now one times if kibitzing
					if (qgobrd->get_Mode() == modeObserve && !qgobrd->get_sentmovescmd())
					{
						qgobrd->set_sentmovescmd(true);
						emit signal_sendcommand("moves " + gi->nr, false);
						emit signal_sendcommand("all " + gi->nr, false);
					}
					else
					{
						// do normal move
  						qgobrd->set_move(sc, pt, mv_nr);
					}
				}
			}

			break;
				
		// adjourn/end/resume of a game (parser)
		case 1:
#ifdef SHOW_MOVES_DLG
			dlg->write("Spiel " + g->nr + (g->running ? " running\n" : " STOPPED\n"));
			dlg->write(g->Sz + "\n");
#endif

			if (!g->running)
			{
				// stopped game do not need a timer
				qgobrd->set_stopTimer();

				ASSERT(g->Sz);

				if (g->Sz == "-")
				{
					return false;
				}
				

				qgobrd->send_kibitz(g->Sz);

				// set correct result entry
				QString rs = QString();
				QString extended_rs = g->Sz;

				if (g->Sz.contains("White forfeits"))
					rs = "B+T";
				else if (g->Sz.contains("Black forfeits"))
					rs = "W+T";
				else if (g->Sz.contains("has run out of time"))
					 rs = ((( g->Sz.contains(myName) && qgobrd->get_myColorIsBlack() ) ||
						( !g->Sz.contains(myName) && !qgobrd->get_myColorIsBlack() )) ?
						"W+T" : "B+T");

				else if (g->Sz.contains("W ", true) && g->Sz.contains("B ", true))
				{
					// NNGS: White resigns. W 59.5 B 66.0
					// IGS: W 62.5 B 93.0
					// calculate result first
					int posw, posb;
					float re1, re2;
					posw = g->Sz.find("W ");
					posb = g->Sz.find("B ");
					bool wfirst = posw < posb;

					if (!wfirst)
					{
						int h = posw;
						posw = posb;
						posb = h;
					}

					QString t1 = g->Sz.mid(posw+1, posb-posw-2);
					QString t2 = g->Sz.right(g->Sz.length()-posb-1);
					re1 = t1.toFloat();
					re2 = t2.toFloat();

					re1 = re2-re1;
					if (re1 < 0)
					{
						re1 = -re1;
						wfirst = !wfirst;
					}
					
					if (re1 < 0.2)
					{
						rs = "Jigo";
						extended_rs = "        Jigo         ";
					}
					else if (wfirst)
					{
						rs = "B+" + QString::number(re1);
						extended_rs = "Black won by " + QString::number(re1) + " points";
					}
					else
					{
						rs = "W+" + QString::number(re1);
						extended_rs = "White won by " + QString::number(re1) + " points";
					}
				}
				else if (g->Sz.contains("White resigns"))
					rs = "B+R";
				else if (g->Sz.contains("Black resigns"))
					rs = "W+R";
				else if (g->Sz.contains("has resigned the game"))
					 rs = ((( g->Sz.contains(myName) && qgobrd->get_myColorIsBlack() ) ||
						( !g->Sz.contains(myName) && !qgobrd->get_myColorIsBlack() )) ?
						"W+R" : "B+R");
				if (rs)
				{
					qgobrd->get_win()->getBoard()->getGameData()->result = rs;
					qgobrd->send_kibitz(rs);
					qDebug("Result: " + rs);
				}

				if (g->Sz.contains("adjourned") && qgobrd->get_bplayer() && qgobrd->get_wplayer())
				{
					if (qgobrd->get_bplayer() == myName || qgobrd->get_wplayer() == myName)
						// can only reload own games
						qgobrd->get_win()->getInterfaceHandler()->refreshButton->setText(tr("LOAD"));
					qDebug("game adjourned... #" + QString::number(qgobrd->get_id()));
					qgobrd->set_adj(true);
					qgobrd->set_id(10000);
					qgobrd->clearObserverList();
					qgo->playGameEndSound();
				}
				else
				{
					// case: error -> game has not correctly been removed from list
					GameMode owngame = qgobrd->get_Mode();
					// game ended - if new game started with same number -> prohibit access
					qgobrd->set_id(-qgobrd->get_id());

					// enable Menus
//					qgobrd->get_win()->setOnlineMenu(false);

					// set board editable...
					qgobrd->set_Mode(1);
					// toggle score 2x
					qgobrd->get_win()->getInterfaceHandler()->toggleMode();
					qgobrd->get_win()->getInterfaceHandler()->toggleMode();

					//autosave ?
					//if ((setting->readBoolEntry("AUTOSAVE")) && (owngame == modeObserve)) 
					//{
					//	qgobrd->get_win()->doSave(qgobrd->get_win()->getBoard()->getCandidateFileName(),true);
					//	qDebug("Game saved");
					//}

					// but allow to use "say" cmd if it was an own game
					//if (owngame == modeMatch)
					bool doSave = ((setting->readBoolEntry("AUTOSAVE")) && (owngame == modeObserve)) ||
							((setting->readBoolEntry("AUTOSAVE_PLAYED")) && (owngame == modeMatch)); 
					wrapupMatchGame(qgobrd, doSave);
					QMessageBox::information(qgobrd->get_win() , tr("Game n° ") + QString::number(-qgobrd->get_id()), extended_rs);
					//else
					//	qgo->playGameEndSound();


				}

				// decrease number of observed games
				emit signal_addToObservationList(-1);
			}
			else if (!qgobrd->get_havegd())
			{
				// check if it's my own game
				if (g->bname == myName || g->wname == myName)
				{
					// case: trying to observe own game: this is the way to get back to match if
					// board has been closed by mistake
					if (g->bname == g->wname)
						// teach
						qgobrd->set_Mode(4);
					else
						// match
						qgobrd->set_Mode(3);
				}
				else if (g->bname == g->wname && !g->bname.contains('*'))
				{
					// case: observing a teaching game
					// make teaching features visible while observing
					qgobrd->get_win()->getMainWidget()->cb_opponent->setDisabled(true);
					qgobrd->get_win()->getMainWidget()->pb_controls->setDisabled(true);
					qgobrd->get_win()->getMainWidget()->pb_mark->setDisabled(true);
					qgobrd->ExtendedTeachingGame = true;
					qgobrd->IamTeacher = false;
					qgobrd->IamPupil = false;
					qgobrd->havePupil = false;
				}
				qgobrd->set_game(g);

				if (qgobrd->get_Mode() == modeMatch)
				{
					if (g->bname == myName)
					{
						qgobrd->get_win()->getBoard()->set_myColorIsBlack(true);
						qgobrd->set_myColorIsBlack(true);

						// I'm black, so check before the first move if komi, handi, free is correct
						if (qgobrd->get_requests_set())
						{
							qDebug("qGoIF::parse_move() : check_requests");
							qgobrd->check_requests();
						}
					}
					else if (g->wname == myName)
					{
						qgobrd->get_win()->getBoard()->set_myColorIsBlack(false);
						qgobrd->set_myColorIsBlack(false);
					}
					else
						qWarning("*** Wanted to set up game without knowing my color ***");
				}
			}
			break;

#ifdef SHOW_MOVES_DLG
		// game to observe (mouse click)
		case 2:
			dlg->write("Observe: " + txt + "\n");
			break;

		// game to play (mouse click and match prompt)
		case 3:
			dlg->write("Play(MATCH): " + txt + "\n");
			break;

		// game to play (mouse click and match prompt)
		case 4:
			dlg->write("Teach: " + txt + "\n");
			break;

      
#endif
		default:
			//qDebug("qGoInterface::parse_move() - illegal parameter");
			break;
	}

	// assume no new window
	return false;
}


// regular move info (parser)
void qGoIF::slot_move(GameInfo *gi)
{
	parse_move(0, gi);
}

// start/adjourn/end/resume of a game (parser)
void qGoIF::slot_move(Game *g)
{
	parse_move(1, 0, g);
}


// game to observe (mouse click)
void qGoIF::set_observe(const QString& gameno)
{
	parse_move(2, 0, 0, gameno);
}

// remove all boards
void qGoIF::set_initIF()
{
	parse_move(-1);
}

// a match is created
void qGoIF::slot_matchcreate(const QString &gameno, const QString &opponent)
{
	if (opponent == myName)
		// teaching game
		parse_move(4, 0, 0, gameno);
	else
		parse_move(3, 0, 0, gameno);
}

void qGoIF::slot_matchsettings(const QString &id, const QString &handicap, const QString &komi, assessType kt)
{
	// seek board
	qGoBoard *qb = boardlist->first();
	while (qb && qb->get_id() != id.toInt())
		qb = boardlist->next();

	if (!qb)
	{
		qWarning("BOARD CORRESPONDING TO GAME SETTINGS NOT FOUND !!!");
		return;
	}

	qb->set_requests(handicap, komi, kt);
	qDebug(QString("qGoIF::slot_matchsettings: h=%1, k=%2, kt=%3").arg(handicap).arg(komi).arg(kt));
}

void qGoIF::slot_title(const QString &title)
{
	// title message follows to move message -> not reliable
	// only teaching games can have a title

	// seek board
	int found = 0;
	qGoBoard *qb_save = 0;
	qGoBoard *qb = boardlist->first();
	while (qb)
	{
		if (//!qb->get_haveTitle() &&
			(qb->get_wplayer() == qb->get_bplayer() ||
			 qb->get_gameData().freegame == TEACHING))
		{
			qb_save = qb;
			found++;
		}
		qb = boardlist->next();
	}

	// set title if only one teaching game (with or without title) found
	if (found == 1 && qb_save)
		qb_save->set_title(title);
}

// komi set or requested
void qGoIF::slot_komi(const QString &nr, const QString &komi, bool isrequest)
{
	qGoBoard *qb;
	static int move_number_memo = -1;
	static QString komi_memo = NULL;

	// correctness:
	if (!komi)
		return;

	if (isrequest)
	{
		// check if opponent is me
		if (!nr || nr == myName)
			return;

		// 'nr' is opponent (IGS/NNGS)
		qb = boardlist->first();
		while (qb != NULL && qb->get_wplayer() != nr && qb->get_bplayer() != nr)
			qb = boardlist->next();

		if (qb)
		{
			// check if same opponent twice (IGS problem...)
			if (move_number_memo == qb->get_mv_counter() && komi_memo  && komi_memo == komi)
			{
				qDebug(QString("...request skipped: opponent %1 wants komi %2").arg(nr).arg(komi));
				return;
			}

			// remember of actual values
			move_number_memo = qb->get_mv_counter();
			komi_memo = komi;

			if (qb->get_reqKomi() == komi  && setting->readBoolEntry("DEFAULT_AUTONEGO"))
			{
				if (qb->get_currentKomi() != komi)
				{
					qDebug("GoIF::slot_komi() : emit komi");
					emit signal_sendcommand("komi " + komi, true);
				}
			}
			else
			{
				slot_requestDialog(tr("komi ") + komi, tr("decline"), QString::number(qb->get_id()), nr);
			}
		}

		return;
	}
	// own game if nr == NULL
	else if (!nr)
	{
		if (!myName)
		{
			// own name not set -> should never happen!
			qWarning("*** Wrong komi because don't know which online name I have ***");
			return;
		}

		qb = boardlist->first();
		while (qb != NULL && qb->get_wplayer() != myName && qb->get_bplayer() != myName)
			qb = boardlist->next();
	}
	else
	{
		// title message follows to move message
		qb = boardlist->first();
		while (qb != NULL && qb->get_id() != nr.toInt())
			qb = boardlist->next();
	}

	if (qb)
		qb->set_komi(komi);
}

// game free/unfree message received
void qGoIF::slot_freegame(bool freegame)
{
	qGoBoard *qb;

	qb = boardlist->first();
	// what if 2 games (IGS) and mv_counter < 3 in each game? -> problem
	while (qb != NULL && qb->get_wplayer() != myName && qb->get_bplayer() != myName && qb->get_mvcount() < 3)
		qb = boardlist->next();

	if (qb)
		qb->set_freegame(freegame);
}

// board window closed...
void qGoIF::slot_closeevent()
{
	int id = 0;
	qGoBoard *qb;

	// erase actual pointer to prevent error
	qgobrd = 0;

	// seek board
	qb = boardlist->first();
	while (qb && (id = qb->get_id()) > 0)
		qb = boardlist->next();

	if (!qb)
	{
		qWarning("CLOSED BOARD NOT FOUND IN LIST !!!");
		return;
	}

	qDebug(QString("qGoIF::slot_closeevent() -> game %1").arg(qb->get_id()));

	// destroy timers
	qb->set_stopTimer();

	if (id < 0 && id > -10000)
	{
		switch (qb->get_Mode())
		{
			case modeObserve:
				// unobserve
				emit signal_sendcommand("observe " + QString::number(-id), false);

				// decrease number of observed games
				emit signal_addToObservationList(-1);
				break;

			case modeMatch:

				// decrease number of observed games
				emit signal_addToObservationList(-1);
				break;

			case modeTeach:
				emit signal_sendcommand("adjourn", false);

				// decrease number of observed games
				emit signal_addToObservationList(-1);
				break;

			default:
				break;
		}
	}

//	MainWindow *win = qb->get_win();
//	qgo->removeBoardWindow(win);
	boardlist->remove();

	delete qb;
//	delete win;
}
*/

/*
 * kibitz strings received from parser
 */
void qGoIF::slot_kibitz(int num, const QString& who, const QString& msg)
{
	qGoBoard *qb;
	QString name;
/*
	// own game if num == NULL
	if (!num)
	{
		if (!myName)
		{
			// own name not set -> should never happen!
			qWarning("*** qGoIF::slot_kibitz(): Don't know my online name ***");
			return;
		}

		qb = boardlist->first();
		while (qb != NULL && qb->get_wplayer() != myName && qb->get_bplayer() != myName)
			qb = boardlist->next();

		if (qb && qb->get_wplayer() == myName)
			name = qb->get_bplayer();
		else if (qb)
			name = qb->get_wplayer();

		if (qb)
			// sound for "say" command
			qgo->playSaySound();
	}
	else
	{

		// seek board to send kibitz
		qb = boardlist->first();
		while (qb && qb->get_id() != num)
			qb = boardlist->next();
*/
		name = who;
//	}


	
	BoardWindow *bw = getBoardWindow(num);

	if (!bw)
		qDebug("Board to send kibitz string not in list...");
/*	else if (!num && who)
	{
		// special case: opponent has resigned - interesting in quiet mode
		qb->send_kibitz(msg);
		//qgo->playGameEndSound();
		wrapupMatchGame(qb, true);
	}
*/
	else
		bw->qgoboard->kibitzReceived(name + ": " + msg);
}
/*
void qGoIF::slot_requestDialog(const QString &yes, const QString &no, const QString & id, const QString &opponent)
{
	QString opp;
	if (opponent)
		opp = opponent;
	else
		opp = tr("Opponent");

	if (!no)
	{
		QMessageBox mb(tr("Request of Opponent"),
			QString(tr("%1 wants to %2\nYES = %3\nCANCEL = %4")).arg(opp).arg(yes).arg(yes).arg(tr("ignore request")),
			QMessageBox::NoIcon,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::Cancel | QMessageBox::Escape,
			QMessageBox::NoButton);
		mb.setActiveWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug(QString(QString("qGoIF::slot_requestDialog(): emit %1").arg(yes)));
			emit signal_sendcommand(yes, false);
		}
	}
	else
	{
		QMessageBox mb(tr("Request of Opponent"),
			//QString(tr("%1 wants to %2\nYES = %3\nCANCEL = %4")).arg(opp).arg(yes).arg(yes).arg(no),
			QString(tr("%1 wants to %2\n\nDo you accept ? \n")).arg(opp).arg(yes),
			QMessageBox::Question,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No | QMessageBox::Escape,
			QMessageBox::NoButton);
		mb.setActiveWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug(QString(QString("qGoIF::slot_requestDialog(): emit %1").arg(yes)));
			emit signal_sendcommand(yes, false);
		}
		else
		{
			qDebug(QString(QString("qGoIF::slot_requestDialog(): emit %1").arg(no)));
			emit signal_sendcommand(no, false);
		}
	}
}
*/

/*
 * sends a command to the server
 */
void qGoIF::slot_sendCommandFromInterface(const QString &text, bool show)
{
	emit signal_sendCommandFromInterface(text, show);
}


/*
void qGoIF::slot_removestones(const QString &pt, const QString &game_id)
{
	qGoBoard *qb = boardlist->first();

	if (!pt && !game_id)
	{
qDebug("slot_removestones(): !pt !game_id");
		// search game
		qb = boardlist->first();
		while (qb && (qb->get_Mode() != modeMatch) && (qb->get_Mode() != modeComputer))
			qb = boardlist->next();
		
		if (!qb)
		{
			qWarning("*** No Match found !!! ***");
			return;
		}

		switch (gsName)
		{
			case IGS:
				// no parameter -> restore counting
				if (qb->get_win()->getInterfaceHandler()->passButton->text() == QString(tr("Done")))
				{
qDebug("slot_removestones(): IGS restore counting");
					// undo has done -> reset counting
					qb->get_win()->doRealScore(false);
					qb->get_win()->getBoard()->setMode(modeMatch);
					qb->get_win()->getBoard()->previousMove();
					qb->dec_mv_counter();
					qb->get_win()->doRealScore(true);
					qb->send_kibitz(tr("SCORE MODE: RESET - click on a stone to mark as dead..."));
				}
				else if (qb->get_Mode() == modeMatch)
				{
qDebug("slot_removestones(): IGS score mode");
					// set to count mode
					qb->get_win()->doRealScore(true);
					qb->send_kibitz(tr("SCORE MODE: click on a stone to mark as dead..."));
				}
				break;

			default:
				if ((qb->get_Mode() == modeMatch) || (qb->get_Mode() == modeComputer))
				{
qDebug("slot_removestones(): NON IGS counting");
					// set to count mode
					qb->get_win()->doRealScore(true);
					qb->send_kibitz(tr("SCORE MODE: click on a stone to mark as dead..."));
				}
				else
				{
qWarning("slot_removestones(): NON IGS no match");
					// back to matchMode
					qb->get_win()->doRealScore(false);
					qb->get_win()->getBoard()->setMode(modeMatch);
					qb->get_win()->getBoard()->previousMove();
					qb->dec_mv_counter();
					qb->send_kibitz(tr("GAME MODE: place stones..."));
				}
				break;
		}

		return;
	}

	if (pt && !game_id)
	{
qDebug("slot_removestones(): pt !game_id");
		// stone coords but no game number:
		// single match mode, e.g. NNGS
		while (qb && qb->get_Mode() != modeMatch && qb->get_Mode() != modeTeach)
			qb = boardlist->next();
	}
	else
	{
qDebug("slot_removestones(): game_id");
		// multi match mode, e.g. IGS
		while (qb && qb->get_id() != game_id.toInt())
			qb = boardlist->next();

		if (qb && qb->get_win()->getInterfaceHandler()->passButton->text() != QString(tr("Done")))
		{
			// set to count mode
			qb->get_win()->doRealScore(true);
			qb->send_kibitz(tr("SCORE MODE: click on a stone to mark as dead..."));
		}
	}
		
	if (!qb)
	{
		qWarning("*** No Match found !!! ***");
		return;
	}

	int i = (QChar) pt[0] - 'A' + 1;
	// skip j
	if (i > 8)
		i--;
	
	int j;
	if (pt[2] >= '0' && pt[2] <= '9')
		j = qb->get_boardsize() + 1 - pt.mid(1,2).toInt();
	else
		j = qb->get_boardsize() + 1 - pt[1].digitValue();

	// mark dead stone
	qb->get_win()->getBoard()->getBoardHandler()->markDeadStone(i, j);
	qb->send_kibitz("removing @ " + pt);
}
*/

/*
 * game status received from parser
 */
void qGoIF::slot_score(const QString &txt, const QString &line, bool isplayer, const QString &komi)
{
	static BoardWindow *qb = 0;
	static float wcount, bcount;
	static int column;
	bool found  = FALSE;	

	if (isplayer)
	{
		qb = 0;//boardlist->first();
		// find right board - slow, but just once a game
//		while (qb != NULL && qb->get_wplayer() != txt && qb->get_bplayer() != txt)
//			qb = boardlist->next();

		QHashIterator<int, BoardWindow*> i(*boardlist);
 		while (i.hasNext() && !found) 
		{
     			i.next();
     			qb = i.value();
			found = ((qb->get_wplayer() == txt) || (qb->get_bplayer() == txt));
		}


		if (qb && qb->get_wplayer() == txt)
		{
			// prisoners white + komi
			wcount = line.toFloat() + komi.toFloat();
		}
		else if (qb && qb->get_bplayer() == txt)
		{
			// prisoners black
			bcount = line.toFloat();
			// ok, start counting column
			column = 0;
		}
	}
	else if (qb)
	{
		// ok, now mark territory points only
		// 0 .. black
		// 1 .. white
		// 2 .. free
		// 3 .. neutral
		// 4 .. white territory
		// 5 .. black territory

		// one line at a time
		column++;
		int i;
		for (i = 0; i < (int) line.length(); i++)
		{
			switch(line[i].digitValue())
			{
				case 4:
					// add white territory
					wcount++;
					qb->qgoboard->addMark(column, i+1, markTerrWhite);//, true);
					break;

				case 5:
					// add black territory
					bcount++;
					qb->qgoboard->addMark(column, i+1, markTerrBlack);//, true);
					break;

				default:
					break;
			}
		}
		// send kibitz string after counting
		if (txt.toInt() == (int) line.length() - 1)
			//send_kibitz
			 qDebug("Game Status: W: %f B: %f", wcount, bcount);

		// show territory
//		qb->get_win()->getBoard()->updateCanvas();
	}
}

/*
 * game end result received from parser
 */
void qGoIF::slot_result(Game *g)
{
	
	BoardWindow *bw = getBoardWindow(g->nr.toInt());
	
	if(!bw)
		return;
	

//	bw->send_kibitz(g->Sz);

	// set correct result entry
	QString rs = QString();
	QString extended_rs = g->res;

	if (g->res.contains("White forfeits"))
		rs = "B+T";
	else if (g->res.contains("Black forfeits"))
		rs = "W+T";
	else if (g->res.contains("has run out of time"))
			rs = ((( g->res.contains(myName) && bw->getMyColorIsBlack() ) ||
			( !g->res.contains(myName) && !bw->getMyColorIsBlack() )) ?
			"W+T" : "B+T");

	else if (g->res.contains("W ", Qt::CaseSensitive) && g->res.contains("B ", Qt::CaseSensitive))
	{
		// NNGS: White resigns. W 59.5 B 66.0
		// IGS: W 62.5 B 93.0
		// calculate result first
		int posw, posb;
		float re1, re2;
		posw = g->res.indexOf("W ");
		posb = g->res.indexOf("B ");
		bool wfirst = posw < posb;

		if (!wfirst)
		{
			int h = posw;
			posw = posb;
			posb = h;
		}

		QString t1 = g->res.mid(posw+1, posb-posw-2);
		QString t2 = g->res.right(g->Sz.length()-posb-1);
		re1 = t1.toFloat();
		re2 = t2.toFloat();

		re1 = re2-re1;
		if (re1 < 0)
		{
			re1 = -re1;
			wfirst = !wfirst;
		}
		
		if (re1 < 0.2)
		{
			rs = "Jigo";
//			extended_rs = "        Jigo         ";
		}
		else if (wfirst)
		{
			rs = "B+" + QString::number(re1);
			extended_rs = "Black won by " + QString::number(re1) + " points";
		}
		else
		{
			rs = "W+" + QString::number(re1);
			extended_rs = "White won by " + QString::number(re1) + " points";
		}
	}
	else if (g->res.contains("White resigns"))
		rs = "B+R";
	else if (g->res.contains("Black resigns"))
		rs = "W+R";
	else if (g->res.contains("has resigned the game"))
			rs = ((( g->res.contains(myName) && bw->getMyColorIsBlack() ) ||
			( !g->res.contains(myName) && !bw->getMyColorIsBlack() )) ?
			"W+R" : "B+R");
	if (!rs.isEmpty())
	{
		bw->getGameData()->result = rs;
		bw->qgoboard->setResult(g->res + "\n" + rs,extended_rs );
		qDebug("Result: %s", rs.toLatin1().constData());
	}
}

/*
// undo a move
void qGoIF::slot_undo(const QString &player, const QString &move)
{
	qDebug("qGoIF::slot_undo()");
	qGoBoard *qb = boardlist->first();

	// check if game number given
	bool ok;
	int nr = player.toInt(&ok);
	if (ok)
		while (qb != NULL && qb->get_id() != nr)
			qb = boardlist->next();
	else
		while (qb != NULL && qb->get_wplayer() != player && qb->get_bplayer() != player)
			qb = boardlist->next();

	if (!qb)
	{
		qWarning("*** board for undo not found!");
		return;
	}

	if (move != "Pass")
	{
		// only the last '0' is necessary
		qb->set_move(stoneNone, 0, 0);
		return;
	}

	// back to matchMode
	qb->get_win()->doRealScore(false);
	qb->get_win()->getBoard()->setMode(modeMatch);
	qb->get_win()->getBoard()->previousMove();
	qb->dec_mv_counter();
	qb->send_kibitz(tr("GAME MODE: place stones..."));
}



// set independent local board
void qGoIF::set_localboard(QString file)
{
	qGoBoard *qb = new qGoBoard(this, qgo);
	qb->set_id(0);
	// normal mode
	qb->set_Mode(1);
	if (file == "/19/")
	{
		qb->set_komi("5.5");
		// special case - set up 19x19 board without any questions
	}
	else if (file)
		qb->get_win()->doOpen(file, 0, false);
	else
		qb->get_win()->slotFileNewGame();
//	qb->get_win()->setOnlineMenu(false);
	qb->get_win()->getInterfaceHandler()->toggleMode();
	qb->get_win()->getInterfaceHandler()->toggleMode();

	boardlist->append(qb);
	qb->set_id(++localBoardCounter);

	// connect with board (MainWindow::CloseEvent())
	connect(qb->get_win(),
		SIGNAL(signal_closeevent()),
		qb,
		SLOT(slot_closeevent()));
	// connect with board (qGoBoard)

}

// set independent local board + open game
void qGoIF::set_localgame()
{
	qGoBoard *qb = new qGoBoard(this, qgo);
	qb->set_id(0);
	// normal mode
	qb->set_Mode(1);
	qb->get_win()->slotFileOpen();
//	qb->get_win()->setOnlineMenu(false);
	qb->get_win()->getInterfaceHandler()->toggleMode();
	qb->get_win()->getInterfaceHandler()->toggleMode();

	boardlist->append(qb);
	qb->set_id(++localBoardCounter);

	// connect with board (MainWindow::CloseEvent())
	connect(qb->get_win(),
		SIGNAL(signal_closeevent()),
		qb,
		SLOT(slot_closeevent()));
	// connect with board (qGoBoard)
}



// time has been added
void qGoIF::slot_timeAdded(int time, bool toMe)
{
	qGoBoard *qb = boardlist->first();
	while (qb != NULL && qb->get_Mode() != modeMatch)
		qb = boardlist->next();

	if (!qb)
	{
		qWarning("*** board for undo not found!");
		return;
	}

	// use time == -1 to pause the game
	if (time == -1)
	{
		qb->set_gamePaused(toMe);
		return;
	}

	if (toMe && qb->get_myColorIsBlack() || !toMe && !qb->get_myColorIsBlack())
		qb->addtime_b(time);
	else
		qb->addtime_w(time);
}


void qGoIF::wrapupMatchGame(qGoBoard * qgobrd, bool doSave)
{
	qgo->playGameEndSound();				
	qgobrd->get_win()->getInterfaceHandler()->commentEdit2->setReadOnly(false);
	qgobrd->get_win()->getInterfaceHandler()->commentEdit2->setDisabled(false);
	
	//autosave ?
	if (doSave) 
	{
		qgobrd->get_win()->doSave(qgobrd->get_win()->getBoard()->getCandidateFileName(),true);
		qDebug("Game saved");
	}


}
*/
