#ifndef _REDIS_SET_AGENT_SCHEDULER_H_
#define _REDIS_SET_AGENT_SCHEDULER_H_
#include "RedisTypeAgentScheduler.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

class CRedisContainer;

class CRedisSetAgentScheduler:public CRedisTypeAgentScheduler
{
public:
	CRedisSetAgentScheduler(CRedisContainer *pRedisContainer):CRedisTypeAgentScheduler(pRedisContainer){}
	~CRedisSetAgentScheduler(){}

	int HasMember(const string &strKey, const string &strValue, string &strIpAddrs);
	int Set(const string &strKey, const string &strValue, int &result, unsigned int dwExpiration, string &strIpAddrs);
	int Delete(const string &strKey, const string &strValue, string &strIpAddrs);
	int Size(const string &strKey, int &size, string &strIpAddrs);
	int GetAll(const string &strKey, vector<string> &vecValue, string &strIpAddrs);
private:
	int HasMemberFromSlave(const string &strKey, const string &strValue, string &strIpAddrs);
	int SizeFromSlave(const string &strKey, int &size, string &strIpAddrs);
	int GetAllFromSlave(const string &strKey, vector<string> &vecValue, string &strIpAddrs);
};
#endif
