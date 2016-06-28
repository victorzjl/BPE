#include "RedisVirtualService.h"
#include "RedisVirtualServiceLog.h"
#include "Common.h"
#include "ErrorMsg.h"
#include "RedisReadThreadGroup.h"
#include "RedisWriteThreadGroup.h"
#include "RedisReConnThread.h"
#include "RedisStringMsgOperator.h"
#include "RedisSetMsgOperator.h"
#include "RedisListMsgOperator.h"
#include "RedisHashMsgOperator.h"
#include "RedisZsetMsgOperator.h"

#include <XmlConfigParser.h>
#include <SapMessage.h>

using std::stringstream;
using namespace sdo::sap;
using namespace redis;

IAsyncVirtualService * create()
{
	return new CRedisVirtualService;
}

void destroy( IAsyncVirtualService *pVirtualService )
{
	if(pVirtualService != NULL)
	{
		delete pVirtualService;
		pVirtualService = NULL;
	}
}


CRedisVirtualService::CRedisVirtualService():
	m_pResponseCallBack(NULL), m_pExceptionWarn(NULL), m_pAsyncLog(NULL), m_bDispatchReadRequest(false),
	m_pRedisReadThreadGroup(NULL), m_pRedisWriteThreadGroup(NULL), m_pRedisReConnThread(NULL),m_pWarnCondition(NULL)
{
#ifdef _WIN32
	XLOG_INIT("log.properties");
#endif

	//注册日志模块
	XLOG_REGISTER(RVS_MODULE, "rvs");
	XLOG_REGISTER(RVS_ASYNCLOG_MODULE, "async_rvs");

	char szLocalAddr[16] = {0};
	GetLocalAddress(szLocalAddr);
	m_strLocalIp = szLocalAddr;

	unsigned long readBit = 0;
	//string 读标志位设置
	readBit = (1 << RVS_STRING_GET) | (1<<RVS_STRING_GETKEY) | (1<<RVS_STRING_TTL) | (1 << RVS_STRING_DECODE);
	readBit = readBit | (1 << RVS_STRING_BATCH_QUERY);
	m_mapReadOperateByRedisType.insert(make_pair(RVS_REDIS_STRING,readBit));
    //set  读标志位设置
	readBit = (1 << RVS_SET_HASMEMBER) | (1 << RVS_SET_SIZE) | (1 << RVS_SET_GETALL) | (1 << RVS_SET_GETKEY) | (1 << RVS_SET_TTL);
	readBit = readBit | (1 << RVS_SET_DECODE);
	m_mapReadOperateByRedisType.insert(make_pair(RVS_REDIS_SET,readBit));
	//zset读标志位设置
	readBit = (1<<RVS_ZSET_GET) | (1<<RVS_ZSET_SIZE) | (1<<RVS_ZSET_GETALL) | (1<<RVS_ZSET_GETREVERSEALL) | (1<<RVS_ZSET_GETSCOREALL);
	readBit = readBit | (1<<RVS_ZSET_RANK) | (1<<RVS_ZSET_REVERSERANK) | (1<<RVS_ZSET_GETKEY) | (1<<RVS_ZSET_TTL);
	readBit = readBit | (1 << RVS_ZSET_DECODE);
	m_mapReadOperateByRedisType.insert(make_pair(RVS_REDIS_ZSET,readBit));
	//list 读标志位设置
	readBit = (1 << RVS_LIST_GET) | (1 << RVS_LIST_SIZE) | (1 << RVS_LIST_GETALL) | (1 << RVS_LIST_GETKEY) | (1 << RVS_LIST_TTL);
	readBit = readBit | (1 << RVS_LIST_DECODE);
	m_mapReadOperateByRedisType.insert(make_pair(RVS_REDIS_LIST,readBit));
    //hash 读标志位设置
	readBit = (1 << RVS_HASH_GET) | (1 << RVS_HASH_SIZE) | (1 << RVS_HASH_GETALL) | (1 << RVS_HASH_GETKEY) | (1 << RVS_HASH_TTL);
	readBit = readBit | (1 << RVS_HASH_DECODE) | (1<< RVS_HASH_BATCH_QUERY);
	m_mapReadOperateByRedisType.insert(make_pair(RVS_REDIS_HASH,readBit));
}

