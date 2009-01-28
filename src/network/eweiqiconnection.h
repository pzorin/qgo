#include "tygemconnection.h"

class EWeiQiConnection : public TygemConnection
{
	public:
		EWeiQiConnection(NetworkDispatch * _dispatch, const QString & user, const QString & pass);
		//~EWeiQiConnection();
	private:
		virtual int requestServerInfo(void);
};
