#include "MQRPartitionConsumerThread.h"
#include "MQRTopicConsumerGroup.h"
#include "ReConnThread.h"
#include "AsyncVirtualClientLog.h"
#include "ErrorMsg.h"

#include "SapMessage.h"
#include <cstdlib>
using namespace sdo::sap;


int CMQRConsumerReConnHelperImpl::TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout)
{
	if (NULL == pContext)
	{
		return -1;
	}
	
	CKafkaConsumer *pConsumer = (CKafkaConsumer *)pContext;

	CKafkaConsumer consumer(this);
	consumer.Initialize(pConsumer->GetKafkaBTP(), pConsumer->GetStartOffset());
	if (!consumer.Connect())
	{
		return -1;
	}

	char buffer[MAX_CONSUME_BUFFER];
    unsigned int contentLength = MAX_CONSUME_BUFFER;
	unsigned int offset;
	if (!consumer.GetOneMessage((void*)buffer, contentLength, offset))
	{
		return -1;
	}
	
	return 0;
}

int CMQRConsumerReConnHelperImpl::ReConnect(void *pContext, std::string& outMsg, unsigned int timeout)
{
	if (NULL == pContext)
	{
		return -1;
	}
	
	CKafkaConsumer *pConsumer = (CKafkaConsumer *)pContext;
	if (!pConsumer->Connect())
	{
		pConsumer->Close();
		return -1;
	}
	return 0;
}

CMQRPartitionConsumerThread::CMQRPartitionConsumerThread(CMQRTopicConsumerGroup *pMQRTopicConsumerGroup)
	:m_pMQRTopicConsumerGroup(pMQRTopicConsumerGroup),
	 m_pConsumer(NULL),
	 m_confirmOffset(0),
	 m_bIsNeedConfirm(false), 
	 m_timerConfirmCheck(this, 1, boost::bind(&CMQRPartitionConsumerThread::OnConfirmOffset,this),CThreadTimer::TIMER_CIRCLE),
	 m_bIsConfirmCheckTimerStopped(true), 
	 m_timerSelfCheck(this, 3, boost::bind(&CMQRPartitionConsumerThread::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE),
	 m_bIsSelfCheckTimerStopped(true),
	 m_isAlive(true),
	 m_reConnHelper()
{
	m_MQRFuncs[MQR_CONSUME] = &CMQRPartitionConsumerThread::DoConsumeMessage;
	m_MQRFuncs[MQR_RESPONSE] = &CMQRPartitionConsumerThread::DoHandleReponse;
	m_MQRFuncs[MQR_RECONSUME] = &CMQRPartitionConsumerThread::DoReSendMessage;
	m_MQRFuncs[MQR_TIMEOUT] = &CMQRPartitionConsumerThread::DoHandleTimeOutMessage;
	m_MQRFuncs[MQR_CONFIRM_OFFSET] = &CMQRPartitionConsumerThread::DoConfirmOffset;
	m_MQRFuncs[MQR_RECONN_SERVER] = &CMQRPartitionConsumerThread::DoReConn;
	m_MQRFuncs[MQR_STOP] = &CMQRPartitionConsumerThread::DoStop;
}

CMQRPartitionConsumerThread::~CMQRPartitionConsumerThread()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.", __FUNCTION__);
	Stop();
	delete m_pConsumer;
	m_pConsumer = NULL;
}

void CMQRPartitionConsumerThread::DoSelfCheck()
{
	m_isAlive = true;
}

bool CMQRPartitionConsumerThread::GetSelfCheckState() 
{
	bool flag = m_isAlive;
	m_isAlive = false;
	return flag;
}

int CMQRPartitionConsumerThread::Initialize(const std::string& brokerList, const std::string& topic, unsigned int partition, unsigned int startOffset)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. BrokerList[%s], Topic[%s], Partition[%u]\n", __FUNCTION__,
	                      brokerList.c_str(), topic.c_str(), partition);
	m_kafkaBTP.brokerlist = brokerList;
	m_kafkaBTP.topic = topic;
	m_kafkaBTP.partition = partition;
	
	m_pConsumer = new CKafkaConsumer(this);
	m_pConsumer->Initialize(m_kafkaBTP, startOffset);
	if (!m_pConsumer->Connect())
	{
		MQR_XLOG(XLOG_ERROR, "CMQRPartitionConsumerThread::%s. Failed to connect server[%s] for topic[%s]:partition[%u]\n", __FUNCTION__,
	                      brokerList.c_str(), topic.c_str(), partition);
		CReConnThread::GetReConnThread()->OnReConn(this, &m_reConnHelper, m_pConsumer);
		return MQR_KAFKA_CONNECT_ERROR;
	}
	return 0;
}

