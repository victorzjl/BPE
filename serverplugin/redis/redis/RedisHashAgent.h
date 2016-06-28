#ifndef _REDIS_HASH_AGENT_H_
#define _REDIS_HASH_AGENT_H_

#include "RedisAgent.h"
#include "RedisTypeAgent.h"
#include "RedisMsg.h"

#include <string>
#include <vector>
#include <map>

using std::string;
using std::map;
using std::vector;
using redis::SKeyFieldValue;

class CRedisHashAgent : public CRedisTypeAgent
{
public:
	CRedisHashAgent(CRedisAgent *pRedisAgent):CRedisTypeAgent(pRedisAgent){}
	~CRedisHashAgent();
	int Get(const string &strKey, map<string, string> &mapFieldValue);
	//int SetAll(const string &strKey, const map<string, string> &mapFieldValue, unsigned int dwExpiration);
	int Set(const string &strKey, const map<string, string> &mapFieldValue, unsigned int dwExpiration);
	int Delete(const string &strKey, const vector<string> &vecField);
	int Size(const string &strKey, int &size);
	int GetAll(const string &strKey, map<string, string> &mapFieldValue);
	int DeleteAll(const string &strKey);
	int Incby(const string &strKey, const string &strField, const int increment);
	int BatchQuery(vector<SKeyFieldValue> &vecKFV);
	int BatchIncrby(vector<SKeyFieldValue> &vecKFV);
private:
	int SaveBinaryData(const string &strKey,const string &strField, const string &strValue);
	int GetBinaryOperation(const string &strKey,const string &strField, string &strValue);
	int DeleteBinaryOperation(const string &strKey,const string &strField);
};
#endif
