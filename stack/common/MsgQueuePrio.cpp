#include "MsgQueuePrio.h"
#include <boost/date_time.hpp>
namespace sdo{
	namespace common{
		CMsgQueuePrio::CMsgQueuePrio(int nHighQueueSize,int nMediumQueueSize,int nLowQueueSize):
			m_nEmptyThread(0)
		{
			m_aQueues[MSG_HIGH].ppBuffer=new void *[nHighQueueSize];
			m_aQueues[MSG_HIGH].nSizeQueue=nHighQueueSize;
			m_aQueues[MSG_HIGH].nLocPut=0;
			m_aQueues[MSG_HIGH].nLocGet=0;
			m_aQueues[MSG_HIGH].nFullThread=0;
			m_aQueues[MSG_HIGH].nData=0;

			m_aQueues[MSG_MEDIUM].ppBuffer=new void *[nMediumQueueSize];
			m_aQueues[MSG_MEDIUM].nSizeQueue=nMediumQueueSize;
			m_aQueues[MSG_MEDIUM].nLocPut=0;
			m_aQueues[MSG_MEDIUM].nLocGet=0;
			m_aQueues[MSG_MEDIUM].nFullThread=0;
			m_aQueues[MSG_MEDIUM].nData=0;

			m_aQueues[MSG_LOW].ppBuffer=new void *[nLowQueueSize];
			m_aQueues[MSG_LOW].nSizeQueue=nLowQueueSize;
			m_aQueues[MSG_LOW].nLocPut=0;
			m_aQueues[MSG_LOW].nLocGet=0;
			m_aQueues[MSG_LOW].nFullThread=0;
			m_aQueues[MSG_LOW].nData=0;
		}

		void CMsgQueuePrio::PutQ(void *pData,EMsgPriority enPrio)
		{
			SSingleQueue *ptrQueue=&(m_aQueues[enPrio]);
			boost::unique_lock<boost::mutex> lock(m_mut);
			while(ptrQueue->nLocPut==ptrQueue->nLocGet&&ptrQueue->nData)
			{ 
				ptrQueue->nFullThread++;
				ptrQueue->condPut.wait(lock);
				ptrQueue->nFullThread--;
			}
			ptrQueue->ppBuffer[ptrQueue->nLocPut++]=pData;
			ptrQueue->nData++;
			if(ptrQueue->nLocPut==ptrQueue->nSizeQueue)
			{
				ptrQueue->nLocPut=0;
			}
			if(m_nEmptyThread)
			{
				m_condGet.notify_one();
			}
		}

		void * CMsgQueuePrio::GetQ()
		{
			void *pData;
			SSingleQueue *ptrQueue=NULL;
			boost::unique_lock<boost::mutex> lock(m_mut);
			while(ptrQueue==NULL)
			{
				if(m_aQueues[MSG_HIGH].nLocGet!=m_aQueues[MSG_HIGH].nLocPut||m_aQueues[MSG_HIGH].nData!=0)
				{
					ptrQueue=&(m_aQueues[MSG_HIGH]);
				}
				else if(m_aQueues[MSG_MEDIUM].nLocGet!=m_aQueues[MSG_MEDIUM].nLocPut||m_aQueues[MSG_MEDIUM].nData!=0)
				{
					ptrQueue=&(m_aQueues[MSG_MEDIUM]);
				}
				else if(m_aQueues[MSG_LOW].nLocGet!=m_aQueues[MSG_LOW].nLocPut||m_aQueues[MSG_LOW].nData!=0)
				{
					ptrQueue=&(m_aQueues[MSG_LOW]);
				}
				if(ptrQueue!=NULL)
				{
					pData=ptrQueue->ppBuffer[ptrQueue->nLocGet++];
					ptrQueue->nData--;
					if(ptrQueue->nLocGet==ptrQueue->nSizeQueue)
					{
						ptrQueue->nLocGet=0;
					}
					if(ptrQueue->nFullThread) 
					{  
						ptrQueue->condPut.notify_one();
					}
					return pData;
				}
				else
				{
					m_nEmptyThread++;
					m_condGet.wait(lock);
					m_nEmptyThread--;     
				}
			}
            return NULL;
		}

