#ifndef _REDIS_STRING_MSG_OPERATOR_H_
#define _REDIS_STRING_MSG_OPERATOR_H_

#include "IRedisTypeMsgOperator.h"
#include "RedisMsg.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

using namespace redis;

typedef enum enRedisStringOperate
{
    RVS_STRING_GET=1,
   	RVS_STRING_SET=2,
   	RVS_STRING_DELETE=3,
   	RVS_STRING_INCBY=4,
   	RVS_STRING_GETKEY=5,
   	RVS_STRING_TTL=6,
   	RVS_STRING_DECODE=7,
   	RVS_STRING_BATCH_QUERY=8,
   	RVS_STRING_BATCH_INCRBY=9,
   	RVS_STRING_BATCH_EXPIRE=10,
   	RVS_STRING_BATCH_DELETE=11,
   	RVS_STRING_GET_INT=12,
   	RVS_STRING_SET_INT=13,
   	RVS_STIRNG_ALL
}ERedisStringOperate;

typedef struct stRedisStringOperateMsg
{
   ERedisStringOperate m_enRedisOperate;
   unsigned int dwServiceId;
   unsigned int dwMsgId;
   unsigned int dwSequenceId;
   
   void *pBuffer;
   unsigned int dwLen;

   void *handler;
   string strGuid;

   CRedisContainer *pRedisContainer;

   struct timeval_a tStart;     //服务收到消息的开始时间，用以计算总耗时
   float fSpentTimeInQueue;     //队列等待时间
	
   stRedisStringOperateMsg(ERedisStringOperate operate):m_enRedisOperate(operate){}
   virtual ~stRedisStringOperateMsg(){}
}SRedisStringOperateMsg;

typedef struct stRedisStringGetMsg : public stRedisStringOperateMsg
{ 
   stRedisStringGetMsg():stRedisStringOperateMsg(RVS_STRING_GET){}
   ~stRedisStringGetMsg(){}
}SRedisStringGetMsg;

typedef struct stRedisStringSetMsg : public stRedisStringOperateMsg
{
   stRedisStringSetMsg():stRedisStringOperateMsg(RVS_STRING_SET){}
   ~stRedisStringSetMsg(){}
}SRedisStringSetMsg;

typedef struct stRedisStringDeleteMsg : public stRedisStringOperateMsg
{
   stRedisStringDeleteMsg():stRedisStringOperateMsg(RVS_STRING_DELETE){}
   ~stRedisStringDeleteMsg(){}
}SRedisStringDeleteMsg;

typedef struct stRedisStringIncbyMsg : public stRedisStringOperateMsg
{
   stRedisStringIncbyMsg():stRedisStringOperateMsg(RVS_STRING_INCBY){}
   ~stRedisStringIncbyMsg(){}
}SRedisStringIncbyMsg;

typedef struct stRedisStringGetKeyMsg : public stRedisStringOperateMsg
{
   stRedisStringGetKeyMsg():stRedisStringOperateMsg(RVS_STRING_GETKEY){}
   ~stRedisStringGetKeyMsg(){}
}SRedisStringGetKeyMsg;

typedef struct stRedisStringTtlMsg : public stRedisStringOperateMsg
{
   stRedisStringTtlMsg():stRedisStringOperateMsg(RVS_STRING_TTL){}
   ~stRedisStringTtlMsg(){}
}SRedisStringTtlMsg;

typedef struct stRedisStringDecodeMsg : public stRedisStringOperateMsg
{
   stRedisStringDecodeMsg():stRedisStringOperateMsg(RVS_STRING_DECODE){}
   ~stRedisStringDecodeMsg(){}
}SRedisStringDecodeMsg;

typedef struct stRedisStringBatchQueryMsg : public stRedisStringOperateMsg
{
   stRedisStringBatchQueryMsg():stRedisStringOperateMsg(RVS_STRING_BATCH_QUERY){}
   ~stRedisStringBatchQueryMsg(){}
}SRedisStringBatchQueryMsg;

typedef struct stRedisStringBatchIncrbyMsg : public stRedisStringOperateMsg
{
   stRedisStringBatchIncrbyMsg():stRedisStringOperateMsg(RVS_STRING_BATCH_INCRBY){}
   ~stRedisStringBatchIncrbyMsg(){}
}SRedisStringBatchIncrbyMsg;

typedef struct stRedisStringBatchExpireMsg : public stRedisStringOperateMsg
{
   stRedisStringBatchExpireMsg():stRedisStringOperateMsg(RVS_STRING_BATCH_EXPIRE){}
   ~stRedisStringBatchExpireMsg(){}
}SRedisStringBatchExpireMsg;

typedef struct stRedisStringBatchDeleteMsg : public stRedisStringOperateMsg
{
   stRedisStringBatchDeleteMsg():stRedisStringOperateMsg(RVS_STRING_BATCH_DELETE){}
   ~stRedisStringBatchDeleteMsg(){}
}SRedisStringBatchDeleteMsg;

typedef struct stRedisStringGetIntMsg : public stRedisStringOperateMsg
{
   stRedisStringGetIntMsg():stRedisStringOperateMsg(RVS_STRING_GET_INT){}
   ~stRedisStringGetIntMsg(){}
}SRedisStringGetIntMsg;

typedef struct stRedisStringSetIntMsg : public stRedisStringOperateMsg
{
   stRedisStringSetIntMsg():stRedisStringOperateMsg(RVS_STRING_SET_INT){}
   ~stRedisStringSetIntMsg(){}
}SRedisStringSetIntMsg;

class CRedisVirtualService;

class CRedisStringMsgOperator : public IRedisTypeMsgOperator
{
	typedef void (CRedisStringMsgOperator::*RedisStringFunc)(SRedisStringOperateMsg *redisStringOperateMsg);

public:
	CRedisStringMsgOperator(CRedisVirtualService *pRedisVirtualService);
	virtual ~CRedisStringMsgOperator();
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
private:
	void DoGet(SRedisStringOperateMsg * pRedisStringOperateMsg);
	void DoSet(SRedisStringOperateMsg * pRedisStringOperateMsg);
	void DoDelete(SRedisStringOperateMsg * pRedisStringOperateMsg);
	void DoIncby(SRedisStringOperateMsg * pRedisStringOperateMsg);
	void DoGetKey(SRedisStringOperateMsg * pRedisStringOperateMsg);
    void DoTtl(SRedisStringOperateMsg * pRedisStringOperateMsg);
	void DoDecode(SRedisStringOperateMsg *pRedisStringOperateMsg);
	void DoBatchQuery(SRedisStringOperateMsg *pRedisStringOperateMsg);
	void DoBatchIncrby(SRedisStringOperateMsg *pRedisStringOperateMsg);
	void DoBatchExpire(SRedisStringOperateMsg *pRedisStringOperateMsg);
	void DoBatchDelete(SRedisStringOperateMsg *pRedisStringOperateMsg);
	void DoGetInt(SRedisStringOperateMsg *pRedisStringOperateMsg);
	void DoSetInt(SRedisStringOperateMsg *pRedisStringOperateMsg);
private:
	void ResponseFromVec( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const vector<RVSKeyValue> & vecKeyValue);
	void ResponseBatch(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const void *pBuffer, unsigned int dwLen, const vector<SKeyValue> &vecKeyValue);
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, unsigned int &dwKeepTime);
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, int &incbyNum);
	
	string VectorToString(const vector<RVSKeyValue> &vecKeyValue);
private:
	RedisStringFunc m_redisStringFunc[RVS_STIRNG_ALL];
};
#endif
