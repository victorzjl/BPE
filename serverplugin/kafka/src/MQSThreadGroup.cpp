#include "MQSThreadGroup.h"
#include "AsyncVirtualServiceLog.h"
#include "MQSVirtualService.h"
#include "MQSThread.h"
#include "ErrorMsg.h"
#include "Common.h"
#include "ZooKeeper.h"

#include <algorithm>
#include <XmlConfigParser.h>
#include <boost/algorithm/string.hpp>


using namespace boost::algorithm;


CMQSThreadGroup::~CMQSThreadGroup()
{
	Stop();
}

void CMQSThreadGroup::Stop()
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThreadGroup::%s\n", __FUNCTION__);
	vector<CMQSThread *>::iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		if(*iter != NULL)
		{
			(*iter)->Stop();
			
			delete *iter;
			*iter = NULL;
		}
	}
	m_vecChildThread.clear();
}

int CMQSThreadGroup::Start( unsigned int dwThreadNum, const std::vector<std::string> &vecMQSenderSvr, std::vector<unsigned int>& vecServiceIds )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThreadGroup::%s, ThreadNum[%d]\n", __FUNCTION__, dwThreadNum);

	int nRet = 0;
	bool bSuccess = false;
	
	//解析配置
	std::vector<SKafkaBTP> vecKafkaBTP;
	std::vector<PairServiceInfo> vecServiceInfo;
	
	for (std::vector<string>::const_iterator iter = vecMQSenderSvr.begin();
	     iter != vecMQSenderSvr.end(); ++iter)
	{
		CXmlConfigParser dataParser;
		dataParser.ParseDetailBuffer(iter->c_str());
		
		std::vector<string> vecAddrs = dataParser.GetParameters("ServerAddr");
		std::string zookeeperAddr;
		for (std::vector<string>::iterator iterTemp = vecAddrs.begin(); iterTemp != vecAddrs.end(); iterTemp++)
		{
			if (iterTemp != vecAddrs.begin())
			{
				zookeeperAddr += ",";
			}
			zookeeperAddr += *iterTemp;
		}
		CZooKeeper *pZooKeeper = new CZooKeeper(this);
		m_vecZooKeeper.push_back(pZooKeeper);
		
		std::vector<string> vecServiceTopics = dataParser.GetParameters("ServiceTopic");
		pZooKeeper->Initialize(zookeeperAddr);
		if (!pZooKeeper->Connect())
		{
			m_mapInoperativeConfig[pZooKeeper] = vecServiceTopics;
			continue;
		}

		for (std::vector<string>::iterator iterTemp=vecServiceTopics.begin(); iterTemp!=vecServiceTopics.end(); iterTemp++)
		{
			SKafkaBTP tempKafkaBTP;
			size_t pos = iterTemp->find_first_of(':');
			if ( (pos == string::npos)
				|| (pos == 0) )
			{
				MQS_XLOG(XLOG_WARNING, "CMQSThreadGroup::%s item <ServiceTopic> configure error\n", __FUNCTION__);
				return MQS_CONFIG_ERROR; 
			}
			tempKafkaBTP.topic = iterTemp->substr(0, pos);
			std::string serviceIds = iterTemp->substr(pos+1);
			std::vector<string> vecStrServiceIds;
			split(vecStrServiceIds, serviceIds, is_any_of(","), boost::algorithm::token_compress_on);
			if (vecStrServiceIds.empty())
			{
				MQS_XLOG(XLOG_WARNING, "CMQSThreadGroup::%s item <ServiceTopic> configure error\n", __FUNCTION__);
				return MQS_CONFIG_ERROR; 
			}
			
			tempKafkaBTP.brokerlist = pZooKeeper->GetBrokerList(tempKafkaBTP.topic);

			std::vector<unsigned int> vecIServiceIds;
			for (std::vector<string>::iterator serviceIdIter=vecStrServiceIds.begin(); serviceIdIter!=vecStrServiceIds.end(); serviceIdIter++)
			{
				string &strServiceId = *serviceIdIter;
				trim(strServiceId);
				unsigned int dwServiceId = atoi(strServiceId.c_str());
				vecIServiceIds.push_back(dwServiceId);
				m_vecServiceIds.push_back(dwServiceId);
			}
			
			vecServiceInfo.push_back(std::make_pair(tempKafkaBTP, vecIServiceIds));
		}
	}
	
	sort(m_vecServiceIds.begin(), m_vecServiceIds.end());
	vecServiceIds = m_vecServiceIds;
	
	//启动线程组
	for(unsigned int i=0;i<dwThreadNum;i++)
	{
		CMQSThread *pMQSThread = new CMQSThread(m_queue, this);
		m_vecChildThread.push_back(pMQSThread);
		nRet = pMQSThread->Start(vecServiceInfo);
		if(nRet != 0)
		{
			MQS_XLOG(XLOG_ERROR, "CMQSThreadGroup::%s, MQSenderThread start error\n", __FUNCTION__);
		}
		else
		{
			bSuccess = true;
		}
	}

	if(bSuccess)
	{
		return 0;
	}
	else
	{
		return nRet;
	}
}

