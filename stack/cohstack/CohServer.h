#ifndef _SDO_COH_SERVER_H_
#define _SDO_COH_SERVER_H_
#include <boost/asio.hpp>
#include "SmallObject.h"
#include "CohConnection.h"
#include <string>
#include "CohHandlerAlloc.h"
using std::string;
namespace sdo{
	namespace coh{
		class ICohServer:public sdo::common::CSmallObject
		{
			public:
				ICohServer();
				int Start(unsigned int dwPort);
				void Stop();
				virtual void OnReceiveRequest(void * pIdentifer,const string &strRequest,const string &strRemoteIp,unsigned int dwRemotPort)=0;
				void DoSendResponse(void *pIdentifer,const string &strResponse);
				static ICohServer * GetServer() {return sm_oServer;}
				virtual ~ICohServer(){}
			private:
				void StartAccept();
				void HandleAccept(CohConnection_ptr new_connection,const boost::system::error_code &e);
				void HandleStop();
				boost::asio::io_service &m_oIoService;
				boost::asio::ip::tcp::acceptor m_oAcceptor;
				static ICohServer * sm_oServer;
				CCohHandlerAllocator m_allocator;
				bool bRunning;
				
		};
	}
}
#endif

