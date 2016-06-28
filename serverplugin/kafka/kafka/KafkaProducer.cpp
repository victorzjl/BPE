#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "KafkaProducer.h"
#include "AsyncVirtualServiceLog.h"

#define KAFKA_PRODUCER_INIT_STATUS -10000

int32_t CHashPartitionerCb::partitioner_cb(const RdKafka::Topic *topic, const std::string *key,
                                           int32_t partition_cnt, void *msg_opaque)
{
	KAFKA_XLOG(XLOG_DEBUG, "CHashPartitionerCb::%s, key[%s], partition_cnt[%d]\n", __FUNCTION__, key->c_str(), partition_cnt);
	srand(time(NULL));
    unsigned int hash = rand();
    for (size_t i = 0 ; i < key->length() ; i++)
	{
		hash = ((hash << 5) + hash) + key->c_str()[i];
	}
	KAFKA_XLOG(XLOG_DEBUG, "CHashPartitionerCb::%s, hash[%d]\n", __FUNCTION__, hash);

	return hash % partition_cnt;
}


CKafkaProducer::CKafkaProducer(IKafkaErrorCallBack* pCallBack)
	: m_producer(NULL),
	  m_topic(NULL),
	  m_conf(NULL),
	  m_topicConf(NULL),
	  m_pCallBack(pCallBack)
{
	m_IsConnected = false;	
}

CKafkaProducer::CKafkaProducer(IKafkaErrorCallBack* pCallBack, const SKafkaBTP& kafkaBTP)
	: m_producer(NULL),
	  m_topic(NULL),
	  m_conf(NULL),
	  m_topicConf(NULL),
	  m_pCallBack(pCallBack)
{
	m_IsConnected = false;	
	Initialize(kafkaBTP);
}


bool CKafkaProducer::Initialize(const SKafkaBTP& kafkaBTP, int timeout)
{
	KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s\n", __FUNCTION__);
	m_kafkaBTP = kafkaBTP;
	m_timeout = timeout;
	return true;
}

bool CKafkaProducer::Connect()
{
	KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s\n", __FUNCTION__);
	
	if (m_kafkaBTP.brokerlist.empty() || m_kafkaBTP.topic.empty())
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Configure is empty: brokerlist[%s] topic[%s]\n", 
		                        __FUNCTION__, m_kafkaBTP.brokerlist.c_str(), m_kafkaBTP.topic.c_str());
		return false;
	}
	
	m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    m_topicConf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
	
	
	std::string errStr;
	
	if (RdKafka::Conf::CONF_OK != m_conf->set("metadata.broker.list", m_kafkaBTP.brokerlist, errStr))
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to set broker list: brokerlist[%s] error[%s]\n", __FUNCTION__, m_kafkaBTP.brokerlist.c_str(), errStr.c_str());
		Close();
		return false;
	}
	
    if (RdKafka::Conf::CONF_OK != m_conf->set("dr_cb", (RdKafka::DeliveryReportCb*)this, errStr))
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to register delivery report callback: error[%s]\n", __FUNCTION__, errStr.c_str());
		Close();
		return false;	
	}
	
	if (RdKafka::Conf::CONF_OK != m_conf->set("event_cb", (RdKafka::EventCb*)this, errStr))
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to register event callback: error[%s]\n", __FUNCTION__, errStr.c_str());
		Close();
		return false;	
	}
	
	m_producer = RdKafka::Producer::create(m_conf, errStr);
    if (m_producer) 
	{
		if (RdKafka::Conf::CONF_OK != m_topicConf->set("partitioner_cb", &m_hashPartitionerCb, errStr)) 
		{ 
			KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to set partitioner callback: error[%s]\n", __FUNCTION__, errStr.c_str());
			Close();
			return false;
        }
		
		m_topic = RdKafka::Topic::create(m_producer, m_kafkaBTP.topic, m_topicConf, errStr);
		if (!m_topic) 
		{
			KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to create topic: error[%s]\n", __FUNCTION__, errStr.c_str());
			Close();
			return false;
		}
    }
	else
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to create producer: error[%s]\n", __FUNCTION__, errStr.c_str());
		Close();
		return false;
	}
	

	m_producer->poll(m_timeout);
	if (!m_IsConnected)
	{
		Close();
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Timeout to Connect to brokerlist[%s].\n", __FUNCTION__, m_kafkaBTP.brokerlist.c_str());
		return false;
	}

	KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s Created producer[%s]\n", __FUNCTION__, m_producer->name().c_str());
	return true;
}

