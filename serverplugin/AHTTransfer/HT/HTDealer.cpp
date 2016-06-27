#include "HTDealer.h"
#include "HTDealerServiceLog.h"
#include "MessageReceive.h"
#include "Common.h"
#include <SapMessage.h>
#include <arpa/inet.h>
#include "HTInfo.h"
#include "SoPluginLog.h"

//class CMessageReceive;

using std::stringstream;
using namespace sdo::sap;
using namespace HT;

IAsyncVirtualService * create()
{
	return new CHTDealer;
}

void destroy( IAsyncVirtualService *pVirtualService )
{
	if(pVirtualService != NULL)
	{
		delete pVirtualService;
		pVirtualService = NULL;
	}
}

CHTDealer::CHTDealer():
	m_pResponseCallBack(NULL), m_pExceptionWarn(NULL), m_pAsyncLog(NULL),m_pWarnCondition(NULL)
{

#ifdef _WIN32
	XLOG_INIT("log.properties");
#endif

	//注册日志模块
	XLOG_REGISTER(HT_MODULE, "ht");
	XLOG_REGISTER(HT_ASYNCLOG_MODULE, "async_http");
	XLOG_REGISTER(SOPLUGIN_BUSINESS_MODULE, "so_plugin");
	
	char szLocalAddr[16] = {0};
	GetLocalAddress(szLocalAddr);
	m_strLocalIp = szLocalAddr;
	
}

CHTDealer::~CHTDealer()
{
	if(m_pWarnCondition != NULL)
	{
		delete m_pWarnCondition;
		m_pWarnCondition = NULL;
	}

	if(m_pAvenueServiceConfigs != NULL)
	{
		delete m_pAvenueServiceConfigs;
		m_pAvenueServiceConfigs = NULL;
	}
	
}

int CHTDealer::Initialize(ResponseService funcResponseService, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog)
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s\n", __FUNCTION__);
	
	if(m_pResponseCallBack)
	{
		HT_XLOG(XLOG_ERROR, "CHTDealer::%s, ResponseCallBack have already setting\n", __FUNCTION__);
		return HT_NO_CALLBACK_FUNC;
	}
	
	m_pExceptionWarn = funcExceptionWarn;
	m_pResponseCallBack = funcResponseService;
	m_pAsyncLog = funcAsyncLog;
	
	//加载业务配置文件中的相关项
	m_pAvenueServiceConfigs = new CAvenueServiceConfigs;
	m_pAvenueServiceConfigs->LoadAvenueServiceConfigs("./avenue_conf",m_vecArrServiceIds);
	
	vector<int>::iterator itr = m_vecArrServiceIds.begin();
	for(;itr != m_vecArrServiceIds.end();++itr)
	{
		m_vecServiceIds.insert(*itr);	
	}
	
	//UNNECESSARY!!
    //创建CMessageReceive对象,以便提前进行插件加载	
	CMessageReceive::GetInstance();
	return 0;	
}

int CHTDealer::RequestService(IN void *pOwner, IN const void *pBuffer, IN int len)
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s, pOwner[%x] Buffer[%x], Len[%d]\n",__FUNCTION__,pOwner, pBuffer, len);
	
	SSapMsgHeader *pSapMsgHeader = (SSapMsgHeader *)pBuffer;
	unsigned int dwServiceId = ntohl(pSapMsgHeader->dwServiceId);
	unsigned int dwMsgId = ntohl(pSapMsgHeader->dwMsgId);
	unsigned int dwSequenceId = ntohl(pSapMsgHeader->dwSequence);
	

	timeval_a tStart;
	gettimeofday_a(&tStart, 0);

	//获取GUID
	string strGuid;
	unsigned char *pExtBuffer = (unsigned char *)pBuffer + sizeof(SSapMsgHeader);
	int nExtLen = pSapMsgHeader->byHeadLen-sizeof(SSapMsgHeader);
	if(nExtLen > 0)
	{
		CSapTLVBodyDecoder objExtHeaderDecoder(pExtBuffer, nExtLen);
		objExtHeaderDecoder.GetValue(GUID_CODE, strGuid);
	}
	
	//判断服务号是否存在
	set<unsigned int>::iterator iter = m_vecServiceIds.find(dwServiceId);
	if(iter == m_vecServiceIds.end())
	{
		timeval_a tEnd;
		gettimeofday_a(&tEnd, 0);
		HT_XLOG(XLOG_ERROR, "CHTDealer::%s, ServiceId[%d] not supported\n", __FUNCTION__, dwServiceId);
		Log(tStart, tEnd,strGuid, "", dwServiceId, dwMsgId,"", 0,"", HT_UNSUPPORT_SERVICE, "");
		
		return HT_UNSUPPORT_SERVICE;
	}
	
	CMessageReceive::GetInstance()->OnProcess(dwServiceId, dwMsgId, dwSequenceId, strGuid, tStart, pOwner, pBuffer, len, this);
	
	return 0;	
}

