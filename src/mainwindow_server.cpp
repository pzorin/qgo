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


#include "globals.h"
#include "mainwindow.h"
#include "parser.h"
#include "talk.h"
#include "gamedialog.h"

/*
 * return pressed in edit line -> command to send
 */
void MainWindow::slot_cmdactivated(const QString &cmd)
{
	if (cmd.trimmed().isEmpty())
		return;

	qDebug("cmd_valid: %i", (int)cmd_valid);
	// check if valid cmd -> cmd number risen
//	if (cmd_valid)
//	{
		// clear field, and restore the blank line at top
//		ui.cb_cmdLine->removeItem(1);
//		ui.cb_cmdLine->insertItem(0,"");
		ui.cb_cmdLine->clearEditText();
 
		// echo & send
		QString cmdLine = cmd;
		sendcommand(cmdLine.trimmed(),true);
		cmd_valid = false;

/*		// check for known commands
		if (cmdLine.mid(0,2).contains("ob"))
		{
			QString testcmd;

			qDebug("found cmd: observe");
			testcmd = element(cmdLine, 1, " ");
			if (testcmd)
			{
//				qgoif->set_observe(testcmd);
				sendcommand("games " + testcmd);
			}
		}
*/
		if (cmdLine.mid(0,3).contains("who"))
		{
			// clear table if manually entered
			prepareTables(WHO);
//			playerListSteadyUpdate = false;
		}
//		if (cmdLine.mid(0,5).contains("; \\-1") && myAccount->get_gsname() == IGS)
//		{
//			// exit all channels
//			prepare_tables(CHANNELS);
//		}
//	}
}


/*
 * slot_connect: emitted when connect button has toggled
 */
void MainWindow::slot_connect(bool b)
{
	if (b)
	{
		// create instance of telnetConnection
		if (!igsConnection)
			qFatal("No telnetConnection!");
		
		bool found = FALSE;
		Host *h;
		for (int i=0; i < hostlist.count() && !found ; i++)
		{
			h= hostlist.at(i);
			if (h->title() == ui.cb_connect->currentText())
			{
				found = true;
			}
		}
	
		if (!found)
		{
			qDebug("Problem in hostlist : did not find list title : %s" , ui.cb_connect->currentText().toLatin1().constData());
			ui.pb_connect->setDown(FALSE);
			return;
		}


		// connect to selected host
		if (igsConnection->openConnection(h->address().toLatin1(),
						h->port(),
						h->loginName().toLatin1(),
						h->password().toLatin1()) )
			return;
		else
			ui.pb_connect->setDown(FALSE);
	}
	else
	{
		// disconnect
		igsConnection->closeConnection();
	}
}



/*
 * connection closed - triggered by parser signal
 */
void MainWindow::slot_connexionClosed()
{
	// no Timers in offline mode!
	killTimer(timer);

	// set to offline:
	myAccount->set_offline();
	//pb_connect->setOn(FALSE);
	ui.pb_connect->setChecked(FALSE);
	seekMenu->clear();
	slot_cancelSeek();	

	// clear channel
	prepareTables(CHANNELS);

	prepareTables(WHO);
	prepareTables(GAMES);
	// remove boards
//	qgoif->set_initIF();

//	qDebug("slot_connclosed()");
//	qDebug(QString("%1 -> slot_connclosed()").arg(statusOnlineTime->text()));

	connectSound->play();

	// show current Server name in status bar
	statusServer->setText(" OFFLINE ");

	// set menu
//	Connect->setEnabled(true);
//	Disconnect->setEnabled(false);
//	toolConnect->setOn(false);
	ui.pb_connect->setIcon(QIcon(":/ressources/pics/connect_no2.png"));
//	toolConnect->setPixmap(disconnectedIcon);
//	QToolTip::remove(toolConnect);
	ui.pb_connect->setToolTip( tr("Connect with") + " " + ui.cb_connect->currentText());
}


// used for singleShot actions
void MainWindow::set_tn_ready()
{
	tn_ready = true;
	tn_wait_for_tn_ready = false;
	sendTextFromApp(0);
}

// tell, say, kibitz...
void MainWindow::slot_message(QString txt, QColor c)
{
	ui.MultiLineEdit2->setTextColor(c);
	// Scroll at bottom of text, set cursor to end of line
	if (! (ui.MultiLineEdit2->toPlainText().endsWith('\n') && txt == "\n"))
		ui.MultiLineEdit2->append(txt);

	ui.MultiLineEdit2->setTextColor(Qt::black);
}  


/*
 * text is send by igsconnection signal
 */
void MainWindow::slot_textReceived(const QString &txt)
{

	static int store_sort_col = -1;
	static int store_games_sort_col = -1;
	static bool player7active = false;

	// put text to parser
	InfoType it_  = parser->put_line(txt);

	// some statistics
//	setBytesIn(txt.length()+2);


	// GAME7_END emulation:
	if (player7active && it_ != GAME7)
	{
		player7active = false;
		if (store_games_sort_col != -1)
			ui.ListView_games->sortByColumn(store_games_sort_col);
		store_games_sort_col = -1;

//		ui.ListView_games->sort();
	}

	switch (it_)
	{
		case READY:
			// ok, telnet is ready to receive commands
      			currentCommand->txt.clear();// = QString("");
			if (!tn_wait_for_tn_ready && !tn_ready)
			{
				QTimer::singleShot(200, this, SLOT(set_tn_ready()));
				tn_wait_for_tn_ready = true;
			}
			sendTextFromApp(NULL);
		case WS:
			// ready or white space -> return
			return;
			break;

		// echo entered command
		// echo server enter line
		case CMD:
			slot_message(txt);
			break;

		// set client mode
		case NOCLIENTMODE:
			set_sessionparameter("client", true);
			break;

		case YOUHAVEMSG:
			// normally no connection -> wait until logged in
	        	youhavemsg = true;
			break;

		case SERVERNAME:
			slot_message(txt);
			// clear send buffer
			do
			{
				// enable sending
				set_tn_ready();
			} while (sendTextFromApp(NULL) != 0);

			switch(myAccount->get_gsname())
			{
				case IGS:
				{
					// IGS - check if client mode
//					bool ok;
//					int cmd_nr = element(txt, 0, " ").toInt(&ok);
//					if (!ok)
						set_sessionparameter("client", true);

					// set quiet true; refresh players, games
					set_sessionparameter("quiet", true);
					//else
					// set id - only available if registerd; who knows why...
					sendcommand("id qGo2 " + QString(VERSION), true);
					sendcommand("toggle newrating", true);
					set_sessionparameter("nmatch",true);

					//TODO temporary settings to prevent use of Koryo BY on IGS (as opposed to canadian)
//					sendcommand("nmatchrange BWN 0-9 2-19 60-60 60-3600 25-25 0 0 0-0",false);
					sendNmatchParameters();
//					}
					sendcommand("toggle newundo",true);
//					sendcommand("toggle review",true);
					sendcommand("toggle seek",true);
					sendcommand("seek config_list ",true);
					sendcommand("room",true);
						
					slot_refresh(11);
					slot_refresh(10);
					break;
				}

				
				default:
					set_sessionparameter("client", true);
					set_sessionparameter("quiet", false);
					slot_refresh(11);
					if (myAccount->get_gsname() != CWS)
						slot_refresh(10);
					break;
			}

			// check if tables are sorted
//			if ( ! ui.ListView_players->isSortingEnabled())
				ui.ListView_players->sortItems(12,Qt::AscendingOrder);
//			if ( ! ui.ListView_games->isSortingEnabled())
				ui.ListView_games->sortItems(12,Qt::AscendingOrder);


			// set menu
//			Connect->setEnabled(false);
//			Disconnect->setEnabled(true);
			ui.pb_connect->setChecked(true);
			ui.pb_connect->setIcon(QIcon(":/ressources/pics/connected.png"));
//			QToolTip::remove(pb_connect);
//			QToolTip::add(pb_connect, tr("Disconnect from") + " " + cb_connect->currentText());
			ui.pb_connect->setToolTip(tr("Disconnect from") + " " + ui.cb_connect->currentText());

			// quiet mode? if yes do clear table before refresh
			gamesListSteadyUpdate = ! ui.setQuietMode->isChecked(); 
			playerListSteadyUpdate = ! ui.setQuietMode->isChecked(); 


			// enable extended user info features
			setColumnsForExtUserInfo();

			// check for messages
			if (youhavemsg)
				sendcommand("message", false);

			// let qgo know which server
			qgoif->set_gsName(myAccount->get_gsname());
			// show current Server name in status bar
			statusServer->setText(" " + myAccount->svname + " ");

			// start timer: event every second
			onlineCount = 0;
			timer = startTimer(1000);
			// init shouts
//TODO			slot_talk("Shouts*", 0, true);
			connectSound->play();

			break;

		// end of 'who'/'user' cmd
		case PLAYER42_END:
		case PLAYER27_END:
			// anyway, end of fast filling
			if (store_sort_col != -1)
				ui.ListView_players->sortByColumn(store_sort_col);

			store_sort_col = -1;

			if (myAccount->get_gsname()==IGS)
//				ui.ListView_players->showOpen(whoOpenCheck->isChecked());
				showOpen(ui.whoOpenCheck->isChecked());
//			ListView_players->sort();
			playerListEmpty = false;
			break;

		// skip table if initial table is to be loaded
		case PLAYER27_START:
		case PLAYER42_START:
			// disable sorting for fast operation; store sort column index
			store_sort_col = ui.ListView_players->sortColumn();
//			ui.ListView_players->setSortingEnabled(FALSE);

			if (playerListEmpty)
				prepareTables(WHO);
			break;

		case GAME7_START:
			// "emulate" GAME7_END
			player7active = true;
			// disable sorting for fast operation; store sort column index
			// unfortunately there is not GAME7_END cmd, thus, it's emulated
			if (playerListEmpty)
			{
				// only if playerListEmpty, else PLAYERXX_END would not arise
				store_games_sort_col = ui.ListView_games->sortColumn();
//				ui.ListView_games->setSortingEnabled(FALSE);
			}
			break;

		case ACCOUNT:
			// let qgo and parser know which account in case of setting something for own games
			qgoif->set_myName(myAccount->acc_name);
			parser->set_myName(myAccount->acc_name);
			break;


		case STATS:
			// we just received a players name as first line of stats -> create the dialog tab
			currentCommand->txt="stats";
		
		// if (!talklist.current())
			slot_talk( parser->get_statsPlayer()->name,0,true);
		
			//else if (parser->get_statsPlayer()->name != talklist.current()->get_name())
			//    slot_talk( parser->get_statsPlayer()->name,0,true);
		
			break;

		case BEEP:
//			QApplication::beep();
			break;

		default:
			break;

	}
//	if (DODEBUG)
//	qDebug(txt.toLatin1());

}


