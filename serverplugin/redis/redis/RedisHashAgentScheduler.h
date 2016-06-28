#ifndef _REDIS_HASH_AGENT_SCHEDULER_H_
#define _REDIS_HASH_AGENT_SCHEDULER_H_

#include "RedisTypeAgentScheduler.h"
#include "RedisMsg.h"
#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;
using redis::SKeyFieldValue;

class CRedisContainer;
class CRedisHashAgentScheduler:public CRedisTypeAgentScheduler
{
public:
	CRedisHashAgentScheduler(CRedisContainer *pRedisContainer):CRedisTypeAgentScheduler(pRedisContainer){}
	~CRedisHashAgentScheduler(){}
	int Get(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs);
	int Set(const string &strKey, const map<string, string> &mapFieldValue, unsigned int dwExpiration, string &strIpAddrs);
	int Delete(const string &strKey, const vector<string> &vecField, string &strIpAddrs);
	int Size(const string &strKey, int &size, string &strIpAddrs);
	int GetAll(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs);
	int Incby(const string &strKey, const string &strField, const int increment, string &strIpAddrs);
	int BatchQuery(vector<SKeyFieldValue> &vecKFV, string &strIpAddrs);
	int BatchIncrby(vector<SKeyFieldValue> &vecKFV, string &strIpAddrs);
private:
	int GetFromSlave(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs);
	int SizeFromSlave(const string &strKey, int &size, string &strIpAddrs);
	int GetAllFromSlave(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs);
	int BatchQueryFromSlave(vector<SKeyFieldValue> &vecKFV, string &strIpAddrs);
};
#endif
