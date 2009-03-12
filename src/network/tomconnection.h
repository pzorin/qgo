#include "tygemconnection.h"

class TomConnection : public TygemConnection
{
	public:
		TomConnection(const QString & user, const QString & pass);
		//~TomConnection();
	private:
		virtual int requestServerInfo(void);
		virtual void handleServerInfo(unsigned char *, unsigned int);
		virtual QString getTygemGameRecordQString(class GameData *);
};