void CHTDealer::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
	
	unsigned int tempAliveNum = 0;
	unsigned int tempAllNum = 0;
	
	CMessageReceive::GetInstance()->GetSelfCheck(tempAliveNum, tempAllNum);
	dwAliveNum = tempAliveNum;
	dwAllNum = tempAllNum;
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s dwAliveNum[%d]  dwAllNum[%d]\n", __FUNCTION__, dwAliveNum, dwAllNum);
	
}

const std::string CHTDealer::OnGetPluginInfo() const
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s\n", __FUNCTION__);
	return string("<tr><td>libht.so</td><td>")
		+ HT_VERSION
		+	"</td><td>"
		+ __TIME__
		+ " " 
		+ __DATE__
		+ "</td></tr>";
}

void CHTDealer::Response(void *handler, const void *pBuffer, unsigned int dwLen)
{
	if(m_pResponseCallBack == NULL)
	{
		HT_XLOG(XLOG_ERROR, "CHTDealer::%s, Response CallBackFunc not setting\n", __FUNCTION__);
		return;
	}
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s handler[%x] pBuffer[%x] dwLen[%d]\n",__FUNCTION__, handler,pBuffer,dwLen );
	m_pResponseCallBack(handler, pBuffer, dwLen);
	
}

void CHTDealer::Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode ,const void * pBuffer, unsigned int dwLen)
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetBody(pBuffer, dwLen);
	Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CHTDealer::Warn(const string &strWarnInfo)
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s, WarnInfo[%s]\n", __FUNCTION__, strWarnInfo.c_str());
	if(m_pExceptionWarn == NULL)
	{
		HT_XLOG(XLOG_ERROR, "CHTDealer::%s, Warn CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pExceptionWarn(strWarnInfo);
}

void CHTDealer::GetServiceId( vector<unsigned int> &vecServiceIds )
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s\n", __FUNCTION__);
	
	vecServiceIds.clear();
	set<unsigned int>::iterator itr;
	for( itr = m_vecServiceIds.begin(); itr != m_vecServiceIds.end(); itr++)
	{
		vecServiceIds.push_back(*itr);
		
	}
	

	vector<unsigned int>::const_iterator iter;
	string log;
	for(iter = vecServiceIds.begin(); iter != vecServiceIds.end(); iter++)
	{
		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d  ", *iter);
		log += szTemp;
	}
	
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s, serviceId[%s]\n", __FUNCTION__, log.c_str());
}

void CHTDealer::Log(const timeval_a &tStart,const timeval_a &tEnd, const string &strGuid, const string &serveraddr, unsigned int dwServiceId, unsigned int dwMsgId, 
					const string &strIpAddrs, float fSpentHttpTime,const string &requestS,int nCode,const string &responseS )
{
	HT_XLOG(XLOG_DEBUG, "CHTDealer::%s, Guid[%s], ServiceId[%d], MsgId[%d], Ip[%s]\n", __FUNCTION__, strGuid.c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str());
	
	struct timeval_a interval;	
	timersub(&tEnd,&tStart,&interval);
	float dwSpent = GetTimeInterval(tStart);	
	float spendTotal = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;	
	string strTempValue;
	
	if(responseS.empty())
	{
		strTempValue += VALUE_SPLITTER;
		strTempValue += VALUE_SPLITTER;
	}
	else
	{
		strTempValue = responseS;
	}

	char szLogBuffer[4096] = {0};
	snprintf(szLogBuffer, sizeof(szLogBuffer)-1, "%s,  %s,	%s,	%d,	%d,  %s,  %f,  %f,  %s,  %d,  %s\n",
				TimevalToString(tStart).c_str(), strGuid.c_str(),serveraddr.c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str(), spendTotal,fSpentHttpTime ,requestS.c_str(),nCode,strTempValue.c_str());

	m_pAsyncLog(HT_ASYNCLOG_MODULE, XLOG_INFO, (unsigned int)dwSpent, szLogBuffer, nCode, m_strLocalIp, dwServiceId, dwMsgId);
}


