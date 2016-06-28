#include "CacheVirtualService.h"
#include "AsyncVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "CacheThreadGroup.h"
#include "CacheWriteThreadGroup.h"
#include "CacheReConnThread.h"
#include "AvenueServiceConfigs.h"
#include <XmlConfigParser.h>
#include "Common.h"
#include "CommonMacro.h"
#include <iostream>
#include <sstream>

#include <SapMessage.h>

#include "AsyncVirtualServiceLog.h"
#include "WarnCondition.h"

using std::stringstream;
using namespace sdo::sap;

IAsyncVirtualService * create()
{
	return new CCacheVirtualService;
}

void destroy( IAsyncVirtualService *pVirtualService )
{
	if(pVirtualService != NULL)
	{
		delete pVirtualService;
		pVirtualService = NULL;
	}
}

CCacheVirtualService::CCacheVirtualService():
	m_pResponseCallBack(NULL), m_pExceptionWarn(NULL),m_pAsyncLog(NULL), 
	m_pCacheThreadGroup(NULL), m_pCacheWriteThreadGroup(NULL), m_pCacheReConnThread(NULL),m_pWarnCondition(NULL)
{
#ifdef _WIN32
	XLOG_INIT("log.properties");
#endif

	//注册日志模块
	XLOG_REGISTER(CVS_MODULE, "cvs");
	XLOG_REGISTER(CVS_ASYNCLOG_MODULE, "async_cvs");

	char szLocalAddr[16] = {0};
	GetLocalAddress(szLocalAddr);
	m_strLocalIp = szLocalAddr;
}

CCacheVirtualService::~CCacheVirtualService()
{
	if(m_pCacheThreadGroup)
	{
		m_pCacheThreadGroup->Stop();
		delete m_pCacheThreadGroup;
		m_pCacheThreadGroup = NULL;
	}

	if(m_pCacheWriteThreadGroup)
	{
		m_pCacheWriteThreadGroup->Stop();
		delete m_pCacheWriteThreadGroup;
		m_pCacheWriteThreadGroup = NULL;
	}

	if(m_pCacheReConnThread)
	{
		m_pCacheReConnThread->Stop();
		delete m_pCacheReConnThread;
		m_pCacheReConnThread = NULL;
	}

	if(m_pWarnCondition)
	{
		delete m_pWarnCondition;
		m_pWarnCondition = NULL;
	}
}

