#include "MemoryPool.h"
#include <cstdio>
#include <boost/pool/singleton_pool.hpp>

#define MEMPOOL_SETVALUE_SLOT(index) \
	m_apMallocs[index]=&(boost::singleton_pool<SNormalPool,MEMORY_POOL_ALIGNSIZE*(index+1) >::malloc);\
    m_apFrees[index]=&(boost::singleton_pool<SNormalPool,MEMORY_POOL_ALIGNSIZE*(index+1) >::free); \
    m_apPurges[index]=&(boost::singleton_pool<SNormalPool,MEMORY_POOL_ALIGNSIZE*(index+1) >::purge_memory); \
    m_apRelease[index]=&(boost::singleton_pool<SNormalPool,MEMORY_POOL_ALIGNSIZE*(index+1) >::release_memory);


#define MMS_4(start) \
    MEMPOOL_SETVALUE_SLOT(4*start) \
    MEMPOOL_SETVALUE_SLOT(4*start+1) \
    MEMPOOL_SETVALUE_SLOT(4*start+2) \
    MEMPOOL_SETVALUE_SLOT(4*start+3) 
namespace sdo{
	namespace common{
		CMemoryPool* CMemoryPool::sm_pInstance=NULL;
		CMemoryPool* CMemoryPool::Instance()
		{
			if(sm_pInstance==NULL)
            {         
				sm_pInstance=new CMemoryPool();
            }
			return sm_pInstance;
		}
		void * CMemoryPool::Malloc(std::size_t dwSize)
		{
			std::size_t  index=(dwSize-1)>>4;    //attention: 4 is magic number
			if(index>MEMORY_POOL_MAXPOOL-1) return ::malloc(dwSize);
            void *ptr=m_apMallocs[index]();
			if(ptr==NULL)
            {
                int i=0;
                for(i=0;i<MEMORY_POOL_MAXPOOL;i++)
                {
                   m_apRelease[i](); 
                }
                ptr=m_apMallocs[index]();
            }
            return ptr;
		}
		void CMemoryPool::Free(void *pBuffer,std::size_t  dwSize)
		{
			std::size_t index=(dwSize-1)>>4;   //attention: 4 is magic number 
			if(index>MEMORY_POOL_MAXPOOL-1) return ::free(pBuffer);
			return m_apFrees[index](pBuffer);
		}
		CMemoryPool::CMemoryPool()
		{
			MMS_4(0);MMS_4(1);MMS_4(2);MMS_4(3);
			MMS_4(4);MMS_4(5);MMS_4(6);MMS_4(7);
			MMS_4(8);MMS_4(9);MMS_4(10);MMS_4(11);
			MMS_4(12);MMS_4(13);MMS_4(14);MMS_4(15);
			MMS_4(16);MMS_4(17);MMS_4(18);MMS_4(19);
			MMS_4(20);MMS_4(21);MMS_4(22);MMS_4(23);
			MMS_4(24);MMS_4(25);MMS_4(26);MMS_4(27);
			MMS_4(28);MMS_4(29);MMS_4(30);MMS_4(31);
            MMS_4(32);MMS_4(33);MMS_4(34);MMS_4(35);
            MMS_4(36);MMS_4(37);MMS_4(38);MMS_4(39);
            MMS_4(40);MMS_4(41);MMS_4(42);MMS_4(43);
            MMS_4(44);MMS_4(45);MMS_4(46);MMS_4(47);
            MMS_4(48);MMS_4(49);MMS_4(50);MMS_4(51);
            MMS_4(52);MMS_4(53);MMS_4(54);MMS_4(55);
            MMS_4(56);MMS_4(57);MMS_4(58);MMS_4(59);
            MMS_4(60);MMS_4(61);MMS_4(62);MMS_4(63);//attention: 63 is magic number
		}
        CMemoryPool::~CMemoryPool()
        {
        /*
            int i=0;
            for(i=0;i<MEMORY_POOL_MAXPOOL;i++)
            {
                m_apPurges[i](); 
            }
            */
        }
	}
}

