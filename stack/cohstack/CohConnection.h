#ifndef _COH_CONNECTION_H_
#define _COH_CONNECTION_H_
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "SmallObject.h"
#include "CohClient.h"
#include "CohHandlerAlloc.h"
#include <boost/asio/deadline_timer.hpp>
#include <string>

using std::string;
using boost::asio::ip::tcp;
namespace sdo{
namespace coh{
	const int MAX_COH_BUFFER=4096;
class CCohConnection:public sdo::common::CSmallObject,
	public boost::enable_shared_from_this<CCohConnection>,
	private boost::noncopyable
{
public:
	CCohConnection(boost::asio::io_service &io_service);
	CCohConnection(ICohClient *pClient,boost::asio::io_service &io_service);
	~CCohConnection();
	void StartReadRequest();
	void StartSendRequest(const string &strIp, unsigned int dwPort,const string & strRequest,int nTimeout);
	void DoSendResponse(const string &strResponse);
	
	tcp::socket& Socket(){return m_socket;}
	static void SetTimeout(int nTimeout){sm_nTimeout=nTimeout;}
private:
	void HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void HandleWriteRequest(const boost::system::error_code& err);
	void HhandleReadResponse(const boost::system::error_code& err,std::size_t dwTransferred);
	void HandleResponseTimeout(const boost::system::error_code& err);
	void OnReceiveResponseCompleted();

	void HandleReadRequest(const boost::system::error_code& err,std::size_t dwTransferred);
	void HandleWriteResponse(const boost::system::error_code& err);
	void HandleRequestTimeout(const boost::system::error_code& err);
	void OnReceiveRequestCompleted();
private:
	ICohClient *m_pClient;
	boost::asio::io_service &m_oIoService;
	tcp::socket m_socket;
	tcp::resolver m_resolver;
	boost::asio::deadline_timer m_timer;
	
	char m_szRequest[MAX_COH_BUFFER];
	string m_strRequest;
	
	char m_szResponse[MAX_COH_BUFFER];
	string m_strResponse;

	static int sm_nTimeout;
	CCohHandlerAllocator m_allocConn;
	CCohHandlerAllocator m_allocTimer;
};

typedef boost::shared_ptr<CCohConnection> CohConnection_ptr;
struct ConnectionAdapter:public sdo::common::CSmallObject
{
	CohConnection_ptr connection_;
};
}
}
#endif

