#ifndef _C_KAFKA_CONSUMER_H_
#define _C_KAFKA_CONSUMER_H_

#include "KafkaCommon.h"
#include "rdkafkacpp.h"

class CKafkaConsumer : public IKafka, public RdKafka::EventCb
{
public:
	CKafkaConsumer(IKafkaErrorCallBack* pCallBack);
	CKafkaConsumer(IKafkaErrorCallBack* pCallBack, const SKafkaBTP& kafkaBTP);
	virtual ~CKafkaConsumer();
	
	bool Initialize(const SKafkaBTP& kafkaBTP, unsigned int offset, int timeout=3);
	void SetStartOffset(unsigned int offset) { m_startOffset = offset; }
	unsigned int GetStartOffset() { return m_startOffset; }
	const SKafkaBTP& GetKafkaBTP() { return m_kafkaBTP; }
	
	bool Connect();
	bool Close();
	
	virtual bool GetOneMessage(void *pMessage, unsigned int &len, unsigned int &offset, int timeout=3);
	
	
private:
	void event_cb(RdKafka::Event &event);
	
private:
	SKafkaBTP m_kafkaBTP;
	
	RdKafka::Consumer *m_consumer;
	RdKafka::Topic *m_topic;
	RdKafka::Conf *m_conf;
	RdKafka::Conf *m_topicConf;
	
	IKafkaErrorCallBack* m_pCallBack;
	unsigned int m_startOffset;
	int m_timeout;
};

#endif //_C_KAFKA_CONSUMER_H_
