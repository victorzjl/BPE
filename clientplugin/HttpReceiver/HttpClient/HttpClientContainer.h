#ifndef _HTTP_CLIENT_CONTAINER_H_
#define _HTTP_CLIENT_CONTAINER_H_

#include "HttpClientSession.h"
#include "HpsCommonInner.h"
#include "HpsCommon.h"

const int HTTP_REQUEST_TIME_OUT = 10;

typedef struct stHttpRequest
{
	CHttpClientSession * pHttpClientSession;
	SAvenueRequestInfo sAvenueReqInfo;

	stHttpRequest():pHttpClientSession(NULL){}
	stHttpRequest(CHttpClientSession * pSession,const SAvenueRequestInfo &sAvenueReqInfo_):
		pHttpClientSession(pSession),sAvenueReqInfo(sAvenueReqInfo_){}
}SHttpRequest;

class CHpsSessionManager;
class CHttpClientContainer
{
public:
	CHttpClientContainer(){}
	CHttpClientContainer(CHpsSessionManager *pManager);

	void StartInThread();
    void StopInThread();

	int SendHttpRequest(SAvenueRequestInfo &sAvenueReqInfo);
	void OnReceiveHttpResponse(unsigned int dwHttpSessionId, int nHttpCode, int nAvenueCode, const string &strHttpResponseBody);

	unsigned int GetCurrentRequestNum(){return m_mapHttpRequestBySequecne.size();}
	void Dump();
	
private:
	void DoReceiveHttpResponse(SAvenueRequestInfo &sAvenueReqInfo);
	int ParseHttpUrl(const string &strUrl, SHttpServerInfo & oServerInfo); 

	void AddHistoryRequest(SHttpRequest &sReq);
	void RemoveHistory(SHttpRequest &sReq);
	void DoCheckHttpRequest();

private:
	CHpsSessionManager *m_pSessionManager;

	map<unsigned int, SHttpRequest> m_mapHttpRequestBySequecne;
	multimap<struct timeval_a,SHttpRequest> m_multimapHttpRequestByEndTime;


	unsigned int m_dwHttpRequestCount;
	CThreadTimer m_tmReqCheck;	
};

#endif

