/*
* globals.h
*/

/* FIXME To be removed */
#ifndef GLOBALS_H
#define GLOBALS_H

#include "defines.h"


#include <QtCore>

class sendBuf
{
public:
	sendBuf(QString text, bool echo=true) { txt = text; localecho = echo; }
	~sendBuf() {}
	QString get_txt() { return txt; }
	bool get_localecho() { return localecho; }
	QString txt;
    
private:
	bool localecho;
};

/*
* Global variables declarations
*/



//	static QSettings  settings;//("qgo2");

#endif
