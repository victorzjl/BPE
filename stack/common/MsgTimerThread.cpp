#include "MsgTimerThread.h"

namespace sdo{
	namespace common{
		CMsgTimerThread::CMsgTimerThread()
		{
		}
        CMsgTimerThread::~CMsgTimerThread()
        {
        }
        void CMsgTimerThread::Start()
        {
            m_thread=boost::thread(boost::bind(&CMsgTimerThread::Run,this));
        }
        void CMsgTimerThread::Stop()
        {
            this->m_thread.interrupt();
			this->m_thread.join();
        }
		void CMsgTimerThread::PutQ(void *pData,EMsgPriority enPrio)
		{
			m_oQueue.PutQ(pData,enPrio);
		}
		bool CMsgTimerThread::TimedPutQ(void *pData,EMsgPriority enPrio,int nSeconds)
		{
			return m_oQueue.TimedPutQ(pData,enPrio,nSeconds);
		}
       

		void  CMsgTimerThread::Run()
		{
			void * pData=NULL;
            Reset();
			StartInThread();
            while(1)
            {
                pData=m_oQueue.TimedGetQ(1);
                if(pData!=NULL)
			        Deal(pData);
                DetectTimerList();
            }
		}
	}
}

