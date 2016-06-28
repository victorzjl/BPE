#ifndef _HTTP_CLIENT_SESSEION_H_
#define _HTTP_CLIENT_SESSEION_H_

#include "HttpClientConnection.h"
#include "SessionManager.h"

typedef boost::shared_ptr<CHttpClientConnection> HttpClientConnection_ptr;

class CHttpClientContainer;
class CHttpClientSession
{
public:
	CHttpClientSession(unsigned int nIndex, CHttpClientContainer *pContainer);
	~CHttpClientSession();
	
	void ConnectToServer(CHpsSessionManager *pManager,const string &strIp, unsigned int dwPort, int nTimeOut=5);
	void ConnectResult(int nResult);
	void SendHttpRequest(const string &strUrl, const string &strBody, const string &strHost, const string &strMethod="GET",const string &strXForwaredIp="", 
		const string &strHttpVersion="HTTP/1.1", const string strConnType="close", int nTimeOut=10);
	
	void OnPeerClose();
	void OnReceiveHttpResponse(char *pBuffer, int nLen);

	void Close();

public:
	bool IsConnected(){return m_isConnected;}
	void SendHttpRequestFailed(int nHttpCode,int nAvenueCode);

private:

private:
	unsigned int m_nId;

	string m_strRemoteServer;
	unsigned int m_dwRemotePort;
	
	bool m_isConnected;
	deque<string> m_dequeRequest;

	CHttpClientContainer *m_pHttpClientContainer;
	HttpClientConnection_ptr m_pConn;
};


#endif


