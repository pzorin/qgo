#include "serverliststorage.h"
#include "serverlistdialog.h"

ServerListStorage::~ServerListStorage()
{
	std::vector<ServerItem *>::iterator iter;
	for(iter = tygemlist.begin(); iter != tygemlist.end(); iter++)
		delete *iter;
	for(iter = eweiqilist.begin(); iter != tygemlist.end(); iter++)
		delete *iter;
	for(iter = tomlist.begin(); iter != tygemlist.end(); iter++)
		delete *iter;
}

bool ServerListStorage::restoreServerList(ConnectionType c, std::vector <ServerItem *> & r)
{
	std::vector<ServerItem *>::iterator iter;
	switch(c)
	{
		case TypeTYGEM:
			if(!tygemlist.size())
				return false;
			for(iter = tygemlist.begin(); iter != tygemlist.end(); iter++)
				r.push_back(*iter);
			break;
		case TypeEWEIQI:
			if(!eweiqilist.size())
				return false;
			for(iter = eweiqilist.begin(); iter != eweiqilist.end(); iter++)
				r.push_back(*iter);
			break;
		case TypeTOM:
			if(!tomlist.size())
				return false;
			for(iter = tomlist.begin(); iter != tomlist.end(); iter++)
				r.push_back(*iter);
			break;
		default:
			qDebug("Can't restore server list for unknown connection type");
			break;
	}
	return true;
}

void ServerListStorage::saveServerList(ConnectionType c, std::vector <ServerItem *> & s)
{
	std::vector<ServerItem *>::iterator iter;
	switch(c)
	{
		case TypeTYGEM:
			for(iter = s.begin(); iter != s.end(); iter++)
				tygemlist.push_back(*iter);
			break;
		case TypeEWEIQI:
			for(iter = s.begin(); iter != s.end(); iter++)
				eweiqilist.push_back(*iter);
			break;
		case TypeTOM:
			for(iter = s.begin(); iter != s.end(); iter++)
				tomlist.push_back(*iter);
			break;
		default:
			qDebug("Can't restore server list for unknown connection type");
			break;
	}
}
