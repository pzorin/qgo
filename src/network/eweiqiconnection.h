#include "tygemconnection.h"

class EWeiQiConnection : public TygemConnection
{
	public:
		EWeiQiConnection(NetworkDispatch * _dispatch, const class ConnectionInfo & info);
		//~EWeiQiConnection();
	private:
		virtual int requestServerInfo(void);
};
