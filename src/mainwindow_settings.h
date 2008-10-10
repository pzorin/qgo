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

#ifndef MAINWINDOW_SETTINGS_H
#define MAINWINDOW_SETTINGS_H

#include "defines.h"

#include <QtGui>

class Host
{
public:
	Host(const QString&, const ConnectionType, const QString&, const unsigned int, const QString&, const QString&, const QString&);
	~Host() {};
	QString title() const { return t; };
	ConnectionType host() const { return h; };
	unsigned int port() const { return pt; };
	QString loginName() const { return lg; };
	QString password() const { return pw; };
	QString codec() const { return cdc; };
	QString address() const { return ad; };
	// operators <, ==
	int operator== (Host h)
		{ return (this->title() == h.title()); };
	int operator== (Host *h)
		{ return (this->title() == h->title()); };
	bool operator< (Host h)
		{ return (this->title() < h.title()); };
	bool operator< (Host *h)
		{ return (this->title() < h->title()); };

private:
	QString t;
	ConnectionType h;
	QString ad;
	QString lg;
	QString pw;
	unsigned int pt;
	QString cdc;
};

class HostList : public QList<Host*>
{
public:
	HostList() {};
	virtual ~HostList() {};
	
	virtual int compareItems(Host * d1, Host * d2);
};

/* FIXME using settings.value for the few settings that are used
 * is awkward because it means searching for a string, not looking up
 * a variable.  that needs to be fixed so it can be used in important places */

#ifdef FIXME
/* We can eventually remove this.  Its all stored on the network connection now */
class Account
{
public:
	Account(QWidget*);
	~Account();
	void set_caption();
	void set_connectionType(ConnectionType);
	void set_offline();
	void set_accname(QString&);
	void set_status(Status);
	void set_rank(QString &rk) { rank = rk; }
	QString get_rank() { return rank; }
	Status  get_status();
	ConnectionType  get_connectionType();

	ConnectionType connectionType;
	QString svname;
	QString acc_name;
	Status  status;
	int     num_players;
	int     num_games;
	int     num_watchedplayers;
	int     num_observedgames;
	Game * getGame(int game_number);
	void addGame(int game_number, Game * game);
	void removeGame(int game_number);

private:
	QString rank;
	QString line;
	QString standard;
	QWidget *parent;
	std::map <int,Game *> games;
};
#endif //FIXME

class PlayerTableItem : public QTreeWidgetItem
{ 
public:
 
	PlayerTableItem(QTreeWidget *parent, int Type = 0) : QTreeWidgetItem( parent,  Type) {};
	PlayerTableItem(QTreeWidget *parent, QStringList s) : QTreeWidgetItem( parent,  s) {};
	PlayerTableItem(QTreeWidgetItem *parent,  int Type = 0): QTreeWidgetItem( parent,  Type) {};
/*	PlayerTableItem(PlayerTable *parent, QString label1, QString label2 = QString::null,
		QString label3 = QString::null, QString label4 = QString::null,
		QString label5 = QString::null, QString label6 = QString::null,
		QString label7 = QString::null, QString label8 = QString::null,
		QString label9 = QString::null, QString label10 = QString::null,
		QString label11 = QString::null, QString label12 = QString::null,
		QString label13 = QString::null);
*/
	~PlayerTableItem() {};

//	void ownRepaint();
//	void replace() ;
//	void set_nmatchSettings(Player *p);

	bool nmatch;

	// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
	bool nmatch_black, nmatch_white, nmatch_nigiri, nmatch_settings;
	int 	nmatch_handicapMin, nmatch_handicapMax, 
		nmatch_timeMin, nmatch_timeMax, 
		nmatch_BYMin, nmatch_BYMax, 
		nmatch_stonesMin, nmatch_stonesMax,
		nmatch_KoryoMin, nmatch_KoryoMax;

	//bool isOpen() {return open;}

protected:
//	virtual QString key(int, bool) const;
//	virtual int compare( QListViewItem *p, int col, bool ascending ) const;
//	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

	bool open;
	bool watched;
	bool exclude;
	bool its_me;
	bool seeking;

};


#endif
