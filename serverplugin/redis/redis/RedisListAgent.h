#ifndef _REDIS_LIST_AGENT_H_
#define _REDIS_LIST_AGENT_H_

#include "RedisAgent.h"
#include "RedisTypeAgent.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

class CRedisVirtualService;

class CRedisListAgent : public CRedisTypeAgent
{
public:
	CRedisListAgent(CRedisAgent *pRedisAgent):CRedisTypeAgent(pRedisAgent),m_pRedisVirtualService(NULL){}
	CRedisListAgent(CRedisAgent *pRedisAgent, CRedisVirtualService *pRedisVirtualService):CRedisTypeAgent(pRedisAgent),m_pRedisVirtualService(pRedisVirtualService){}
	~CRedisListAgent(){}
	int Get(const string &strKey, string &strValue, const int index);
	int Set(const string &strKey, const string &strValue, const int index);
	int Delete(const string &strKey, const string &strValue, const int count);
	int Size(const string &strKey, int &size);
	int GetAll(const string &strKey, vector<string> &vecValue, const int start = 0, const int end = -1);
	int DeleteAll(const string &strKey);
	int PopBack(const string &strKey, string &strValue);
	int PushBack(const string &strKey, const string &strValue);
	int PopFront(const string &strKey, string &strValue);
	int PushFront(const string &strKey, const string &strValue);
	int Reserve(const string &strKey, const int start, const int end);
private:
	void WarnLength(const unsigned int currentLength, const string &strAddr);	
private:
	CRedisVirtualService *m_pRedisVirtualService;
};
#endif
