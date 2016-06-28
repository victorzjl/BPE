#ifndef _REDIS_TYPE_AGENT_H_
#define _REDIS_TYPE_AGENT_H_

#include "RedisAgent.h"
#include "RedisMsg.h"
#include <string>
using std::string;
using redis::SKeyValue;

class CRedisTypeAgent
{
public:
	CRedisTypeAgent(CRedisAgent *pRedisAgent):m_pRedisAgent(pRedisAgent){}
	virtual ~CRedisTypeAgent(){}
	
	int Del(const string &strKey);
	int Expire(const string &strKey, unsigned int dwExpiration);
	int ExpireInTransaction(const string &strKey, unsigned int dwExpiration); 
	int Ttl(const string &strKey, int &ttl); 
	int BatchExpire(vector<SKeyValue> &vecKeyValue);
	int BatchDelete(vector<SKeyValue> &vecKeyValue);
protected:
	CRedisAgent *m_pRedisAgent;
};
#endif
