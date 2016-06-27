#include <boost/test/unit_test.hpp>
#include "ThreadTimer.h"
#include "MsgThread.h"
#include <boost/thread.hpp>

using namespace sdo::common;
/*
class CThread:public CMsgThread
{
public:
    CThread():Timer(0,1,&CThread::TimeoutCb),n(0)
        {}
    static void TimeoutCb(void *pData)
        {
            ((CThread *)pData)->n++;
        }
    void StartTimer ()
        {
        Timer.Start();
        }
    void StopTimer()
        {
        Timer.Stop();
        }
    virtual void Deal(void *pData)
        {
            if((int)pData==1)
            {
            StartTimer();
            
            BOOST_CHECK_EQUAL(TimerState(),CThreadTimer::TIMER_ALIVE);
            }
        }
    int GetN()const{return n;}
    CThreadTimer::ETimerState TimerState(){return Timer.State();}
private:
    CThreadTimer Timer;
    int n;
};

BOOST_AUTO_TEST_SUITE(test_thread_timer )
BOOST_AUTO_TEST_CASE(test_thread_timer)
{
    CThread thread1,thread2;
    thread1.Start();
    thread2.Start();
    BOOST_CHECK_EQUAL(thread1.GetN(),0);
    BOOST_CHECK_EQUAL(thread2.GetN(),0);
    BOOST_CHECK_EQUAL(thread1.TimerState(),CThreadTimer::TIMER_IDLE);
    BOOST_CHECK_EQUAL(thread2.TimerState(),CThreadTimer::TIMER_IDLE);

    thread1.PutQ((void *)1);
    thread2.PutQ((void *)1);

    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,1,100000));
    BOOST_CHECK_EQUAL(thread1.TimerState(),CThreadTimer::TIMER_IDLE);
    BOOST_CHECK_EQUAL(thread2.TimerState(),CThreadTimer::TIMER_IDLE);
    BOOST_CHECK_EQUAL(thread1.GetN(),1);
    BOOST_CHECK_EQUAL(thread2.GetN(),1);
    
    thread1.Stop();
    thread2.Stop();
}
BOOST_AUTO_TEST_SUITE_END()
*/

