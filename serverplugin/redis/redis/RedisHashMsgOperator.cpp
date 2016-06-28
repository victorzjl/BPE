#include "RedisHashMsgOperator.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisContainer.h"
#include "CommonMacro.h"
#include "RedisHashAgentScheduler.h"
#include "RedisHelper.h"
#include "AvenueServiceConfigs.h"

#include <SapMessage.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace sdo::sap;
using namespace redis;

CRedisHashMsgOperator::CRedisHashMsgOperator(CRedisVirtualService *pRedisVirtualService):IRedisTypeMsgOperator(pRedisVirtualService)
{
    m_redisHashFunc[RVS_HASH_GET] = &CRedisHashMsgOperator::DoGet;
	m_redisHashFunc[RVS_HASH_SET] = &CRedisHashMsgOperator::DoSet;
	m_redisHashFunc[RVS_HASH_DELETE] = &CRedisHashMsgOperator::DoDelete;
	m_redisHashFunc[RVS_HASH_SIZE] = &CRedisHashMsgOperator::DoSize;
	m_redisHashFunc[RVS_HASH_SETALL] = &CRedisHashMsgOperator::DoSetAll;
	m_redisHashFunc[RVS_HASH_GETALL] = &CRedisHashMsgOperator::DoGetAll;
	m_redisHashFunc[RVS_HASH_DELETEALL] = &CRedisHashMsgOperator::DoDeleteAll;
	m_redisHashFunc[RVS_HASH_INCBY] = &CRedisHashMsgOperator::DoIncby;
	m_redisHashFunc[RVS_HASH_GETKEY] = &CRedisHashMsgOperator::DoGetKey;
	m_redisHashFunc[RVS_HASH_TTL] = &CRedisHashMsgOperator::DoTtl;
	m_redisHashFunc[RVS_HASH_DECODE] = &CRedisHashMsgOperator::DoDecode;
	m_redisHashFunc[RVS_HASH_BATCH_QUERY] = &CRedisHashMsgOperator::DoBatchQuery;
	m_redisHashFunc[RVS_HASH_BATCH_INCRBY] = &CRedisHashMsgOperator::DoBatchIncrby;
	m_redisHashFunc[RVS_HASH_BATCH_EXPIRE] = &CRedisHashMsgOperator::DoBatchExpire;
}

void CRedisHashMsgOperator::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	   
	SRedisHashOperateMsg * redisHashOperateMsg = NULL;
	switch(dwMsgId)
	{
		case RVS_HASH_GET:
			redisHashOperateMsg = new SRedisHashGetMsg;
			break;
		case RVS_HASH_SET:
			redisHashOperateMsg = new SRedisHashSetMsg;
			break;
		case RVS_HASH_DELETE:
			redisHashOperateMsg = new SRedisHashDeleteMsg;
			break;
		case RVS_HASH_SIZE:
			redisHashOperateMsg = new SRedisHashSizeMsg;
			break;
		case RVS_HASH_SETALL:
			redisHashOperateMsg = new SRedisHashSetAllMsg;
			break;
		case RVS_HASH_GETALL:
			redisHashOperateMsg = new SRedisHashGetAllMsg;
			break;
		case RVS_HASH_DELETEALL:
			redisHashOperateMsg = new SRedisHashDeleteAllMsg;
			break;
		case RVS_HASH_INCBY:
			redisHashOperateMsg = new SRedisHashIncbyMsg;
			break;
		case RVS_HASH_GETKEY:
			redisHashOperateMsg = new SRedisHashGetKeyMsg;
			break;
		case RVS_HASH_TTL:
			redisHashOperateMsg = new SRedisHashTtlMsg;
			break;
		case RVS_HASH_DECODE:
			redisHashOperateMsg = new SRedisHashDecodeMsg;
			break;
		case RVS_HASH_BATCH_QUERY:
			redisHashOperateMsg = new SRedisHashBatchQueryMsg;
			break;
		case RVS_HASH_BATCH_INCRBY:
			redisHashOperateMsg = new SRedisHashBatchIncrbyMsg;
			break;
		case RVS_HASH_BATCH_EXPIRE:
			redisHashOperateMsg = new SRedisHashBatchExpireMsg;
			break;
		default:
			RVS_XLOG(XLOG_ERROR, "CRedisHashMsgOperator::%s, MsgId[%d] not supported\n", __FUNCTION__, dwMsgId);
			m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_MSG, "" , fSpentInQueueTime);
			m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_UNKNOWN_MSG);
			return;
	}
	
	redisHashOperateMsg->dwServiceId = dwServiceId;
	redisHashOperateMsg->dwMsgId = dwMsgId;
	redisHashOperateMsg->dwSequenceId = dwSequenceId;
	redisHashOperateMsg->pBuffer = pBuffer;
	redisHashOperateMsg->dwLen = nLen;
	redisHashOperateMsg->handler = pHandler;
	redisHashOperateMsg->strGuid = strGuid;
	redisHashOperateMsg->tStart = tStart;
	redisHashOperateMsg->fSpentTimeInQueue = fSpentInQueueTime;
	redisHashOperateMsg->pRedisContainer = pRedisContainer;
	
	(this->*(m_redisHashFunc[dwMsgId]))(redisHashOperateMsg);
	
	delete redisHashOperateMsg;
}