int CCacheVirtualService::Initialize( ResponseService funcResponseService, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s\n", __FUNCTION__);
	if(m_pResponseCallBack)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, ResponseCallBack have already setting\n", __FUNCTION__);
		return CVS_SYSTEM_ERROR;
	}

	m_pExceptionWarn = funcExceptionWarn;
	m_pResponseCallBack = funcResponseService;
	m_pAsyncLog = funcAsyncLog;
	
	int nRet = 0;

	//加载CacheMonitor.xml
	m_pWarnCondition = new CWarnCondition;
	m_pWarnCondition->Init("./CacheMonitor.xml");

	//加载Cache业务配置文件中的相关项
	CAvenueServiceConfigs oServiceConfigs;
	oServiceConfigs.LoadAvenueServiceConfigs("./avenue_conf");

	map<unsigned int , SServiceConfig> mapServiceConfigById = oServiceConfigs.GetServiceConfigByIdMap();
	map<unsigned int , SServiceConfig>::iterator iterServiceConfig;
	for (iterServiceConfig = mapServiceConfigById.begin(); iterServiceConfig != mapServiceConfigById.end(); iterServiceConfig++)
	{
		m_vecServiceIds.push_back(iterServiceConfig->first);

		SServiceConfig &objServiceConfig = iterServiceConfig->second;
		map<int, SConfigType> mapTypeByCode = objServiceConfig.oConfig.GetTypeByCodeMap();
		m_mapClearText[iterServiceConfig->first]=objServiceConfig.oConfig.IsClearText();
		m_mapKeyPrefix[iterServiceConfig->first]=objServiceConfig.oConfig.GetKeyPrefix();
		
		map<int, SConfigType>::iterator iterConfitType;
		TYPE_CODE_MAP mapNewTypeByCode; 
		for (iterConfitType = mapTypeByCode.begin(); iterConfitType != mapTypeByCode.end(); iterConfitType++)
		{
			mapNewTypeByCode.insert(make_pair(iterConfitType->first, iterConfitType->second.eType));
			if(iterConfitType->first == 10000)
			{
				m_mapCacheKeepTime.insert(make_pair(iterServiceConfig->first, iterConfitType->second.nDefaultValue));
			}
		}

		m_mapCodeTypeByService.insert(make_pair(iterServiceConfig->first, mapNewTypeByCode));
	}

	//Dump();

	//读取业务配置文件
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile("./config.xml") != 0)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, Config Parser fail, file[./config.xml], error[%s]\n", __FUNCTION__, objConfigParser.GetErrorMessage().c_str());
		return CVS_CACHE_CONFIG_ERROR;
	}

	unsigned int dwThreadNum = objConfigParser.GetParameter("CacheThreadNum", 3);
	unsigned int dwWriteThreadNum = objConfigParser.GetParameter("CacheWriteThreadNum", 0);
	CVS_XLOG(XLOG_INFO, "CCacheVirtualService::%s, ThreadNum[%d], WriteThreadNum[%d]\n", __FUNCTION__, dwThreadNum, dwWriteThreadNum);

	m_dwThreadNum = dwThreadNum;
	m_dwWriteThreadNum = dwWriteThreadNum;
	
	vector<string> vecCacheSvr = objConfigParser.GetParameters("CacheSosList");

	m_pCacheReConnThread = new CCacheReConnThread;
	if(m_pCacheReConnThread == NULL)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, New Space CacheReConnThread Error\n", __FUNCTION__);
		return CVS_NEW_SPACE_ERROR;
	}
	m_pCacheReConnThread->Start(this);

	m_pCacheThreadGroup = new CCacheThreadGroup;
	if(m_pCacheThreadGroup == NULL)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, New Space CacheThreadGroup Error\n", __FUNCTION__);
		return CVS_NEW_SPACE_ERROR;
	}
	nRet = m_pCacheThreadGroup->Start(dwThreadNum, m_mapCodeTypeByService, m_mapCacheKeepTime, vecCacheSvr, this, m_mapClearText, m_mapKeyPrefix);
	if(nRet != 0)
	{
		return nRet;
	}

	if (m_dwWriteThreadNum>0)
	{
		m_pCacheWriteThreadGroup = new CCacheWriteThreadGroup;
		if(m_pCacheWriteThreadGroup == NULL)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, New Space CacheWriteThreadGroup Error\n", __FUNCTION__);
			return CVS_NEW_SPACE_ERROR;
		}
		nRet = m_pCacheWriteThreadGroup->Start(dwWriteThreadNum, m_mapCodeTypeByService, m_mapCacheKeepTime, vecCacheSvr, this, m_mapClearText, m_mapKeyPrefix);
		if(nRet != 0)
		{
			return nRet;
		}
	}
	
	Dump();

	return 0;
}

int CCacheVirtualService::RequestService( IN void *pOwner, IN const void *pBuffer, IN int len )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s, Buffer[%x], Len[%d]\n",__FUNCTION__, pBuffer, len);

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

	SERVICE_TYPE_CODE_MAP::iterator iter = m_mapCodeTypeByService.find(dwServiceId);
	if(iter == m_mapCodeTypeByService.end())
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, ServiceId[%d] not supported\n", __FUNCTION__, dwServiceId);
		Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, CVS_UNKNOWN_SERVICE, "");
		Response(pOwner, dwServiceId, dwMsgId, dwSequenceId, CVS_UNKNOWN_SERVICE);
		return CVS_UNKNOWN_SERVICE;
	}

	if (m_dwWriteThreadNum>0)
	{
		if(dwMsgId != CVS_CACHE_SET)
		{
			m_pCacheThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, strGuid, pOwner, pBuffer, len);
		}
		else
		{
			m_pCacheWriteThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, strGuid, pOwner, pBuffer, len);
		}
	}
	else
	{
		m_pCacheThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, strGuid, pOwner, pBuffer, len);
	}
	
	return 0;
}

