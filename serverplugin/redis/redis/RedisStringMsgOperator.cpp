#include "RedisStringMsgOperator.h"
#include "RvsArchiveAdapter.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "ServiceConfig.h"
#include "RedisVirtualService.h"
#include "RedisContainer.h"
#include "CommonMacro.h"
#include "RedisStringAgentScheduler.h"

#include <boost/algorithm/string.hpp>
#include <XmlConfigParser.h>
#include <SapMessage.h>

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace boost::algorithm;
using namespace sdo::sap;
using namespace redis;

CRedisStringMsgOperator::CRedisStringMsgOperator(CRedisVirtualService *pRedisVirtualService):IRedisTypeMsgOperator(pRedisVirtualService)
{
   m_redisStringFunc[RVS_STRING_GET] = &CRedisStringMsgOperator::DoGet;
   m_redisStringFunc[RVS_STRING_SET] = &CRedisStringMsgOperator::DoSet;
   m_redisStringFunc[RVS_STRING_DELETE] = &CRedisStringMsgOperator::DoDelete;
   m_redisStringFunc[RVS_STRING_INCBY] = &CRedisStringMsgOperator::DoIncby;
   m_redisStringFunc[RVS_STRING_GETKEY] = &CRedisStringMsgOperator::DoGetKey;
   m_redisStringFunc[RVS_STRING_TTL] = &CRedisStringMsgOperator::DoTtl;
   m_redisStringFunc[RVS_STRING_DECODE] = &CRedisStringMsgOperator::DoDecode;
   m_redisStringFunc[RVS_STRING_BATCH_QUERY] = &CRedisStringMsgOperator::DoBatchQuery;
   m_redisStringFunc[RVS_STRING_BATCH_INCRBY] = &CRedisStringMsgOperator::DoBatchIncrby;
   m_redisStringFunc[RVS_STRING_BATCH_EXPIRE] = &CRedisStringMsgOperator::DoBatchExpire;
   m_redisStringFunc[RVS_STRING_BATCH_DELETE] = &CRedisStringMsgOperator::DoBatchDelete;
   m_redisStringFunc[RVS_STRING_GET_INT] = &CRedisStringMsgOperator::DoGetInt;
   m_redisStringFunc[RVS_STRING_SET_INT] = &CRedisStringMsgOperator::DoSetInt;
}

CRedisStringMsgOperator::~CRedisStringMsgOperator()
{
}

void CRedisStringMsgOperator::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
   RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
   
   SRedisStringOperateMsg * redisStringOperateMsg = NULL;
   switch(dwMsgId)
   {
   	case RVS_STRING_GET:
		redisStringOperateMsg = new SRedisStringGetMsg;
		break;
	case RVS_STRING_SET:
		redisStringOperateMsg = new SRedisStringSetMsg;
		break;
	case RVS_STRING_DELETE:
		redisStringOperateMsg = new SRedisStringDeleteMsg;
		break;
	case RVS_STRING_INCBY:
		redisStringOperateMsg = new SRedisStringIncbyMsg;
		break;
	case RVS_STRING_GETKEY:
		redisStringOperateMsg = new SRedisStringGetKeyMsg;
		break;
	case RVS_STRING_TTL:
		redisStringOperateMsg = new SRedisStringTtlMsg;
		break;
	case RVS_STRING_DECODE:
		redisStringOperateMsg = new SRedisStringDecodeMsg;
		break;
	case RVS_STRING_BATCH_QUERY:
		redisStringOperateMsg = new SRedisStringBatchQueryMsg;
		break;
	case RVS_STRING_BATCH_INCRBY:
		redisStringOperateMsg = new SRedisStringBatchIncrbyMsg;
		break;
	case RVS_STRING_BATCH_EXPIRE:
		redisStringOperateMsg = new SRedisStringBatchExpireMsg;
		break;
	case RVS_STRING_BATCH_DELETE:
		redisStringOperateMsg = new SRedisStringBatchDeleteMsg;
		break;
	case RVS_STRING_GET_INT:
		redisStringOperateMsg = new SRedisStringGetIntMsg;
		break;
	case RVS_STRING_SET_INT:
		redisStringOperateMsg = new SRedisStringSetIntMsg;
		break;
	default:
		RVS_XLOG(XLOG_ERROR, "CRedisStringMsgOperator::%s, MsgId[%d] not supported\n", __FUNCTION__, dwMsgId);
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_MSG, "" , fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_UNKNOWN_MSG);
		return;
   }

   redisStringOperateMsg->dwServiceId = dwServiceId;
   redisStringOperateMsg->dwMsgId = dwMsgId;
   redisStringOperateMsg->dwSequenceId = dwSequenceId;
   redisStringOperateMsg->pBuffer = pBuffer;
   redisStringOperateMsg->dwLen = nLen;
   redisStringOperateMsg->handler = pHandler;
   redisStringOperateMsg->strGuid = strGuid;
   redisStringOperateMsg->tStart = tStart;
   redisStringOperateMsg->fSpentTimeInQueue = fSpentInQueueTime;
   redisStringOperateMsg->pRedisContainer = pRedisContainer;

   (this->*(m_redisStringFunc[dwMsgId]))(redisStringOperateMsg);

   delete redisStringOperateMsg;
}

