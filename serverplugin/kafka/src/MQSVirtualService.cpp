#include "MQSVirtualService.h"
#include "AsyncVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "MQSThreadGroup.h"
#include "Common.h"
#include "ReConnThread.h"

#include <XmlConfigParser.h>
#include <iostream>
#include <sstream>

#include <SapMessage.h>

#define		GUID_CODE			9

using std::stringstream;
using namespace sdo::sap;

IAsyncVirtualService * create()
{
	return new CMQSVirtualService;
}

void destroy( IAsyncVirtualService *pVirtualService )
{
	if(pVirtualService != NULL)
	{
		delete pVirtualService;
		pVirtualService = NULL;
	}
}

CMQSVirtualService::CMQSVirtualService():
	m_pResponseCallBack(NULL), m_pExceptionWarn(NULL),m_pAsyncLog(NULL), 
	m_pMQSThreadGroup(NULL)
{
//#ifdef _WIN32
	XLOG_INIT("log.properties");
//#endif
	//注册日志模块
	XLOG_REGISTER(MQS_MODULE, "mqs");
	XLOG_REGISTER(MQS_ASYNC_MODULE, "async_mqs");
	
	
	char szLocalAddr[16] = {0};
	GetLocalAddress(szLocalAddr);
	m_strLocalAddress = szLocalAddr;
}

CMQSVirtualService::~CMQSVirtualService()
{
	CReConnThread::GetReConnThread()->Stop();
	if(m_pMQSThreadGroup)
	{
		delete m_pMQSThreadGroup;
		m_pMQSThreadGroup = NULL;
	}
}

int CMQSVirtualService::Initialize( ResponseService funcResponseService, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s\n", __FUNCTION__);
	if(m_pResponseCallBack)
	{
		MQS_XLOG(XLOG_ERROR, "CMQSVirtualService::%s, ResponseCallBack have already setting\n", __FUNCTION__);
		return MQS_SYSTEM_ERROR;
	}

	m_pExceptionWarn = funcExceptionWarn;
	m_pResponseCallBack = funcResponseService;
	m_pAsyncLog = funcAsyncLog;
	
	int nRet = 0;

	//读取业务配置文件
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile("./config.xml") != 0)
	{
		MQS_XLOG(XLOG_ERROR, "CMQSVirtualService::%s, Config Parser fail, file[./config.xml], error[%s]\n",
                        		__FUNCTION__, objConfigParser.GetErrorMessage().c_str());
		return MQS_CONFIG_ERROR;
	}

	unsigned int dwThreadNum = objConfigParser.GetParameter("MQSenderThreadNum", 3);
	MQS_XLOG(XLOG_INFO, "CMQSVirtualService::%s, ThreadNum[%d]\n", __FUNCTION__, dwThreadNum);

	m_dwThreadNum = dwThreadNum;
	
	vector<string> vecMQSenderSvr = objConfigParser.GetParameters("MQSenderSosList");
	
	CReConnThread::GetReConnThread()->Initialize(this, 3);

	m_pMQSThreadGroup = new CMQSThreadGroup(this);
	if(m_pMQSThreadGroup == NULL)
	{
		MQS_XLOG(XLOG_ERROR, "CMQSVirtualService::%s, New Space MQSThreadGroup Error\n", __FUNCTION__);
		return MQS_NEW_SPACE_ERROR;
	}
	nRet = m_pMQSThreadGroup->Start(m_dwThreadNum, vecMQSenderSvr, m_vecServiceIds);
	CReConnThread::GetReConnThread()->Start();
	if(nRet != 0)
	{
		return nRet;
	}
	
	Dump();

	return 0;
}

