#ifndef _REDIS_SET_MSG_OPERATOR_H_
#define _REDIS_SET_MSG_OPERATOR_H_

#include "IRedisTypeMsgOperator.h"
#include "RedisVirtualService.h"

#include <string>
using std::string;

typedef enum enRedisSetOperate
{
    RVS_SET_HASMEMBER=1,
	RVS_SET_SET=2,
	RVS_SET_DELETE=3,
	RVS_SET_SIZE=4,
	RVS_SET_GETALL=5,
	RVS_SET_DELETEALL=6,
	RVS_SET_INTER=7,      //求交集
	RVS_SET_UNION=8,         //求并集
	RVS_SET_GETKEY=9,
	RVS_SET_TTL=10,
	RVS_SET_DECODE=11,
	RVS_SET_ALL
}ERedisSetOperate;

typedef struct stRedisSetOperateMsg
{
   ERedisSetOperate m_enRedisOperate;
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
	
   stRedisSetOperateMsg(ERedisSetOperate operate):m_enRedisOperate(operate){}
   virtual ~stRedisSetOperateMsg(){}
}SRedisSetOperateMsg;

typedef struct stRedisSetHasMemberMsg : public stRedisSetOperateMsg
{ 
   stRedisSetHasMemberMsg():stRedisSetOperateMsg(RVS_SET_HASMEMBER){}
   ~stRedisSetHasMemberMsg(){}
}SRedisSetHasMemberMsg;

typedef struct stRedisSetSetMsg : public stRedisSetOperateMsg
{ 
   stRedisSetSetMsg():stRedisSetOperateMsg(RVS_SET_SET){}
   ~stRedisSetSetMsg(){}
}SRedisSetSetMsg;

typedef struct stRedisSetDeleteMsg : public stRedisSetOperateMsg
{ 
   stRedisSetDeleteMsg():stRedisSetOperateMsg(RVS_SET_DELETE){}
   ~stRedisSetDeleteMsg(){}
}SRedisSetDeleteMsg;

typedef struct stRedisSetSizeMsg : public stRedisSetOperateMsg
{ 
   stRedisSetSizeMsg():stRedisSetOperateMsg(RVS_SET_SIZE){}
   ~stRedisSetSizeMsg(){}
}SRedisSetSizeMsg;

typedef struct stRedisSetGetAllMsg : public stRedisSetOperateMsg
{ 
   stRedisSetGetAllMsg():stRedisSetOperateMsg(RVS_SET_GETALL){}
   ~stRedisSetGetAllMsg(){}
}SRedisSetGetAllMsg;

typedef struct stRedisSetDeleteAllMsg : public stRedisSetOperateMsg
{ 
   stRedisSetDeleteAllMsg():stRedisSetOperateMsg(RVS_SET_DELETEALL){}
   ~stRedisSetDeleteAllMsg(){}
}SRedisSetDeleteAllMsg;

typedef struct stRedisSetInterMsg : public stRedisSetOperateMsg
{ 
   stRedisSetInterMsg():stRedisSetOperateMsg(RVS_SET_INTER){}
   ~stRedisSetInterMsg(){}
}SRedisSetInterMsg;

typedef struct stRedisSetUnionMsg : public stRedisSetOperateMsg
{ 
   stRedisSetUnionMsg():stRedisSetOperateMsg(RVS_SET_UNION){}
   ~stRedisSetUnionMsg(){}
}SRedisSetUnionMsg;

typedef struct stRedisSetGetKeyMsg : public stRedisSetOperateMsg
{ 
   stRedisSetGetKeyMsg():stRedisSetOperateMsg(RVS_SET_GETKEY){}
   ~stRedisSetGetKeyMsg(){}
}SRedisSetGetKeyMsg;

typedef struct stRedisSetTtlMsg : public stRedisSetOperateMsg
{ 
   stRedisSetTtlMsg():stRedisSetOperateMsg(RVS_SET_TTL){}
   ~stRedisSetTtlMsg(){}
}SRedisSetTtlMsg;

typedef struct stRedisSetDecodeMsg : public stRedisSetOperateMsg
{ 
   stRedisSetDecodeMsg():stRedisSetOperateMsg(RVS_SET_DECODE){}
   ~stRedisSetDecodeMsg(){}
}SRedisSetDecodeMsg;

class CRedisSetMsgOperator : public IRedisTypeMsgOperator
{
	typedef void (CRedisSetMsgOperator::*RedisSetFunc)(SRedisSetOperateMsg *redisSetOperateMsg);

public:
	CRedisSetMsgOperator(CRedisVirtualService *pRedisVirtualService);
	virtual ~CRedisSetMsgOperator(){}
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
private:
	void DoHasMember(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoSet(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoDelete(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoSize(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoGetAll(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoDeleteAll(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoInter(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoUnion(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoGetKey(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoTtl(SRedisSetOperateMsg *pRedisSetOperateMsg);
	void DoDecode(SRedisSetOperateMsg *pRedisSetOperateMsg);
private:
	int GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, unsigned int &dwKeepTime);
private:
	RedisSetFunc m_redisSetFunc[RVS_SET_ALL];
};
#endif