// send text via telnet session; skipping empty string!
int MainWindow::sendTextFromApp(const QString &txt, bool localecho)
{

	// implements a simple buffer
	int valid = txt.length();

	// some statistics
//	if (valid)
//		setBytesOut(valid+2);

	if (myAccount->get_status() == OFFLINE)
	{
		// skip all commands while not telnet connection
		slot_textReceived("Command skipped - no telnet connection: " + txt);
		// reset buffer
		sendBuffer.clear();
		return 0;
	}

	// check if telnet ready
	if (tn_ready)
	{
		sendBuf *s = NULL;

		if (!sendBuffer.isEmpty())
			s = sendBuffer.first();

		if (s)
		{
			// send buffer cmd first; then put current cmd to buffer
			igsConnection->sendTextToHost(s->get_txt());
			qDebug("SENDBUFFER send: %s", s->get_txt().toLatin1().constData());

			// hold the line if cmd is sent; 'ayt' is autosend cmd
//			if (s->get_txt().indexOf("ayt") != -1)
//				resetCounter();
			if (s->get_localecho())
				slot_textReceived(CONSOLECMDPREFIX + QString(" ") + s->get_txt());
			tn_ready = false;

			// delete sent command from buffer 
			//currentCommand->txt = s->get_txt();
			sendBuffer.removeFirst();

      
			if (valid)
			{
				// append current command to send as soon as possible
				sendBuffer.append(new sendBuf(txt, localecho));
				qDebug("SENDBUFFER added: %s" , txt.toLatin1().constData());
			}
		}
		else if (valid)
		{
			// buffer empty -> send direct
			igsConnection->sendTextToHost(txt);
      //currentCommand->txt = txt;
      
//TODO			if (!txt.contains("ayt"))
//				resetCounter();
			if (localecho)
				slot_textReceived(CONSOLECMDPREFIX + QString(" ") + txt);
			tn_ready = false;

			qDebug("SENDBUFFER send direct: %s" , txt.toLatin1().constData());
		}
	}
	else if (valid)
	{
//qDebug("SENDBUFFER added: " + txt);
		sendBuffer.append(new sendBuf(txt, localecho));
	}

	return sendBuffer.count();

}

// set session parameter
void MainWindow::set_sessionparameter(QString par, bool val)
{
	QString value;
	if (val)
		value = " true";
	else
		value = " false";

	switch(myAccount->get_gsname())
	{
		// only toggling...
		case IGS:
			sendcommand("toggle " + par + value);
			break;
			
		default:
			sendcommand("set " + par + value);
			break;
	}
}


void MainWindow::slot_sendCommand(const QString &cmd, bool localecho)
{
	localecho = true;
	sendcommand( cmd,  localecho);
}


// show command, send it, and tell parser
void MainWindow::sendcommand(const QString &cmd, bool localecho)
 {
	QString testcmd = cmd;

	// for testing
	if (cmd.indexOf("#") == 0)
	{
		qDebug("detected TEST (#) command");
		testcmd = testcmd.remove(0, 1).trimmed();

		// help
		if (testcmd.length() <= 1)
		{
			slot_textReceived("local cmds available:\n"
				"#+dbgwin\t#+dbg\t\t#-dbg\n");
			return;
		}

		// detect internal commands
		if (testcmd.contains("+dbgwin"))
		{
			// show debug window
//TODO deal with all this			DD->show();
//			this->setActiveWindow();
		}
		else if (testcmd.contains("+dbg"))
		{
			// show debug window and activate debug mode
			qDebug("*** set Debug on ***");
//			DD->show();
//			DODEBUG = true;
//			this->setActiveWindow();
		}
		else if (testcmd.contains("-dbg"))
		{
			// hide debug window and deactivate debug mode
			qDebug("*** set Debug off ***");
//			DODEBUG = false;
//			DD->hide();
		}

		slot_textReceived(testcmd);
		return;
	}

	// echo
	if (localecho)
	{
		// add to Messages, anyway
		// Scroll at bottom of text, set cursor to end of line
//		if (DODEBUG)
//			qDebug(cmd);
		slot_message(cmd,Qt::blue);
	}

	// send to Host
	sendTextFromApp(cmd, localecho);
}

/*
 * Shows only the 'open' players in the list
 */
void MainWindow::showOpen(bool show)
{
	QTreeWidgetItemIterator lv(ui.ListView_players);
/*	
	for (; lv.current() ;lv++)
	{
		// player is not open or is playing a match
		if ((lv.current()->text(0).contains('X')) || (!lv.current()->text(3).contains('-')))
			lv.current()->setVisible(!show);
		
	}
*/

	while (*lv) 
	{
		// player is not open or is playing a match
		if (((*lv)->text(0).contains('X')) || (!(*lv)->text(3).contains('-')))
			(*lv)->setHidden(show);		
        	++lv;
	}

}

/*
 * room list clicked
 */
void MainWindow::slot_roomListClicked(const QString& text)
{
	QString room;

	if (text == QObject::tr("Lobby"))
		room = "0";
	else
		room = text.section(":",0,0);

	sendcommand("join " + room);	
	
/*	if (room == "0")
		statusBar()->message(tr("rooms left"));
	else
		statusBar()->message(tr("Room ")+ room);
*/	
	//refresh the players table
	slot_refresh(0);	
}





/*
 * refresh games button clicked
 */
void MainWindow::slot_RefreshGames()
{
//	if (gamesListSteadyUpdate)
//		slot_refresh(1);
//	else
//	{
		// clear table in case of quiet mode
		slot_refresh(11);
//		gamesListSteadyUpdate = !setQuietMode->isOn(); 
//	}
}

/*
 * refresh players button clicked
 */
void MainWindow::slot_RefreshPlayers()
{
//	if (playerListSteadyUpdate)
//		slot_refresh(0);
//	else
//	{
		// clear table in case of quiet mode
		slot_refresh(10);
//		playerListSteadyUpdate = !setQuietMode->isOn(); 
//	}
}


/*
 * refresh lists
 */
void MainWindow::slot_refresh(int i)
{

	slot_setRankSpread();

	QString wparam =  rkMax + "-" + rkMin ;
	// refresh depends on selected page
	switch (i)
	{
		case 10:
			prepareTables(WHO);
		case 0:
			// send "WHO" command
      			//set the params of "who command"
/*			if ((ui.whoBox1->currentIndex() >1)  || (ui.whoBox2->currentIndex() >1))
        		{
				wparam.append(ui.whoBox1->currentIndex()==1 ? "9p" : ui.whoBox1->currentText());
				if ((ui.whoBox1->currentIndex())  && (ui.whoBox2->currentIndex()))
					wparam.append("-");

				wparam.append(ui.whoBox2->currentIndex()==1 ? "9p" : ui.whoBox2->currentText());
         		} 
			else if ((ui.whoBox1->currentIndex())  || (ui.whoBox2->currentIndex()))
        			wparam.append("1p-9p");
			else
				wparam.append(((myAccount->get_gsname() == IGS) ? "9p-BC" : " "));
*/				

			if (ui.whoOpenCheck->isChecked())
				wparam.append(((myAccount->get_gsname() == WING) ? " O" : " o"));//wparam.append(" o");

			if (myAccount->get_gsname() == IGS )//&& extUserInfo)
				sendcommand(wparam.prepend("userlist "));
			else
				sendcommand(wparam.prepend("who "));

      			prepareTables(WHO);
			break;

		case 11:
			prepareTables(GAMES);
		case 1:
			// send "GAMES" command
			sendcommand("games");
//			prepare_tables(GAMES);
			// which games are watched right now
			// don't work correct at IGS !!!!
//			sendcommand("watching");
			break;

		default:
			break;
	}

}

