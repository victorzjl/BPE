#include "CohServer.h"
#include "CohStack.h"
#include <boost/bind.hpp>
#include <iostream>
#include "CohLogHelper.h"

namespace sdo{
namespace coh{
ICohServer * ICohServer::sm_oServer=NULL;
ICohServer::ICohServer():
    m_oIoService(CCohStack::GetIoService()),
    m_oAcceptor(m_oIoService),bRunning(false)
{
}
int ICohServer::Start(unsigned int dwPort)
{
    CS_XLOG(XLOG_DEBUG,"ICohServer::Start,port[%d]...\n",dwPort);
    boost::system::error_code ec;
    m_oAcceptor.open(boost::asio::ip::tcp::v4(),ec);
    if(ec)
    {
        CS_SLOG(XLOG_ERROR,"ICohServer::Start,open socket error:" <<ec.message()<<"\n");
        m_oAcceptor.close(ec);
        return -1;
    }
    m_oAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
    m_oAcceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),dwPort),ec);
    if(ec)
    {
        CS_XLOG(XLOG_ERROR,"ICohServer::Start,bind port error!\n");
        m_oAcceptor.close(ec);
        return -2;
    }
    m_oAcceptor.listen(boost::asio::socket_base::max_connections, ec);
    if(ec)
    {
        CS_XLOG(XLOG_ERROR,"ICohServer::Start,listen port error!\n");
        m_oAcceptor.close(ec);
        return -3;
    }
    sm_oServer=this;
    bRunning=true;
    StartAccept();
    return 0;
    
}
void ICohServer::Stop()
{
    CS_XLOG(XLOG_DEBUG,"ICohServer::Stop\n");
	m_oIoService.post(boost::bind(&ICohServer::HandleStop, this));
}
void ICohServer::StartAccept()
{
    CS_XLOG(XLOG_DEBUG,"ICohServer::StartAccept...\n");
    CohConnection_ptr new_connection(new CCohConnection(m_oIoService));
    m_oAcceptor.async_accept(new_connection->Socket(),
        MakeCohAllocHandler(m_allocator,boost::bind(&ICohServer::HandleAccept, this, new_connection,
          boost::asio::placeholders::error)));
}
void ICohServer::HandleAccept(CohConnection_ptr new_connection,const boost::system::error_code &err)
{
    CS_XLOG(XLOG_DEBUG,"ICohServer::HandleAccept\n");
    if (!err)
      {
        new_connection->StartReadRequest();
        StartAccept();
      }
    else if(bRunning)
    {
        CS_SLOG(XLOG_WARNING,"ICohServer::HandleAccept, error:"<< err.message() << "\n");
      m_oAcceptor.async_accept(new_connection->Socket(),
        MakeCohAllocHandler(m_allocator,boost::bind(&ICohServer::HandleAccept, this, new_connection,
          boost::asio::placeholders::error)));
    }
    else
    {
        CS_SLOG(XLOG_WARNING,"ICohServer::HandleAccept, error:"<< err.message() << "\n");
    }
}
void ICohServer::DoSendResponse(void *pIdentifer,const string &strResponse)
{
    ConnectionAdapter *pAdapter=(ConnectionAdapter *)pIdentifer;
    CS_XLOG(XLOG_DEBUG,"ICohServer::DoSendResponse, identifer[%0x]\n",pIdentifer);
    pAdapter->connection_->DoSendResponse(strResponse);
    delete pAdapter;
}
void ICohServer::HandleStop()
{
    CS_XLOG(XLOG_DEBUG,"ICohServer::HandleStop\n");
    boost::system::error_code ignore_ec;
    m_oAcceptor.close(ignore_ec);
    bRunning=false;
}

}
}
