/***************************************************************************
qgtp.cpp  -  description
-------------------
begin                : Thu Nov 29 2001
copyright            : (C) 2001 by PALM Thomas , DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien
email                : 
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/


#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#include <QApplication>
#include <QDebug>
enum CommandType {PROTOCOL, BOARDSIZE, KNOWN_COMMAND, LEVEL, KOMI, PLAY_BLACK, PLAY_WHITE, GENMOVE};
#include "qgtp.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

/* Function:  open a session
* Arguments: name and path of go program
* Fails:     never
* Returns:   nothing
*/
QGtp::QGtp()
{
	/*openGtpSession(filename);*/
	programProcess = NULL; // This to permit proper ending
	responseReceived = false;
	answer = "";
    busy = false;
}

QGtp::~QGtp()
{
	if (programProcess)
		programProcess->kill();     // not very clean (should use tryTerminate)
}

/* Code */

/* Function:  get the last message from gnugo
* Arguments: none
* Fails:     never
* Returns:   last message from GO
*/


QString QGtp::getLastMessage()
{
	return _response;
}

int QGtp::openGtpSession(QString path, QString args, int size, float komi, int handicap)
{
	_cpt = 1000;
	
	programProcess = new QProcess();
    programProcess->setReadChannel(QProcess::StandardOutput);
    QStringList arguments = args.split(' ',QString::SkipEmptyParts);
    issueCmdNb = false;

    if (path.contains(QRegExp("gnugo$", Qt::CaseInsensitive)))
        issueCmdNb = true; // FIXME: are command numbers really gnugo-specific?

	connect(programProcess, SIGNAL(readyRead()),
		this, SLOT(slot_readFromStdout()) );
	connect(programProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
		this, SLOT(slot_processExited(int , QProcess::ExitStatus )) );
	
    qDebug() << "QGtp::openGtpSession(" << path << "," << args << ")";

    programProcess->start(path, arguments);
	

	if (!programProcess->waitForStarted())
	{
          _response="Could not start "+path;
		  return FAIL ;
	}
	
	//?? never know ... otherwise, I get a segfault with sprint ...
	if ((outFile = (char *)malloc (100)) == NULL)
	{
		  _response="Yuck ! Could not allocate 100 bytes !!!"  ;
		  return FAIL ;
	}   
	
	if (protocolVersion()==OK)
	{
		if(getLastMessage().toInt() !=2)
		{
			qDebug("Protocol version problem???");
			//				quit();
			_response="Protocol version not supported";
			//				return FAIL;
		}
		if(setBoardsize(size)==FAIL)
		{
			return FAIL;
		}
		if(clearBoard()==FAIL)
		{
			// removed by frosla -> protocol changes...
			// return FAIL;
		}

/*
		if(knownCommand("level")==FAIL)
		{
			  return FAIL;
		}
	
		else if (getLastMessage().contains("true"))
        	{
        		if (setLevel(level)==FAIL)
			{
				return FAIL;
			}
        	}
*/ 
      
		if(setKomi(komi)==FAIL)
		{
			return FAIL;
		}

		if(fixedHandicap(handicap)==FAIL)
		{
			return FAIL;
		}
	}
	else
	{
		quit();
		_response="Protocol version error";
		return FAIL;
	}

    busy = false;
	return OK;
}

// Read from stdout
void QGtp::slot_readFromStdout()
{
	int number;
	int pos;

    while (programProcess->canReadLine())
    {
        answer = programProcess->readLine();
        answer.chop(1); // remove the trailing '\n'
        responseReceived = ((! answer.isEmpty()) &&
                            ((answer.at(0) == '=') || (answer.at(0) == '?')));
        if (responseReceived)
            break;
    }
    if (!responseReceived)
        return;

    _response = answer;
    qDebug("** QGtp::slot_readFromStdout():  %s" , _response.toLatin1().constData());

	// do we have any answer after the command number ?
	pos = _response.indexOf(" ");
	number = _response.mid(1,pos).toInt();

	if (pos < 1)
		_response = "";
	else
		_response = _response.right(_response.length() - pos - 1);

    int i = moveRequests.indexOf(number);
    if (i == -1)
        return;
    // Otherwise we have an answer to a move request.
    // This is the one case in which we currently do not busy-wait.
    busy=false;
    moveRequests.removeAt(i);

    if (_response == "resign")
    {
        emit computerResigned();
        return;
    } else if (_response.contains("Pass",Qt::CaseInsensitive))
    {
        emit computerPassed();
        return;
    }

    int x = _response[0].unicode() - QChar::fromLatin1('A').unicode() + 1;
    // skip 'J'
    if (x > 8)
        x--;

    int y;
    if (_response[2] >= '0' && _response[2] <= '9')
        y = _response.mid(1,2).toInt();
    else
        y = _response[1].digitValue();
    emit signal_computerPlayed( x, y );
}

