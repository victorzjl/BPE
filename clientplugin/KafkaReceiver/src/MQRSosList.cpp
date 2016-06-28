#include "MQRSosList.h"
#include "MQRSosListManager.h"
#include "SapMessage.h"
#include "AsyncVirtualClientLog.h"
#include "ErrorMsg.h"

#include <netinet/in.h>

using namespace sdo::sap;

CMQRSosList::CMQRSosList(CMQRSosListManager *pMQRSosListManager)
           :m_pMQRSosListManager(pMQRSosListManager),
   		    m_pZookeeper(new CZooKeeper(this)),
			m_bConsumerStarted(false)
{
	
}

CMQRSosList::~CMQRSosList()
{
	m_pZookeeper->Close();
	delete m_pZookeeper;
	m_pZookeeper = NULL;
	
	for (std::map<std::string, CMQRTopicConsumerGroup*>::iterator iter=m_mapTopicConsumerGroup.begin();
	     iter!=m_mapTopicConsumerGroup.end(); ++iter)
	{
		 delete (iter->second);
		 iter->second = NULL;
	}
	m_mapTopicConsumerGroup.clear();
}

void CMQRSosList::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
	for (std::map<std::string, CMQRTopicConsumerGroup*>::iterator iter=m_mapTopicConsumerGroup.begin();
        	iter!=m_mapTopicConsumerGroup.end(); ++iter)
	{
		unsigned int aliveNum = 0, allNum = 0;
		iter->second->GetSelfCheckState(aliveNum, allNum);
		dwAliveNum += aliveNum;
		dwAllNum += allNum;
	}
}
	
int CMQRSosList::AddBusinessInfo(SBusinessInfo& businessInfo)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosList::%s\n", __FUNCTION__);
	m_mapServiceAndBusinessInfo[businessInfo.originalServiceId] = businessInfo;
	return 0;
}

int CMQRSosList::Initialize(const std::string& zookeeperAddrs, const std::vector<std::string>& vecHostConfigs)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosList::%s\n", __FUNCTION__);
	m_zookeeperAddrs = zookeeperAddrs;
	
	for (std::vector<std::string>::const_iterator iter=vecHostConfigs.begin(); iter!=vecHostConfigs.end(); ++iter)
	{
		char topic[64] = {0};
		unsigned int hostNumber, hostNode;
		sscanf(iter->c_str(), "%[^:]:%u_%u", topic, &hostNumber, &hostNode);
		
		SHostConfig hostConfig;
		hostConfig.topic = topic;
		hostConfig.hostNumber = hostNumber;
		hostConfig.hostNode = hostNode;
		
		m_mapTopicAndHostConfig[hostConfig.topic] = hostConfig;
	}
	
	return m_pZookeeper->Initialize(zookeeperAddrs)? 0:-1;
}

int CMQRSosList::Start()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosList::%s\n", __FUNCTION__);
	
	if (m_pZookeeper->Connect())
	{
		return StartConsumers();
	}
	else
	{
		return MQR_ZOOKEEPER_CONNECT_ERROR;
	}
}
	
int CMQRSosList::StartConsumers()
{
	if (m_bConsumerStarted)
	{
		return 0;
	}
	
	for (std::map<std::string, SHostConfig>::iterator iter=m_mapTopicAndHostConfig.begin(); iter!=m_mapTopicAndHostConfig.end(); ++iter)
	{
		CMQRTopicConsumerGroup* pMQRTopicConsumerGroup = new CMQRTopicConsumerGroup(this);
		pMQRTopicConsumerGroup->Initialize(iter->first, iter->second.hostNumber, iter->second.hostNode);
		m_mapTopicConsumerGroup[iter->first]=pMQRTopicConsumerGroup;
		m_pZookeeper->CreateConfirmOffsetNode(iter->first);
	}
	
	for (std::map<std::string, CMQRTopicConsumerGroup*>::iterator iter=m_mapTopicConsumerGroup.begin(); 
	     iter!=m_mapTopicConsumerGroup.end(); ++iter)
	{
		iter->second->Start();
	}
	
	m_bConsumerStarted = true;
	return 0;
}

