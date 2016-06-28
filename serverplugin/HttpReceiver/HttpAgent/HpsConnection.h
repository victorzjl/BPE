#ifndef _HPS_CONNECTION_H_
#define _HPS_CONNECTION_H_

#include "SmallObject.h"
#include "HpsHandlerAlloc.h"
#include "HpsBuffer.h"
#include "IHttpVirtualClient.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <string>
#include <deque>

using namespace std;
using namespace sdo::hps;
using boost::asio::ip::tcp;
class CHpsSession;
	
const unsigned int MAX_HPS_BUFFER=1024*1024;

typedef struct stHpsLenMsg
{
	int nLen;
	void *pBuffer;
}SHpsLenMsg;

class CHpsConnection:public boost::enable_shared_from_this<CHpsConnection>,
	private boost::noncopyable
{
public:	
	void AsyncRead();
	void DoSendResponse(bool isKeepAlive, int nHttpVersion, int nCode);
	void DoSendResponse(bool isKeepAlive, int nHttpVersion, const string &strCharSet,
		const string &strContentType, const string &strResponse, int nHttpCode,
		const vector<string>& vecCookie, const string& strLocationAddr,
		const string& strHeaders);
		
	void DoSendResponse(bool isKeepAlive, const string &strHttpResponse);
	void HandleSendResponse(const string &strResponse);
	void OnPeerClose();
        
        void OnReceiveVirtalClientRequest(char *pBuff, unsigned int nLen);
        void OnVirtalClientClosed();
public:
	void SetOwner(CHpsSession *pSession){m_pSession = pSession;}
	CHpsSession* GetOwner(){return m_pSession;}
	unsigned int Id(){return m_dwId;}
	tcp::socket& Socket(){return m_socket;}
	const string & GetRemoteIp()const {return m_strRemoteIp;}
	unsigned int GetRemotePort()const {return m_nRemotePort;}

public:
	CHpsConnection(boost::asio::io_service &io_service, IHttpVirtualClient *pHttpVirtualClient=NULL);
	~CHpsConnection();
	void Dump();	
	
private:	
	void AsyncReadInner();
	void HandleReadRequest(const boost::system::error_code& err,std::size_t dwTransferred);
	void HandleWriteResponse(const boost::system::error_code& err);
	void OnReceiveRequestCompleted(char * pBuff, int nLen);
    void DoReceiveVirtalClientRequest();
    void DoVirtalClientClosed();
	
private:
	boost::asio::io_service &m_oIoService;
	tcp::socket m_socket;
	
	CHpsBuffer hpsBuffer;
	char *m_pHead;

	unsigned int m_dwId;
	static unsigned int sm_dwMark;
	string m_strRemoteIp;
    unsigned int m_nRemotePort;

	CHpsHandlerAllocator m_allocRead;
	CHpsHandlerAllocator m_allocWrite;

	bool m_bRunning;
	CHpsSession *m_pSession;
	bool m_isKeepAlive;

	deque<SHpsLenMsg> m_queueOut;
        
        IHttpVirtualClient *m_pHttpVirtualClient;
};

typedef boost::shared_ptr<CHpsConnection> HpsConnection_ptr;

#endif

