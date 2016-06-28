#ifndef _MQS_VIRUTAL_SERVICE_H_
#define _MQS_VIRUTAL_SERVICE_H_

#include "IAsyncVirtualService.h"
#include "IReConnHelp.h"
#include <detail/_time.h>

#define MQS_VERSION "1.0.10"

#ifdef __cplusplus
extern "C"
{
#endif

	IAsyncVirtualService *create();
	void destroy(IAsyncVirtualService *pVirtualService);

#ifdef __cplusplus
}
#endif

class CMQSThreadGroup;
class CMQSReConnThread;
class CWarnCondition;
class CMQSVirtualService: public IAsyncVirtualService, public IReConnWarnRecevier
{
public:
	CMQSVirtualService();
	virtual ~CMQSVirtualService();

	virtual int Initialize(ResponseService funcResponseService, 
		ExceptionWarn funcExceptionWarn,
		AsyncLog funcAsyncLog);
	virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
	virtual void GetServiceId(vector<unsigned int> &vecServiceIds);
	virtual const std::string OnGetPluginInfo() const;
	
	virtual int RequestService(IN void *pOwner, IN const void *pBuffer, IN int len);
	
	int ReConnWarn(const std::string &warnning) { Warn(warnning); return 0; }
	
public:
	void Response(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, 
	              unsigned int dwSequenceId, int nCode);
	void Warn(const std::string &strWarnInfo);
	void Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	         const timeval_a &tStart, int nCode, const std::string &strIpAddrs);

	void Dump();
	
private:
	ResponseService       m_pResponseCallBack;
	ExceptionWarn         m_pExceptionWarn;
	AsyncLog              m_pAsyncLog;
	
private:
	std::vector<unsigned int>  m_vecServiceIds;
	std::string                m_strLocalAddress;
	unsigned int               m_dwThreadNum;
	CMQSThreadGroup           *m_pMQSThreadGroup;
};

#endif //_MQS_VIRUTAL_SERVICE_H_
