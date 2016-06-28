#include "AsyncVirtualClientLog.h"
#include "KafkaConsumer.h"

CKafkaConsumer::CKafkaConsumer(IKafkaErrorCallBack* pCallBack)
	: m_consumer(NULL),
	  m_topic(NULL),
	  m_conf(NULL),
	  m_topicConf(NULL),
	  m_pCallBack(pCallBack)
{	
	m_IsConnected = false;	
}

CKafkaConsumer::CKafkaConsumer(IKafkaErrorCallBack* pCallBack, const SKafkaBTP& kafkaBTP)
	: m_consumer(NULL),
	  m_topic(NULL),
	  m_conf(NULL),
	  m_topicConf(NULL),
	  m_pCallBack(pCallBack)
{
    m_IsConnected = false;	
	Initialize(kafkaBTP, 0);
}

CKafkaConsumer::~CKafkaConsumer()
{
	Close();
}

bool CKafkaConsumer::Initialize(const SKafkaBTP& kafkaBTP, unsigned int offset, int timeout)
{
	MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s\n", __FUNCTION__);
	
	m_kafkaBTP = kafkaBTP;
	m_startOffset = offset;
	m_timeout = timeout;
	return true;
}

bool CKafkaConsumer::Connect()
{
	m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    m_topicConf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
	
	
	std::string errStr;
	
	if (RdKafka::Conf::CONF_OK != m_conf->set("metadata.broker.list", m_kafkaBTP.brokerlist, errStr))
	{
		MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s Failed to set broker list: brokerlist[%s] error[%s]\n", __FUNCTION__, 
		         m_kafkaBTP.brokerlist.c_str(), errStr.c_str());
		Close();
		return false;
	}
	
	
	if (RdKafka::Conf::CONF_OK != m_conf->set("event_cb", (RdKafka::EventCb*)this, errStr))
	{
		MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s Failed to register event callback: error[%s]\n", __FUNCTION__, errStr.c_str());
		Close();
		return false;	
	}
	
	m_consumer = RdKafka::Consumer::create(m_conf, errStr);
    if (m_consumer) 
	{
		m_topic = RdKafka::Topic::create(m_consumer, m_kafkaBTP.topic, m_topicConf, errStr);
		if (!m_topic) 
		{
			MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s Failed to create topic: error[%s]\n", __FUNCTION__, errStr.c_str());
			Close();
			return false;
		}
		
		RdKafka::ErrorCode resp = m_consumer->start(m_topic, m_kafkaBTP.partition, m_startOffset);
		if (resp != RdKafka::ERR_NO_ERROR)
		{
			MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s Failed to start: error[%d]\n", __FUNCTION__, resp);
			Close();
			return false;
		}
		else
		{
			m_consumer->poll(m_timeout);
			if (!m_IsConnected)
			{
				Close();
				KAFKA_XLOG(XLOG_WARNING, "CKafkaConsumer::%s Timeout to Connect to brokerlist[%s].\n", __FUNCTION__, m_kafkaBTP.brokerlist.c_str());
				return false;
			}
		}
    }
	else
	{
		MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s Failed to create consumer: error[%s]\n", __FUNCTION__, errStr.c_str());
		Close();
		return false;
	}
	


	MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s Created consumer[%s]\n", __FUNCTION__, m_consumer->name().c_str());
	return true;
}

bool CKafkaConsumer::GetOneMessage(void *pMessage, unsigned int &len, unsigned int &offset, int timeout)
{
	MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s, dist[%p], length[%d], timeout[%d]\n", __FUNCTION__, pMessage, len, timeout);
	bool ret = false;
	RdKafka::Message *message = m_consumer->consume(m_topic, m_kafkaBTP.partition, timeout*1000);
    if (RdKafka::ERR_NO_ERROR == message->err())
	{
		MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s, get one message. Offset[%u], Length[%d], Content[%s].\n", 
		                            __FUNCTION__, (unsigned int)message->offset(), message->len(), message->payload());
		
		if (len >= message->len())
		{
			ret = true;
			memcpy(pMessage, message->payload(), message->len());
			len = message->len();
			offset = message->offset();
		}
		else
		{
			len=0;
			MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s, The buffer of destination is not enough. "
                	 "MessageLength[%d], BufferLength[%d].\n", __FUNCTION__,message->len(), len);
		}
	}
	else if (RdKafka::ERR__PARTITION_EOF == message->err())
	{
		MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s, No Message fetch. Code[%d]\n", __FUNCTION__, message->err());
		len = 0;
		ret = true;
	}
	else
	{
		MQR_XLOG(XLOG_WARNING, "CKafkaConsumer::%s, [%s].\n", __FUNCTION__, message->errstr().c_str());
	}
    delete message;
	return ret;
}

bool CKafkaConsumer::Close()
{
	if (NULL != m_consumer)
	{
		m_consumer->stop(m_topic, m_kafkaBTP.partition);
		delete m_consumer;
		m_consumer = NULL;
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
	
	m_IsConnected = false;
	return true;
}


void CKafkaConsumer::event_cb(RdKafka::Event &event) 
{
	MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s\n", __FUNCTION__);
    switch (event.type())
    {
		case RdKafka::Event::EVENT_ERROR:
		{
			MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s ERROR(%s):%s\n", __FUNCTION__, 
			         RdKafka::err2str(event.err()).c_str(), event.str().c_str());
			m_pCallBack->OnKafkaError(this, event.str());
			break;
		}
		case RdKafka::Event::EVENT_STATS:
		{
			MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s STATS:%s\n", __FUNCTION__, event.str().c_str());
			break;
		}
		case RdKafka::Event::EVENT_LOG:
		{
			MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s LOG-%d-%s[%s]\n", __FUNCTION__, event.severity(), event.fac().c_str(), event.str().c_str());
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
			MQR_XLOG(XLOG_DEBUG, "CKafkaConsumer::%s EVENT %d(%s):%s\n", __FUNCTION__, event.type(), RdKafka::err2str(event.err()).c_str(), event.str().c_str());
			break;
		}
    }
}
