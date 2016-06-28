#ifndef _HPS_SERVER_H_
#define _HPS_SERVER_H_

#include <boost/asio.hpp>
#include <string>
#include <map>
#include "HpsHandlerAlloc.h"
#include "HpsConnection.h"
#include "SessionManager.h"

using std::string;
using std::map;
using namespace sdo::hps;
	
class CHpsServer
{
public:
	CHpsServer(boost::asio::io_service &oIoService);
	virtual ~CHpsServer(){}
	virtual int StartServer(unsigned int dwPort);
	virtual void StopServer();
    bool IsStartStatus() { return bRunning; }
private:
	void StartAccept();
	void HandleStop();
	void HandleAccept(CHpsSessionManager * pManager,HpsConnection_ptr new_connection,const boost::system::error_code &e);
private:
	boost::asio::io_service &m_oIoService;
	boost::asio::ip::tcp::acceptor m_oAcceptor;

	CHpsHandlerAllocator m_allocator;
	bool bRunning;
};

#endif

