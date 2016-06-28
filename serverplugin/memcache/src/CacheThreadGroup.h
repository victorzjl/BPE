#ifndef _CVS_CACHE_THREAD_GROUP_H_
#define _CVS_CACHE_THREAD_GROUP_H_

#include <MsgQueuePrio.h>
#include "CacheThread.h"
#include <string>
#include <vector>
#include "CacheMsg.h"

using std::string;
using std::vector;

using namespace sdo::common;

class CCacheVirtualService;
class CCacheThreadGroup
{
public:
	CCacheThreadGroup():m_pCacheVirtualService(NULL){}
	virtual ~CCacheThreadGroup();

	int Start( unsigned int dwThreadNum, const SERVICE_TYPE_CODE_MAP &mapCodeTypeByService, const CACHE_SET_TIME_MAP &mapCacheKeepTime, const vector<string> &vecCacheSvr, CCacheVirtualService *pCacheVirtualService, map<unsigned int, bool>&clearText, map<unsigned int, string>&mapKeyPrefix);
	void Stop();
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, const void *pBuffer, int nLen);
	void GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum );	
private:
	sdo::common::CMsgQueuePrio m_queue;
	vector<CCacheThread *> m_vecChildThread;
	vector<unsigned int> m_vecServiceIds;
	SERVICE_TYPE_CODE_MAP m_mapCodeTypeByService;
	CACHE_SET_TIME_MAP m_mapCacheKeepTime;
	map<unsigned int, bool> m_mapClearText;
	map<unsigned int, string> m_mapKeyPrefix;
	CCacheVirtualService *m_pCacheVirtualService;
};

#endif