string CRedisHashMsgOperator::VecKFVToString(vector<SKeyFieldValue> &vecKFV)
{
	string strValue;
	vector<SKeyFieldValue>::const_iterator iter;
	for (iter = vecKFV.begin(); iter != vecKFV.end(); iter++)
	{
		const SKeyFieldValue &kfv = *iter;

        if(!strValue.empty())
        {
            strValue += VALUE_SPLITTER;
        }
		strValue += kfv.strKey;
		strValue += " ";
		strValue += kfv.strField;
		strValue += " ";
		strValue += kfv.strValue;
	}

	return strValue;
}

int CRedisHashMsgOperator::GetKFVFromStruct(unsigned int dwServiceId, unsigned int nCode, const void *pBuffer, unsigned int dwLen,string &strKey, string &strField, string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s, serviceId[%d], Code[%d], dwLen[%d]\n", __FUNCTION__, dwServiceId, nCode, dwLen);
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByCode(nCode, oConfigType);
	vector<SConfigField> &vecField = oConfigType.vecConfigField;
    unsigned int resolveFieldSize = vecField.size();

	if(strValue == "0")
	{
	    unsigned int i = 0;
		for(;i < vecField.size(); i++)
		{
			SConfigField &field = vecField[i];
			if(field.nCode > 20000)
			{
				break;
			}
		}
		resolveFieldSize = i;
	}
    //key前缀
    char szPrefix[16] = {0};
    snprintf(szPrefix, sizeof(szPrefix), "RVS%d", dwServiceId);
    strKey = szPrefix;
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s, Size[%d]\n", __FUNCTION__, resolveFieldSize);
	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	for(unsigned int i = 0; i < resolveFieldSize; i++)
	{
		const SConfigField &oConfigField = vecField[i];
		RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s, fieldName[%s] fieldCode[%d] fieldType[%d]\n", __FUNCTION__, oConfigField.strName.c_str(), oConfigField.nCode, oConfigField.eStructFieldType);
		string tempStr;
		if(oConfigField.eStructFieldType == MSG_FIELD_INT)
		{
		    if(ptrLoc + 4 > (unsigned char *)pBuffer + dwLen)
		    {
		       RVS_XLOG(XLOG_ERROR, "CRedisHashMsgOperator::%s, Type[ int ] over the range\n", __FUNCTION__);
			   return -1;
		    }
			char szTemp[16] = {0};
		    snprintf(szTemp, sizeof(szTemp), "%d", ntohl(*(int *)ptrLoc));
			tempStr = szTemp;
			ptrLoc += 4;
		}
		else
		{
		    if(i != vecField.size() - 1)
		    {
				if(ptrLoc + oConfigField.nLen > (unsigned char *)pBuffer + dwLen)
		        {
		            RVS_XLOG(XLOG_ERROR, "CRedisHashMsgOperator::%s, Type[ string ] over the range\n", __FUNCTION__);
			        return -1;
		        }

		    	tempStr = string((char *)ptrLoc, oConfigField.nLen);
			    ptrLoc += oConfigField.nLen;
		    }
			else
			{
			    unsigned int length = ptrLoc - (unsigned char *)pBuffer;
				tempStr = string((char *)ptrLoc, dwLen - length);
			}
			tempStr = tempStr.c_str();
		}

		RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s, tempStr[%s %d]\n", __FUNCTION__, tempStr.c_str(), tempStr.length());
		if(tempStr.empty())
		{
			continue;
		}
		
        if(oConfigField.nCode < 10000)
        {
            strKey += "_";
			strKey += tempStr;
        }
		else
		{
			char szTemp[16] = {0};
			snprintf(szTemp, sizeof(szTemp), "%d#", oConfigField.nCode);
			tempStr = szTemp + tempStr;
			if(strncasecmp(oConfigField.strName.c_str(), "field_", 6) == 0 || (oConfigField.nCode > 10000 && oConfigField.nCode < 20000))
			{		
				if(strField.empty())
				{
					strField = tempStr;
				}
				else
				{
					strField += VALUE_SPLITTER + tempStr;
				}
			}
			else
			{
						
				if(strValue.empty())
				{
					strValue =	tempStr;
				}
				else
				{
					strValue += VALUE_SPLITTER + tempStr;
				}
			}
		}
	}
	
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s, return strKey[%s] strField[%s], strValue[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str(), strValue.c_str());
    return 0;
}

int CRedisHashMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, map<string, string> &mapFieldValue, unsigned int &dwKeepTime, int &increment)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	multimap<unsigned int, string> mapKey;
	map<unsigned int, string> mapField;
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
		
		if(mapTypeByCode[nType] == MSG_FIELD_STRUCT)
		{
		    string strKey;
			string strField;
			string strValue;
			int nRet = GetKFVFromStruct(dwServiceId, nType, ptrLoc+sizeof(SSapMsgAttribute), nLen-sizeof(SSapMsgAttribute), strKey, strField, strValue);
			if(nRet != 0)
			{
				return RVS_REDIS_STRUCT_IS_WRONG;
			}
			if(!strField.empty())
			{
				mapFieldValue[strField] = strValue;
			}
		}
		else
		{
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
			    dwKeepTime = atoi(strValue.c_str());
		    }
		    else if(nType > 10000 && nType < 20000)
		    {
		        mapField.insert(make_pair(nType, strValue));
		    }
		    else if(nType > 20000 && nType < 30000)
		    {
			    mapValue.insert(make_pair(nType, strValue));
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
					increment = atoi(strValue.c_str());
				}
			}
		}
		
		ptrLoc += nFactLen;
	}
		
	/*
	RVS_XLOG(XLOG_DEBUG, "############begin message info############\n");
	RVS_XLOG(XLOG_DEBUG, "KeepTime:\t%d\n", dwKeepTime);
	RVS_XLOG(XLOG_DEBUG, "mapKey:\t%d\n", mapKey.size());
	multimap<unsigned int, string>::const_iterator iterLogKey;
	map<unsigned int, string>::iterator iterLog;
	for (iterLogKey = mapKey.begin(); iterLogKey != mapKey.end(); iterLogKey++)
	{
		RVS_XLOG(XLOG_DEBUG, "\t%d:\t%s\n", iterLogKey->first, iterLogKey->second.c_str());
	}
	
	RVS_XLOG(XLOG_DEBUG, "mapField:\t%d\n", mapField.size());
	for (iterLog = mapField.begin(); iterLog != mapField.end(); iterLog++)
	{
		RVS_XLOG(XLOG_DEBUG, "\t%d:\t%s\n", iterLog->first, iterLog->second.c_str());
	}

	RVS_XLOG(XLOG_DEBUG, "mapValue:\t%d\n", mapValue.size());
	for (iterLog = mapValue.begin(); iterLog != mapValue.end(); iterLog++)
	{
		RVS_XLOG(XLOG_DEBUG, "\t%d:\t%s\n", iterLog->first, iterLog->second.c_str());
	}
	
	RVS_XLOG(XLOG_DEBUG, "############end message info############\n");
       */
       
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
		RVS_XLOG(XLOG_ERROR, "CRedisHashMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}
	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}

    string strField, strValue;
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
	
	if(!strField.empty())
	{
		mapFieldValue[strField] = strValue;
	}
	
	return RVS_SUCESS;
}

