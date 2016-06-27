#include "CohStack.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "CohLogHelper.h"

namespace sdo{
namespace coh{
boost::asio::io_service CCohStack::sm_oIoService;
boost::asio::io_service::work *CCohStack::sm_oWork=NULL;
boost::thread CCohStack::m_thread;

void CCohStack::Start()
{
    CS_XLOG(XLOG_DEBUG,"CCohStack::Start...\n");
    sm_oWork=new boost::asio::io_service::work(sm_oIoService);
    m_thread=boost::thread(boost::bind(&boost::asio::io_service::run,&sm_oIoService));
}
void CCohStack::Stop()
{
    CS_XLOG(XLOG_DEBUG,"CCohStack::Stop\n");
    delete sm_oWork;
    //sm_oIoService.stop();
    m_thread.join();
    sm_oIoService.reset();
}
}
}
