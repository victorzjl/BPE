#ifndef _HPS_COMMON_FUNC_H_
#define _HPS_COMMON_FUNC_H_
#include <sstream>
#include <sys/types.h> 
//#include <sys/socket.h> 
//#include <sys/ioctl.h> 
//#include <netinet/in.h> 
//#include <net/if.h> 
//#include <net/if_arp.h> 
//#include <arpa/inet.h> 
//#include <unistd.h>
//#include <net/route.h>
#include <errno.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <signal.h>
#include <vector>
#include <string>

#ifdef WIN32
    #include <WinSock2.h>
	#include <objbase.h>
#else
	#include <uuid/uuid.h>
	typedef struct GUID
	{
		unsigned int Data1;
		unsigned short Data2;
		unsigned short Data3;
		unsigned char Data4[8];
	}GUID, *PGUUID;
#endif

using std::string;

inline void split(const string& s, const string& delim, std::vector<string>* ret)
{
	size_t last = 0;
	size_t index=s.find_first_of(delim,last);
	while (index!=std::string::npos)
	{
		ret->push_back(s.substr(last,index-last));
		last=index+1;
		index=s.find_first_of(delim,last);
	}
	if (index-last>0)
	{
		ret->push_back(s.substr(last,index-last));
	}
} 



inline string getguid()
{
	try
	{
		char pUUID[128] = {0};
		GUID uuid;
#ifdef WIN32
		CoCreateGuid(&uuid);
#else
		uuid_generate(reinterpret_cast<unsigned char *>(&uuid));
#endif
		sprintf(pUUID,"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",uuid.Data1,uuid.Data2,uuid.Data3,uuid.Data4[0],uuid.Data4[1],uuid.Data4[2],uuid.Data4[3],uuid.Data4[4],uuid.Data4[5],uuid.Data4[6],uuid.Data4[7]);
		return pUUID;
	}
	catch(...)
	{
	}
	return "";
}

#endif
