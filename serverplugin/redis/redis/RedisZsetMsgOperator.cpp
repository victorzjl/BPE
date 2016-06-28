#include "RedisZsetMsgOperator.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "ServiceConfig.h"
#include "RedisVirtualService.h"
#include "RedisContainer.h"
#include "CommonMacro.h"
#include "RedisZsetAgentScheduler.h"

#include <boost/algorithm/string.hpp>
#include <XmlConfigParser.h>
#include <SapMessage.h>

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace boost::algorithm;
using namespace sdo::sap;
using namespace redis;

CRedisZsetMsgOperator::CRedisZsetMsgOperator(CRedisVirtualService *pRedisVirtualService):IRedisTypeMsgOperator(pRedisVirtualService)
{
   m_redisZsetFunc[RVS_ZSET_GET] = &CRedisZsetMsgOperator::DoGet;
   m_redisZsetFunc[RVS_ZSET_SET] = &CRedisZsetMsgOperator::DoSet;
   m_redisZsetFunc[RVS_ZSET_DELETE] = &CRedisZsetMsgOperator::DoDelete;
   m_redisZsetFunc[RVS_ZSET_SIZE] = &CRedisZsetMsgOperator::DoSize;
   m_redisZsetFunc[RVS_ZSET_GETALL] = &CRedisZsetMsgOperator::DoGetAll;
   m_redisZsetFunc[RVS_ZSET_GETREVERSEALL] = &CRedisZsetMsgOperator::DoGetReverseAll;
   m_redisZsetFunc[RVS_ZSET_GETSCOREALL] = &CRedisZsetMsgOperator::DoGetScoreAll;
   m_redisZsetFunc[RVS_ZSET_DELETEALL] = &CRedisZsetMsgOperator::DoDeleteAll;
   m_redisZsetFunc[RVS_ZSET_RANK] = &CRedisZsetMsgOperator::DoRank;
   m_redisZsetFunc[RVS_ZSET_REVERSERANK] = &CRedisZsetMsgOperator::DoReverseRank;
   m_redisZsetFunc[RVS_ZSET_INCBY] = &CRedisZsetMsgOperator::DoIncby;
   m_redisZsetFunc[RVS_ZSET_DELRANGEBYRANK] = &CRedisZsetMsgOperator::DoDelRangeByRank;
   m_redisZsetFunc[RVS_ZSET_DELRANGEBYSCORE] = &CRedisZsetMsgOperator::DoDelRangeByScore;
   m_redisZsetFunc[RVS_ZSET_GETKEY] = &CRedisZsetMsgOperator::DoGetKey;
   m_redisZsetFunc[RVS_ZSET_TTL] = &CRedisZsetMsgOperator::DoTtl;
   m_redisZsetFunc[RVS_ZSET_DECODE] = &CRedisZsetMsgOperator::DoDecode;
}

CRedisZsetMsgOperator::~CRedisZsetMsgOperator()
{
}

