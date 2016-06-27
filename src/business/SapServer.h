#ifndef _SDO_SAP_AGENT_H_
#define _SDO_SAP_AGENT_H_
#include "SapConnection.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include "SapHandlerAlloc.h"
using std::string;
using std::map;

	
class CSapServer
{
public:
	static CSapServer *Instance();
	virtual int StartServer(unsigned int dwPort);
	virtual void StopServer();
private:
	void StartAccept();
	void HandleStop();
	void HandleAccept(SapConnection_ptr new_connection,const boost::system::error_code &e);
private:
	CSapServer();
	virtual ~CSapServer(){}
	static CSapServer *sm_pInstance;

	boost::asio::io_service &m_oIoService;
	boost::asio::ip::tcp::acceptor m_oAcceptor;

	CSapHandlerAllocator m_allocator;
	bool bRunning;

	
};
#endif