int CRedisHashMsgOperator::GetRequestInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, vector<SKeyFieldValue> &vecKFV, bool onlyResolveKF)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s, serviceId[%d], pBuffer[%x], dwLen[%d]\n", __FUNCTION__, dwServiceId, pBuffer, dwLen);
	SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
    CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];

	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	while(ptrLoc < (unsigned char *)pBuffer + dwLen)
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen = ntohs(pAtti->wLength);
		unsigned short nType = ntohs(pAtti->wType);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
			return RVS_AVENUE_PACKET_ERROR;
		}

		if(mapTypeByCode[nType] == MSG_FIELD_STRUCT)
		{
			string strKey;
			string strField;
			string strValue;
			if(onlyResolveKF)
			{
				strValue = "0";
			}
			int nRet = GetKFVFromStruct(dwServiceId, nType, ptrLoc+sizeof(SSapMsgAttribute), nLen-sizeof(SSapMsgAttribute), strKey, strField, strValue);
			if(nRet != 0)
			{
				return RVS_REDIS_STRUCT_IS_WRONG;
			}

			if(strKey.length() > 1024)
	        {
		        RVS_XLOG(XLOG_ERROR, "CRedisHashMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		        return RVS_KEY_TOO_LONG;
	        }

	        if(!IsRedisKeyValid(strKey))
	        {
		       strKey = MakeRedisKeyValid(strKey);
	        }

			SKeyFieldValue kfv;
			kfv.strKey = strKey;
			kfv.strField = strField;
			kfv.strValue = strValue;
			vecKFV.push_back(kfv);
		}

		ptrLoc+=nFactLen;
	}
	
    if(vecKFV.size() == 0)
    {
    	return RVS_KEY_IS_NULL;
    }
	return 0;
}

void CRedisHashMsgOperator::DoGet(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashGetMsg *pRedisHashGetMsg = (SRedisHashGetMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashGetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashGetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashGetMsg->dwSequenceId;
	   
	CSapDecoder decoder(pRedisHashGetMsg->pBuffer, pRedisHashGetMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int increment = 0;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
    if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisHashGetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashGetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisHashGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
	}

    if(mapFieldValue.size() == 0)
    {
    	m_pRedisVirtualService->Log(pRedisHashGetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_FIELD_IS_NULL, "", pRedisHashGetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_FIELD_IS_NULL);
		return;
    }
	
	CRedisContainer *pRedisContainer = pRedisHashGetMsg->pRedisContainer;
    if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisHashGetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashGetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisHashGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
	
	string strIpAddrs;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Get(strKey, mapFieldValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		 m_pRedisVirtualService->Log(pRedisHashGetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs, pRedisHashGetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisHashGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		 return;
    }

	map<string, string>::const_iterator iter = mapFieldValue.begin();
	string strValue = iter->second;  
    unsigned int pos = strValue.find(CODE_VALUE_SPLITTER);
    if(pos == string::npos)
	{
	     //获得响应里的Code码
        int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_HASH_GET);
	    if(dwCode == -1)
	    {
		    m_pRedisVirtualService->Log(pRedisHashGetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisHashGetMsg->fSpentTimeInQueue);
	        m_pRedisVirtualService->Response(pRedisHashGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		    return;
	    }
		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp), "%d#", dwCode);
		strValue = szTemp;
		strValue += iter->second;
    }
	string strLogValue;
	MapToString(mapFieldValue, strLogValue);
	m_pRedisVirtualService->Log(pRedisHashGetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisHashGetMsg->fSpentTimeInQueue);
	Response(pRedisHashGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);
}

