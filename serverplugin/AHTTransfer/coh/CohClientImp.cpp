#include "CohClientImp.h"
#include "UrlCode.h"
#include <boost/algorithm/string.hpp>
#include "CohResponseMsg.h"
#include "HTDealerServiceLog.h"


CCohClientImp * CCohClientImp::GetInstance()
{
	static CCohClientImp objCohClientImp;
	return &objCohClientImp;
}

void CCohClientImp::Init(const string &strAllUrl,const string &strDetailUrl,const string &strErrorUrl)
{
	ParseHttpUrl(strAllUrl, m_oAllUrl);
	ParseHttpUrl(strDetailUrl, m_oDetailUrl);
	ParseHttpUrl(strErrorUrl, m_oErrUrl);
}

void CCohClientImp::SendAllRequest(const string &strContent)
{
#ifndef M_SUPPORT_PUBLIC
    char szBuf[2048]={0};
	string strRequest;
	strRequest.append("POST /" + m_oAllUrl.strPath + " HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
    strRequest.append("Content-Type: application/x-www-form-urlencoded\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
    
    snprintf(szBuf,2047,"Host: %s:%d\r\n",m_oAllUrl.strIp.c_str(),m_oAllUrl.dwPort);
    strRequest.append(szBuf);
    
    char bufLength[48]={0};
    sprintf(bufLength,"Content-Length: %d\r\n",strContent.length());
    strRequest.append(bufLength);
            
    strRequest.append("Connection: close\r\n\r\n");
    strRequest.append(strContent.c_str());
   
    DoSendRequest(m_oAllUrl.strIp, m_oAllUrl.dwPort, strRequest);
#endif
}

void CCohClientImp::SendDetailRequest(const string &strContent)
{
#ifndef M_SUPPORT_PUBLIC
	string strRequest;
	strRequest.append("POST /" + m_oDetailUrl.strPath + " HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
	strRequest.append("Content-Type: application/x-www-form-urlencoded\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
	
	char szBuf[2048]={0};
	snprintf(szBuf,2047,"Host: %s:%d\r\n",m_oDetailUrl.strIp.c_str(), m_oDetailUrl.dwPort);
	strRequest.append(szBuf);
	
	char bufLength[48]={0};
	sprintf(bufLength,"Content-Length: %d\r\n",strContent.length());
	strRequest.append(bufLength);
			
	strRequest.append("Connection: close\r\n\r\n");
	strRequest.append(strContent.c_str());
	
	DoSendRequest(m_oDetailUrl.strIp, m_oDetailUrl.dwPort, strRequest);
#endif
}

void CCohClientImp::SendOurRequeset(const string &strContent)
{
}

void CCohClientImp::SendErrRequest(const string &strContent)
{
#ifndef M_SUPPORT_PUBLIC
    char szBuf[2048]={0};
    string strRequest;
	strRequest.append("GET /"+m_oErrUrl.strPath+"?"+strContent+" HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
    snprintf(szBuf,2047,"Host: %s:%d\r\n",m_oErrUrl.strIp.c_str(),m_oErrUrl.dwPort);
    strRequest.append(szBuf);
    strRequest.append("Connection: close\r\n\r\n");
    DoSendRequest(m_oErrUrl.strIp,m_oErrUrl.dwPort,strRequest);
#endif
}

void CCohClientImp::OnReceiveResponse(const string &strResponse)
{
	HT_XLOG(XLOG_DEBUG,"CCohClientImp::%s,strResponse[%s]\n",__FUNCTION__,strResponse.c_str());
}




