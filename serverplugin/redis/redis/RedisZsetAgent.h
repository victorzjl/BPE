#ifndef _REDIS_ZSET_AGENT_H_
#define _REDIS_ZSET_AGENT_H_

#include "RedisAgent.h"
#include "RedisTypeAgent.h"

#include <string>
#include <map>
using std::string;
using std::map;

class CRedisZsetAgent : public CRedisTypeAgent
{
public:
	CRedisZsetAgent(CRedisAgent *pRedisAgent):CRedisTypeAgent(pRedisAgent){}
	~CRedisZsetAgent(){}
	int Get(const string &strKey, const string &strValue, int &score);
	int Set(const string &strKey, const string &strValue, const int score, unsigned int dwExpiration, int &result);
	int Delete(const string &strKey, const string &strField);
	int Size(const string &strKey, int &size);
	int GetAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore);
	int GetReverseAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore);
	int GetScoreAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, bool bInputStart = true, bool bInputEnd = true);
	int Rank(const string &strKey, const string &strField, int &rank);
	int ReverseRank(const string &strKey, const string &strField, int &rank);
	int Incby(const string &strKey, const string &strField, const int score);
	int DelRangeByRank(const string &strKey, const int start, const int end);
	int DelRangeByScore(const string &strKey, const int start, const int end, bool bInputStart = true, bool bInputEnd = true);
private:
	int SaveBinaryDataInTransaction(const string &strKey,const string &strValue, const int score);
};
#endif
