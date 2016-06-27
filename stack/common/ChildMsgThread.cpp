#include "ChildMsgThread.h"

namespace sdo{
	namespace common{
		CChildMsgThread::CChildMsgThread(CMsgQueuePrio & oPublicQueue,  bool bSelfPriority):m_oPublicQueue(oPublicQueue),
			m_bSelfPriority(bSelfPriority)
		{
		}
		CChildMsgThread::~CChildMsgThread()
		{
		}
		void CChildMsgThread::Start()
		{
			m_thread=boost::thread(boost::bind(&CChildMsgThread::Run,this));
		}
		void CChildMsgThread::Stop()
		{
			this->m_thread.interrupt();
			this->m_thread.join();
		}
		void CChildMsgThread::PutQ(void *pData,EMsgPriority enPrio)
		{
			m_oQueue.PutQ(pData,enPrio);
		}
		bool CChildMsgThread::TimedPutQ(void *pData,EMsgPriority enPrio,int nSeconds)
		{
			return m_oQueue.TimedPutQ(pData,enPrio,nSeconds);
		}

		void  CChildMsgThread::Run()
		{
			void * pData=NULL;
			Reset();
			StartInThread();
			while(1)
			{
				if(m_bSelfPriority)
				{
					
					pData=m_oQueue.TimedGetQ(1);
					if(pData!=NULL)
						Deal(pData);
					if(!m_oPublicQueue.IsEmpty())
					{
						pData=m_oPublicQueue.TimedGetQ(1);
						if(pData!=NULL)
							Deal(pData);
					}
				}
				else 
				{
					pData=m_oPublicQueue.TimedGetQ(1);
					if(pData!=NULL)
						Deal(pData);
					if(!m_oQueue.IsEmpty())
					{
						pData=m_oQueue.TimedGetQ(1);
						if(pData!=NULL)
							Deal(pData);
					}
				}
				DetectTimerList();
			}
		}
	}
}

