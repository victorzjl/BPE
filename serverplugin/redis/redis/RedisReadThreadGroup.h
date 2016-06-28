#ifndef _RVS_REDIS_READ_THREAD_GROUP_H_
#define _RVS_REDIS_READ_THREAD_GROUP_H_

#include "RedisThread.h"
#include "RedisMsg.h"

#include <MsgQueuePrio.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace redis;
using namespace sdo::common;

class CRedisVirtualService;

class CRedisReadThreadGroup
{
public:
	CRedisReadThreadGroup():m_pRedisVirtualService(NULL){}
	virtual ~CRedisReadThreadGroup();
	
	int Start( unsigned int dwThreadNum, const vector<string> &vecRedisSvr, CRedisVirtualService *pRedisVirtualService, short readTag = 0);
	void Stop();
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, ERedisType eRedisType, const string &strGuid, void *pHandler, const void *pBuffer, int nLen);
    void OnAddRedisServer(const string &strAddr);
	void GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum );	
private:
	sdo::common::CMsgQueuePrio m_queue;
	vector<CRedisThread *> m_vecChildThread;

	CRedisVirtualService *m_pRedisVirtualService;
};

#endif