// exit
void QGtp::slot_processExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug("Process Exited with exit code %i and status  %d", exitCode, exitStatus);
//	fflush ("quit");
    //	return waitResponse();
}

/* Function:  wait for a go response
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::waitResponse()
{
    busy=true;
    do //FIXME : use the readyRead signal of the process
	{
        programProcess->waitForReadyRead(10);
        qApp->processEvents();
    } while (!responseReceived);

	qDebug("** QGtp::waitResponse():  \'%s\'" , _response.toLatin1().constData());
	responseReceived = false;
    busy=false;

    return ((answer.at(0) == '=') ? OK : FAIL );
}

/****************************
* Program identity.        *
****************************/

/* Function:  Report the name of the program.
* Arguments: none
* Fails:     never
* Returns:   program name
*/
int
QGtp::name ()
{
    fflush("name");
	return waitResponse();
}

/* Function:  Report protocol version.
* Arguments: none
* Fails:     never
* Returns:   protocol version number
*/
int
QGtp::protocolVersion ()
{
    fflush("protocol_version");
	return waitResponse();
}

void QGtp::fflush(QByteArray s)
{
    if (issueCmdNb)
    {
        s = QString().setNum(_cpt++).toLatin1() + " " + s;
    }
    qDebug("flush -> %s",s.constData());
    s += '\n';
    int i= programProcess->write(s);

    if ( i != s.size())
        qDebug("Error writing %s",s.constData());
}

/****************************
* Administrative commands. *
****************************/

/* Function:  Quit
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::quit ()
{
    fflush("quit");
	return waitResponse();
}

/* Function:  Report the version number of the program.
* Arguments: none
* Fails:     never
* Returns:   version number
*/
int
QGtp::version ()
{
    fflush("version");
	return waitResponse();
}


/***************************
* Setting the board size. *
***************************/

/* Function:  Set the board size to NxN.
* Arguments: integer
* Fails:     board size outside engine's limits
* Returns:   nothing
*/
int
QGtp::setBoardsize (int size)
{
    fflush("boardsize "+intToQByteArray(size));
	return waitResponse();
}


/* Function:  Find the current boardsize
* Arguments: none
* Fails:     never
* Returns:   board_size
*/
int
QGtp::queryBoardsize()
{
    fflush ("query_boardsize");
	return waitResponse();
}

/***********************
* Clearing the board. *
***********************/

/* Function:  Clear the board.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/

int
QGtp::clearBoard ()
{
    fflush ("clear_board");
	return waitResponse();
}

/***************************
* Setting.           *
***************************/

/* Function:  Set the komi.
* Arguments: float
* Fails:     incorrect argument
* Returns:   nothing
*/
int
QGtp::setKomi(float f)
{
    fflush (QString("komi %1").arg(f,0,'f',2).toLatin1());
    return waitResponse();
}

/* Function:  Set the playing level.
* Arguments: int
* Fails:     incorrect argument
* Returns:   nothing
*/
int
QGtp::setLevel (int level)
{
    fflush ("level "+intToQByteArray(level));
	return waitResponse();
}
/******************
* Playing moves. *
******************/

/* Function:  Play a black stone at the given vertex.
* Arguments: vertex
* Fails:     invalid vertex, illegal move
* Returns:   nothing
*/
int
QGtp::playblack (int x, int y)
{
    fflush (QByteArray("play black ")+encodeCoors(x,y));
	return waitResponse();
}

/* Function:  Black pass.
* Arguments: None
* Fails:     never
* Returns:   nothing
*/
int
QGtp::playblackPass ()
{
    fflush ("play black pass");
    return waitResponse();
}

