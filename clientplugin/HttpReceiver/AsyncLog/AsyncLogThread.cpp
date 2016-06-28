#include <boost/bind.hpp>
#include <sys/types.h> 
#include <errno.h> 
#include <signal.h>
#include <sys/types.h>

#include "LogFilter.h"
#include "Common.h"
#include "CommonMacro.h"
#include "BusinessLogHelper.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"
#include "HpsStack.h"
#include "CohClientImp.h"
#include "HpsRequestMsg.h"
#include "HpsResponseMsg.h"
#include "boost/regex.hpp"
#include "boost/regex.hpp"
#include "boost/function.hpp"
#include "boost/bind.hpp"
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

struct branch{
	const char* forbidden_char;
	unsigned char size;
};
struct branch TABLE[] = {{"password=", 9},{"pwd=",4}};

void forbiddenPassowrd(const string & strUriAttr, string& attri)
{   
    for (unsigned int i=0; i<sizeof(TABLE)/sizeof(TABLE[0]); ++i)
    {
    	string::size_type loc = strUriAttr.find(TABLE[i].forbidden_char);
        while(loc!=string::npos)
        {
            if (loc ==0||strUriAttr[loc-1]=='&')
            {
                string::size_type begin = loc + TABLE[i].size; 
                string::size_type pos = attri.find("&", begin);
                string::size_type pwdLen = 0;
                if(pos != string::npos)
                {
                    pwdLen = pos - begin;                   
                }
                else
                {
                    pwdLen = attri.size() - begin;
                }
                attri.replace(begin, pwdLen, pwdLen, '*');  
                loc += pwdLen;
            }

			loc += TABLE[i].size;
            
            loc = strUriAttr.find(TABLE[i].forbidden_char, loc);
        }
    }
}

IAsyncLogModule * GetAsyncLogModule()
{
	return CHpsAsyncLogThread::GetInstance();
}

CHpsAsyncLogThread * CHpsAsyncLogThread::sm_pInstance=NULL;

CHpsAsyncLogThread * CHpsAsyncLogThread::GetInstance()
{
	if(sm_pInstance==NULL)
		sm_pInstance=new CHpsAsyncLogThread;
	return sm_pInstance;
}


