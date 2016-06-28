#ifndef _REDIS_SET_AGENT_H_
#define _REDIS_SET_AGENT_H_

#include "RedisAgent.h"
#include "RedisTypeAgent.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

class CRedisSetAgent : public CRedisTypeAgent
{
public:
	CRedisSetAgent(CRedisAgent *pRedisAgent):CRedisTypeAgent(pRedisAgent){}
	~CRedisSetAgent();
	int HasMember(const string &strKey, const string &strValue);
	int Set(const string &strKey, const string &strValue, int &result, unsigned int dwExpiration);
	int Delete(const string &strKey, const string &strValue);
	int Size(const string &strKey, int &size);
	int GetAll(const string &strKey, vector<string> &vecValue);
	int Inter();
	int Union();
private:
	int SaveBinaryDataInTransaction(const string &strKey,const string &strValue);
};
#endif
