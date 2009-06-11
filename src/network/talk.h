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
#ifndef TALK_H
#define TALK_H

#include "ui_talk_gui.h"
#include <QtGui>

class NetworkConnection;
class PlayerListing;

class Talk : public QDialog, public Ui::TalkGui
{
Q_OBJECT

public:
	Talk(NetworkConnection * conn, PlayerListing & player);
	virtual ~Talk();
	QTextEdit      *get_mle() const { return ui.MultiLineEdit1; } 
	QLineEdit      *get_le() const {return ui.LineEdit1; }
	QWidget        *get_tabWidget()  { return this; }
	QString        get_name() const;
	PlayerListing & get_opponent() const;
	bool	 	getConversationOpened(void) const { return conversationOpened; };
	void		setConversationOpened(bool c) { conversationOpened = c; };
	//void           set_name(QString &n) { name = n; }
	void           write(const QString &text = QString()) const;
	void recvTalk(QString text);
	void	       updatePlayerListing(void);
	bool           pageActive;
	void           setTalkWindowColor(QPalette pal);
	Ui::TalkGui	getUi() 		{return ui;}
public slots:
	void slot_returnPressed();
	void slot_pbRelTab();
	void slot_match();

signals:
	void signal_talkTo(QString&, QString&);
	void signal_pbRelOneTab(QWidget*);
	void signal_matchRequest(const QString&);


private:
	void closeEvent(QCloseEvent *e);
	Ui::TalkGui ui;
	static int     counter;
	NetworkConnection * connection;
	PlayerListing & opponent;
	bool conversationOpened;

};

#endif
