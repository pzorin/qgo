#include "tygemconnection.h"

class EWeiQiConnection : public TygemConnection
{
	public:
		EWeiQiConnection(const QString & user, const QString & pass);
		//~EWeiQiConnection();
	private:
		virtual int requestServerInfo(void);
		virtual QString getTygemGameRecordQString(class GameData *);
};
