#include "HttpRecVirtualClient.h"
#include "LogFilter.h"
#include "AsyncVirtualClientLog.h"
#include "Common.h"
#include "HttpRecConfig.h"
#include "ContextServerFactory.h"
#include "UploadFileHandleThread.h"

#include "HpsServer.h"
#include "HpsStack.h"
#include "UpdateManager.h"
#include "IpLimitManager.h"
#include "AsyncLogThread.h"
#include "FileSystemUpdateThread.h"
#include "CohStack.h"
#include "CohClientImp.h"

#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>

#define MAX_PATH_MAX 512

#ifdef SDG_SINGLETON_PLUGIN
//use for singleton plugin
enum enPluginStatus
{
    PLUGIN_UNINITIALIZED = 0,
    PLUGIN_INITIALIZING = 1,
    PLUGIN_INITIALIZED = 2
};
static CHttpRecVirtualClient *p_global_single_plugin = NULL;
enPluginStatus global_plugin_status = PLUGIN_UNINITIALIZED;
static unsigned int nReference = 0;
boost::mutex global_mutex;


IAsyncVirtualClient *create()
{
    boost::lock_guard<boost::mutex> guard(global_mutex);
    if (NULL == p_global_single_plugin)
    {
        p_global_single_plugin = new CHttpRecVirtualClient;
    }
    nReference++;
    return p_global_single_plugin;
}

void destroy(IAsyncVirtualClient *pVirtualClient)
{
    if (pVirtualClient == p_global_single_plugin)
    {
        boost::lock_guard<boost::mutex> guard(global_mutex);
        nReference--;
        if (0 == nReference)
        {
            delete p_global_single_plugin;
            p_global_single_plugin = NULL;
        }
    }
}
#else
    IAsyncVirtualClient *create()
    {
        return new CHttpRecVirtualClient;
    }

    void destroy(IAsyncVirtualClient *pVirtualClient)
    {
        if (NULL != pVirtualClient)
        {
            delete pVirtualClient;
            pVirtualClient = NULL;
        }
    }
#endif

CHttpRecVirtualClient::CHttpRecVirtualClient() 
{
#ifdef WIN32
    XLOG_INIT((boost::filesystem::current_path().string() + "/log.properties").c_str(), true);
#endif
    //注册日志模块
    XLOG_REGISTER(HTTP_REC_MODULE, "HttpRec");
    CHpsAsyncLogThread::GetInstance()->RegisterLogger();

    char szLocalAddr[16] = {0};
    GetLocalAddress(szLocalAddr);
    m_strLocalAddress = szLocalAddr;
}

CHttpRecVirtualClient::~CHttpRecVirtualClient() 
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s\n", __FUNCTION__);
    CFSUpdateThread::GetInstance()->Stop();
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s CFSUpdateThread stoped\n", __FUNCTION__);
    CHpsAsyncLogThread::GetInstance()->Stop();
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s CHpsAsyncLogThread stoped\n", __FUNCTION__);

    unsigned int nSize = m_vecHpsServer.size();
    for (unsigned int iIter = 0; iIter < nSize; ++iIter)
    {
        CHpsServer *pServer = m_vecHpsServer[iIter];
        pServer->StopServer();
        while (pServer->IsStartStatus())
        {
            //wait for 20 milliseconds
#ifdef WIN32
            Sleep(20);
#else
            usleep(1*20*1000);
#endif
        }
    }
    m_vecHpsServer.clear();

    CHpsStack::Instance()->Stop();
    sdo::coh::CCohStack::Stop();
}

#ifdef WIN32
int CHttpRecVirtualClient::Initialize(const string &strConfigFile, GVirtualClientService fun2Request, Response2BPE fun2Response, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog)
#else
int CHttpRecVirtualClient::Initialize(GVirtualClientService fun2Request, Response2BPE fun2Response, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog)
#endif
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s\n", __FUNCTION__);

#ifdef SDG_SINGLETON_PLUGIN
    boost::lock_guard<boost::mutex> guard(global_mutex);
    if (PLUGIN_INITIALIZED == global_plugin_status
        || PLUGIN_INITIALIZING == global_plugin_status)
    {
        return 0;
    }
    global_plugin_status = PLUGIN_INITIALIZING;
