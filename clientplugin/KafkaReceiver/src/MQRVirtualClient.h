#ifndef _MQR_VIRUTAL_CLIENT_H_
#define _MQR_VIRUTAL_CLIENT_H_

#include "IAsyncVirtualClient.h"
#include "MQRCommon.h"
#include "IReConnHelp.h"
#include "Common.h"

#include <vector>
#include <boost/thread/mutex.hpp>
#include <map>


#define MQR_VERSION "1.0.10"


#ifdef __cplusplus
extern "C"
{
#endif

	IAsyncVirtualClient *create();
	void destroy(IAsyncVirtualClient *pVirtualClient);

#ifdef __cplusplus
}
#endif

class CMQRSosListManager;

class CMQRVirtualClient : public IAsyncVirtualClient, public IReConnWarnRecevier
{
public:
    CMQRVirtualClient();
	virtual ~CMQRVirtualClient();
	
	virtual int Initialize(GVirtualClientService fnResponse, ExceptionWarn funcExceptionWarn, AsyncLog funcAsyncLog);
	virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
	virtual void GetServiceId(std::vector<unsigned int> &vecServiceIds);
	virtual const string OnGetPluginInfo() const;
	

	virtual int ResponseService(IN const void *handle, IN const void *pBuffer, IN int len);
	void SendRequest(void *pRequest, int len, const SExtendData& extend);
	
	void Dump();
	
public:
	void Warn(const std::string &strWarnInfo);
	void Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	         const timeval_a &tStart, int nCode, const std::string &strIpAddrs);
	virtual int ReConnWarn(const std::string& warnning);
	
private:
	std::vector<unsigned int>     m_vecServiceIds;	
	boost::mutex                  m_mutex;
	unsigned int                  m_sequence;
	std::map<int, SExtendData>    m_mapSequenceAndExtendData;
	CMQRSosListManager           *m_pMQRSosListManager;
	std::string                   m_strLocalAddress;
	GVirtualClientService         m_pVirtualClientService;
	ExceptionWarn                 m_pExceptionWarn;
	AsyncLog                      m_pAsyncLog;
};

#endif //_MQR_VIRUTAL_CLIENT_H_
