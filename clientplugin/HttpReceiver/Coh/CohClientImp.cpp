#include "CohClientImp.h"
#include "CohLogHelper.h"
#include "CommonMacro.h"


CHpsCohClientImp * CHpsCohClientImp::sm_instance=NULL;

CHpsCohClientImp * CHpsCohClientImp::GetInstance()
{
    if(sm_instance==NULL)
        sm_instance=new CHpsCohClientImp;
    return sm_instance;
}

void CHpsCohClientImp::Init(const string &strMonitorUrl, const string &strDetailMonitorUrl, const string &strDgsUrl, const string &strErrorUrl)
{
    ParseHttpUrl(strMonitorUrl,m_oMonitorServerInfo);
	ParseHttpUrl(strDetailMonitorUrl,m_oDetailMonitorServerInfo);
    ParseHttpUrl(strDgsUrl,m_oDgsServerInfo);
	ParseHttpUrl(strErrorUrl, m_oErrorServerInfo);
}

void CHpsCohClientImp::ParseHttpUrl(const string &strUrl,SHttpServerInfo & oServerInfo)
{
    char szHost[100]={0};
    char szPath[200]={'/','\0'};
    
    oServerInfo.dwPort=80;
    if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u/%199s", szHost, &oServerInfo.dwPort, szPath)!=3)
    {
		if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u", szHost, &oServerInfo.dwPort)!=2)
        {      
			sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]/%s", szHost, szPath);
        }
    }
    oServerInfo.strIp= szHost;
    oServerInfo.strPath=szPath;
}


void CHpsCohClientImp::SendReportDataToMonitor(const string &strContent)
{
#ifdef M_SUPPORT_PUBLIC
	return;
#endif
	string strRequest;
	strRequest.append("POST /"+m_oMonitorServerInfo.strPath+" HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
    strRequest.append("Content-Type: application/x-www-form-urlencoded\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
    
    char szBuf[128]={0};
    snprintf(szBuf,127,"Host: %s:%d\r\n",m_oMonitorServerInfo.strIp.c_str(),m_oMonitorServerInfo.dwPort);
    strRequest.append(szBuf);
    
    char bufLength[48]={0};
    sprintf(bufLength,"Content-Length: %d\r\n",strContent.length());
    strRequest.append(bufLength);
            
    strRequest.append("Connection: close\r\n\r\n");
    strRequest.append(strContent.c_str());
    DoSendRequest(m_oMonitorServerInfo.strIp,m_oMonitorServerInfo.dwPort,strRequest);
}

void CHpsCohClientImp::SendDetailReportDataToMonitor(const string &strContent)
{
#ifdef M_SUPPORT_PUBLIC
	return;
#endif
	string strRequest;
	strRequest.append("POST /"+m_oDetailMonitorServerInfo.strPath+" HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
    strRequest.append("Content-Type: application/x-www-form-urlencoded\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
    
    char szBuf[128]={0};
    snprintf(szBuf,127,"Host: %s:%d\r\n",m_oDetailMonitorServerInfo.strIp.c_str(),m_oDetailMonitorServerInfo.dwPort);
    strRequest.append(szBuf);
    
    char bufLength[48]={0};
    sprintf(bufLength,"Content-Length: %d\r\n",strContent.length());
    strRequest.append(bufLength);
            
    strRequest.append("Connection: close\r\n\r\n");
    strRequest.append(strContent.c_str());
    DoSendRequest(m_oDetailMonitorServerInfo.strIp,m_oDetailMonitorServerInfo.dwPort,strRequest);
}



void CHpsCohClientImp::SendToDgs(const string &strContent)
{
#ifdef M_SUPPORT_PUBLIC
	return;
#endif
    string strRequest;
	strRequest.append("POST /"+m_oDgsServerInfo.strPath+" HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
    strRequest.append("Content-Type: application/x-www-form-urlencoded\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
    
    char szBuf[128]={0};
    snprintf(szBuf,127,"Host: %s:%d\r\n",m_oDgsServerInfo.strIp.c_str(),m_oDgsServerInfo.dwPort);
    strRequest.append(szBuf);
    
    char bufLength[48]={0};
    sprintf(bufLength,"Content-Length: %d\r\n",strContent.length());
    strRequest.append(bufLength);
            
    strRequest.append("Connection: close\r\n\r\n");
    strRequest.append(strContent.c_str());
    DoSendRequest(m_oDgsServerInfo.strIp,m_oDgsServerInfo.dwPort,strRequest);
}

void CHpsCohClientImp::SendErrorDataToMonitor(const string &strContent)
{
#ifdef M_SUPPORT_PUBLIC
	return;
#endif
    string strRequest;
	strRequest.append("POST /"+m_oErrorServerInfo.strPath+" HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
    strRequest.append("Content-Type: application/x-www-form-urlencoded\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");
    
    char szBuf[128]={0};
    snprintf(szBuf,127,"Host: %s:%d\r\n",m_oErrorServerInfo.strIp.c_str(),m_oErrorServerInfo.dwPort);
    strRequest.append(szBuf);
    
    char bufLength[48]={0};
    sprintf(bufLength,"Content-Length: %d\r\n",strContent.length());
    strRequest.append(bufLength);
            
    strRequest.append("Connection: close\r\n\r\n");
    strRequest.append(strContent.c_str());
    DoSendRequest(m_oErrorServerInfo.strIp,m_oErrorServerInfo.dwPort,strRequest);
}

void CHpsCohClientImp::OnReceiveResponse(const string &strResponse)
{
    int httpCode=0;
    sscanf(strResponse.c_str(),"%*[HTTPhttp]/%*lf %d",&httpCode);
    if(httpCode<200||httpCode>=300)
    {
        CS_XLOG(XLOG_WARNING,"CHpsCohClientImp::OnReceiveResponse,httpCode[%d]\n",httpCode);
    }
}

