#ifndef _C_KAFKA_PRODUCER_H_
#define _C_KAFKA_PRODUCER_H_

#include <string>

#include "rdkafkacpp.h"
#include "KafkaCommon.h"


class CHashPartitionerCb : public RdKafka::PartitionerCb
{
public:
	int32_t partitioner_cb(const RdKafka::Topic *topic, const std::string *key,
                          int32_t partition_cnt, void *msg_opaque);
};

class CKafkaProducer : public IKafka, public RdKafka::DeliveryReportCb, public RdKafka::EventCb
{
public:
	CKafkaProducer(IKafkaErrorCallBack* pCallBack);
	CKafkaProducer(IKafkaErrorCallBack* pCallBack, const SKafkaBTP& kafkaBTP);
	virtual ~CKafkaProducer();
	
	bool Initialize(const SKafkaBTP& kafkaBTP, int timeout=3);
	virtual bool Connect();
	virtual bool Close();
	const SKafkaBTP& GetKafkaBTP() { return m_kafkaBTP; }
	virtual bool PutOneMessage(void* pMessage, size_t len, int timeout=3);
	
private:
	void dr_cb(RdKafka::Message &message);
	void event_cb(RdKafka::Event &event);
	
private:
	SKafkaBTP m_kafkaBTP;
	
	RdKafka::Producer *m_producer;
	RdKafka::Topic *m_topic;
	RdKafka::Conf *m_conf;
	RdKafka::Conf *m_topicConf;
	
	IKafkaErrorCallBack* m_pCallBack;
	
	CHashPartitionerCb m_hashPartitionerCb;
	int32_t m_partition;
	unsigned int m_timeout;
};

#endif //_C_KAFKA_PRODUCER_H_
