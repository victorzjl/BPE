#include "SapStack.h"
#include <boost/bind.hpp>
#include "SapLogHelper.h"
#include <boost/date_time.hpp>
#include "SapConnManager.h"

namespace sdo{
namespace sap{
CSapStack * CSapStack::sm_instance=NULL;
ISapStack * GetSapStackInstance()
{
    return CSapStack::Instance();
}
CSapStack * CSapStack::Instance()
{
    if(sm_instance==NULL)
        sm_instance=new CSapStack;
    return sm_instance;
}
CSapStack::CSapStack():ppThreads(NULL),m_nThread(1),m_dwIndex(0),m_bRunning(false)
{
}

void CSapStack::Start(int nThread)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::Start[%d],running[%d]...\n",nThread,m_bRunning);
    boost::lock_guard<boost::mutex> guard(m_mut);
    if(m_bRunning==false)
    {
        m_bRunning=true;
        m_nThread=nThread;
        ppThreads=new CSapStackThread *[m_nThread];
        for(int i=0;i<m_nThread;i++)
        {
            CSapStackThread * pThread=new  CSapStackThread;
            ppThreads[i]=pThread;
            pThread->Start();
        }
        CSapConnManager::Instance()->Start();
    }
}
void CSapStack::Stop()
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::Stop\n");
    boost::lock_guard<boost::mutex> guard(m_mut);
    if(m_bRunning)
    {
        m_bRunning=false;
        CSapConnManager::Instance()->Stop();
        for(int i=0;i<m_nThread;i++)
        {
            CSapStackThread * pThread=ppThreads[i];
            pThread->Stop();
            delete pThread;
        }
    }
}
boost::asio::io_service & CSapStack::GetIoService()
{
    int i=(m_dwIndex++)%m_nThread;
    CSapStackThread * pThread=ppThreads[i];
    return pThread->GetIoService();
}

}
}

