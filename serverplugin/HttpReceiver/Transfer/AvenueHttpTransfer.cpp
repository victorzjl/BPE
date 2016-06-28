#include "AvenueHttpTransfer.h"
#include "AvenueHttpTransferLogHelper.h"
#include "HpsCommon.h"
#include <boost/algorithm/string.hpp>

CAvenueHttpTransfer::CAvenueHttpTransfer(CHpsSessionManager *pManager):
	m_timerCleanRubbish(pManager,CLEAN_INTERVAL, boost::bind(&CAvenueHttpTransfer::CleanRubbish, this), CThreadTimer::TIMER_CIRCLE)
{
}

CAvenueHttpTransfer::~CAvenueHttpTransfer()
{
}

void CAvenueHttpTransfer::StartInThread()
{
	m_timerCleanRubbish.Start();
}

void CAvenueHttpTransfer::StopInThread()
{
    m_timerCleanRubbish.Stop();
}



int CAvenueHttpTransfer::DoLoadTransferObj(const string &strDirH2A, const string &strFilterH2A, const string &strDirA2H, const string &strFilterA2H)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, strDirH2A[%s], strFilterH2A[%s], strFilterH2A[%s], strFilterA2H[%s]\n",__FUNCTION__,
		strDirH2A.c_str(),strFilterH2A.c_str(), strDirA2H.c_str(),strFilterA2H.c_str());

	m_strRequestH2APluginDir = strDirH2A;
	m_strRequestA2HPluginDir = strDirA2H;
	
	int nRet1 = m_objH2ATransfer.DoLoadTransferObj(strDirH2A,strFilterH2A);
	/*int nRet2 = */m_objA2HTransfer.DoLoadTransferObj(strDirA2H,strFilterA2H);

	return nRet1/* + nRet2*/;
}


int CAvenueHttpTransfer::UnLoadTransferObj(const string &strDirPath, const vector<string> &vecSo)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, strDirPath[%s], vecSo.size[%d]\n",__FUNCTION__,strDirPath.c_str(),vecSo.size());

	vector<string>::const_iterator itr;
	for(itr=vecSo.begin();itr!=vecSo.end();itr++)
	{
		
	}
	if(0 == strDirPath.compare(m_strRequestH2APluginDir))
	{
		return m_objH2ATransfer.UnLoadTransferObj(strDirPath,vecSo);
	}
	else
	{
		TS_XLOG(XLOG_ERROR,"CAvenueHttpTransfer::%s, not support strDirPath[%s].. must be [%s] or [%s]\n",__FUNCTION__,strDirPath.c_str(),m_strRequestH2APluginDir.c_str(),m_strRequestA2HPluginDir.c_str());
		return -4;
	}

	return 0;
}

int CAvenueHttpTransfer::ReLoadTransferObj(const string &strDirPath, const vector<string> &vecSo)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, strDirPath[%s], vecSo.size[%d]\n",__FUNCTION__,strDirPath.c_str(),vecSo.size());

	if(0 == strDirPath.compare(m_strRequestH2APluginDir))
	{
		return m_objH2ATransfer.UnLoadTransferObj(strDirPath,vecSo);
	}
	else
	{
		TS_XLOG(XLOG_ERROR,"CAvenueHttpTransfer::%s, not support strDirPath[%s].. must be [%s] or [%s]\n",__FUNCTION__,strDirPath.c_str(),m_strRequestH2APluginDir.c_str(),m_strRequestA2HPluginDir.c_str());
		return -4;
	}

	return 0;
}

int CAvenueHttpTransfer::ReLoadCommonTransfer(const string &strTransferName)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s\n",__FUNCTION__);
	return m_objH2ATransfer.ReLoadCommonTransfer(strTransferName);
}

int CAvenueHttpTransfer::WaitForReload(int nPluginType, string &strReloadErorMsg)
{
	if(1 == nPluginType) //
	{
		return CReqHttp2AvenueTransfer::WaitForReload(strReloadErorMsg);
	}
	else
	{
		char szMsg[128] = {0};
		snprintf(szMsg, 127, "invalid type[%d], must be 1[HTTP2AvenueTransferSoType] or 2[Avenue2HTTPTransferSoType]", nPluginType);
		TS_XLOG(XLOG_ERROR,"CAvenueHttpTransfer::%s, %s\n",__FUNCTION__,szMsg);
		strReloadErorMsg = szMsg;
		return -4;
	}
}

int CAvenueHttpTransfer::SetReloadFlag(int nPluginType, string &strReloadErorMsg)
{
	if(1 == nPluginType)
	{
		return CReqHttp2AvenueTransfer::SetReloadFlag();
	}
	else
	{
		char szMsg[128] = {0};
		snprintf(szMsg, 127, "invalid type[%d], must be 1[HTTP2AvenueTransferSoType] or 2[Avenue2HTTPTransferSoType]", nPluginType);
		TS_XLOG(XLOG_ERROR,"CAvenueHttpTransfer::%s, %s\n",__FUNCTION__,szMsg);
		strReloadErorMsg = szMsg;
		return -4;
	}
}


