#include "tomconnection.h"
#include "../room.h"
#include "serverlistdialog.h"
#include "serverliststorage.h"
#include "codecwarndialog.h"

TomConnection::TomConnection(const QString & user, const QString & pass)
: TygemConnection(user, pass, TypeTOM)
{
	serverCodec = QTextCodec::codecForName("GB2312");
	if(!serverCodec)
	{
		new CodecWarnDialog("GB2312");
		serverCodec = QTextCodec::codecForLocale();
	}
	if(!getServerListStorage().restoreServerList(TypeTOM, serverList))
			requestServerInfo();
	else
	{
		if(reconnectToServer() < 0)
		{
			qDebug("User canceled");
			connectionState = CANCELED;
			return;
		}
	}
}

int TomConnection::requestServerInfo(void)
{
	qDebug("Requesting Tom Server Info");
	if(!openConnection("61.135.158.147", 80))
	{
		qDebug("Can't get server info");
		return -1;
	}
	unsigned int length = 0x94;
	unsigned char * packet = new unsigned char[length];  //term 0x00
	snprintf((char *)packet, length,
		 "GET /service/files/livebaduk3.cfg HTTP/1.1\r\n" \
		 "User-Agent: Tygem HTTP\r\n" \
		 "Host: duiyi.sports.tom.com\r\n" \
		 "Connection: Keep-Alive\r\n" \
		 "Cache-Control: no-cache\r\n\r\n");
#ifdef RE_DEBUG
	for(int i = 0; i < length; i++)
		printf("%02x", packet[i]);
	printf("\n");
#endif //RE_DEBUG
	if(write((const char *)packet, length) < 0)
	{
		qWarning("*** failed sending server info request");
		return -1;
	}
	delete[] packet;
	
	connectionState = INFO;
	return 0;
}

/* We need to write a tygem parser that doesn't need to be different for tom FIXME */
void TomConnection::handleServerInfo(unsigned char * msg, unsigned int length)
{
	char * p = (char *)msg;
	int i, j;
	ServerItem * si;

#ifdef RE_DEBUG
	for(i = 0; i < length; i++)
		printf("%c", p[i]);
	printf("\n");
#endif //RE_DEBUG
	
	while(p < ((char *)msg + length))
	{
		while(p < ((char *)msg + length - 1) && (p[0] != '\r' || p[1] != '\n'))
			p++;
		p += 2;
		//FIXME, check for size
		if(strncmp(p, "/LIVE ", 6) == 0)
		{
			//same as for /SERVER but with only 2 [], name and ip
		}
		else if(strncmp(p, "/LECTURE ", 9) == 0)
		{
			//same as for /SERVER but with only 2 [], name and ip
		}
		else if(strncmp(p, "/SERVER ", 8) == 0 ||
		        strncmp(p, "/_SERVER ", 9) == 0)	//whats the "_" FIXME
		{
			//TOM has only 2 [], name and ip
			if(p[1] == '_')
				p++;
			p += 8;
			unsigned char ipaddr[16];
			unsigned char name[20];
			i = 0;
			while(i < 2 && p < ((char *)msg + length))
			{
				if(*p != '[')
				{
					qDebug("Server info parse error");
					closeConnection();
					return;
				}
				p++;
				si = new ServerItem();
				unsigned char * dest;
				if(i == 1)
					dest = ipaddr;
				else if(i == 0)
					dest = name;
				j = 0;
				while(*p != ']' && p < ((char *)msg + length))
				{
					if((i == 1 && j < 15) || (i == 0 && j < 19))
						dest[j++] = *p;
					p++;
				}
				if(i == 0 || i == 1)
				{
					dest[j++] = 0x00;
				}
				p += 2;
				if(i == 1)
				{
					strncpy(si->ipaddress, (const char *)ipaddr, 16);
					si->name =  serverCodec->toUnicode((char *)name, strlen((char *)name));
					qDebug("Server: %s", ipaddr);
					serverList.push_back(si);
				}
				i++;
				if(i == 7)
					p--;	//no space
			}
		}
		else if(strncmp(p, "/ENCRYPT ", 9) == 0)
		{
			//FIXME
			//[encode] [on]	
		}
	}
	/* We close here because this first time, its going to close
	* anyway, and if we don't close here, we'll get an error
	* and lose the object */
	connectionState = RECONNECTING;
	closeConnection(false);
	
	if(reconnectToServer() < 0)
	{
		qDebug("User canceled");
		closeConnection(false);
		connectionState = CANCELED;
		//if(dispatch)
		//	dispatch->onError();	//not great... FIXME
		return;
	}
	//sendLogin();
}
