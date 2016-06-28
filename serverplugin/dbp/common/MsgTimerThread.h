#ifndef _MSG_TIMER_THREAD_H_
#define _MSG_TIMER_THREAD_H_
#include <boost/thread.hpp>
#include "MsgQueuePrio.h"
#include "TimerManager.h"


namespace sdo{
	namespace dbp{
		class CMsgTimerThread:public CTimerManager
		{
			public:
				CMsgTimerThread();
				void Start();
				void Stop();
				bool IsQueueFull(){return m_oQueue.IsFull();}
				virtual ~CMsgTimerThread();
				void PutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM);
				bool TimedPutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM,int nSeconds=3);
				virtual void StartInThread(){}
				virtual void Deal(void *pData)=0;
			private:
				void Run();
				CMsgQueuePrio m_oQueue;
				boost::thread m_thread;
		};
	}
}
#endif

