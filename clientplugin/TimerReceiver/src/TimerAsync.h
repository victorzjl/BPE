#ifndef _TIMER_RECEIVER_ASYNC_H_
#define _TIMER_RECEIVER_ASYNC_H_
#include "IAsyncVirtualClient.h"
#include "TimerCommon.h"
#include <string.h>
#include "SapMessage.h"
#include "SapTLVBody.h"
#include <boost/thread/mutex.hpp>

using namespace sdo::sap;
using namespace std;

#define TIMER_VERSION "1.0.0"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
    IAsyncVirtualClient* create();
    void destroy(IAsyncVirtualClient* p);
#ifdef __cplusplus
}
#endif // __cplusplus

class CTimerAsync:public IAsyncVirtualClient
{
public:
    int Initialize(GVirtualClientService fnResponse,Response2BPE fnResponse2BPE, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog);
    int RequestService(void* pOwner, const void *pBuffer, int len);
    int ResponseService(const void *handle, const void *pBuffer, int len);
    void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
    void GetServiceId(vector<unsigned int> &vecServiceIds)
    {
        vecServiceIds.push_back(SERVICE_ID);
    }
    const string OnGetPluginInfo()const
    {
        return string("<tr><td>libtimer.so</td><td>")+TIMER_VERSION+
		"</td><td>"+__TIME__+" "+__DATE__+"</td></tr>";
    }
public:
    static CTimerAsync* GetInstance();
    static void Release();
private:
    static void init();
    static void destory();
    static void doOnce();
    void RecordLog(CSapDecoder& decode, const void *handle);
    CTimerAsync(){}
    ~CTimerAsync();
    CTimerAsync(const CTimerAsync& rhd);
    CTimerAsync* operator=(const CTimerAsync& rhd);

public:
	GVirtualClientService gVRequest;
	AsyncLog m_funcAsyncLog;
private:
    static CTimerAsync* m_pInstance;
    boost::mutex m_mut;
};

#endif
