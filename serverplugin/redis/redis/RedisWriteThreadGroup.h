#ifndef _REDIS_WRITE_THREAD_GROUP_H_
#define _REDIS_WRITE_THREAD_GROUP_H_

#include <MsgQueuePrio.h>
#include "RedisThread.h"
#include <string>
#include <vector>
#include "RedisMsg.h"

using std::string;
using std::vector;

using namespace sdo::common;
using namespace redis;

class CRedisVirtualService;

class CRedisWriteThreadGroup
{
public:
	CRedisWriteThreadGroup():m_bWriteQueueFullDiscardFlag(true),m_pRedisVirtualService(NULL){}
	virtual ~CRedisWriteThreadGroup();
	
	int Start( unsigned int dwThreadNum, const vector<string> &vecRedisSvr, CRedisVirtualService *pRedisVirtualService);
	void Stop();
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, ERedisType eRedisType, const string &strGuid, void *pHandler, const void *pBuffer, int nLen);
	void OnAddRedisServer(const string &strAddr);
	void GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum );
private:
	sdo::common::CMsgQueuePrio m_queue;
	vector<CRedisThread *> m_vecChildThread;
	vector<unsigned int> m_vecServiceIds;

	bool m_bWriteQueueFullDiscardFlag;
	CRedisVirtualService *m_pRedisVirtualService;

};
#endif