void CRedisStringMsgOperator::DoGet(SRedisStringOperateMsg * pRedisStringOperateMsg)
{
   RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
   SRedisStringGetMsg *pRedisStringGetMsg = (SRedisStringGetMsg *)pRedisStringOperateMsg;
   unsigned int dwServiceId = pRedisStringGetMsg->dwServiceId;
   unsigned int dwMsgId = pRedisStringGetMsg->dwMsgId;
   unsigned int dwSequenceId = pRedisStringGetMsg->dwSequenceId;
   
   CSapDecoder decoder(pRedisStringGetMsg->pBuffer, pRedisStringGetMsg->dwLen);

   void *pBody = NULL;
   unsigned int dwBodyLen = 0;
   decoder.GetBody(&pBody, &dwBodyLen);
   
   string strKey;
   string strValue;
   unsigned int dwKeepTime = 0;
   int nRet = RVS_SUCESS;

   nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
   if(nRet != 0)
   {
       m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisStringGetMsg->fSpentTimeInQueue);
	   m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
	   return;
   }

   CRedisContainer *pRedisContainer = pRedisStringGetMsg->pRedisContainer;
   if(pRedisContainer == NULL)
   {
	   m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisStringGetMsg->fSpentTimeInQueue);
	   m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
	   return;
   }

   string strIpAddrs;
   CRedisStringAgentScheduler agentScheduler(pRedisContainer);
   nRet = agentScheduler.Get(strKey, strValue, strIpAddrs);
   if(nRet != RVS_SUCESS)
   {
	   m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
	   m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
	   return;
   }

   RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());

   if(IsStrDigit(strValue))
   {
	   //发送响应
	   int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_STRING_GET);
	   char szTemp[16] = {0};
	   snprintf(szTemp, sizeof(szTemp), "%d#", dwCode);
	   strValue = szTemp + strValue;
	   m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
	   Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);
   }
   else
   {
   	    vector<RVSKeyValue> vecKeyValue;
        nRet = CRvsArchiveAdapter::Iarchive(strValue, vecKeyValue);

        if(nRet != 0)
        {
	      m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, RVS_SERIALIZE_ERROR, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
	      m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_SERIALIZE_ERROR);
	      return;
        }

        string strLogValue = VectorToString(vecKeyValue);
        m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
        ResponseFromVec(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
   }
}

