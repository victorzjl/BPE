#ifndef _SDO_COMMON_SMALLOBJECT_H_
#define _SDO_COMMON_SMALLOBJECT_H_
#include "MemoryPool.h"
#include <new>
namespace sdo{
	namespace common{
		class CSmallObjAllocator
		{
			public:
#ifdef _MSC_VER
				static void * operator new ( std::size_t dwSize )
#else
					static void * operator new ( std::size_t dwSize ) throw ( std::bad_alloc )
#endif
					{
						return CMemoryPool::Instance()->Malloc(dwSize);
					}
				static void operator delete ( void * p, std::size_t dwSize ) throw ()
				{
					return CMemoryPool::Instance()->Free(p,dwSize);
				}

#ifdef _MSC_VER
				static void * operator new [] ( std::size_t dwSize )
#else
					static void * operator new [] ( std::size_t dwSize )throw ( std::bad_alloc )
#endif
					{
						return CMemoryPool::Instance()->Malloc(dwSize);
					}
				static void operator delete [] ( void * p, std::size_t dwSize ) throw ()
				{
					return CMemoryPool::Instance()->Free(p,dwSize);
				}

			protected:
				inline CSmallObjAllocator( void ) {}
				inline CSmallObjAllocator( const CSmallObjAllocator & ) {}
				inline CSmallObjAllocator & operator = ( const CSmallObjAllocator & ){ return *this; }
				inline ~CSmallObjAllocator() {}
		};

		class CSmallObject : public CSmallObjAllocator
		{
			public:
				virtual ~CSmallObject() {}
			protected:
				inline CSmallObject( void ) {}
			private:
				CSmallObject( const CSmallObject & );
				CSmallObject & operator = ( const CSmallObject & );
		}; 
		class CSmallValueObject :public CSmallObjAllocator

		{
			protected:
				inline CSmallValueObject( void ) {}
				inline CSmallValueObject( const CSmallValueObject & ) {}
				inline CSmallValueObject & operator = ( const CSmallValueObject & ){ return *this; }
				inline ~CSmallValueObject() {}
		};
	}
}
#endif

