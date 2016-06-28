#ifndef _REDIS_HASH_MSG_OPERATOR_H_
#define _REDIS_HASH_MSG_OPERATOR_H_

#include "IRedisTypeMsgOperator.h"
#include "RedisVirtualService.h"
#include "ServiceConfig.h"

#include <map>
using namespace redis;
using std::map;
using std::multimap;

typedef enum enRedisHashOperate
{
    RVS_HASH_GET=1,
   	RVS_HASH_SET=2,
   	RVS_HASH_DELETE=3,
   	RVS_HASH_SIZE=4,
   	RVS_HASH_SETALL=5,
   	RVS_HASH_GETALL=6,
   	RVS_HASH_DELETEALL=7,
   	RVS_HASH_INCBY=8,
   	RVS_HASH_GETKEY=9,
   	RVS_HASH_TTL=10,
   	RVS_HASH_DECODE=11,
   	RVS_HASH_BATCH_QUERY=12,
   	RVS_HASH_BATCH_INCRBY=13,
   	RVS_HASH_BATCH_EXPIRE=14,
   	RVS_HASH_ALL
}ERedisHashOperate;

typedef struct stRedisHashOperateMsg
{
	ERedisHashOperate m_enRedisOperate;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
    unsigned int dwSequenceId;
	   
    void *pBuffer;
	unsigned int dwLen;
	
	void *handler;
    string strGuid;
	
	CRedisContainer *pRedisContainer;
	
	struct timeval_a tStart; 	//服务收到消息的开始时间，用以计算总耗时
	float fSpentTimeInQueue; 	//队列等待时间
		
	stRedisHashOperateMsg(ERedisHashOperate operate):m_enRedisOperate(operate){}
	virtual ~stRedisHashOperateMsg(){}

}SRedisHashOperateMsg;

typedef struct stRedisHashGetMsg : public stRedisHashOperateMsg
{
    stRedisHashGetMsg():stRedisHashOperateMsg(RVS_HASH_GET){}
	~stRedisHashGetMsg(){}
}SRedisHashGetMsg;

typedef struct stRedisHashSetMsg : public stRedisHashOperateMsg
{
    stRedisHashSetMsg():stRedisHashOperateMsg(RVS_HASH_SET){}
	~stRedisHashSetMsg(){}
}SRedisHashSetMsg;

typedef struct stRedisHashDeleteMsg : public stRedisHashOperateMsg
{
    stRedisHashDeleteMsg():stRedisHashOperateMsg(RVS_HASH_DELETE){}
	~stRedisHashDeleteMsg(){}
}SRedisHashDeleteMsg;

typedef struct stRedisHashSizeMsg : public stRedisHashOperateMsg
{
    stRedisHashSizeMsg():stRedisHashOperateMsg(RVS_HASH_SIZE){}
	~stRedisHashSizeMsg(){}
}SRedisHashSizeMsg;

typedef struct stRedisHashSetAllMsg : public stRedisHashOperateMsg
{
    stRedisHashSetAllMsg():stRedisHashOperateMsg(RVS_HASH_GETALL){}
	~stRedisHashSetAllMsg(){}
}SRedisHashSetAllMsg;

typedef struct stRedisHashGetAllMsg : public stRedisHashOperateMsg
{
    stRedisHashGetAllMsg():stRedisHashOperateMsg(RVS_HASH_GETALL){}
	~stRedisHashGetAllMsg(){}
}SRedisHashGetAllMsg;

typedef struct stRedisHashDeleteAllMsg : public stRedisHashOperateMsg
{
    stRedisHashDeleteAllMsg():stRedisHashOperateMsg(RVS_HASH_DELETEALL){}
	~stRedisHashDeleteAllMsg(){}
}SRedisHashDeleteAllMsg;

typedef struct stRedisHashIncbyMsg : public stRedisHashOperateMsg
{
    stRedisHashIncbyMsg():stRedisHashOperateMsg(RVS_HASH_INCBY){}
	~stRedisHashIncbyMsg(){}
}SRedisHashIncbyMsg;

typedef struct stRedisHashGetKeyMsg : public stRedisHashOperateMsg
{
    stRedisHashGetKeyMsg():stRedisHashOperateMsg(RVS_HASH_GETKEY){}
	~stRedisHashGetKeyMsg(){}
}SRedisHashGetKeyMsg;

typedef struct stRedisHashTtlMsg : public stRedisHashOperateMsg
{
    stRedisHashTtlMsg():stRedisHashOperateMsg(RVS_HASH_TTL){}
	~stRedisHashTtlMsg(){}
}SRedisHashTtlMsg;

typedef struct stRedisHashDecodeMsg : public stRedisHashOperateMsg
{
    stRedisHashDecodeMsg():stRedisHashOperateMsg(RVS_HASH_DECODE){}
	~stRedisHashDecodeMsg(){}
}SRedisHashDecodeMsg;

typedef struct stRedisHashBatchQueryMsg : public stRedisHashOperateMsg
{
    stRedisHashBatchQueryMsg():stRedisHashOperateMsg(RVS_HASH_BATCH_QUERY){}
	~stRedisHashBatchQueryMsg(){}
}SRedisHashBatchQueryMsg;

typedef struct stRedisHashBatchIncrbyMsg : public stRedisHashOperateMsg
{
    stRedisHashBatchIncrbyMsg():stRedisHashOperateMsg(RVS_HASH_BATCH_INCRBY){}
	~stRedisHashBatchIncrbyMsg(){}
}SRedisHashBatchIncrbyMsg;

typedef struct stRedisHashBatchExpireMsg : public stRedisHashOperateMsg
{
    stRedisHashBatchExpireMsg():stRedisHashOperateMsg(RVS_HASH_BATCH_EXPIRE){}
	~stRedisHashBatchExpireMsg(){}
}SRedisHashBatchExpireMsg;

class CRedisHashMsgOperator : public IRedisTypeMsgOperator
{
   typedef void (CRedisHashMsgOperator::*RedisHashFunc)(SRedisHashOperateMsg *pRedisHashOperateMsg);
public:
	CRedisHashMsgOperator(CRedisVirtualService *pRedisVirtualService);
	virtual ~CRedisHashMsgOperator(){}
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
	
private:
	void DoGet(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoSet(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoDelete(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoSize(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoSetAll(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoGetAll(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoDeleteAll(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoIncby(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoGetKey(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoTtl(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoDecode(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoBatchQuery(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoBatchIncrby(SRedisHashOperateMsg *pRedisHashOperateMsg);
	void DoBatchExpire(SRedisHashOperateMsg *pRedisHashOperateMsg);
private:
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, map<string, string> &mapFieldValue, unsigned int &dwKeepTime, int &increment);
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, vector<SKeyFieldValue> &vecKFV, bool onlyResolveKF=false);
	int GetKFVFromStruct(unsigned int dwServiceId, unsigned int nCode, const void *pBuffer, unsigned int dwLen, string &strKey, string &strField, string &strValue);
	string VecKFVToString(vector<SKeyFieldValue> &vecKFV);
private:
    RedisHashFunc m_redisHashFunc[RVS_HASH_ALL];
};
#endif
