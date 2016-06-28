#include "SessionManager.h"
#include "HttpClientContainer.h"
#include "HttpClientLogHelper.h"

CHttpClientContainer::CHttpClientContainer(CHpsSessionManager *pManager):m_pSessionManager(pManager),m_dwHttpRequestCount(0),
	m_tmReqCheck(m_pSessionManager, HTTP_REQUEST_TIME_OUT, boost::bind(&CHttpClientContainer::DoCheckHttpRequest,this),CThreadTimer::TIMER_CIRCLE)
{
	
}

void CHttpClientContainer::StartInThread()
{
	m_tmReqCheck.Start();
}

void CHttpClientContainer::StopInThread()
{
    m_tmReqCheck.Stop();
}

int CHttpClientContainer::SendHttpRequest(SAvenueRequestInfo &sAvenueReqInfo)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientContainer::%s, url[%s], body[%s]\n",__FUNCTION__,sAvenueReqInfo.strHttpUrl.c_str(),sAvenueReqInfo.strHttpRequestBody.c_str());
	sAvenueReqInfo.dwHttpSequence = ++m_dwHttpRequestCount;
	gettimeofday_a(&(sAvenueReqInfo.tHttpRequestStart),0);

	SHttpServerInfo oSrvInfo;
	if(0 > ParseHttpUrl(sAvenueReqInfo.strHttpUrl, oSrvInfo))
	{
		sAvenueReqInfo.nAvenueRetCode = ERROR_HTTP_REQUEST_URL_ILLEGAL;
		DoReceiveHttpResponse(sAvenueReqInfo);

		return -1;
	}	
	
	CHttpClientSession *pHttpClientSession = new CHttpClientSession(sAvenueReqInfo.dwHttpSequence, this);
	
	SHttpRequest sHttpRequest(pHttpClientSession, sAvenueReqInfo);
	AddHistoryRequest(sHttpRequest);
	m_mapHttpRequestBySequecne.insert(make_pair(sAvenueReqInfo.dwHttpSequence,sHttpRequest));	
	
	pHttpClientSession->ConnectToServer(m_pSessionManager,oSrvInfo.strIp,oSrvInfo.dwPort);
	pHttpClientSession->SendHttpRequest(oSrvInfo.strPath,sAvenueReqInfo.strHttpRequestBody,oSrvInfo.strHost,sAvenueReqInfo.strHttpMethod,sAvenueReqInfo.strSosIp);

	
	return 0;
}


void CHttpClientContainer::OnReceiveHttpResponse(unsigned int dwHttpSessionId, int nHttpCode, int nAvenueCode, const string &strHttpResponseBody)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientContainer::%s, sequence[%u],nHttpCode[%d],strHttpResponseBody[%s]\n",__FUNCTION__,dwHttpSessionId,nHttpCode,strHttpResponseBody.c_str());
	map<unsigned int, SHttpRequest>::iterator itr = m_mapHttpRequestBySequecne.find(dwHttpSessionId);
	if(itr != m_mapHttpRequestBySequecne.end())
	{
		SHttpRequest &sReq = itr->second;
		CHttpClientSession *pSession = sReq.pHttpClientSession;
		SAvenueRequestInfo &sAvenueReqInfo = sReq.sAvenueReqInfo;
		sAvenueReqInfo.nHttpCode = nHttpCode;
		sAvenueReqInfo.nAvenueRetCode = nAvenueCode;

		sAvenueReqInfo.strHttpResponse = strHttpResponseBody;
		DoReceiveHttpResponse(sAvenueReqInfo);
		
		RemoveHistory(sReq);
		delete pSession;
		m_mapHttpRequestBySequecne.erase(itr);
	}	
}


void CHttpClientContainer::DoReceiveHttpResponse(SAvenueRequestInfo &sAvenueReqInfo)
{	
	gettimeofday_a(&(sAvenueReqInfo.tHttpResponseStop),0);
	
	struct timeval_a interval2;
	timersub(&(sAvenueReqInfo.tHttpResponseStop),&(sAvenueReqInfo.tHttpRequestStart),&interval2);
	sAvenueReqInfo.fHttpResponseTime = (static_cast<float>(interval2.tv_sec))*1000 + (static_cast<float>(interval2.tv_usec))/1000;

	m_pSessionManager->OnReceiveHttpResponse(sAvenueReqInfo);
}



