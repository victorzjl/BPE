#ifndef _CVS_CACHE_VIRUTAL_SERVER_H_
#define _CVS_CACHE_VIRUTAL_SERVER_H_

#include "IAsyncVirtualService.h"
#include <detail/_time.h>
#include "CacheMsg.h"

#define CVS_VERSION "1.0.10"

#ifdef __cplusplus
extern "C"
{
#endif

	IAsyncVirtualService *create();
	void destroy(IAsyncVirtualService *pVirtualService);

#ifdef __cplusplus
}
#endif

class CCacheThreadGroup;
class CCacheWriteThreadGroup;
class CCacheReConnThread;
class CWarnCondition;
class CCacheVirtualService: public IAsyncVirtualService
{
public:
	CCacheVirtualService();
	virtual ~CCacheVirtualService();

	virtual int Initialize(ResponseService funcResponseService, 
		ExceptionWarn funcExceptionWarn,
		AsyncLog funcAsyncLog);
	virtual int RequestService(IN void *pOwner, IN const void *pBuffer, IN int len);
	virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
	virtual void GetServiceId(vector<unsigned int> &vecServiceIds);
	virtual const std::string OnGetPluginInfo() const;
public:
	void Response(void *handler, const void *pBuffer, unsigned int dwLen);
	void Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode );
	void Warn(const string &strWarnInfo);
	void Log(const string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, const timeval_a &tStart, const string &strKey, const string &strValue, int nKeepTime, int nCode, const string &strIpAddrs);
	CCacheReConnThread *GetReConnThread(){return m_pCacheReConnThread;}
	CWarnCondition *GetWarnCondition(){return m_pWarnCondition;}
private:
	void Dump();
private:
	ResponseService m_pResponseCallBack;
	ExceptionWarn m_pExceptionWarn;
	AsyncLog m_pAsyncLog;
private:
	vector<unsigned int> m_vecServiceIds;
	SERVICE_TYPE_CODE_MAP m_mapCodeTypeByService;
	CACHE_SET_TIME_MAP m_mapCacheKeepTime;
	map<unsigned int, bool> m_mapClearText;
	map<unsigned int, string> m_mapKeyPrefix;
	string m_strLocalIp;
	unsigned int m_dwThreadNum;
	unsigned int m_dwWriteThreadNum;
	CCacheThreadGroup *m_pCacheThreadGroup;
	CCacheWriteThreadGroup *m_pCacheWriteThreadGroup;
	CCacheReConnThread *m_pCacheReConnThread;
	CWarnCondition *m_pWarnCondition;
};

#endif
