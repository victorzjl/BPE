#ifndef _SDO_COMMON_MSG_THREAD_H_
#define _SDO_COMMON_MSG_THREAD_H_
#include "MsgQueuePrio.h"

namespace sdo{
	namespace common{
		class CMsgThread
		{
			public:
				CMsgThread();
				void Start(int nThread=1);
				void Stop();
				bool IsQueueFull(){return m_oQueue.IsFull();}
				virtual ~CMsgThread();
				void PutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM);
				bool TimedPutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM,int nSeconds=3);
				virtual void Deal(void *pData)=0;
			private:
				void Run();
				CMsgQueuePrio m_oQueue;
				boost::thread_group m_threads;
		};
	}
}
#endif

