#include <boost/test/unit_test.hpp>
#include "MemoryPool.h"
#include <string>
#include <cstdio>
using std::string;
using namespace sdo::common;


BOOST_AUTO_TEST_SUITE(test_memorypool)

BOOST_AUTO_TEST_CASE(test_malloc_free)
{
    void *p0=CMemoryPool::Instance()->Malloc(32);
    memcpy(p0,"1234",32);
    CMemoryPool::Instance()->Free(p0,32);

    
    char *p1=(char *)CMemoryPool::Instance()->Malloc(17);
    char *p2=(char *)CMemoryPool::Instance()->Malloc(25);
    char *p3=(char *)CMemoryPool::Instance()->Malloc(32);

    BOOST_CHECK_EQUAL(p2-p1,32);
    BOOST_CHECK_EQUAL(p3-p2,32);

    CMemoryPool::Instance()->Free(p2,32);
    char *p4=(char *)CMemoryPool::Instance()->Malloc(18);

    BOOST_CHECK_EQUAL(p4-p1,32);
    BOOST_CHECK_EQUAL(p3-p4,32);

    CMemoryPool::Instance()->Free(p1,32);
    CMemoryPool::Instance()->Free(p3,32);
    CMemoryPool::Instance()->Free(p4,32);

    char *pMax1=(char *)CMemoryPool::Instance()->Malloc(4096);
    char *pMax2=(char *)CMemoryPool::Instance()->Malloc(4096);
    BOOST_CHECK_EQUAL(pMax2-pMax1,4096);
    CMemoryPool::Instance()->Free(pMax1,4096);
    CMemoryPool::Instance()->Free(pMax2,4096);

}
BOOST_AUTO_TEST_CASE(test_malloc_big)
{
    char *p1=(char *)CMemoryPool::Instance()->Malloc(8090);
    char *p2=(char *)CMemoryPool::Instance()->Malloc(8091);

    BOOST_CHECK_NE(p2-p1,8092);
    CMemoryPool::Instance()->Free(p1,8090);
    CMemoryPool::Instance()->Free(p2,8091);
}

BOOST_AUTO_TEST_SUITE_END()

