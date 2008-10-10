#include <string.h>

#ifdef PROBABLY_DONT_NEED
ConnectionInfo::ConnectionInfo(char * h, unsigned int p, char * u, char * p) : user(0), pass(0)
{
	//memcpy(host, h, 4);
	port = p;
	/*if(u && strlen(u))
		user = new char[strlen(u)];
	if(p && strlen(p))
		pass = new char[strlen(p)];*/
}

ConnectionInfo::~ConnectionInfo()
{
	/*if(user)
		delete[] user;
	if(pass)
		delete[] pass;*/
}
#endif //PROBABLY_DONT_NEED
