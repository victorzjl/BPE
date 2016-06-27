#include "SapStackThread.h"
#include "SapLogHelper.h"
#include "SapStack.h"
#include "SapAgent.h"

namespace sdo{
namespace sap{
CSapStackThread::CSapStackThread()
{
}
void CSapStackThread::Start()
{
    SS_XLOG(XLOG_DEBUG,"CSapStackThread::Start...\n");
    m_oWork=new boost::asio::io_service::work(m_oIoService);
    m_thread=boost::thread(boost::bind(&boost::asio::io_service::run,&m_oIoService)); 

}
void CSapStackThread::Stop()
{
    delete m_oWork;
    m_thread.join();
    m_oIoService.reset();
}


CSapStackThread::~CSapStackThread()
{
}

}
}