int CMQSVirtualService::RequestService( IN void *pOwner, IN const void *pBuffer, IN int len )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s, Buffer[%p], Len[%d]\n",__FUNCTION__, pBuffer, len);

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
	
	if (!binary_search(m_vecServiceIds.begin(), m_vecServiceIds.end(), dwServiceId))
	{
		MQS_XLOG(XLOG_WARNING, "CMQSVirtualService::%s, ServiceId[%d] not supported\n", __FUNCTION__, dwServiceId);
		Log(strGuid, dwServiceId, dwMsgId, tStart, MQS_UNKNOWN_SERVICE, "");
		Response(pOwner, dwServiceId, dwMsgId, dwSequenceId, MQS_UNKNOWN_SERVICE);
		return MQS_UNKNOWN_SERVICE;
	}

	m_pMQSThreadGroup->OnProcess(dwServiceId, dwMsgId, dwSequenceId, strGuid, pOwner, pBuffer, len);
	
	return 0;
}

void CMQSVirtualService::GetServiceId( vector<unsigned int> &vecServiceIds )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s\n", __FUNCTION__);
	vecServiceIds = m_vecServiceIds;
}


const std::string CMQSVirtualService::OnGetPluginInfo() const
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s\n", __FUNCTION__);
	return string("<tr><td>libmqsender.so</td><td>")
		+ MQS_VERSION
		+	"</td><td>"
		+ __TIME__
		+ " " 
		+ __DATE__
		+ "</td></tr>";
}

void CMQSVirtualService::GetSelfCheckState( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s\n", __FUNCTION__);
	unsigned int tempAliveNum = 0;
	unsigned int tempAllNum = 0;

	m_pMQSThreadGroup->GetSelfCheck(tempAliveNum, tempAllNum);
	dwAliveNum = tempAliveNum;
	dwAllNum = tempAllNum;
}


void CMQSVirtualService::Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, 
                                   unsigned int dwSequenceId, int nCode )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s, Handler[%p], ServiceId[%d], MsgId[%d], Code[%d]\n",
                        	__FUNCTION__, handler, dwServiceId, dwMsgId, nCode);
							
	if(m_pResponseCallBack == NULL)
	{
		MQS_XLOG(XLOG_ERROR, "CMQSVirtualService::%s, Response CallBackFunc not setting\n", __FUNCTION__);
		return;
	}
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);

	m_pResponseCallBack(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CMQSVirtualService::Warn(const string &strWarnInfo)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s, WarnInfo[%s]\n", __FUNCTION__, strWarnInfo.c_str());
	if(m_pExceptionWarn == NULL)
	{
		MQS_XLOG(XLOG_ERROR, "CMQSVirtualService::%s, Warn CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pExceptionWarn(strWarnInfo);
}

void CMQSVirtualService::Log(const string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId,
                             const timeval_a &tStart, int nCode, const string &strIpAddrs)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSVirtualService::%s, Guid[%s], ServiceId[%d], MsgId[%d], Ip[%s]\n", 
	                      __FUNCTION__, strGuid.c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str());
	float dwSpent = GetTimeInterval(tStart);

	char szLogBuffer[2048] = {0};
	snprintf(szLogBuffer, sizeof(szLogBuffer)-1, "%s,  %s,  %d,  %d, %s,  %f,  %d\n",
				strGuid.c_str(), TimevalToString(tStart).c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str(), dwSpent, nCode);

	m_pAsyncLog(MQS_ASYNC_MODULE, XLOG_INFO, (unsigned int)dwSpent, szLogBuffer, nCode, m_strLocalAddress, dwServiceId, dwMsgId);
}


void CMQSVirtualService::Dump()
{
	MQS_XLOG(XLOG_NOTICE, "############begin dump############\n");
	
	MQS_XLOG(XLOG_NOTICE, "CMQSVirtualService::%s\n", __FUNCTION__);
	
	MQS_XLOG(XLOG_NOTICE, "CMQSVirtualService::%s < SupportedServiceIds[%s] >\n", __FUNCTION__, GetServiceIdString(m_vecServiceIds).c_str());
	
	MQS_XLOG(XLOG_NOTICE, "CMQSVirtualService::%s < LocalIp[%s] >\n", __FUNCTION__, m_strLocalAddress.c_str());	
	MQS_XLOG(XLOG_NOTICE, "CMQSVirtualService::%s < ThreadNumber[%d]\n", __FUNCTION__, m_dwThreadNum);
	
	m_pMQSThreadGroup->Dump();
	
	MQS_XLOG(XLOG_NOTICE, "############end dump############\n");
}