// prepare tables (clear, ...)
void MainWindow::prepareTables(InfoType cmd)
{
	switch (cmd)
	{
		case WHO: // delete player table
		{
//			QListViewItemIterator lv(ListView_players);
//			for (QListViewItem *lvi; (lvi = lv.current());)
//			{
//				lv++;
//				delete lvi;
//			}
			ui.ListView_players->clear();

			// set number of players to 0
			myAccount->num_players = 0;
			myAccount->num_watchedplayers = 0;
			// use this for fast filling
			playerListEmpty = true;
			statusUsers->setText(" P: 0");// / 0 ");
			break;
		}

		case GAMES: // delete games table
		{
//			QListViewItemIterator lv(ListView_games);
//			for (QListViewItem *lvi; (lvi = lv.current());)
//			{
//				lv++;
//				delete lvi;
//			}
			ui.ListView_games->clear();

			// set number of games to 0
			myAccount->num_games = 0;
			statusGames->setText(" G: 0");         //(" G: 0 / 0 ");
			break;
		}

		case CHANNELS:
		{
			// delete channel info
//			channellist.clear();
//			statusChannel->setText("");

			// delete tooltips too
//			QToolTip::remove(statusChannel);
//			QToolTip::add(statusChannel, tr("Current channels and users"));

			//We prepare the rooms list as well here
			while (ui.RoomList->count() > 1)
     				ui.RoomList->removeItem(1);

			break;
		}

		default: // unknown command???
			qWarning("unknown Command in 'prepare_tables()'");
			break;

	}
}

/*
 * sets the columns for specific IGS infos
 */
void MainWindow::setColumnsForExtUserInfo()
{
	
	if (/* !extUserInfo || */(myAccount->get_gsname() != IGS) )
	{
		// set player table's columns to 'who' mode
		ui.ListView_players->setColumnHidden(11,TRUE);
		ui.ListView_players->setColumnHidden(10,TRUE);
		ui.ListView_players->setColumnHidden(9,TRUE);
		ui.ListView_players->setColumnHidden(8,TRUE);
		ui.ListView_players->setColumnHidden(7,TRUE);
	}
	else// if ( ui.ListView_players->columns()  < 9 )  
	{
		// set player table's columns to 'user' mode
		// first: remove invisible column
//		ui.ListView_players->removeColumn(7);
		// second: add new columns
//		ui.ListView_players->addColumn(tr("Info"));
//		ui.ListView_players->addColumn(tr("Won"));
//		ui.ListView_players->addColumn(tr("Lost"));
//		ui.ListView_players->addColumn(tr("Country"));
//		ui.ListView_players->addColumn(tr("Match prefs"));
//		ui.ListView_players->setColumnAlignment(7, Qt::AlignRight);
//		ui.ListView_players->setColumnAlignment(8, AlignRight);
//		ui.ListView_players->setColumnAlignment(9, AlignRight);
		ui.ListView_players->setColumnHidden(11,FALSE);
		ui.ListView_players->setColumnHidden(10,FALSE);
//		ui.ListView_players->setColumnHidden(9,FALSE);
//		ui.ListView_players->setColumnHidden(8,FALSE);
//		ui.ListView_players->setColumnHidden(7,FALSE);
	}
}

/*
 * server name found by parser
 */
void MainWindow::slot_svname(GSName &gs)
{
	// save local at 'gsname'
	// and change caption
	myAccount->set_gsname(gs);
	myAccount->set_caption();
}

/*
 * account name found by parser
 */
void MainWindow::slot_accname(QString &name)
{
	// save local at 'gsname'
	// and change caption
	myAccount->set_accname(name);
	myAccount->set_caption();
}


/*
 * 'seek' time condition was received from parser
 */
void MainWindow::slot_addSeekCondition(const QString& a, const QString& b, const QString& c, const QString& d, const QString& )
{
	QString time_condition ;
	
	time_condition = QString::number(int(b.toInt() / 60)) + " min + " + QString::number(int(c.toInt() / 60)) + " min / " + d + " stones";

	QAction *act = seekMenu->addAction(time_condition);// , this, SLOT(slot_seek(QAction*)));//, 0, a.toInt());
	act->setData(a.toInt());
}

/*
 * seek header received after 'seek config_list' command
 */
void MainWindow::slot_clearSeekCondition()
{
	seekMenu->clear();
}

/* 
 * seek request is being canceled
 */
void MainWindow::slot_cancelSeek()
{

	ui.toolSeek->setChecked(FALSE);
	ui.toolSeek->setMenu(seekMenu);
//	ui.toolSeek->setPopupDelay(1);
	ui.toolSeek->setIcon(QIcon(":/ressources/pics/not_seeking.png"));
	killTimer(seekButtonTimer);
	seekButtonTimer = 0;

}


/*
 * the 'seek' button was pressed
 */
void MainWindow::slot_seek(bool b)
{
//qDebug("seek button pressed : status %i", (int)b);

	//if the button was just pressed on, we have already used the popup menu : nothing to do
	if (b)
		return;

	sendcommand("seek entry_cancel",false);
}


/*
 * seek button : menu entry (time conditions) selected
 */
void MainWindow::slot_seek(QAction *act)
{
	int i = act->data().toInt();
	ui.toolSeek->setChecked(true);
	ui.toolSeek->setMenu(NULL);

	//seek entry 1 19 5 3 0
	QString send_seek = 	"seek entry " + 
				QString::number(i) + 
				" 19 " ;

	//childish, but we want to have an animated icon there
	seekButtonTimer = startTimer(200);

	switch (ui.seekHandicapList->currentIndex())
	{
		case 0 :
			send_seek.append("1 1 0");
			break ;

		case 1 :
			send_seek.append("2 2 0");
			break ;
		
		case 2 :
			send_seek.append("5 5 0");
			break ;

		case 3 :
			send_seek.append("9 9 0");
			break ;

		case 4 :
			send_seek.append("0 9 0");
			break ;	

		case 5 :
			send_seek.append("9 0 0");
			break ;
	}
	
	sendcommand(send_seek,false);
	qDebug(send_seek.toLatin1().constData());
}

/*
 * seek entry received from parser
 */
void MainWindow::slot_seekList(const QString& player, const QString& condition)
{
	QString Rk;

	QTreeWidgetItemIterator lv(ui.ListView_players);
	QTreeWidgetItem *lvi;
	for (; (*lv); lv++)
	{
		lvi = *lv;
		// compare names
		if (lvi->text(1) == player)
		{
			Rk = lvi->text(2);
			break;
		}
	}


//	QString Rk = getPlayerRk(player);
	QString hop = Rk.right(1);
	if ((Rk.right(1) != "?") && (Rk.right(1) != "+"))
		Rk.append(" ");

	slot_message(player.leftJustified(15,' ',true) + Rk.rightJustified(5) +  " : " + condition, Qt::darkRed);

}

/*
 * room string received after 'room' command
 */
void MainWindow::slot_room(const QString& room, bool b)
{
	//do we already have the same room number in list ?
	if (ui.RoomList->findText(room.left(3), Qt::MatchStartsWith )!= -1)
		return;
	//so far, we just create the room if it is open
	if (!b)
		ui.RoomList->addItem (room);//,  -1 );
	
	//RoomList->item(RoomList->count()-1)->setSelectable(!b);		
}	


/*
 * game info received after 'games' command
 */
