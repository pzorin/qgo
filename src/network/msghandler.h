#ifndef MSGHANDLER_H
#define MSGHANDLER_H
#include <QString>
#include "registry.h"

class NetworkConnection;

class MsgHandler
{
	public:
		MsgHandler(NetworkConnection * _c) : connection(_c) {};
		virtual ~MsgHandler() {};
		virtual void handleMsg(QString) {};
		/* I would add a handleMsg(unsigned char * , size) variation
		 * here except the dispatch would have to differ anyway, so
		 * I'm not so sure of the point.  Really this is like a
		 * new parser */
	protected:
		QString element(const QString &line, int index, const QString &del1, const QString &del2="", bool killblanks = FALSE);
		NetworkConnection * connection;	
		/* I don't like having these two since they're only used by maybe a
		 * third or less of messages... but we'll give it a whirl 
		 * I'm starting to think that with these two and rankToScore, we need
		 * a utility class, maybe the mainapp class could have these for
		 * whatever needs them.*/
		unsigned int idleTimeToSeconds(QString time);
		void fixRankString(QString * rank);
};

class MsgHandlerRegistry : public Registry <unsigned int, MsgHandler> {};
#endif //MSGHANDLER_H
