#ifndef _REDIS_STRING_AGENT_H_
#define _REDIS_STRING_AGENT_H_

#include "RedisAgent.h"
#include "RedisTypeAgent.h"
#include "RedisMsg.h"

#include <string>
#include <map>
#include <vector>
using std::string;
using std::map;
using std::vector;
using namespace redis;

class CRedisStringAgent : public CRedisTypeAgent
{
public:
	CRedisStringAgent(CRedisAgent *pRedisAgent):CRedisTypeAgent(pRedisAgent){}
	~CRedisStringAgent();
	int Get(const string &strKey, string &strValue);
	int Set(const string &strKey, const string &strValue, unsigned int dwExpiration);
	int Delete(const string &strKey);
	int Incby(const string &strKey, const int incbyNum);
	int BatchQuery(vector<SKeyValue> &vecKeyValue);
	int BatchIncrby(vector<SKeyValue> &vecKeyValue);
};
#endif