int CMQRPartitionConsumerThread::Start()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	OnConsumeMessage();
	CMsgTimerThread::Start();
	return 0;	
}

void CMQRPartitionConsumerThread::StartInThread()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	
	m_timerSelfCheck.Start();
	m_bIsSelfCheckTimerStopped = false;
	
	m_timerConfirmCheck.Start();
	m_bIsConfirmCheckTimerStopped = false;
}

void CMQRPartitionConsumerThread::Stop()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	m_pConsumer->Close();
	if (m_bIsNeedConfirm)
	{
		m_pMQRTopicConsumerGroup->ConfirmOffset(m_kafkaBTP.topic, m_kafkaBTP.partition, m_confirmOffset);
	}
	
	SMQRStopMsg *pMQRStopMsg = new SMQRStopMsg;
	PutQ(pMQRStopMsg);
	
	do 
	{
		sleep(1);
	}while((!m_bIsSelfCheckTimerStopped)||(!m_bIsConfirmCheckTimerStopped));
	
	CMsgTimerThread::Stop();
}

int CMQRPartitionConsumerThread::OnReConn(void *pContext)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRReConnServerMsg *pMsg = new SMQRReConnServerMsg;
	PutQ(pMsg);
	return 0;
}

int CMQRPartitionConsumerThread::OnKafkaError(IKafka *, const std::string&)
{
	/* MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	CReConnThread::GetReConnThread()->OnReConn(this, &m_reConnHelper, m_pConsumer); */
	return 0;
}

int CMQRPartitionConsumerThread::OnConsumeMessage()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRConsumeMsg *pMsg = new SMQRConsumeMsg;
	PutQ(pMsg);
	return 0;
}

int CMQRPartitionConsumerThread::OnHandleReponse(const void *pBuffer, int len, const SExtendData& extend)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. pBuffer[%p], len[%d]\n", __FUNCTION__, pBuffer, len);
	SMQRResponseMsg *pMsg = new SMQRResponseMsg;
	pMsg->info = *((SConsumeInfo *)extend.pData);
	PutQ(pMsg);
	return 0;
}

int CMQRPartitionConsumerThread::OnReSendMessage()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRReConsumeMsg *pMsg = new SMQRReConsumeMsg;
	pMsg->offset = m_message.offset;
	PutQ(pMsg);
	return 0;
}

int CMQRPartitionConsumerThread::OnConfirmOffset()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRConfirmOffsetMsg *pMsg = new SMQRConfirmOffsetMsg;
	PutQ(pMsg);
	return 0;
}

void CMQRPartitionConsumerThread::Deal(void *pData)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRMsg *pMQRMsg = (SMQRMsg *)pData;
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. operator[%d]\n", __FUNCTION__, pMQRMsg->m_enMQROperator);
	(this->*(m_MQRFuncs[pMQRMsg->m_enMQROperator]))(pMQRMsg);
	delete pMQRMsg;
}

void CMQRPartitionConsumerThread::DoConsumeMessage(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	if (!m_pConsumer->IsConnected())
	{
		MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. Connection is lost.\n", __FUNCTION__);
		sleep(3);
		OnConsumeMessage();
		return;
	}
	
	unsigned int offset;
	m_message.contentLength=MAX_CONSUME_BUFFER;
	bool bGetMsg = m_pConsumer->GetOneMessage(m_message.buffer, m_message.contentLength, offset);
	
	if (!bGetMsg)
	{
		m_pConsumer->Close();
		CReConnThread::GetReConnThread()->OnReConn(this, &m_reConnHelper, m_pConsumer);
	}
	else
	{
		MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. ContentLength[%d]\n", __FUNCTION__, m_message.contentLength);
		if ((0 == m_message.contentLength)
		    || (m_confirmOffset == offset))
		{	
			sleep(1);
			OnConsumeMessage();
		}
		else
		{
			m_message.isAck = false;
			
			/* CSapEncoder request;			
			request.SetService(0xA1, 40001, offset);
			std::string msg(m_message.buffer, m_message.contentLength);
			memcpy(m_message.buffer, request.GetBuffer(), request.GetLen()); */
			
			m_message.offset = offset;
			SendMessage();
		}
	}
}