void CRedisStringMsgOperator::DoSet(SRedisStringOperateMsg * pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	
	SRedisStringSetMsg *pRedisStringSetMsg = (SRedisStringSetMsg *)pRedisStringOperateMsg;
	
	CSapDecoder decoder(pRedisStringSetMsg->pBuffer, pRedisStringSetMsg->dwLen);
	unsigned int dwServiceId = pRedisStringSetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisStringSetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisStringSetMsg->dwSequenceId;
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisStringSetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringSetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisStringSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	vector<RVSKeyValue> vecKeyValue;
	CRvsArchiveAdapter::Iarchive(strValue, vecKeyValue);
	string strLogValue = VectorToString(vecKeyValue);

	CRedisContainer *pRedisContainer = pRedisStringSetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisStringSetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringSetMsg->tStart, strKey, strLogValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisStringSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, strValue, dwKeepTime, strIpAddrs);
	
	m_pRedisVirtualService->Log(pRedisStringSetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisStringSetMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisStringSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisStringMsgOperator::DoDelete(SRedisStringOperateMsg * pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringDeleteMsg *pRedisStringDeleteMsg = (SRedisStringDeleteMsg *)pRedisStringOperateMsg;
	
	CSapDecoder decoder(pRedisStringDeleteMsg->pBuffer, pRedisStringDeleteMsg->dwLen);
	unsigned int dwServiceId = pRedisStringDeleteMsg->dwServiceId;
	unsigned int dwMsgId = pRedisStringDeleteMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisStringDeleteMsg->dwSequenceId;
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisStringDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisStringDeleteMsg->tStart, strKey, "", 0, nRet, "", pRedisStringDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	
	CRedisContainer *pRedisContainer = pRedisStringDeleteMsg->pRedisContainer;
	if(pRedisContainer == NULL)
    {
		m_pRedisVirtualService->Log(pRedisStringDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisStringDeleteMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisStringDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Delete(strKey, strIpAddrs);
	
	m_pRedisVirtualService->Log(pRedisStringDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisStringDeleteMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisStringDeleteMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisStringDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisStringMsgOperator::DoIncby(SRedisStringOperateMsg * pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringIncbyMsg *pRedisStringIncbyMsg = (SRedisStringIncbyMsg *)pRedisStringOperateMsg;
	
	CSapDecoder decoder(pRedisStringIncbyMsg->pBuffer, pRedisStringIncbyMsg->dwLen);
	unsigned int dwServiceId = pRedisStringIncbyMsg->dwServiceId;
	unsigned int dwMsgId = pRedisStringIncbyMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisStringIncbyMsg->dwSequenceId;
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	int incbyNum = 0;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, incbyNum);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisStringIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisStringIncbyMsg->tStart, strKey, "", 0, nRet, "", pRedisStringIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	char szTemp[16] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", incbyNum);
	string strLogValue = szTemp;
	CRedisContainer *pRedisContainer = pRedisStringIncbyMsg->pRedisContainer;
	if(pRedisContainer == NULL)
    {
		m_pRedisVirtualService->Log(pRedisStringIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisStringIncbyMsg->tStart, strKey, strLogValue, 0, RVS_NO_REDIS_SERVER, "", pRedisStringIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Incby(strKey, incbyNum, strIpAddrs);
	
	m_pRedisVirtualService->Log(pRedisStringIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisStringIncbyMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisStringIncbyMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisStringIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisStringMsgOperator::DoGetKey(SRedisStringOperateMsg * pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringGetKeyMsg *pRedisStringGetKeyMsg = (SRedisStringGetKeyMsg *)pRedisStringOperateMsg;
	
	GetKey(pRedisStringGetKeyMsg->dwServiceId,
			pRedisStringGetKeyMsg->dwMsgId,
			pRedisStringGetKeyMsg->dwSequenceId,
			pRedisStringGetKeyMsg->strGuid,
			pRedisStringGetKeyMsg->handler,
			pRedisStringGetKeyMsg->pBuffer,
			pRedisStringGetKeyMsg->dwLen,
			pRedisStringGetKeyMsg->tStart,
			pRedisStringGetKeyMsg->fSpentTimeInQueue,
			RVS_STRING_GETKEY);
}

void CRedisStringMsgOperator::DoTtl(SRedisStringOperateMsg * pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringTtlMsg *pRedisStringTtlMsg = (SRedisStringTtlMsg *)pRedisStringOperateMsg;

    Ttl(pRedisStringTtlMsg->dwServiceId,
		pRedisStringTtlMsg->dwMsgId,
		pRedisStringTtlMsg->dwSequenceId,
		pRedisStringTtlMsg->strGuid,
		pRedisStringTtlMsg->handler,
		pRedisStringTtlMsg->pBuffer,
		pRedisStringTtlMsg->dwLen,
		pRedisStringTtlMsg->tStart,
		pRedisStringTtlMsg->fSpentTimeInQueue,
		pRedisStringTtlMsg->pRedisContainer,
		RVS_STRING_TTL);
}

void CRedisStringMsgOperator::DoDecode(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringDecodeMsg *pRedisStringDecodeMsg = (SRedisStringDecodeMsg *)pRedisStringOperateMsg;
	CSapDecoder decoder(pRedisStringDecodeMsg->pBuffer, pRedisStringDecodeMsg->dwLen);
	unsigned int dwServiceId = pRedisStringDecodeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisStringDecodeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisStringDecodeMsg->dwSequenceId;
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	
	int nRet = RVS_SUCESS;
	string strEncodedField, strEncodedValue;
	GetRequestEnocdedInfo(dwServiceId, pBody, dwBodyLen, strEncodedField, strEncodedValue);
	if(IsStrDigit(strEncodedValue))
	{
		//发送响应
		int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_STRING_DECODE);
		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d#", dwCode);
		strEncodedValue = szTemp + strEncodedValue;
		m_pRedisVirtualService->Log(pRedisStringDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisStringDecodeMsg->tStart, "", strEncodedValue, 0, nRet, "", pRedisStringDecodeMsg->fSpentTimeInQueue);
		Response(pRedisStringDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strEncodedValue);
	}
	else
	{
		 vector<RVSKeyValue> vecKeyValue;
		 nRet = CRvsArchiveAdapter::Iarchive(strEncodedValue, vecKeyValue);
	     RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s Iarchive Response Code[%d]\n", __FUNCTION__, nRet);
		 if(nRet != 0)
		 {
		   m_pRedisVirtualService->Log(pRedisStringDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisStringDecodeMsg->tStart, "", "", 0, RVS_SERIALIZE_ERROR, "", pRedisStringDecodeMsg->fSpentTimeInQueue);
		   m_pRedisVirtualService->Response(pRedisStringDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_SERIALIZE_ERROR);
		   return;
		 }
	
		 string strLogValue = VectorToString(vecKeyValue);
		 m_pRedisVirtualService->Log(pRedisStringDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisStringDecodeMsg->tStart, "", strLogValue, 0, nRet, "", pRedisStringDecodeMsg->fSpentTimeInQueue);
		 ResponseFromVec(pRedisStringDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
	}
}

void CRedisStringMsgOperator::DoBatchQuery(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringBatchQueryMsg *pRedisBactchQueryMsg = (SRedisStringBatchQueryMsg *)pRedisStringOperateMsg;
	unsigned int dwServiceId = pRedisBactchQueryMsg->dwServiceId;
	unsigned int dwMsgId = pRedisBactchQueryMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisBactchQueryMsg->dwSequenceId;
	
	CSapDecoder decoder(pRedisBactchQueryMsg->pBuffer, pRedisBactchQueryMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	
	vector<SKeyValue> vecKeyValue;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestVecKeyValue(dwServiceId, pBody, dwBodyLen, vecKeyValue, true);
	string strLogValue = VectorToString1(vecKeyValue);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisBactchQueryMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchQueryMsg->tStart, strLogValue, "", 0, nRet, "", pRedisBactchQueryMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchQueryMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisBactchQueryMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisBactchQueryMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchQueryMsg->tStart, strLogValue, "", 0, RVS_NO_REDIS_SERVER, "", pRedisBactchQueryMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchQueryMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.BatchQuery(vecKeyValue, strIpAddrs);
	strLogValue = VectorToString1(vecKeyValue);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisBactchQueryMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchQueryMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchQueryMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchQueryMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	ResponseBatch(pRedisBactchQueryMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, pBody, dwBodyLen, vecKeyValue);
	m_pRedisVirtualService->Log(pRedisBactchQueryMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchQueryMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchQueryMsg->fSpentTimeInQueue);

}

void CRedisStringMsgOperator::DoBatchIncrby(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringBatchIncrbyMsg *pRedisBactchIncrbyMsg = (SRedisStringBatchIncrbyMsg *)pRedisStringOperateMsg;
	unsigned int dwServiceId = pRedisBactchIncrbyMsg->dwServiceId;
	unsigned int dwMsgId = pRedisBactchIncrbyMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisBactchIncrbyMsg->dwSequenceId;
	
	CSapDecoder decoder(pRedisBactchIncrbyMsg->pBuffer, pRedisBactchIncrbyMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	
	vector<SKeyValue> vecKeyValue;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestVecKeyValue(dwServiceId, pBody, dwBodyLen, vecKeyValue);
	string strLogValue = VectorToString1(vecKeyValue);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, nRet, "", pRedisBactchIncrbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	//check param
	for(unsigned int i = 0;i < vecKeyValue.size(); i++)
	{
		SKeyValue &keyValue = vecKeyValue[i];
		if(!IsStrDigit(keyValue.strValue))
		{
			m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, RVS_PARAMETER_ERROR, "", pRedisBactchIncrbyMsg->fSpentTimeInQueue);
		    m_pRedisVirtualService->Response(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		    return;
		}
	}

	CRedisContainer *pRedisContainer = pRedisBactchIncrbyMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, RVS_NO_REDIS_SERVER, "", pRedisBactchIncrbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.BatchIncrby(vecKeyValue, strIpAddrs);
	strLogValue = VectorToString1(vecKeyValue);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchIncrbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	ResponseBatch(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, pBody, dwBodyLen, vecKeyValue);
	m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchIncrbyMsg->fSpentTimeInQueue);

}

void CRedisStringMsgOperator::DoBatchExpire(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringBatchExpireMsg *pRedisStringBatchExpireMsg = (SRedisStringBatchExpireMsg *)pRedisStringOperateMsg;

    BatchExpire(pRedisStringBatchExpireMsg->dwServiceId,
		pRedisStringBatchExpireMsg->dwMsgId,
		pRedisStringBatchExpireMsg->dwSequenceId,
		pRedisStringBatchExpireMsg->strGuid,
		pRedisStringBatchExpireMsg->handler,
		pRedisStringBatchExpireMsg->pBuffer,
		pRedisStringBatchExpireMsg->dwLen,
		pRedisStringBatchExpireMsg->tStart,
		pRedisStringBatchExpireMsg->fSpentTimeInQueue,
		pRedisStringBatchExpireMsg->pRedisContainer);
}

void CRedisStringMsgOperator::DoBatchDelete(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringBatchDeleteMsg *pRedisStringBatchDeleteMsg = (SRedisStringBatchDeleteMsg *)pRedisStringOperateMsg;

    BatchDelete(pRedisStringBatchDeleteMsg->dwServiceId,
		pRedisStringBatchDeleteMsg->dwMsgId,
		pRedisStringBatchDeleteMsg->dwSequenceId,
		pRedisStringBatchDeleteMsg->strGuid,
		pRedisStringBatchDeleteMsg->handler,
		pRedisStringBatchDeleteMsg->pBuffer,
		pRedisStringBatchDeleteMsg->dwLen,
		pRedisStringBatchDeleteMsg->tStart,
		pRedisStringBatchDeleteMsg->fSpentTimeInQueue,
		pRedisStringBatchDeleteMsg->pRedisContainer);

}

void CRedisStringMsgOperator::DoGetInt(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	SRedisStringGetMsg *pRedisStringGetMsg = (SRedisStringGetMsg *)pRedisStringOperateMsg;
	unsigned int dwServiceId = pRedisStringGetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisStringGetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisStringGetMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisStringGetMsg->pBuffer, pRedisStringGetMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisStringGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisStringGetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisStringGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Get(strKey, strValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());
	if(IsStrDigit(strValue))
	{
		//发送响应
		int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_STRING_GET_INT);
		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d#", dwCode);
		strValue = szTemp + strValue;
		m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
		Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);
	}
	else
	{
		m_pRedisVirtualService->Log(pRedisStringGetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringGetMsg->tStart, strKey, strValue, dwKeepTime, RVS_REDIS_DATA_FORMAT_ERROR, strIpAddrs, pRedisStringGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_DATA_FORMAT_ERROR);
	}
}

void CRedisStringMsgOperator::DoSetInt(SRedisStringOperateMsg *pRedisStringOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s\n", __FUNCTION__);
	
	SRedisStringSetMsg *pRedisStringSetMsg = (SRedisStringSetMsg *)pRedisStringOperateMsg;
	
	CSapDecoder decoder(pRedisStringSetMsg->pBuffer, pRedisStringSetMsg->dwLen);
	unsigned int dwServiceId = pRedisStringSetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisStringSetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisStringSetMsg->dwSequenceId;
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue = "0";
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisStringSetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringSetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisStringSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CRedisContainer *pRedisContainer = pRedisStringSetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisStringSetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringSetMsg->tStart, strKey, strValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisStringSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisStringSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisStringAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, strValue, dwKeepTime, strIpAddrs);
	
	m_pRedisVirtualService->Log(pRedisStringSetMsg->strGuid, dwServiceId, dwMsgId, pRedisStringSetMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisStringSetMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisStringSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

int CRedisStringMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, unsigned int &dwKeepTime)
{
    multimap<unsigned int, string> mapRequest;
	multimap<unsigned int, string> mapResponse;

	SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
    CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];

	REDIS_SET_TIME_MAP &mapRedisKeepTime = m_pRedisVirtualService->GetRedisKeepTime();
	dwKeepTime = mapRedisKeepTime[dwServiceId];

	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	while(ptrLoc < (unsigned char *)pBuffer + dwLen)
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen=ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
			return RVS_AVENUE_PACKET_ERROR;
		}

		unsigned short nType = ntohs(pAtti->wType);
		string strValue = string((char *)(ptrLoc+sizeof(SSapMsgAttribute)), nLen-sizeof(SSapMsgAttribute));

		if(mapTypeByCode[nType] == MSG_FIELD_INT)
		{
			char szTemp[16] = {0};
			snprintf(szTemp, sizeof(szTemp)-1, "%d", ntohl(*(int *)strValue.c_str()));
			strValue = szTemp;
		}

		if(nType < 10000)
		{
			mapRequest.insert(make_pair(nType, strValue));
		}
		else if(nType == 10000)
		{
			dwKeepTime = atoi(strValue.c_str());
		}
		else if(nType > 10000 && nType < 20000)
		{
			mapResponse.insert(make_pair(nType, strValue));
		}

		ptrLoc+=nFactLen;
	}
	
    if(mapRequest.size() == 0)
    {
    	return RVS_KEY_IS_NULL;
    }
	//key前缀
	char szPrefix[16] = {0};
	snprintf(szPrefix, sizeof(szPrefix), "RVS%d", dwServiceId);
	strKey = szPrefix;
	
	//获取key值
	multimap<unsigned int, string>::const_iterator iterRequest;
	for (iterRequest = mapRequest.begin(); iterRequest != mapRequest.end(); iterRequest++)
	{
		strKey += "_" ;
		strKey += iterRequest->second.c_str();
	}
	
	if(strKey.length() > 1024)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisStringMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}

	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}

	//获取value值
	if(strValue == "0")
	{
	    if(mapResponse.size() != 0)
	    {
	    	multimap<unsigned int, string>::const_iterator iterResponse = mapResponse.begin();
		    RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, Key[%s], Only One Value[%s]\n", __FUNCTION__, strKey.c_str(), iterResponse->second.c_str());
		    if(IsStrDigit(iterResponse->second))
		    {
			    strValue = iterResponse->second;
			    return 0;
		    }
		    else
		    {
			    return RVS_REDIS_DATA_FORMAT_ERROR;
		    }
	    }
		else
		{
			return RVS_REDIS_DATA_FORMAT_ERROR;
		}
		
	}
	
	vector<RVSKeyValue> vecKeyValue;
	multimap<unsigned int, string>::const_iterator iterResponse;
    for (iterResponse = mapResponse.begin(); iterResponse != mapResponse.end(); iterResponse++)
	{
	    RVSKeyValue tempKeyValue;
		tempKeyValue.dwKey = iterResponse->first;
		tempKeyValue.strValue = iterResponse->second;
		vecKeyValue.push_back(tempKeyValue);
    }

	strValue = CRvsArchiveAdapter::Oarchive(vecKeyValue);
	return 0;
}

int CRedisStringMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, int &incbyNum)
{
    multimap<unsigned int, string> mapRequest;

	SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
    CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];

	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	while(ptrLoc < (unsigned char *)pBuffer + dwLen)
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen=ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
			return RVS_AVENUE_PACKET_ERROR;
		}

		unsigned short nType = ntohs(pAtti->wType);
		string strValue = string((char *)(ptrLoc+sizeof(SSapMsgAttribute)), nLen-sizeof(SSapMsgAttribute));

		if(mapTypeByCode[nType] == MSG_FIELD_INT)
		{
			char szTemp[16] = {0};
			snprintf(szTemp, sizeof(szTemp)-1, "%d", ntohl(*(int *)strValue.c_str()));
			strValue = szTemp;
		}

		if(nType < 10000)
		{
			mapRequest.insert(make_pair(nType, strValue));
		}
		
		else
		{
			CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	        SServiceConfig *serviceConfig;
	        oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
		    SConfigType oConfigType;
			serviceConfig->oConfig.GetTypeByCode(nType, oConfigType);
			if(oConfigType.strName.compare(INCBY_TYPE_IDENTIFIER) == 0)
			{
				if(oConfigType.eType != MSG_FIELD_INT)
				{
				    return RVS_REDIS_CONFIG_ERROR;
				}
				incbyNum = atoi(strValue.c_str());
			}
		}

		ptrLoc+=nFactLen;
	}

    if(mapRequest.size() == 0)
    {
    	return RVS_KEY_IS_NULL;
    }
	//key前缀
	char szPrefix[16] = {0};
	snprintf(szPrefix, sizeof(szPrefix), "RVS%d", dwServiceId);
	strKey = szPrefix;
	
	//获取key值
	multimap<unsigned int, string>::const_iterator iterRequest;
	for (iterRequest = mapRequest.begin(); iterRequest != mapRequest.end(); iterRequest++)
	{
		strKey += "_" ;
		strKey += iterRequest->second.c_str();
	}
	
	if(strKey.length() > 1024)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisStringMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}

	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}

	return 0;
}

