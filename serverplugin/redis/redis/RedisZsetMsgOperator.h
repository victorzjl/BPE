#ifndef _REDIS_ZSET_MSG_OPERATOR_H_
#define _REDIS_ZSET_MSG_OPERATOR_H_

#include "IRedisTypeMsgOperator.h"
#include "RedisMsg.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

using namespace redis;

typedef enum enRedisZsetOperate
{
    RVS_ZSET_GET=1,
   	RVS_ZSET_SET=2,
   	RVS_ZSET_DELETE=3,
   	RVS_ZSET_SIZE=4,
   	RVS_ZSET_GETALL=5,
   	RVS_ZSET_GETREVERSEALL=6,
   	RVS_ZSET_GETSCOREALL=7,
   	RVS_ZSET_DELETEALL=8,
   	RVS_ZSET_RANK=9,
   	RVS_ZSET_REVERSERANK=10,
   	RVS_ZSET_INCBY=11,
   	RVS_ZSET_DELRANGEBYRANK=12,
   	RVS_ZSET_DELRANGEBYSCORE=13,
   	RVS_ZSET_GETKEY=14,
   	RVS_ZSET_TTL=15,
   	RVS_ZSET_DECODE=16,
   	RVS_ZSET_ALL
}ERedisZsetOperate;

typedef struct stRedisZsetOperateMsg
{
   ERedisZsetOperate m_enRedisOperate;
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
	
   stRedisZsetOperateMsg(ERedisZsetOperate operate):m_enRedisOperate(operate){}
   virtual ~stRedisZsetOperateMsg(){}
}SRedisZsetOperateMsg;

typedef struct stRedisZsetGetMsg : public stRedisZsetOperateMsg
{ 
   stRedisZsetGetMsg():stRedisZsetOperateMsg(RVS_ZSET_GET){}
   ~stRedisZsetGetMsg(){}
}SRedisZsetGetMsg;

typedef struct stRedisZsetSetMsg : public stRedisZsetOperateMsg
{
   stRedisZsetSetMsg():stRedisZsetOperateMsg(RVS_ZSET_SET){}
   ~stRedisZsetSetMsg(){}
}SRedisZsetSetMsg;

typedef struct stRedisZsetDeleteMsg : public stRedisZsetOperateMsg
{
   stRedisZsetDeleteMsg():stRedisZsetOperateMsg(RVS_ZSET_DELETE){}
   ~stRedisZsetDeleteMsg(){}
}SRedisZsetDeleteMsg;

typedef struct stRedisZsetSizeMsg : public stRedisZsetOperateMsg
{
   stRedisZsetSizeMsg():stRedisZsetOperateMsg(RVS_ZSET_SIZE){}
   ~stRedisZsetSizeMsg(){}
}SRedisZsetSizeMsg;

typedef struct stRedisZsetGetAllMsg : public stRedisZsetOperateMsg
{
   stRedisZsetGetAllMsg():stRedisZsetOperateMsg(RVS_ZSET_GETALL){}
   ~stRedisZsetGetAllMsg(){}
}SRedisZsetGetAllMsg;

typedef struct stRedisZsetGetReverseAllMsg : public stRedisZsetOperateMsg
{
   stRedisZsetGetReverseAllMsg():stRedisZsetOperateMsg(RVS_ZSET_GETREVERSEALL){}
   ~stRedisZsetGetReverseAllMsg(){}
}SRedisZsetGetReverseAllMsg;

typedef struct stRedisZsetGetScoreAllMsg : public stRedisZsetOperateMsg
{
   stRedisZsetGetScoreAllMsg():stRedisZsetOperateMsg(RVS_ZSET_GETSCOREALL){}
   ~stRedisZsetGetScoreAllMsg(){}
}SRedisZsetGetScoreAllMsg;

typedef struct stRedisZsetDeleteAllMsg : public stRedisZsetOperateMsg
{
   stRedisZsetDeleteAllMsg():stRedisZsetOperateMsg(RVS_ZSET_DELETEALL){}
   ~stRedisZsetDeleteAllMsg(){}
}SRedisZsetDeleteAllMsg;

typedef struct stRedisZsetRankMsg : public stRedisZsetOperateMsg
{
   stRedisZsetRankMsg():stRedisZsetOperateMsg(RVS_ZSET_RANK){}
   ~stRedisZsetRankMsg(){}
}SRedisZsetRankMsg;

typedef struct stRedisZsetReverseRankMsg : public stRedisZsetOperateMsg
{
   stRedisZsetReverseRankMsg():stRedisZsetOperateMsg(RVS_ZSET_REVERSERANK){}
   ~stRedisZsetReverseRankMsg(){}
}SRedisZsetReverseRankMsg;

typedef struct stRedisZsetIncbyMsg : public stRedisZsetOperateMsg
{
   stRedisZsetIncbyMsg():stRedisZsetOperateMsg(RVS_ZSET_INCBY){}
   ~stRedisZsetIncbyMsg(){}
}SRedisZsetIncbyMsg;

typedef struct stRedisZsetDelRangeByRankMsg : public stRedisZsetOperateMsg
{
   stRedisZsetDelRangeByRankMsg():stRedisZsetOperateMsg(RVS_ZSET_DELRANGEBYRANK){}
   ~stRedisZsetDelRangeByRankMsg(){}
}SRedisZsetDelRangeByRankMsg;

typedef struct stRedisZsetDelRangeByScoreMsg : public stRedisZsetOperateMsg
{
   stRedisZsetDelRangeByScoreMsg():stRedisZsetOperateMsg(RVS_ZSET_DELRANGEBYSCORE){}
   ~stRedisZsetDelRangeByScoreMsg(){}
}SRedisZsetDelRangeByScoreMsg;

typedef struct stRedisZsetGetKeyMsg : public stRedisZsetOperateMsg
{
   stRedisZsetGetKeyMsg():stRedisZsetOperateMsg(RVS_ZSET_GETKEY){}
   ~stRedisZsetGetKeyMsg(){}
}SRedisZsetGetKeyMsg;

typedef struct stRedisZsetTtlMsg : public stRedisZsetOperateMsg
{
   stRedisZsetTtlMsg():stRedisZsetOperateMsg(RVS_ZSET_TTL){}
   ~stRedisZsetTtlMsg(){}
}SRedisZsetTtlMsg;

typedef struct stRedisZsetDecodeMsg : public stRedisZsetOperateMsg
{
   stRedisZsetDecodeMsg():stRedisZsetOperateMsg(RVS_ZSET_DECODE){}
   ~stRedisZsetDecodeMsg(){}
}SRedisZsetDecodeMsg;

class CRedisVirtualService;

class CRedisZsetMsgOperator : public IRedisTypeMsgOperator
{
	typedef void (CRedisZsetMsgOperator::*RedisZsetFunc)(SRedisZsetOperateMsg *redisZsetOperateMsg);

public:
	CRedisZsetMsgOperator(CRedisVirtualService *pRedisVirtualService);
	virtual ~CRedisZsetMsgOperator();
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
private:
	void DoGet(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoSet(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoDelete(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoSize(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoGetAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoGetReverseAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoGetScoreAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoDeleteAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoRank(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoReverseRank(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoIncby(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoDelRangeByRank(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoDelRangeByScore(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoGetKey(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
    void DoTtl(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
	void DoDecode(SRedisZsetOperateMsg * pRedisZsetOperateMsg);
private:
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strField, int &score, map<string, int> &mapAddtionParams);
private:
	RedisZsetFunc m_redisZsetFunc[RVS_ZSET_ALL];
};
#endif