void MainWindow::slot_game(Game* g)
{
	// insert into ListView
	QTreeWidgetItemIterator lv(ui.ListView_games);

	if (g->running)
	{
		bool found = false;
		QTreeWidgetItem *lvi_mem = NULL;

		// check if game already exists
		if (!playerListEmpty)
		{
			QTreeWidgetItemIterator lvii = lv;
//			for (GamesTableItem *lvi; (lvi = static_cast<GamesTableItem*>(lvii.current())) && !found;)
//			{
//				lvii++;
//				// compare game id
//				if (lvi->text(0) == g->nr)
//				{
//					found = true;
//					lvi_mem = lvi;
//				}
//			}

			while (*lvii) 
			{
        			if ((*lvii)->text(0) == g->nr)
				{
					found = true;
					lvi_mem = *lvii;
				}
         			++lvii;
     			}


		}
		else if (g->H.isEmpty() && !myAccount->num_games)
		{
			// skip games until initial table has loaded
			qDebug("game skipped because no init table");
			return;
		}
/*
		QString excludeMark = "";
		QString myMark = "B";
		
		// check if exclude entry is done later
		if (g->H) //g->status.length() > 1)
		{
			QString emw;
			QString emb;

			// no: do it now
			emw = getPlayerExcludeListEntry(g->wname);
			emb = getPlayerExcludeListEntry(g->bname);

			// ensure that my game is listed first
			if (emw && emw == "M" || emb && emb == "M")
			{
				myMark = "A";
				excludeMark = "M";

				// I'm playing, thus I'm open, except teaching games
				if (emw && emw && (emw != "M" || emb != "M"))
				{
					// checkbox open
					slot_checkbox(0, true);
				}
			}
			else if (emw && emw == "W" || emb && emb == "W")
			{
				excludeMark = "W";
			}
		}
*/		
		if (found)
		{
			// supersede entry
			//lvi_mem->setText(0, g->nr);
			lvi_mem->setText(1, g->wname);
			lvi_mem->setText(2, g->wrank);
			lvi_mem->setText(3, g->bname);
			lvi_mem->setText(4, g->brank);
			lvi_mem->setText(5, g->mv);
			lvi_mem->setText(6, g->Sz);
			lvi_mem->setText(7, g->H);
			lvi_mem->setText(8, g->K);
			lvi_mem->setText(9, g->By);
			lvi_mem->setText(10, g->FR);
			lvi_mem->setText(11, g->ob);
//			lvi_mem->setText(6, g->status + " (" + g->ob + ")");
			lvi_mem->setText(12, /*myMark +*/ rkToKey(g->wrank) + g->wname.toLower()  /*+ ":" + excludeMark */);
			lvi_mem->setText(13,  rkToKey(g->brank) + g->bname.toLower()  /*+ ":" + excludeMark */);

//			lvi_mem->ownRepaint();
		}
		else
		{
			// from GAMES command or game info{...}
			QStringList sl;	

//			if (!g->H.isEmpty())
//			{
//				lv = new QTreeWidgetItem(ListView_games,
				sl 	<< g->nr
					<< " " + g->wname
					<< g->wrank
					<< " " + g->bname
					<< g->brank
					<< g->mv
					<< g->Sz
					<< g->H
					<< g->K
					<< g->By
					<< g->FR
					<< g->ob
					<< rkToKey(g->wrank) + g->wname.toLower() 
					<< rkToKey(g->brank) + g->bname.toLower();

//			}
//			else
//			{
//				lv = new QTreeWidgetItem(ListView_games,
//				sl 	<< g->nr
//					<< " " + g->wname
//					<< g->wrank
//					<< " " + g->bname
//					<< g->brank
//					<< g->mv
//					<< g->Sz;



//			}
			QTreeWidgetItem *lvi = new QTreeWidgetItem(ui.ListView_games, sl);

//			lvi->setText(12, myMark + rkToKey(g->wrank) + g->wname.toLower() + ":" + excludeMark);
//			lvi->setText(13, myMark + rkToKey(g->brank) + g->bname.toLower() + ":" + excludeMark);

//			lvi->setText(12, rkToKey(g->wrank) + g->wname.toLower() );
//			lvi->setText(13, rkToKey(g->brank) + g->bname.toLower() );

			lvi->setTextAlignment(0,Qt::AlignRight);
			lvi->setTextAlignment(5,Qt::AlignRight);
			lvi->setTextAlignment(6,Qt::AlignRight);
			lvi->setTextAlignment(7,Qt::AlignRight);
			lvi->setTextAlignment(8,Qt::AlignRight);
			lvi->setTextAlignment(9,Qt::AlignRight);
			lvi->setTextAlignment(10,Qt::AlignRight);
			lvi->setTextAlignment(11,Qt::AlignRight);

//			static_cast<GamesTableItem*>(lv.current())->ownRepaint();

			// increase number of games
			myAccount->num_games++;
			statusGames->setText(" G: " + QString::number(ui.ListView_games->topLevelItemCount()));//myAccount->num_games) + " / " + QString::number(myAccount->num_observedgames) + " ");
		}
/*
		// update player info if this is not a 'who'-result or if it's me
		if (!g->H || myMark == "A") //g->status.length() < 2)
		{
			QListViewItemIterator lvp(ListView_players);
			QListViewItem *lvpi;
			int found = 0;

			// look for players in playerlist
			for (; (lvpi = lvp.current()) && found < 2;)
			{
				// check if names are identical
				if (lvpi->text(1) == g->wname || lvpi->text(1) == g->bname)
				{
					lvpi->setText(3, g->nr);
					found++;

					// check if players has a rank
					if (g->wrank == "??" || g->brank == "??")
					{
						// no rank given in case of continued game -> set rank in games table
						if (lvpi->text(1) == g->wname)
						{
							lv.current()->setText(2, lvpi->text(2));
							// correct sorting of col 2 -> set col 12
							lv.current()->setText(12, myMark + rkToKey(lvpi->text(2)) + lvpi->text(1).lower() + ":" + excludeMark);
						}

						// no else case! bplayer could be identical to wplayer!
						if (lvpi->text(1) == g->bname)
							lv.current()->setText(4, lvpi->text(2));

						static_cast<GamesTableItem*>(lv.current())->ownRepaint();
					}
				}

				lvp++;
			}
			ListView_games->sort();
		}
*/
	}
	else
	{
		// from game info {...}
		bool found = false;
		QString game_id;

		if (g->nr != "@")
		{
//			for (QListViewItem *lvi; (lvi = lv.current()) && !found;)
//			{
//				lv++;
//				// compare game id
//				if (lvi->text(0) == g->nr)
//				{
//					lv++;
//					delete lvi;
//					found = true;;
//				}
//			}
			while (*lv) 
			{
        			if ((*lv)->text(0) == g->nr)
				{
					found = true;
					delete *lv;
				}
         			++lv;
     			}

			// used for player update below
			game_id = g->nr;
		}
		else
		{
//			for (QListViewItem *lvi; (lvi = lv.current()) && !found;)
//			{
//				lv++;
//				// look for name
//				if (lvi->text(1) == myAccount->acc_name ||
//				    lvi->text(3) == myAccount->acc_name)
//				{
					// used for player update below
//					game_id = lvi->text(0);

//					lv++;
//					delete lvi;
//					found = true;;
//				}
			while (*lv) 
			{
        			if ((*lv)->text(1) == myAccount->acc_name ||
				    (*lv)->text(3) == myAccount->acc_name)
				{
					found = true;
					delete *lv;
				}
         			++lv;
     			}

		}

		if (!found)
			qWarning("game not found");
		else
		{
			// decrease number of games
			myAccount->num_games--;
			statusGames->setText(" G: " + QString::number(ui.ListView_games->topLevelItemCount()));
//			statusGames->setText(" G: " + QString::number(myAccount->num_games) + " / " + QString::number(myAccount->num_observedgames) + " ");
/*
			QTreeWidgetItemIterator lvp(ListView_players);
			QListViewItem *lvpi;
			int found = 0;

			// look for players in playerlist
			for (; (lvpi = lvp.current()) && found < 2;)
			{
				// check if numbers are identical
				if (lvpi->text(3) == game_id)
				{
					lvpi->setText(3, "-");
					found++;
				}

				lvp++;
			}
*/
		}
	}
}


/*
 * take a new player from parser
 */
