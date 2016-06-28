#ifndef _REDIS_THREAD_H_
#define _REDIS_THREAD_H_

#include "IRedisTypeMsgOperator.h"
#include "RedisMsg.h"

#include <ChildMsgThread.h>
#include <vector>

using namespace sdo::common;
using namespace redis;
using std::vector;

class CRedisVirtualService;
class CRedisContainer;
class CRedisAgent;

class CRedisThread: public CChildMsgThread
{
	typedef void (CRedisThread::*RedisFunc)(SReidsTypeMsg *pRedisTypeMsg);
public:
	CRedisThread( CMsgQueuePrio & oPublicQueue, CRedisVirtualService *pRedisVirtualService);
    ~CRedisThread();
	virtual void StartInThread();
	virtual void Deal(void *pData);
	int Start(const vector<string> &vecRedisSvr, short readTag = 0);
	bool GetSelfCheck();
	void OnAddRedisServer(const string &strAddr);
	CRedisAgent * GetRedisAgentByAddr(const string &strAddr);
	void DelRedisServer(const string &strAddr);

private:
	void DoStringTypeMsg(SReidsTypeMsg *pRedisTypeMsg);
	void DoSetTypeMsg(SReidsTypeMsg *pRedisTypeMsg);
	void DoZsetTypeMsg(SReidsTypeMsg *pRedisTypeMsg);
	void DoListTypeMsg(SReidsTypeMsg *pRedisTypeMsg);
	void DoHashTypeMsg(SReidsTypeMsg *pRedisTypeMsg);
	void DoAddRedisServer(SReidsTypeMsg *pRedisTypeMsg);
private:	
	void DoSelfCheck();
	CRedisContainer *GetRedisContainer(unsigned int dwServiceId);
	void connectRedisServer(const vector<string> &vecAddrs, map<string, bool> &mapRedisAddr, map<string, bool> &mapNotConnectedAddr);
	
private:
	RedisFunc m_redisFunc[RVS_REDIS_TYPE_ALL];
	IRedisTypeMsgOperator * m_redisTypeMsgOperator[RVS_REDIS_ADD_SERVER];
	
	map<unsigned int, CRedisContainer *> m_mapRedisContainer;
	map<string, CRedisAgent *> m_mapRedisAgent;
	CRedisVirtualService *m_pRedisVirtualService;
	
	bool			m_isAlive;
	unsigned int	m_dwTimeoutNum;
	unsigned int	m_dwTimeoutInterval;
	unsigned int	m_dwConditionTimes;
	unsigned int	m_dwTimeoutAlarmFrequency;
	timeval_a		m_tmLastTimeoutWarn;
	
	CThreadTimer	m_timerSelfCheck;
};
#endif
