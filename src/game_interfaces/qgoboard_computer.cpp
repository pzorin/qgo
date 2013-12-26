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
#include "qgtp.h"
#include "tree.h"
#include "move.h"
#include "../network/messages.h"
#include "boardhandler.h"
#include "gamedata.h"
#include "boardwindow.h"
#include "qgtp.h"

#include <QMessageBox>

qGoBoardComputerInterface::qGoBoardComputerInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoard(bw,  t, gd) //, QObject(bw)
{
	QSettings settings;

	gtp = new QGtp() ;

    connect (gtp, SIGNAL(signal_computerPlayed(bool, int, int)), SLOT(slot_playComputer(bool, int, int)));
    connect (gtp, SIGNAL(computerPassed()), SLOT(slot_passComputer()));
    connect (gtp, SIGNAL(computerResigned()), SLOT(slot_resignComputer()));

	if (gtp->openGtpSession(settings.value("COMPUTER_PATH").toString(),
				gameData->board_size,
				gameData->komi,
				gameData->handicap,
				GNUGO_LEVEL)==FAIL)
	{
		throw QString(QObject::tr("Error opening program: %1")).arg(gtp->getLastMessage());
	}


	if (!(gameData->fileName.isNull() || gameData->fileName.isEmpty()))
	{
//		QTemporaryFile tempFile("GNUgo_file") ;
//		tempFile.createLocalFile(gameData->fileName);//= QTemporaryFile::createLocalFile(gameData->fileName);
//		QFileInfo fi(tempFile);
//		gtp->loadsgf(fi.absoluteFilePath());
		if (gtp->loadsgf(gameData->fileName))
		{
			throw QString(QObject::tr("Error GNUgo loading file %1 : %2")).arg(gameData->fileName).arg(gtp->getLastMessage());
		}	
	}
	//FIXME : GNU go will not want whitespaces in the name. Either feed it with the moves, or get through a temporary file.
	
//	prepareComputerBoard();  
	if (gameData->handicap && gameData->fileName.isEmpty())
	//if(gameData->handicap)
		setHandicap(gameData->handicap);


	/* What is playSound for??*/
	// value 1 = no sound, 0 all games, 2 my games
	playSound = (settings.value("SOUND") != 1);
}

qGoBoardComputerInterface::~qGoBoardComputerInterface()
{
	qDebug("Deconstructing computer interface");
	delete gtp;
}

/*
 * This is called just after intialisation, when the computer is playing the firt move
 */
void qGoBoardComputerInterface::startGame() 
{
	bool blackToPlay = getBlackTurn();
	
	Move *m = tree->findLastMoveInMainBranch();
	tree->setCurrent(m);
	boardwindow->getBoardHandler()->updateMove(m);

	if ((boardwindow->getMyColorIsBlack() && blackToPlay) || (boardwindow->getMyColorIsWhite() && !blackToPlay))
		return;

	playComputer( blackToPlay ? stoneBlack : stoneWhite );
}

/*
 * This processes a 'Pass' button pressed
 * If the preceding move was also a pass request, we enter the score mode
 */
void qGoBoardComputerInterface::localPassRequest()
//was slot_doPass()
{
	qDebug("qgbCI::localPassRequest");
	/* Don't think we're using localPassRequest any more since
	 * its redundant with subclass sendPassToInterface FIXME */
	StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );

	tree->doPass(false);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

	sendPassToInterface(c);

	if (tree->getCurrent()->parent->isPassMove())
		enterScoreMode();	
	else
		playComputer( c == stoneWhite ? stoneBlack : stoneWhite );
}

void qGoBoardComputerInterface::slotDonePressed()
{
	/* FIXME  Since there's no server to tell us the result, we have
	 * to calculate it or whatever, print out a winner a score, etc..
	 * I'm really surprised there's no existing code to do this
	 * actually.  Also feels like the kind of thing that could
	 * be in qGoBoard, some generic win reporting thing, score
	 * reporting, etc.. and then the set_result functions could
	 * call that qGoBoard thing with the specifics.*/
	qGoBoard::slotDonePressed();
	qDebug("Done Pressed\n");
}

void qGoBoardComputerInterface::slotUndoPressed()
{
	if(boardwindow->getGamePhase() == phaseScore)
		leaveScoreMode();
	else
	{
		gtp->undo(0);
		qGoBoard::slotUndoPressed();
	}
}

/*
 * This sends the move to the computer through GTP
 */
