#include "HpsServer.h"
#include <iostream>
#include <boost/bind.hpp>
#include "HpsLogHelper.h"
#include "HpsStack.h"

CHpsServer::CHpsServer(boost::asio::io_service &oIoService):
    m_oIoService(oIoService),
    m_oAcceptor(oIoService),bRunning(false)
{
}

int CHpsServer::StartServer(unsigned int dwPort)
{
    HP_XLOG(XLOG_DEBUG,"CHpsServer::%s,port[%d]...\n",__FUNCTION__,dwPort);
    boost::system::error_code ec;
    m_oAcceptor.open(boost::asio::ip::tcp::v4(),ec);
    if(ec)
    {
        HP_SLOG(XLOG_ERROR,"CHpsServer::"<<__FUNCTION__<<",open socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -1;
    }
    m_oAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
	m_oAcceptor.set_option(boost::asio::ip::tcp::no_delay(true),ec);
    m_oAcceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),dwPort),ec);
    if(ec)
    {
        HP_SLOG(XLOG_ERROR,"CHpsServer::"<<__FUNCTION__<<",bind socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -2;
    }
    m_oAcceptor.listen(boost::asio::socket_base::max_connections, ec);
    if(ec)
    {
        HP_SLOG(XLOG_ERROR,"CHpsServer::"<<__FUNCTION__<<",listen socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -3;
    }
    bRunning=true;

    StartAccept();
    return 0;
}
void CHpsServer::StopServer()
{
    HP_XLOG(XLOG_DEBUG,"CHpsServer::%s\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CHpsServer::HandleStop, this));
}
void CHpsServer::HandleStop()
{
    HP_XLOG(XLOG_DEBUG,"CHpsServer::%s\n",__FUNCTION__);
    boost::system::error_code ec;
    m_oAcceptor.close(ec);
    bRunning=false;
}


void CHpsServer::StartAccept()
{
    HP_XLOG(XLOG_DEBUG,"CHpsServer::%s...\n",__FUNCTION__);
    CHpsSessionManager * pManager=CHpsStack::Instance()->GetThread();
    HpsConnection_ptr new_connection(new CHpsConnection(pManager->GetIoService()));
	pManager->AddConnection(new_connection);
	
	m_oAcceptor.async_accept(new_connection->Socket(),
        MakeHpsAllocHandler(m_allocator,boost::bind(&CHpsServer::HandleAccept, this, pManager, new_connection,
          boost::asio::placeholders::error)));
}

void CHpsServer::HandleAccept(CHpsSessionManager * pManager,HpsConnection_ptr new_connection,const boost::system::error_code &err)
{
    HP_XLOG(XLOG_DEBUG,"CHpsServer::%s\n",__FUNCTION__);
    if (!err)
    {
    	new_connection->AsyncRead();
        pManager->OnHttpConnected(new_connection->Id(),new_connection->GetRemoteIp(), new_connection->GetRemotePort());
	    
        StartAccept();
    }
    else if(bRunning)
    {
        HP_SLOG(XLOG_WARNING,"CHpsServer::HandleAccept, error:"<< err.message() << "\n");
        m_oAcceptor.async_accept(new_connection->Socket(),
            MakeHpsAllocHandler(m_allocator,boost::bind(&CHpsServer::HandleAccept, this, pManager, new_connection,
                boost::asio::placeholders::error)));
    }
    else
    {
        HP_SLOG(XLOG_WARNING,"CHpsServer::HandleAccept, stop. error:"<< err.message() << "\n");
    }
}



