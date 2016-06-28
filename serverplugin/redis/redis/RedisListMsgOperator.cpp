#include "RedisListMsgOperator.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "ServiceConfig.h"
#include "RedisVirtualService.h"
#include "RedisContainer.h"
#include "CommonMacro.h"
#include "RedisListAgentScheduler.h"

#include <SapMessage.h>
#include <map>

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace sdo::sap;
using std::multimap;
using namespace redis;

CRedisListMsgOperator::CRedisListMsgOperator(CRedisVirtualService *pRedisVirtualService):IRedisTypeMsgOperator(pRedisVirtualService)
{
    m_redisListFunc[RVS_LIST_GET] = &CRedisListMsgOperator::DoGet;
	m_redisListFunc[RVS_LIST_SET] = &CRedisListMsgOperator::DoSet;
	m_redisListFunc[RVS_LIST_DELETE] = &CRedisListMsgOperator::DoDelete;
	m_redisListFunc[RVS_LIST_SIZE] = &CRedisListMsgOperator::DoSize;
	m_redisListFunc[RVS_LIST_GETALL] = &CRedisListMsgOperator::DoGetAll;
	m_redisListFunc[RVS_LIST_DELETEALL] = &CRedisListMsgOperator::DoDeleteAll;
	m_redisListFunc[RVS_LIST_POPBACK] = &CRedisListMsgOperator::DoPopBack;
	m_redisListFunc[RVS_LIST_PUSHBACK] = &CRedisListMsgOperator::DoPushBack;
	m_redisListFunc[RVS_LIST_POPFRONT] = &CRedisListMsgOperator::DoPopFront;
	m_redisListFunc[RVS_LIST_PUSHFRONT] = &CRedisListMsgOperator::DoPushFront;
	m_redisListFunc[RVS_LIST_RESERVE] = &CRedisListMsgOperator::DoReserve;
	m_redisListFunc[RVS_LIST_GETKEY] = &CRedisListMsgOperator::DoGetKey;
	m_redisListFunc[RVS_LIST_TTL] = &CRedisListMsgOperator::DoTtl;
	m_redisListFunc[RVS_LIST_DECODE] = &CRedisListMsgOperator::DoDecode;
}

void CRedisListMsgOperator::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime,CRedisContainer *pRedisContainer)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	   
	SRedisListOperateMsg * redisListOperateMsg = NULL;
	switch(dwMsgId)
	{
		case RVS_LIST_GET:
			redisListOperateMsg = new SRedisListGetMsg;
			break;
		case RVS_LIST_SET:
			redisListOperateMsg = new SRedisListSetMsg;
			break;
		case RVS_LIST_DELETE:
			redisListOperateMsg = new SRedisListDeleteMsg;
			break;
		case RVS_LIST_SIZE:
			redisListOperateMsg = new SRedisListSizeMsg;
			break;
		case RVS_LIST_GETALL:
			redisListOperateMsg = new SRedisListGetAllMsg;
			break;
		case RVS_LIST_DELETEALL:
			redisListOperateMsg = new SRedisListDeleteAllMsg;
			break;
		case RVS_LIST_POPBACK:
			redisListOperateMsg = new SRedisListPopBackMsg;
			break;
		case RVS_LIST_PUSHBACK:
			redisListOperateMsg = new SRedisListPushBackMsg;
			break;
		case RVS_LIST_POPFRONT:
			redisListOperateMsg = new SRedisListPopFrontMsg;
			break;
		case RVS_LIST_PUSHFRONT:
			redisListOperateMsg = new SRedisListPushFrontMsg;
			break;
		case RVS_LIST_RESERVE:
			redisListOperateMsg = new SRedisListReserveMsg;
			break;
		case RVS_LIST_GETKEY:
			redisListOperateMsg = new SRedisListGetKeyMsg;
			break;
		case RVS_LIST_TTL:
			redisListOperateMsg = new SRedisListTtlMsg;
			break;
		case RVS_LIST_DECODE:
			redisListOperateMsg = new SRedisListDecodeMsg;
			break;
		default:
			RVS_XLOG(XLOG_ERROR, "CRedisListMsgOperator::%s, MsgId[%d] not supported\n", __FUNCTION__, dwMsgId);
			m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_MSG, "" , fSpentInQueueTime);
			m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_UNKNOWN_MSG);
			return;
	 }
	
	 redisListOperateMsg->dwServiceId = dwServiceId;
	 redisListOperateMsg->dwMsgId = dwMsgId;
	 redisListOperateMsg->dwSequenceId = dwSequenceId;
	 redisListOperateMsg->pBuffer = pBuffer;
	 redisListOperateMsg->dwLen = nLen;
	 redisListOperateMsg->handler = pHandler;
	 redisListOperateMsg->strGuid = strGuid;
	 redisListOperateMsg->tStart = tStart;
	 redisListOperateMsg->fSpentTimeInQueue = fSpentInQueueTime;
	 redisListOperateMsg->pRedisContainer = pRedisContainer;
	
	 (this->*(m_redisListFunc[dwMsgId]))(redisListOperateMsg);
	
	 delete redisListOperateMsg;

}

