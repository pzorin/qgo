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

					//temporaary settings to prevent use of Koryo BY on IGS (as opposed to canadian)
					sendcommand("nmatchrange BWN 0-9 19-19 60-60 60-3600 25-25 0 0 0-0",false);
//					send_nmatch_range_parameters();
//					}
					sendcommand("toggle newundo",true);
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
//TODO			qgoif->set_gsName(myAccount->get_gsname());
			// show current Server name in status bar
//			statusServer->setText(" " + myAccount->svname + " ");

			// start timer: event every second
			onlineCount = 0;
			startTimer(1000);
			// init shouts
//TODO			slot_talk("Shouts*", 0, true);
			
//TODO				qgoif->get_qgo()->playConnectSound();
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
//			qgoif->set_myName(myAccount->acc_name);
//TODO			parser->set_myname(myAccount->acc_name);
			break;


		case STATS:
			// we just received a players name as first line of stats -> create the dialog tab
			currentCommand->txt="stats";
		
		// if (!talklist.current())
//TODO			slot_talk( parser->get_statsPlayer()->name,0,true);
		
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


// refresh button clicked
void MainWindow::slot_refresh(int i)
{

	QString wparam = "" ;
	// refresh depends on selected page
	switch (i)
	{
		case 10:
			prepareTables(WHO);
		case 0:
/* TODO			// send "WHO" command
      			//set the params of "who command"
			if ((whoBox1->currentItem() >1)  || (whoBox2->currentItem() >1))
        		{
				wparam.append(whoBox1->currentItem()==1 ? "9p" : whoBox1->currentText());
				if ((whoBox1->currentItem())  && (whoBox2->currentItem()))
					wparam.append("-");

				wparam.append(whoBox2->currentItem()==1 ? "9p" : whoBox2->currentText());
         		} 
			else if ((whoBox1->currentItem())  || (whoBox2->currentItem()))
        			wparam.append("1p-9p");
			else
				wparam.append(((myAccount->get_gsname() == IGS) ? "9p-BC" : " "));
				

			if (whoOpenCheck->isChecked())
				wparam.append(((myAccount->get_gsname() == WING) ? "O" : "o"));//wparam.append(" o");
*/
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
//			statusUsers->setText(" P: 0 / 0 ");
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
//			statusGames->setText(" G: 0 / 0 ");
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
 * seek string received after 'seek config_list' command
 */
void MainWindow::slot_addSeekCondition(const QString& /*a*/, const QString& b, const QString& c, const QString& d, const QString& )
{
	QString time_condition ;
	
	time_condition = QString::number(int(b.toInt() / 60)) + " min + " + QString::number(int(c.toInt() / 60)) + " min / " + d + " stones";

	seekMenu->addAction(time_condition);//, this, SLOT(slot_seek(int))), 0, a.toInt());
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

		QString excludeMark = "";
		QString myMark = "B";
/*		
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
			lvi_mem->setText(12, myMark + rkToKey(g->wrank) + g->wname.toLower() + ":" + excludeMark);

//			lvi_mem->ownRepaint();
		}
		else
		{
			// from GAMES command or game info{...}
			QStringList sl;	

			if (!g->H.isEmpty())
			{
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

			}
			else
			{
//				lv = new QTreeWidgetItem(ListView_games,
				sl 	<< g->nr
					<< " " + g->wname
					<< g->wrank
					<< " " + g->bname
					<< g->brank
					<< g->mv
					<< g->Sz;



			}
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
//			statusGames->setText(" G: " + QString::number(myAccount->num_games) + " / " + QString::number(myAccount->num_observedgames) + " ");
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
	QTreeWidgetItem *topViewItem = ui.ListView_players->itemAt(pp);
  	bool deleted_topViewItem = false;
  	QTreeWidgetItem *lvi;

	if (p->online)
	{
		// check if it's an empty list, i.e. all items deleted before
		if (cmdplayers && !playerListEmpty)
		{
			for (; (*lv); lv++)
			{
//				lv++;
				lvi = *lv;
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
//						lvi->set_nmatchSettings(p);					
						//lvi->nmatch = p->nmatch;

//						lvi->ownRepaint();
					}

/*					if (p->name == myAccount->acc_name)
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
					}
*/
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
			}
		}
*/
		QStringList sl;
		// from WHO command or {... has connected}
		if (/*extUserInfo && */myAccount->get_gsname() == IGS)
		{
			
			
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
		}
		else
		{
//			PlayerTableItem *lv1 = new PlayerTableItem(ListView_players,
			sl	<<	p->info
				<<	p->name
				<<	p->rank
				<<	p->play_str
				<<	p->obs_str
				<<	p->idle
				<<	mark;
//			lv1->setText(12, rkToKey(p->rank) + p->name.lower());
//			lv1->set_nmatchSettings(p);
		}
		
		lvi = new QTreeWidgetItem(ui.ListView_players, sl);

		lvi->setTextAlignment(0,Qt::AlignRight);
		lvi->setTextAlignment(3,Qt::AlignRight);
		lvi->setTextAlignment(4,Qt::AlignRight);
		lvi->setTextAlignment(5,Qt::AlignRight);
		lvi->setTextAlignment(6,Qt::AlignRight);
		lvi->setTextAlignment(7,Qt::AlignRight);
		lvi->setTextAlignment(8,Qt::AlignRight);
		lvi->setTextAlignment(9,Qt::AlignRight);


		// increase number of players
		myAccount->num_players++;
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
			lvi = *lv;
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
		if (rk == "NR")
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
