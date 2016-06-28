#ifndef _MQS_MSG_H_
#define _MQS_MSG_H_

#include <string>
#include <vector>
#include <map>

#include <detail/_time.h>

#include "KafkaCommon.h"

using std::string;
using std::vector;
using std::map;
using std::make_pair;

#define		BASE64_CAPACITY		1024
#define		TIMEOUT_INTERVAL	3
#define		MAX_ASYN_BUFFER		1280
#define		VALUE_SPLITTER		"^_^"

#define		CONHASH_VIRTUAL_NODE 10
#define		MAX_NODE_NUM		 10

typedef enum enMQSOperator
{
	MQS_PRODUCE = 1,
	MQS_STOP = 2,
	MQS_RECONN = 3,
	MQS_ALL
}EMQSOperator;


typedef struct stMQSMsg
{
	EMQSOperator m_enMQSOperator;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwSequenceId;

	void *pBuffer;
	unsigned int dwLen;

	void *handler;
	string strGuid;

	struct timeval_a tStart;	
	
	stMQSMsg(EMQSOperator enMQSOp):m_enMQSOperator(enMQSOp),dwServiceId(0), dwMsgId(0), dwSequenceId(0), pBuffer(NULL){}
	virtual ~stMQSMsg(){if(pBuffer != NULL){free(pBuffer); pBuffer = NULL;}}
}SMQSMsg;

typedef struct stMQSProduceMsg : public stMQSMsg
{
	stMQSProduceMsg():stMQSMsg(MQS_PRODUCE){}
	virtual ~stMQSProduceMsg(){}
}SMQSProduceMsg;

typedef struct stMQSStopMsg : public stMQSMsg
{
	stMQSStopMsg():stMQSMsg(MQS_STOP){}
	virtual ~stMQSStopMsg(){}
}SMQSStopMsg;

typedef struct stMQSReConnMsg : public stMQSMsg
{
	IKafka *pKafka;
	stMQSReConnMsg():stMQSMsg(MQS_RECONN){}
	virtual ~stMQSReConnMsg(){}
}SMQSReConnMsg;

#endif //_MQS_MSG_H_
