#include "HttpClientSession.h"
#include "HttpRequestEncoder.h"
#include "HttpResponseDecoder.h"
#include "HttpClientContainer.h"
#include "SessionManager.h"
#include "HttpClientLogHelper.h"


CHttpClientSession::CHttpClientSession(unsigned int nIndex, CHttpClientContainer *pContainer):
	m_nId(nIndex),m_isConnected(false),m_pHttpClientContainer(pContainer)
{
}

CHttpClientSession::~CHttpClientSession()
{
	m_pConn->SetOwner(NULL);	
	m_pConn->OnPeerClose();
}


void CHttpClientSession::ConnectToServer(CHpsSessionManager *pManager, const string &strIp, unsigned int dwPort, int nTimeOut)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientSession::%s,strIp[%s],dwPort[%u],timeOut[%d]\n",__FUNCTION__,strIp.c_str(),dwPort,nTimeOut);
	m_strRemoteServer = strIp;
	m_dwRemotePort = dwPort;
	m_pConn.reset(new CHttpClientConnection(pManager->GetIoService(),this));
    m_pConn->AsyncConnectToServer(strIp,dwPort,nTimeOut);
}

void CHttpClientSession::ConnectResult(int nResult)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientSession::%s, nResult[%d]\n",__FUNCTION__,nResult);
	if(nResult == 0)
	{
		m_isConnected = true;
		while(!m_dequeRequest.empty())
		{
			deque<string>::iterator iter =  m_dequeRequest.begin();
			m_pConn->SendHttpRequest(*iter);			
			m_dequeRequest.pop_front();
		}
	}
	else
	{		
		HTTPCLIENT_XLOG(XLOG_WARNING,"CHttpClientSession::%s, connect to server fail. nCode[%d]\n",__FUNCTION__,ERROR_CONNECT_TO_HTTP_SERVER_FAIL);
		SendHttpRequestFailed(HTTP_CODE_CONNECT_HTTP_SERVER_FAIL, ERROR_CONNECT_TO_HTTP_SERVER_FAIL);
	}
}

void CHttpClientSession::SendHttpRequest(const string &strUrl, const string &strBody, const string &strHost, const string &strMethod, 
	const string &strXForwaredIp, const string &strHttpVersion, const string strConnType, int nTimeOut)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientSession::%s, strUrl[%s], strBody[%s], strHost[%s], strMethod[%s], strXForwaredIp[%s], strHttpVersion[%s]\n",__FUNCTION__,
		strUrl.c_str(),strBody.c_str(),strHost.c_str(),strMethod.c_str(),strXForwaredIp.c_str(),strHttpVersion.c_str());
	CHttpRequestEncoder requestEncoder;
	string strMsg = requestEncoder.Encode(strUrl,strBody,strHost,strXForwaredIp,strMethod,strHttpVersion,strConnType);
	if(m_isConnected==false)
	{
		HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientSession::%s, server is not connected, send request later\n",__FUNCTION__);
		m_dequeRequest.push_back(strMsg);
	}
	else
	{
		m_pConn->SendHttpRequest(strMsg);	
	}
}

void CHttpClientSession::SendHttpRequestFailed(int nHttpCode,int nAvenueCode)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientSession::%s, nHttpCode[%d], nAvenueCode[%d]\n",__FUNCTION__,nHttpCode,nAvenueCode);
	if(m_pHttpClientContainer != NULL)
	{
		m_pHttpClientContainer->OnReceiveHttpResponse(m_nId,nHttpCode,nAvenueCode,"");
	}
}

void CHttpClientSession::OnPeerClose()
{
}

void CHttpClientSession::Close()
{	
	m_pConn->SetOwner(NULL);
	m_pConn->OnPeerClose();
}


void CHttpClientSession::OnReceiveHttpResponse(char *pBuffer, int nLen)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientSession::%s, nLen[%d]\n",__FUNCTION__,nLen);
	CHttpResponseDecoder resDecoder;
	int nResult = resDecoder.Decode(pBuffer,nLen);
	if( nResult != 0)
	{
		HTTPCLIENT_XLOG(XLOG_WARNING,"CHttpClientSession::%s, Decode http response packet fail. nResult[%d] nCode[%d]\n",__FUNCTION__,nResult,ERROR_CONNECT_TO_HTTP_SERVER_FAIL);
		SendHttpRequestFailed(HTTP_CODE_DECODE_RESPONSE_FAIL, ERROR_DECODE_HTTP_RESPONSE_FAIL);
		return;
	}

	if(m_pHttpClientContainer != NULL)
	{
		int nAvenueCode = resDecoder.GetHttpCode()==200 ? 0 : ERROR_HTTP_SERVER_RESPONSE_FAIL;
		m_pHttpClientContainer->OnReceiveHttpResponse(m_nId,resDecoder.GetHttpCode(),nAvenueCode,resDecoder.GetBody());
	}	
}

