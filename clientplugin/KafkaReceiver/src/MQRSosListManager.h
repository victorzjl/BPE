#ifndef _MQR_SOS_LIST_MANAGER_H_
#define _MQR_SOS_LIST_MANAGER_H_

#include <map>
#include <string>
#include <detail/_time.h>
#include "MQRSosList.h"
#include "MQRVirtualClient.h"
#include "MQRCommon.h"

class CMQRSosListManager
{
public:
	CMQRSosListManager(CMQRVirtualClient *pMQRVirtualClient):m_pCMQRVirtualClient(pMQRVirtualClient) {}
	virtual ~CMQRSosListManager();
	
	void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
	
	int Initialize(std::vector<unsigned int>& vecServiceIds);
	
	void SendRequest(void *pRequest, int len, const SExtendData& extend);
	int HandleResponse(const void *pBuffer, int len, const SExtendData& extend);
	
	void Dump();
	
	void Warn(const std::string &strWarnInfo);
	void Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	                const timeval_a &tStart, int nCode, const std::string &strIpAddrs);
	
protected:
	bool ParseServiceTopicConfig(const std::string& config, SBusinessInfo& businessInfo);
	
private:
    CMQRVirtualClient             *m_pCMQRVirtualClient; 
	std::map<int, CMQRSosList*>    m_mapServiceIdAndSosList;
};

#endif //_MQR_SOS_LIST_MANAGER_H_
