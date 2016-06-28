#include "MQRSosListManager.h"
#include "SapMessage.h"
#include "AsyncVirtualClientLog.h"
#include "ErrorMsg.h"

#include <XmlConfigParser.h>
#include <cstdio>
#include <boost/algorithm/string.hpp>

CMQRSosListManager::~CMQRSosListManager()
{
	for (std::map<int, CMQRSosList*>::iterator iter=m_mapServiceIdAndSosList.begin();
	     iter!=m_mapServiceIdAndSosList.end(); ++iter)
	{
		delete (iter->second);
		iter->second = NULL;
	}
	
	m_mapServiceIdAndSosList.clear();
}

void CMQRSosListManager::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{ 
	for (std::map<int, CMQRSosList*>::iterator iter=m_mapServiceIdAndSosList.begin(); iter!=m_mapServiceIdAndSosList.end(); ++iter)
	{
		unsigned int aliveNum = 0, allNum = 0;
		iter->second->GetSelfCheckState(aliveNum, allNum);
		dwAliveNum += aliveNum;
		dwAllNum += allNum;
	}
}

int CMQRSosListManager::Initialize(std::vector<unsigned int>& vecServiceIds)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosListManager::%s.\n", __FUNCTION__);
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile("./config.xml") != 0)
	{
		MQR_XLOG(XLOG_ERROR, "CMQRSosListManager::%s, Config Parser fail, file[./config.xml], error[%s]\n", __FUNCTION__, objConfigParser.GetErrorMessage().c_str());
		return MQR_CONFIG_ERROR;
	}
		
	std::vector<std::string> vecRecevierConfigures = objConfigParser.GetParameters("MQRecevierSosList");
	for (std::vector<std::string>::iterator iter=vecRecevierConfigures.begin(); iter!=vecRecevierConfigures.end(); ++iter)
	{
		CXmlConfigParser sosListParser;
		sosListParser.ParseDetailBuffer(iter->c_str());
		
		//Get zookeeper host config
		std::string zookeeperAddrs;
		std::vector<std::string> vecAddrs = sosListParser.GetParameters("ServerAddr");
		for (std::vector<std::string>::iterator iterTemp = vecAddrs.begin(); iterTemp != vecAddrs.end(); iterTemp++)
		{
			if (iterTemp != vecAddrs.begin())
			{
				zookeeperAddrs += ",";
			}
			zookeeperAddrs += *iterTemp;
		}
		
		//Get topic consume config
		std::vector<std::string> topicConsumeConfigs = sosListParser.GetParameters("TopicConsumeConfig");
		
		//Get consumer and business config
		std::vector<std::string> vecServiceTopicConfigs = sosListParser.GetParameters("ServiceTopicConfig");
		CMQRSosList *pSosList = new CMQRSosList(this);
		pSosList->Initialize(zookeeperAddrs, topicConsumeConfigs);
		for (std::vector<std::string>::iterator iterTemp = vecServiceTopicConfigs.begin(); iterTemp != vecServiceTopicConfigs.end(); iterTemp++)
		{
			SBusinessInfo businessInfo;			
			if (ParseServiceTopicConfig(*iterTemp, businessInfo))
			{
				pSosList->AddBusinessInfo(businessInfo);	
			}
		}
		
		//Get zookeeper service id config
		std::vector<std::string> vecStrServiceIds;
		string strServiceIds = sosListParser.GetParameter("ServiceId");
		boost::algorithm::split(vecStrServiceIds, strServiceIds, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
		
		for (std::vector<std::string>::iterator iterTemp = vecStrServiceIds.begin(); iterTemp != vecStrServiceIds.end(); iterTemp++)
		{
			std::string &strServiceId = *iterTemp;
			boost::algorithm::trim(strServiceId);
			int iServiceId = atoi(strServiceId.c_str());
			vecServiceIds.push_back(iServiceId);
			m_mapServiceIdAndSosList[iServiceId] = pSosList;
		}
	}
	
	sort(vecServiceIds.begin(), vecServiceIds.end());
	
	for (std::map<int, CMQRSosList*>::iterator iter=m_mapServiceIdAndSosList.begin(); iter!=m_mapServiceIdAndSosList.end(); ++iter)
	{
		iter->second->Start();
	}
	
	return MQR_SUCESS;
}

