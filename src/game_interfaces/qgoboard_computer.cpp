/***************************************************************************
 *   Copyright (C) 2006 by EB   *
 *      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "qgoboard.h"
#include "qgtp.h"
#include "tree.h"
#include "move.h"


qGoBoardComputerInterface::qGoBoardComputerInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoard(bw,  t, gd) //, QObject(bw)
{

}

/*
 * This functions initialises the board for computer game
 * It also starts the go engine.
 * it sends True if sucessful, 0 if the engine could not be started
 */
bool qGoBoardComputerInterface::init()
{
	QSettings settings;

	gtp = new QGtp() ;

	connect (gtp, SIGNAL(signal_computerPlayed(bool, const QString&)), SLOT(slot_playComputer(bool, const QString&)));

	if (gtp->openGtpSession(settings.value("COMPUTER_PATH").toString(),
				gameData->size,
				gameData->komi,
				gameData->handicap,
				GNUGO_LEVEL)==FAIL)
	{
		// if gnugo fails
//		QString mesg = QString(QObject::tr("Error opening program: %1\n")).arg(gtp->getLastMessage());
		QMessageBox msg(QObject::tr("Error"), //mesg,
			QString(QObject::tr("Error opening program: %1")).arg(gtp->getLastMessage()),
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
//		msg.setActiveWindow();
		msg.raise();
		msg.exec();

		return FALSE ;
	}


	if (!(gameData->fileName.isNull() || gameData->fileName.isEmpty()))
	{
//		QTemporaryFile tempFile("GNUgo_file") ;
//		tempFile.createLocalFile(gameData->fileName);//= QTemporaryFile::createLocalFile(gameData->fileName);
//		QFileInfo fi(tempFile);
//		gtp->loadsgf(fi.absoluteFilePath());
		if (gtp->loadsgf(gameData->fileName))
		{
			// if gnugo fails
	//		QString mesg = QString(QObject::tr("Error opening program: %1\n")).arg(gtp->getLastMessage());
			QMessageBox msg(QObject::tr("Error"), //mesg,
				QString(QObject::tr("Error GNUgo loading file %1 : %2")).arg(gameData->fileName).arg(gtp->getLastMessage()),
				QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
	//		msg.setActiveWindow();
			msg.raise();
			msg.exec();
	
			return FALSE ;
		}	
	}
	//FIXME : GNU go will not want whitespaces in the name. Either feed it with the moves, or get through a temporary file.
	
//	prepareComputerBoard();  
	if (gameData->handicap && gameData->fileName.isEmpty())
		setHandicap(gameData->handicap);


	Move *m = tree->findLastMoveInMainBranch();
	tree->setCurrent(m);
	boardwindow->getBoardHandler()->updateMove(m);

	return TRUE;

}

/*
 * This is called just after intialisation, when the computer is playing the firt move
 */
void qGoBoardComputerInterface::startGame() 
{
	bool blackToPlay = getBlackTurn();
	
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
	StoneColor c = (getBlackTurn() ? stoneBlack : stoneWhite );

	tree->doPass(FALSE);
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

	sendPassToInterface(c);

	if (tree->getCurrent()->parent->isPassMove())
		enterScoreMode();	
	else
		playComputer( c == stoneWhite ? stoneBlack : stoneWhite );
}


/*
 * This functions gets the move request (from a board click)
 * and displays the resulting stone (if valid)
 * It also passes it to the 'proxy' or interface to the go engine
 */
void qGoBoardComputerInterface::localMoveRequest(StoneColor c, int x, int y)
{
//	if (!doMove(c,x,y))
//		return;
	
//	sendMoveToInterface(c,x,y);

	qGoBoard::localMoveRequest(c,  x,  y);
	// FIXME : this should be made in a better way : wait for the interface to acknowledge before adding the move to the tree
	playComputer( c == stoneWhite ? stoneBlack : stoneWhite );
}


/*
 * This sends the move to the computer through GTP
 */
void qGoBoardComputerInterface::sendMoveToInterface(StoneColor c,int x, int y)
{
//was void qGoBoard::slot_stoneComputer(StoneColor c, int x, int y) in qGo1

//	if (id < 0)
//		return;

	if (x > 8)
		x++;
	char c1 = x - 1 + 'A';
	//int c2 = gd.size + 1 - y;
	int c2 = boardwindow->getBoardSize()  + 1 - y;
	
	
//	mv_counter++;
  
	switch (c)
	{
		case stoneWhite :
		if (gtp->playwhite(c1 ,c2))
        	{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to play the stone within program \n") + 	gtp->getLastMessage());
			return;
        	}
//		if (!getMyColorIsBlack() && (gtp->getLastMessage() != "illegal move") )
//			playComputer(c);  

		break;

		case stoneBlack :
		if (gtp->playblack(c1 ,c2))
		{
			QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to play the stone within program \n") + gtp->getLastMessage());
			return;
		}

//		if (!getMyColorIsWhite() && (gtp->getLastMessage() != "illegal move"))
//			playComputer(c);   
		break;  

		default :
		; 
	}
}

/*
 * This asks the computer to make a move
 */
void qGoBoardComputerInterface::playComputer(StoneColor c)
// was void qGoBoard::playComputer(StoneColor c) in qGo1
{

	// have the computer play a stone of color 'c'

//	get_win()->getInterfaceHandler()->passButton->setEnabled(false);
//TODO	get_win()->getInterfaceHandler()->undoButton->setEnabled(false);

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
void qGoBoardComputerInterface::slot_playComputer(bool ok, const QString &computer_answer)
{
	if (!ok)
	{
		QMessageBox::warning(boardwindow, PACKAGE, tr("Failed to have the program play its stone\n") + gtp->getLastMessage());
		return;
	}
//	qDebug("Computer interface - computer answer = %s",  computer_answer.toLatin1().constData());

//	win->getBoard()->unsetCursor();
//	get_win()->getInterfaceHandler()->passButton->setEnabled(true);
//	get_win()->getInterfaceHandler()->undoButton->setEnabled(true);
	bool b = getBlackTurn();

	if (computer_answer == "resign")
   	{
		boardwindow->getInterfaceHandler()->displayComment((!b ? "White resigned" : "Black resigned"));
		boardwindow->getGameData()->result = (!b ? "B+R" : "W+R");
//		slot_DoneComputer();
		return ;
	}	

	set_move(b ? stoneBlack : stoneWhite , computer_answer, "" /*mv_nr*/);

//	qDebug ("computer move played");

	//the computer just played. Are we after 2 passes moves ?
 
	if ((tree->getCurrent()->isPassMove())&&(tree->getCurrent()->parent->isPassMove()))
//	{
		enterScoreMode();
//		return;
//	}

	// trick : if we have the computer play against himself, we recurse ...
//	if (win->blackPlayerType ==   win->whitePlayerType)
//		playComputer( c==stoneBlack ? stoneWhite : stoneBlack);
   
}


void qGoBoardComputerInterface::sendPassToInterface(StoneColor c)
{
//	if (id < 0)
//		return;

//	mv_counter++;

	//check if this is the second pass move.
	if ((tree->getCurrent()->isPassMove())&&(tree->getCurrent()->parent->isPassMove()))
	{
//		emit signal_2passes(0,0);
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

}


/*
 * A move string is incoming from the interface (computer)
 */
void qGoBoardComputerInterface::set_move(StoneColor sc, QString pt, QString/* mv_nr*/)
{

	if (pt.contains("Pass",Qt::CaseInsensitive))
		doPass();
	
	else
	{
		int i = pt[0].unicode() - QChar::fromAscii('A').unicode() + 1;
		// skip j
		if (i > 8)
			i--;

		int j;

		if (pt[2] >= '0' && pt[2] <= '9')
			j = boardwindow->getGameData()->size + 1 - pt.mid(1,2).toInt();
		else
			j = boardwindow->getGameData()->size + 1 - pt[1].digitValue();


		if (!doMove(sc, i, j))
			QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move %1 seems to be invalid").arg(pt.toLatin1().constData()));
	}

	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

}

