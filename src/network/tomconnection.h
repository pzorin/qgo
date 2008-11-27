#include "tygemconnection.h"

class TomConnection : public TygemConnection
{
	public:
		TomConnection(NetworkDispatch * _dispatch, const class ConnectionInfo & info);
		//~TomConnection();
	private:
		virtual int requestServerInfo(void);
		virtual void handleServerInfo(unsigned char *, unsigned int);
};