void CCacheVirtualService::GetServiceId( vector<unsigned int> &vecServiceIds )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s\n", __FUNCTION__);
	vecServiceIds = m_vecServiceIds;
}


const std::string CCacheVirtualService::OnGetPluginInfo() const
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s\n", __FUNCTION__);
	return string("<tr><td>libcache.so</td><td>")
		+ CVS_VERSION
		+	"</td><td>"
		+ __TIME__
		+ " " 
		+ __DATE__
		+ "</td></tr>";
}

void CCacheVirtualService::GetSelfCheckState( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s\n", __FUNCTION__);
	unsigned int tempAliveNum = 0;
	unsigned int tempAllNum = 0;

	m_pCacheThreadGroup->GetSelfCheck(tempAliveNum, tempAllNum);
	dwAliveNum = tempAliveNum;
	dwAllNum = tempAllNum;

	m_pCacheWriteThreadGroup->GetSelfCheck(tempAliveNum, tempAllNum);
	dwAliveNum += tempAliveNum;
	dwAllNum += tempAllNum;
}

void CCacheVirtualService::Response( void *handler, const void *pBuffer, unsigned int dwLen )
{
	if(m_pResponseCallBack == NULL)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, Response CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pResponseCallBack(handler, pBuffer, dwLen);
}

void CCacheVirtualService::Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);
	Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CCacheVirtualService::Warn(const string &strWarnInfo)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s, WarnInfo[%s]\n", __FUNCTION__, strWarnInfo.c_str());
	if(m_pExceptionWarn == NULL)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheVirtualService::%s, Warn CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pExceptionWarn(strWarnInfo);
}

void CCacheVirtualService::Log(const string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, const timeval_a &tStart, const string &strKey, const string &strValue, int nKeepTime, int nCode, const string &strIpAddrs)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheVirtualService::%s, Guid[%s], ServiceId[%d], MsgId[%d], Ip[%s]\n", __FUNCTION__, strGuid.c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str());
	float dwSpent = GetTimeInterval(tStart);
	string strTempValue;
	if(strValue.empty())
	{
		strTempValue += VALUE_SPLITTER;
		strTempValue += VALUE_SPLITTER;
	}
	else
	{
		strTempValue = strValue;
	}

	char szLogBuffer[2048] = {0};
	snprintf(szLogBuffer, sizeof(szLogBuffer)-1, "%s,  %s,  %d,  %d,  %s,  %s, %d,  %s,  %f,  %d\n",
				strGuid.c_str(), TimevalToString(tStart).c_str(), dwServiceId, dwMsgId, strKey.c_str(), strTempValue.c_str(), nKeepTime, strIpAddrs.c_str(), dwSpent, nCode);

	m_pAsyncLog(CVS_ASYNCLOG_MODULE, XLOG_INFO, (unsigned int)dwSpent, szLogBuffer, nCode, m_strLocalIp, dwServiceId, dwMsgId);
}


void CCacheVirtualService::Dump()
{
	CVS_XLOG(XLOG_DEBUG, "############begin dump############\n");
	SERVICE_TYPE_CODE_MAP::iterator iter;
	for (iter = m_mapCodeTypeByService.begin(); iter != m_mapCodeTypeByService.end(); iter++)
	{
		CVS_XLOG(XLOG_DEBUG, "ServiceId :	%6d\n", iter->first);

		TYPE_CODE_MAP::iterator iterTemp;
		for (iterTemp = iter->second.begin(); iterTemp != iter->second.end(); iterTemp++)
		{
			CVS_XLOG(XLOG_DEBUG, "Code : %5d,	Type : %d\n", iterTemp->first, iterTemp->second);
		}
	}

	CACHE_SET_TIME_MAP::const_iterator itrCacheKeepTime;
	for (itrCacheKeepTime=m_mapCacheKeepTime.begin(); itrCacheKeepTime!=m_mapCacheKeepTime.end(); itrCacheKeepTime++)
	{
		CVS_XLOG(XLOG_FATAL, "ServiceId:%d, KeepTime:%d\n", itrCacheKeepTime->first, itrCacheKeepTime->second);
	}
	CVS_XLOG(XLOG_DEBUG, "############end dump############\n");
}
