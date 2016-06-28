#ifndef _I_ASYNC_VIRTUAL_CLIENT_H_
#define _I_ASYNC_VIRTUAL_CLIENT_H_
#include <string>
#include <vector>
#include "ILog.h"
using std::string;
using std::vector;
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif


class IAsyncVirtualClient;

typedef void(*GVirtualClientService)(IAsyncVirtualClient* pPlugIn, void*, const void *, int);
typedef void(*Response2BPE)(void*, const void *, int);
typedef void(*ExceptionWarn)(const string &);
typedef void(*AsyncLog)(int nModel, XLOG_LEVEL level, int nInterval, const string &strMsg,
    int nCode, const string& strAddr, int serviceId, int msgId);


class IAsyncVirtualClient
{
public:
    virtual int Initialize(GVirtualClientService fnResponse, Response2BPE fnResponse2BPE, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog) = 0;
    virtual int RequestService(IN void* pOwner, IN const void *pBuffer, IN int len) = 0;
    virtual int ResponseService(IN const void *handle, IN const void *pBuffer, IN int len) = 0;
    virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum) = 0;
    virtual void GetServiceId(vector<unsigned int> &vecServiceIds) = 0;
    virtual const string OnGetPluginInfo()const = 0;
    virtual ~IAsyncVirtualClient(){}
};



#endif 

