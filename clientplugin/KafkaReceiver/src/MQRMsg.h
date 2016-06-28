#ifndef _MQS_CACHE_MSG_H_
#define _MQS_CACHE_MSG_H_

#include <string>
#include <vector>
#include <map>

#include <detail/_time.h>

#include "KafkaCommon.h"
#include "MQRCommon.h"

using std::string;
using std::vector;
using std::map;
using std::make_pair;

#define		BASE64_CAPACITY		1024
#define		TIMEOUT_INTERVAL	3
#define		MAX_ASYN_BUFFER		1280
#define		VALUE_SPLITTER		"^_^"

#define		GUID_CODE			9
#define		CONHASH_VIRTUAL_NODE 10
#define		MAX_NODE_NUM		 10

typedef enum enMQROperator
{
	MQR_CONSUME = 1,
	MQR_RESPONSE = 2,
	MQR_RECONSUME = 3,
	MQR_TIMEOUT = 4,
	MQR_STOP = 5,
	MQR_RECONN_SERVER = 6,
	MQR_CONFIRM_OFFSET = 7,
	MQR_ALL
}EMQROperator;


typedef struct stMQRMsg
{
	EMQROperator m_enMQROperator;
	struct timeval_a tStart;	
	
	stMQRMsg(EMQROperator enMQROp):m_enMQROperator(enMQROp){}
	virtual ~stMQRMsg(){}
}SMQRMsg;

typedef struct stMQRConsumeMsg : public stMQRMsg
{
	stMQRConsumeMsg():stMQRMsg(MQR_CONSUME){}
	virtual ~stMQRConsumeMsg(){}
}SMQRConsumeMsg;

typedef struct stMQRResponseMsg : public stMQRMsg
{
	SConsumeInfo info;
	stMQRResponseMsg():stMQRMsg(MQR_RESPONSE){}
	virtual ~stMQRResponseMsg(){}
}SMQRResponseMsg;

typedef struct stMQRReConsumeMsg : public stMQRMsg
{
	unsigned int offset;
	stMQRReConsumeMsg():stMQRMsg(MQR_RECONSUME){}
	virtual ~stMQRReConsumeMsg(){}
}SMQRReConsumeMsg;

typedef struct stMQRTimeoutMsg : public stMQRMsg
{
	unsigned int offset;
	stMQRTimeoutMsg():stMQRMsg(MQR_TIMEOUT){}
	virtual ~stMQRTimeoutMsg(){}
}SMQRTimeoutMsg;

typedef struct stMQRStopMsg : public stMQRMsg
{
	stMQRStopMsg():stMQRMsg(MQR_STOP){}
	virtual ~stMQRStopMsg(){}
}SMQRStopMsg;

typedef struct stMQRReConnServerMsg : public stMQRMsg
{
	stMQRReConnServerMsg():stMQRMsg(MQR_RECONN_SERVER){}
	virtual ~stMQRReConnServerMsg(){}
}SMQRReConnServerMsg;

typedef struct stMQRConfirmOffsetMsg : public stMQRMsg
{
	stMQRConfirmOffsetMsg():stMQRMsg(MQR_CONFIRM_OFFSET){}
	virtual ~stMQRConfirmOffsetMsg(){}
}SMQRConfirmOffsetMsg;

#endif
