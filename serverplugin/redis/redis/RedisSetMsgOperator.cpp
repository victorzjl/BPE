#include "RedisSetMsgOperator.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "ServiceConfig.h"
#include "RedisVirtualService.h"
#include "RedisContainer.h"
#include "CommonMacro.h"
#include "RedisSetAgentScheduler.h"

#include <SapMessage.h>
#include <map>

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace sdo::sap;
using std::multimap;
using std::map;
using namespace redis;

CRedisSetMsgOperator::CRedisSetMsgOperator(CRedisVirtualService *pRedisVirtualService):IRedisTypeMsgOperator(pRedisVirtualService)
{
	m_redisSetFunc[RVS_SET_HASMEMBER] = &CRedisSetMsgOperator::DoHasMember;
	m_redisSetFunc[RVS_SET_SET] = &CRedisSetMsgOperator::DoSet;
	m_redisSetFunc[RVS_SET_DELETE] = &CRedisSetMsgOperator::DoDelete;
	m_redisSetFunc[RVS_SET_SIZE] = &CRedisSetMsgOperator::DoSize;
	m_redisSetFunc[RVS_SET_GETALL] = &CRedisSetMsgOperator::DoGetAll;
	m_redisSetFunc[RVS_SET_DELETEALL] = &CRedisSetMsgOperator::DoDeleteAll;
	m_redisSetFunc[RVS_SET_INTER] = &CRedisSetMsgOperator::DoInter;
	m_redisSetFunc[RVS_SET_UNION] = &CRedisSetMsgOperator::DoUnion;
	m_redisSetFunc[RVS_SET_GETKEY] = &CRedisSetMsgOperator::DoGetKey;
	m_redisSetFunc[RVS_SET_TTL] = &CRedisSetMsgOperator::DoTtl;
	m_redisSetFunc[RVS_SET_DECODE] = &CRedisSetMsgOperator::DoDecode;
}

void CRedisSetMsgOperator::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	   
	SRedisSetOperateMsg * redisSetOperateMsg = NULL;
	switch(dwMsgId)
	{
		case RVS_SET_HASMEMBER:
			redisSetOperateMsg = new SRedisSetHasMemberMsg;
			break;
		case RVS_SET_SET:
			redisSetOperateMsg = new SRedisSetSetMsg;
			break;
		case RVS_SET_DELETE:
			redisSetOperateMsg = new SRedisSetDeleteMsg;
			break;
		case RVS_SET_SIZE:
			redisSetOperateMsg = new SRedisSetSizeMsg;
			break;
		case RVS_SET_GETALL:
			redisSetOperateMsg = new SRedisSetGetAllMsg;
			break;
		case RVS_SET_DELETEALL:
			redisSetOperateMsg = new SRedisSetDeleteAllMsg;
			break;
		case RVS_SET_GETKEY:
			redisSetOperateMsg = new SRedisSetGetKeyMsg;
			break;
		case RVS_SET_TTL:
			redisSetOperateMsg = new SRedisSetTtlMsg;
			break;
		case RVS_SET_DECODE:
			redisSetOperateMsg = new SRedisSetDecodeMsg;
			break;
		default:
			RVS_XLOG(XLOG_ERROR, "CRedisSetMsgOperator::%s, MsgId[%d] not supported\n", __FUNCTION__, dwMsgId);
			m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_MSG, "" , fSpentInQueueTime);
			m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_UNKNOWN_MSG);
			return;
	 }
	
	 redisSetOperateMsg->dwServiceId = dwServiceId;
	 redisSetOperateMsg->dwMsgId = dwMsgId;
	 redisSetOperateMsg->dwSequenceId = dwSequenceId;
	 redisSetOperateMsg->pBuffer = pBuffer;
	 redisSetOperateMsg->dwLen = nLen;
	 redisSetOperateMsg->handler = pHandler;
	 redisSetOperateMsg->strGuid = strGuid;
	 redisSetOperateMsg->tStart = tStart;
	 redisSetOperateMsg->fSpentTimeInQueue = fSpentInQueueTime;
	 redisSetOperateMsg->pRedisContainer = pRedisContainer;
	
	 (this->*(m_redisSetFunc[dwMsgId]))(redisSetOperateMsg);
	
	 delete redisSetOperateMsg;
}

int CRedisSetMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, unsigned int &dwKeepTime)
{
	multimap<unsigned int, string> mapKey;
	map<unsigned int, string> mapValue;
	
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
			mapKey.insert(make_pair(nType, strValue));
		}
		else if(nType == 10000)
		{
			dwKeepTime = atoi(strValue.c_str());
		}
		else if(nType > 10000 && nType < 20000)
		{
			mapValue.insert(make_pair(nType, strValue));
		}
	
		ptrLoc += nFactLen;
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
	multimap<unsigned int, string>::const_iterator iterRequest;
	for (iterRequest = mapKey.begin(); iterRequest != mapKey.end(); iterRequest++)
	{
		strKey += "_" ;
		strKey += iterRequest->second.c_str();
	}
		
	if(strKey.length() > 1024)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}
	
	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}
	
	//获取value值
	strValue = "";
	map<unsigned int, string>::const_iterator iterValue;
	for (iterValue = mapValue.begin(); iterValue != mapValue.end(); iterValue++)
	{
	    char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d#", iterValue->first);
		string temp = szTemp;
		temp += iterValue->second.c_str();
		if(strValue.empty())
		{
			strValue = temp;
		}
		else
		{
			strValue += VALUE_SPLITTER + temp;
		}
	}
	return 0;
}