#endif

    m_pFun2Request = fun2Request;
    m_pFun2Response = fun2Response;
    m_pExceptionWarn = funcExceptionWarn;
    m_pAsyncLog = funcAsyncLog;

#ifndef WIN32 
    string strConfigFile = boost::filesystem::current_path().string() + "/config.xml";
#endif

    HttpRecConfigPtr pConfig = CHttpRecConfig::GetInstance();
    if (!pConfig->Load(strConfigFile))
	{
#ifdef SDG_SINGLETON_PLUGIN
        global_plugin_status = PLUGIN_UNINITIALIZED;
#endif
		return -1;
	}

    CLogFilter::Initialize(boost::filesystem::current_path().string() +"/logfilter.xml");

    string strMonitorUrl, strDetailMonitorUrl, strDgsUrl, strErrorUrl;
    pConfig->GetReportURLs(strMonitorUrl, strDetailMonitorUrl, strDgsUrl, strErrorUrl);
    CHpsCohClientImp::GetInstance()->Init(strMonitorUrl, strDetailMonitorUrl, strDgsUrl, strErrorUrl);

    CContextServerFactory::GetInstance()->Load();

    sdo::coh::CCohStack::Start();

    CHpsStack::Instance()->Start(this);
    
    CHpsAsyncLogThread::GetInstance()->Start();

    CFSUpdateThread::GetInstance()->Start();

    CUploadFileHandleThread::GetInstance()->Start();
 
    const vector<unsigned> &vecPorts = pConfig->GetHttpPorts();
    unsigned int nSize = vecPorts.size();
    for (unsigned int iIter = 0; iIter < nSize; ++iIter)
    {
        boost::asio::io_service &oService = CHpsStack::Instance()->GetAcceptThread()->GetIoService();
        CHpsServer *pServer = new CHpsServer(oService);
        HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s start server [%d]\n", __FUNCTION__, vecPorts[iIter]);
        if (pServer->StartServer(vecPorts[iIter]) != 0)
        {
            HTTP_REC_XLOG(XLOG_ERROR, "CHttpRecVirtualClient::%s, Http Recevier fails to start, port[%d]!\n", __FUNCTION__, vecPorts[iIter]);
#ifdef SDG_SINGLETON_PLUGIN
            global_plugin_status = PLUGIN_UNINITIALIZED;
#endif
            return -2;
        } 
        m_vecHpsServer.push_back(pServer);
    }    

#ifdef SDG_SINGLETON_PLUGIN
    global_plugin_status = PLUGIN_INITIALIZED;
#endif
    
    return 0;
}

int CHttpRecVirtualClient::ResponseService(IN const void *handle, IN const void *pBuffer, IN int len)
{
    HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecVirtualClient::%s, Get one response, handler[%p], pBuffer[%p], len[%d]\n", __FUNCTION__, handle, pBuffer, len);
    if (NULL == handle)
    {
        HTTP_REC_XLOG(XLOG_ERROR, "CHttpRecVirtualClient::%s, The handler for this response is NULL!\n", __FUNCTION__);
        return -1;
    }
    
    CHpsSessionManager *pSessionManager = (CHpsSessionManager *)handle;
    pSessionManager->OnReceiveServiceResponse(pBuffer, len);
    return 0;
}


void CHttpRecVirtualClient::SendRequest(CHpsSessionManager *handle, const void *pBuffer, int len)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s, handle[%p], pBuffer[%p], len[%d]!\n", __FUNCTION__, handle, pBuffer, len);
    m_pFun2Request(this, handle, pBuffer, len);
}

void CHttpRecVirtualClient::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
    
}

void CHttpRecVirtualClient::GetServiceId(vector<unsigned int> &vecServiceIds)
{
    vecServiceIds = m_vecServiceIds;
}

const string CHttpRecVirtualClient::OnGetPluginInfo() const
{
    return string("<tr><td>libhttprecevier.so</td><td>")
            + HTTP_REC_VERSION
            +	"</td><td>"
            + __TIME__
            + " " 
            + __DATE__
            + "</td></tr>";
}