void MainWindow::slot_player(Player *p, bool cmdplayers)
{
	// insert into ListView

  	QTreeWidgetItemIterator lv(ui.ListView_players);  

	QPoint pp(0,0);
//	QTreeWidgetItem *topViewItem = ui.ListView_players->itemAt(pp);
	PlayerTableItem *topViewItem = (PlayerTableItem *)ui.ListView_players->itemAt(pp);

  	bool deleted_topViewItem = false;
//  	QTreeWidgetItem *lvi;
	PlayerTableItem *lvi;

	if (p->online)
	{
		// check if it's an empty list, i.e. all items deleted before
		if (cmdplayers && !playerListEmpty)
		{
			for (; (*lv); lv++)
			{
//				lv++;
				lvi = (PlayerTableItem *)*lv;
				// compare names
				if (lvi->text(1) == p->name)
				{
					// check if new player info is less than old
					if (p->info != "??")
					{
						// new entry has more info
						lvi->setText(0, p->info);
						//p->name,
						lvi->setText(2, p->rank);
						lvi->setText(3, p->play_str);
						lvi->setText(4, p->obs_str);
						lvi->setText(5, p->idle);
						//mark,
						lvi->setText(12, rkToKey(p->rank) + p->name.toLower());

						if (/*extUserInfo &&*/ myAccount->get_gsname() == IGS)
						{
							lvi->setText(7, p->extInfo);
							lvi->setText(8, p->won);
							lvi->setText(9, p->lost);
							lvi->setText(10, p->country);
							lvi->setText(11, p->nmatch_settings);
						}
						lvi->set_nmatchSettings(p);					
						//lvi->nmatch = p->nmatch;

//						lvi->ownRepaint();
					}

					if (p->name == myAccount->acc_name)
					{
						qDebug("updating my account info... (1)");
						// checkbox open
						bool b = (p->info.contains('X') == 0);
						slot_checkbox(0, b);
						// checkbox looking - don't set if closed
						if (p->info.contains('!') != 0)
							// "!" found
							slot_checkbox(1, true);
						else if (b)
							// "!" not found && open
							slot_checkbox(1, false);
						// checkbox quiet
						// NOT CORRECT REPORTED BY SERVER!
						//b = (p->info.contains('Q') != 0);
						//slot_checkbox(2, b);
						// -> WORKAROUND
						if (p->info.contains('Q') != 0)
							slot_checkbox(2, true);

						// get rank to calc handicap when matching
						myAccount->set_rank(p->rank);

						for (int i = 0; i < lvi->columnCount(); i++)
							lvi->setForeground(i, QBrush::QBrush(Qt::blue));

					}

					return;
				}
			}
		}
		else if (!cmdplayers && !myAccount->num_players)
		{
			qDebug("player skipped because no init table");
			// skip players until initial table has loaded
			return;
		}


		QString mark;
/*
		// check for watched players
		if (watch && watch.contains(";" + p->name + ";"))
		{
			mark = "W";

			// sound for entering - no sound while "who" cmd is executing
			if (!cmdplayers)
				qgoif->get_qgo()->playEnterSound();
			else if (p->name == myAccount->acc_name)
				// it's me
				// - only possible if 'who'/'user' cmd is executing
				// - I am on the watchlist, however
				// don't count!
				myAccount->num_watchedplayers--;

			myAccount->num_watchedplayers++;
		}
		// check for excluded players
		else if (exclude && exclude.contains(";" + p->name + ";"))
		{
			mark = "X";
		}
*/

		QStringList sl;
		// from WHO command or {... has connected}
//		if (/*extUserInfo && */myAccount->get_gsname() == IGS)
//		{
			
			
//			PlayerTableItem *lv1 = new PlayerTableItem(ListView_players,
			sl	<<	p->info
				<<	p->name
				<<	p->rank
				<<	p->play_str
				<<	p->obs_str
				<<	p->idle
				<<	mark
				<<	p->extInfo
				<<	p->won
				<<	p->lost
				<<	p->country
				<<	p->nmatch_settings
				<<	rkToKey(p->rank) + p->name.toLower();
//			lv1->set_nmatchSettings(p);
//		}
//		else
//		{
//			PlayerTableItem *lv1 = new PlayerTableItem(ListView_players,
//			sl	<<	p->info
//				<<	p->name
//				<<	p->rank
//				<<	p->play_str
//				<<	p->obs_str
//				<<	p->idle
//				<<	mark << "" << "" << "" << "" << "" << rkToKey(p->rank) + p->name.toLower();;
//			lv1->setText(12, rkToKey(p->rank) + p->name.lower());
//			lv1->set_nmatchSettings(p);
//		}
		
//		lvi = new QTreeWidgetItem(ui.ListView_players, sl);
		lvi = new PlayerTableItem(ui.ListView_players, sl);
		lvi->set_nmatchSettings(p);

		lvi->setTextAlignment(0,Qt::AlignRight);
		lvi->setTextAlignment(3,Qt::AlignRight);
		lvi->setTextAlignment(4,Qt::AlignRight);
		lvi->setTextAlignment(5,Qt::AlignRight);
		lvi->setTextAlignment(6,Qt::AlignRight);
		lvi->setTextAlignment(7,Qt::AlignRight);
		lvi->setTextAlignment(8,Qt::AlignRight);
		lvi->setTextAlignment(9,Qt::AlignRight);

		// check for open/looking state
		if (cmdplayers)
		{
			if (p->name == myAccount->acc_name)
			{
				qDebug("updating my account info...(2)");
				// checkbox open
				bool b = (p->info.contains('X') == 0);
				slot_checkbox(0, b);
				// checkbox looking
				b = (p->info.contains('!') != 0);
				slot_checkbox(1, b);
				// checkbox quiet
				// NOT CORRECT REPORTED BY SERVER!
				//b = (p->info.contains('Q') != 0);
				//slot_checkbox(2, b);
				// -> WORKAROUND
				if (p->info.contains('Q') != 0)
					slot_checkbox(2, true);

				// get rank to calc handicap when matching
				myAccount->set_rank(p->rank);
				mark = "M";

				for (int i = 0; i < lvi->columnCount(); i++)
					lvi->setForeground(i, QBrush::QBrush(Qt::blue));


			}
		}

		// increase number of players
		myAccount->num_players++;
		statusUsers->setText(" P: " + QString::number(ui.ListView_players->topLevelItemCount()));
//		statusUsers->setText(" P: " + QString::number(myAccount->num_players) + " / " + QString::number(myAccount->num_watchedplayers) + " ");
		

		//if (!cmdplayers)
		//	ListView_players->sort() ;

	}
	else
	{
		// {... has disconnected}
		bool found = false;
//		for (QListViewItem *lvi; (lvi = lv.current()) && !found;)
		for (; (*lv); lv++)
		{
//			lv++;
			lvi = (PlayerTableItem *)*lv;
			// compare names
			if (lvi->text(1) == p->name)
			{
/*				// check if it was a watched player
				if (lvi->text(6) == "W")
				{
					qgoif->get_qgo()->playLeaveSound();
					myAccount->num_watchedplayers--;
				}
*/
				lv++;
				if (lvi == topViewItem)     // are we trying to delete the 'anchor' of the list viewport ?
					deleted_topViewItem = true  ;
				delete lvi;
				found = true;;

				// decrease number of players
				myAccount->num_players--;
				statusUsers->setText(" P: " + QString::number(ui.ListView_players->topLevelItemCount()));
//				statusUsers->setText(" P: " + QString::number(myAccount->num_players) + " / " + QString::number(myAccount->num_watchedplayers) + " ");
			}
		}

		if (!found)
			qWarning(QString("disconnected player not found: " + p->name).toLatin1());
	}

//	if (! deleted_topViewItem) //don't try to refer to a deleted element ...
//	{
//		int ip = topViewItem->itemPos();
//		ListView_players->setContentsPos(0,ip);
//	}
}





// convert rk to key for sorting;
// if integer is true, then result can be used to calculate handicap
QString MainWindow::rkToKey(QString txt, bool integer)
{
	QString rk = txt;
	QString keyStr;

	if (integer)
	{
		// NR
		if (rk == "NR") 
			return "0000";
		
		// BC ( IGS new rating stops at 23 k = BC )
		if (rk == "BC") 
			return "0800";

		// get number
		QString buffer = rk;
		buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
		bool ok;
		int pt = buffer.toInt(&ok);
		if (!ok)
			return "0000";

		// check for k,d,p
		if (rk.indexOf("k") != -1)
		{
			pt = (31 - pt)*100 ;//+ ( (rk.find("+") != -1) ? 10:0) ;
		}
		else if (rk.indexOf("d") != -1)
		{
			/* Former code replaced 
			// 7d == 1p
			if (pt > 7)
				pt = 3670 + (pt - 6)*30 + ((rk.find("+") != -1) ? 10:0);
			else
				pt = 3000 + pt*100 + ((rk.find("+") != -1) ? 10:0);
			*/

			/* New formula */
			pt = 3000 + pt*100 ;//+ ((rk.find("+") != -1) ? 10:0);
		}
		else if (rk.indexOf("p") != -1)
		{
			
			/* Former code replaced 
			// 7d == 1p
			pt = 3670 + pt*30; 
			*/
			/* New formula : still 7d ~ 1p */
			pt = 3600 + pt*100 ;//+ ((rk.find("+") != -1) ? 10:0);
		
		}
		else
			return "0000";

		buffer = QString::number(pt).rightJustified(4, '0');
		return buffer;
	}
	else
	{
		// NR
		if (rk == "NR" || rk == "BC")
			return "nr";
		
		// check for k,d,p
		if (rk.indexOf("k") != -1)
			keyStr = "c";
		else if (rk.indexOf("d") != -1)
			keyStr = "b";
		else if (rk.indexOf("p") != -1)
			keyStr = "a";
		else
			keyStr = "z";
		
		// get number
		QString buffer = rk;
		buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
		if (buffer.length() < 2)
		{
			keyStr += "0";
			
			// reverse sort order for dan/pro players
			if ((keyStr == "a0") || (keyStr == "b0"))
			{
				int i = buffer.toInt();
				i = 10 - i;
				buffer = QString::number(i);
			}
		}

		return keyStr + buffer;
	}
}


 // handle chat boxes in a list
