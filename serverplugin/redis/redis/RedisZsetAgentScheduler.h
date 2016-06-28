#ifndef _REDIS_ZSET_AGENT_SCHEDULER_H_
#define _REDIS_ZSET_AGENT_SCHEDULER_H_
#include "RedisTypeAgentScheduler.h"

#include <string>
#include <map>
using std::string;
using std::map;

class CRedisContainer;

class CRedisZsetAgentScheduler:public CRedisTypeAgentScheduler
{
public:
	CRedisZsetAgentScheduler(CRedisContainer *pRedisContainer):CRedisTypeAgentScheduler(pRedisContainer){}
	~CRedisZsetAgentScheduler(){}
	int Get(const string &strKey, const string &strValue, int &score, string &strIpAddrs);
	int Set(const string &strKey, const string &strValue, const int score, unsigned int dwExpiration, int &result, string &strIpAddrs);
	int Delete(const string &strKey, const string &strField, string &strIpAddrs);
	int Size(const string &strKey, int &size, string &strIpAddrs);
	int GetAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs);
	int GetReverseAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs);
	int GetScoreAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs, bool bInputStart, bool bInputEnd);
	int Rank(const string &strKey, const string &strField, int &rank, string &strIpAddrs);
	int ReverseRank(const string &strKey, const string &strField, int &rank, string &strIpAddrs);
	int Incby(const string &strKey, const string &strField, const int score, string &strIpAddrs);
	int DelRangeByRank(const string &strKey, const int start, const int end, string &strIpAddrs);
	int DelRangeByScore(const string &strKey, const int start, const int end, string &strIpAddrs, bool bInputStart, bool bInputEnd);
private:
	int GetFromSlave(const string &strKey, const string &strValue, int &score, string &strIpAddrs);
	int SizeFromSlave(const string &strKey, int &size, string &strIpAddrs);
	int GetAllFromSlave(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs);
	int GetReverseAllFromSlave(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs);
	int GetScoreAllFromSlave(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs, bool bInputStart, bool bInputEnd);
	int RankFromSlave(const string &strKey, const string &strField, int &rank, string &strIpAddrs);
	int ReverseRankFromSlave(const string &strKey, const string &strField, int &rank, string &strIpAddrs);
};
#endif
