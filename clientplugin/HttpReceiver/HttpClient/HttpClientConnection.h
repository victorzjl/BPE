#ifndef _HTTP_CLIENT_CONNECTION_H_
#define _HTTP_CLIENT_CONNECTION_H_

#include "SmallObject.h"
#include "HttpClientHandlerAlloc.h"
#include "HttpClientBuffer.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <string>
#include <deque>

using namespace std;
using boost::asio::ip::tcp;

typedef struct stHttpClientMsg
{
	int nLen;
	void *pBuffer;
}SHttpClientMsg;

const unsigned int MAX_HTTP_CLIENT_BUFFER=1024*1024;


class CHttpClientSession;

class CHttpClientConnection:public sdo::common::CSmallObject,private boost::noncopyable,
	public boost::enable_shared_from_this<CHttpClientConnection>
{
public:
	CHttpClientConnection(boost::asio::io_service &io_service,CHttpClientSession *pSession);
	~CHttpClientConnection();

	void AsyncConnectToServer(const string &strIp,unsigned int dwPort,int nTimeout);

	void SendHttpRequest(const string &strRequest);
	void OnPeerClose();

	void SetOwner(CHttpClientSession *pSession){m_pSession = pSession;}
	
	tcp::socket& Socket(){return m_socket;}
	
	
private:	
	void HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void HandleConnectTimeout(const boost::system::error_code& err);
	void ConnectFinish(int nResult);
	
	void HandleSendRequest(const boost::system::error_code& err);
	
	void StartRead();
	void HandleReadResponse(const boost::system::error_code& err,std::size_t dwTransferred);
	void OnReceiveResponseCompleted(char *pBuffer, int nLen);



private:
	boost::asio::io_service &m_oIoService;
	tcp::socket m_socket;
	tcp::resolver m_resolver;
	boost::asio::deadline_timer m_connTimer;

	CHttpClientBuffer m_buffer;
	char *m_pHead;
	
	string m_strRemoteIp;
    unsigned int m_nRemotePort;
	bool m_isConnected;
	
	CHttpClientSession *m_pSession;

	CHttpClientHandlerAllocator m_allocConn;
	CHttpClientHandlerAllocator m_allocTimer;
	CHttpClientHandlerAllocator m_allocWriter;
	CHttpClientHandlerAllocator m_allocReader;
	
	deque<SHttpClientMsg> m_queueOut;
};

typedef boost::shared_ptr<CHttpClientConnection> CHttpClientConnection_ptr;


#endif