bool CKafkaProducer::Close()
{
		m_IsConnected = false;
	if (NULL != m_producer)
	{
		delete m_producer;
		m_producer = NULL;
	}
	
	if (NULL != m_topic)
	{
		delete m_topic;
		m_topic = NULL;
	}
	
	
	if (NULL != m_conf)
	{
		delete m_conf;
		m_conf = NULL;
	}
	
	
	if (NULL != m_topicConf)
	{
		delete m_topicConf;
		m_topicConf = NULL;
	}
	
	return true;
}

CKafkaProducer::~CKafkaProducer()
{
	Close();
}

bool CKafkaProducer::PutOneMessage(void* pMessage, size_t len, int timeout)
{
	KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s\n", __FUNCTION__);
	if ((NULL == m_topic) 
		|| (NULL == m_producer)
		|| (NULL == pMessage)
		|| (0 == len))
	{
		return false;
	}
	
	int produceStatus = KAFKA_PRODUCER_INIT_STATUS;
	RdKafka::ErrorCode resp = m_producer->produce(m_topic, m_kafkaBTP.partition,
			  RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
			  const_cast<char *>((char *)pMessage), len, NULL, &produceStatus);
			  
    if (resp != RdKafka::ERR_NO_ERROR)
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Produce failed: [%s]\n", __FUNCTION__, RdKafka::err2str(resp).c_str());
		return false;
	}
	else
	{
		KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s Produced message(%d bytes)\n", __FUNCTION__, len);
	}
	

	m_producer->poll(timeout);

	
	if (RdKafka::ERR_NO_ERROR == produceStatus)
	{
		return true;
	}
	else if (KAFKA_PRODUCER_INIT_STATUS == produceStatus)
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Timeout to produce message\n", __FUNCTION__);
		return false;
	}
	else
	{
		KAFKA_XLOG(XLOG_WARNING, "CKafkaProducer::%s Failed to produce message, errcode[%d]\n", __FUNCTION__, produceStatus);
		return false;
	}
}

void CKafkaProducer::dr_cb(RdKafka::Message &message) 
{
	KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s Message delivery (%d bytes) result: code[%d], return_message[%s], partition[%d], offset[%d]\n",  
	                        __FUNCTION__, message.len(), message.err(), message.errstr().c_str(), message.partition(), message.offset());
	*(int *)message.msg_opaque() = message.err();
}

void CKafkaProducer::event_cb(RdKafka::Event &event) 
{
	KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s\n", __FUNCTION__);
    switch (event.type())
    {
		case RdKafka::Event::EVENT_ERROR:
		{
			KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s ERROR(%s):%s\n", __FUNCTION__, RdKafka::err2str(event.err()).c_str(), event.str().c_str());
			m_pCallBack->OnKafkaError(this, event.str());
			break;
		}
		case RdKafka::Event::EVENT_STATS:
		{
			KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s STATS:%s\n", __FUNCTION__, event.str().c_str());
			break;
		}
		case RdKafka::Event::EVENT_LOG:
		{
			KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s LOG-%d-%s[%s]\n", __FUNCTION__, event.severity(), event.fac().c_str(), event.str().c_str());
			if ((RdKafka::Event::EVENT_SEVERITY_NOTICE == event.severity())
				&& ("CONNECTED" == event.fac()))
			{
				m_IsConnected = true;
			}
			if (RdKafka::Event::EVENT_SEVERITY_ERROR == event.severity())
			{
				m_pCallBack->OnKafkaError(this, event.str());
			}
			break;
		}
		default:
		{
			KAFKA_XLOG(XLOG_DEBUG, "CKafkaProducer::%s EVENT %d(%s):%s\n", __FUNCTION__, event.type(), RdKafka::err2str(event.err()).c_str(), event.str().c_str());
			break;
		}
    }
}
	
	