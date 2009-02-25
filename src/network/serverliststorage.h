#include <vector>
#include "../defines.h"

class ServerItem;

class ServerListStorage
{
	public:
		~ServerListStorage();
		bool restoreServerList(ConnectionType c, std::vector <ServerItem *> & r);
		void saveServerList(ConnectionType c, std::vector <ServerItem *> & s);
	private:
		std::vector <ServerItem *> tygemlist;
		std::vector <ServerItem *> eweiqilist;
		std::vector <ServerItem *> tomlist;
};
