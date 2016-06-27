#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_
#include "ChildMsgThread.h"
#include <vector>
#include "MsgQueuePrio.h"

using std::vector;
namespace sdo{
namespace common{
const int THREAD_GROUP_SIZE=102400;
template<typename ChildThreadType>
class CThreadGroup
{
public:
	CThreadGroup():m_queue(THREAD_GROUP_SIZE){}
	void Start(int nThread)
	{
		for(int i=0;i<nThread;i++)
		{
			CChildMsgThread *pThread=new ChildThreadType(m_queue);
			m_vecChildThread.push_back(pThread);
			pThread->Start();
		}
	}
	void Stop()
	{
		CChildMsgThread *pThread=NULL;
		while(!m_vecChildThread.empty())
		{
			pThread=*(m_vecChildThread.rbegin());
			m_vecChildThread.pop_back();
			pThread->Stop();
			delete pThread;
		}
	}
	void PutQ(void *pData)
	{
		m_queue.PutQ(pData);
	}
	bool TimedPutQ(void *pData)
	{
		return m_queue.TimedPutQ(pData,MSG_MEDIUM);
	}

	void PutQAll(void *pData)
	{
		CChildMsgThread *pThread=NULL;
		vector<CChildMsgThread *>::iterator itr;
		for(itr=m_vecChildThread.begin();itr!=m_vecChildThread.end();++itr)
		{
			pThread=*itr;
			pThread->PutQ(pData);
		}
	}


private:
	CMsgQueuePrio m_queue;
	vector<CChildMsgThread *> m_vecChildThread;
};
}
}
#endif
