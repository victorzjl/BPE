#ifndef _MQR_PARTITION_CONSUMER_THREAD_H_
#define _MQR_PARTITION_CONSUMER_THREAD_H_

#include <string>
#include <vector>

#include "MsgTimerThread.h"
#include "KafkaConsumer.h"
#include "MQRMsg.h"
#include "MQRCommon.h"
#include "IReConnHelp.h"

#include <netinet/in.h>

#define MAX_CONSUME_BUFFER 1280

using namespace sdo::common;

typedef struct stMessage
{
	char buffer[MAX_CONSUME_BUFFER];
    unsigned int contentLength;
	unsigned int offset;
	bool isAck;
	
	stMessage():contentLength(MAX_CONSUME_BUFFER), isAck(true) {}
}SMessage;

class CMQRConsumerReConnHelperImpl : public IReConnHelper, public IKafkaErrorCallBack
{
public:
	CMQRConsumerReConnHelperImpl() {}
	
	virtual int TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3);
	virtual int ReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3);
	
	int OnKafkaError(IKafka *, const std::string&) { return 0; }
};

class CMQRTopicConsumerGroup;
class CMQRPartitionConsumerThread : public CMsgTimerThread, public IKafkaErrorCallBack,
				                    public IReConnOwner
{
	typedef void (CMQRPartitionConsumerThread::*MQRFunc)(SMQRMsg *pMQRMsg);
public:
	CMQRPartitionConsumerThread(CMQRTopicConsumerGroup *pMQRTopicConsumerGroup);
	virtual ~CMQRPartitionConsumerThread();
	
	int Initialize(const std::string& brokerList, const std::string& topic,
                   unsigned int partition, unsigned int startOffset);	   
	bool GetSelfCheckState();
	int Start();
	void StartInThread();
	void Stop();
	
	void Dump();
	
	
	int OnReConn(void *pContext);
	int OnKafkaError(IKafka *, const std::string&);
	int OnConsumeMessage();
	int OnHandleReponse(const void *pBuffer, int len, const SExtendData& extend);
	virtual void Deal(void *pData);
	
protected:
	int OnConfirmOffset();
	int OnReSendMessage();
	
private:
    void DoSelfCheck();
	void DoConsumeMessage(SMQRMsg *pMQRMsg);
	void DoHandleReponse(SMQRMsg *pMQRMsg);
	void DoReSendMessage(SMQRMsg *pMQRMsg);
	void DoHandleTimeOutMessage(SMQRMsg *pMQRMsg);
	void DoConfirmOffset(SMQRMsg *pMQRMsg);
	void DoReConn(SMQRMsg *pMQRMsg);
	void DoStop(SMQRMsg *pMQRMsg);
	void SendMessage();
	
private:
	MQRFunc                  m_MQRFuncs[MQR_ALL];
	CMQRTopicConsumerGroup  *m_pMQRTopicConsumerGroup;
	CKafkaConsumer          *m_pConsumer;
	SKafkaBTP                m_kafkaBTP;
	SMessage                 m_message;
	
	//for confirming offset
	unsigned int             m_confirmOffset;
	bool                     m_bIsNeedConfirm;
	CThreadTimer	         m_timerConfirmCheck;
	bool                     m_bIsConfirmCheckTimerStopped;
	
	//for checking alive state
	CThreadTimer	         m_timerSelfCheck;
	bool                     m_bIsSelfCheckTimerStopped;
	bool			         m_isAlive;
	
	CMQRConsumerReConnHelperImpl m_reConnHelper;
};

#endif //_MQR_PARTITION_CONSUMER_THREAD_H_