void CRedisHashMsgOperator::DoSet(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashSetMsg *pRedisHashSetMsg = (SRedisHashSetMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashSetMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashSetMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashSetMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisHashSetMsg->pBuffer, pRedisHashSetMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int increment = 0;
	int nRet = RVS_SUCESS;
		
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashSetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashSetMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisHashSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(mapFieldValue.size() == 0)
    {
    	m_pRedisVirtualService->Log(pRedisHashSetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_FIELD_IS_NULL, "", pRedisHashSetMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_FIELD_IS_NULL);
		return;
    }
	else
	{
	    //参数校验
		map<string, string>::iterator iter;
		for(iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
		{
			string &strValue = iter->second;
			if(strValue.empty())
			{
				strValue = VALUE_SPLITTER;
			}
		}
	}
	
	string strLogValue;
	MapToString(mapFieldValue, strLogValue);
	CRedisContainer *pRedisContainer = pRedisHashSetMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisHashSetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetMsg->tStart, strKey, strLogValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashSetMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisHashSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
		
	string strIpAddrs;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, mapFieldValue, dwKeepTime, strIpAddrs);
		
	m_pRedisVirtualService->Log(pRedisHashSetMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisHashSetMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisHashSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisHashMsgOperator::DoDelete(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashDeleteMsg *pRedisHashDeleteMsg = (SRedisHashDeleteMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashDeleteMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashDeleteMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashDeleteMsg->dwSequenceId;
			   
	CSapDecoder decoder(pRedisHashDeleteMsg->pBuffer, pRedisHashDeleteMsg->dwLen);
			
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
			   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int increment = 0 ;
	int nRet = RVS_SUCESS;
			
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDeleteMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	if(mapFieldValue.size() == 0)
	{
		m_pRedisVirtualService->Log(pRedisHashDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDeleteMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_FIELD_IS_NULL, "", pRedisHashDeleteMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_FIELD_IS_NULL);
		return;
	}
	
	string strLogValue;
	MapToString(mapFieldValue, strLogValue);
	CRedisContainer *pRedisContainer = pRedisHashDeleteMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		 m_pRedisVirtualService->Log(pRedisHashDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDeleteMsg->tStart, strKey, strLogValue, dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashDeleteMsg->fSpentTimeInQueue);
		 m_pRedisVirtualService->Response(pRedisHashDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		 return;
	}
			
	string strIpAddrs;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	vector<string> vecField;
	map<string, string>::const_iterator iterFieldValue;
	for(iterFieldValue = mapFieldValue.begin(); iterFieldValue != mapFieldValue.end(); iterFieldValue++)
	{
		vecField.push_back(iterFieldValue->first);
	}
	nRet = agentScheduler.Delete(strKey, vecField, strIpAddrs);
			
	m_pRedisVirtualService->Log(pRedisHashDeleteMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDeleteMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisHashDeleteMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisHashDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CRedisHashMsgOperator::DoSize(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashSizeMsg *pRedisHashSizeMsg = (SRedisHashSizeMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashSizeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashSizeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashSizeMsg->dwSequenceId;
				   
	CSapDecoder decoder(pRedisHashSizeMsg->pBuffer, pRedisHashSizeMsg->dwLen);
				
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
				   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int increment = 0;
	int nRet = RVS_SUCESS;
				
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSizeMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
		
    CRedisContainer *pRedisContainer = pRedisHashSizeMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisHashSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSizeMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
				
	string strIpAddrs;
	int size = 0;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Size(strKey, size, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSizeMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs, pRedisHashSizeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
				
    //获得响应里的Code码
    int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_HASH_SIZE);
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(pRedisHashSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSizeMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_CONFIG_ERROR, strIpAddrs, pRedisHashSizeMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisHashSizeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

    char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", size);
    string strLogValue = szTemp;
	m_pRedisVirtualService->Log(pRedisHashSizeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSizeMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisHashSizeMsg->fSpentTimeInQueue);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, size);
	m_pRedisVirtualService->Response(pRedisHashSizeMsg->handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void CRedisHashMsgOperator::DoSetAll(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashSetAllMsg *pRedisHashSetAllMsg = (SRedisHashSetAllMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashSetAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashSetAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashSetAllMsg->dwSequenceId;
					   
	CSapDecoder decoder(pRedisHashSetAllMsg->pBuffer, pRedisHashSetAllMsg->dwLen);
					
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
					   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int increment = 0;
	int nRet = RVS_SUCESS;
					
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashSetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetAllMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashSetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(mapFieldValue.size() == 0)
    {
    	m_pRedisVirtualService->Log(pRedisHashSetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetAllMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_FIELD_IS_NULL, "", pRedisHashSetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_FIELD_IS_NULL);
		return;
    }
	else
	{
	    //参数校验
		map<string, string>::iterator iter;
		for(iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
		{
			string &strValue = iter->second;
			if(strValue.empty())
			{
				strValue = VALUE_SPLITTER;
			}
		}
	}
			
	CRedisContainer *pRedisContainer = pRedisHashSetAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisHashSetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetAllMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashSetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashSetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
					
	string strIpAddrs;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Set(strKey, mapFieldValue, dwKeepTime, strIpAddrs);
					
	m_pRedisVirtualService->Log(pRedisHashSetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashSetAllMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs, pRedisHashSetAllMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisHashSetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisHashMsgOperator::DoGetAll(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashGetAllMsg *pRedisHashGetAllMsg = (SRedisHashGetAllMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashGetAllMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashGetAllMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashGetAllMsg->dwSequenceId;
		   
	CSapDecoder decoder(pRedisHashGetAllMsg->pBuffer, pRedisHashGetAllMsg->dwLen);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int increment = 0;
	int nRet = RVS_SUCESS;
		
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetAllMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
	
	CRedisContainer *pRedisContainer = pRedisHashGetAllMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisHashGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetAllMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisHashGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	//获得要返回的结构体属性信息
    int structCode = GetFirstResponseCodeByMsgId(dwServiceId, RVS_HASH_GETALL);
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByCode(structCode, oConfigType);
	vector<SConfigField> &vecConfigField = oConfigType.vecConfigField;
	bool config = true;
	if(vecConfigField.size() < 2)
	{
		config = false;
	}

	if(structCode == -1 || !config)
	{
		m_pRedisVirtualService->Log(pRedisHashGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetAllMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_CONFIG_ERROR, "", pRedisHashGetAllMsg->fSpentTimeInQueue);
	    m_pRedisVirtualService->Response(pRedisHashGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}
		
	string strIpAddrs;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.GetAll(strKey, mapFieldValue, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
	    m_pRedisVirtualService->Log(pRedisHashGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetAllMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs, pRedisHashGetAllMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strLogValue;
	MapToString(mapFieldValue, strLogValue);

	m_pRedisVirtualService->Log(pRedisHashGetAllMsg->strGuid, dwServiceId, dwMsgId, pRedisHashGetAllMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisHashGetAllMsg->fSpentTimeInQueue);
	ResponseStructArrayFromMap(pRedisHashGetAllMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, mapFieldValue, structCode, vecConfigField);

}

void CRedisHashMsgOperator::DoDeleteAll(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashDeleteAllMsg *pRedisHashDeleteAllMsg = (SRedisHashDeleteAllMsg *)pRedisHashOperateMsg;
	
	DeleteAll(pRedisHashDeleteAllMsg->dwServiceId,
		pRedisHashDeleteAllMsg->dwMsgId,
		pRedisHashDeleteAllMsg->dwSequenceId,
		pRedisHashDeleteAllMsg->strGuid,
		pRedisHashDeleteAllMsg->handler,
		pRedisHashDeleteAllMsg->pBuffer,
		pRedisHashDeleteAllMsg->dwLen,
		pRedisHashDeleteAllMsg->tStart,
		pRedisHashDeleteAllMsg->fSpentTimeInQueue,
		pRedisHashDeleteAllMsg->pRedisContainer);
}

void CRedisHashMsgOperator::DoIncby(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashIncbyMsg *pRedisHashIncbyMsg = (SRedisHashIncbyMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashIncbyMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashIncbyMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashIncbyMsg->dwSequenceId;
					   
	CSapDecoder decoder(pRedisHashIncbyMsg->pBuffer, pRedisHashIncbyMsg->dwLen);
					
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
					   
	string strKey;
	map<string, string> mapFieldValue;
	unsigned int dwKeepTime = 0;
	int nRet = RVS_SUCESS;
	int increment = 0;
					
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, strKey, mapFieldValue, dwKeepTime, increment);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisHashIncbyMsg->tStart, strKey, "", dwKeepTime, nRet, "", pRedisHashIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
			
	CRedisContainer *pRedisContainer = pRedisHashIncbyMsg->pRedisContainer;
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(pRedisHashIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisHashIncbyMsg->tStart, strKey, "", dwKeepTime, RVS_NO_REDIS_SERVER, "", pRedisHashIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}

	if(mapFieldValue.size() == 0)
	{
		m_pRedisVirtualService->Log(pRedisHashIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisHashIncbyMsg->tStart, strKey, "", dwKeepTime, RVS_REDIS_FIELD_IS_NULL, "", pRedisHashIncbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_FIELD_IS_NULL);
		return;
	}
					
	string strIpAddrs;
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	map<string, string>::const_iterator iter = mapFieldValue.begin();
	const string &strField = iter->first;
	nRet = agentScheduler.Incby(strKey, strField, increment, strIpAddrs);
	
	string strLogValue = strField;
	char szTemp[21] = {0};
	snprintf(szTemp, sizeof(szTemp), " + (%d)", increment);
	strLogValue += szTemp;
	m_pRedisVirtualService->Log(pRedisHashIncbyMsg->strGuid, dwServiceId, dwMsgId, pRedisHashIncbyMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs, pRedisHashIncbyMsg->fSpentTimeInQueue);
	m_pRedisVirtualService->Response(pRedisHashIncbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);

}

void CRedisHashMsgOperator::DoGetKey(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashGetKeyMsg *pRedisHashGetKeyMsg = (SRedisHashGetKeyMsg *)pRedisHashOperateMsg;
	
    GetKey(pRedisHashGetKeyMsg->dwServiceId,
		pRedisHashGetKeyMsg->dwMsgId,
		pRedisHashGetKeyMsg->dwSequenceId,
		pRedisHashGetKeyMsg->strGuid,
		pRedisHashGetKeyMsg->handler,
		pRedisHashGetKeyMsg->pBuffer,
		pRedisHashGetKeyMsg->dwLen,
		pRedisHashGetKeyMsg->tStart,
		pRedisHashGetKeyMsg->fSpentTimeInQueue,
		RVS_HASH_GETKEY);
}

void CRedisHashMsgOperator::DoTtl(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashTtlMsg *pRedisHashTtlMsg = (SRedisHashTtlMsg *)pRedisHashOperateMsg;

    Ttl(pRedisHashTtlMsg->dwServiceId,
		pRedisHashTtlMsg->dwMsgId,
		pRedisHashTtlMsg->dwSequenceId,
		pRedisHashTtlMsg->strGuid,
		pRedisHashTtlMsg->handler,
		pRedisHashTtlMsg->pBuffer,
		pRedisHashTtlMsg->dwLen,
		pRedisHashTtlMsg->tStart,
		pRedisHashTtlMsg->fSpentTimeInQueue,
		pRedisHashTtlMsg->pRedisContainer,
		RVS_HASH_TTL);
}

void CRedisHashMsgOperator::DoDecode(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashDecodeMsg *pRedisHashDecodeMsg = (SRedisHashDecodeMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisHashDecodeMsg->dwServiceId;
	unsigned int dwMsgId = pRedisHashDecodeMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisHashDecodeMsg->dwSequenceId;
					   
	CSapDecoder decoder(pRedisHashDecodeMsg->pBuffer, pRedisHashDecodeMsg->dwLen);
					
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);

	int nRet = RVS_SUCESS;
	string strEncodedField, strEncodedValue;
	nRet = GetRequestEnocdedInfo(dwServiceId, pBody, dwBodyLen, strEncodedField, strEncodedValue);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisHashDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDecodeMsg->tStart, "", "", 0, nRet, "", pRedisHashDecodeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(strEncodedField.empty() && strEncodedValue.empty())
	{
		m_pRedisVirtualService->Log(pRedisHashDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDecodeMsg->tStart, "", "", 0, RVS_PARAMETER_ERROR, "", pRedisHashDecodeMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisHashDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		return;
	}

    RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n strEncodedField[%s] strEncodedValue[%s]\n", __FUNCTION__, strEncodedField.c_str(), strEncodedValue.c_str());
	unsigned int pos = strEncodedField.find(CODE_VALUE_SPLITTER);
	string strValue;
	if(pos != string::npos)
	{ 
	    string strFirstFieldCode = strEncodedField.substr(0, pos);
		int firstFieldCode = atoi(strFirstFieldCode.c_str());
		RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n firstFieldCode[%d]\n", __FUNCTION__, firstFieldCode);
		
		SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
	    CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];
	    if(mapTypeByCode[firstFieldCode] == MSG_FIELD_STRUCT)
	    {
	    	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	        SServiceConfig *serviceConfig;
	        oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	        SConfigType oConfigType;
	        serviceConfig->oConfig.GetTypeByCode(firstFieldCode, oConfigType);
	        vector<SConfigField> &vecConfigField = oConfigType.vecConfigField;
			vector<string> vecStr;
			strEncodedField = strEncodedField.substr(pos+1);
			CRedisHelper::StringSplit(strEncodedField, VALUE_SPLITTER, vecStr);

			unsigned int i = 0;
			for(unsigned int j=0; i < vecConfigField.size() && j < vecStr.size(); i++,j++)
			{
				char szTemp[16] = {0};
				snprintf(szTemp, sizeof(szTemp), "%d#", vecConfigField[i].nCode);
				strValue += szTemp;
				strValue += vecStr[j];
				strValue += VALUE_SPLITTER;
			}

			vecStr.clear();
			CRedisHelper::StringSplit(strEncodedValue, VALUE_SPLITTER, vecStr);
			for(unsigned int j=0; i < vecConfigField.size() && j < vecStr.size(); i++,j++)
			{
				char szTemp[16] = {0};
				snprintf(szTemp, sizeof(szTemp), "%d#", vecConfigField[i].nCode);
				strValue += szTemp;
				strValue += vecStr[j];
				strValue += VALUE_SPLITTER;
			}
	    }
		else
		{
			unsigned int pos = strEncodedValue.find(VALUE_SPLITTER);
			if(pos == string::npos)
			{
			   //获得响应里的Code码
			   int dwCode = GetFirstResponseValueCodeByMsgId(dwServiceId, RVS_HASH_DECODE, 20001);
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
			strValue = strEncodedField + VALUE_SPLITTER + strEncodedValue;
		}
	}
	
	m_pRedisVirtualService->Log(pRedisHashDecodeMsg->strGuid, dwServiceId, dwMsgId, pRedisHashDecodeMsg->tStart, "", strValue, 0, nRet, "", pRedisHashDecodeMsg->fSpentTimeInQueue);
	Response(pRedisHashDecodeMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, strValue);
}

void CRedisHashMsgOperator::DoBatchQuery(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashBatchQueryMsg *pRedisBactchQueryMsg = (SRedisHashBatchQueryMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisBactchQueryMsg->dwServiceId;
	unsigned int dwMsgId = pRedisBactchQueryMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisBactchQueryMsg->dwSequenceId;
	
	CSapDecoder decoder(pRedisBactchQueryMsg->pBuffer, pRedisBactchQueryMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	
	vector<SKeyFieldValue> vecKFV;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, vecKFV, true);
	string strLogValue = VecKFVToString(vecKFV);
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
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.BatchQuery(vecKFV, strIpAddrs);
	strLogValue = VecKFVToString(vecKFV);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisBactchQueryMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchQueryMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchQueryMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchQueryMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	ResponseStructArrayFromVecKFV(pRedisBactchQueryMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, pBody, dwBodyLen, vecKFV);
	m_pRedisVirtualService->Log(pRedisBactchQueryMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchQueryMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchQueryMsg->fSpentTimeInQueue);
}

void CRedisHashMsgOperator::DoBatchIncrby(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashBatchIncrbyMsg *pRedisBactchIncrbyMsg = (SRedisHashBatchIncrbyMsg *)pRedisHashOperateMsg;
	unsigned int dwServiceId = pRedisBactchIncrbyMsg->dwServiceId;
	unsigned int dwMsgId = pRedisBactchIncrbyMsg->dwMsgId;
	unsigned int dwSequenceId = pRedisBactchIncrbyMsg->dwSequenceId;
	
	CSapDecoder decoder(pRedisBactchIncrbyMsg->pBuffer, pRedisBactchIncrbyMsg->dwLen);
	
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	
	vector<SKeyFieldValue> vecKFV;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestInfo(dwServiceId, pBody, dwBodyLen, vecKFV);
	string strLogValue = VecKFVToString(vecKFV);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, nRet, "", pRedisBactchIncrbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	//check param
	for(unsigned int i = 0;i < vecKFV.size(); i++)
	{
		SKeyFieldValue &kfv = vecKFV[i];
		if(!IsStrDigit(kfv.strValue))
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
	CRedisHashAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.BatchIncrby(vecKFV, strIpAddrs);
	strLogValue = VecKFVToString(vecKFV);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchIncrbyMsg->fSpentTimeInQueue);
		m_pRedisVirtualService->Response(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	ResponseStructArrayFromVecKFV(pRedisBactchIncrbyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, pBody, dwBodyLen, vecKFV);
	m_pRedisVirtualService->Log(pRedisBactchIncrbyMsg->strGuid, dwServiceId, dwMsgId, pRedisBactchIncrbyMsg->tStart, strLogValue, "", 0, nRet, strIpAddrs, pRedisBactchIncrbyMsg->fSpentTimeInQueue);
}

void CRedisHashMsgOperator::DoBatchExpire(SRedisHashOperateMsg *pRedisHashOperateMsg)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashMsgOperator::%s\n", __FUNCTION__);
	SRedisHashBatchExpireMsg *pRedisHashBatchExpireMsg = (SRedisHashBatchExpireMsg *)pRedisHashOperateMsg;

    BatchExpire(pRedisHashBatchExpireMsg->dwServiceId,
		pRedisHashBatchExpireMsg->dwMsgId,
		pRedisHashBatchExpireMsg->dwSequenceId,
		pRedisHashBatchExpireMsg->strGuid,
		pRedisHashBatchExpireMsg->handler,
		pRedisHashBatchExpireMsg->pBuffer,
		pRedisHashBatchExpireMsg->dwLen,
		pRedisHashBatchExpireMsg->tStart,
		pRedisHashBatchExpireMsg->fSpentTimeInQueue,
		pRedisHashBatchExpireMsg->pRedisContainer);
}

