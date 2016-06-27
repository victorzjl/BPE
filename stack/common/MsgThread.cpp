#include "MsgThread.h"
namespace sdo{
	namespace common{
		CMsgThread::CMsgThread()
		{
		}
        CMsgThread::~CMsgThread()
        {
        }
        void CMsgThread::Start(int nThread)
        {
            for(int i=0;i<nThread;i++)
    		{
    			m_threads.add_thread(new boost::thread(boost::bind(&CMsgThread::Run,this)));
    		}
        }
        void CMsgThread::Stop()
        {
			m_threads.interrupt_all();
		    m_threads.join_all();
        }
		void CMsgThread::PutQ(void *pData,EMsgPriority enPrio)
		{
			m_oQueue.PutQ(pData,enPrio);
		}
		bool CMsgThread::TimedPutQ(void *pData,EMsgPriority enPrio,int nSeconds)
		{
			return m_oQueue.TimedPutQ(pData,enPrio,nSeconds);
		}
		void  CMsgThread::Run()
		{
			void * pData;
            while(1)
            {
                pData=m_oQueue.GetQ();
			    Deal(pData);
            }
		}
	}
}