void MainWindow::slot_talk(const QString &name, const QString &text, bool isplayer)
{
	static Talk *dlg;
	QString txt;
//	bool bonus = false;
//	bool autoAnswer = true;

//	if (text && text != "@@@")
		// text given or player logged in
		txt = text;
//	else if (text == "@@@" && isplayer)
//	{
		// player logged out -> iplayer == false
//		txt = tr("USER NOT LOGGED IN.");
//		autoAnswer = false;
//	}
//	else
//	{
//		txt = "";
//		autoAnswer = false;
//	}

	// dialog recently used?
	if (dlg && dlg->get_name() == name)
		dlg->write(txt);
	else if (!name.isEmpty() && name != tr("msg*"))
	{
		// seek dialog
		dlg = 0;
//		dlg = talkList.first();
//		while (dlg != NULL && dlg->get_name() != name)
//			dlg = talkList.next();
		for (int i=0; i<talkList.count(); i++)
		{
//			dlg = talkList.at(i);
			if (talkList.at(i)->get_name() == name)
				dlg = talkList.at(i);
//				break;
		}
		// not found -> create new dialog
		if (!dlg)
		{
			dlg = new Talk(name, 0, isplayer);
			talkList.insert(0, dlg);     
//			dlg = talkList.current();
			connect(dlg, SIGNAL(signal_talkTo(QString&, QString&)), this, SLOT(slot_talkTo(QString&, QString&)));
			connect(dlg, SIGNAL(signal_matchRequest(const QString&,bool)), this, SLOT(slot_matchRequest(const QString&,bool)));
      
			// make new multiline field
			ui.talkTabs->addTab(dlg->get_tabWidget(), dlg->get_name());
			
			if (name != tr("Shouts*"))
				ui.talkTabs->setCurrentWidget(dlg->get_tabWidget());
				
			dlg->pageActive = true;
			connect(dlg->get_le(), SIGNAL(returnPressed()), dlg, SLOT(slot_returnPressed()));
			connect(dlg, SIGNAL(signal_pbRelOneTab(QWidget*)), this, SLOT(slot_pbRelOneTab(QWidget*)));

//			QPalette pal = dlg->get_mle()->palette();
//			pal.setColor(QColorGroup::Base, setting->colorBackground);
//			dlg->get_mle()->setPalette(pal);
//			dlg->get_le()->setPalette(pal);
			
			//if (!name.isEmpty() && name != tr("Shouts*") && currentCommand->get_txt() !="stats")
			if (!name.isEmpty() && isplayer)
				slot_sendCommand("stats " + name, false);    // automatically request stats
      
		}

		Q_CHECK_PTR(dlg);
		dlg->write(txt);

		// play sound on new created dialog
//		bonus = true;
	}
/*
	// check if it was a channel message
	if (autoAnswer &= (isplayer && autoAwayMessage && !name.contains('*') && text[0] == '>'))
	{
		// send when qGo is NOT the active application - TO DO
		sendcommand("tell " + name + " [I'm away right now]");
	}
*/
	if (!dlg->pageActive)
	{
		ui.talkTabs->addTab(dlg->get_tabWidget(), dlg->get_name());
		dlg->pageActive = true;
		ui.talkTabs->setCurrentWidget(dlg->get_tabWidget());
	}
/*
	// play a sound - not for shouts
	if ((text[0] == '>' && bonus || !dlg->get_le()->hasFocus()) && !name.contains('*'))
	{
		qgoif->get_qgo()->playTalkSound();

		// set cursor to last line
		//dlg->get_mle()->setCursorPosition(dlg->get_mle()->lines(), 999); //eb16
		dlg->get_mle()->append("");                                        //eb16
		//dlg->get_mle()->removeParagraph(dlg->get_mle()->paragraphs()-2);   //eb16

		// write time stamp
		MultiLineEdit3->append(statusOnlineTime->text() + " " + name + (autoAnswer ? " (A)" : ""));
	}
	else if (name == tr("msg*"))
	{
		qgoif->get_qgo()->playTalkSound();

		// set cursor to last line
//		dlg->get_mle()->setCursorPosition(dlg->get_mle()->numLines(), 999); //eb16
		dlg->get_mle()->append(""); //eb16
//		dlg->get_mle()->removeParagraph(dlg->get_mle()->paragraphs()-2);   //eb16

		// write time stamp
		MultiLineEdit3->append(tr("Message") + ": " + text);
	}
*/
}

/*
 * close button pressed on talk tab
 */
void MainWindow::slot_pbRelOneTab(QWidget *w)
{
	// seek dialog
	Talk *dlg = talkList.at(0);
	int i= talkList.indexOf((Talk*)w);

	if ( i != -1)
	{
		if (w != ui.talkTabs->currentWidget())
			//we have a problem !
			return;
		
//		dlg = talkList.takeAt(i);
		ui.talkTabs->removeTab(ui.talkTabs->currentIndex()) ;
		dlg->pageActive = false;
	}

}



/*
 * 'stats' information has been received by the parser
 */
void MainWindow::slot_statsPlayer(Player *p)
{
	 Talk *dlg = 0;
  
	if (!p->name.isEmpty())
	{
		// seek dialog
		for (int i=0; i<talkList.count(); i++)
		{
			if (talkList.at(i)->get_name() == p->name)
				dlg = talkList.at(i);
		}
	  	//  found 
	  	if (dlg)
		{
			dlg->getUi().stats_rating->setText(p->rank);
			dlg->getUi().stats_info->setText(p->info);
			dlg->getUi().stats_default->setText(p->extInfo);
			dlg->getUi().stats_wins->setText(p->won + " /");
			dlg->getUi().stats_loss->setText(p->lost );
			dlg->getUi().stats_country->setText(p->country);
			dlg->getUi().stats_playing->setText(p->play_str);
//			dlg->getUi().stats_rated->setText(p->rated);
			dlg->getUi().stats_address->setText(p->address);
			
			// stored either idle time or last access	 
			dlg->getUi().stats_idle->setText(p->idle);
			if (!p->idle.isEmpty())
				dlg->getUi().Label_Idle->setText(p->idle.at(0).isDigit() ? "Idle :": "Last log :");
	
		
		}   
	}
}

/*
 * set checkbox status because of server info or because menu / checkbok toggled
 */
void MainWindow::slot_checkbox(int nr, bool val)
{
//	QCheckBox::ToggleState v = (val ? 

	// set checkbox (nr) to val
	switch (nr)
	{
		// open
		case 0:
			//toolOpen->setOn(val); 
			ui.setOpenMode->setChecked(val);
			break;

		// looking
		case 1:
			//toolLooking->setOn(val); 
			ui.setLookingMode->setChecked(val);
			break;

		// quiet
		case 2:
			//toolQuiet->setOn(val); 
			ui.setQuietMode->setChecked(val);
			break;

		default:
			qWarning("checkbox doesn't exist");
			break;
	}
}


/*
 * checkbox looking cklicked
 */
void MainWindow::slot_cblooking()
{
	bool val = ui.setLookingMode->isChecked(); 
	set_sessionparameter("looking", val);

	if (val)
		// if looking then set open
		set_sessionparameter("open", true);
}


/*
 * checkbox open clicked
 */
void MainWindow::slot_cbopen()
{
	bool val = ui.setOpenMode->isChecked(); 
	set_sessionparameter("open", val);

	if (!val)
		// if not open then set close
		set_sessionparameter("looking", false);
}

/*
 * checkbox quiet clicked
 */
void MainWindow::slot_cbquiet()
{
	bool val = ui.setQuietMode->isChecked(); 
	set_sessionparameter("quiet", val);

	if (val)
	{
		// if 'quiet' button is once set to true the list is not reliable any longer
//		gamesListSteadyUpdate = false;
//		playerListSteadyUpdate = false;
	}
}

/*
 * A player has connected
 */
void MainWindow::slot_playerConnected(Player *p)
{
	//don't do anything if we are in a room (otherwise, it cripples the room players list)
	if (ui.RoomList->currentIndex())
		return;

	switch(myAccount->get_gsname())
	{
		case IGS:
		{
			// we send the informaton request only if the players rank is within our rank selection
			if ((rkToKey(p->rank) <= rkToKey(rkMin)) && (rkToKey(p->rank) >= rkToKey(rkMax)))
				sendcommand("userlist " + p->name, false);
			break;
		}	
		default:
		{
			slot_player(p,false);
		}			
	}
}


/*
 * A match has been requested
 * 
 */
