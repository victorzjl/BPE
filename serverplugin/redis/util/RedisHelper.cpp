#include "RedisHelper.h"
#include "RedisMsg.h"

using namespace redis;

void CRedisHelper::AddIpAddrs(string &strIpAddrs, const string &strAddr)
{
	
	if(strIpAddrs.empty())
	{
		strIpAddrs += strAddr;
	}
	else
	{
		strIpAddrs += VALUE_SPLITTER + strAddr;
	}
}

void CRedisHelper::StringSplit(const string &s, const string &delim, vector<string> &vecString)
{
    if(s.empty())
    {
    	return;
   	}
    char *p = strtok(const_cast<char*>(s.c_str()), delim.c_str());
	while(p != NULL)
	{
		vecString.push_back(string(p));
		p = strtok(NULL, delim.c_str());
	}
}


