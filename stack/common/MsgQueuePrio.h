#ifndef _SDO_COMMON_MSG_QUEUE_PRIO__H_
#define _SDO_COMMON_MSG_QUEUE_PRIO__H_
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#define DEFAULT_MEDIUMQUEUESIZE 1024*64
namespace sdo{
	namespace common{
		enum EMsgPriority
		{
			MSG_HIGH=0,
			MSG_MEDIUM=1,
			MSG_LOW=2,
			MSG_PRIO_MAX
		};
		class CMsgQueuePrio
		{
			public:
				typedef struct tagSingleQueue
				{
					boost::condition_variable condPut;
					void * * ppBuffer;   
					int nSizeQueue;        
					int nLocPut;     
					int nLocGet;      
					int nFullThread;    
					int nData;
				}SSingleQueue;
			public:
				CMsgQueuePrio(int nHighQueueSize=512,int nMediumQueueSize=DEFAULT_MEDIUMQUEUESIZE,int nLowQueueSize=512);

				void PutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM);
				void * GetQ();

				bool TimedPutQ(void *pData,EMsgPriority enPrio=MSG_MEDIUM,int nSeconds=3);
				void * TimedGetQ(int nSeconds=3);

				bool IsEmpty();
				bool IsFull(EMsgPriority enPrio=MSG_MEDIUM);
				virtual ~CMsgQueuePrio();
			private:
				boost::mutex m_mut;
				boost::condition_variable  m_condGet;
				int m_nEmptyThread;   
				SSingleQueue  m_aQueues[MSG_PRIO_MAX];
		};
	}
}
#endif

