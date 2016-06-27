#include "CohHttpsStack.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

boost::asio::io_service CCohHttpsStack::sm_oIoService;
boost::asio::io_service::work *CCohHttpsStack::sm_oWork=NULL;
boost::thread CCohHttpsStack::m_thread;

void CCohHttpsStack::Start()
{
    sm_oWork=new boost::asio::io_service::work(sm_oIoService);
    m_thread=boost::thread(boost::bind(&boost::asio::io_service::run,&sm_oIoService));
}
void CCohHttpsStack::Stop()
{
    delete sm_oWork;
    m_thread.join();
    sm_oIoService.reset();
}

