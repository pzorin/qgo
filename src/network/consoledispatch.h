#ifndef CONSOLEDISPATCH_H
#define CONSOLEDISPATCH_H

class QString;
class NetworkConnection;

class ConsoleDispatch
{
	public:
		ConsoleDispatch(NetworkConnection * conn);
		~ConsoleDispatch();
		void recvText(const char * text);
		void recvText(QString text);
		void sendText(const char * text);
	private:
		NetworkConnection * connection;
};
#endif //CONSOLEDISPATCH_H
