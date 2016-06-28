#ifndef _CVS_CACHE_RECONN_THREAD_H_
#define _CVS_CACHE_RECONN_THREAD_H_

#include <MsgTimerThread.h>
#include <string>
#include <vector>
#include "CacheAgent.h"

using std::string;
using std::vector;

using namespace sdo::common;

class CCacheThread;

typedef struct stCacheReConnMsg
{
	timeval_a tmLastWarn;		//上次告警时间
	vector<unsigned int> dwVecServiceId;
	string strAddr;
	CCacheThread *pCacheThread;
	CCacheAgent *pCacheAgent;
	bool bCacheAgentInitSucc;
	stCacheReConnMsg():pCacheThread(NULL), pCacheAgent(NULL), bCacheAgentInitSucc(false){}
	virtual ~stCacheReConnMsg(){}
}SCacheReConnMsg;

class CCacheVirtualService;
class CCacheReConnThread : public CMsgTimerThread
{
public:
	CCacheReConnThread();
	virtual ~CCacheReConnThread(){}
	int Start(CCacheVirtualService *pCacheVirtualService);
	void OnReConn( vector<unsigned int> dwVecServiceId, const string &strAddr, CCacheThread *pCacheThread);
	virtual void StartInThread();
	virtual void Deal(void *pData);
private:
	void DoReConn();
	void Warn(const string &strAddr);
private:
	vector<SCacheReConnMsg *> m_vecReConnMsg;
	CThreadTimer m_timerReConn;
	CCacheVirtualService *m_pCacheVirtualService;
	unsigned int m_dwAlarmFrequency;
};

#endif
