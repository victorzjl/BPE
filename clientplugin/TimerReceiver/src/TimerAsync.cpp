#include "TimerAsync.h"
#include <boost/thread/once.hpp>
#include <boost/algorithm/string.hpp>
#include "AsyncVirtualServiceLog.h"
#include "TimerThread.h"
#include "TimerServiceConfig.h"

static boost::once_flag once_flag = BOOST_ONCE_INIT;
static boost::once_flag call_once_flag = BOOST_ONCE_INIT;

CTimerAsync * CTimerAsync::m_pInstance = NULL;

void CTimerAsync::init()
{
    m_pInstance = new CTimerAsync();
    XLOG_REGISTER(ASYNC_VIRTUAL_MODULE,"timer_client");
    SV_XLOG(XLOG_DEBUG,"create timer receiver async succ...\n");
}

void CTimerAsync::destory()
{
    delete m_pInstance;
}

CTimerAsync * CTimerAsync::GetInstance()
{
    boost::call_once(&init, once_flag);
    return m_pInstance;
}

void CTimerAsync::Release()
{
    boost::call_once(&destory, once_flag);
}

CTimerAsync::~CTimerAsync()
{

}

int CTimerAsync::Initialize(GVirtualClientService fnResponse,Response2BPE fnResponse2BPE, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog)
{
    SV_XLOG(XLOG_DEBUG,"CTimerAsync::%s, begin init function.\n", __FUNCTION__);

    boost::lock_guard<boost::mutex> lock(m_mut);

    gVRequest = fnResponse;
    m_funcAsyncLog = funcAsyncLog;


    CTimerServiceConfig oConfig;

    if(0 != oConfig.loadServiceConfig("./timerconfig.xml"))
    {
        return -1;
    }

    vector<SConfigRequest> vecConfig = oConfig.getRequests();
    CTimerThread::GetInstance()->setRequests(vecConfig);

    boost::call_once(&doOnce, call_once_flag);

    return 0;
}

void CTimerAsync::doOnce()
{
    SV_XLOG(XLOG_DEBUG,"CTimerAsync::%s --------begin-------.\n", __FUNCTION__);

    CTimerThread::GetInstance()->Start();
}

int CTimerAsync::RequestService(void* pOwner, const void *pBuffer, int len)
{
    return 0;
}

int CTimerAsync::ResponseService(const void *handle, const void *pBuffer, int len)
{
    SV_XLOG(XLOG_DEBUG,"CTimerAsync::%s --------begin-------.\n", __FUNCTION__);

    CSapDecoder msgDecode(pBuffer,len);
	msgDecode.DecodeBodyAsTLV();

    RecordLog(msgDecode, handle);

    return 0;
}

void CTimerAsync::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
    dwAllNum = 1;
    if(CTimerThread::GetInstance()->isAlive())
    {
        dwAliveNum = 1;
    }else{
        dwAliveNum = 0;
    }
}


void CTimerAsync::RecordLog(CSapDecoder& decode, const void *handle)
{
	struct timeval_a now;
	gettimeofday_a(&now, 0);

	char szguid[64] = {0};
    snprintf(szguid, sizeof(szguid), "%s", (char*)handle);

    char szBuf[2048]={0};

    SV_XLOG(XLOG_DEBUG,"CTimerAsync::%s ServiceId[%d] MessageId[%d] Guid[%s] code[%d] .\n", __FUNCTION__, decode.GetServiceId(), decode.GetMsgId(), szguid, (int)decode.GetCode());

	struct tm tmNow;
    localtime_r(&(now.tv_sec), &tmNow);

	snprintf(szBuf, sizeof(szBuf),
        "%04u-%02u-%02u %02u:%02u:%02u.%03ld,  %s,  %d,  %d,  %d \n",
		tmNow.tm_year+1900,
        tmNow.tm_mon+1,
        tmNow.tm_mday,
        tmNow.tm_hour,
        tmNow.tm_min,
        tmNow.tm_sec,
        now.tv_usec/1000,
        szguid,
        decode.GetServiceId(),
        decode.GetMsgId(),
		(int)decode.GetCode());
    m_funcAsyncLog(ASYNC_VIRTUAL_MODULE, XLOG_INFO, 0, szBuf,	0, "127.0.0.1", decode.GetServiceId(), decode.GetMsgId());
}

IAsyncVirtualClient* create()
{
    return CTimerAsync::GetInstance();
}

void destroy(IAsyncVirtualClient* p)
{
    CTimerAsync::Release();
}