		bool CMsgQueuePrio::TimedPutQ(void *pData,EMsgPriority enPrio,int nSeconds)
		{
			SSingleQueue *ptrQueue=&(m_aQueues[enPrio]);
			boost::unique_lock<boost::mutex> lock(m_mut);
			while(ptrQueue->nLocPut==ptrQueue->nLocGet&&ptrQueue->nData)
			{ 
				if(nSeconds==0) 
				{
					return false;
				}
				
				ptrQueue->nFullThread++;
				if(ptrQueue->condPut.timed_wait<boost::posix_time::seconds>(lock,boost::posix_time::seconds(nSeconds))==false) 
				{
					ptrQueue->nFullThread--;
					return false;
				}
				ptrQueue->nFullThread--;
			}
			ptrQueue->ppBuffer[ptrQueue->nLocPut++]=pData;
			ptrQueue->nData++;
			if(ptrQueue->nLocPut==ptrQueue->nSizeQueue)
			{
				ptrQueue->nLocPut=0;
			}
			if(m_nEmptyThread)
			{
				m_condGet.notify_one();
			}
			return true;
		}

		void * CMsgQueuePrio::TimedGetQ(int nSeconds)
		{
			void *pData;
			SSingleQueue *ptrQueue=NULL;
			boost::unique_lock<boost::mutex> lock(m_mut);
			while(ptrQueue==NULL)
			{
				if(m_aQueues[MSG_HIGH].nLocGet!=m_aQueues[MSG_HIGH].nLocPut||m_aQueues[MSG_HIGH].nData!=0)
				{
					ptrQueue=&(m_aQueues[MSG_HIGH]);
				}
				else if(m_aQueues[MSG_MEDIUM].nLocGet!=m_aQueues[MSG_MEDIUM].nLocPut||m_aQueues[MSG_MEDIUM].nData!=0)
				{
					ptrQueue=&(m_aQueues[MSG_MEDIUM]);
				}
				else if(m_aQueues[MSG_LOW].nLocGet!=m_aQueues[MSG_LOW].nLocPut||m_aQueues[MSG_LOW].nData!=0)
				{
					ptrQueue=&(m_aQueues[MSG_LOW]);
				}
				if(ptrQueue!=NULL)
				{
					pData=ptrQueue->ppBuffer[ptrQueue->nLocGet++];
					ptrQueue->nData--;
					if(ptrQueue->nLocGet==ptrQueue->nSizeQueue)
					{
						ptrQueue->nLocGet=0;
					}
					if(ptrQueue->nFullThread) 
					{  
						ptrQueue->condPut.notify_one();
					}
					return pData;
				}
				else
				{
					m_nEmptyThread++;
					if(m_condGet.timed_wait<boost::posix_time::seconds>(lock,boost::posix_time::seconds(nSeconds))==false) 
					{
						m_nEmptyThread--;   
						return NULL;
					}
					m_nEmptyThread--;     
				}
			}
            return NULL;
		}
		bool CMsgQueuePrio::IsEmpty()
		{
			return m_aQueues[MSG_HIGH].nData==0
				&&m_aQueues[MSG_MEDIUM].nData==0
				&&m_aQueues[MSG_LOW].nData==0;
		}
		bool CMsgQueuePrio::IsFull(EMsgPriority enPrio)
		{
			boost::unique_lock<boost::mutex> lock(m_mut);
			return m_aQueues[enPrio].nLocPut==m_aQueues[enPrio].nLocGet
				&&m_aQueues[enPrio].nData!=0;
		}
		CMsgQueuePrio::~CMsgQueuePrio()
		{
			delete[] m_aQueues[MSG_HIGH].ppBuffer;
			delete[] m_aQueues[MSG_MEDIUM].ppBuffer;
			delete[] m_aQueues[MSG_LOW].ppBuffer;
		}
	}
}