void CRedisZsetMsgOperator::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
   RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
   
   SRedisZsetOperateMsg * redisZsetOperateMsg = NULL;
   switch(dwMsgId)
   {
   	case RVS_ZSET_GET:
		redisZsetOperateMsg = new SRedisZsetGetMsg;
		break;
	case RVS_ZSET_SET:
		redisZsetOperateMsg = new SRedisZsetSetMsg;
		break;
	case RVS_ZSET_DELETE:
		redisZsetOperateMsg = new SRedisZsetDeleteMsg;
		break;
	case RVS_ZSET_SIZE:
		redisZsetOperateMsg = new SRedisZsetSizeMsg;
		break;
	case RVS_ZSET_GETALL:
		redisZsetOperateMsg = new SRedisZsetGetAllMsg;
		break;
	case RVS_ZSET_GETREVERSEALL:
		redisZsetOperateMsg = new SRedisZsetGetReverseAllMsg;
		break;
	case RVS_ZSET_GETSCOREALL:
		redisZsetOperateMsg = new SRedisZsetGetScoreAllMsg;
		break;
	case RVS_ZSET_DELETEALL:
		redisZsetOperateMsg = new SRedisZsetDeleteAllMsg;
		break;
	case RVS_ZSET_RANK:
		redisZsetOperateMsg = new SRedisZsetRankMsg;
		break;
	case RVS_ZSET_REVERSERANK:
		redisZsetOperateMsg = new SRedisZsetReverseRankMsg;
		break;
	case RVS_ZSET_INCBY:
		redisZsetOperateMsg = new SRedisZsetIncbyMsg;
		break;
	case RVS_ZSET_DELRANGEBYRANK:
		redisZsetOperateMsg = new SRedisZsetDelRangeByRankMsg;
		break;
	case RVS_ZSET_DELRANGEBYSCORE:
		redisZsetOperateMsg = new SRedisZsetDelRangeByScoreMsg;
		break;
	case RVS_ZSET_GETKEY:
		redisZsetOperateMsg = new SRedisZsetGetKeyMsg;
		break;
	case RVS_ZSET_TTL:
		redisZsetOperateMsg = new SRedisZsetTtlMsg;
		break;
	case RVS_ZSET_DECODE:
		redisZsetOperateMsg = new SRedisZsetDecodeMsg;
		break;
	default:
		RVS_XLOG(XLOG_ERROR, "CRedisZsetMsgOperator::%s, MsgId[%d] not supported\n", __FUNCTION__, dwMsgId);
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_MSG, "" , fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_UNKNOWN_MSG);
		return;
   }

   redisZsetOperateMsg->dwServiceId = dwServiceId;
   redisZsetOperateMsg->dwMsgId = dwMsgId;
   redisZsetOperateMsg->dwSequenceId = dwSequenceId;
   redisZsetOperateMsg->pBuffer = pBuffer;
   redisZsetOperateMsg->dwLen = nLen;
   redisZsetOperateMsg->handler = pHandler;
   redisZsetOperateMsg->strGuid = strGuid;
   redisZsetOperateMsg->tStart = tStart;
   redisZsetOperateMsg->fSpentTimeInQueue = fSpentInQueueTime;
   redisZsetOperateMsg->pRedisContainer = pRedisContainer;

   (this->*(m_redisZsetFunc[dwMsgId]))(redisZsetOperateMsg);

   delete redisZsetOperateMsg;
}

