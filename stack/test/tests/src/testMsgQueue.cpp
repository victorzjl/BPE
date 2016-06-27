#include <boost/test/unit_test.hpp>
#include "MsgQueuePrio.h"

using namespace sdo::common;
struct QueueFixture
{
    QueueFixture(){m_pQueue=new CMsgQueuePrio(2,2,2);}
    ~QueueFixture(){delete m_pQueue;}
    CMsgQueuePrio *m_pQueue;
};


BOOST_FIXTURE_TEST_SUITE(test_msgqueue,QueueFixture)

BOOST_AUTO_TEST_CASE(test_putq_getq)
{
    BOOST_CHECK(m_pQueue->IsEmpty());
    
    m_pQueue->PutQ((void *)1);
    m_pQueue->PutQ((void *)2);
    m_pQueue->PutQ((void *)3,MSG_LOW);
    m_pQueue->PutQ((void *)4,MSG_LOW);
    m_pQueue->PutQ((void *)5,MSG_HIGH);
    m_pQueue->PutQ((void *)6,MSG_HIGH);

    BOOST_CHECK(m_pQueue->IsFull(MSG_LOW));
    BOOST_CHECK(m_pQueue->IsFull());
    BOOST_CHECK(m_pQueue->IsFull(MSG_HIGH));

    int data=0;
    data=(int)(m_pQueue->GetQ());
    BOOST_CHECK_EQUAL(data,5);
    data=(int)(m_pQueue->GetQ());
    BOOST_CHECK_EQUAL(data,6);
    data=(int)(m_pQueue->GetQ());
    BOOST_CHECK_EQUAL(data,1);
    data=(int)(m_pQueue->GetQ());
    BOOST_CHECK_EQUAL(data,2);
    data=(int)(m_pQueue->GetQ());
    BOOST_CHECK_EQUAL(data,3);
    data=(int)(m_pQueue->GetQ());
    BOOST_CHECK_EQUAL(data,4);
}
BOOST_AUTO_TEST_CASE(test_timed)
{
    m_pQueue->PutQ((void *)1);
    m_pQueue->PutQ((void *)2);
    m_pQueue->PutQ((void *)3,MSG_LOW);
    m_pQueue->PutQ((void *)4,MSG_LOW);
    m_pQueue->PutQ((void *)5,MSG_HIGH);
    m_pQueue->PutQ((void *)6,MSG_HIGH);

    BOOST_CHECK(m_pQueue->TimedPutQ((void *)1,MSG_HIGH,1)==false);

    BOOST_CHECK(m_pQueue->TimedGetQ(1)!=NULL);
    BOOST_CHECK(m_pQueue->TimedGetQ(1)!=NULL);
    BOOST_CHECK(m_pQueue->TimedGetQ(1)!=NULL);
    BOOST_CHECK(m_pQueue->TimedGetQ(1)!=NULL);
    BOOST_CHECK(m_pQueue->TimedGetQ(1)!=NULL);
    BOOST_CHECK(m_pQueue->TimedGetQ(1)!=NULL);

    BOOST_CHECK(m_pQueue->TimedGetQ(1)==NULL);
}
BOOST_AUTO_TEST_SUITE_END()