void MainWindow::slot_matchRequest(const QString &line, bool myrequest)
{
	// set up match dialog
	GameDialog *dlg = NULL;
	QString opponent;

	qDebug("Match has been Requested");
	// seek dialog
	if (!myrequest)
	{
		// match xxxx B 19 1 10
//		opponent = element(line, 1, " ");
		opponent = line.section(" ",1,1);

		// play sound
//		qgoif->get_qgo()->playMatchSound();
	}
	else
	{
		// xxxxx 4k*
//		opponent = element(line, 0, " ");
		opponent = line.section(" ",0,0);
	}

	// look for same opponent
//	dlg = matchlist.first();
//	while (dlg && dlg->getUi().playerOpponentEdit->text() != opponent)// && dlg->getUi().playerBlackEdit->text() != opponent)
//		dlg = matchlist.next();

	for (int i=0; i < matchList.count(); i++)
	{
		if (matchList.at(i)->getUi().playerOpponentEdit->text() == opponent)
			dlg = matchList.at(i);
	}


	if (!dlg)
	{
		dlg = new GameDialog();

		matchList.insert(0, dlg);//new GameDialog(/* tr("New Game")*/);
//		dlg = matchlist.current();
		
/*		if (myAccount->get_gsname() == NNGS ||
			myAccount->get_gsname() == LGS)
		{
			// now connect suggest signal
			connect(parser,
				SIGNAL(signal_suggest(const QString&, const QString&, const QString&, const QString&, int)),
				dlg,
				SLOT(slot_suggest(const QString&, const QString&, const QString&, const QString&, int)));
		}
*/		
		connect(dlg,
			SIGNAL(signal_removeDialog(GameDialog *)), 
			this, 
			SLOT(slot_removeDialog(GameDialog *)));

		connect(dlg,
			SIGNAL(signal_sendCommand(const QString&, bool)),
			this,
			SLOT(slot_sendCommand(const QString&, bool)));

		connect(parser,
			SIGNAL(signal_matchCreate(const QString &, const QString &)),
			this,
			SLOT(slot_removeDialog(const QString &, const QString &)));

// CAUTION : this is used in qGo1 for sending parameters (handicap, komi) to the server. We won't use this for now
//		connect(parser,
//			SIGNAL(signal_matchCreate(const QString&, const QString&)),
//			dlg,
//			SLOT(slot_matchCreate(const QString&, const QString&)));
		connect(parser,
			SIGNAL(signal_notOpen(const QString&, int)),
			dlg,
			SLOT(slot_notOpen(const QString&, int)));
		connect(parser,
			SIGNAL(signal_komiRequest(const QString&, int, int, bool)),
			dlg,
			SLOT(slot_komiRequest(const QString&, int, int, bool)));
		connect(parser,
			SIGNAL(signal_opponentOpen(const QString&)),
			dlg,
			SLOT(slot_opponentOpen(const QString&)));
		connect(parser,
			SIGNAL(signal_dispute(const QString&, const QString&)),
			dlg,
			SLOT(slot_dispute(const QString&, const QString&)));

//		connect(dlg,
//			SIGNAL(signal_matchSettings(const QString&, const QString&, const QString&, assessType)),
//			qgoif,
//			SLOT(slot_matchSettings(const QString&, const QString&, const QString&, assessType)));
	}

	if (myrequest)
	{
//		QString rk = element(line, 1, " ");
		QString rk = line.section(" ",1,1);
		// set values

		dlg->getUi().playerOpponentEdit->setText(opponent);		
//		dlg->getUi().playerOpponentEdit->setReadOnly(true);
		dlg->set_myName( myAccount->acc_name);

		// set my and opponent's rank for suggestion
		dlg->set_oppRk(rk);
		dlg->getUi().playerOpponentRkEdit->setText(rk);
		rk = myAccount->get_rank();
		dlg->set_myRk(rk);

		dlg->set_gsName(myAccount->get_gsname());
		dlg->getUi().handicapSpin->setEnabled(false);

		dlg->getUi().buttonDecline->setDisabled(true);
		

		// teaching game:
		if (dlg->getUi().playerOpponentEdit->text() == myAccount->acc_name)
			dlg->getUi().buttonOffer->setText(tr("Teaching"));

		//nmatch settings from opponent 
		bool is_nmatch = false;

		// we want to make sure the player is selected, because the match request may come from an other command (match button on the tab dialog)
		QString lv_popup_name ;
		PlayerTableItem* lv_popupPlayer = (PlayerTableItem*)ui.ListView_players->currentItem();

		if (lv_popupPlayer )
		{
			lv_popup_name = (lv_popupPlayer->text(1).right(1) == "*" ? lv_popupPlayer->text(1).left( lv_popupPlayer->text(1).length() -1 ):lv_popupPlayer->text(1));
		

			is_nmatch = ((lv_popupPlayer->nmatch ) &&  (lv_popup_name == opponent));// && setting->readBoolEntry("USE_NMATCH");
		}

		dlg->set_is_nmatch(is_nmatch);

		if ( (is_nmatch) && (lv_popupPlayer->nmatch_settings ))
		{
			//FIXME make sure we can set this with canadian when the opponent is set to japanese
			dlg->getUi().timeSpin->setRange((int)(lv_popupPlayer->nmatch_timeMin/60), (int)(lv_popupPlayer->nmatch_timeMax/60));
			dlg->getUi().byoTimeSpin->setRange((int)(lv_popupPlayer->nmatch_BYMin/60), (int)(lv_popupPlayer->nmatch_BYMax/60));
			
			dlg->getUi().handicapSpin->setRange( (lv_popupPlayer->nmatch_handicapMin == 0 ? 1 : lv_popupPlayer->nmatch_handicapMin), lv_popupPlayer->nmatch_handicapMax);
		}
		else
		{
			dlg->getUi().timeSpin->setRange(0,60);
			dlg->getUi().byoTimeSpin->setRange(0,60);
//			dlg->getUi().handicapSpin->setRange(0,9);
		}
		//no handicap , nigiri,  Jap. Byo yomi - with usual game requests
		dlg->getUi().handicapSpin->setEnabled(is_nmatch);
		dlg->getUi().play_nigiri_button->setEnabled(is_nmatch);
		if (!is_nmatch)
			dlg->getUi().timeTab->removeTab(1);		

		//default settings

		QSettings settings;

		dlg->getUi().boardSizeSpin->setValue(settings.value("DEFAULT_SIZE").toInt());
		dlg->getUi().timeSpin->setValue(settings.value("DEFAULT_TIME").toInt());
		dlg->getUi().byoTimeSpin->setValue(settings.value("DEFAULT_BY").toInt());
		dlg->getUi().komiSpin->setValue(settings.value("DEFAULT_KOMI").toInt() );  // *10+5);

//		dlg->getUi().slot_pbsuggest();
	}
	else
	{
		// match xxxx B 19 1 10 - using this line means: I am black!
		bool opp_plays_white = (line.section(" ",2,2) == "B");//QString(tr("B")));
		bool opp_plays_nigiri = (line.section(" ",2,2) == "N");

		QString handicap, size, time,byotime, byostones ;

		if (line.contains("nmatch"))
		{
			//specific behavior here : IGS nmatch not totally supported
			// disputes are hardly supported
			dlg->set_is_nmatch(true);
			handicap = line.section(" ",3,3);
			size = line.section(" ",4,4);
			time = line.section(" ",5,5);
			byotime = line.section(" ",6,6);
			byostones = line.section(" ",7,7);
			dlg->getUi().timeSpin->setRange(0,100);
			dlg->getUi().timeSpin->setValue(time.toInt()/60);
			dlg->getUi().byoTimeSpin->setRange(0,100);
			dlg->getUi().byoTimeSpin->setValue(byotime.toInt()/60);
			dlg->getUi().BY_label->setText(tr(" Byo Time : (") + byostones+ tr(" stones)"));
			dlg->getUi().handicapSpin->setRange(1,9);
			dlg->getUi().handicapSpin->setValue(handicap.toInt());
			dlg->getUi().boardSizeSpin->setRange(1,19);
			dlg->getUi().boardSizeSpin->setValue(size.toInt());
		}
		else
		{
			dlg->set_is_nmatch(false);
			size = line.section(" ",3,3);//element(line, 3, " ");
			time = line.section(" ",4,4);//element(line, 4, " ");
			byotime = line.section(" ",5,5);//element(line, 5, " ");
			dlg->getUi().timeSpin->setRange(0,1000);
			dlg->getUi().timeSpin->setValue(time.toInt());
			dlg->getUi().byoTimeSpin->setRange(0,100);
			dlg->getUi().byoTimeSpin->setValue(byotime.toInt());
			dlg->getUi().handicapSpin->setEnabled(false);
			dlg->getUi().play_nigiri_button->setEnabled(false);
			dlg->getUi().boardSizeSpin->setRange(1,19);
			dlg->getUi().boardSizeSpin->setValue(size.toInt());

			dlg->getUi().timeTab->removeTab(1);
		}
		

//		QString rk = getPlayerRk(opponent);
		// look for players in playerlist
		QTreeWidgetItemIterator lv(ui.ListView_players);
		QTreeWidgetItem *lvi;
		for (; (*lv); lv++)
		{
			lvi = *lv;
			// compare names
			if (lvi->text(1) == opponent)
			dlg->set_oppRk(lvi->text(2));
			break;
		}


//		dlg->set_oppRk(rk);
		QString myrk = myAccount->get_rank();
		dlg->set_myRk(myrk);

		dlg->getUi().playerOpponentEdit->setText(opponent);		
		dlg->getUi().playerOpponentEdit->setReadOnly(true);		
//		dlg->getUi().playerOpponentRkEdit->setText(rk);
		dlg->set_myName( myAccount->acc_name);

		if (opp_plays_white)
		{
/*			dlg->getUi().playerBlackEdit->setText(myAccount->acc_name);
			dlg->getUi().playerBlackEdit->setReadOnly(true);
			dlg->getUi().playerBlackRkEdit->setText(myAccount->get_rank());
			dlg->getUi().playerWhiteEdit->setText(opponent);
			dlg->getUi().playerWhiteEdit->setReadOnly(false);
			dlg->getUi().playerWhiteRkEdit->setText(rk);
*/
			dlg->getUi().play_black_button->setChecked(true);


		}
		else if (opp_plays_nigiri)
		{
/*			dlg->getUi().playerWhiteEdit->setText(myAccount->acc_name);
			dlg->getUi().playerWhiteEdit->setReadOnly(true);
			dlg->getUi().playerWhiteRkEdit->setText(myAccount->get_rank());
			dlg->getUi().playerBlackEdit->setText(opponent);
			dlg->getUi().playerBlackEdit->setReadOnly(false);
			dlg->getUi().playerBlackRkEdit->setText(rk);
*/
			dlg->getUi().play_nigiri_button->setChecked(true);
		}
		else
			dlg->getUi().play_white_button->setChecked(true);		

		dlg->getUi().buttonDecline->setEnabled(true);
//		dlg->getUi().buttonOffer->setText(tr("Accept"));
		dlg->getUi().buttonCancel->setDisabled(true);

	}

	dlg->slot_changed();
	dlg->show();
//	dlg->setWindowState(Qt::WindowActive);
	dlg->raise();

	if (!myrequest)
		gameSound->play();
}