CRedisVirtualService::~CRedisVirtualService()
{
    if(m_pRedisReadThreadGroup != NULL)
	{
		m_pRedisReadThreadGroup->Stop();
		delete m_pRedisReadThreadGroup;
		m_pRedisReadThreadGroup = NULL;
	}

	if(m_pRedisWriteThreadGroup != NULL)
	{
		m_pRedisWriteThreadGroup->Stop();
		delete m_pRedisWriteThreadGroup;
		m_pRedisWriteThreadGroup = NULL;
	}

	if(m_pRedisReConnThread != NULL)
	{
		m_pRedisReConnThread->Stop();
		delete m_pRedisReConnThread;
		m_pRedisReConnThread = NULL;
	}

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

int CRedisVirtualService::Initialize(ResponseService funcResponseService,ExceptionWarn funcExceptionWarn,AsyncLog funcAsyncLog)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s\n", __FUNCTION__);
	
	if(m_pResponseCallBack)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, ResponseCallBack have already setting\n", __FUNCTION__);
		return RVS_SYSTEM_ERROR;
	}
	
	m_pExceptionWarn = funcExceptionWarn;
	m_pResponseCallBack = funcResponseService;
	m_pAsyncLog = funcAsyncLog;
	
	//加载RedisMonitor.xml
	m_pWarnCondition = new CWarnCondition;
	m_pWarnCondition->Init("./RedisMonitor.xml");

	
	//加载Redis业务配置文件中的相关项
	m_pAvenueServiceConfigs = new CAvenueServiceConfigs;
	m_pAvenueServiceConfigs->LoadAvenueServiceConfigs("./avenue_conf");
	
	map<unsigned int , SServiceConfig> & mapServiceConfigById = m_pAvenueServiceConfigs->GetServiceConfigByIdMap();
	map<unsigned int , SServiceConfig>::iterator iterServiceConfig;
	for (iterServiceConfig = mapServiceConfigById.begin(); iterServiceConfig != mapServiceConfigById.end(); iterServiceConfig++)
	{
		m_vecServiceIds.push_back(iterServiceConfig->first);
	
		SServiceConfig &objServiceConfig = iterServiceConfig->second;
		map<int, SConfigType> mapTypeByCode = objServiceConfig.oConfig.GetTypeByCodeMap();
		map<int, SConfigType>::iterator iterConfitType;
		CODE_TYPE_MAP mapNewTypeByCode; 
		for (iterConfitType = mapTypeByCode.begin(); iterConfitType != mapTypeByCode.end(); iterConfitType++)
		{
			mapNewTypeByCode.insert(make_pair(iterConfitType->first, iterConfitType->second.eType));
			if(iterConfitType->first == 10000)
			{
				m_mapRedisKeepTime.insert(make_pair(iterServiceConfig->first, iterConfitType->second.nDefaultValue));
			}
		}
	
		m_mapCodeTypeByService.insert(make_pair(iterServiceConfig->first, mapNewTypeByCode));
	}

	//读取业务配置文件
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile("./config.xml") != 0)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, Config Parser fail, file[./config.xml], error[%s]\n", __FUNCTION__, objConfigParser.GetErrorMessage().c_str());
		return RVS_REDIS_CONFIG_ERROR;
	}

	unsigned int dwReadThreadNum = objConfigParser.GetParameter("RedisReadThreadNum", 0);
	if(dwReadThreadNum > 0)
	{
		m_bDispatchReadRequest = true;
	}
	unsigned int dwWriteThreadNum = objConfigParser.GetParameter("RedisWriteThreadNum", 3);
	short readPolicy = objConfigParser.GetParameter("RedisReadPolicy", 0);
	RVS_XLOG(XLOG_INFO, "CRedisVirtualService::%s, ReadThreadNum[%d], WriteThreadNum[%d], ReadPolicy[%d]\n", __FUNCTION__, dwReadThreadNum, dwWriteThreadNum, readPolicy);

	vector<string> vecRedisSvr = objConfigParser.GetParameters("RedisSosList");

	m_pRedisReConnThread = new CRedisReConnThread;
	if(m_pRedisReConnThread == NULL)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, New Space RedisReConnThread Error\n", __FUNCTION__);
		return RVS_NEW_SPACE_ERROR;
	}
	m_pRedisReConnThread->Start(this);

	int nRet = 0;
	//读取线程数大于0说明要读写分离
    if(m_bDispatchReadRequest)
    {
    	m_pRedisReadThreadGroup = new CRedisReadThreadGroup;
	    if(m_pRedisReadThreadGroup == NULL)
	    {
		    RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, New Space RedisReadThreadGroup Error\n", __FUNCTION__);
		    return RVS_NEW_SPACE_ERROR;
	    }
	
	    nRet = m_pRedisReadThreadGroup->Start(dwReadThreadNum, vecRedisSvr, this, readPolicy);
	    if(nRet != 0)
	    {
		    return nRet;
	    }
    }
	
	m_pRedisWriteThreadGroup = new CRedisWriteThreadGroup;
	if(m_pRedisWriteThreadGroup == NULL)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, New Space RedisWriteThreadGroup Error\n", __FUNCTION__);
		return RVS_NEW_SPACE_ERROR;
	}
	nRet = m_pRedisWriteThreadGroup->Start(dwWriteThreadNum, vecRedisSvr, this);
	if(nRet != 0)
	{
		return nRet;
	}

	Dump();

	return 0;

}

