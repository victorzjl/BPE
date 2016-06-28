#ifndef _HTTP_REC_VIRTUAL_CLIENT_H_
#define	_HTTP_REC_VIRTUAL_CLIENT_H_

#include "IAsyncVirtualClient.h"
#include <map>

#define HTTP_REC_VERSION "1.0.10"

using std::string;
using std::vector;
using std::map;

#ifdef WIN32
#define SDG_DLL_EXPORT _declspec(dllexport)
#else
#define SDG_DLL_EXPORT 
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    SDG_DLL_EXPORT IAsyncVirtualClient *create();
    SDG_DLL_EXPORT void destroy(IAsyncVirtualClient *pVirtualClient);

#ifdef __cplusplus
}
#endif

class CHpsSessionManager;
class CHpsServer;

class CHttpRecVirtualClient : public IAsyncVirtualClient
{
public:
    CHttpRecVirtualClient();
    virtual ~CHttpRecVirtualClient();

#ifdef WIN32
    virtual int Initialize(const string &strConfigFile, GVirtualClientService fun2Request, Response2BPE fun2Response, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog);
#else
    virtual int Initialize(GVirtualClientService fun2Request, Response2BPE fun2Response, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog);
#endif
    virtual int RequestService(IN void* pOwner, IN const void *pBuffer, IN int len) { return 0; };
    virtual int ResponseService(IN const void *handle, IN const void *pBuffer, IN int len);
    virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
    virtual void GetServiceId(vector<unsigned int> &vecServiceIds);
    virtual const string OnGetPluginInfo() const;
    
    virtual void SendRequest(CHpsSessionManager *handle, const void *pBuffer, int len);
        
private:
    GVirtualClientService          m_pFun2Request;
    Response2BPE                   m_pFun2Response;
    ExceptionWarn                  m_pExceptionWarn;
    AsyncLog                       m_pAsyncLog;
    vector<unsigned int>           m_vecServiceIds;
    string                         m_strLocalAddress;
    vector<CHpsServer *>           m_vecHpsServer;
};

#endif	/* _HTTP_REC_VIRTUAL_CLIENT_H_ */

