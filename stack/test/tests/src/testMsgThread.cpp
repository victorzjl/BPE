#include <boost/test/unit_test.hpp>
#include "MsgThread.h"
#include <boost/thread.hpp>
using namespace sdo::common;

class MyThread:public CMsgThread
{
public:
    MyThread():m_nInternal(0){}
    virtual void Deal(void *pData)
    {
        m_nInternal=(int)pData;
    }
    int GetInerval()const{return m_nInternal;}
private:
    int m_nInternal;
};
struct MsgThreadFixture
{
    MsgThreadFixture(){m_pThread.Start();}
    ~MsgThreadFixture(){m_pThread.Stop();}
    MyThread m_pThread;
};
BOOST_FIXTURE_TEST_SUITE(test_data, MsgThreadFixture )
BOOST_AUTO_TEST_CASE(test_thread)
{
    m_pThread.PutQ((void *)100);
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(m_pThread.GetInerval(),100);

    m_pThread.TimedPutQ((void *)200);
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(m_pThread.GetInerval(),200);
}
BOOST_AUTO_TEST_SUITE_END()