void CRedisListMsgOperator::DoGet(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListGetMsg *pRedisListGetMsg = (SRedisListGetMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListGetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListGetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListGetMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisListGetMsg->pBuffer, pRedisListGetMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
    if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisListGetMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetMsg->tStart, strKey, "", 0, nRet, "", pRedisListGetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
	}
	//获取额外参数
	map<string, int>::const_iterator iter = mapAdditionParams.find(INDEX_TYPE_IDENTIFIER);
	if(iter == mapAdditionParams.end())
	{
		m_pRedisVirtualService->Log(pRedisListGetMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisListGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisListGetMsg->pRedisContainer;
    if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisListGetMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisListGetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
	
	string strIpAddrs;
	int index = iter->second;
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Get(strKey, strValue, index, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisListGetMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisListGetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
    }

	m_pRedisVirtualService->Log(pRedisListGetMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListGetMsg->fSpentTimeInQueue);
	Response(pRedisListGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);
}

void CRedisListMsgOperator::DoSet(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListSetMsg *pRedisListSetMsg = (SRedisListSetMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListSetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListSetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListSetMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisListSetMsg->pBuffer, pRedisListSetMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
		
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListSetMsg->strGuid, dwServiceId, dwMsgId, pRedisListSetMsg->tStart, strKey, "", 0, nRet, "", pRedisListSetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisListSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	//获取额外参数
	map<string, int>::const_iterator iter = mapAdditionParams.find(INDEX_TYPE_IDENTIFIER);
	if(iter == mapAdditionParams.end() || strValue.empty())
	{
		m_pRedisVirtualService->Log(pRedisListSetMsg->strGuid, dwServiceId, dwMsgId, pRedisListSetMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisListSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisListSetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisListSetMsg->strGuid, dwServiceId, dwMsgId, pRedisListSetMsg->tStart, strKey, strValue, 0, RVS_NO_REDIS_SERVER, "", pRedisListSetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
		
	string strIpAddrs;
	int index = mapAdditionParams[INDEX_TYPE_IDENTIFIER];
	int dwKeepTime = 0 ;
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, strValue, index, dwKeepTime, strIpAddrs);
		
	m_pRedisVirtualService->Log(pRedisListSetMsg->strGuid, dwServiceId, dwMsgId, pRedisListSetMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisListSetMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisListSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisListMsgOperator::DoDelete(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListDeleteMsg *pRedisListDeleteMsg = (SRedisListDeleteMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListDeleteMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListDeleteMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListDeleteMsg->dwSequenceId;
			   
	CSapDecoder decoder(pRedisListDeleteMsg->pBuffer, pRedisListDeleteMsg->dwLen);
			
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
			   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
			
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisListDeleteMsg->tStart, strKey, "", 0, nRet, "", pRedisListDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	//获取额外参数
	if(strValue.empty())
	{
		m_pRedisVirtualService->Log(pRedisListDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisListDeleteMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisListDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}

	CRedisContainer *pRedisContainer = pRedisListDeleteMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisListDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisListDeleteMsg->tStart, strKey, strValue, 0, RVS_NO_REDIS_SERVER, "", pRedisListDeleteMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
			
	string strIpAddrs;
	int count = 1;
	map<string, int>::const_iterator iter = mapAdditionParams.find(COUNT_TYPE_IDENTIFIER);
	if(iter != mapAdditionParams.end())
	{
		count = mapAdditionParams[COUNT_TYPE_IDENTIFIER];
	}
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Delete(strKey, strValue, count, strIpAddrs);
			
	m_pRedisVirtualService->Log(pRedisListDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisListDeleteMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListDeleteMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisListDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisListMsgOperator::DoSize(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListSizeMsg *pRedisListSizeMsg = (SRedisListSizeMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListSizeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListSizeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListSizeMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisListSizeMsg->pBuffer, pRedisListSizeMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisListSizeMsg->tStart, strKey, "", 0, nRet, "", pRedisListSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
    CRedisContainer *pRedisContainer = pRedisListSizeMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisListSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisListSizeMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisListSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int size = 0;
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Size(strKey, size, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisListSizeMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisListSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
				
    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_LIST_SIZE);
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisListSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisListSizeMsg->tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisListSizeMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisListSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

    char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", size);
    string strLogValue = szTemp;
	m_pRedisVirtualService->Log(pRedisListSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisListSizeMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisListSizeMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, size);
	m_pRedisVirtualService->Response(pRedisListSizeMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());

}

void CRedisListMsgOperator::DoGetAll(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListGetAllMsg *pRedisListGetAllMsg = (SRedisListGetAllMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListGetAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListGetAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListGetAllMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisListGetAllMsg->pBuffer, pRedisListGetAllMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
		
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetAllMsg->tStart, strKey, "", 0, nRet, "", pRedisListGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisListGetAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisListGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetAllMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisListGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisListGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_LIST_GETALL);
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
		m_pRedisVirtualService->Log(pRedisListGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetAllMsg->tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, "", pRedisListGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisListGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
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
	vector<string> vecValue;
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.GetAll(strKey, vecValue, start, end, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
	    m_pRedisVirtualService->Log(pRedisListGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetAllMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisListGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strLogValue;
	VectorToString(vecValue, strLogValue);

	m_pRedisVirtualService->Log(pRedisListGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisListGetAllMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisListGetAllMsg->fSpentTimeInQueue);
	ResponseStructArrayFromVec(pRedisListGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecValue, structCode, vecConfigField);

}

void CRedisListMsgOperator::DoDeleteAll(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
    SRedisListDeleteAllMsg *pRedisListDeleteAllMsg = (SRedisListDeleteAllMsg *)pRedisListOperateMsg;
	
	DeleteAll(pRedisListDeleteAllMsg->dwServiceId,
		pRedisListDeleteAllMsg->dwMsgId,
		pRedisListDeleteAllMsg->dwSequenceId,
		pRedisListDeleteAllMsg->strGuid,
		pRedisListDeleteAllMsg->handler,
		pRedisListDeleteAllMsg->pBuffer,
		pRedisListDeleteAllMsg->dwLen,
		pRedisListDeleteAllMsg->tStart,
		pRedisListDeleteAllMsg->fSpentTimeInQueue,
		pRedisListDeleteAllMsg->pRedisContainer);

}

void CRedisListMsgOperator::DoPopBack(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListPopBackMsg *pRedisListPopBackMsg = (SRedisListPopBackMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListPopBackMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListPopBackMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListPopBackMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisListPopBackMsg->pBuffer, pRedisListPopBackMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strKey;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
    if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisListPopBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopBackMsg->tStart, strKey, "", 0, nRet, "", pRedisListPopBackMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListPopBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
	}

	CRedisContainer *pRedisContainer = pRedisListPopBackMsg->pRedisContainer;
    if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisListPopBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopBackMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisListPopBackMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListPopBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
	
	string strIpAddrs, strValue;
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.PopBack(strKey, strValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisListPopBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopBackMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisListPopBackMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListPopBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
    }

	m_pRedisVirtualService->Log(pRedisListPopBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopBackMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListPopBackMsg->fSpentTimeInQueue);
	Response(pRedisListPopBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);

}

void CRedisListMsgOperator::DoPushBack(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListPushBackMsg *pRedisListPushBackMsg = (SRedisListPushBackMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListPushBackMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListPushBackMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListPushBackMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisListPushBackMsg->pBuffer, pRedisListPushBackMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListPushBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushBackMsg->tStart, strKey, "", 0, nRet, "", pRedisListPushBackMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListPushBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
    CRedisContainer *pRedisContainer = pRedisListPushBackMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisListPushBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushBackMsg->tStart, strKey, strValue, 0, RVS_NO_REDIS_SERVER, "", pRedisListPushBackMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListPushBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisListAgentScheduler agentScheduler(pRedisContainer, m_pRedisVirtualService);
	nRet = agentScheduler.PushBack(strKey, strValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListPushBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushBackMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListPushBackMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListPushBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
				
	m_pRedisVirtualService->Log(pRedisListPushBackMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushBackMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListPushBackMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisListPushBackMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisListMsgOperator::DoPopFront(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListPopFrontMsg *pRedisListPopFrontMsg = (SRedisListPopFrontMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListPopFrontMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListPopFrontMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListPopFrontMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisListPopFrontMsg->pBuffer, pRedisListPopFrontMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strKey;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
    if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisListPopFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopFrontMsg->tStart, strKey, "", 0, nRet, "", pRedisListPopFrontMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListPopFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
	}
	
	CRedisContainer *pRedisContainer = pRedisListPopFrontMsg->pRedisContainer;
    if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisListPopFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopFrontMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisListPopFrontMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListPopFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
	
	string strIpAddrs, strValue;
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.PopFront(strKey, strValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisListPopFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopFrontMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisListPopFrontMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisListPopFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
    }

	m_pRedisVirtualService->Log(pRedisListPopFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPopFrontMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListPopFrontMsg->fSpentTimeInQueue);
	Response(pRedisListPopFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);

}

void CRedisListMsgOperator::DoPushFront(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListPushFrontMsg *pRedisListPushFrontMsg = (SRedisListPushFrontMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListPushFrontMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListPushFrontMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListPushFrontMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisListPushFrontMsg->pBuffer, pRedisListPushFrontMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListPushFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushFrontMsg->tStart, strKey, "", 0, nRet, "", pRedisListPushFrontMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListPushFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
    CRedisContainer *pRedisContainer = pRedisListPushFrontMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisListPushFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushFrontMsg->tStart, strKey, strValue, 0, RVS_NO_REDIS_SERVER, "", pRedisListPushFrontMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListPushFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisListAgentScheduler agentScheduler(pRedisContainer, m_pRedisVirtualService);
	nRet = agentScheduler.PushFront(strKey, strValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListPushFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushFrontMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListPushFrontMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListPushFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
				
	m_pRedisVirtualService->Log(pRedisListPushFrontMsg->strGuid, dwServiceId, dwMsgId, pRedisListPushFrontMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListPushFrontMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisListPushFrontMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisListMsgOperator::DoReserve(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListReserveMsg *pRedisListReserveMsg = (SRedisListReserveMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListReserveMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListReserveMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListReserveMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisListReserveMsg->pBuffer, pRedisListReserveMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey, strValue;
	map<string, int> mapAdditionParams;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, mapAdditionParams);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListReserveMsg->strGuid, dwServiceId, dwMsgId, pRedisListReserveMsg->tStart, strKey, "", 0, nRet, "", pRedisListReserveMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListReserveMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
    //获取额外参数
    bool bParametersRight = true;
	map<string, int>::const_iterator iter = mapAdditionParams.find(START_TYPE_IDENTIFIER);
	if(iter == mapAdditionParams.end())
	{
		bParametersRight = false;
	}
	iter = mapAdditionParams.find(END_TYPE_IDENTIFIER);
	if(iter == mapAdditionParams.end())
	{
		bParametersRight = false;
	}
	if(!bParametersRight)
	{
		m_pRedisVirtualService->Log(pRedisListReserveMsg->strGuid, dwServiceId, dwMsgId, pRedisListReserveMsg->tStart, strKey, "", 0, RVS_PARAMETER_ERROR, "", pRedisListReserveMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListReserveMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
		
    CRedisContainer *pRedisContainer = pRedisListReserveMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisListReserveMsg->strGuid, dwServiceId, dwMsgId, pRedisListReserveMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisListReserveMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListReserveMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int start = mapAdditionParams[START_TYPE_IDENTIFIER];
	int end = mapAdditionParams[END_TYPE_IDENTIFIER];
	char szTemp[35] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d-%d", start, end);
	strValue = szTemp;
	
	CRedisListAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Reserve(strKey, start, end,strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisListReserveMsg->strGuid, dwServiceId, dwMsgId, pRedisListReserveMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListReserveMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisListReserveMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
				
   m_pRedisVirtualService->Log(pRedisListReserveMsg->strGuid, dwServiceId, dwMsgId, pRedisListReserveMsg->tStart, strKey, strValue, 0, nRet, strIpAddrs, pRedisListReserveMsg->fSpentTimeInQueue);
   m_pRedisVirtualService->Response(pRedisListReserveMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisListMsgOperator::DoGetKey(SRedisListOperateMsg *pRedisListOperateMsg)
{
	SRedisListGetKeyMsg *pRedisListGetKeyMsg = (SRedisListGetKeyMsg *)pRedisListOperateMsg;
	
	GetKey(pRedisListGetKeyMsg->dwServiceId,
			pRedisListGetKeyMsg->dwMsgId,
			pRedisListGetKeyMsg->dwSequenceId,
			pRedisListGetKeyMsg->strGuid,
			pRedisListGetKeyMsg->handler,
			pRedisListGetKeyMsg->pBuffer,
			pRedisListGetKeyMsg->dwLen,
			pRedisListGetKeyMsg->tStart,
			pRedisListGetKeyMsg->fSpentTimeInQueue,
			RVS_LIST_GETKEY);

}

void CRedisListMsgOperator::DoTtl(SRedisListOperateMsg *pRedisListOperateMsg)
{
	SRedisListTtlMsg *pRedisListTtlMsg = (SRedisListTtlMsg *)pRedisListOperateMsg;

    Ttl(pRedisListTtlMsg->dwServiceId,
		pRedisListTtlMsg->dwMsgId,
		pRedisListTtlMsg->dwSequenceId,
		pRedisListTtlMsg->strGuid,
		pRedisListTtlMsg->handler,
		pRedisListTtlMsg->pBuffer,
		pRedisListTtlMsg->dwLen,
		pRedisListTtlMsg->tStart,
		pRedisListTtlMsg->fSpentTimeInQueue,
		pRedisListTtlMsg->pRedisContainer,
		RVS_LIST_TTL);

}

void CRedisListMsgOperator::DoDecode(SRedisListOperateMsg *pRedisListOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	SRedisListDecodeMsg *pRedisListDecodeMsg = (SRedisListDecodeMsg *)pRedisListOperateMsg;
	unsigned int dwServiceId = pRedisListDecodeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisListDecodeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisListDecodeMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisListDecodeMsg->pBuffer, pRedisListDecodeMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strEncodedField, strEncodedValue;
	GetRequestEnocdedInfo(dwServiceId, pBody, dwBodyLen, strEncodedField, strEncodedValue);
	
	m_pRedisVirtualService->Log(pRedisListDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisListDecodeMsg->tStart, "", strEncodedValue, 0, RVS_SUCESS, "", pRedisListDecodeMsg->fSpentTimeInQueue);
	Response(pRedisListDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_SUCESS, strEncodedValue);
}

int CRedisListMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, map<string, int> &mapAdditionParams)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisListMsgOperator::%s\n", __FUNCTION__);
	multimap<unsigned int, string> mapKey;
	map<unsigned int, string> mapValue;
	
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
		    mapValue.insert(make_pair(nType, strValue));
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
		RVS_XLOG(XLOG_ERROR, "CRedisListMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}
	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}

    strValue = "";
	map<unsigned int, string>::iterator iter;
	for(iter = mapValue.begin(); iter != mapValue.end(); iter++)
    {
    	char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d#", iter->first);
		string temp = szTemp;
		temp += iter->second.c_str();
    	if(strValue.empty())
    	{
    		strValue = temp;
    	}
		else
		{
		    strValue += VALUE_SPLITTER + temp; 
		}
    }
	
	return RVS_SUCESS;
}

