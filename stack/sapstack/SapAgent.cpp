#include "SapAgent.h"
#include "SapStack.h"
#include <iostream>
#include "SapConnManager.h"
#include <boost/bind.hpp>
#include "SapLogHelper.h"
namespace sdo{
namespace sap{
ISapAgent * GetSapAgentInstance()
{
    return CSapAgent::Instance();
}
ISapAgent * GetServerAgentInstance()
{
    return CSapAgent::Instance2();
}

CSapAgent::CSapAgent():m_pCallback(NULL),
    m_oIoService(CSapStack::Instance()->GetIoService()),
    m_oAcceptor(m_oIoService),bRunning(false)
{
}
CSapAgent * CSapAgent::sm_pInstance=NULL;
CSapAgent * CSapAgent::sm_pInstance2=NULL;

CSapAgent * CSapAgent::Instance()
{
    if(sm_pInstance==NULL)
    {
        sm_pInstance=new CSapAgent;
    }
    return sm_pInstance;
}
CSapAgent * CSapAgent::Instance2()
{
    if(sm_pInstance2==NULL)
    {
        sm_pInstance2=new CSapAgent;
    }
    return sm_pInstance2;
}

void CSapAgent::SetInterval(unsigned int dwRequestTimeout,int nHeartIntervl,int nHeartDisconnectInterval)
{
    CSapMsgTimerList::SetRequestTimeout(dwRequestTimeout);
    CSapConnection::SetInterval(nHeartIntervl,nHeartDisconnectInterval);
}

void CSapAgent::OnConnectResult(int nId,int nResult)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d],result[%d]\n",__FUNCTION__,nId,nResult);
    if(m_pCallback!=NULL) 
        m_pCallback->OnConnectResult(nId,nResult);
    if(nResult!=0)
    {
        CSapConnManager::Instance()->DelConnection(nId);
    }
}
void CSapAgent::OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d],remote[%s:%d]\n",__FUNCTION__,nId,strRemoteIp.c_str(),dwRemotPort);
    if(m_pCallback!=NULL) 
        m_pCallback->OnReceiveConnection(nId,strRemoteIp,dwRemotPort);
}
void CSapAgent::OnReceiveMessage(int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotPort)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d],buffer[%x],len[%d],addr[%s:%d]\n",__FUNCTION__,nId,pBuffer,nLen,strRemoteIp.c_str(),dwRemotPort);
    if(m_pCallback!=NULL) 
        m_pCallback->OnReceiveMessage(nId,pBuffer,nLen,strRemoteIp,dwRemotPort);
}
void CSapAgent::OnPeerClose(int nId)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d]\n",__FUNCTION__,nId);
    if(m_pCallback!=NULL) 
        m_pCallback->OnPeerClose(nId);
    CSapConnManager::Instance()->DelConnection(nId);
}
int CSapAgent::StartServer(unsigned int dwPort)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,port[%d]...\n",__FUNCTION__,dwPort);
    boost::system::error_code ec;
    m_oAcceptor.open(boost::asio::ip::tcp::v4(),ec);
    if(ec)
    {
        SS_SLOG(XLOG_ERROR,"SapAgent::"<<__FUNCTION__<<",open socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -1;
    }
    m_oAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
    m_oAcceptor.set_option(boost::asio::ip::tcp::no_delay(true),ec);
    m_oAcceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),dwPort),ec);
    if(ec)
    {
        SS_SLOG(XLOG_ERROR,"SapAgent::"<<__FUNCTION__<<",bind socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -2;
    }
    m_oAcceptor.listen(boost::asio::socket_base::max_connections, ec);
    if(ec)
    {
        SS_SLOG(XLOG_ERROR,"SapAgent::"<<__FUNCTION__<<",listen socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -3;
    }
    bRunning=true;
    StartAccept();
    return 0;
}
void CSapAgent::StopServer()
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSapAgent::HandleStop, this));
}
void CSapAgent::HandleStop()
{
    SS_XLOG(XLOG_TRACE,"CSapAgent::%s\n",__FUNCTION__);
    boost::system::error_code ec;
    m_oAcceptor.close(ec);
    bRunning=false;
}

