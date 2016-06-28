#ifndef _CHILD_MSG_THREAD_H_
#define _CHILD_MSG_THREAD_H_
#include <boost/thread.hpp>
#include "MsgQueuePrio.h"
#include "TimerManager.h"

namespace sdo{
namespace dbp{
class CChildMsgThread:public CTimerManager
{
public:
	CChildMsgThread(CMsgQueuePrio & oPublicQueue, bool bSelfPriority = false);
	void Start();
	void Stop();
	virtual ~CChildMsgThread();
	void PutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM);
	bool TimedPutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM,int nSeconds=3);
	int GetCurrentCount();
	virtual void StartInThread(){};
	virtual void Deal(void *pData){};
private:
	void Run();
	CMsgQueuePrio & m_oPublicQueue;
	CMsgQueuePrio m_oQueue;
	boost::thread m_thread;
	bool m_bSelfPriority;
	
};
}
}
#endif

