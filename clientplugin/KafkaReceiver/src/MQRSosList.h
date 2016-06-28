#ifndef _MQR_SOS_LIST_H_
#define _MQR_SOS_LIST_H_

#include <vector>
#include <map>
#include <string>
#include <detail/_time.h>

#include "MQRTopicConsumerGroup.h"
#include "ZooKeeper.h"
#include "IZooKeeperWatcher.h"
#include "MQRCommon.h"


typedef struct stBusinessInfo
{
	std::string topic;
	int originalServiceId;
	int aimServiceId;
	std::vector<int> ackCodes;
}SBusinessInfo;

typedef struct stHostConfig
{
	std::string topic;
	int hostNumber;
	int hostNode;
}SHostConfig;


class CMQRSosListManager;
class CMQRSosList : public IZooKeeperWatcher
{
public:
	CMQRSosList(CMQRSosListManager *pMQRSosListManager);
	virtual ~CMQRSosList();
	
	int Initialize(const std::string& zookeeperAddrs, const std::vector<std::string>& vecHostConfigs);
	void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
	int AddBusinessInfo(SBusinessInfo& businessInfo);
	CZooKeeper* GetZooKeeper() { return m_pZookeeper; }
	
	int Start();
	
	virtual int OnReConnSuccEvent(CZooKeeper *pZooKeeper);
	
	int ConfirmOffset(const std::string& topic, unsigned int partition, unsigned int offset);
	void SendRequest(void *pRequest, int len, const SExtendData& extend);
	int HandleResponse(const void *pBuffer, int len, const SExtendData& extend);
	
	void Dump();
	
	void Warn(const std::string &strWarnInfo);
	void Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	                const timeval_a &tStart, int nCode, const std::string &strIpAddrs);
			 
private:
	int StartConsumers();
	
private:
	CMQRSosListManager                              *m_pMQRSosListManager;
	std::map<int, SBusinessInfo>                     m_mapServiceAndBusinessInfo;
	std::map<std::string, SHostConfig>               m_mapTopicAndHostConfig;
	std::map<std::string, CMQRTopicConsumerGroup*>   m_mapTopicConsumerGroup;
	std::string                                      m_zookeeperAddrs;
	CZooKeeper                                      *m_pZookeeper;
	bool                                             m_bConsumerStarted;
};

#endif //_MQR_SOS_LIST_H_