/* Function:  Play a white stone at the given vertex.
* Arguments: vertex
* Fails:     invalid vertex, illegal move
* Returns:   nothing
*/
int
QGtp::playwhite (int x, int y)
{
    fflush ("play white "+encodeCoors(x,y));
    return waitResponse();
}

/* Function:  White pass.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::playwhitePass ()
{
    fflush ("play white pass");
    return waitResponse();
}

/* Function:  Set up fixed placement handicap stones.
* Arguments: number of handicap stones
* Fails:     invalid number of stones for the current boardsize
* Returns:   list of vertices with handicap stones
*/
int
QGtp::fixedHandicap (int handicap)
{
	if (handicap < 2) 
		return OK;

    fflush ("fixed_handicap "+intToQByteArray(handicap));
    return waitResponse();
}

int QGtp::set_free_handicap(QList<Point> handicap_stones)
{
    QByteArray message("set_free_handicap");
    for ( int i=0; i<handicap_stones.length(); ++i )
    {
        message.append(" ").append(encodeCoors(handicap_stones[i].x,handicap_stones[i].y));
    }

    fflush (message);
    return waitResponse();
}

/* Function:  Load an sgf file, possibly up to a move number or the first
*            occurence of a move.
* Arguments: filename + move number, vertex, or nothing
* Fails:     missing filename or failure to open or parse file
* Returns:   color to play
*/
int QGtp::loadsgf (QString filename,int /*movNumber*/,char /*c*/,int /*i*/)
{
    fflush (QString("loadsgf %1").arg(filename).toLatin1());
	return waitResponse();
}

/*****************
* Board status. *
*****************/

