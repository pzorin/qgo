/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "qgoboard.h"
#include "boardwindow.h"
#include "tree.h"
#include "move.h"
#include "defines.h"
#include "audio.h"
#include "boarddispatch.h"
#include "messages.h"
#include "resultdialog.h"
#include "boardhandler.h"
#include "clockdisplay.h"
#include "gamedata.h"
#include "matrix.h"

qGoBoard::qGoBoard(BoardWindow *bw, Tree * t, GameData *gd) : QObject(bw)
{
	isModified = false;	
	tree = t;
	boardwindow = bw;
	
	//data used for observing games, when getting the already played moves
	stated_mv_count = -1;	
	lastMoveInGame = 0;

	gameData = gd;

    clickSound = new Sound("stone.wav");

	dontCheckValidity = false;
	lastSound = QTime(0,0,0);
}

/* FIXME: Make sure this isn't called from places it shouldn't be. Like
 * subclass interfaces.  */
void qGoBoard::setHandicap(int handicap)
{
	GamePhase store = boardwindow->getGamePhase();
	//qDebug("set Handicap " + QString::number(handicap) + ", stored mode == " + QString::number(store));
	boardwindow->setGamePhase(phaseEdit);
	qDebug("setHandicap called\n");
	int size = gameData->board_size;
	int edge_dist = (size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (size + 1) / 2;
	int high = size + 1 - edge_dist;
	
	if (size > 25 || size < 7)
	{
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
//		setMode(store);
		return;
	}
	
	// change: handicap is first move
//	if (handicap > 1)
//	{
		/* Commented out because handi should be root move */
		//tree->createMoveSGF();
		//createNode(*tree->getCurrent()->getMatrix(), false, false);
		/* Move should already be set to 0 by board handler and the placement
		 * of handicap stones won't change that */
		//currentMove++;
		tree->getCurrent()->setMoveNumber(0);
		tree->getCurrent()->setHandicapMove(true);
//	}
	if(tree->getCurrent()->getNumBrothers())
		qDebug("handi has brother??");

	// extra:
	if (size == 19 && handicap > 9)
		switch (handicap)
		{
		case 13:  // Hehe, this is nuts... :)
			tree->addStoneToCurrentMove(stoneBlack, 17, 17);
		case 12:
			tree->addStoneToCurrentMove(stoneBlack, 3, 3);
		case 11:
			tree->addStoneToCurrentMove(stoneBlack, 3, 17);
		case 10:
			tree->addStoneToCurrentMove(stoneBlack, 17, 3);
			
		default:
			handicap = 9;
			break;
		}
	
	switch (size % 2)
	{
	// odd board size
	case 1:
		switch (handicap)
		{
		case 9:
			tree->addStoneToCurrentMove(stoneBlack, middle, middle);
		case 8:
		case 7:
			if (handicap >= 8)
			{
				tree->addStoneToCurrentMove(stoneBlack, middle, low);
				tree->addStoneToCurrentMove(stoneBlack, middle, high);
			}
			else
				tree->addStoneToCurrentMove(stoneBlack, middle, middle);
		case 6:
		case 5:
			if (handicap >= 6)
			{
				tree->addStoneToCurrentMove(stoneBlack, low, middle);
				tree->addStoneToCurrentMove(stoneBlack, high, middle);
			}
			else
				tree->addStoneToCurrentMove(stoneBlack, middle, middle);
		case 4:
			tree->addStoneToCurrentMove(stoneBlack, high, high);
		case 3:
			tree->addStoneToCurrentMove(stoneBlack, low, low);
		case 2:
			tree->addStoneToCurrentMove(stoneBlack, high, low);
			tree->addStoneToCurrentMove(stoneBlack, low, high);
		case 1:
//			if (store != modeObserve && store != modeMatch &&  store != modeTeach)
//				gameData->komi = 0.5;
			break;
			
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;
		
	// even board size
	case 0:
		switch (handicap)
		{
		case 4:
			tree->addStoneToCurrentMove(stoneBlack, high, high);
		case 3:
			tree->addStoneToCurrentMove(stoneBlack, low, low);
		case 2:
			tree->addStoneToCurrentMove(stoneBlack, high, low);
			tree->addStoneToCurrentMove(stoneBlack, low, high);
		case 1:
//			if (store != modeObserve && store != modeMatch &&  store != modeTeach)
//				gameData->komi = 0.5;
			break;
			
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;
		
	default:
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
				
	}
	
	// Change cursor stone color
	//board->setCurStoneColor();
	gameData->handicap = handicap;
	boardwindow->setGamePhase(store);
	//board->getInterfaceHandler()->disableToolbarButtons();
}

/*
 * adds a mark to the current move's matrix
 * This is used in edit phase to add mark on the board
 */
void qGoBoard::addMark( int x, int y, MarkType t )
{
	Matrix * mat = tree->getCurrent()->getMatrix();
	if (mat->getMarkAt(x,y) != markNone)
		mat->removeMark(x,y);

	if (t == markText || t == markNumber)
	{
		QString txt = mat->getFirstTextAvailable(t);
		mat->setMarkText(x,y,txt);
	}

	mat->insertMark(x,y,t);
	/* FIXME this is a bit awkward here, but markTerr does not
	 * mark dead and they aren't ghosted immediately for some
	 * reason unless they're dead.  Assuming this generally
	 * comes through qgoboard network code, we'll mark them
	 * dead here for now. 
	 * I'm also still not certain if this is necessary, there
	 * was another bug elsewhere, but it can't hurt. */
	if(t == markTerrBlack && mat->getStoneAt(x,y) == stoneWhite)
		mat->markStoneDead(x,y);
	else if(t == markTerrWhite && mat->getStoneAt(x,y) == stoneBlack)
		mat->markStoneDead(x,y);
	
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
	setModified();
}

/*
 * removes a mark to the current move's matrix
 * This is used in edit phase to remove a mark on the board
 */
void qGoBoard::removeMark( int x, int y)
{
	if (tree->getCurrent()->getMatrix()->getMarkAt(x,y) != markNone)
		tree->getCurrent()->getMatrix()->removeMark(x,y);
		
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
	setModified();
}

/*
 * adds a stone to the current move's matrix
 * This is used in edit phase to add stones (and not moves) on the board
 */
void qGoBoard::addStone(StoneColor c, int x, int y)
{
	tree->addStoneToCurrentMove(c, x, y);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
	setModified();
}

int qGoBoard::getMoveNumber(void)
{
	return tree->getCurrent()->getMoveNumber();
}

/*
 * 'Pass' button pressed
 * FIXME we could just overload the slotPassPressed
 * which would obviate sendPassToInterface
 */
void qGoBoard::slotPassPressed()
//was slot_doPass()
{
	StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );

	if((getBlackTurn() && boardwindow->getMyColorIsBlack()) ||
	   (!getBlackTurn() && boardwindow->getMyColorIsWhite()))
		sendPassToInterface(c);
}

/*
 * 'Score' button toggled
 */
void qGoBoard::slotScoreToggled(bool pressed)
{
	if (pressed)
		enterScoreMode();
	else 
		leaveScoreMode();
}

void qGoBoard::setResult(GameResult & r)
{
	if (tree->getCurrent() == NULL)		//how can this happen???
	{
		qDebug("How can this possibly happen???");
		return;
	}
	if(boardwindow->getGamePhase() == phaseEnded)
	{
		/* FIXME This is only okay if this is the only function
		 * setting the gamePhase to phaseEnded */
		qDebug("Already received result");
		return;
	}
	lastMoveInGame = tree->getCurrent();
	if(r.result != GameResult::NOGAME)
		kibitzReceived("\n" + r.shortMessage());
	boardwindow->getGameData()->result = r.shortMessage();
	/* This is all a little ugly. FIXME.  The problem is that the
	 * QString result is used by the sgf loader, its sort of
	 * an easy way to store and load results through sgf files.
	 * The problem is that these aren't necessarily anything
	 * more than a string which means either we have to interpret
	 * them and allow partial GameResult records or this, 
	 * which is to have two different results on the game data.  And then
	 * all of this is just basically so the network code
	 * can get the margin when the client calulates it for
	 * a protocol.  So yeah, find a better way. FIXME 
	 * Really, I'd like to make the gamedata be one, single
	 * stand alone object instead of something we copy to different
	 * places, and then I'd like to do the same with the result
	 * as an add on to it.  Just add whatever interpretation of
	 * SGF files is necessary.*/
	boardwindow->getGameData()->fullresult = new GameResult(r);
	//just one gamedata now, FIXME delete comment
	BoardDispatch * boarddispatch = 0;
	boarddispatch = boardwindow->getBoardDispatch();
	/*if(!boarddispatch)
	{
		qDebug("No board dispatch for game result");
		return;
	}
	GameData * gr = boarddispatch->getGameData();
	gr->fullresult = new GameResult(r);*/
	/* This should be parent window modal, otherwise its annoying.
	 * I'm going to make it its own dialog so it could be extended
	 * later */
	if(boarddispatch && r.result != GameResult::NOGAME)
	{
		ResultDialog * rd = new ResultDialog(boardwindow, boarddispatch, boardwindow->getId(), &r);
		boarddispatch->setRematchDialog(rd);		//necessary right?
		rd->setWindowModality(Qt::WindowModal);
		rd->show();
	}
	QSettings settings;
	if( settings.value("AUTOSAVE").toBool())
		boardwindow->doSave(boardwindow->getCandidateFileName(),true);
	
	boardwindow->setGamePhase(phaseEnded);
	/* FIXME:  getting the result doesn't set the result
	 * in the toolbar for some reason.  Like it doesn't
	 * show score or captures or anything else. 
	 * this was an enterScoreMode thing, but there's more
	 * issues there*/
}

/*
 * This handles the main envent with qGo : something has been clicked on the board
 */
void qGoBoard::slotBoardClicked(bool , int x, int y , Qt::MouseButton mouseState)
{
	bool blackToPlay = getBlackTurn();
	
	switch (boardwindow->getGamePhase())
	{
		case phaseInit:
			//should not happen
			return ;
		
		case phaseNavTo:
		{
			boardwindow->getBoardHandler()->findMoveByPos(x, y); 
			return;
		}

		case phaseEdit:
		{
			switch (boardwindow->getEditMark())
			{
				case markNone:
				{
					if(tree->getCurrent()->getMatrix()->getStoneAt(x,y) == stoneNone)
						addStone(mouseState == Qt::LeftButton ? stoneBlack : stoneWhite, x, y);
					else
						addStone(stoneErase, x, y);
					setModified(true);
					return;
				}
				default:
				{
					if (mouseState == Qt::LeftButton)
						addMark(x,y, boardwindow->getEditMark());
					else
						removeMark(x,y);
					setModified(true);
					return;
				}
					
			}
			return;
		}

		case phaseEnded:
			return;

		case phaseScore:
		{
			localMarkDeadRequest(x,y);
			return;
		}

		case phaseOngoing:
			if (blackToPlay && boardwindow->getMyColorIsBlack())
				localMoveRequest(stoneBlack,x,y);

			if (!blackToPlay && boardwindow->getMyColorIsWhite())
				localMoveRequest(stoneWhite,x,y);
			return;
	}
}

/* FIXME sendMoveToInterface is awkward.  Should be a virtual function and anyway,
 * its redundant in different places.
 * but come on, doMove, sendMoveToInterface and localMoveRequest?  And that's just
 * one of these. */

/*
 * This functions gets the move request (from a board click)
 * and displays the resulting stone (if valid)
 */
void qGoBoard::localMoveRequest(StoneColor c, int x, int y)
{
	if (doMove(c,x,y))
	{
		boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
		sendMoveToInterface(c,x,y);
	}
}

/*
 * This functions gets the request (from a board click)
 * to mark a stone as dead/undead (score mode). All local but 'match'
 */
void qGoBoard::localMarkDeadRequest(int x, int y)
{
	toggleGroupAt(x,y);
	/* FIXME maybe should be markDeadArea()?  Maybe as option? */
}

/* We need to combine the below perhaps with the above sendPassToInterface */
/*
 * This function adds a pass move to a game. there is no need to return anything
 */
void qGoBoard::doPass()
{
//	StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );

	tree->doPass(false);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

	setModified();

}

/*
 * This functions adds a move to a game. returns 1 if move was valid, 0 if not)
 */
bool qGoBoard::doMove(StoneColor c, int x, int y, bool dontplayyet)
{
	bool validMove = true;

	if(dontplayyet && !tree->checkMoveIsValid(c, x, y))
		validMove = false;
	else if(!dontplayyet)
	{
		if(dontCheckValidity)
		{
			dontCheckValidity = false;
			validMove = true;
		}
        else
            validMove = tree->checkMoveIsValid(c, x, y);
		if(validMove)
		{
			tree->addStoneOrLastValidMove();
			setModified(true);
			/* Not a great place for this, but maybe okay: */
			TimeRecord t = boardwindow->getClockDisplay()->getTimeRecord(!getBlackTurn());
			if(t.time != 0 || t.stones_periods != -1)
			{
				tree->getCurrent()->setTimeinfo(true);
				tree->getCurrent()->setTimeLeft(t.time);
				tree->getCurrent()->setOpenMoves(t.stones_periods);
			}
		}
	}
	else
	{
		dontCheckValidity = true;
	}

	/* Non trivial here.  We don't want to play a sound as we get all
	 * the moves from an observed game.  But there's no clean way
	 * to tell when the board has stopped loading, particularly for IGS.
	 * so we only play a sound every 500 msecs... 
	 * Also, maybe it should play even if we aren't looking at last move, yeah not sure on that FIXME */
	if(!dontplayyet && validMove && boardwindow->getGamePhase() == phaseOngoing &&
		   QTime::currentTime() > lastSound /*&& (boardwindow->getGameMode() != modeObserve ||
		   tree->getCurrent()->getMoveNumber() == tree->findLastMoveInMainBranch()->getMoveNumber())*/)
	{
			if (playSound)
				clickSound->play();
			//setModified();
			lastSound = QTime::currentTime();	
			lastSound = lastSound.addMSecs(250);	//500 too slow
	}
	
	return validMove;
}

/*
 * Returns true wether it's black to play
 * FIXME shouldn't this be in board.cpp or some place?
 * maybe not...
 */
bool qGoBoard::getBlackTurn(bool time)
{
	if (tree->getCurrent()->getPLinfo())
		// color of next stone is same as current
		return tree->getCurrent()->getPLnextMove() == stoneBlack;
	
	//if (tree->getCurrent() == tree->getRoot())
	//	return true;
	
	// the first handicap move bears number 0 as well
	if (tree->getCurrent()->getMoveNumber() == 0)
	{
		// Handicap, so white starts
		if (gameData->handicap >= 2)
			return false;
			
		return true;
	}
	
	// Normal mode
/*	if (tree->getCurrent()->getGameMode() == modeNormal ||
		tree->getCurrent()->getGameMode() == modeObserve ||
		tree->getCurrent()->getGameMode() == modeMatch ||
		tree->getCurrent()->getGameMode() == modeTeach ||
    		tree->getCurrent()->getGameMode() == modeComputer)
*/
	if (boardwindow->getGamePhase() != phaseEdit)
	{
		// change color
		/* This is to prevent clock ticking for wrong player
		 * when looking back over moves in an observed game,
		 * for instance */
		if(time)
			return (tree->findLastMoveInMainBranch()->getColor() == stoneWhite);
		else
			return (tree->getCurrent()->getColor() == stoneWhite);
	}
	// Edit mode. Return color of parent move.
	else if (tree->getCurrent()->parent != NULL)
		return tree->getCurrent()->parent->getColor() == stoneWhite;

	// Crap happened. 50% chance this is correct .)
	qWarning("Oops, crap happened in BoardHandler::getBlackTurn() !");
	return true;
}

/*
 * This functions initialises the scoring mode
 */
void qGoBoard::enterScoreMode()
{
	if(boardwindow->getGamePhase() == phaseScore)
		return;
	qDebug("qgb::enterScoreMode()");
	boardwindow->setGamePhase (phaseScore);
	boardwindow->getUi()->tabDisplay->setCurrentIndex(1);
	if(boardwindow->getGameMode() != modeObserve)
		boardwindow->getBoardHandler()->updateCursor();
	boardwindow->getBoardHandler()->countScore();
}

/*
 * This functions leaves the scoring mode
 */
void qGoBoard::leaveScoreMode()
{
	if(boardwindow->getGamePhase() != phaseScore)
		return;
	qDebug("leaving score mode");
	boardwindow->getUi()->tabDisplay->setCurrentIndex(0);
	boardwindow->setGamePhase ( phaseOngoing );
	boardwindow->getBoardHandler()->exitScore();
}

void qGoBoard::toggleGroupAt(int x, int y)
{
	// is the click on a stone ?
	if ( tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneNone )
		return;
	
	tree->getCurrent()->getMatrix()->toggleGroupAt(x, y);
	boardwindow->getBoardHandler()->countScore();
}

/*
 * This functions is called in scoring phase when clicking on a dead group
 */
void qGoBoard::markDeadStone(int x, int y)
{
	// is the click on a stone ?
	if ( tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneNone )
		return;
	
	tree->getCurrent()->getMatrix()->markGroupDead(x, y);
	boardwindow->getBoardHandler()->countScore();
}

void qGoBoard::markLiveStone(int x, int y)
{
	// is the click on a stone ?
	if ( tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneNone )
		return;

	tree->getCurrent()->getMatrix()->markGroupAlive(x, y);
	boardwindow->getBoardHandler()->countScore();
}

void qGoBoard::markDeadArea(int x, int y)
{
	// is the click on a stone ?
	if ( tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneNone )
		return;
	
	tree->getCurrent()->getMatrix()->markAreaDead(x, y);
	boardwindow->getBoardHandler()->countScore();
}

void qGoBoard::markLiveArea(int x, int y)
{
	// is the click on a stone ?
	if ( tree->getCurrent()->getMatrix()->getStoneAt(x, y) == stoneNone )
		return;
	
	tree->getCurrent()->getMatrix()->markAreaAlive(x, y);
	boardwindow->getBoardHandler()->countScore();
}

void qGoBoard::slotUndoPressed(void)
{
	/* Are we supposed to make a brother node or something ??? FIXME */
	tree->deleteNode();
	/* Why doesn't the move get updated when we delete a node automatically FIXME? */
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}

void qGoBoard::slotResignPressed(void)
{
	GameResult g((getBlackTurn() ? stoneWhite : stoneBlack), GameResult::RESIGN);
	setResult(g);
}

void qGoBoard::slotDonePressed(void)
{
	GameResult g = boardwindow->getBoardHandler()->retrieveScore();
	setResult(g);
}

/*
 * The text in the comment zone has been changed
 */
void qGoBoard::slotUpdateComment()
{
	QString s = boardwindow->getUi()->commentEdit->toPlainText();

	// case where the text has 'really' been altered, versus text changed
	// because we traverse moves
	if (tree->getCurrent()->getComment() != s)
	{
		tree->getCurrent()->setComment(s);
		setModified();
	}
}

/*
 * kibitz was received. Tranfer it to move and comment window
 */
void qGoBoard::kibitzReceived(const QString& text)
{

	QString k = text;
	// be nice to do this in bold or something
	if(stated_mv_count != tree->findLastMoveInMainBranch()->getMoveNumber())
	{
		stated_mv_count = tree->findLastMoveInMainBranch()->getMoveNumber();
		k.prepend( "(" + QString::number(stated_mv_count) + ") ");
	}
	QString txt = tree->getCurrent()->getComment();

	if (!txt.isEmpty())
		txt.append('\n');

	txt.append(k);
	tree->getCurrent()->setComment(txt);

//	if (!boardwindow->getUi()->commentEdit->toPlainText().isEmpty())
//		boardwindow->getUi()->commentEdit->append("\n");

	
	boardwindow->getUi()->commentEdit->append(k);
	//qDebug("kibitzReceived: %s\n", text.toLatin1().constData());
}

// send regular time Info
void qGoBoard::timerEvent(QTimerEvent*)
{
	// wait until first move
//	if (mv_counter < 0 || id < 0 || game_paused)
	if (boardwindow->getGamePhase() != phaseOngoing)
		return;

	boardwindow->getClockDisplay()->setTimeStep(getBlackTurn(true));
/*
	// warn if I am within the last 10 seconds
	if (gameMode == modeMatch)
	{
		if (myColorIsBlack && bt_i <= BY_timer && bt_i > -1 && mv_counter % 2) // ||
		{
			// each second we alternate button background color
			if (bt_i %2)
				win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(Qt::red);
			else 
				win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;//setPaletteBackgroundColor(Qt::PaletteBase);
			qgo->playTimeSound();
		}		
	
		else if ( !myColorIsBlack && wt_i <= BY_timer && wt_i > -1 && (mv_counter % 2) == 0)
		{
			if (wt_i %2)
				win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(Qt::red);
			else 
				win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;//setPaletteBackgroundColor(Qt::PaletteBase);
			qgo->playTimeSound();
		}
		
		//we have to reset the color when not anymore in warning period)
		else if (win->getInterfaceHandler()->normalTools->pb_timeBlack->paletteBackgroundColor() == Qt::red)
			win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;

		
		else if (win->getInterfaceHandler()->normalTools->pb_timeWhite->paletteBackgroundColor() == Qt::red)
			win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;

		

	}
*/
}

/* No pointer?  Just an object?? FIXME */
TimeRecord qGoBoard::getOurTimeRecord(void)
{
	return boardwindow->getClockDisplay()->getTimeRecord(boardwindow->getMyColorIsBlack());
}

TimeRecord qGoBoard::getTheirTimeRecord(void)
{
	return boardwindow->getClockDisplay()->getTimeRecord(!boardwindow->getMyColorIsBlack());
}


/***************************************************************************
 *
 * Normal Interface
 *
 ****************************************************************************/


qGoBoardNormalInterface::qGoBoardNormalInterface(BoardWindow *bw, Tree * t, GameData *gd) 
	: qGoBoard(bw, t, gd)
{
	boardwindow->getUi()->board->clearData();

	// If we have handicap, but not from a loaded file, we have to set the handicap move
	if (gameData->handicap && gameData->fileName.isEmpty())
	{
		setHandicap(gameData->handicap);
		boardwindow->getBoardHandler()->slotNavLast();
	}
	else 
//	tree->setToFirstMove();	
		boardwindow->getBoardHandler()->slotNavFirst();

	QSettings settings;
	// value 1 = no sound, 0 all games, 2 my games
	playSound = (settings.value("SOUND") != 1);
}
