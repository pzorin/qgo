/*
 *   gamedialog.h
 */

#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

#include "ui_gamedialog.h"
//#include "gs_globals.h"
#include "defines.h"
//#include "misc.h"
#include <QtGui>

class GameDialog : public QDialog,/*public NewGameDialog,*/public Ui::GameDialog //, public Misc<QString>
{ 
	Q_OBJECT

public:
	GameDialog( );//QWidget* parent = 0, const char* name = 0, bool modal = true, WFlags fl = 0);
	~GameDialog();
	void set_gsName(GSName g) 	{ gsname = g; }
	void set_oppRk(QString rk) 	{ oppRk = rk; qDebug("oppRk: %s",  rk.toLatin1().constData()); }
	void set_myRk(QString &rk) 	{ myRk = rk; qDebug("myRk: %s",  rk.toLatin1().constData()); }
	void set_myName(QString &name) 	{ myName = name; }
	void set_is_nmatch (bool b) 	{ is_nmatch = b; }
	Ui::GameDialog	getUi() 	{return ui;}

protected:
	void closeEvent(QCloseEvent *e);

signals:
	void signal_sendCommand(const QString &cmd, bool localecho);
	void signal_matchSettings(const QString&, const QString&, const QString&, assessType);
	void signal_removeDialog(GameDialog *);//const QString&);

public slots:
	// pushbuttons
	void slot_statsOpponent();
//	void slot_swapcolors();
//	void slot_pbsuggest();
	void slot_offer(bool);
	void slot_decline();
	void slot_changed();
	void slot_cancel();
	// parser
//	void slot_suggest(const QString&, const QString&, const QString&, const QString&, int);
	void slot_matchCreate(const QString&, const QString&);
	void slot_notOpen(const QString&, int motive = 0);
	void slot_komiRequest(const QString&, int, int, bool);
//	void slot_opponentopen(const QString&);
	void slot_dispute(const QString&, const QString&);

private:
	Ui::GameDialog ui;
	bool have_suggestdata;
	QString pwhite;
	QString pblack;
	QString h19, h13, h9;
	QString k19, k13, k9;
	GSName  gsname;
	QString oppRk;
	QString myRk;
	QString myName;
	bool komi_request;
	bool is_nmatch;
};

#endif