int CMQRSosList::OnReConnSuccEvent(CZooKeeper *pZooKeeper)
{
	return StartConsumers();
}

void CMQRSosList::SendRequest(void *pRequest, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosList::%s\n", __FUNCTION__);
	SSapMsgHeader *pHeader = (SSapMsgHeader *)pRequest;
	int serviceId = ntohl(pHeader->dwServiceId);
	
	std::map<int, SBusinessInfo>::iterator iter = m_mapServiceAndBusinessInfo.find(serviceId);
	
	((SConsumeInfo*)extend.pData)->serviceId = serviceId;
	if (iter != m_mapServiceAndBusinessInfo.end())
	{
		if (iter->second.aimServiceId > 0)
		{
			pHeader->dwServiceId = htonl(iter->second.aimServiceId);
		}
	}
	
	m_pMQRSosListManager->SendRequest(pRequest, len, extend);
}

int CMQRSosList::HandleResponse(const void *pBuffer, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosList::%s\n", __FUNCTION__);
	SConsumeInfo *pInfo = (SConsumeInfo *)extend.pData;
	int code = ntohl(((SSapMsgHeader *)pBuffer)->dwCode);

	std::map<int, SBusinessInfo>::iterator iterBusinessInfo = m_mapServiceAndBusinessInfo.find(pInfo->serviceId);
	if (iterBusinessInfo == m_mapServiceAndBusinessInfo.end())
	{
		MQR_XLOG(XLOG_ERROR, "CMQRSosList::%s. The Service id is not supported [%d]\n", __FUNCTION__, pInfo->serviceId);
		return MQR_UNKNOWN_SERVICE;
	}
	
	pInfo->isAck = false;
	for (std::vector<int>::iterator iter=iterBusinessInfo->second.ackCodes.begin(); 
	       iter!=iterBusinessInfo->second.ackCodes.end(); ++iter)
	{
		if (*iter == code)
		{
			pInfo->isAck = true;
			break;
		}
	}
	
	std::map<std::string, CMQRTopicConsumerGroup*>::iterator iterTopicConsumerGroup 
	                    = m_mapTopicConsumerGroup.find(iterBusinessInfo->second.topic);
	if (iterTopicConsumerGroup == m_mapTopicConsumerGroup.end())
	{
		MQR_XLOG(XLOG_ERROR, "CMQRSosList::%s. The topic[%s] is not servicing \n",
                         		__FUNCTION__, iterBusinessInfo->second.topic.c_str());
		return MQR_NO_TOPIC;
	}
	
	return iterTopicConsumerGroup->second->HandleResponse(pBuffer, len, extend);
}

int CMQRSosList::ConfirmOffset(const std::string& topic, unsigned int partition, unsigned int offset)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosList::%s\n", __FUNCTION__);
	if (NULL == m_pZookeeper)
	{
		return -1;
	}
	
	return m_pZookeeper->ConfirmOffset(topic, partition, offset)? 0 : -1;
}

void CMQRSosList::Warn(const std::string &strWarnInfo) 
{ 
	m_pMQRSosListManager->Warn(strWarnInfo); 
}

void CMQRSosList::Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
		              const timeval_a &tStart, int nCode, const std::string &strIpAddrs)
{
	m_pMQRSosListManager->Log(strGuid, dwServiceId, dwMsgId, tStart, nCode, strIpAddrs);
}

void CMQRSosList::Dump()
{
	MQR_XLOG(XLOG_NOTICE, "CMQRSosList::%s\n", __FUNCTION__);
	MQR_XLOG(XLOG_NOTICE, "CMQRSosList::%s < ZooKeeper Addresses[%s] >\n", __FUNCTION__, m_zookeeperAddrs.c_str());
		
	m_pZookeeper->Dump();
	for (std::map<std::string, CMQRTopicConsumerGroup*>::iterator iter=m_mapTopicConsumerGroup.begin();
	     iter!=m_mapTopicConsumerGroup.end(); ++iter)
	{
		iter->second->Dump();
	}
}