CHpsAsyncLogThread::CHpsAsyncLogThread():m_nConnInfoLoc(0),m_nRequestLoc(0),m_nAvenueRequestLoc(0),
	m_tmDetailLog(this,DETAIL_LOG_INTERVAL,boost::bind(&CHpsAsyncLogThread::DoDetailLogTimeout,this),CThreadTimer::TIMER_CIRCLE),
	m_tmSelfcheck(this,SELFCHECK_INTERVAL,boost::bind(&CHpsAsyncLogThread::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE),
	m_tmReport(this,REPORT_INTERVAL,boost::bind(&CHpsAsyncLogThread::DoReport,this),CThreadTimer::TIMER_CIRCLE),
	m_tmConnectWarn(this, RE_CONN_INTERVAL, boost::bind(&CHpsAsyncLogThread::DoWarn, this), CThreadTimer::TIMER_CIRCLE)
{  
    m_szHttpConnInfoBuf[m_nConnInfoLoc] = '\0';
    m_szRequestBuf[m_nRequestLoc] = '\0';

	m_funMap[ASYNCLOG_Http_Connect_MSG] = &CHpsAsyncLogThread::DoHttpConnectInfo;
	m_funMap[ASYNCLOG_Http_Request_MSG] = &CHpsAsyncLogThread::DoReceiveHttpRequest;
	m_funMap[ASYNCLOG_DETAIL_REQUEST_MSG] = &CHpsAsyncLogThread::DoDetailLog;
	m_funMap[ASYNCLOG_SOS_STATUS_MSG] = &CHpsAsyncLogThread::DoReportSosStatus;
	m_funMap[ASYNCLOG_STOP_MSG] = &CHpsAsyncLogThread::DoStop;
	m_funMap[ASYNCLOG_Receive_Avenue_Request_MSG] = &CHpsAsyncLogThread::DoReceiveAvenueRequest;
	m_funMap[ASYNCLOG_Avenue_ReqRes_Detail_MSG] = &CHpsAsyncLogThread::DoDetailAvenueRequestLog;
	m_funMap[ASYNCLOG_Report_No_Http_Response_Num] = &CHpsAsyncLogThread::DoReportNoHttpResNum;
	m_funMap[ASYNCLOG_ReConn_Warn] = &CHpsAsyncLogThread::DoReConnWarn;
}

void CHpsAsyncLogThread::StartInThread()
{
	m_tmDetailLog.Start();
	m_tmSelfcheck.Start();
	m_tmReport.Start();
	m_tmConnectWarn.Start();
	m_logConfg.Load();
	m_logConfg.GetLogParam("default", m_defautLogKey1, m_defautLogKey2);


	char pIp[16] = { 0 };
	if (GetLocalAddress(pIp))
	{
		m_strIp = pIp;
	}
	else
	{
		m_strIp = "0.0.0.0";
	}
	

	time_t curTime;
    time(&curTime);   

    std::stringstream sout; 
    sout<<"{";     

    //session manager thread
    sout<< "\"HPS SessionManagerThread\":{\"error_id\":" << "60075000"
    << ",\"stat\":" << 0
    << ",\"msg\":\"" << "ThreadNum is \""
    << ",\"timestamp\":" << curTime
    << ",\"ip\":\""<< m_strIp << "\""<< "}"; 

     //sos session
    sout<< ",\"HPS SosSession\":{\"error_id\":" << "60075001"
    << ",\"stat\":" << 0
    << ",\"msg\":\"" << "ActiveSosSessionNum is "
    << ", DeadSosSession is " 
    << "\",\"timestamp\":" << curTime
    << ",\"ip\":\""<< m_strIp << "\"}"; 
     
    sout<< "}";

	{
		boost::mutex::scoped_lock sl(m_mutex);
	    m_strSelfCheck = sout.str(); 
	}
}

void CHpsAsyncLogThread::Deal(void * pData)
{
	SAsyncLogMsg * pMsg=(SAsyncLogMsg * )pData;
	(this->*(m_funMap[pMsg->enType]))(pMsg);
	delete pMsg;
}

void CHpsAsyncLogThread::RegisterLogger()
{
    XLOG_REGISTER(HTTP_CONNECT_DETAIL_MODULE, "connect_detail");
    XLOG_REGISTER(HTTP_REQUEST_DETAIL_MODULE, "request_detail");
    XLOG_REGISTER(HPS_STATISTICS_MODULE, "net_statistics");
    XLOG_REGISTER(SOS_STATISTICS_MODULE, "sos_statistics");
    XLOG_REGISTER(MSG_STATISTICS_MODULE, "msg_statistics");
    XLOG_REGISTER(SELF_CHECK_MODULE, "self_check");
    XLOG_REGISTER(AVENUE_REQUEST_DETAIL_MODULE, "avenue_request");

    XLOG_REGISTER(AVENUE_REQUEST_ALL_STATISTICS_MODULE, "all_avn_req_statistic");
    XLOG_REGISTER(SRVMSG_AVENUE_REQUEST_STATISTICS_MODULE, "srvmsg_avn_req_statistic");
    XLOG_REGISTER(SERVICE_AVENUE_REQUEST_STATISTICS_MODULE, "service_avn_req_statistic");
}

void CHpsAsyncLogThread::OnHttpConnectInfo(const SHttpConnectInfo &connInfo)
{
	SAsyncLogHttpConnect *pMsg = new SAsyncLogHttpConnect;
	pMsg->sConnectInfo = connInfo;
	
	TimedPutQ(pMsg);
}

void CHpsAsyncLogThread::OnReceiveHttpRequest(unsigned int dwServiceId, unsigned int dwMsgId)
{
	SAsyncLogHttpRequest *pMsg = new SAsyncLogHttpRequest;
	pMsg->dwServiceId = dwServiceId;
	pMsg->dwMsgId = dwMsgId;
	
	TimedPutQ(pMsg);
}

void CHpsAsyncLogThread::OnDetailLog(const SRequestInfo &reqInfo)
{
	SAsyncLogDetailRequestMsg *pMsg = new SAsyncLogDetailRequestMsg;
	pMsg->sRequest = reqInfo;
	
	TimedPutQ(pMsg);
}

void CHpsAsyncLogThread::OnReportSosStatus(const SSosStatusData &sosStatus)
{
	SAsyncLogSosStatusMsg *pMsg = new SAsyncLogSosStatusMsg;
	pMsg->sSosStatus = sosStatus;
	
	PutQ(pMsg);
}

void CHpsAsyncLogThread::OnStop()
{
	SAsyncLogStopMsg *pMsg = new SAsyncLogStopMsg;
	PutQ(pMsg);
}

void CHpsAsyncLogThread::OnDetailAvenueRequestLog(SAvenueRequestInfo &sAvenueReqInfo)
{
	SAsyncLogAvenueRequestDetailMsg *pMsg = new SAsyncLogAvenueRequestDetailMsg;
	pMsg->sAvenueReqInfo = sAvenueReqInfo;
	TimedPutQ(pMsg);
}

void CHpsAsyncLogThread::OnReceiveAvenueRequest(unsigned int dwServiceId, unsigned int dwMsgId)
{
	SAsyncLogReceiveAvenueRequestMsg *pMsg = new SAsyncLogReceiveAvenueRequestMsg;
	pMsg->dwServiceId = dwServiceId;
	pMsg->dwMsgId = dwMsgId;
	
	TimedPutQ(pMsg);
}

void CHpsAsyncLogThread::OnReConnWarn(const string &strReConnIp)
{
	SS_XLOG(XLOG_DEBUG, "CHpsAsyncLogThread::%s\n", __FUNCTION__);
	SAsyncLogReConnWarnMsg *pMsg = new SAsyncLogReConnWarnMsg;
	pMsg->strReConnIp = strReConnIp;
	PutQ(pMsg);
}

void CHpsAsyncLogThread::DoReceiveAvenueRequest(SAsyncLogMsg * pMsg)
{
	SAsyncLogHttpRequest *pAsyncLogMsg = (SAsyncLogHttpRequest*)pMsg;
	m_sAvenueReqStatisticDataAll.sReqResStatisticData.dwReqNum++;
	m_sAvenueReqStatisticDataAll.sHttpServerStatisticData.dwReqNum++;

	if(pAsyncLogMsg->dwServiceId != 0)
	{
		SAvenueReqStatisticData &sData1 = m_mapAvnReqStatisticDataByServiceId[pAsyncLogMsg->dwServiceId];
		sData1.sReqResStatisticData.dwReqNum++;
		
		SServiceMsgId sServMsg(pAsyncLogMsg->dwServiceId, pAsyncLogMsg->dwMsgId);
		SAvenueReqStatisticData &sData = m_mapAvnReqStatisticDataBySrvMsgId[sServMsg];
		sData.sReqResStatisticData.dwReqNum++;
	}
}

void CHpsAsyncLogThread::OnReportNoHttpResNum(int nThreadIndex, unsigned int nCurrentNoResNum)
{
	SAsyncLogReportNoHttpResNumMsg *pMsg = new SAsyncLogReportNoHttpResNumMsg;
	pMsg->nThreadIndex = nThreadIndex;
	pMsg->dwAvenueRequestNum = nCurrentNoResNum;
	
	PutQ(pMsg);
}

void CHpsAsyncLogThread::DoReportNoHttpResNum(SAsyncLogMsg * pMsg)
{
	SAsyncLogReportNoHttpResNumMsg *pAsyncLogMsg = (SAsyncLogReportNoHttpResNumMsg*)pMsg;
	boost::mutex::scoped_lock sl(m_mutexNoResNum);
	m_mapNoHttpResNumByThreadId[pAsyncLogMsg->nThreadIndex] = pAsyncLogMsg->dwAvenueRequestNum;
}


void CHpsAsyncLogThread::DoStop(SAsyncLogMsg * pMsg)
{
	DoDetailLogTimeout();
}


void CHpsAsyncLogThread::DoHttpConnectInfo(SAsyncLogMsg * pMsg)
{
	SAsyncLogHttpConnect *pAsyncLogMsg = (SAsyncLogHttpConnect*)pMsg;

	struct tm tmNow;
	struct timeval_a now;
	gettimeofday_a(&now,0);
	localtime_r(&(now.tv_sec), &tmNow);

	int nLen = -1;
	if(pAsyncLogMsg->sConnectInfo.eType == E_Http_Connect)
	{
		nLen = snprintf(m_szHttpConnInfoBuf + m_nConnInfoLoc,MAX_BUFFER_SIZE - m_nConnInfoLoc-1024,
			"HTTPConnect, %04u-%02u-%02u %02u:%02u:%02u %03u.%03lu, %u, %s:%u\n",
			tmNow.tm_year+1900,tmNow.tm_mon+1,tmNow.tm_mday,tmNow.tm_hour,tmNow.tm_min,tmNow.tm_sec,
			(unsigned int)(now.tv_usec/1000),now.tv_usec%1000, pAsyncLogMsg->sConnectInfo.dwId,
			pAsyncLogMsg->sConnectInfo.strRemoteIp.c_str(),pAsyncLogMsg->sConnectInfo.dwRemotePort);		
	
		m_sHpsStatisticData.dwConnected++;
		m_sHpsStatisticData.dwConnecting++;
	}
	else if(pAsyncLogMsg->sConnectInfo.eType == E_Http_Close)
	{	
		nLen = snprintf(m_szHttpConnInfoBuf + m_nConnInfoLoc,MAX_BUFFER_SIZE - m_nConnInfoLoc-1024,
			"HTTPClose, %04u-%02u-%02u %02u:%02u:%02u %03u.%03lu, %u, %s:%u\n",
			tmNow.tm_year+1900,tmNow.tm_mon+1,tmNow.tm_mday,tmNow.tm_hour,tmNow.tm_min,tmNow.tm_sec,
			(unsigned int)(now.tv_usec/1000),now.tv_usec%1000, pAsyncLogMsg->sConnectInfo.dwId,
			pAsyncLogMsg->sConnectInfo.strRemoteIp.c_str(),pAsyncLogMsg->sConnectInfo.dwRemotePort);

		m_sHpsStatisticData.dwDisConnect++;
		m_sHpsStatisticData.dwConnecting--;
	}
	else
	{
	}

	if(nLen > 0)
	{
		m_nConnInfoLoc += nLen;
		if (m_nConnInfoLoc > MAX_BUFFER_SIZE - 1024)
			m_nConnInfoLoc = MAX_BUFFER_SIZE - 1024;
		m_szHttpConnInfoBuf[m_nConnInfoLoc]='\0';
	}
	
    if(m_nConnInfoLoc > (MAX_BUFFER_SIZE-2048))
    {
        XLOG_BUFFER(HTTP_CONNECT_DETAIL_MODULE,XLOG_INFO,m_szHttpConnInfoBuf);
        m_nConnInfoLoc = 0;
        m_szHttpConnInfoBuf[m_nConnInfoLoc]=0;
    }	
}

void CHpsAsyncLogThread::DoReceiveHttpRequest(SAsyncLogMsg * pMsg)
{
	SAsyncLogHttpRequest *pAsyncLogMsg = (SAsyncLogHttpRequest*)pMsg;
	m_sHpsStatisticData.dwRequest++;

	if(pAsyncLogMsg->dwServiceId != 0)
	{
		SSosStatisticData &sData1 = m_mapStatisticDataByServiceId[pAsyncLogMsg->dwServiceId];
		sData1.dwRequest++;
		
		SServiceMsgId sServMsg(pAsyncLogMsg->dwServiceId, pAsyncLogMsg->dwMsgId);
		SSosStatisticData &sData2 = m_mapStatisticDataBySrvMsgId[sServMsg];
		sData2.dwRequest++;
	}
}

void CHpsAsyncLogThread::DoReConnWarn(SAsyncLogMsg *pMsg)
{
	SS_XLOG(XLOG_DEBUG, "CHpsAsyncLogThread::%s\n", __FUNCTION__);
	SAsyncLogReConnWarnMsg *pReConnWarnMsg = (SAsyncLogReConnWarnMsg *)pMsg;
	m_setReConnIps.insert(pReConnWarnMsg->strReConnIp);
}


void CHpsAsyncLogThread::DoDetailLog(SAsyncLogMsg * pMsg)
{
	SAsyncLogDetailRequestMsg *pAsyncLogMsg = (SAsyncLogDetailRequestMsg*)pMsg;
	SRequestInfo &sReq = pAsyncLogMsg->sRequest;

	struct tm tmStart;
	localtime_r(&(sReq.tStart.tv_sec),&tmStart);

// get request key-parameter
	string strUriAttr = sReq.strUriAttribute;
	transform(strUriAttr.begin(), strUriAttr.end(), strUriAttr.begin(), (int(*)(int))tolower);

//hide the password information
	if( sReq.nSignatureStatus != OSAP_ERROR_SIG)
	{
		forbiddenPassowrd(strUriAttr, sReq.strUriAttribute);
	}
    

	string strLogKey1 = m_defautLogKey1, strLogKey2 = m_defautLogKey2;
	string strLogParam1, strLogParam2;

	m_logConfg.GetLogParam(sReq.strUrlIdentify, strLogKey1, strLogKey2);
	if (!strLogKey1.empty())
	{
		strLogParam1 = sdo::hps::CHpsRequestMsg::GetValueByKey(strUriAttr, strLogKey1);
	}
	if (!strLogKey2.empty())
	{
		strLogParam2 = sdo::hps::CHpsRequestMsg::GetValueByKey(strUriAttr, strLogKey2);
	}

	if (sReq.strUriAttribute.length() > 1024)
	{
		sReq.strUriAttribute = sReq.strUriAttribute.substr(0, 1024) + "...";
	}

	if (!sReq.strCookie.empty())
	{
		sReq.strUriAttribute += "," + sReq.strCookie;
	}
	for (vector<string>::iterator iter = sReq.vecSetCookie.begin();
		iter != sReq.vecSetCookie.end(); ++iter)
	{
		sReq.strResponse += *iter;
	}
	if(sReq.strResponse.length() > 4096)
	{
	    sReq.strResponse = sReq.strResponse.substr(0,4096)+"...";
	}
	string::size_type pos=sReq.strResponse.find_first_of("\n\r");
	while(pos != string::npos)
	{
	    sReq.strResponse[pos]=' ';
	    pos=sReq.strResponse.find_first_of("\n\r",pos+1);
	}
	
    //Filter Sensitive Information in Request
    string strFilterOut;
    transform(sReq.strUrlIdentify.begin(), sReq.strUrlIdentify.end(), sReq.strUrlIdentify.begin(), ::tolower);
    if (CLogFilter::FilterKeyValue("REQUEST:"+sReq.strUrlIdentify, sReq.strUriAttribute, strFilterOut))
    {
        sReq.strUriAttribute = strFilterOut;
    }
    //Filter Sensitive Information in Response
    strFilterOut = "";
    if (CLogFilter::FilterJson("RESPONSE:" + sReq.strUrlIdentify, sReq.strResponse, strFilterOut))
    {
        sReq.strResponse = strFilterOut;
    }
	
    string strResponse;
    if (sReq.bIsTransfer)
    {
        strResponse = "[Agent]";
        strResponse += sReq.strResponse.substr(0, 100);
    }
    else if (!sReq.strResourceAbsPath.empty())
    {
        strResponse = sReq.strResourceAbsPath;
    }
    else
    {
        strResponse = sReq.strResponse;
    }
	
	//HttpRequest, httpSessionId, remoteIp:remotePort, strUriCommand, strUriAttribute, request time, serviced, msgid, SosIp:SosPort, interval,  code, strOriginReq,strResponseMsg
	int nLen = snprintf(m_szRequestBuf + m_nRequestLoc,MAX_BUFFER_SIZE - m_nRequestLoc - 1024,
			"%04u-%02u-%02u %02u:%02u:%02u %03u.%03lu,   %s:%u,   "\
			"%s,   %u,   %u,   %d,   %d,   %s,   %s,   %s,   %s,   %s:%u,   %.3f,   %d,   %s,   tmstamp:%d,   sigstat:%d,   %s,   %s,  rpcGuid:%s,  "\
            "user-agent:%s,  fail_reason:%s\n",            
            tmStart.tm_year+1900,tmStart.tm_mon+1,tmStart.tm_mday,tmStart.tm_hour, tmStart.tm_min, tmStart.tm_sec,
            (unsigned int)(sReq.tStart.tv_usec/1000), sReq.tStart.tv_usec%1000, 
            sReq.strRemoteIp.c_str(), sReq.dwRemotePort, sReq.strUriCommand.c_str(), 
            sReq.nServiceId, sReq.nMsgId, sReq.nAppid, sReq.nAreaId,
			sReq.strUserName.c_str(), strLogParam1.c_str(), strLogParam2.c_str(),
			sReq.strUriAttribute.c_str(), sReq.strSosIp.c_str(), sReq.dwSosPort, sReq.fSpentTime,
            sReq.nCode, strResponse.c_str(), sReq.nTimestamp, sReq.nSignatureStatus, sReq.strGuid.c_str(),
            sReq.strHost.c_str(), sReq.strRpcGuid.c_str(), sReq.strUserAgent.c_str(), sReq.strErrorReason.c_str());

	if(nLen > 0)
	{
		m_nRequestLoc += nLen;
		if (m_nRequestLoc >= MAX_BUFFER_SIZE - 1024)
			m_nRequestLoc = MAX_BUFFER_SIZE - 1024;
		m_szRequestBuf[m_nRequestLoc]='\0';
	}
    if(m_nRequestLoc > (MAX_BUFFER_SIZE-24000))
    {
		if (m_szRequestBuf[m_nRequestLoc-1] != '\n')
			m_szRequestBuf[m_nRequestLoc-1] = '\n';
        XLOG_BUFFER(HTTP_REQUEST_DETAIL_MODULE,XLOG_INFO,m_szRequestBuf);
        m_nRequestLoc = 0;
        m_szRequestBuf[m_nRequestLoc]=0;
    }	

	if(sReq.nCode == 0)
	{
		m_sHpsStatisticData.dwResponseSucc++;
	}
	else
	{
		m_sHpsStatisticData.dwResponseFail++;
	}
	
	if(sReq.nServiceId != 0)
	{
		SSosStatisticData &sData1 = m_mapStatisticDataByServiceId[sReq.nServiceId];
		sData1.UpdateData(sReq.nCode,sReq.fSpentTime);
		
		SServiceMsgId sServMsg(sReq.nServiceId, sReq.nMsgId);
		SSosStatisticData &sData2 = m_mapStatisticDataBySrvMsgId[sServMsg];
		sData2.UpdateData(sReq.nCode,sReq.fSpentTime);
	}
}


void CHpsAsyncLogThread::DoDetailAvenueRequestLog(SAsyncLogMsg * pMsg)
{
	SAsyncLogAvenueRequestDetailMsg *pAsyncLogMsg = (SAsyncLogAvenueRequestDetailMsg*)pMsg;
	SAvenueRequestInfo &sReq = pAsyncLogMsg->sAvenueReqInfo;

	struct tm tmStart;
	localtime_r(&(sReq.tStart.tv_sec), &tmStart);

	int nLen = snprintf(m_szAvenueRequestBuf + m_nAvenueRequestLoc,MAX_BUFFER_SIZE - m_nAvenueRequestLoc - 1024,
			"%04u-%02u-%02u %02u:%02u:%02u %03u.%03lu,  %.3f,   %.3f   "\
			"%u,   %u,   %u,   %u,   %s:%u,   %d,   %d,   %s,    %s,   %s,   %s\n",
            tmStart.tm_year+1900, tmStart.tm_mon+1, tmStart.tm_mday, tmStart.tm_hour, tmStart.tm_min, tmStart.tm_sec, 
            (unsigned int)(sReq.tStart.tv_usec/1000), sReq.tStart.tv_usec%1000, sReq.fAllSpentTime, sReq.fHttpResponseTime,
			sReq.dwServiceId,sReq.dwMsgId,sReq.dwAvenueSequence,sReq.dwHttpSequence, sReq.strSosIp.c_str(),sReq.dwSosPort,
			sReq.nHttpCode,sReq.nAvenueRetCode,sReq.strHttpMethod.c_str(),sReq.strHttpUrl.c_str(),
			sReq.strHttpRequestBody.c_str(),sReq.strHttpResponse.c_str());

	if(nLen > 0)
	{
		m_nAvenueRequestLoc += nLen;
		if (m_nAvenueRequestLoc >= MAX_BUFFER_SIZE - 1024)
			m_nAvenueRequestLoc = MAX_BUFFER_SIZE - 1024;
		m_szAvenueRequestBuf[m_nAvenueRequestLoc]='\0';
	}
    if(m_nAvenueRequestLoc > (MAX_BUFFER_SIZE-2048))
    {
        XLOG_BUFFER(AVENUE_REQUEST_DETAIL_MODULE,XLOG_INFO,m_szAvenueRequestBuf);
        m_nAvenueRequestLoc = 0;
        m_szAvenueRequestBuf[m_nAvenueRequestLoc]=0;
    }

	m_sAvenueReqStatisticDataAll.sReqResStatisticData.UpdateData(sReq.nAvenueRetCode);
	if(sReq.nHttpCode != -1)
	{
		int nCode = sReq.nHttpCode==200 ? 0 : -1;
		m_sAvenueReqStatisticDataAll.sHttpServerStatisticData.UpdateData(nCode);
	}
	else
	{
		m_sAvenueReqStatisticDataAll.sHttpServerStatisticData.dwReqNum--;
	}
	m_sAvenueReqStatisticDataAll.sAllTimeStatisticData.UpdateData(sReq.fAllSpentTime);
	m_sAvenueReqStatisticDataAll.sHttpServerTimeStatisticData.UpdateData(sReq.fHttpResponseTime);
	
	if(sReq.dwServiceId != 0)
	{	
		SAvenueReqStatisticData &sData1 = m_mapAvnReqStatisticDataByServiceId[sReq.dwServiceId];
		sData1.sReqResStatisticData.UpdateData(sReq.nAvenueRetCode);
		sData1.sAllTimeStatisticData.UpdateData(sReq.fAllSpentTime);

		
		SServiceMsgId sServMsg(sReq.dwServiceId, sReq.dwMsgId);
		SAvenueReqStatisticData &sData = m_mapAvnReqStatisticDataBySrvMsgId[sServMsg];
		sData.sReqResStatisticData.UpdateData(sReq.nAvenueRetCode);
		sData.sAllTimeStatisticData.UpdateData(sReq.fAllSpentTime);
		sData.sHttpServerTimeStatisticData.UpdateData(sReq.fHttpResponseTime);
	}
}


void CHpsAsyncLogThread::DoReportSosStatus(SAsyncLogMsg * pMsg)
{
	SAsyncLogSosStatusMsg *pAsyncLogMsg = (SAsyncLogSosStatusMsg *)pMsg;
	m_sSosStatusData = pAsyncLogMsg->sSosStatus;
}


void CHpsAsyncLogThread::DoDetailLogTimeout()
{
	if(m_nConnInfoLoc>0)
    {
        XLOG_BUFFER(HTTP_CONNECT_DETAIL_MODULE,XLOG_INFO,m_szHttpConnInfoBuf);
        m_nConnInfoLoc = 0;
        m_szHttpConnInfoBuf[m_nConnInfoLoc] = 0;
    }
	if(m_nRequestLoc>0)
    {
        XLOG_BUFFER(HTTP_REQUEST_DETAIL_MODULE,XLOG_INFO,m_szRequestBuf);
        m_nRequestLoc=0;
        m_szRequestBuf[m_nRequestLoc]=0;
    }
	
	if(m_nAvenueRequestLoc>0)
    {
        XLOG_BUFFER(AVENUE_REQUEST_DETAIL_MODULE,XLOG_INFO,m_szAvenueRequestBuf);
        m_nAvenueRequestLoc=0;
        m_szAvenueRequestBuf[m_nAvenueRequestLoc]=0;
    }
}

void CHpsAsyncLogThread::DoSelfCheck()
{
	time_t curTime;
    time(&curTime);   

	int nThreads = 0;
	int nDeadThreads = 0;
	CHpsStack::Instance()->GetThreadStatus(nThreads,nDeadThreads);

	string strDeadConnectAddr = "";
	vector<string>::iterator iter;
	for(iter = m_sSosStatusData.vecDeadSosAddr.begin(); iter != m_sSosStatusData.vecDeadSosAddr.end(); ++iter)
	{
		strDeadConnectAddr += *iter + ";";
	}
	if(!strDeadConnectAddr.empty())
	{
		strDeadConnectAddr = strDeadConnectAddr.substr(0,strDeadConnectAddr.size()-1);
	}
	
    
    std::stringstream sout; 
    sout<<"{";     

    //session manager thread
    sout<< "\"HPS SessionManagerThread\":{\"error_id\":" << "60075000"
    << ",\"stat\":" << nDeadThreads
    << ",\"msg\":\"" << "ThreadNum is " << nThreads
    << "\",\"timestamp\":" << curTime
    << ",\"ip\":\""<< m_strIp << "\""<< "}"; 

     //sos session
    sout<< ",\"HPS SosSession\":{\"error_id\":" << "60075001"
    << ",\"stat\":" << m_sSosStatusData.dwDeadConnect
    << ",\"msg\":\"" << "ActiveSosSessionNum is " <<m_sSosStatusData.dwActiveConnect
    << ", DeadSosSession is "  << strDeadConnectAddr
    << "\",\"timestamp\":" << curTime
    << ",\"ip\":\""<< m_strIp << "\"}"; 
     
    sout<< "}";

	{
		boost::mutex::scoped_lock sl(m_mutex);
		m_strSelfCheck=sout.str();
	}

	SELF_CEHCK_XLOG(XLOG_INFO,"SessinoManager[%d/%d] SosSession[%d/%d] SosDeadAddr[%s] \n", 
			nDeadThreads,nThreads,m_sSosStatusData.dwDeadConnect, 
			m_sSosStatusData.dwActiveConnect + m_sSosStatusData.dwDeadConnect,
			strDeadConnectAddr.c_str());
}

void CHpsAsyncLogThread::DoReport()
{
	SHPSStatisticData hpsData = m_sHpsStatisticData - m_sHpsStatisticOldData;
	m_sHpsStatisticOldData = m_sHpsStatisticData;
/*hps report data: */
	/* write to disk */
	HPS_STATISTIC_XLOG(XLOG_INFO, "connected[%u,%u,%d], request[%u,%u,%u]\n",
		hpsData.dwConnected,hpsData.dwDisConnect,(int)(hpsData.dwConnecting),
		hpsData.dwRequest,hpsData.dwResponseSucc,hpsData.dwResponseFail);

	/* report to monitor */
	char szReport[1024]={0};
	snprintf(szReport,1023,\
		"action[]=2053|2058|2068|2071,%d&action[]=2053|2058|2068|2072,%d&action[]=2053|2058|2068|2073,%d" \
        "&action[]=2053|2058|2068|2074,%d&action[]=2053|2058|2068|2075,%d&action[]=2053|2058|2068|2076,%d"\
        "&ip=%s",
        hpsData.dwConnected,hpsData.dwDisConnect,(int)(hpsData.dwConnecting),
        hpsData.dwRequest,hpsData.dwResponseSucc,hpsData.dwResponseFail, m_strIp.c_str());
	CHpsCohClientImp::GetInstance()->SendReportDataToMonitor(szReport);

/* sos report data by serviceId*/
	std::stringstream streamSosStatisticsToMonitor;
	
	map<unsigned int, SSosStatisticData>::iterator iter = m_mapStatisticDataByServiceId.begin();
	while(iter != m_mapStatisticDataByServiceId.end())
	{
		map<unsigned int, SSosStatisticData>::iterator iterTemp = iter++;

		unsigned int dwServiceId = iterTemp->first;
		SSosStatisticData &sSrvData = iterTemp->second;
		SSosStatisticData &sOldSrvData = m_mapStatisticOldDataByServiceId[dwServiceId];
		SSosStatisticData sSrvDataInterval = sSrvData - sOldSrvData;
		sOldSrvData = sSrvData;

		if(sSrvDataInterval == 0)
		{
			m_mapStatisticDataByServiceId.erase(iterTemp);
			m_mapStatisticOldDataByServiceId.erase(dwServiceId);
			continue;
		}
		
		/* write to disk */		
		SOS_STATISTIC_XLOG(XLOG_INFO,"serviceId[%u], reqNum[%u,%u,%u], timeSpent[%u, %u, %u, %u, %u, %u]\n",
			dwServiceId, sSrvDataInterval.dwRequest, sSrvDataInterval.dwResSucc, sSrvDataInterval.dwResFail, 
			sSrvDataInterval.dwRes10ms, sSrvDataInterval.dwRes50ms, sSrvDataInterval.dwRes250ms, sSrvDataInterval.dwRes1s, 
			sSrvDataInterval.dwRes3s, sSrvDataInterval.dwResOthers);

		/* report to monitor */		
		streamSosStatisticsToMonitor << "action[]=2053|2059|2078," << dwServiceId << "," << sSrvDataInterval.dwRequest
				<< "&action[]=2053|2060|2081," << dwServiceId << "," << sSrvDataInterval.dwResSucc
				<< "&action[]=2053|2061|2084," << dwServiceId << "," << sSrvDataInterval.dwResFail
				<< "&action[]=2053|2062|2087," << dwServiceId << "," << sSrvDataInterval.dwRes10ms
				<< "&action[]=2053|2063|2090," << dwServiceId << "," << sSrvDataInterval.dwRes50ms
				<< "&action[]=2053|2064|2093," << dwServiceId << "," << sSrvDataInterval.dwRes250ms
				<< "&action[]=2053|2065|2096," << dwServiceId << "," << sSrvDataInterval.dwRes1s
				<< "&action[]=2053|2066|2099," << dwServiceId << "," << sSrvDataInterval.dwRes3s
				<< "&action[]=2053|2067|2102," << dwServiceId << "," << sSrvDataInterval.dwResOthers
				<< "&";
	}
	if(!(streamSosStatisticsToMonitor.str().empty()))
	{
		streamSosStatisticsToMonitor << "&ip=" << m_strIp;
		CHpsCohClientImp::GetInstance()->SendDetailReportDataToMonitor(streamSosStatisticsToMonitor.str());
	}



/* sos report data by service+msgId*/
	std::stringstream streamMsgStatisticDataToMonitor;
	map<SServiceMsgId, SSosStatisticData>::iterator iter2 = m_mapStatisticDataBySrvMsgId.begin();
	while(iter2 != m_mapStatisticDataBySrvMsgId.end())
	{
		map<SServiceMsgId, SSosStatisticData>::iterator itr2Tmp = iter2++;
		SServiceMsgId sKey = itr2Tmp->first;
		SSosStatisticData &sMsgData = itr2Tmp->second;
		SSosStatisticData &sOldMsgData = m_mapStatisticOldDataBySrvMsgId[sKey];
		SSosStatisticData sMsgDataInterval = sMsgData - sOldMsgData;
		sOldMsgData = sMsgData;

		if(sMsgDataInterval == 0)
		{
			m_mapStatisticDataBySrvMsgId.erase(itr2Tmp);
			m_mapStatisticOldDataBySrvMsgId.erase(sKey);
			continue;
		}

		char szMsgInfo[64] = {0};
		snprintf(szMsgInfo, 63, "%u_%u", sKey.dwServiceID, sKey.dwMsgID);

		/*serviceId_msgId[%s], reqNum[request,responseSucc, responseFail], timeSpent[10ms, 50ms, 250ms, 1s, 5s, other]*/
		MSG_STATISTIC_XLOG(XLOG_INFO,"serviceId_msgId[%s], reqNum[%u,%u,%u], timeSpent[%u, %u, %u, %u, %u, %u]\n",
			szMsgInfo, sMsgDataInterval.dwRequest, sMsgDataInterval.dwResSucc, sMsgDataInterval.dwResFail, 
			sMsgDataInterval.dwRes10ms, sMsgDataInterval.dwRes50ms, sMsgDataInterval.dwRes250ms, sMsgDataInterval.dwRes1s, sMsgDataInterval.dwRes3s, sMsgDataInterval.dwResOthers);


		streamMsgStatisticDataToMonitor << "action[]=2053|2942|2952," << szMsgInfo << "," << sMsgDataInterval.dwRequest
				<< "&action[]=2053|2943|2955," << szMsgInfo << "," << sMsgDataInterval.dwResSucc
				<< "&action[]=2053|2944|2958," << szMsgInfo << "," << sMsgDataInterval.dwResFail
				<< "&action[]=2053|2945|2961," << szMsgInfo << "," << sMsgDataInterval.dwRes10ms
				<< "&action[]=2053|2946|2964," << szMsgInfo << "," << sMsgDataInterval.dwRes50ms
				<< "&action[]=2053|2947|2967," << szMsgInfo << "," << sMsgDataInterval.dwRes250ms
				<< "&action[]=2053|2948|2970," << szMsgInfo << "," << sMsgDataInterval.dwRes1s
				<< "&action[]=2053|2949|2973," << szMsgInfo << "," << sMsgDataInterval.dwRes3s
				<< "&action[]=2053|2950|2976," << szMsgInfo << "," << sMsgDataInterval.dwResOthers
				<< "&";
	}

	if(!(streamMsgStatisticDataToMonitor.str().empty()))
	{
		streamMsgStatisticDataToMonitor << "&ip=" << m_strIp;
		CHpsCohClientImp::GetInstance()->SendDetailReportDataToMonitor(streamMsgStatisticDataToMonitor.str());
	}

	ReportAvenueReqStatisticData();
}


void CHpsAsyncLogThread::ReportAvenueReqStatisticData()
{
	SAvenueReqStatisticData sData = m_sAvenueReqStatisticDataAll - m_sOldAvenueReqStatisticDataAll;
	m_sOldAvenueReqStatisticDataAll = m_sAvenueReqStatisticDataAll;
	/* write to disk */
	ALL_AVENUE_REQUEST_STATISTIC_XLOG(XLOG_INFO, "AllRequest[%u,%u,%u], AllTimeSpent[%u, %u, %u, %u, %u, %u], HttpServerRequest[%u,%u,%u], HttpServerTimeSpent[%u, %u, %u, %u, %u, %u]\n",
		sData.sReqResStatisticData.dwReqNum,sData.sReqResStatisticData.dwResSuccNum,sData.sReqResStatisticData.dwResFailNum,
		sData.sAllTimeStatisticData.dwRes10ms,sData.sAllTimeStatisticData.dwRes50ms,sData.sAllTimeStatisticData.dwRes250ms,
		sData.sAllTimeStatisticData.dwRes1s,sData.sAllTimeStatisticData.dwRes3s,sData.sAllTimeStatisticData.dwResOthers,
		sData.sHttpServerStatisticData.dwReqNum,sData.sHttpServerStatisticData.dwResSuccNum,sData.sHttpServerStatisticData.dwResFailNum,
		sData.sHttpServerTimeStatisticData.dwRes10ms,sData.sHttpServerTimeStatisticData.dwRes50ms,sData.sHttpServerTimeStatisticData.dwRes250ms,
		sData.sHttpServerTimeStatisticData.dwRes1s,sData.sHttpServerTimeStatisticData.dwRes3s,sData.sHttpServerTimeStatisticData.dwResOthers);

	/* report to monitor */
	char szReport[1024]={0};
	snprintf(szReport,1023,\
		"action[]=2053|5532|5533|5536,%d&action[]=2053|5532|5533|5537,%d&action[]=2053|5532|5533|5538,%d" \
        "&action[]=2053|5532|5533|5539,%d&action[]=2053|5532|5533|5540,%d&action[]=2053|5532|5533|5541,%d"\
        "&action[]=2053|5532|5533|5542,%d&action[]=2053|5532|5533|5543,%d&action[]=2053|5532|5533|5544,%d"\
		"&action[]=2053|5532|5533|5641,%d&action[]=2053|5532|5533|5642,%d&action[]=2053|5532|5533|5643,%d" \
        "&ip=%s",
        sData.sReqResStatisticData.dwReqNum,sData.sReqResStatisticData.dwResSuccNum,sData.sReqResStatisticData.dwResFailNum,
		sData.sAllTimeStatisticData.dwRes10ms,sData.sAllTimeStatisticData.dwRes50ms,sData.sAllTimeStatisticData.dwRes250ms,
		sData.sAllTimeStatisticData.dwRes1s,sData.sAllTimeStatisticData.dwRes3s,sData.sAllTimeStatisticData.dwResOthers,
		sData.sHttpServerStatisticData.dwReqNum,sData.sHttpServerStatisticData.dwResSuccNum,sData.sHttpServerStatisticData.dwResFailNum,
		m_strIp.c_str());
	CHpsCohClientImp::GetInstance()->SendReportDataToMonitor(szReport);

	
	std::stringstream streamMsgAvnReqStatisticDataToMonitor;
	map<SServiceMsgId, SAvenueReqStatisticData>::iterator iter = m_mapAvnReqStatisticDataBySrvMsgId.begin();
	while(iter != m_mapAvnReqStatisticDataBySrvMsgId.end())
	{
		map<SServiceMsgId, SAvenueReqStatisticData>::iterator itrTmp = iter++;
		SServiceMsgId sKey = itrTmp->first;
		SAvenueReqStatisticData &sMsgData = itrTmp->second;
		SAvenueReqStatisticData &sOldMsgData = m_mapAvnReqStatisticOldDataBySrvMsgId[sKey];
		SAvenueReqStatisticData sMsgDataInterval = sMsgData - sOldMsgData;
		sOldMsgData = sMsgData;

		if(sMsgDataInterval == 0)
		{
			m_mapAvnReqStatisticDataBySrvMsgId.erase(itrTmp);
			m_mapAvnReqStatisticOldDataBySrvMsgId.erase(sKey);
			continue;
		}

		char szMsgInfo[64] = {0};
		snprintf(szMsgInfo, 63, "%u_%u", sKey.dwServiceID, sKey.dwMsgID);

		/*serviceId_msgId[%s], reqNum[request,responseSucc, responseFail], timeSpent[10ms, 50ms, 250ms, 1s, 5s, other]*/
		SRVMSG_AVENUE_REQUEST_STATISTIC_XLOG(XLOG_INFO,"serviceId_msgId[%s], reqNum[%u,%u,%u], allTimeSpent[%u, %u, %u, %u, %u, %u], httpServerTimeSpent[%u, %u, %u, %u, %u, %u]\n",
			szMsgInfo,
			sMsgDataInterval.sReqResStatisticData.dwReqNum,sMsgDataInterval.sReqResStatisticData.dwResSuccNum,sMsgDataInterval.sReqResStatisticData.dwResFailNum,
			sMsgDataInterval.sAllTimeStatisticData.dwRes10ms,sMsgDataInterval.sAllTimeStatisticData.dwRes50ms,sMsgDataInterval.sAllTimeStatisticData.dwRes250ms,
			sMsgDataInterval.sAllTimeStatisticData.dwRes1s,sMsgDataInterval.sAllTimeStatisticData.dwRes3s,sMsgDataInterval.sAllTimeStatisticData.dwResOthers,
			sMsgDataInterval.sHttpServerTimeStatisticData.dwRes10ms,sMsgDataInterval.sHttpServerTimeStatisticData.dwRes50ms,sMsgDataInterval.sHttpServerTimeStatisticData.dwRes250ms,
			sMsgDataInterval.sHttpServerTimeStatisticData.dwRes1s,sMsgDataInterval.sHttpServerTimeStatisticData.dwRes3s,sMsgDataInterval.sHttpServerTimeStatisticData.dwResOthers);


		streamMsgAvnReqStatisticDataToMonitor << "action[]=2053|5566|5571," << szMsgInfo << "," << sMsgDataInterval.sReqResStatisticData.dwReqNum
				<< "&action[]=2053|5573|5612," << szMsgInfo << "," << sMsgDataInterval.sReqResStatisticData.dwResSuccNum
				<< "&action[]=2053|5574|5609," << szMsgInfo << "," << sMsgDataInterval.sReqResStatisticData.dwResFailNum
				<< "&action[]=2053|5575|5606," << szMsgInfo << "," << sMsgDataInterval.sAllTimeStatisticData.dwRes10ms
				<< "&action[]=2053|5579|5603," << szMsgInfo << "," << sMsgDataInterval.sAllTimeStatisticData.dwRes50ms
				<< "&action[]=2053|5583|5600," << szMsgInfo << "," << sMsgDataInterval.sAllTimeStatisticData.dwRes250ms
				<< "&action[]=2053|5587|5597," << szMsgInfo << "," << sMsgDataInterval.sAllTimeStatisticData.dwRes1s
				<< "&action[]=2053|5591|5594," << szMsgInfo << "," << sMsgDataInterval.sAllTimeStatisticData.dwRes3s
				<< "&action[]=2053|5592|5615," << szMsgInfo << "," << sMsgDataInterval.sAllTimeStatisticData.dwResOthers
				
				<< "&action[]=2053|5617|5624," << szMsgInfo << "," << sMsgDataInterval.sHttpServerTimeStatisticData.dwRes10ms
				<< "&action[]=2053|5618|5627," << szMsgInfo << "," << sMsgDataInterval.sHttpServerTimeStatisticData.dwRes50ms
				<< "&action[]=2053|5619|5630," << szMsgInfo << "," << sMsgDataInterval.sHttpServerTimeStatisticData.dwRes250ms
				<< "&action[]=2053|5620|5633," << szMsgInfo << "," << sMsgDataInterval.sHttpServerTimeStatisticData.dwRes1s
				<< "&action[]=2053|5621|5636," << szMsgInfo << "," << sMsgDataInterval.sHttpServerTimeStatisticData.dwRes3s
				<< "&action[]=2053|5622|5639," << szMsgInfo << "," << sMsgDataInterval.sHttpServerTimeStatisticData.dwResOthers
				<< "&";
	}

	if(!(streamMsgAvnReqStatisticDataToMonitor.str().empty()))
	{
		streamMsgAvnReqStatisticDataToMonitor << "&ip=" << m_strIp;
		CHpsCohClientImp::GetInstance()->SendDetailReportDataToMonitor(streamMsgAvnReqStatisticDataToMonitor.str());
	}

	std::stringstream streamServiceStatisticsToMonitor;	
	map<unsigned int, SAvenueReqStatisticData>::iterator iterSrv = m_mapAvnReqStatisticDataByServiceId.begin();
	while(iterSrv != m_mapAvnReqStatisticDataByServiceId.end())
	{
		map<unsigned int, SAvenueReqStatisticData>::iterator iterTemp = iterSrv++;
		unsigned int dwServiceId = iterTemp->first;
		SAvenueReqStatisticData &sSrvData = iterTemp->second;
		SAvenueReqStatisticData &sOldSrvData = m_mapAvnReqStatisticOldDataByServiceId[dwServiceId];
		SAvenueReqStatisticData sSrvDataInterval = sSrvData - sOldSrvData;
		sOldSrvData = sSrvData;

		if(sSrvDataInterval == 0)
		{
			m_mapAvnReqStatisticDataByServiceId.erase(iterTemp);
			m_mapAvnReqStatisticOldDataByServiceId.erase(dwServiceId);
			continue;
		}
		
		/* write to disk */		
		SERVICE_AVENUE_REQUEST_STATISTIC_XLOG(XLOG_INFO,"serviceId[%u], reqNum[%u,%u,%u], timeSpent[%u, %u, %u, %u, %u, %u]\n",
			dwServiceId, 
			sSrvDataInterval.sReqResStatisticData.dwReqNum,sSrvDataInterval.sReqResStatisticData.dwResSuccNum,sSrvDataInterval.sReqResStatisticData.dwResFailNum,
			sSrvDataInterval.sAllTimeStatisticData.dwRes10ms,sSrvDataInterval.sAllTimeStatisticData.dwRes50ms,sSrvDataInterval.sAllTimeStatisticData.dwRes250ms,
			sSrvDataInterval.sAllTimeStatisticData.dwRes1s,sSrvDataInterval.sAllTimeStatisticData.dwRes3s,sSrvDataInterval.sAllTimeStatisticData.dwResOthers);

		/* report to monitor */		
		streamServiceStatisticsToMonitor << "action[]=2053|5545|5555," << dwServiceId << "," << sSrvDataInterval.sReqResStatisticData.dwReqNum
				<< "&action[]=2053|5546|5558," << dwServiceId << "," << sSrvDataInterval.sReqResStatisticData.dwResSuccNum
				<< "&action[]=2053|5547|5561," << dwServiceId << "," << sSrvDataInterval.sReqResStatisticData.dwResFailNum
				<< "&action[]=2053|5548|5564," << dwServiceId << "," << sSrvDataInterval.sAllTimeStatisticData.dwRes10ms
				<< "&action[]=2053|5549|5577," << dwServiceId << "," << sSrvDataInterval.sAllTimeStatisticData.dwRes50ms
				<< "&action[]=2053|5550|5581," << dwServiceId << "," << sSrvDataInterval.sAllTimeStatisticData.dwRes250ms
				<< "&action[]=2053|5551|5585," << dwServiceId << "," << sSrvDataInterval.sAllTimeStatisticData.dwRes1s
				<< "&action[]=2053|5552|5589," << dwServiceId << "," << sSrvDataInterval.sAllTimeStatisticData.dwRes3s
				<< "&action[]=2053|5553|5568," << dwServiceId << "," << sSrvDataInterval.sAllTimeStatisticData.dwResOthers
				<< "&";
	}
	if(!(streamServiceStatisticsToMonitor.str().empty()))
	{
		streamServiceStatisticsToMonitor << "&ip=" << m_strIp;
		CHpsCohClientImp::GetInstance()->SendDetailReportDataToMonitor(streamServiceStatisticsToMonitor.str());
	}
}


const string & CHpsAsyncLogThread::GetSelfCheckData()
{
	boost::mutex::scoped_lock sl(m_mutex);
	return m_strSelfCheck;
}

unsigned int CHpsAsyncLogThread::GetNoHttpResponseNum()
{
	boost::mutex::scoped_lock sl(m_mutexNoResNum);
	unsigned int dwNum = 0;
	map<int, unsigned int>::iterator itr;
	for(itr=m_mapNoHttpResNumByThreadId.begin(); itr!=m_mapNoHttpResNumByThreadId.end(); ++itr)
	{
		dwNum += itr->second;
	}

	return dwNum;
}

void CHpsAsyncLogThread::DoWarn()
{
	SS_XLOG(XLOG_DEBUG, "CHpsAsyncLogThread::%s\n", __FUNCTION__);
	if (m_setReConnIps.empty())
	{
		return;
	}
	string strMsg = "connect error IPs:";
	bool bFirst = true;
	set<string>::const_iterator itr;
	for (itr=m_setReConnIps.begin(); itr!=m_setReConnIps.end(); itr++)
	{
		if(bFirst)
		{
			strMsg += *itr;
			bFirst = false;
		}
		else
		{
			strMsg += "," + *itr;
		}
		
	}

	string strWarnInfo = "host="+ m_strIp + "&type=60075001&datanum=1&happen_time=" + GetCurrenStringTime() + "&msg=" + strMsg;
	CHpsCohClientImp::GetInstance()->SendErrorDataToMonitor(strWarnInfo);
	m_setReConnIps.clear();
}
