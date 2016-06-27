#include "CohClientImp.h"
#include "CohLogHelper.h"


CCohClientImp * CCohClientImp::sm_instance=NULL;

CCohClientImp * CCohClientImp::GetInstance()
{
    if(sm_instance==NULL)
        sm_instance=new CCohClientImp;
    return sm_instance;
}

void CCohClientImp::Init(const string &strMonitorUrl, 
    const string &strDetailMonitorUrl, 
    const string &strDgsUrl,
    const string &strErrUrl)
{
    ParseHttpUrl(strMonitorUrl,m_oMonitorServerInfo);
	ParseHttpUrl(strDetailMonitorUrl,m_oDetailMonitorServerInfo);
    ParseHttpUrl(strDgsUrl,m_oDgsServerInfo);
    ParseHttpUrl(strErrUrl,m_oErrUrl);
}

void CCohClientImp::ParseHttpUrl(const string &strUrl,SHttpServerInfo & oServerInfo)
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
    oServerInfo.strIp   = szHost;
    oServerInfo.strPath = szPath;
}


void CCohClientImp::SendDataToMonitor(const string &strContent)
{
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
    strRequest.append(strContent);
    DoSendRequest(m_oMonitorServerInfo.strIp,m_oMonitorServerInfo.dwPort,strRequest);
}

void CCohClientImp::SendDetailDataToMonitor(const string &strContent)
{
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
    strRequest.append(strContent);
    DoSendRequest(m_oDetailMonitorServerInfo.strIp,m_oDetailMonitorServerInfo.dwPort,strRequest);
}



void CCohClientImp::SendToDgs(const string &strContent)
{
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
    strRequest.append(strContent);
    DoSendRequest(m_oDgsServerInfo.strIp,m_oDgsServerInfo.dwPort,strRequest);
}

void CCohClientImp::SendErrRequest(const string &strContent)
{
    char szBuf[128]={0};
    snprintf(szBuf,127,"Host: %s:%d\r\n",m_oErrUrl.strIp.c_str(),m_oErrUrl.dwPort);
    
    string strRequest("GET /");
	strRequest.append(m_oErrUrl.strPath+"?"+strContent+" HTTP/1.0\r\n");
	strRequest.append("Accept: */*\r\n");
	strRequest.append("User-Agent: COH Client/1.0\r\n");    
    strRequest.append(szBuf);
    strRequest.append("Connection: close\r\n\r\n");
    DoSendRequest(m_oErrUrl.strIp,m_oErrUrl.dwPort,strRequest);
}

void CCohClientImp::OnReceiveResponse(const string &strResponse)
{
    int httpCode=0;
    sscanf(strResponse.c_str(),"%*[HTTPhttp]/%*f %d",&httpCode);
    if(httpCode<200||httpCode>=300)
    {
        CS_XLOG(XLOG_WARNING,"CCohClientImp::OnReceiveResponse,httpCode[%d]\n",httpCode);
    }
}