int CSapAgent::DoConnect(const string &strIp, unsigned int dwPort,int nTtimeout,unsigned int dwLocalPort)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,addr[%s:%d],timeout[%d],localPort[%d]\n",__FUNCTION__,strIp.c_str(),dwPort,nTtimeout,dwLocalPort);
    SapConnection_ptr new_connection(new CSapConnection(sm_pInstance,CSapStack::Instance()->GetIoService()));
    CSapConnManager::Instance()->AddConnection(new_connection);
    new_connection->AsyncConnect(strIp,dwPort,nTtimeout,dwLocalPort);
    return new_connection->Id();
}
int CSapAgent::DoSendMessage(int nId,const void *pBuffer, int nLen,unsigned int dwTimeout)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d],buffer[%x],len[%d],timeout[%d]\n",__FUNCTION__,nId,pBuffer,nLen,dwTimeout);

    SapConnection_ptr new_connection=CSapConnManager::Instance()->GetConnection(nId);
    if(new_connection.get()!=NULL)
    {
        new_connection->AsyncWrite(pBuffer,nLen,dwTimeout);
    	return 0;
    }
    return -1;
}
void CSapAgent::DoClose(int nId)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d]\n",__FUNCTION__,nId);

    SapConnection_ptr new_connection=CSapConnManager::Instance()->GetConnection(nId);
    if(new_connection.get()!=NULL)
    {
        new_connection->Close();
        CSapConnManager::Instance()->DelConnection(nId);
    }

}
void CSapAgent::SetLocalDigest(int nId,const string &strDigest)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d]\n",__FUNCTION__,nId);
    
    SapConnection_ptr new_connection=CSapConnManager::Instance()->GetConnection(nId);
    if(new_connection.get()!=NULL)
    {
        new_connection->SetLocalDigest(strDigest);
    }
}
void CSapAgent::SetPeerDigest(int nId,const string &strDigest)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d]\n",__FUNCTION__,nId);
        
    SapConnection_ptr new_connection=CSapConnManager::Instance()->GetConnection(nId);
    if(new_connection.get()!=NULL)
    {
        new_connection->SetPeerDigest(strDigest);
    }
}
void CSapAgent::SetEncKey(int nId,unsigned char arKey[16])
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s,id[%d]\n",__FUNCTION__,nId);
        
    SapConnection_ptr new_connection=CSapConnManager::Instance()->GetConnection(nId);
    if(new_connection.get()!=NULL)
    {
        new_connection->SetEncKey(arKey);
    }

}

void CSapAgent::StartAccept()
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s...\n",__FUNCTION__);
    if(sm_pInstance2==NULL)
    {
        SapConnection_ptr new_connection(new CSapConnection(sm_pInstance,CSapStack::Instance()->GetIoService()));
        m_oAcceptor.async_accept(new_connection->Socket(),
        MakeSapAllocHandler(m_allocator,boost::bind(&CSapAgent::HandleAccept, this, new_connection,
          boost::asio::placeholders::error)));
    }
    else
    {
        SapConnection_ptr new_connection(new CSapConnection(sm_pInstance2,CSapStack::Instance()->GetIoService()));
        m_oAcceptor.async_accept(new_connection->Socket(),
        MakeSapAllocHandler(m_allocator,boost::bind(&CSapAgent::HandleAccept, this, new_connection,
          boost::asio::placeholders::error)));
    }
}
void CSapAgent::HandleAccept(SapConnection_ptr new_connection,const boost::system::error_code &err)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s\n",__FUNCTION__);
    if (!err)
    {
        new_connection->SetRemoteAddr();
        CSapConnManager::Instance()->AddConnection(new_connection);
        OnReceiveConnection(new_connection->Id(),new_connection->GetRemoteIp() ,new_connection->GetRemotePort());
        new_connection->AsyncRead();

        StartAccept();
    }
    else if(bRunning)
    {
        SS_SLOG(XLOG_WARNING,"CSapAgent::HandleAccept, error:"<< err.message() << "\n");
        m_oAcceptor.async_accept(new_connection->Socket(),
            MakeSapAllocHandler(m_allocator,boost::bind(&CSapAgent::HandleAccept, this, new_connection,
                boost::asio::placeholders::error)));
    }
    else
    {
        SS_SLOG(XLOG_WARNING,"CSapAgent::HandleAccept, stop. error:"<< err.message() << "\n");
    }
}


}
}