/* Function:  Return the color at a vertex.
* Arguments: vertex
* Fails:     invalid vertex
* Returns:   "black", "white", or "empty"
*/
int
QGtp::whatColor (char c, int i)
{
    fflush ("color "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Count number of liberties for the string at a vertex.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   Number of liberties.
*/
int
QGtp::countlib (char c, int i)
{
    fflush ("countlib "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Return the positions of the liberties for the string at a vertex.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   Sorted space separated list of vertices.
*/
int
QGtp::findlib (char c, int i)
{
    fflush ("findlib "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Tell whether a move is legal.
* Arguments: move
* Fails:     invalid move
* Returns:   1 if the move is legal, 0 if it is not.
*/
int
QGtp::isLegal (QString color, char c, int i)
{
    fflush (QString("is_legal %1 ").arg(color).toLatin1()+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  List all legal moves for either color.
* Arguments: color
* Fails:     invalid color
* Returns:   Sorted space separated list of vertices.
*/
int
QGtp::allLegal (QString color)
{
    fflush (QByteArray("all_legal ")+color.toLatin1());
	return waitResponse();
}

/* Function:  List the number of captures taken by either color.
* Arguments: color
* Fails:     invalid color
* Returns:   Number of captures.
*/
int
QGtp::captures (QString color)
{
    fflush (QByteArray("captures")+color.toLatin1());
	return waitResponse();
}

/**********************
* Retractable moves. *
**********************/

/* Function:  Play a stone of the given color at the given vertex.
* Arguments: move (color + vertex)
* Fails:     invalid color, invalid vertex, illegal move
* Returns:   nothing
*/
int
QGtp::trymove (QString color, char c, int i)
{
    fflush (QByteArray("trymove ")+color.toLatin1()+" "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Undo a trymove.
* Arguments: none
* Fails:     stack empty
* Returns:   nothing
*/
int
QGtp::popgo ()
{
    fflush ("popgo");
	return waitResponse();
}

/*********************
* Tactical reading. *
*********************/

/* Function:  Try to attack a string.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   attack code followed by attack point if attack code nonzero.
*/
int
QGtp::attack (char c, int i)
{
    fflush ("attack "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Try to defend a string.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   defense code followed by defense point if defense code nonzero.
*/
int
QGtp::defend (char c, int i)
{
    fflush ("defend "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Increase depth values by one.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::increaseDepths ()
{
    fflush("increase_depths");
	return waitResponse();
}

/* Function:  Decrease depth values by one.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::decreaseDepths ()
{
    fflush("decrease_depths");
	return waitResponse();
}

/******************
* owl reading. *
******************/

/* Function:  Try to attack a dragon.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   attack code followed by attack point if attack code nonzero.
*/
int
QGtp::owlAttack (char c, int i)
{
    fflush("owl_attack "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Try to defend a dragon.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   defense code followed by defense point if defense code nonzero.
*/
int
QGtp::owlDefend (char c, int i)
{
    fflush("owl_defend "+c+intToQByteArray(i));
	return waitResponse();
}

/********
* eyes *
********/

/* Function:  Evaluate an eye space
* Arguments: vertex
* Fails:     invalid vertex
* Returns:   Minimum and maximum number of eyes. If these differ an
*            attack and a defense point are additionally returned.
*            If the vertex is not an eye space or not of unique color,
*            a single -1 is returned.
*/

int
QGtp::evalEye (char c, int i)
{
    fflush("eval_eye "+c+intToQByteArray(i));
	return waitResponse();
}

/*****************
* dragon status *
*****************/

/* Function:  Determine status of a dragon.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   status ("alive", "critical", "dead", or "unknown"),
*            attack point, defense point. Points of attack and
*            defense are only given if the status is critical and the
*            owl code is enabled.
*
* FIXME: Should be able to distinguish between life in seki
*        and life with territory. Should also be able to identify ko.
*/

int
QGtp::dragonStatus (char c, int i)
{
    fflush(QString("dragon_status %1%2").arg(c).arg(i).toLatin1());
	return waitResponse();
}

/* Function:  Determine whether two stones belong to the same dragon.
* Arguments: vertex, vertex
* Fails:     invalid vertex, empty vertex
* Returns:   1 if the vertices belong to the same dragon, 0 otherwise
*/

int
QGtp::sameDragon (char c1, int i1, char c2, int i2)
{
    fflush(QString("same_dragon %1%2 %3%4").arg(c1).arg(i1).arg(c2).arg(i2).toLatin1());
	return waitResponse();
}

/* Function:  Return the information in the dragon data structure.
* Arguments: nothing
* Fails:     never
* Returns:   Dragon data formatted in the corresponding way to gtp_worm__
*/
int
QGtp::dragonData ()
{
    fflush("dragon_data");
	return waitResponse();
}

/* Function:  Return the information in the dragon data structure.
* Arguments: intersection
* Fails:     invalid coordinate
* Returns:   Dragon data formatted in the corresponding way to gtp_worm__
*/
int
QGtp::dragonData (char c,int i)
{
    fflush("dragon_data "+c+intToQByteArray(i));
	return waitResponse();
}

/***********************
* combination attacks *
***********************/

/* Function:  Find a move by color capturing something through a
*            combination attack.
* Arguments: color
* Fails:     invalid color
* Returns:   Recommended move, PASS if no move found
*/

int
QGtp::combinationAttack (QString color)
{
    fflush(QByteArray("combination_attack ")+color.toLatin1());
	return waitResponse();
}

/********************
* generating moves *
********************/

/* Function:  Generate and play the supposedly best black move.
* Arguments: none
* Fails:     never
* Returns:   a move coordinate (or "PASS")
*/
int
QGtp::genmoveBlack ()
{
    if (busy)
        return FAIL; // Ignore multiple requests while the engine is busy

    moveRequests.append(_cpt);
    busy=true;
    fflush("genmove black");
    return OK;
}

/* Function:  Generate and play the supposedly best white move.
* Arguments: none
* Fails:     never
* Returns:   a move coordinate (or "PASS")
*/
int
QGtp::genmoveWhite ()
{
    if (busy)
        return FAIL;

    moveRequests.append(_cpt);
    busy=true;
    fflush("genmove white");
    return OK;
}

/* Function:  Generate the supposedly best move for either color.
* Arguments: color to move, optionally a random seed
* Fails:     invalid color
* Returns:   a move coordinate (or "PASS")
*/
int
QGtp::genmove (QString color,int seed)
{
    fflush(QByteArray("gg_genmove ") +color.toLatin1()+" "+intToQByteArray(seed));
	return waitResponse();
}

/* Function : Generate a list of the best moves for White with weights
* Arguments: none
* Fails:   : never
* Returns  : list of moves with weights
*/

int
QGtp::topMovesWhite ()
{
    fflush("top_moves_white");
	return waitResponse();
}

/* Function : Generate a list of the best moves for Black with weights
* Arguments: none
* Fails:   : never
* Returns  : list of moves with weights
*/

int
QGtp::topMovesBlack ()
{
    fflush("top_moves_black");
	return waitResponse();
}

/* Function:  Undo last move
* Arguments: int
* Fails:     If move pointer is 0
* Returns:   nothing
*/
int
QGtp::undo (int i)
{
    fflush("undo "+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Report the final status of a vertex in a finished game.
* Arguments: Vertex, optional random seed
* Fails:     invalid vertex
* Returns:   Status in the form of one of the strings "alive", "dead",
*            "seki", "white_territory", "black_territory", or "dame".
*/
int
QGtp::finalStatus (char c, int i, int seed)
{
    fflush("final_status "+c+intToQByteArray(i)+" "+intToQByteArray(seed));
	return waitResponse();
}

/* Function:  Report vertices with a specific final status in a finished game.
* Arguments: Status in the form of one of the strings "alive", "dead",
*            "seki", "white_territory", "black_territory", or "dame".
*            An optional random seed can be added.
* Fails:     missing or invalid status string
* Returns:   Vertices having the specified status. These are split with
*            one string on each line if the vertices are nonempty (i.e.
*            for "alive", "dead", and "seki").
*/
int
QGtp::finalStatusList (QString status, int seed)
{
    fflush(QByteArray("final_status_list ")+status.toLatin1()+" "+intToQByteArray(seed));
	return waitResponse();
}

/**************
* score *
**************/

/* Function:  Compute the score of a finished game.
* Arguments: Optional random seed
* Fails:     never
* Returns:   Score in SGF format (RE property).
*/
int
QGtp::finalScore (int seed)
{
    fflush("final_score "+intToQByteArray(seed));
	return waitResponse();
}

int
QGtp::estimateScore ()
{
    fflush("estimate_score");
	return waitResponse();
}

int
QGtp::newScore ()
{
    fflush("new_score");
	return waitResponse();
}

/**************
* statistics *
**************/

/* Function:  Reset the count of life nodes.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetLifeNodeCounter ()
{
    fflush("reset_life_node_counter");
	return waitResponse();
}

/* Function:  Retrieve the count of life nodes.
* Arguments: none
* Fails:     never
* Returns:   number of life nodes
*/
int
QGtp::getLifeNodeCounter ()
{
    fflush("get_life_node_counter");
	return waitResponse();
}

/* Function:  Reset the count of owl nodes.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetOwlNodeCounter ()
{
    fflush("reset_owl_node_counter");
	return waitResponse();
}

/* Function:  Retrieve the count of owl nodes.
* Arguments: none
* Fails:     never
* Returns:   number of owl nodes
*/
int
QGtp::getOwlNodeCounter ()
{
    fflush("get_owl_node_counter");
	return waitResponse();
}

/* Function:  Reset the count of reading nodes.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetReadingNodeCounter ()
{
    fflush("reset_reading_node_counter");
	return waitResponse();
}

/* Function:  Retrieve the count of reading nodes.
* Arguments: none
* Fails:     never
* Returns:   number of reading nodes
*/
int
QGtp::getReadingNodeCounter ()
{
    fflush("get_reading_node_counter");
	return waitResponse();
}

/* Function:  Reset the count of trymoves/trykos.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetTrymoveCounter ()
{
    fflush("reset_trymove_counter");
	return waitResponse();
}

/* Function:  Retrieve the count of trymoves/trykos.
* Arguments: none
* Fails:     never
* Returns:   number of trymoves/trykos
*/
int
QGtp::getTrymoveCounter ()
{
    fflush("get_trymove_counter");
	return waitResponse();
}

/*********
* debug *
*********/

/* Function:  Write the position to stderr.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::showboard ()
{
    fflush("showboard");
	return waitResponse();
}

/* Function:  Dump stack to stderr.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::dumpStack ()
{
    fflush("dump_stack");
	return waitResponse();
}

/* Function:  Write information about the influence function to stderr.
* Arguments: color to move, optionally a list of what to show
* Fails:     invalid color
* Returns:   nothing
*/
int
QGtp::debugInfluence (QString color,QString list)
{
    fflush(QByteArray("debug_influence ")+color.toLatin1()+" "+list.toLatin1());
	return waitResponse();
}

/* Function:  Write information about the influence function after making
*            a move to stderr.
* Arguments: move, optionally a list of what to show
* Fails:     never
* Returns:   nothing
*/
int
QGtp::debugMoveInfluence (QString color, char c, int i,QString list)
{
    fflush(QByteArray("debug_move_influence ")+color.toLatin1()+
           " "+c+intToQByteArray(i)+" "+list.toLatin1());
	return waitResponse();
}

/* Function:  Return information about the influence function.
* Arguments: color to move
* Fails:     never
* Returns:   Influence data formatted
*/
int
QGtp::influence (QString color)
{
    fflush(QByteArray("influence ")+color.toLatin1());
	return waitResponse();
}

/* Function:  Return information about the influence function after a move
* Arguments: move
* Fails:     never
* Returns:   Influence data formatted in the same way as for gtp_influence.
*/
int
QGtp::moveInfluence (QString color, char c, int i)
{
    fflush(QByteArray("move_influence ")+color.toLatin1()+" "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Return the information in the worm data structure.
* Arguments: none
* Fails:     never
* Returns:   Worm data formatted
*/
int
QGtp::wormData ()
{
    fflush("worm_data");
	return waitResponse();
}

/* Function:  Return the information in the worm data structure.
* Arguments: vertex
* Fails:     never
* Returns:   Worm data formatted
*/
int
QGtp::wormData (char c, int i)
{
    fflush("worm_data "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Return the cutstone field in the worm data structure.
* Arguments: non-empty vertex
* Fails:     never
* Returns:   cutstone
*/
int
QGtp::wormCutstone (char c, int i)
{
    fflush("worm_cutstone "+c+intToQByteArray(i));
	return waitResponse();
}

/* Function:  Tune the parameters for the move ordering in the tactical
*            reading.
* Arguments: MOVE_ORDERING_PARAMETERS integers
* Fails:     incorrect arguments
* Returns:   nothing
*/
int
QGtp::tuneMoveOrdering (int MOVE_ORDERING_PARAMETERS)
{
    fflush("tune_move_ordering "+intToQByteArray(MOVE_ORDERING_PARAMETERS));
	return waitResponse();
}

/* Function:  Echo the parameter
* Arguments: string
* Fails:     never
* Returns:   nothing
*/
int
QGtp::echo (QString param)
{
    fflush(QByteArray("echo ")+param.toLatin1());
	return waitResponse();
}

/* Function:  List all known commands
* Arguments: none
* Fails:     never
* Returns:   list of known commands, one per line
*/
int
QGtp::help ()
{
    fflush("help");
	return waitResponse();
}

/* Function:  evaluate wether a command is known
* Arguments: command word
* Fails:     never
* Returns:   true or false
*/
int
QGtp::knownCommand (QString s)
{
    fflush(QByteArray("known_command ")+s.toLatin1());
    return waitResponse();
}

QByteArray QGtp::encodeCoors(int x, int y)
{
    if (x > 8)
        x++;
    QByteArray result;
    char c1 = x - 1 + 'A';
    result.append(c1);
    result.append(QString().setNum(y).toLatin1());
    return result;
}

QByteArray QGtp::intToQByteArray(int i)
{
    return QString().setNum(i).toLatin1();
}

/* Function:  Turn uncertainty reports from owl_attack
*            and owl_defend on or off.
* Arguments: "on" or "off"
* Fails:     invalid argument
* Returns:   nothing
*/
int
QGtp::reportUncertainty (QString s)
{
    fflush(QByteArray("report_uncertainty ")+s.toLatin1());
	return waitResponse();
}

/* Function:  List the stones of a worm
* Arguments: the location
* Fails:     if called on an empty or off-board location
* Returns:   list of stones
*/
int
QGtp::wormStones()
{
    fflush("worm_stones");
	return waitResponse();
}

int
QGtp::shell(QString s)
{
    fflush(s.toLatin1());
	return waitResponse();
}

