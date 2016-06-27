#include "SapConnManager.h"
#include "SapLogHelper.h"
#include "SapStack.h"
#include "SapAgent.h"

namespace sdo{
namespace sap{
CSapConnManager * CSapConnManager::sm_pInstance=NULL;
int CSapConnManager::sm_nId=0;

CSapConnManager * CSapConnManager::Instance()
{
    if(sm_pInstance==NULL)
    {
        sm_pInstance=new CSapConnManager;
    }
    return sm_pInstance;
}
CSapConnManager::CSapConnManager():m_tmTimeoOut(CSapStack::Instance()->GetIoService()),
    bRunning(false)
{
}
void CSapConnManager::Start()
{
    bRunning=true;
    m_tmTimeoOut.expires_from_now(boost::posix_time::seconds(2));
	m_tmTimeoOut.async_wait(
        MakeSapAllocHandler(m_allocTimeOut,boost::bind(&CSapConnManager::HandleTimeoutInterval, this)));
}
void CSapConnManager::Stop()
{
    bRunning=false;
    {
        SapConnection_ptr ptrConn;
        boost::lock_guard<boost::mutex> guard(m_mut);
        map<int,SapConnection_ptr>::iterator itr;
        for(itr=m_mapConns.begin();itr!=m_mapConns.end();itr++)
        {
            ptrConn=itr->second;
            ptrConn->Close();
        }
        while(!m_mapConns.empty())
        {
            m_mapConns.erase(m_mapConns.begin());
        }
    }

}

void CSapConnManager::HandleTimeoutInterval()
{
    {
        boost::lock_guard<boost::mutex> guard(m_mut);
        map<int,SapConnection_ptr>::iterator itr;
        SapConnection_ptr ptrConn;
        for(itr=m_mapConns.begin();itr!=m_mapConns.end();itr++)
        {
            ptrConn=itr->second;
            ptrConn->DetectInterval();
        }

    }
    if(bRunning)
    {
        m_tmTimeoOut.expires_from_now(boost::posix_time::seconds(2));
        m_tmTimeoOut.async_wait(
                MakeSapAllocHandler(m_allocTimeOut,boost::bind(&CSapConnManager::HandleTimeoutInterval, this)));
    }
}

void CSapConnManager::AddConnection(SapConnection_ptr ptrConn)
{
    //SS_XLOG(XLOG_DEBUG,"CSapConnManager::AddConnection,id[%d]\n",ptrConn->Id());
    boost::lock_guard<boost::mutex> guard(m_mut);
    //m_mapConns[ptrConn->Id()]=ptrConn;
    int nId=0;
    do
    {   
        nId=sm_nId++;
    }while(m_mapConns.find(nId)!=m_mapConns.end());
    ptrConn->SetId(nId);
    m_mapConns[nId]=ptrConn;
    SS_XLOG(XLOG_DEBUG,"CSapConnManager::AddConnection,id[%d]\n",ptrConn->Id());
    return;
}
SapConnection_ptr CSapConnManager::GetConnection(int nId)
{
    SapConnection_ptr ptrConn;
    boost::lock_guard<boost::mutex> guard(m_mut);
    map<int,SapConnection_ptr>::iterator itr=m_mapConns.find(nId);
    if(itr!=m_mapConns.end())
    {
        ptrConn=itr->second;
    }
    return ptrConn;
}
void CSapConnManager::DelConnection(SapConnection_ptr ptrConn)
{   
    SS_XLOG(XLOG_DEBUG,"CSapConnManager::DelConnection,id[%d]\n",ptrConn->Id());
    boost::lock_guard<boost::mutex> guard(m_mut);
    m_mapConns.erase(ptrConn->Id());

}
void CSapConnManager::DelConnection(int nId)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnManager::DelConnection,id[%d]\n",nId);
    boost::lock_guard<boost::mutex> guard(m_mut);
    m_mapConns.erase(nId);
}
CSapConnManager::~CSapConnManager()
{
    while(!m_mapConns.empty())
    {
        m_mapConns.erase(m_mapConns.begin());
    }
}

}
}

