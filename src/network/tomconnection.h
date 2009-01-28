#include "tygemconnection.h"

class TomConnection : public TygemConnection
{
	public:
		TomConnection(NetworkDispatch * _dispatch, const QString & user, const QString & pass);
		//~TomConnection();
	private:
		virtual int requestServerInfo(void);
		virtual void handleServerInfo(unsigned char *, unsigned int);
};