int CRedisVirtualService::RequestService( IN void *pOwner, IN const void *pBuffer, IN int len )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s, Buffer[%x], Len[%d]\n",__FUNCTION__, pBuffer, len);

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

	SERVICE_CODE_TYPE_MAP::iterator iter = m_mapCodeTypeByService.find(dwServiceId);
	if(iter == m_mapCodeTypeByService.end())
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, ServiceId[%d] not supported\n", __FUNCTION__, dwServiceId);
		Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_SERVICE, "", 0);
		
		return RVS_UNKNOWN_SERVICE;
	}

	SServiceConfig & serviceConfig = m_pAvenueServiceConfigs->GetServiceConfigByIdMap()[dwServiceId];
	ERedisType redisType = serviceConfig.oConfig.GetRedisType();
	if(m_bDispatchReadRequest)
	{
	    //分派读写操作
		unsigned long readBit = m_mapReadOperateByRedisType[redisType];
	    unsigned long requetOperate = (1 << dwMsgId);
	    if((readBit & requetOperate) != 0)
	    {
		    m_pRedisReadThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, redisType, strGuid, pOwner, pBuffer, len);
	    }
	    else
	    {
		    m_pRedisWriteThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, redisType, strGuid, pOwner, pBuffer, len);
	    }
	}
	else
	{
		m_pRedisWriteThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, redisType, strGuid, pOwner, pBuffer, len);
	}

	return 0;
}

void CRedisVirtualService::GetServiceId( vector<unsigned int> &vecServiceIds )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s\n", __FUNCTION__);
	vecServiceIds = m_vecServiceIds;

	vector<unsigned int>::const_iterator iter;
	string log;
	for(iter = vecServiceIds.begin(); iter != vecServiceIds.end(); iter++)
	{
		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d  ", *iter);
		log += szTemp;
	}
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s, serviceId[%s]\n", __FUNCTION__, log.c_str());
}


const std::string CRedisVirtualService::OnGetPluginInfo() const
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s\n", __FUNCTION__);
	return string("<tr><td>libredis.so</td><td>")
		+ RVS_VERSION
		+	"</td><td>"
		+ __TIME__
		+ " " 
		+ __DATE__
		+ "</td></tr>";
}