void qGoBoardComputerInterface::sendMoveToInterface(StoneColor c,int x, int y)
{
	boardwindow->getUi()->resignButton->setEnabled(false);
  
	switch (c)
	{
		case stoneWhite :
        if (gtp->playwhite(x ,y))
        	{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to play the stone within program \n") + 	gtp->getLastMessage());
			return;
        	}
		break;

		case stoneBlack :
        if (gtp->playblack(x ,y))
		{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to play the stone within program \n") + gtp->getLastMessage());
			return;
        }
        break;

		default :
		; 
	}

	/* Moved here from after the localMoveRequest */
	playComputer( c == stoneWhite ? stoneBlack : stoneWhite );
}

/*
 * This asks the computer to make a move
 */
void qGoBoardComputerInterface::playComputer(StoneColor c)
// was void qGoBoard::playComputer(StoneColor c) in qGo1
{

	// have the computer play a stone of color 'c'

	//get_win()->getInterfaceHandler()->passButton->setEnabled(false);
	//get_win()->getInterfaceHandler()->undoButton->setEnabled(false);

	switch (c)
	{
		case stoneBlack :
		if (gtp->genmoveBlack())
		{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to have the program play its stone\n") + gtp->getLastMessage());
			return;
		}
		break;

		case stoneWhite :
		if (gtp->genmoveWhite())
		{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to have the program play its stone\n") + gtp->getLastMessage());
			return;
		}
		break;

		default :
		; 	
	
	}
}

/*
 * This slot is triggeres by the signal emitted by 'gtp' when getting a move
 */
void qGoBoardComputerInterface::slot_playComputer(bool ok, int x, int y)
{
	if (!ok)
	{
		QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to have the program play its stone\n") + gtp->getLastMessage());
		return;
    }

    bool b = getBlackTurn();
    set_move(b ? stoneBlack : stoneWhite , x, y);

	//qDebug ("computer move played");

	//the computer just played. Are we after 2 passes moves ?
 
	if ((tree->getCurrent()->isPassMove())&&(tree->getCurrent()->parent->isPassMove()))
//	{
		enterScoreMode();
//		return;
//	}

	// trick : if we have the computer play against himself, we recurse ...
//	if (win->blackPlayerType ==   win->whitePlayerType)
//		playComputer( c==stoneBlack ? stoneWhite : stoneBlack);
	/* We shouldn't be able to resign during computers turn
	 * but this is a little awkward to put it in the comp
	 * interface here.  But otherwise there's no real
	 * way to check who the current player is since the computer
	 * moves are done with a function call rather than based on
	 * some check of who's turn it is */
    boardwindow->getUi()->resignButton->setEnabled(true);
}

void qGoBoardComputerInterface::slot_passComputer()
{

}

void qGoBoardComputerInterface::slot_resignComputer()
{
    bool b = getBlackTurn();
    GameResult g((!b ? stoneBlack : stoneWhite), GameResult::RESIGN);
    setResult(g);
}

/*
 * sends a "pass" move to GTP
 */
void qGoBoardComputerInterface::sendPassToInterface(StoneColor c)
{
//	if (id < 0)
//		return;

//	mv_counter++;
	//StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );
	doPass();
	//tree->doPass(false);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

	
	//check if this is the second pass move.
	if ((tree->getCurrent()->isPassMove())&&(tree->getCurrent()->parent->isPassMove()))
	{
//		emit signal_2passes(0,0);
		enterScoreMode();
		qDebug("2nd pass move");
		return;
	}
  
	// if simple pass, tell computer and move on
	switch (c)
	{
		case stoneWhite :
		if (gtp->playwhitePass())
		{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to pass within program \n") + gtp->getLastMessage());
			return;
		}
		else
			qDebug("comp notified of white pass");
	//	if (win->blackPlayerType==COMPUTER)
	//		playComputer(c);
	
		break;

		case stoneBlack :
		if (gtp->playblackPass())
		{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to pass within program \n") + gtp->getLastMessage());
			return;
		}

//		if (win->whitePlayerType==COMPUTER)
//			playComputer(c);
		break;

		default :
		;
	}
	
	if (tree->getCurrent()->parent->isPassMove())
		enterScoreMode();	
	else
		playComputer( c == stoneWhite ? stoneBlack : stoneWhite );

}

/*
 * A move string is incoming from the interface (computer)
 */
void qGoBoardComputerInterface::set_move(StoneColor sc, int x, int y)
{
    if (!doMove(sc, x, y))
        QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move %1 seems to be invalid"));
    boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}
