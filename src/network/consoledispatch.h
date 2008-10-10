#ifndef CONSOLEDISPATCH_H
#define CONSOLEDISPATCH_H
#include "networkdispatch.h"

class MainWindow;

class ConsoleDispatch : public NetworkDispatch
{
	public:
		ConsoleDispatch(MainWindow * m);
		~ConsoleDispatch();
		void recvText(const char * text);
		void recvText(QString text);
		void sendText(const char * text);
	private:
		MainWindow * mainwindow;
};
#endif //CONSOLEDISPATCH_H
