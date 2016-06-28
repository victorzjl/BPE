#ifndef _REDIS_STRING_AGENT_SCHEDULER_H_
#define _REDIS_STRING_AGENT_SCHEDULER_H_
#include "RedisTypeAgentScheduler.h"
#include "RedisMsg.h"

#include <string>
#include <vector>
using std::string;
using std::vector;
using redis::SKeyValue;

class CRedisContainer;

class CRedisStringAgentScheduler:public CRedisTypeAgentScheduler
{
public:
	CRedisStringAgentScheduler(CRedisContainer *pRedisContainer):CRedisTypeAgentScheduler(pRedisContainer){}
	~CRedisStringAgentScheduler(){}
	int Get(const string &strKey, string &strValue, string &strIpAddrs);
	int Set(const string &strKey, const string &strValue, unsigned int dwExpiration, string &strIpAddrs);
	int Delete(const string &strKey, string &strIpAddrs);
	int Incby(const string &strKey, const int incbyNum, string &strIpAddrs);
	int BatchQuery(vector<SKeyValue> &vecKeyValue, string &strIpAddrs);
	int BatchIncrby(vector<SKeyValue> &vecKeyValue, string &strIpAddrs);
private:
	int GetFromSlave(const string &strKey, string &strValue, string &strIpAddrs);
	int BatchQueryFromSlave(vector<SKeyValue> &vecKeyValue, string &strIpAddrs);
};
#endif
