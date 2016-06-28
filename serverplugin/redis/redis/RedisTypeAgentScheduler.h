#ifndef _REDIS_TYPE_AGENT_SCHEDULER_H_
#define _REDIS_TYPE_AGENT_SCHEDULER_H_

#include "RedisMsg.h"
#include<string>
#include<vector>
using std::string;
using std::vector;
using redis::SKeyValue;

class CRedisContainer;
class CRedisTypeAgentScheduler
{
public:
	CRedisTypeAgentScheduler(CRedisContainer *pRedisContainer):m_pRedisContainer(pRedisContainer){}
	virtual ~CRedisTypeAgentScheduler(){}
	int Ttl(const string &strKey, int &ttl, string &strIpAddrs);
	int DeleteAll(const string &strKey, string &strIpAddrs);
	int BatchExpire(vector<SKeyValue> &vecKeyValue, string &strIpAddrs);
	int BatchDelete(vector<SKeyValue> &vecKeyValue, string &strIpAddrs);
private:
	int GetTtlFromSlave(const string &strKey, int &ttl, string &strIpAddrs);
protected:
	CRedisContainer *m_pRedisContainer;
};
#endif
