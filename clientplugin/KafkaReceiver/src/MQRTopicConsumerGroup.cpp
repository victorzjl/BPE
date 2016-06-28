#include "MQRTopicConsumerGroup.h"
#include "MQRPartitionConsumerThread.h"
#include "MQRSosList.h"
#include "AsyncVirtualClientLog.h"
#include "ErrorMsg.h"
#include "MQRCommon.h"

CMQRTopicConsumerGroup::~CMQRTopicConsumerGroup()
{
	
}

void CMQRTopicConsumerGroup::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s\n", __FUNCTION__);
	for (std::map<unsigned int, CMQRPartitionConsumerThread *>::iterator iter=m_mapPartitionConsumer.begin(); iter!=m_mapPartitionConsumer.end(); ++iter)
	{
		++dwAllNum;
		if (iter->second->GetSelfCheckState())
		{
			++dwAliveNum;
		}
	}
}

int CMQRTopicConsumerGroup::Initialize(const std::string& topic, unsigned int hostNumber, unsigned int hostNode)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s. Topic[%s], HostNumber[%u], HostNode[%u]\n", __FUNCTION__, topic.c_str(), hostNumber, hostNode);
	m_topic = topic;	
	CZooKeeper* pZooKeeper = m_pMQRSosList->GetZooKeeper();
	if (NULL == pZooKeeper)
	{
		MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s. Can't get zookeeper connection for use.\n", __FUNCTION__);
		return MQR_ZOOKEEPER_CONNECT_ERROR;
	}
	
	m_partitionNum = pZooKeeper->GetPartitionNumber(m_topic);
	std::string brokerList = pZooKeeper->GetBrokerList(m_topic);
	
	unsigned int num = m_partitionNum / hostNumber;
	unsigned int start = num * (hostNode - 1);
	if (hostNumber == hostNode)
	{
		num += m_partitionNum % hostNumber;
	}
	unsigned int end = start + num;
	
	for (unsigned int iIter=start; iIter<end; ++iIter)
	{
		CMQRPartitionConsumerThread *pMQRPartitionConsumerThread = new CMQRPartitionConsumerThread(this);
		unsigned int startOffset = 0;
		m_pMQRSosList->GetZooKeeper()->GetConfirmOffset(m_topic, iIter, startOffset)? 0:(startOffset = 0);
		pMQRPartitionConsumerThread->Initialize(brokerList, m_topic, iIter, startOffset);
		m_mapPartitionConsumer[iIter] = pMQRPartitionConsumerThread;
	}
	
	return 0;
}

int CMQRTopicConsumerGroup::Start()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s\n", __FUNCTION__);
	for (std::map<unsigned int, CMQRPartitionConsumerThread *>::iterator iter=m_mapPartitionConsumer.begin(); iter!=m_mapPartitionConsumer.end(); ++iter)
	{
		iter->second->Start();
	}
	
	return 0;
}

void CMQRTopicConsumerGroup::Stop()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s\n", __FUNCTION__);
}

void CMQRTopicConsumerGroup::SendRequest(void *pRequest, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s. pRequest[%p], len[%d]\n", __FUNCTION__, pRequest, len);
	m_pMQRSosList->SendRequest(pRequest, len, extend);
}

int CMQRTopicConsumerGroup::HandleResponse(const void *pBuffer, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s. pBuffer[%p], len[%d]\n", __FUNCTION__, pBuffer, len);
	unsigned int partition = ((SConsumeInfo *)extend.pData)->partition;
	std::map<unsigned int, CMQRPartitionConsumerThread *>::iterator iter = m_mapPartitionConsumer.find(partition);
	if (iter == m_mapPartitionConsumer.end())
	{
		MQR_XLOG(XLOG_ERROR, "CMQRTopicConsumerGroup::%s. Can't find the request corresponding to the request\n", __FUNCTION__, pBuffer, len);
		return MQR_NO_CORRESPONDING_REQUEST;
	}
	
	return iter->second->OnHandleReponse(pBuffer, len, extend);
}

int CMQRTopicConsumerGroup::ConfirmOffset(const std::string& topic, unsigned int partition, unsigned int offset)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRTopicConsumerGroup::%s. Topic[%p], Partition[%u], Offset[%u]\n", __FUNCTION__, topic.c_str(), partition, offset);
	return m_pMQRSosList->ConfirmOffset(topic, partition, offset);
}

void CMQRTopicConsumerGroup::Warn(const std::string &strWarnInfo)
{
	m_pMQRSosList->Warn(strWarnInfo); 
}

void CMQRTopicConsumerGroup::Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
		                         const timeval_a &tStart, int nCode, const std::string &strIpAddrs)
{
	m_pMQRSosList->Log(strGuid, dwServiceId, dwMsgId, tStart, nCode, strIpAddrs);
}

void CMQRTopicConsumerGroup::Dump()
{
	MQR_XLOG(XLOG_NOTICE, "CMQRSosList::%s\n", __FUNCTION__);
	MQR_XLOG(XLOG_NOTICE, "CMQRSosList::%s < Topic[%s] >\n", __FUNCTION__, m_topic.c_str());
	MQR_XLOG(XLOG_NOTICE, "CMQRSosList::%s < BrokerList[%s] >\n", __FUNCTION__, m_brokerList.c_str());
	MQR_XLOG(XLOG_NOTICE, "CMQRSosList::%s < PartitionNum[%d] >\n", __FUNCTION__, m_partitionNum);
	
	for (std::map<unsigned int, CMQRPartitionConsumerThread *>::iterator iter=m_mapPartitionConsumer.begin();
	     iter!=m_mapPartitionConsumer.end(); ++iter)
	{
		iter->second->Dump();
	}
}