void CMQSThreadGroup::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId,
                                const std::string &strGuid, void *pHandler, const void *pBuffer, int nLen)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThreadGroup::%s, ServiceId[%d], MsgId[%d], SequenceId[%d], Guid[%s], Handler[%p], pBuffer[%p], Len[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, dwSequenceId, strGuid.c_str(), pHandler, pBuffer, nLen);
	
	timeval_a tStart;
	gettimeofday_a(&tStart, 0);

	SMQSMsg *pMQSMsg = new SMQSProduceMsg;
	
	pMQSMsg->dwServiceId = dwServiceId;
	pMQSMsg->dwMsgId = dwMsgId;
	pMQSMsg->dwSequenceId = dwSequenceId;

	pMQSMsg->strGuid = strGuid;
	pMQSMsg->handler = pHandler;
	pMQSMsg->tStart = tStart;

	pMQSMsg->pBuffer = malloc(nLen);
	pMQSMsg->dwLen = nLen;
	memcpy(pMQSMsg->pBuffer, pBuffer, nLen);

	if(m_queue.IsFull())
	{
		MQS_XLOG(XLOG_ERROR, "CMQSThreadGroup::%s, read queue full\n", __FUNCTION__);
		m_pMQSVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, MQS_QUEUE_FULL, "");
		m_pMQSVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, MQS_QUEUE_FULL);
		return;
	}
	else
	{
		m_queue.PutQ(pMQSMsg);
	}
}

void CMQSThreadGroup::Response(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, 
	                           unsigned int dwSequenceId, int nCode)
{
  m_pMQSVirtualService->Response(handler, dwServiceId, dwMsgId, dwSequenceId, nCode);
}

void CMQSThreadGroup::Warn(const std::string &strWarnInfo)
{
	m_pMQSVirtualService->Warn(strWarnInfo);
}

void CMQSThreadGroup::Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	                      const timeval_a &tStart, int nCode, const std::string &strIpAddrs)
{
	m_pMQSVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, nCode, strIpAddrs);
}

int CMQSThreadGroup::OnReConnSuccEvent(CZooKeeper *pZooKeeper)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThreadGroup::%s\n", __FUNCTION__);
	std::map< CZooKeeper*, std::vector<string> >::iterator iter = m_mapInoperativeConfig.find(pZooKeeper);
	if (iter == m_mapInoperativeConfig.end())
	{
		return -1;
	}
	
	vector<PairServiceInfo> vecServiceInfo;
	
	for (std::vector<string>::iterator iterConfig=iter->second.begin();
	     iterConfig!=iter->second.end(); ++iterConfig)
	{
		SKafkaBTP tempKafkaBTP;
		size_t pos = iterConfig->find_first_of(':');
		if ( (pos == std::string::npos)
			|| (pos == 0) )
		{
			MQS_XLOG(XLOG_WARNING, "CMQSThreadGroup::%s item <ServiceTopic> configure error\n", __FUNCTION__);
			return -1; 
		}
		tempKafkaBTP.topic = iterConfig->substr(0, pos);
		std::string serviceIds = iterConfig->substr(pos+1);
		std::vector<string> vecStrServiceIds;
		split(vecStrServiceIds, serviceIds, is_any_of(","), boost::algorithm::token_compress_on);
		if (vecStrServiceIds.empty())
		{
			MQS_XLOG(XLOG_WARNING, "CMQSThreadGroup::%s item <ServiceTopic> configure error\n", __FUNCTION__);
			return -1; 
		}
		
		tempKafkaBTP.brokerlist = pZooKeeper->GetBrokerList(tempKafkaBTP.topic);

		std::vector<unsigned int> vecIServiceIds;
		for (std::vector<std::string>::iterator serviceIdIter=vecStrServiceIds.begin(); 
		     serviceIdIter!=vecStrServiceIds.end(); serviceIdIter++)
		{
			std::string &strServiceId = *serviceIdIter;
			trim(strServiceId);
			unsigned int dwServiceId = atoi(strServiceId.c_str());
			vecIServiceIds.push_back(dwServiceId);
			m_vecServiceIds.push_back(dwServiceId);
		}
		
		vecServiceInfo.push_back(std::make_pair(tempKafkaBTP, vecIServiceIds));
	}
	
	m_mapInoperativeConfig.erase(iter);
	
	for(std::vector<CMQSThread *>::iterator iterThread=m_vecChildThread.begin();
	    iterThread!=m_vecChildThread.end(); ++iterThread)
	{
		if(0 != (*iterThread)->OnAddService(vecServiceInfo))
		{
			MQS_XLOG(XLOG_WARNING, "CMQSThreadGroup::%s, failed to add service \n", __FUNCTION__);
		}
	}
	
	return 0;
}

void CMQSThreadGroup::GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThreadGroup::%s\n", __FUNCTION__);
	dwAliveNum = 0;
	dwAllNum = m_vecChildThread.size();
	vector<CMQSThread *>::const_iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		if((*iter)->GetSelfCheck())
		{
			dwAliveNum ++;
		}
	}
}

void CMQSThreadGroup::Dump()
{
	MQS_XLOG(XLOG_NOTICE, "CMQSThreadGroup::%s\n", __FUNCTION__);
	MQS_XLOG(XLOG_NOTICE, "CMQSThreadGroup::%s < SupportedServiceIds[%s] >\n", __FUNCTION__, GetServiceIdString(m_vecServiceIds).c_str());
	
	for (std::vector<CMQSThread *>::iterator iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); ++iter)
	{
		(*iter)->Dump();
	}
}



