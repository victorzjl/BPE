#include "SapServer.h"
#include <iostream>
#include <boost/bind.hpp>
#include "SessionManager.h"
#include "SapLogHelper.h"
#include "SapStack.h"
CSapServer::CSapServer():
    m_oIoService(CSapStack::Instance()->GetThread()->GetIoService()),
    m_oAcceptor(m_oIoService),bRunning(false)
{
}
CSapServer * CSapServer::sm_pInstance=NULL;

CSapServer * CSapServer::Instance()
{
    if(sm_pInstance==NULL)
    {
        sm_pInstance=new CSapServer;
    }
    return sm_pInstance;
}
int CSapServer::StartServer(unsigned int dwPort)
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
void CSapServer::StopServer()
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSapServer::HandleStop, this));
}
void CSapServer::HandleStop()
{
    SS_XLOG(XLOG_TRACE,"CSapAgent::%s\n",__FUNCTION__);
    boost::system::error_code ec;
    m_oAcceptor.close(ec);
    bRunning=false;
}


void CSapServer::StartAccept()
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s...\n",__FUNCTION__);
    CSessionManager * pManager=CSapStack::Instance()->GetThread();
    SapConnection_ptr new_connection(new CSapConnection(pManager->GetIoService(),pManager));
    m_oAcceptor.async_accept(new_connection->Socket(),
        MakeSapAllocHandler(m_allocator,boost::bind(&CSapServer::HandleAccept, this, new_connection,
          boost::asio::placeholders::error)));
}
void CSapServer::HandleAccept(SapConnection_ptr new_connection,const boost::system::error_code &err)
{
    SS_XLOG(XLOG_DEBUG,"CSapAgent::%s\n",__FUNCTION__);
    if (!err)
    {
	    new_connection->SetRemoteAddr();
        ((CSessionManager *)(new_connection->GetManager()))->OnReceiveSoc(new_connection);
	    new_connection->AsyncRead();
        StartAccept();
    }
    else if(bRunning)
    {
        SS_SLOG(XLOG_WARNING,"CSapAgent::HandleAccept, error:"<< err.message() << "\n");
        m_oAcceptor.async_accept(new_connection->Socket(),
            MakeSapAllocHandler(m_allocator,boost::bind(&CSapServer::HandleAccept, this, new_connection,
                boost::asio::placeholders::error)));
    }
    else
    {
        SS_SLOG(XLOG_WARNING,"CSapAgent::HandleAccept, stop. error:"<< err.message() << "\n");
    }
}