void CRedisSetMsgOperator::DoHasMember(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetHasMemberMsg *pRedisSetHasMemberMsg = (SRedisSetHasMemberMsg *)pRedisSetOperateMsg;
	unsigned int dwServiceId = pRedisSetHasMemberMsg->dwServiceId;
	unsigned int dwMsgId = pRedisSetHasMemberMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisSetHasMemberMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisSetHasMemberMsg->pBuffer, pRedisSetHasMemberMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetHasMemberMsg->strGuid, dwServiceId, dwMsgId, pRedisSetHasMemberMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisSetHasMemberMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetHasMemberMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(strValue.empty())
	{
		m_pRedisVirtualService->Log(pRedisSetHasMemberMsg->strGuid, dwServiceId, dwMsgId, pRedisSetHasMemberMsg->tStart, strKey, "", dwKeepTime, RVS_PARAMETER_ERROR, "", pRedisSetHasMemberMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetHasMemberMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisSetHasMemberMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisSetHasMemberMsg->strGuid, dwServiceId, dwMsgId, pRedisSetHasMemberMsg->tStart, strKey, strValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisSetHasMemberMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetHasMemberMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisSetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.HasMember(strKey, strValue, strIpAddrs);
				
	m_pRedisVirtualService->Log(pRedisSetHasMemberMsg->strGuid, dwServiceId, dwMsgId, pRedisSetHasMemberMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisSetHasMemberMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisSetHasMemberMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisSetMsgOperator::DoSet(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetSetMsg *pRedisSetSetMsg = (SRedisSetSetMsg *)pRedisSetOperateMsg;
	unsigned int dwServiceId = pRedisSetSetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisSetSetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisSetSetMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisSetSetMsg->pBuffer, pRedisSetSetMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisSetSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisSetSetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisSetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSetMsg->tStart, strKey, strValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisSetSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int result = -1;
	CRedisSetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, strValue, result, dwKeepTime, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSetMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisSetSetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisSetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
	}
	
    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_SET_SET);
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", result);
    string strLogValue = strValue;
	strLogValue += "   response[";
	strLogValue += szTemp;
	strLogValue += "]";
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisSetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisSetSetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisSetSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	m_pRedisVirtualService->Log(pRedisSetSetMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisSetSetMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, result);
	m_pRedisVirtualService->Response(pRedisSetSetMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());

}

void CRedisSetMsgOperator::DoDelete(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetDeleteMsg *pRedisSetDeleteMsg = (SRedisSetDeleteMsg *)pRedisSetOperateMsg;
	unsigned int dwServiceId = pRedisSetDeleteMsg->dwServiceId;
	unsigned int dwMsgId = pRedisSetDeleteMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisSetDeleteMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisSetDeleteMsg->pBuffer, pRedisSetDeleteMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisSetDeleteMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisSetDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisSetDeleteMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisSetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisSetDeleteMsg->tStart, strKey, strValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisSetDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	CRedisSetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Delete(strKey, strValue, strIpAddrs);
				
	m_pRedisVirtualService->Log(pRedisSetDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisSetDeleteMsg->tStart, strKey, strValue, dwKeepTime, nRet, strIpAddrs, pRedisSetDeleteMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisSetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisSetMsgOperator::DoSize(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetSizeMsg *pRedisSetSizeMsg = (SRedisSetSizeMsg *)pRedisSetOperateMsg;
	unsigned int dwServiceId = pRedisSetSizeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisSetSizeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisSetSizeMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisSetSizeMsg->pBuffer, pRedisSetSizeMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSizeMsg->tStart, strKey, "", 0, nRet, "", pRedisSetSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
	CRedisContainer *pRedisContainer = pRedisSetSizeMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisSetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSizeMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisSetSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int size = 0;
	CRedisSetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Size(strKey, size, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSizeMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisSetSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_SET_SIZE);
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", size);
    string strLogValue = szTemp;
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisSetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSizeMsg->tStart, strKey, strLogValue, 0, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisSetSizeMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisSetSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	m_pRedisVirtualService->Log(pRedisSetSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisSetSizeMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisSetSizeMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, size);
	m_pRedisVirtualService->Response(pRedisSetSizeMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisSetMsgOperator::DoGetAll(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetGetAllMsg *pRedisSetGetAllMsg = (SRedisSetGetAllMsg *)pRedisSetOperateMsg;
	unsigned int dwServiceId = pRedisSetGetAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisSetGetAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisSetGetAllMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisSetGetAllMsg->pBuffer, pRedisSetGetAllMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	int nRet = RVS_SUCESS;
		
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisSetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisSetGetAllMsg->tStart, strKey, "", 0, nRet, "", pRedisSetGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisSetGetAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisSetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisSetGetAllMsg->tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", pRedisSetGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisSetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_SET_GETALL);
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
		m_pRedisVirtualService->Log(pRedisSetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisSetGetAllMsg->tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, "", pRedisSetGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisSetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}
		
	string strIpAddrs;
	vector<string> vecValue;
	CRedisSetAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.GetAll(strKey, vecValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
	    m_pRedisVirtualService->Log(pRedisSetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisSetGetAllMsg->tStart, strKey, "", 0, nRet, strIpAddrs, pRedisSetGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisSetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strLogValue;
	VectorToString(vecValue, strLogValue);

	m_pRedisVirtualService->Log(pRedisSetGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisSetGetAllMsg->tStart, strKey, strLogValue, 0, nRet, strIpAddrs, pRedisSetGetAllMsg->fSpentTimeInQueue);
	ResponseStructArrayFromVec(pRedisSetGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecValue, structCode, vecConfigField);

}

void CRedisSetMsgOperator::DoDeleteAll(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetDeleteAllMsg *pRedisSetDeleteAllMsg = (SRedisSetDeleteAllMsg *)pRedisSetOperateMsg;

	DeleteAll(pRedisSetDeleteAllMsg->dwServiceId,
		pRedisSetDeleteAllMsg->dwMsgId,
		pRedisSetDeleteAllMsg->dwSequenceId,
		pRedisSetDeleteAllMsg->strGuid,
		pRedisSetDeleteAllMsg->handler,
		pRedisSetDeleteAllMsg->pBuffer,
		pRedisSetDeleteAllMsg->dwLen,
		pRedisSetDeleteAllMsg->tStart,
		pRedisSetDeleteAllMsg->fSpentTimeInQueue,
		pRedisSetDeleteAllMsg->pRedisContainer);
}

void CRedisSetMsgOperator::DoInter(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
}

void CRedisSetMsgOperator::DoUnion(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
}

void CRedisSetMsgOperator::DoGetKey(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	SRedisSetGetKeyMsg *pRedisSetGetKeyMsg = (SRedisSetGetKeyMsg *)pRedisSetOperateMsg;
	
	GetKey(pRedisSetGetKeyMsg->dwServiceId,
			pRedisSetGetKeyMsg->dwMsgId,
			pRedisSetGetKeyMsg->dwSequenceId,
			pRedisSetGetKeyMsg->strGuid,
			pRedisSetGetKeyMsg->handler,
			pRedisSetGetKeyMsg->pBuffer,
			pRedisSetGetKeyMsg->dwLen,
			pRedisSetGetKeyMsg->tStart,
			pRedisSetGetKeyMsg->fSpentTimeInQueue,
			RVS_SET_GETKEY);
}

void CRedisSetMsgOperator::DoTtl(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	SRedisSetTtlMsg *pRedisSetTtlMsg = (SRedisSetTtlMsg *)pRedisSetOperateMsg;

    Ttl(pRedisSetTtlMsg->dwServiceId,
		pRedisSetTtlMsg->dwMsgId,
		pRedisSetTtlMsg->dwSequenceId,
		pRedisSetTtlMsg->strGuid,
		pRedisSetTtlMsg->handler,
		pRedisSetTtlMsg->pBuffer,
		pRedisSetTtlMsg->dwLen,
		pRedisSetTtlMsg->tStart,
		pRedisSetTtlMsg->fSpentTimeInQueue,
		pRedisSetTtlMsg->pRedisContainer,
		RVS_SET_TTL);
}

void CRedisSetMsgOperator::DoDecode(SRedisSetOperateMsg *pRedisSetOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetMsgOperator::%s\n", __FUNCTION__);
	SRedisSetDecodeMsg *pRedisSetDecodeMsg = (SRedisSetDecodeMsg *)pRedisSetOperateMsg;
	unsigned int dwServiceId = pRedisSetDecodeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisSetDecodeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisSetDecodeMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisSetDecodeMsg->pBuffer, pRedisSetDecodeMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strEncodedField, strEncodedValue;
	GetRequestEnocdedInfo(dwServiceId, pBody, dwBodyLen, strEncodedField, strEncodedValue);
	
	m_pRedisVirtualService->Log(pRedisSetDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisSetDecodeMsg->tStart, "", strEncodedValue, 0, RVS_SUCESS, "", pRedisSetDecodeMsg->fSpentTimeInQueue);
	Response(pRedisSetDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_SUCESS, strEncodedValue);
}