void CRedisStringMsgOperator::ResponseFromVec( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const vector<RVSKeyValue> & vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);
	
	SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
	CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];

	vector<RVSKeyValue>::const_iterator iter;
	for (iter = vecKeyValue.begin(); iter != vecKeyValue.end(); iter++)
	{
		if(mapTypeByCode[iter->dwKey] == MSG_FIELD_INT)
		{
			sapEncoder.SetValue(iter->dwKey, atoi(iter->strValue.c_str()));
		}
		else
		{
			sapEncoder.SetValue(iter->dwKey, iter->strValue);
		}
	}

	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisStringMsgOperator::ResponseBatch( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode,const void *pBuffer, unsigned int dwLen, const vector<SKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, dwMsgId);
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByCode(structCode, oConfigType);
	vector<SConfigField> &vecConfigField = oConfigType.vecConfigField;

    int lastFieldLen = vecConfigField.back().nLen;
	if(vecConfigField.back().eStructFieldType == MSG_FIELD_INT)
	{
		lastFieldLen = 4;
	}
	int lastFieldCode = vecConfigField.back().nCode;
	int totalStructFieldLen = 0;
    int valueBeginPos = -1;
	int totalKeyLen = 0;
	RVS_XLOG(XLOG_DEBUG, "CRedisStringMsgOperator::%s, lastFieldLen[%d]\n", __FUNCTION__, lastFieldLen);
	for(unsigned int i = 0;i < vecConfigField.size(); i++)
	{
		SConfigField &field = vecConfigField[i];

        if(field.eStructFieldType == MSG_FIELD_INT)
	    {
	    	totalStructFieldLen += 4;
	    }
		else if(field.nLen >= 0)
		{
			totalStructFieldLen += field.nLen;
		}
		
		if(field.nCode >= 10000)
		{
		    if(valueBeginPos == -1)
		    {
		    	valueBeginPos = i;
		    }
		}
		else
		{
			totalKeyLen += field.nLen;
		}
	}

    unsigned char *ptrLoc = (unsigned char *)pBuffer;
	unsigned int i = 0;
	map<unsigned int, string> mapCodeValue;
	while(i < vecKeyValue.size() && ptrLoc < ((unsigned char *)pBuffer + dwLen))
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen = ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisStringMsgOperator::%s, Packet Error\n", __FUNCTION__);
			return;
		}

        const SKeyValue &keyValue = vecKeyValue[i];
		vector<RVSKeyValue> vecResult;
		int resultLen = totalStructFieldLen;
		bool valueIsDigit = false;
		if(IsStrDigit(keyValue.strValue))
		{
			valueIsDigit = true;
			if(lastFieldLen < 0)
			{
				resultLen += keyValue.strValue.length();
			}	
		}
		else
		{
			 if(CRvsArchiveAdapter::Iarchive(keyValue.strValue, vecResult) != 0)
			 {
			     RVS_XLOG(XLOG_ERROR, "CRedisStringMsgOperator::%s, Deserialize Error, strKey[%s], strValue[%s]\n", __FUNCTION__, keyValue.strKey.c_str(), keyValue.strValue.c_str());
			     return;
			 }

		     for(unsigned int j =0;j < vecResult.size();j++)
			 {
			    RVSKeyValue &codeValue = vecResult[j];
				mapCodeValue.insert(make_pair(codeValue.dwKey, codeValue.strValue));
			 }
			 if(lastFieldLen < 0)
			 {
			    map<unsigned int, string>::iterator iter = mapCodeValue.find(lastFieldCode);
				if(iter != mapCodeValue.end())
				{
					resultLen += iter->second.length();
				}
			 }
		}

	    char * const resultBuffer = (char *)malloc(resultLen);
		char *resultLoc = resultBuffer;
		memset(resultBuffer, 0 , resultLen);
	    memcpy(resultBuffer, ptrLoc+sizeof(SSapMsgAttribute), totalKeyLen);
		resultLoc += totalKeyLen;

        unsigned int configField = valueBeginPos;
        SConfigField &field = vecConfigField[configField];
		if(valueIsDigit)
		{
		     if(field.eStructFieldType == MSG_FIELD_INT)
			 {
				  int nNetValue = htonl(atoi(keyValue.strValue.c_str()));
				  memcpy(resultLoc, &nNetValue, 4);
				  resultLoc += 4;
			 }
			 else
			 {
			 	unsigned int strValuelen = keyValue.strValue.length();
			    unsigned int len = field.nLen < 0 ? strValuelen: field.nLen;
		        unsigned int copyLen = len > strValuelen ? strValuelen : len; 
			    memcpy(resultLoc, keyValue.strValue.c_str(), copyLen);
			 }
		}
		else
		{
		    for(;configField < vecConfigField.size(); configField++)
		    {
		    	SConfigField &field = vecConfigField[configField];
				map<unsigned int, string>::iterator iter = mapCodeValue.find(field.nCode);	
			    if(iter != mapCodeValue.end())
			    {
			        string &strValue = iter->second;
			        if(field.eStructFieldType == MSG_FIELD_INT)
			        {
				        int nNetValue = htonl(atoi(strValue.c_str()));
				        memcpy(resultLoc, &nNetValue, 4);
				        resultLoc += 4;
			        }
				    else
				    {
					    unsigned int strValuelen = strValue.length();
			            unsigned int len = field.nLen < 0 ? strValuelen: field.nLen;
		                unsigned int copyLen = len > strValuelen ? strValuelen : len; 
			            memcpy(resultLoc, keyValue.strValue.c_str(), copyLen);
					    resultLoc += copyLen;
				    }
			    }
			    else
			    {
				    if(field.eStructFieldType == MSG_FIELD_INT)
			        {
				        resultLoc += 4;
			        }
			        else
			        {
				        resultLoc += field.nLen;
			        }		
			    }
		    }
		}

        sapEncoder.SetValue(structCode, resultBuffer, resultLen);
        free(resultBuffer);
		mapCodeValue.clear();
		ptrLoc += nFactLen;
		i++;
	}
	
	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

string CRedisStringMsgOperator::VectorToString(const vector<RVSKeyValue> &vecKeyValue)
{
	string strValue;
	vector<RVSKeyValue>::const_iterator iter;
	bool bFirst = true;
	for (iter = vecKeyValue.begin(); iter != vecKeyValue.end(); iter++)
	{
		const RVSKeyValue &objKeyValue = *iter;

		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp)-1, "%d#", objKeyValue.dwKey);
		if(bFirst)
		{
			strValue += szTemp + objKeyValue.strValue;

			bFirst = false;
		}
		else
		{
			strValue += VALUE_SPLITTER;
			strValue += szTemp + objKeyValue.strValue;
		}
	}

	return strValue;
}

