#ifndef _REDIS_LIST_MSG_OPERATOR_H_
#define _REDIS_LIST_MSG_OPERATOR_H_

#include "IRedisTypeMsgOperator.h"
#include "RedisVirtualService.h"

#include <string>
#include <map>
using std::map;
using std::string;

typedef enum enRedisListOperate
{
    RVS_LIST_GET=1,
	RVS_LIST_SET=2,
	RVS_LIST_DELETE=3,
	RVS_LIST_SIZE=4,
	RVS_LIST_GETALL=5,
	RVS_LIST_DELETEALL=6,
	RVS_LIST_POPBACK=7,
	RVS_LIST_PUSHBACK=8,
	RVS_LIST_POPFRONT=9,
	RVS_LIST_PUSHFRONT=10,
	RVS_LIST_RESERVE=11,        //只保留指定范围内的元素
	RVS_LIST_GETKEY=12,
	RVS_LIST_TTL=13,
	RVS_LIST_DECODE=14,
	RVS_LIST_ALL
}ERedisListOperate;

typedef struct stRedisListOperateMsg
{
   ERedisListOperate m_enRedisOperate;
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
	
   stRedisListOperateMsg(ERedisListOperate operate):m_enRedisOperate(operate){}
   virtual ~stRedisListOperateMsg(){}
}SRedisListOperateMsg;

typedef struct stRedisListGetMsg : public stRedisListOperateMsg
{ 
   stRedisListGetMsg():stRedisListOperateMsg(RVS_LIST_GET){}
   ~stRedisListGetMsg(){}
}SRedisListGetMsg;

typedef struct stRedisListSetMsg : public stRedisListOperateMsg
{ 
   stRedisListSetMsg():stRedisListOperateMsg(RVS_LIST_SET){}
   ~stRedisListSetMsg(){}
}SRedisListSetMsg;

typedef struct stRedisListDeleteMsg : public stRedisListOperateMsg
{ 
   stRedisListDeleteMsg():stRedisListOperateMsg(RVS_LIST_DELETE){}
   ~stRedisListDeleteMsg(){}
}SRedisListDeleteMsg;

typedef struct stRedisListSizeMsg : public stRedisListOperateMsg
{ 
   stRedisListSizeMsg():stRedisListOperateMsg(RVS_LIST_SIZE){}
   ~stRedisListSizeMsg(){}
}SRedisListSizeMsg;

typedef struct stRedisListGetAllMsg : public stRedisListOperateMsg
{ 
   stRedisListGetAllMsg():stRedisListOperateMsg(RVS_LIST_GETALL){}
   ~stRedisListGetAllMsg(){}
}SRedisListGetAllMsg;

typedef struct stRedisListDeleteAllMsg : public stRedisListOperateMsg
{ 
   stRedisListDeleteAllMsg():stRedisListOperateMsg(RVS_LIST_DELETEALL){}
   ~stRedisListDeleteAllMsg(){}
}SRedisListDeleteAllMsg;

typedef struct stRedisListPopBackMsg : public stRedisListOperateMsg
{ 
   stRedisListPopBackMsg():stRedisListOperateMsg(RVS_LIST_POPBACK){}
   ~stRedisListPopBackMsg(){}
}SRedisListPopBackMsg;

typedef struct stRedisListPushBackMsg : public stRedisListOperateMsg
{ 
   stRedisListPushBackMsg():stRedisListOperateMsg(RVS_LIST_PUSHBACK){}
   ~stRedisListPushBackMsg(){}
}SRedisListPushBackMsg;

typedef struct stRedisListPopFrontMsg : public stRedisListOperateMsg
{ 
   stRedisListPopFrontMsg():stRedisListOperateMsg(RVS_LIST_POPFRONT){}
   ~stRedisListPopFrontMsg(){}
}SRedisListPopFrontMsg;

typedef struct stRedisListPushFrontMsg : public stRedisListOperateMsg
{ 
   stRedisListPushFrontMsg():stRedisListOperateMsg(RVS_LIST_PUSHFRONT){}
   ~stRedisListPushFrontMsg(){}
}SRedisListPushFrontMsg;

typedef struct stRedisListReserveMsg : public stRedisListOperateMsg
{ 
   stRedisListReserveMsg():stRedisListOperateMsg(RVS_LIST_RESERVE){}
   ~stRedisListReserveMsg(){}
}SRedisListReserveMsg;

typedef struct stRedisListGetKeyMsg : public stRedisListOperateMsg
{ 
   stRedisListGetKeyMsg():stRedisListOperateMsg(RVS_LIST_GETKEY){}
   ~stRedisListGetKeyMsg(){}
}SRedisListGetKeyMsg;

typedef struct stRedisListTtlMsg : public stRedisListOperateMsg
{ 
   stRedisListTtlMsg():stRedisListOperateMsg(RVS_LIST_TTL){}
   ~stRedisListTtlMsg(){}
}SRedisListTtlMsg;

typedef struct stRedisListDecodeMsg : public stRedisListOperateMsg
{ 
   stRedisListDecodeMsg():stRedisListOperateMsg(RVS_LIST_DECODE){}
   ~stRedisListDecodeMsg(){}
}SRedisListDecodeMsg;

class CRedisListMsgOperator : public IRedisTypeMsgOperator
{
	typedef void (CRedisListMsgOperator::*RedisListFunc)(SRedisListOperateMsg *redisListOperateMsg);
public:
	CRedisListMsgOperator(CRedisVirtualService *pRedisVirtualService);
	virtual ~CRedisListMsgOperator(){}
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
private:
	void DoGet(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoSet(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoDelete(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoSize(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoGetAll(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoDeleteAll(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoPopBack(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoPushBack(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoPopFront(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoPushFront(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoReserve(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoGetKey(SRedisListOperateMsg *pRedisListOperateMsg);
	void DoTtl(SRedisListOperateMsg *pRedisListOperateMsg);
    void DoDecode(SRedisListOperateMsg *pRedisListOperateMsg);
private:
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, map<string, int> &mapAddtionParams);	
private:
	RedisListFunc m_redisListFunc[RVS_LIST_ALL];
};
#endif