/*
 * talk dialog -> return pressed
 */
void MainWindow::slot_talkTo(QString &receiver, QString &txt)
{
	// echo
	if (txt.length())
	{
		switch (myAccount->get_gsname())
		{
			case IGS:
			{
				bool ok;
				// test if it's a number -> channel
				/*int nr =*/ receiver.toInt(&ok);
				if (ok)
					// yes, channel talk
					sendcommand("yell " + txt, false);
//				else if (receiver.contains('*'))
//					sendcommand("shout " + txt, false);
				else
					sendcommand("tell " + receiver + " " + txt, false);
			}
				break;

			default:
				// send tell command w/o echo
				if (receiver.contains('*'))
					sendcommand("shout " + txt, false);
				else
					sendcommand("tell " + receiver + " " + txt, false);
				break;
		}

		// lokal echo in talk window
		slot_talk(receiver, "-> " + txt, true);
	}
}


/*
 * A game dialog has sent a 'remove' signal
 */
void MainWindow::slot_removeDialog(GameDialog *dlg)
{

	int i = matchList.indexOf(dlg);

	if (i == -1)
	{
		qDebug("match dialog not found in List !!!");
		delete dlg;
		return;
	}
	
	delete matchList.takeAt(i); 

}

/*
 * The parser has sent 'match create' signal because a game has started
 */
void MainWindow::slot_removeDialog(const QString & nr, const QString & opp)
{
	GameDialog * dlg;

	int i;
	for ( i=0; i < matchList.count(); i++)
	{
		dlg = matchList.at(i);
		if (dlg->getUi().playerOpponentEdit->text() == opp)
		{
			Game * g = new Game();
			g->nr = nr;
			if(dlg->getUi().play_white_button->isChecked())
			{
				g->wname = myAccount->acc_name;
				g->wrank = myAccount->get_rank(); 
				g->bname = dlg->getUi().playerOpponentEdit->text();
				g->brank = dlg->getUi().playerOpponentRkEdit->text();
			}
			else
			{
				g->bname = myAccount->acc_name;
				g->brank = myAccount->get_rank(); 
				g->wname = dlg->getUi().playerOpponentEdit->text();
				g->wrank = dlg->getUi().playerOpponentRkEdit->text();
			}
			g->H = dlg->getUi().handicapSpin->text();
			g->Sz = dlg->getUi().boardSizeSpin->text();
			g->K = dlg->getUi().komiSpin->text();
			g->By = dlg->getUi().byoTimeSpin->text();
			g->mv = "0";
			qgoif->createMatch(g);
			delete matchList.takeAt(i); 
			return ;
		}
	}
}




/*
 * The parser has sent a signal that opponent canceled the match request
 */
void MainWindow::slot_matchCanceled(const QString& opp)
{
	GameDialog *dlg=NULL;
	
	for (int i=0; i < matchList.count(); i++)
	{
		if (matchList.at(i)->getUi().playerOpponentEdit->text() == opp)
			dlg = matchList.at(i);
	}


	if (dlg)
		dlg->slot_notOpen(opp, 2);

}

/*
 * 
 */
void MainWindow::timerEvent(QTimerEvent* e)
{
	// some variables for forcing send buffer and keep line established
	static int counter = 899;
	static int holdTheLine = true;
	static int tnwait = 0;
//	static int statusCnt = 0;
	static QString statusTxt = QString();
	static int imagecounter = 0;
	
	//qDebug( "timer event, id %d", e->timerId() );

	if (e->timerId() == seekButtonTimer)
	{	
		imagecounter = (imagecounter+1) % 4;
		QString ic = ":/ressources/pics/seeking" + QString::number(imagecounter + 1) +".png";
		ui.toolSeek->setIcon(QIcon(ic));
		return;
	}

	if (tn_ready)
	{
		// case: ready to send
		tnwait = 0;
	}
	else if (tnwait < 2)
	{
		//qDebug(QString("%1: HoldTheLine SET: something has been sent").arg(statusOnlineTime->text()));
		// case: not ready to send, but maybe waiting for READY message
		tnwait++;
		// something was sent, so reset counter
		counter = 899;//resetCounter();
		holdTheLine = true;
		autoAwayMessage = false;
	}

	if (counter % 300 == 0)
	{
		// 5 mins away -> set auto answering
		autoAwayMessage = true;
		if (counter == 0)
		{
			// send "ayt" every half hour anyway
			// new: reset timer after one hour of idle state
			//      -> if not observing a game!
			//qDebug(QString("%1 -> HoldTheLine: status = %2").arg(statusOnlineTime->text()).arg(holdTheLine));
			if (holdTheLine)
				sendcommand("ayt", false);

			// 12*5 min
			counter = 899;//	resetCounter();

			if (myAccount->num_observedgames == 0)
			{
				// nothing observing
				holdTheLine = false;
//qDebug(QString("%1: HoldTheLine END!").arg(statusOnlineTime->text()));
			}
			else
			{
//qDebug(QString("%1: HoldTheLine LENGTHENED: observing game...").arg(statusOnlineTime->text()));
			}
		}
		else if (myAccount->get_gsname() == IGS && holdTheLine)
		{
			sendcommand("gamelist", false);
			qDebug(QString("%1 -> gamelist").arg(statusOnlineTime->text()).toLatin1().constData());
		}
	}

	counter--;

	// display online time
	onlineCount++;
	int hr = onlineCount/3600;
	int min = (onlineCount % 3600)/60;
	int sec = onlineCount % 60;
	QString pre = " ";
	QString min_;
	QString sec_;

	if (min < 10)
		min_ = "0" + QString::number(min);
	else
		min_ = QString::number(min);

	if (sec < 10)
		sec_ = "0" + QString::number(sec);
	else
		sec_ = QString::number(sec);

	if (hr)
		pre += QString::number(hr) + "h ";

	statusOnlineTime->setText(pre + min_ + ":" + sec_ + " ");

	// some statistics
//	QToolTip::remove(statusServer);
//	QToolTip::add(statusServer, tr("Current server") + "\n" +
//		tr("Bytes in:") + " " + QString::number(bytesIn) + "\n" +
//		tr("Bytes out:") + " " + QString::number(bytesOut));
//	LineEdit_bytesIn->setText(QString::number(bytesIn));
//	LineEdit_bytesOut->setText(QString::number(bytesOut));

// DEBUG ONLY BEGIN ****
	// DEBUG display remaining time
	hr = counter/3600;
	min = (counter % 3600)/60;
	sec = counter % 60;
	if (autoAwayMessage)
		pre = "(A) ";
	else
		pre = " ";

	if (min < 10)
		min_ = "0" + QString::number(min);
	else
		min_ = QString::number(min);

	if (sec < 10)
		sec_ = "0" + QString::number(sec);
	else
		sec_ = QString::number(sec);

	if (hr)
		pre += QString::number(hr) + "h ";

	statusMessage->setText(pre + min_ + ":" + sec_ + (holdTheLine ? " Hold" : " "));
// DEBUG ONLY END ****
}


/*
 * The parser has sent a message to be put in the messages box
 */
void MainWindow::slot_msgBox(const QString& msg)
{
	ui.talkTabs->setCurrentIndex(1);
	ui.msgTextEdit->append(msg);
}


/*
* on IGS, sends the 'nmatch'time, BY, handicap ranges
* command syntax : "nmatchrange 	BWN 	0-9 19-19	 60-60 		60-3600 	25-25 		0 0 0-0"
*				(B/W/ nigiri)	Hcp Sz	   Main time (secs)	BY time (secs)	BY stones	Koryo time
*/
void MainWindow::sendNmatchParameters()
{
	
	if ((myAccount->get_gsname() != IGS) || (myAccount->get_status() == OFFLINE))
		return ;

	QString c = "nmatchrange ";
	QSettings settings;

	c.append(settings.value("NMATCH_BLACK").toBool() ? "B" : "");
	c.append(settings.value("NMATCH_WHITE").toBool() ? "W" : "");
	c.append(settings.value("NMATCH_NIGIRI").toBool() ? "N" : "");
	c.append(" 0-");
	c.append(settings.value("NMATCH_HANDICAP").toString());
	c.append(" ");
	c.append(settings.value("DEFAULT_SIZE").toString());
	c.append("-19 ");
	c.append(QString::number(settings.value("DEFAULT_TIME").toInt()*60));
	c.append("-");
	c.append(QString::number(settings.value("NMATCH_MAIN_TIME").toInt()*60));
	c.append(" ");
	c.append(QString::number(settings.value("DEFAULT_BY").toInt()*60));
	c.append("-");
	c.append(QString::number(settings.value("NMATCH_BYO_TIME").toInt()*60));
	c.append(" 25-25 0 0 0-0");
	
	sendcommand(c, true);
}
