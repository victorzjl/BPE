#include "AsyncVirtualClientLog.h"
#include "MQRVirtualClient.h"
#include "MQRSosListManager.h"
#include "SapMessage.h"
#include "ReConnThread.h"
#include "ErrorMsg.h"

#include <XmlConfigParser.h>
#include <vector>

using namespace sdo::sap;


IAsyncVirtualClient * create()
{
	return new CMQRVirtualClient;
}

void destroy( IAsyncVirtualClient *pVirtualClient )
{
	if(pVirtualClient != NULL)
	{
		delete pVirtualClient;
		pVirtualClient = NULL;
	}
}

CMQRVirtualClient::CMQRVirtualClient()
        : m_pMQRSosListManager(NULL)
{
//#ifdef _WIN32
	XLOG_INIT("log.properties");
//#endif
	//注册日志模块
	XLOG_REGISTER(MQR_MODULE, "mqr");
	XLOG_REGISTER(MQR_ASYNC_MODULE, "async_mqr");
	
	char szLocalAddr[16] = {0};
	GetLocalAddress(szLocalAddr);
	m_strLocalAddress = szLocalAddr;
}

CMQRVirtualClient::~CMQRVirtualClient()
{
	if (NULL != m_pMQRSosListManager)
	{
		delete m_pMQRSosListManager;
		m_pMQRSosListManager = NULL;
	}
	
	CReConnThread::GetReConnThread()->Stop();
}

int CMQRVirtualClient::Initialize(GVirtualClientService fnResponse, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s\n", __FUNCTION__);
	m_pVirtualClientService = fnResponse;
	m_pExceptionWarn = funcExceptionWarn;
	m_pAsyncLog = funcAsyncLog;
	
	m_sequence = 0;
	
	CReConnThread::GetReConnThread()->Initialize(this, 3);
	CReConnThread::GetReConnThread()->Start();
	
	m_pMQRSosListManager = new CMQRSosListManager(this);
	return m_pMQRSosListManager->Initialize(m_vecServiceIds);
}

int CMQRVirtualClient::ResponseService(IN const void *handle, IN const void *pBuffer, IN int len)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s. pBuffer[%p], len[%d]\n", __FUNCTION__, pBuffer, len);
	int sequence = ntohl(((SSapMsgHeader *)pBuffer)->dwSequence);
	SExtendData extend;
	
	{
		boost::lock_guard<boost::mutex> guard(m_mutex);
		std::map<int, SExtendData>::iterator iter = m_mapSequenceAndExtendData.find(sequence);
		if (iter == m_mapSequenceAndExtendData.end())
		{
			MQR_XLOG(XLOG_ERROR, "ResponseService::%s. Can't find the request corresponding to the response sequence[%d]\n", __FUNCTION__, sequence);
		    return MQR_NO_CORRESPONDING_REQUEST;
		}
		
		extend = iter->second;
		m_mapSequenceAndExtendData.erase(iter);
	}
	
	int ret = m_pMQRSosListManager->HandleResponse(pBuffer, len, extend);
	
	delete [] (char*)extend.pData;

	return ret;
}

	
void CMQRVirtualClient::SendRequest(void *pRequest, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s. pRequest[%p], len[%d]\n", __FUNCTION__, pRequest, len);
	
	void *pBuffer = new char[extend.length];
	memcpy(pBuffer, extend.pData, extend.length);
	SExtendData extendCopy;
	extendCopy.pData = pBuffer;
	extendCopy.length = extend.length;
	
	{
		boost::lock_guard<boost::mutex> guard(m_mutex);
		SSapMsgHeader *pSapMsgHeader = (SSapMsgHeader *)pRequest;
		pSapMsgHeader->dwSequence = htonl(m_sequence);
		m_mapSequenceAndExtendData[m_sequence] = extendCopy;
		m_sequence++;
	}
	
	m_pVirtualClientService(this, NULL, pRequest, len);
}

void CMQRVirtualClient::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
	dwAliveNum = 0;
	dwAllNum = 0;
	m_pMQRSosListManager->GetSelfCheckState(dwAliveNum, dwAllNum);
}

void CMQRVirtualClient::GetServiceId(std::vector<unsigned int> &vecServiceIds)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s\n", __FUNCTION__);
	vecServiceIds = m_vecServiceIds;
}

const string CMQRVirtualClient::OnGetPluginInfo() const
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s\n", __FUNCTION__);
	return string("<tr><td>libmqrecevier.so</td><td>")
		+ MQR_VERSION
		+	"</td><td>"
		+ __TIME__
		+ " " 
		+ __DATE__
		+ "</td></tr>";
}

void CMQRVirtualClient::Warn(const std::string &strWarnInfo)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s\n", __FUNCTION__);
	if(m_pExceptionWarn == NULL)
	{
		MQR_XLOG(XLOG_ERROR, "CMQSVirtualService::%s, Warn CallBackFunc not setting\n", __FUNCTION__);
		return;
	}

	m_pExceptionWarn(strWarnInfo);
}

void CMQRVirtualClient::Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
		 const timeval_a &tStart, int nCode, const std::string &strIpAddrs)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s, Guid[%s], ServiceId[%d], MsgId[%d], Ip[%s]\n", 
	                      __FUNCTION__, strGuid.c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str());
	float dwSpent = GetTimeInterval(tStart);

	char szLogBuffer[2048] = {0};
	snprintf(szLogBuffer, sizeof(szLogBuffer)-1, "%s,  %s,  %d,  %d, %s,  %f,  %d\n",
				strGuid.c_str(), TimevalToString(tStart).c_str(), dwServiceId, dwMsgId, strIpAddrs.c_str(), dwSpent, nCode);

	m_pAsyncLog(MQR_ASYNC_MODULE, XLOG_INFO, (unsigned int)dwSpent, szLogBuffer, nCode, m_strLocalAddress, dwServiceId, dwMsgId);
}

int CMQRVirtualClient::ReConnWarn(const std::string& warnning)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRVirtualClient::%s\n", __FUNCTION__);
	Warn(warnning);
	return 0;
}
	

void CMQRVirtualClient::Dump()
{
	MQR_XLOG(XLOG_NOTICE, "############begin dump############\n");
	
	MQR_XLOG(XLOG_NOTICE, "CMQRVirtualClient::%s\n", __FUNCTION__);
	MQR_XLOG(XLOG_NOTICE, "CMQRVirtualClient::%s < SupportedServiceIds[%s] >\n", __FUNCTION__, GetServiceIdString(m_vecServiceIds).c_str());
	MQR_XLOG(XLOG_NOTICE, "CMQRVirtualClient::%s < LocalIp[%s] >\n", __FUNCTION__, m_strLocalAddress.c_str());	
	
	m_pMQRSosListManager->Dump();
	CReConnThread::GetReConnThread()->Dump();
	
	MQR_XLOG(XLOG_NOTICE, "############end dump############\n");
}