string CAvenueHttpTransfer::GetPluginSoInfo()
{
	string strInfo = "{Request Http2Avenue transfer lib plugin directory[" + m_strRequestH2APluginDir + "]}<p>";
	strInfo += string("<table border='1' align='center'>");
	strInfo += string("<th>So Name</th><th>Version</th><th>Build Time</th>");
	strInfo += m_objH2ATransfer.GetPluginSoInfo();
	strInfo += string("</table><p>");

	strInfo += string("{Request Avenue2Http transfer lib plugin directory[") + m_strRequestA2HPluginDir + "]}<p>";
	strInfo += string("<table border='1' align='center'>");
	strInfo += string("<th>So Name</th><th>Version</th><th>Build Time</th>");
	strInfo += m_objA2HTransfer.GetPluginSoInfo();
	strInfo += string("</table><p>");
	return strInfo;
}









int CAvenueHttpTransfer::DoTransferRequestMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN unsigned int dwSequence, IN const char *szUriCommand, 
	IN const char *szUriAttribute, IN const char* szRemoteIp, IN const unsigned int dwRemotePort, OUT void *ppAvenuePacket , OUT int *pOutLen)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, url[%s] serviceId[%u]\n",__FUNCTION__,strUrlIdentify.c_str(),dwServiceId);
	return m_objH2ATransfer.DoTransferRequestMsg(strUrlIdentify,dwServiceId,dwMsgId,dwSequence,szUriCommand,szUriAttribute,szRemoteIp,dwRemotePort,ppAvenuePacket,pOutLen);
}

int CAvenueHttpTransfer::DoTransferResponseMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId,IN const void *pAvenuePacket,
		IN unsigned int nLen,OUT void *ppHttpPacket,OUT int * pOutLen)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, url[%s] serviceId[%u]\n",__FUNCTION__,strUrlIdentify.c_str(),dwServiceId);

	return m_objH2ATransfer.DoTransferResponseMsg(strUrlIdentify,dwServiceId,dwMsgId,pAvenuePacket,nLen,ppHttpPacket,pOutLen);
}

int CAvenueHttpTransfer::DoTransferRequestAvenue2Http(IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN const void * pAvenuePacket, IN int pAvenueLen, 
	IN const char* szRemoteIp, IN const unsigned int dwRemotePort,OUT string &strHttpUrl, OUT void * pHttpReqBody,OUT int *pHttpBodyLen, 
	OUT string &strHttpMethod, IN const void *pInReserve, IN int nInReverseLen,  	OUT void* pOutReverse, OUT int *pOutReverseLen)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, serviceId[%u], msgId[%u], pAvenueLen[%d], remoteIp[%s], remotePort[%u]\n",__FUNCTION__,dwServiceId,dwMsgId,pAvenueLen,szRemoteIp,dwRemotePort);

	return m_objA2HTransfer.RequestTransferAvenue2Http(dwServiceId,dwMsgId,pAvenuePacket, pAvenueLen,szRemoteIp, dwRemotePort, 
				strHttpUrl,pHttpReqBody,pHttpBodyLen,strHttpMethod,pInReserve,nInReverseLen,pOutReverse,pOutReverseLen);
}
    	
int CAvenueHttpTransfer::DoTransferResponseHttp2Avenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequence,IN int nAvenueCode,IN int nHttpCode, IN const string &strHttpResponseBody,
    	OUT void * pAvenueResponsePacket, OUT int *pAvenueLen, IN const void *pInReserve, IN int nInReverseLen, OUT void* pOutReverse, OUT int *pOutReverseLen)
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s, serviceId[%u], msgId[%u],pHttpResponseBody[%s]\n",__FUNCTION__,dwServiceId,dwMsgId,strHttpResponseBody.c_str());

	return m_objA2HTransfer.ResponseTransferHttp2Avenue(dwServiceId, dwMsgId, dwSequence,nAvenueCode,nHttpCode,strHttpResponseBody,
					pAvenueResponsePacket,pAvenueLen,pInReserve,nInReverseLen,pOutReverse,pOutReverseLen);
}



void CAvenueHttpTransfer::CleanRubbish()
{
	TS_XLOG(XLOG_DEBUG,"CAvenueHttpTransfer::%s\n",__FUNCTION__);
	m_objH2ATransfer.CleanRubbish();
	m_objA2HTransfer.CleanRubbish();

}

SUrlInfo CAvenueHttpTransfer::GetSoNameByUrl(const string& strUrl)
{
	return m_objH2ATransfer.GetSoNameByUrl(strUrl);
}

void CAvenueHttpTransfer::Dump()
{
	TS_XLOG(XLOG_NOTICE,"============CAvenueHttpTransfer::%s ==============\n",__FUNCTION__);
	m_objH2ATransfer.Dump();
	m_objA2HTransfer.Dump();

	TS_XLOG(XLOG_NOTICE,"============CAvenueHttpTransfer::%s  end==============\n",__FUNCTION__);
}