void CMQRSosListManager::SendRequest(void *pRequest, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosListManager::%s. pRequest[%p], len[%d]\n", __FUNCTION__, pRequest, len);
	m_pCMQRVirtualClient->SendRequest(pRequest, len, extend);
}

int CMQRSosListManager::HandleResponse(const void *pBuffer, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosListManager::%s. pBuffer[%p], len[%d]\n", __FUNCTION__, pBuffer, len);
	std::map<int, CMQRSosList*>::iterator iter = m_mapServiceIdAndSosList.find(((SConsumeInfo *)extend.pData)->serviceId);
	if (iter == m_mapServiceIdAndSosList.end())
	{
		MQR_XLOG(XLOG_ERROR, "CMQRSosListManager::%s. The Service id is not supported [%d]\n", __FUNCTION__, ((SConsumeInfo *)extend.pData)->serviceId);
		return MQR_UNKNOWN_SERVICE;
	}
	
	return iter->second->HandleResponse(pBuffer, len, extend);
}

bool CMQRSosListManager::ParseServiceTopicConfig(const std::string& config, SBusinessInfo& businessInfo)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRSosListManager::%s. config[%s]\n", __FUNCTION__, config.c_str());
	if (config.empty())
	{
		return false;
	}
	
	char buffer1[64] = {0};
	char buffer2[64] = {0};
	char buffer3[64] = {0};
	
	int ret = sscanf(config.c_str(), "%[^#]#%[^#]#%s", buffer1, buffer2, buffer3);
	
	if ( (ret<=0) || (ret==EOF) )
	{
		MQR_XLOG(XLOG_ERROR, "CMQRSosListManager::%s. Failed to Parse ServiceTopic Config [%s]\n", __FUNCTION__, config.c_str());
		return false;
	}
	
	businessInfo.aimServiceId = -1;
	businessInfo.ackCodes.push_back(0);
	if (ret>0)
	{
		char topic[64] = {0};
		if (2 != sscanf(buffer1, "%d:%s", &businessInfo.originalServiceId, topic))
		{
			MQR_XLOG(XLOG_ERROR, "CMQRSosListManager::%s. Failed to Parse OriginalServiceId and Topic Config [%s]\n", __FUNCTION__, buffer1);
			return false;
		}
		businessInfo.topic = topic;
	}
	
	if (ret>1)
	{
		businessInfo.aimServiceId = atoi(buffer2);
	}
	
	if (ret>2)
	{
		businessInfo.ackCodes.clear();
		std::vector<std::string> vecStrServiceIds;
		boost::algorithm::split(vecStrServiceIds, buffer3, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
		for (vector<string>::iterator iter = vecStrServiceIds.begin(); iter != vecStrServiceIds.end(); iter++)
		{
			string &strServiceId = *iter;
			boost::algorithm::trim(strServiceId);
			businessInfo.ackCodes.push_back(atoi(strServiceId.c_str()));
		}
	}
	
	return true;
}

void CMQRSosListManager::Warn(const std::string &strWarnInfo)
{
	m_pCMQRVirtualClient->Warn(strWarnInfo); 
}

void CMQRSosListManager::Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
		                     const timeval_a &tStart, int nCode, const std::string &strIpAddrs)
{
	m_pCMQRVirtualClient->Log(strGuid, dwServiceId, dwMsgId, tStart, nCode, strIpAddrs);
}
	

void CMQRSosListManager::Dump()
{
	MQR_XLOG(XLOG_NOTICE, "CMQRSosListManager::%s\n", __FUNCTION__);
	for (std::map<int, CMQRSosList*>::iterator iter=m_mapServiceIdAndSosList.begin(); 
	     iter!=m_mapServiceIdAndSosList.end(); ++iter)
	{
		iter->second->Dump();
	}
}
