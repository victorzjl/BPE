#ifndef _SDO_COMMON_MEMORY_POOL_H_
#define _SDO_COMMON_MEMORY_POOL_H_
#include <cstdio>

namespace sdo{
	namespace common{
		const int MEMORY_POOL_ALIGNSIZE=16;
		const int MEMORY_POOL_MAXPOOL=256;// 4096/MEMORY_POOL_ALIGNSIZE;
		class CMemoryPool
		{
			public:
				typedef struct tagNormalPool{}SNormalPool;
				static CMemoryPool* Instance();
				void * Malloc(std::size_t  dwSize);
				void Free(void *pBuffer,std::size_t  dwSize);
			private:
				CMemoryPool();
				~CMemoryPool();
				static CMemoryPool* sm_pInstance;
				void * (*m_apMallocs[MEMORY_POOL_MAXPOOL])() ;
				void (*m_apFrees[MEMORY_POOL_MAXPOOL])(void * const) ;
				bool (*m_apPurges[MEMORY_POOL_MAXPOOL])() ;
				bool (*m_apRelease[MEMORY_POOL_MAXPOOL])() ;
		};
	}
}
#endif