void CRedisVirtualService::GetSelfCheckState( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s\n", __FUNCTION__);
	unsigned int tempAliveNum = 0;
	unsigned int tempAllNum = 0;

    if(m_bDispatchReadRequest)
   	{
    	m_pRedisReadThreadGroup->GetSelfCheck(tempAliveNum, tempAllNum);
	    dwAliveNum = tempAliveNum;
	    dwAllNum = tempAllNum;
    }
	
	m_pRedisWriteThreadGroup->GetSelfCheck(tempAliveNum, tempAllNum);
	dwAliveNum += tempAliveNum;
	dwAllNum += tempAllNum;
}

void CRedisVirtualService::Response( void *handler, const void *pBuffer, unsigned int dwLen )
{
	if(m_pResponseCallBack == NULL)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, Response CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pResponseCallBack(handler, pBuffer, dwLen);
}

void CRedisVirtualService::Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);
	Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisVirtualService::Warn(const string &strWarnInfo)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s, WarnInfo[%s]\n", __FUNCTION__, strWarnInfo.c_str());
	if(m_pExceptionWarn == NULL)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisVirtualService::%s, Warn CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pExceptionWarn(strWarnInfo);
}

void CRedisVirtualService::Log(const string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, const timeval_a &tStart, const string &strKey, const string &strValue, int nKeepTime, int nCode, const string &strIpAddrs, float fSpentTimeInQueue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s, Guid[%s], ServiceId[%d], MsgId[%d], Ip[%s]\n", __FUNCTION__, strGuid.c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str());
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
	snprintf(szLogBuffer, sizeof(szLogBuffer)-1, "%s,  %s,  %d,  %d,  %s,  %s, %d,  %s,  %f,  %f,  %d\n",
				strGuid.c_str(), TimevalToString(tStart).c_str(), dwServiceId, dwMsgId, strKey.c_str(), strTempValue.c_str(), nKeepTime, strIpAddrs.c_str(), dwSpent, fSpentTimeInQueue, nCode);

	m_pAsyncLog(RVS_ASYNCLOG_MODULE, XLOG_INFO, (unsigned int)dwSpent, szLogBuffer, nCode, m_strLocalIp, dwServiceId, dwMsgId);
}

void CRedisVirtualService::AddRedisServer(const string &strAddr)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisVirtualService::%s, strAddr[%s]\n", __FUNCTION__, strAddr.c_str());
    if(m_pRedisReadThreadGroup != NULL)
    {
    	m_pRedisReadThreadGroup->OnAddRedisServer(strAddr);
    }
	if(m_pRedisWriteThreadGroup != NULL)
	{
		m_pRedisWriteThreadGroup->OnAddRedisServer(strAddr);
	}
}

void CRedisVirtualService::Dump()
{
	RVS_XLOG(XLOG_DEBUG, "############begin dump############\n");
	SERVICE_CODE_TYPE_MAP::iterator iter;
	for (iter = m_mapCodeTypeByService.begin(); iter != m_mapCodeTypeByService.end(); iter++)
	{
		RVS_XLOG(XLOG_DEBUG, "ServiceId :	%6d\n", iter->first);

		CODE_TYPE_MAP::iterator iterTemp;
		for (iterTemp = iter->second.begin(); iterTemp != iter->second.end(); iterTemp++)
		{
			RVS_XLOG(XLOG_DEBUG, "Code : %5d,	Type : %d\n", iterTemp->first, iterTemp->second);
		}
	}

	REDIS_SET_TIME_MAP::const_iterator itrRedisKeepTime;
	for (itrRedisKeepTime=m_mapRedisKeepTime.begin(); itrRedisKeepTime!=m_mapRedisKeepTime.end(); itrRedisKeepTime++)
	{
		RVS_XLOG(XLOG_DEBUG, "ServiceId:%d, KeepTime:%d\n", itrRedisKeepTime->first, itrRedisKeepTime->second);
	}
	RVS_XLOG(XLOG_DEBUG, "############end dump############\n");
}



