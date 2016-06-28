#ifndef _REDIS_LIST_AGENT_SCHEDULER_H_
#define _REDIS_LIST_AGENT_SCHEDULER_H_
#include "RedisTypeAgentScheduler.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

class CRedisContainer;
class CRedisVirtualService;

class CRedisListAgentScheduler:public CRedisTypeAgentScheduler
{
public:
	CRedisListAgentScheduler(CRedisContainer *pRedisContainer):CRedisTypeAgentScheduler(pRedisContainer),m_pRedisVirtualService(NULL){}
	CRedisListAgentScheduler(CRedisContainer *pRedisContainer, CRedisVirtualService *pRedisVirtualService):CRedisTypeAgentScheduler(pRedisContainer),m_pRedisVirtualService(pRedisVirtualService){}
	~CRedisListAgentScheduler(){}
	int Get(const string &strKey, string &strValue, const int index , string &strIpAddrs);
	int Set(const string &strKey, const string &strValue, const int index , unsigned int dwExpiration, string &strIpAddrs);
	int Delete(const string &strKey, const string &strValue, const int count, string &strIpAddrs);
	int Size(const string &strKey, int &size, string &strIpAddrs);
	int GetAll(const string &strKey, vector<string> &vecValue, const int start, const int end, string &strIpAddrs);
	int PopBack(const string &strKey, string &strValue, string &strIpAddrs);
	int PushBack(const string &strKey, const string &strValue, string &strIpAddrs);
	int PopFront(const string &strKey, string &strValue, string &strIpAddrs);
	int PushFront(const string &strKey, const string &strValue, string &strIpAddrs);
	int Reserve(const string &strKey, const int start, const int end, string &strIpAddrs);
private:
	int GetFromSlave(const string &strKey, string &strValue, const int index , string &strIpAddrs);
	int SizeFromSlave(const string &strKey, int &size, string &strIpAddrs);
	int GetAllFromSlave(const string &strKey, vector<string> &vecValue, const int start, const int end, string &strIpAddrs);
private:
	CRedisVirtualService *m_pRedisVirtualService;
};
#endif