void CRedisZsetMsgOperator::DoGet(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetGetMsg *pRedisZsetGetMsg = (SRedisZsetGetMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetGetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetGetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetGetMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetGetMsg->pBuffer, pRedisZsetGetMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	map<string, int> mapAdditionParams;
	int score = 0;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetMsg->tStart, strKey, strField, 0, nRet, "", pRedisZsetGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(strField.empty())
	{
		m_pRedisVirtualService->Log(pRedisZsetGetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisZsetGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisZsetGetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetMsg->tStart, strKey, strField, 0, RVS_NO_REDIS_SERVER, "", pRedisZsetGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Get(strKey, strField, score, strIpAddrs);
	if(nRet != RVS_SUCESS)	
	{
		m_pRedisVirtualService->Log(pRedisZsetGetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetGetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
	    return;
	}

	 //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_GET);
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetGetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

    char szTemp[18] = {0};
	snprintf(szTemp, sizeof(szTemp), "  %d", score);
    strField += szTemp;
	m_pRedisVirtualService->Log(pRedisZsetGetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetGetMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, score);
	m_pRedisVirtualService->Response(pRedisZsetGetMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisZsetMsgOperator::DoSet(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetSetMsg *pRedisZsetSetMsg = (SRedisZsetSetMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetSetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetSetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetSetMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetSetMsg->pBuffer, pRedisZsetSetMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSetMsg->tStart, strKey, strField, 0, nRet, "", pRedisZsetSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(strField.empty())
	{
		m_pRedisVirtualService->Log(pRedisZsetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSetMsg->tStart, strKey, strField, 0, nRet, "", pRedisZsetSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisZsetSetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSetMsg->tStart, strKey, strField, 0, RVS_NO_REDIS_SERVER, "", pRedisZsetSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int result = -1;
	int dwKeepTime = 0;
	map<string, int>::const_iterator iter = mapAdditionParams.find(EXPIRE_TIME_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		dwKeepTime = mapAdditionParams[EXPIRE_TIME_IDENTIFIER];
	}
	char szScoreTemp[18] = {0};
	snprintf(szScoreTemp, sizeof(szScoreTemp), "  %d", score);
	string strLogValue = strField + szScoreTemp;
	
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, strField, score, dwKeepTime, result, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisZsetSetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
	}
	
    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_SET);
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", result);
	strLogValue += "   response[";
	strLogValue += szTemp;
	strLogValue += "]";
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisZsetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisZsetSetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	m_pRedisVirtualService->Log(pRedisZsetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisZsetSetMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, result);
	m_pRedisVirtualService->Response(pRedisZsetSetMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisZsetMsgOperator::DoDelete(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetDeleteMsg *pRedisZsetDeleteMsg = (SRedisZsetDeleteMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetDeleteMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetDeleteMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetDeleteMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetDeleteMsg->pBuffer, pRedisZsetDeleteMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDeleteMsg->tStart, strKey, strField, 0, nRet, "", pRedisZsetDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	if(strField.empty())
	{
		m_pRedisVirtualService->Log(pRedisZsetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDeleteMsg->tStart, strKey, strField, 0, RVS_PARAMETER_ERROR, "", pRedisZsetDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisZsetDeleteMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDeleteMsg->tStart, strKey, strField, 0, RVS_NO_REDIS_SERVER, "", pRedisZsetDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Delete(strKey, strField, strIpAddrs);
				
	m_pRedisVirtualService->Log(pRedisZsetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDeleteMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetDeleteMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisZsetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisZsetMsgOperator::DoSize(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetSizeMsg *pRedisZsetSizeMsg = (SRedisZsetSizeMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetSizeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetSizeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetSizeMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetSizeMsg->pBuffer, pRedisZsetSizeMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSizeMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisZsetSizeMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSizeMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisZsetSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int size = 0;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Size(strKey, size, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSizeMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisZsetSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_SIZE);
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", size);
    string strLogValue = szTemp;
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisZsetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSizeMsg->tStart, strKey, strLogValue, 0, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisZsetSizeMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	m_pRedisVirtualService->Log(pRedisZsetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetSizeMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetSizeMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, size);
	m_pRedisVirtualService->Response(pRedisZsetSizeMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisZsetMsgOperator::DoGetAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetGetAllMsg *pRedisZsetGetAllMsg = (SRedisZsetGetAllMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetGetAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetGetAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetGetAllMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisZsetGetAllMsg->pBuffer, pRedisZsetGetAllMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetAllMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisZsetGetAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetAllMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisZsetGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_GETALL);
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByCode(structCode, oConfigType);
	vector<SConfigField> &vecConfigField = oConfigType.vecConfigField;
	bool config = true;
	if(vecConfigField.size() < 1)
	{
		config = false;
	}
	else
	{
	    //结构体只有最后一项可以不配置长度，其他情况出错
		for(unsigned int i = 0; i < vecConfigField.size() - 1; i++)
		{
		    if(vecConfigField[i].eStructFieldType != MSG_FIELD_INT)
		    {
		    	if(vecConfigField[i].nLen <= 0)
			    {
				   config = false;
				   break;
			    }
		    }
		}
	}

	if(structCode == -1 || !config)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetAllMsg->tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, "", pRedisZsetGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	//获取额外参数
	int start = 0, end = -1;
	map<string, int>::const_iterator iter = mapAdditionParams.find(START_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		start = mapAdditionParams[START_TYPE_IDENTIFIER];
	}
	iter = mapAdditionParams.find(END_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		end = mapAdditionParams[END_TYPE_IDENTIFIER];
	}
	string strIpAddrs;
	map<string, string> mapFieldAndScore;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.GetAll(strKey, start, end, mapFieldAndScore, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
	    m_pRedisVirtualService->Log(pRedisZsetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetAllMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisZsetGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strLogValue;
	MapToString(mapFieldAndScore, strLogValue);

	m_pRedisVirtualService->Log(pRedisZsetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetAllMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetGetAllMsg->fSpentTimeInQueue);
	ResponseStructArrayFromMap(pRedisZsetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, mapFieldAndScore, structCode, vecConfigField);
}

void CRedisZsetMsgOperator::DoGetReverseAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetGetReverseAllMsg *pRedisZsetGetReverseAllMsg = (SRedisZsetGetReverseAllMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetGetReverseAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetGetReverseAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetGetReverseAllMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisZsetGetReverseAllMsg->pBuffer, pRedisZsetGetReverseAllMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetReverseAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetReverseAllMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetGetReverseAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetReverseAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisZsetGetReverseAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetReverseAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetReverseAllMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisZsetGetReverseAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetReverseAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_GETREVERSEALL);
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByCode(structCode, oConfigType);
	vector<SConfigField> &vecConfigField = oConfigType.vecConfigField;
	bool config = true;
	if(vecConfigField.size() < 1)
	{
		config = false;
	}
	else
	{
	    //结构体只有最后一项可以不配置长度，其他情况出错
		for(unsigned int i = 0; i < vecConfigField.size() - 1; i++)
		{
		    if(vecConfigField[i].eStructFieldType != MSG_FIELD_INT)
		    {
		    	if(vecConfigField[i].nLen <= 0)
			    {
				   config = false;
				   break;
			    }
		    }
		}
	}

	if(structCode == -1 || !config)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetReverseAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetReverseAllMsg->tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, "", pRedisZsetGetReverseAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetReverseAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	//获取额外参数
	int start = 0, end = -1;
	map<string, int>::const_iterator iter = mapAdditionParams.find(START_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		start = mapAdditionParams[START_TYPE_IDENTIFIER];
	}
	iter = mapAdditionParams.find(END_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		end = mapAdditionParams[END_TYPE_IDENTIFIER];
	}
	string strIpAddrs;
	map<string, string> mapFieldAndScore;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.GetReverseAll(strKey, start, end, mapFieldAndScore, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
	    m_pRedisVirtualService->Log(pRedisZsetGetReverseAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetReverseAllMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisZsetGetReverseAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetReverseAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strLogValue;
	MapToString(mapFieldAndScore, strLogValue);

	m_pRedisVirtualService->Log(pRedisZsetGetReverseAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetReverseAllMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetGetReverseAllMsg->fSpentTimeInQueue);
	ResponseStructArrayFromMap(pRedisZsetGetReverseAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, mapFieldAndScore, structCode, vecConfigField);
}

void CRedisZsetMsgOperator::DoGetScoreAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetGetScoreAllMsg *pRedisZsetGetScoreAllMsg = (SRedisZsetGetScoreAllMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetGetScoreAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetGetScoreAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetGetScoreAllMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisZsetGetScoreAllMsg->pBuffer, pRedisZsetGetScoreAllMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetScoreAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetScoreAllMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetGetScoreAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetScoreAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisZsetGetScoreAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetScoreAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetScoreAllMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisZsetGetScoreAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetScoreAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_GETSCOREALL);
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByCode(structCode, oConfigType);
	vector<SConfigField> &vecConfigField = oConfigType.vecConfigField;
	bool config = true;
	if(vecConfigField.size() < 1)
	{
		config = false;
	}
	else
	{
	    //结构体只有最后一项可以不配置长度，其他情况出错
		for(unsigned int i = 0; i < vecConfigField.size() - 1; i++)
		{
		    if(vecConfigField[i].eStructFieldType != MSG_FIELD_INT)
		    {
		    	if(vecConfigField[i].nLen <= 0)
			    {
				   config = false;
				   break;
			    }
		    }
		}
	}

	if(structCode == -1 || !config)
	{
		m_pRedisVirtualService->Log(pRedisZsetGetScoreAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetScoreAllMsg->tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, "", pRedisZsetGetScoreAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetGetScoreAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	//获取额外参数
	int start = 0, end = -1;
	bool bInputStart = false, bInputEnd = false;
	map<string, int>::const_iterator iter = mapAdditionParams.find(START_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		start = mapAdditionParams[START_TYPE_IDENTIFIER];
		bInputStart = true;
	}
	iter = mapAdditionParams.find(END_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		end = mapAdditionParams[END_TYPE_IDENTIFIER];
		bInputEnd = true;
	}
	string strIpAddrs;
	map<string, string> mapFieldAndScore;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.GetScoreAll(strKey, start, end, mapFieldAndScore, strIpAddrs, bInputStart, bInputEnd);
	if(nRet != RVS_SUCESS)
	{
	    m_pRedisVirtualService->Log(pRedisZsetGetScoreAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetScoreAllMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisZsetGetScoreAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetGetScoreAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strLogValue;
	MapToString(mapFieldAndScore, strLogValue);

	m_pRedisVirtualService->Log(pRedisZsetGetScoreAllMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetGetScoreAllMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetGetScoreAllMsg->fSpentTimeInQueue);
	ResponseStructArrayFromMap(pRedisZsetGetScoreAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, mapFieldAndScore, structCode, vecConfigField);
}

void CRedisZsetMsgOperator::DoDeleteAll(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetDeleteAllMsg *pRedisZsetDeleteAllMsg = (SRedisZsetDeleteAllMsg *)pRedisZsetOperateMsg;
	
	DeleteAll(pRedisZsetDeleteAllMsg->dwServiceId,
		pRedisZsetDeleteAllMsg->dwMsgId,
		pRedisZsetDeleteAllMsg->dwSequenceId,
		pRedisZsetDeleteAllMsg->strGuid,
		pRedisZsetDeleteAllMsg->handler,
		pRedisZsetDeleteAllMsg->pBuffer,
		pRedisZsetDeleteAllMsg->dwLen,
		pRedisZsetDeleteAllMsg->tStart,
		pRedisZsetDeleteAllMsg->fSpentTimeInQueue,
		pRedisZsetDeleteAllMsg->pRedisContainer);
}

void CRedisZsetMsgOperator::DoRank(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetRankMsg *pRedisZsetRankMsg = (SRedisZsetRankMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetRankMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetRankMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetRankMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetRankMsg->pBuffer, pRedisZsetRankMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetRankMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	if(strField.empty())
	{
		m_pRedisVirtualService->Log(pRedisZsetRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetRankMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisZsetRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisZsetRankMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetRankMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisZsetRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int rank = 0;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Rank(strKey, strField, rank, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetRankMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_RANK);
	char szTemp[18] = {0};
	snprintf(szTemp, sizeof(szTemp), "  %d", rank);
    string strLogValue = strField + szTemp;
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisZsetRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetRankMsg->tStart, strKey, strLogValue, 0, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisZsetRankMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	m_pRedisVirtualService->Log(pRedisZsetRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetRankMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetRankMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, rank);
	m_pRedisVirtualService->Response(pRedisZsetRankMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisZsetMsgOperator::DoReverseRank(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetReverseRankMsg *pRedisZsetReverseRankMsg = (SRedisZsetReverseRankMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetReverseRankMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetReverseRankMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetReverseRankMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetReverseRankMsg->pBuffer, pRedisZsetReverseRankMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetReverseRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetReverseRankMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetReverseRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetReverseRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	if(strField.empty())
	{
		m_pRedisVirtualService->Log(pRedisZsetReverseRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetReverseRankMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisZsetReverseRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetReverseRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisZsetReverseRankMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetReverseRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetReverseRankMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisZsetReverseRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetReverseRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int rank = 0;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.ReverseRank(strKey, strField, rank, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetReverseRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetReverseRankMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetReverseRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetReverseRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_ZSET_REVERSERANK);
	char szTemp[18] = {0};
	snprintf(szTemp, sizeof(szTemp), "  %d", rank);
    string strLogValue = strField + szTemp;
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisZsetReverseRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetReverseRankMsg->tStart, strKey, strLogValue, 0, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisZsetReverseRankMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisZsetReverseRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	m_pRedisVirtualService->Log(pRedisZsetReverseRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetReverseRankMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetReverseRankMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, rank);
	m_pRedisVirtualService->Response(pRedisZsetReverseRankMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisZsetMsgOperator::DoIncby(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetIncbyMsg *pRedisZsetIncbyMsg = (SRedisZsetIncbyMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetIncbyMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetIncbyMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetIncbyMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetIncbyMsg->pBuffer, pRedisZsetIncbyMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetIncbyMsg->tStart, strKey, strField, 0, nRet, "", pRedisZsetIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	if(strField.empty())
	{
		m_pRedisVirtualService->Log(pRedisZsetIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetIncbyMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisZsetIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}

	//获取额外参数
	int incByNum = 0;
	map<string, int>::const_iterator iter = mapAdditionParams.find(INCBY_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		incByNum = mapAdditionParams[INCBY_TYPE_IDENTIFIER];
	}
	char szTemp[18] = {0};
	snprintf(szTemp, sizeof(szTemp), "  %d", incByNum);
    string strLogValue = strField + szTemp;
	
	CRedisContainer *pRedisContainer = pRedisZsetIncbyMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetIncbyMsg->tStart, strKey, strLogValue, 0, RVS_NO_REDIS_SERVER, "", pRedisZsetIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Incby(strKey, strField, incByNum, strIpAddrs);

	m_pRedisVirtualService->Log(pRedisZsetIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetIncbyMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisZsetIncbyMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisZsetIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisZsetMsgOperator::DoDelRangeByRank(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetDelRangeByRankMsg *pRedisZsetDelRangeByRankMsg = (SRedisZsetDelRangeByRankMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetDelRangeByRankMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetDelRangeByRankMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetDelRangeByRankMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetDelRangeByRankMsg->pBuffer, pRedisZsetDelRangeByRankMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetDelRangeByRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDelRangeByRankMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetDelRangeByRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDelRangeByRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	//获取额外参数
	int start = 0, end = -1;
	map<string, int>::const_iterator iter = mapAdditionParams.find(START_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		start = mapAdditionParams[START_TYPE_IDENTIFIER];
	}
	iter = mapAdditionParams.find(END_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		end = mapAdditionParams[END_TYPE_IDENTIFIER];
	}
	//用于日志记录
	char szTemp[34] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d %d", start, end);
    strField = szTemp;
	
	CRedisContainer *pRedisContainer = pRedisZsetDelRangeByRankMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetDelRangeByRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDelRangeByRankMsg->tStart, strKey, strField, 0, RVS_NO_REDIS_SERVER, "", pRedisZsetDelRangeByRankMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDelRangeByRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	string strIpAddrs;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.DelRangeByRank(strKey, start, end, strIpAddrs);

	m_pRedisVirtualService->Log(pRedisZsetDelRangeByRankMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDelRangeByRankMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetDelRangeByRankMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisZsetDelRangeByRankMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisZsetMsgOperator::DoDelRangeByScore(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetDelRangeByScoreMsg *pRedisZsetDelRangeByScoreMsg = (SRedisZsetDelRangeByScoreMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetDelRangeByScoreMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetDelRangeByScoreMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetDelRangeByScoreMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisZsetDelRangeByScoreMsg->pBuffer, pRedisZsetDelRangeByScoreMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strField;
	int score = 0;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strField, score, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisZsetDelRangeByScoreMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDelRangeByScoreMsg->tStart, strKey, "", 0, nRet, "", pRedisZsetDelRangeByScoreMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDelRangeByScoreMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	//获取额外参数
	int start = 0, end = -1;
	bool bInputStart = false, bInputEnd = false;
	map<string, int>::const_iterator iter = mapAdditionParams.find(START_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		start = mapAdditionParams[START_TYPE_IDENTIFIER];
		bInputStart = true;
	}
	iter = mapAdditionParams.find(END_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		end = mapAdditionParams[END_TYPE_IDENTIFIER];
		bInputEnd = true;
	}
	if(bInputStart)
	{
		char szTemp[16] = {0};
	    snprintf(szTemp, sizeof(szTemp), "%d", start);
		strField = szTemp;
	}
	else
	{
		strField = "-inf";
	}
	strField += "  ";
	if(bInputEnd)
	{
		char szTemp[16] = {0};
	    snprintf(szTemp, sizeof(szTemp), "%d", end);
		strField += szTemp;
	}
	else
	{
		strField += "+inf";
	}
		
	CRedisContainer *pRedisContainer = pRedisZsetDelRangeByScoreMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisZsetDelRangeByScoreMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDelRangeByScoreMsg->tStart, strKey, strField, 0, RVS_NO_REDIS_SERVER, "", pRedisZsetDelRangeByScoreMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisZsetDelRangeByScoreMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	string strIpAddrs;
	CRedisZsetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.DelRangeByScore(strKey, start, end, strIpAddrs, bInputStart, bInputEnd);

	m_pRedisVirtualService->Log(pRedisZsetDelRangeByScoreMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDelRangeByScoreMsg->tStart, strKey, strField, 0, nRet, strIpAddrs, pRedisZsetDelRangeByScoreMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisZsetDelRangeByScoreMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisZsetMsgOperator::DoGetKey(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetGetKeyMsg *pRedisZsetGetKeyMsg = (SRedisZsetGetKeyMsg *)pRedisZsetOperateMsg;
	
	GetKey(pRedisZsetGetKeyMsg->dwServiceId,
			pRedisZsetGetKeyMsg->dwMsgId,
			pRedisZsetGetKeyMsg->dwSequenceId,
			pRedisZsetGetKeyMsg->strGuid,
			pRedisZsetGetKeyMsg->handler,
			pRedisZsetGetKeyMsg->pBuffer,
			pRedisZsetGetKeyMsg->dwLen,
			pRedisZsetGetKeyMsg->tStart,
			pRedisZsetGetKeyMsg->fSpentTimeInQueue,
			RVS_ZSET_GETKEY);

}

void CRedisZsetMsgOperator::DoTtl(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetTtlMsg *pRedisZsetTtlMsg = (SRedisZsetTtlMsg *)pRedisZsetOperateMsg;

    Ttl(pRedisZsetTtlMsg->dwServiceId,
		pRedisZsetTtlMsg->dwMsgId,
		pRedisZsetTtlMsg->dwSequenceId,
		pRedisZsetTtlMsg->strGuid,
		pRedisZsetTtlMsg->handler,
		pRedisZsetTtlMsg->pBuffer,
		pRedisZsetTtlMsg->dwLen,
		pRedisZsetTtlMsg->tStart,
		pRedisZsetTtlMsg->fSpentTimeInQueue,
		pRedisZsetTtlMsg->pRedisContainer,
		RVS_ZSET_TTL);

}

void CRedisZsetMsgOperator::DoDecode(SRedisZsetOperateMsg * pRedisZsetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	SRedisZsetDecodeMsg *pRedisZsetDecodeMsg = (SRedisZsetDecodeMsg *)pRedisZsetOperateMsg;
	unsigned int dwServiceId = pRedisZsetDecodeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisZsetDecodeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisZsetDecodeMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisZsetDecodeMsg->pBuffer, pRedisZsetDecodeMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strEncodedField, strEncodedValue;
	GetRequestEnocdedInfo(dwServiceId, pBody, dwBodyLen, strEncodedField, strEncodedValue);

	 //获得响应里的Code码
	if(!strEncodedValue.empty())
	{
		int dwCode = GetFirstResponseValueCodeByMsgId(dwServiceId, RVS_ZSET_DECODE, 20001);
	    if(dwCode == -1)
	    {
		   strEncodedValue = "";
	    }
        else
        {
    	   char szTemp[16] = {0};
	       snprintf(szTemp, sizeof(szTemp), "%d#", dwCode);
           strEncodedValue.insert(0, szTemp);
        }
	}
   
	strEncodedField += VALUE_SPLITTER + strEncodedValue;
	m_pRedisVirtualService->Log(pRedisZsetDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisZsetDecodeMsg->tStart, "", strEncodedField, 0, RVS_SUCESS, "", pRedisZsetDecodeMsg->fSpentTimeInQueue);
	Response(pRedisZsetDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_SUCESS, strEncodedField);
}

int CRedisZsetMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strField, int &score, map<string, int> &mapAdditionParams)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetMsgOperator::%s\n", __FUNCTION__);
	multimap<unsigned int, string> mapKey;
	map<unsigned int, string> mapField;
	
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
			snprintf(szTemp, sizeof(szTemp), "%d", ntohl(*(int *)strValue.c_str()));
			strValue = szTemp;
		}
	  
		if(nType < 10000)
	    {
			mapKey.insert(make_pair(nType, strValue));
		}
		else if(nType == 10000)
		{
			mapAdditionParams[EXPIRE_TIME_IDENTIFIER] = atoi(strValue.c_str());
		}
		else if(nType > 10000 && nType < 20000)
		{
		    mapField.insert(make_pair(nType, strValue));
		}
		else if(nType > 20000 && nType < 30000)
		{
			score = atoi(strValue.c_str());
		}
		else
		{
		    if(mapAdditionParams.size() <= 20)
		    {
		    	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	            SServiceConfig *serviceConfig;
	            oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
		        SConfigType oConfigType;
			    serviceConfig->oConfig.GetTypeByCode(nType, oConfigType);
			    mapAdditionParams[oConfigType.strName] = atoi(strValue.c_str());
		    }
		}
		
		ptrLoc += nFactLen;
	}

    //如果没有传超时时间，取默认配置
    map<string ,int>::const_iterator findTimeIter = mapAdditionParams.find(EXPIRE_TIME_IDENTIFIER);
	if(findTimeIter == mapAdditionParams.end())
	{
		REDIS_SET_TIME_MAP &mapRedisKeepTime = m_pRedisVirtualService->GetRedisKeepTime();
	    REDIS_SET_TIME_MAP::const_iterator keepTimeIter =mapRedisKeepTime.find(dwServiceId);
	    if(keepTimeIter != mapRedisKeepTime.end())
	    {
		   mapAdditionParams[EXPIRE_TIME_IDENTIFIER] = keepTimeIter->second;
	    }
	}
       
	if(mapKey.size() == 0)
    {
    	return RVS_KEY_IS_NULL;
    }
	
	//key前缀
	char szPrefix[16] = {0};
	snprintf(szPrefix, sizeof(szPrefix), "RVS%d", dwServiceId);
	strKey = szPrefix;

	//获取key值
	multimap<unsigned int, string>::const_iterator iterKey;
	for (iterKey = mapKey.begin(); iterKey != mapKey.end(); iterKey++)
	{
		strKey += "_";
		strKey += iterKey->second.c_str();
	}
	if(strKey.length() > 1024)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}
	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}

    strField = "";
	map<unsigned int, string>::iterator iter;
	for(iter = mapField.begin(); iter != mapField.end(); iter++)
    {
    	char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d#", iter->first);
		string temp = szTemp;
		temp += iter->second.c_str();
    	if(strField.empty())
    	{
    		strField = temp;
    	}
		else
		{
		    strField += VALUE_SPLITTER + temp; 
		}
    }
	
	return RVS_SUCESS;
}