void CMQRPartitionConsumerThread::DoHandleReponse(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRResponseMsg *pMsg = (SMQRResponseMsg *)pMQRMsg;
	if ((pMsg->info.offset != m_message.offset) || (m_message.isAck == true))
	{
		return;
	}
	
	if (pMsg->info.isAck)
	{
		m_message.isAck = true;
		m_bIsNeedConfirm = true;
		m_confirmOffset = m_message.offset;
		m_pConsumer->SetStartOffset(m_confirmOffset);
		OnConsumeMessage();
	}
	else
	{
		OnReSendMessage();
	}
}

void CMQRPartitionConsumerThread::DoReSendMessage(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRReConsumeMsg *pMsg = (SMQRReConsumeMsg *)pMQRMsg;
	if ((pMsg->offset != m_message.offset) || (m_message.isAck == true))
	{
		return;
	}
	
	SendMessage();
}

void CMQRPartitionConsumerThread::DoHandleTimeOutMessage(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SMQRTimeoutMsg *pMsg = (SMQRTimeoutMsg *)pMQRMsg;
	if ((pMsg->offset != m_message.offset) || (m_message.isAck == true))
	{
		return;
	}
	
	SendMessage();
}

void CMQRPartitionConsumerThread::DoConfirmOffset(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	if (m_bIsNeedConfirm)
	{
		if (0 == m_pMQRTopicConsumerGroup->ConfirmOffset(m_kafkaBTP.topic, m_kafkaBTP.partition, m_confirmOffset))
		{
			m_bIsNeedConfirm = false;
		}
	}
}

void CMQRPartitionConsumerThread::DoReConn(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	
	if (m_pConsumer->IsConnected())
	{
		MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. Has been Connected\n", __FUNCTION__);
		return;
	}
	
	std::string out;
	if (0 != m_reConnHelper.ReConnect((void*)m_pConsumer, out))
	{
		MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s. Still fail, try again.\n", __FUNCTION__);
		CReConnThread::GetReConnThread()->OnReConn(this, &m_reConnHelper, m_pConsumer);
	}
	else
	{
		OnConsumeMessage();
	}
}

void CMQRPartitionConsumerThread::DoStop(SMQRMsg *pMQRMsg)
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s\n", __FUNCTION__);
	m_timerSelfCheck.Stop();
	m_bIsSelfCheckTimerStopped = true;
	m_timerConfirmCheck.Stop();
	m_bIsConfirmCheckTimerStopped = true;
}

void CMQRPartitionConsumerThread::SendMessage()
{
	MQR_XLOG(XLOG_DEBUG, "CMQRPartitionConsumerThread::%s.\n", __FUNCTION__);
	SConsumeInfo info;
	info.partition = m_kafkaBTP.partition;
	info.offset = m_message.offset;
	
	SExtendData extend;
	extend.pData = (void *)&info;
	extend.length = sizeof(info);
	
	m_pMQRTopicConsumerGroup->SendRequest((void *)m_message.buffer, m_message.contentLength, extend);
	
	SMQRTimeoutMsg *pMsg = new SMQRTimeoutMsg;
	pMsg->offset = m_message.offset;
	TimedPutQ(pMsg);
}

void CMQRPartitionConsumerThread::Dump()
{
	MQR_XLOG(XLOG_NOTICE, "CMQRPartitionConsumerThread::%s\n", __FUNCTION__);
	MQR_XLOG(XLOG_NOTICE, "CMQRPartitionConsumerThread::%s < BrokerList[%s] >\n", __FUNCTION__, m_kafkaBTP.brokerlist.c_str());
	MQR_XLOG(XLOG_NOTICE, "CMQRPartitionConsumerThread::%s < Topic[%s] >\n", __FUNCTION__, m_kafkaBTP.topic.c_str());
	MQR_XLOG(XLOG_NOTICE, "CMQRPartitionConsumerThread::%s < Partition[%d] >\n", __FUNCTION__, m_kafkaBTP.partition);
}
