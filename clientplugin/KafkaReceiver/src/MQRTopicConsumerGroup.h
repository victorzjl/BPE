#ifndef _MQR_TOPIC_CONSUMER_GROUP_H_
#define _MQR_TOPIC_CONSUMER_GROUP_H_

#include "MQRPartitionConsumerThread.h"

#include <string>
#include <map>
#include <detail/_time.h>

class CMQRSosList;

using namespace sdo::common;

class CMQRTopicConsumerGroup
{
public:
	CMQRTopicConsumerGroup(CMQRSosList *pMQRSosList):m_pMQRSosList(pMQRSosList){}
	virtual ~CMQRTopicConsumerGroup();
	
	int Initialize(const std::string& topic, unsigned int hostNumber, unsigned int hostNode);
	void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
	
	int Start();
	void Stop();
	
	int ConfirmOffset(const std::string& topic, unsigned int partition, unsigned int offset);
	void SendRequest(void *pRequest, int len, const SExtendData& extend);
	int HandleResponse(const void *pBuffer, int len, const SExtendData& extend);

	void Dump();
	
	void Warn(const std::string &strWarnInfo);
	void Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	         const timeval_a &tStart, int nCode, const std::string &strIpAddrs);
	
private:
	CMQRSosList      *m_pMQRSosList;
	std::string       m_topic;
	int               m_partitionNum;
	std::string       m_brokerList;
	std::map<unsigned int, CMQRPartitionConsumerThread *> m_mapPartitionConsumer;
};

#endif //_MQR_TOPIC_CONSUMER_GROUP_H_