int CHttpClientContainer::ParseHttpUrl(const string &strUrl, SHttpServerInfo & oServerInfo)
{
    char szIp[100]={0};
    char szPath[200]={'/','\0'};
    
    oServerInfo.dwPort=80;
	
    if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u%199s", szIp, &oServerInfo.dwPort, szPath)!=3)
    {
		if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u", szIp, &oServerInfo.dwPort)!=2)
        {      
			if(sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]%s", szIp, szPath) < 1)
			{
				return -1;
			}
        }
    }

	oServerInfo.strIp= szIp;

	char szHost[256] = {0};
	if (oServerInfo.dwPort != 80)
		snprintf(szHost, 255, "%s:%u", szIp,oServerInfo.dwPort);
	else
		snprintf(szHost, 255, "%s", szIp);
	oServerInfo.strHost = szHost;
	
    oServerInfo.strPath=szPath;

	return 0;
}


void CHttpClientContainer::AddHistoryRequest(SHttpRequest &sReq)
{
	struct timeval_a interval;
	interval.tv_sec = HTTP_REQUEST_TIME_OUT;
	interval.tv_usec = 0;
    timeradd(&(sReq.sAvenueReqInfo.tHttpRequestStart),&interval,&(sReq.sAvenueReqInfo.tTimeOutEnd));

    m_multimapHttpRequestByEndTime.insert(make_pair(sReq.sAvenueReqInfo.tTimeOutEnd,sReq));
}

void CHttpClientContainer::RemoveHistory(SHttpRequest &sReq)
{
	std::pair<multimap<struct timeval_a,SHttpRequest>::iterator, multimap<struct timeval_a,SHttpRequest>::iterator> itr_pair;
	itr_pair= m_multimapHttpRequestByEndTime.equal_range(sReq.sAvenueReqInfo.tTimeOutEnd);
    multimap<struct timeval_a,SHttpRequest>::iterator itr;
    for(itr=itr_pair.first; itr!=itr_pair.second; ++itr)
    {
        SHttpRequest &oQueryRequest = itr->second;
        if(oQueryRequest.sAvenueReqInfo.dwHttpSequence == sReq.sAvenueReqInfo.dwHttpSequence)
        {
            m_multimapHttpRequestByEndTime.erase(itr);
            break;
        }
    }
}


void CHttpClientContainer::DoCheckHttpRequest()
{
    struct timeval_a now;
	gettimeofday_a(&now,0);
    while(!m_multimapHttpRequestByEndTime.empty())
    {
		HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientContainer::%s,   m_multimapHttpRequestByEndTime.size[%u]\n",__FUNCTION__,m_multimapHttpRequestByEndTime.size());
        if((m_multimapHttpRequestByEndTime.begin()->first)<now)
        {
            SHttpRequest & oRequest = m_multimapHttpRequestByEndTime.begin()->second;
			SAvenueRequestInfo &sAvenueReqInfo = oRequest.sAvenueReqInfo;
			sAvenueReqInfo.nHttpCode = HTTP_CODE_RESPONSE_TIME_OUT;
			sAvenueReqInfo.nAvenueRetCode = ERROR_HTTP_SERVER_RESPONSE_TIMEOUT;
			DoReceiveHttpResponse(sAvenueReqInfo);  
			delete oRequest.pHttpClientSession;
			m_mapHttpRequestBySequecne.erase(oRequest.sAvenueReqInfo.dwHttpSequence);
            m_multimapHttpRequestByEndTime.erase(m_multimapHttpRequestByEndTime.begin());
        }
        else
        {
            break;
        }
    }
}

void CHttpClientContainer::Dump()
{	
	HTTPCLIENT_XLOG(XLOG_NOTICE,"===========CHttpClientContainer::%s======m_dwHttpRequestCount[%u]======\n",__FUNCTION__,m_dwHttpRequestCount);

	HTTPCLIENT_XLOG(XLOG_NOTICE,"  +m_mapHttpRequestBySequecne.size[%u]\n",m_mapHttpRequestBySequecne.size());
	map<unsigned int, SHttpRequest>::iterator itr = m_mapHttpRequestBySequecne.begin();
	for(;itr!=m_mapHttpRequestBySequecne.end(); ++itr)
	{		
		HTTPCLIENT_XLOG(XLOG_NOTICE,"     - <%u, %s>\n",itr->first,itr->second.sAvenueReqInfo.strHttpUrl.c_str());
	}

	HTTPCLIENT_XLOG(XLOG_NOTICE,"  +m_multimapHttpRequestByEndTime.size[%u]\n",m_multimapHttpRequestByEndTime.size());
	multimap<struct timeval_a,SHttpRequest>::iterator itrM = m_multimapHttpRequestByEndTime.begin();
	for(;itrM!=m_multimapHttpRequestByEndTime.end(); ++itrM)
	{		
		HTTPCLIENT_XLOG(XLOG_NOTICE,"     - <%u.%u, %u>\n",itrM->first.tv_sec,itrM->first.tv_usec,itrM->second.sAvenueReqInfo.dwHttpSequence);
	}
}


