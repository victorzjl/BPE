#include "IRedisTypeMsgOperator.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisContainer.h"
#include "RedisVirtualService.h"
#include "CommonMacro.h"
#include "RedisTypeAgentScheduler.h"
#include "RedisHelper.h"
#include "AvenueServiceConfigs.h"

#include <Cipher.h>
#include <SapMessage.h>

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace sdo::common;
using namespace sdo::sap;
using namespace redis;

void IRedisTypeMsgOperator::Ttl(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer, int responseType)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	CSapDecoder decoder(pBuffer, nLen);
					
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	int nRet = RVS_SUCESS;				
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, nRet, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
			
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
					
	string strIpAddrs;
	int ttl = 0;
	CRedisTypeAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.Ttl(strKey, ttl, strIpAddrs);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, nRet, strIpAddrs, fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
					
	//获得响应里的Code码
	int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, responseType);
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, strIpAddrs, fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", ttl);
    string strLogValue = szTemp;
	m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, strLogValue, 0, nRet, strIpAddrs, fSpentInQueueTime);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, ttl);
	m_pRedisVirtualService->Response(pHandler, sapEncoder.GetBuffer(), sapEncoder.GetLen());

}

void IRedisTypeMsgOperator::DeleteAll(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	CSapDecoder decoder(pBuffer, nLen);
					
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
		   
	string strKey;
	int nRet = RVS_SUCESS;				
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, nRet, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}
			
	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, RVS_NO_REDIS_SERVER, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisTypeAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.DeleteAll(strKey, strIpAddrs);

	m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, nRet, strIpAddrs, fSpentInQueueTime);
	m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void IRedisTypeMsgOperator::BatchExpire(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	CSapDecoder decoder(pBuffer, nLen);
						
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);

	vector<SKeyValue> vecKeyValue;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestVecKeyValue(dwServiceId, pBody, dwBodyLen, vecKeyValue);
	string strLogValue = VectorToString1(vecKeyValue);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, nRet, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	//check param
	for(unsigned int i = 0;i < vecKeyValue.size(); i++)
	{
		SKeyValue &keyValue = vecKeyValue[i];
		if(!IsDigit(keyValue.strValue))
		{
			m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, RVS_PARAMETER_ERROR, "", fSpentInQueueTime);
		    m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_PARAMETER_ERROR);
		    return;
		}
	}

	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, RVS_NO_REDIS_SERVER, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisTypeAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.BatchExpire(vecKeyValue, strIpAddrs);
	strLogValue = VectorToString1(vecKeyValue);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, nRet, strIpAddrs, fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet, pBody, dwBodyLen, vecKeyValue);
	m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, nRet, strIpAddrs, fSpentInQueueTime);
}

void IRedisTypeMsgOperator::BatchDelete(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	CSapDecoder decoder(pBuffer, nLen);
						
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);

	vector<SKeyValue> vecKeyValue;
	int nRet = RVS_SUCESS;
	
	nRet = GetRequestVecKeyValue(dwServiceId, pBody, dwBodyLen, vecKeyValue);
	string strLogValue = VectorToString1(vecKeyValue);
	if(nRet != 0)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, nRet, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	if(pRedisContainer == NULL)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, RVS_NO_REDIS_SERVER, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_NO_REDIS_SERVER);
		return;
	}
	
	string strIpAddrs;
	CRedisTypeAgentScheduler agentScheduler(pRedisContainer);
	nRet = agentScheduler.BatchDelete(vecKeyValue, strIpAddrs);
	strLogValue = VectorToString1(vecKeyValue);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, nRet, strIpAddrs, fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
	m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strLogValue, "", 0, nRet, strIpAddrs, fSpentInQueueTime);
}

void IRedisTypeMsgOperator::GetKey(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, int responseType)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	CSapDecoder decoder(pBuffer, nLen);
						
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
			   
	string strKey;
	int nRet = RVS_SUCESS;				
	nRet = GetRequestKey(dwServiceId, pBody, dwBodyLen, strKey);
	if(nRet != RVS_SUCESS)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, nRet, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

    //获得响应里的Code码
	int dwCode = GetFirstResponseCodeByMsgId(dwServiceId, responseType);
	if(dwCode == -1)
	{
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, RVS_REDIS_CONFIG_ERROR, "", fSpentInQueueTime);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_REDIS_CONFIG_ERROR);
		return;
	}

	m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, strKey, "", 0, nRet, "", fSpentInQueueTime);
	//发送响应
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nRet);
	sapEncoder.SetSequence(dwSequenceId);
	sapEncoder.SetValue(dwCode, strKey);
	m_pRedisVirtualService->Response(pHandler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

int IRedisTypeMsgOperator::GetRequestKey(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
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
		RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		return RVS_KEY_TOO_LONG;
	}
	
	if(!IsRedisKeyValid(strKey))
	{
		strKey = MakeRedisKeyValid(strKey);
	}
	
	return RVS_SUCESS;
}

int IRedisTypeMsgOperator::GetRequestVecKeyValue(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, vector<SKeyValue> &vecKeyValue, bool onlyResolveKey)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, serviceId[%d], pBuffer[%x], dwLen[%d]\n", __FUNCTION__, dwServiceId, pBuffer, dwLen);
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
			string strValue;
			if(onlyResolveKey)
			{
				strValue = "0";
			}
			int nRet = GetKeyValueFromStruct(dwServiceId, nType, ptrLoc+sizeof(SSapMsgAttribute), nLen-sizeof(SSapMsgAttribute), strKey, strValue);
			if(nRet != 0)
			{
				return RVS_REDIS_STRUCT_IS_WRONG;
			}

			if(strKey.length() > 1024)
	        {
		        RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
		        return RVS_KEY_TOO_LONG;
	        }

	        if(!IsRedisKeyValid(strKey))
	        {
		       strKey = MakeRedisKeyValid(strKey);
	        }

			SKeyValue keyValue;
			keyValue.strKey = strKey;
			keyValue.strValue = strValue;
			vecKeyValue.push_back(keyValue);
		}

		ptrLoc+=nFactLen;
	}
	
    if(vecKeyValue.size() == 0)
    {
    	return RVS_KEY_IS_NULL;
    }
	return 0;
}

int IRedisTypeMsgOperator::GetKeyValueFromStruct(unsigned int dwServiceId, unsigned int nCode, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, serviceId[%d], Code[%d], dwLen[%d]\n", __FUNCTION__, dwServiceId, nCode, dwLen);
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
			if(field.nCode >= 10000)
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
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, resolveFieldSize[%d]\n", __FUNCTION__, resolveFieldSize);
	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	for(unsigned int i = 0; i < resolveFieldSize; i++)
	{
		const SConfigField &oConfigField = vecField[i];
		RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, fieldName[%s], fieldType[%d]\n", __FUNCTION__, oConfigField.strName.c_str(), oConfigField.eStructFieldType);
		string tempStr;
		if(oConfigField.eStructFieldType == MSG_FIELD_INT)
		{
		    if(ptrLoc + 4 > (unsigned char *)pBuffer + dwLen)
		    {
		       RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, Type[ int ] over the range\n", __FUNCTION__);
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
		        if(oConfigField.nLen == -1)
		        {
		            RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, The type[ not int ] not set the len\n", __FUNCTION__);
		        	return -1;
		        }
				if(ptrLoc + oConfigField.nLen > (unsigned char *)pBuffer + dwLen)
		        {
		            RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, Type[ string ] over the range\n", __FUNCTION__);
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

		RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, tempStr[%s %d]\n", __FUNCTION__, tempStr.c_str(), tempStr.length());
		if(oConfigField.nCode < 10000)
		{
		    if(!tempStr.empty())
		    {
		    	strKey += "_";
			    strKey += tempStr;
		    }
		}
		else
		{
			if(strValue.empty())
			{
				strValue =  tempStr;
			}
		}

	}
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, return strKey[%s], strValue[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
    return 0;
}

int IRedisTypeMsgOperator::GetRequestEnocdedInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strEncodedField, string &strEncodedValue)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
	CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];

	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);

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
	
		
		SConfigType oConfigType;
		serviceConfig->oConfig.GetTypeByCode(nType, oConfigType);
		if(oConfigType.strName.compare(ENCODED_FIELD_TYPE) == 0)
		{
			strEncodedField = strValue;
		}
		if(oConfigType.strName.compare(ENCODED_VALUE_TYPE) == 0)
		{
			strEncodedValue = strValue;
		}
	
		ptrLoc+=nFactLen;
	}
    return 0;
}

void IRedisTypeMsgOperator::ResponseStructArrayFromVec( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, vector<string> &vecValue, int structCode, const vector<SConfigField> &vecConfigField)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, ServiceId[%d], MsgId[%d], responseCode[%d], structCode[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode, structCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);

	vector<SConfigField>::const_iterator fieldIter;
	int nLen = 0;
	for(fieldIter = vecConfigField.begin(); fieldIter != vecConfigField.end(); fieldIter++)
	{
	    if((*fieldIter).eStructFieldType == MSG_FIELD_INT)
	    {
	    	nLen += 4;
	    }
		else
		{
			if((*fieldIter).nLen >= 0)
	        {
	    	    nLen += (*fieldIter).nLen;
	        }
		}
	}
	const int fixedLen = nLen;
	int lastFieldLen = vecConfigField.back().nLen;
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, fixedLen[%d], lastFieldLen[%d]\n", __FUNCTION__, fixedLen, lastFieldLen);
	vector<string>::const_iterator iter;
	for (iter = vecValue.begin(); iter != vecValue.end(); iter++)
	{
	    int nCode = 0;
		vector<string> vecString;
		CRedisHelper::StringSplit(*iter, VALUE_SPLITTER, vecString);

        nLen = fixedLen;
		map<int, string> mapCodeValue;
		string str = vecString[0];
		unsigned int pos = str.find('#');
		if(pos != string::npos)
		{
			string strCode = str.substr(0, pos);
			nCode = atoi(strCode.c_str());
			str = str.substr(pos+(unsigned int)1, str.length()-pos-(unsigned int)1);
			if(nCode != structCode)
			{
				mapCodeValue[nCode] = str;
			}
			else if(lastFieldLen == -1 && vecString.size() == vecConfigField.size())
            {
        	    nLen += vecString.back().length();
            }
		}
		if(mapCodeValue.size() != 0)
		{
			for(int i = 1; i < (int)vecString.size(); i++)
			{
				str = vecString[i];
			    unsigned int pos = str.find('#');
			    if(pos != string::npos)
			    {
				    string strCode = str.substr(0, pos);
				    nCode = atoi(strCode.c_str());
					str = str.substr(pos+(unsigned int)1, str.length()-pos-(unsigned int)1);
					mapCodeValue[nCode] = str;
				}
				else
				{
					int code = vecConfigField.back().nCode;
					mapCodeValue[code] = str;
				}
			}
			vecString.clear();

            if(lastFieldLen == -1)
            {
            	map<int, string>::const_iterator iter = mapCodeValue.find(vecConfigField.back().nCode);
			    if(iter != mapCodeValue.end())
			    {
				   nLen += iter->second.length();
			    }
            }
		}

        char * const pBuffer = (char *)malloc(nLen);
		memset(pBuffer, 0 , nLen);
	    char *ptrLoc = pBuffer;
		unsigned int i = 0;
		for(fieldIter = vecConfigField.begin(); fieldIter != vecConfigField.end(); fieldIter++)
		{ 
			if(mapCodeValue.size() != 0)
			{
				map<int, string>::const_iterator iter = mapCodeValue.find((*fieldIter).nCode);
				if(iter != mapCodeValue.end())
				{
					str = iter->second;
				}
				else
				{
					str = "";
				}
			}
			else
			{
				if(i >= vecString.size())
				{
					break;
				}
				if(i != 0)
				{
					str = vecString[i];
				}	
                i++;
			}

			RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, str[%s %d]\n", __FUNCTION__, str.c_str(), str.length());
			if(!str.empty())
			{
			    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, nCode[%d] type[%d]\n", __FUNCTION__, (*fieldIter).nCode, (*fieldIter).eStructFieldType);
			    if((*fieldIter).eStructFieldType == MSG_FIELD_INT)
			    {
				     int nNetValue = htonl(atoi(str.c_str()));
				     memcpy(ptrLoc, &nNetValue, 4);
				     ptrLoc += 4;
			     }
			     else
			     {
				     unsigned int len = (*fieldIter).nLen == -1 ? str.length() : (unsigned int)((*fieldIter).nLen);
					 unsigned int copyLen = len > str.length() ? str.length() : len; 
				     memcpy(ptrLoc, str.c_str(), copyLen);
				     ptrLoc += len;
					 RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, Len[%d] copyLen[%d]\n", __FUNCTION__, len , copyLen);
			     }		
			}
			else
			{
				if((*fieldIter).eStructFieldType == MSG_FIELD_INT)
			    {
				     ptrLoc += 4;
			    }
			    else
			    {
				     ptrLoc += (*fieldIter).nLen;
			    }		
			}
					
		}
		sapEncoder.SetValue(structCode, pBuffer, nLen);
		free(pBuffer);
	}

	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());

}

void IRedisTypeMsgOperator::ResponseStructArrayFromMap( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, map<string, string> &mapFieldValue, int structCode, const vector<SConfigField> &vecConfigField)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, ServiceId[%d], MsgId[%d], responseCode[%d], structCode[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode, structCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);

	vector<SConfigField>::const_iterator fieldIter;
	int nLen = 0;
	for(fieldIter = vecConfigField.begin(); fieldIter != vecConfigField.end(); fieldIter++)
	{
	    if((*fieldIter).eStructFieldType == MSG_FIELD_INT)
	    {
	    	nLen += 4;
	    }
		else
		{
			if((*fieldIter).nLen >= 0)
	        {
	    	    nLen += (*fieldIter).nLen;
	        }
		}
	}
	const int fixedLen = nLen;
	int lastFieldLen = vecConfigField.back().nLen;
	if(vecConfigField.back().eStructFieldType == MSG_FIELD_INT)
	{
		lastFieldLen = 4;
	}
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, fixedLen[%d], lastFieldLen[%d]\n", __FUNCTION__, fixedLen, lastFieldLen);
	map<string, string>::const_iterator iter;
	for (iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
	{
	    int nCode = 0;
	    string strFieldAndValue = iter->first + VALUE_SPLITTER + iter->second;
		vector<string> vecString;
		CRedisHelper::StringSplit(strFieldAndValue, VALUE_SPLITTER, vecString);

        nLen = fixedLen;
		map<int, string> mapCodeValue;
		string str = vecString[0];
		unsigned int pos = str.find('#');
		if(pos != string::npos)
		{
			string strCode = str.substr(0, pos);
			nCode = atoi(strCode.c_str());
			str = str.substr(pos+(unsigned int)1, str.length()-pos-(unsigned int)1);
			if(nCode != structCode)
			{
				mapCodeValue[nCode] = str;
			}
			else if(lastFieldLen == -1 && vecString.size() == vecConfigField.size())
            {
        	    nLen += vecString.back().length();
            }
		}
		if(mapCodeValue.size() != 0)
		{
			for(int i = 1; i < (int)vecString.size(); i++)
			{
				str = vecString[i];
			    unsigned int pos = str.find('#');
			    if(pos != string::npos)
			    {
				    string strCode = str.substr(0, pos);
				    nCode = atoi(strCode.c_str());
					str = str.substr(pos+(unsigned int)1, str.length()-pos-(unsigned int)1);
					mapCodeValue[nCode] = str;
				}
				else
				{
					int code = vecConfigField.back().nCode;
					mapCodeValue[code] = str;
				}
			}
			vecString.clear();

            if(lastFieldLen == -1)
            {
            	map<int, string>::const_iterator iter = mapCodeValue.find(vecConfigField.back().nCode);
			    if(iter != mapCodeValue.end())
			    {
				   nLen += iter->second.length();
			    }
            }
		}

        char * const pBuffer = (char *)malloc(nLen);
		memset(pBuffer, 0 , nLen);
	    char *ptrLoc = pBuffer;
		unsigned int i = 0;
		for(fieldIter = vecConfigField.begin(); fieldIter != vecConfigField.end(); fieldIter++)
		{ 
			if(mapCodeValue.size() != 0)
			{
				map<int, string>::const_iterator iter = mapCodeValue.find((*fieldIter).nCode);
				if(iter != mapCodeValue.end())
				{
					str = iter->second;
				}
				else
				{
					str = "";
				}
			}
			else
			{
				if(i >= vecString.size())
				{
					break;
				}
				if(i != 0)
				{
					str = vecString[i];
				}	
                i++;
			}

			RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, str[%s %d]\n", __FUNCTION__, str.c_str(), str.length());
			if(!str.empty())
			{
			    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, nCode[%d] type[%d]\n", __FUNCTION__, (*fieldIter).nCode, (*fieldIter).eStructFieldType);
			    if((*fieldIter).eStructFieldType == MSG_FIELD_INT)
			    {
				     int nNetValue = htonl(atoi(str.c_str()));
				     memcpy(ptrLoc, &nNetValue, 4);
				     ptrLoc += 4;
			     }
			     else
			     {
				     unsigned int len = (*fieldIter).nLen == -1 ? str.length() : (unsigned int)((*fieldIter).nLen);
					 unsigned int copyLen = len > str.length() ? str.length() : len; 
				     memcpy(ptrLoc, str.c_str(), copyLen);
				     ptrLoc += len;
					 RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, Len[%d] copyLen[%d]\n", __FUNCTION__, len , copyLen);
			     }		
			}
			else
			{
				if((*fieldIter).eStructFieldType == MSG_FIELD_INT)
			    {
				     ptrLoc += 4;
			    }
			    else
			    {
				     ptrLoc += (*fieldIter).nLen;
			    }		
			}
					
		}
		sapEncoder.SetValue(structCode, pBuffer, nLen);
		free(pBuffer);
	}

	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void IRedisTypeMsgOperator::ResponseStructArrayFromVecKFV( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const void *pBuffer, unsigned int dwLen, const vector<SKeyFieldValue> &vecKFV)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
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
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, lastFieldLen[%d]\n", __FUNCTION__, lastFieldLen);
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
		
		if(field.nCode > 20000)
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
	while(i < vecKFV.size() && ptrLoc < ((unsigned char *)pBuffer + dwLen))
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen = ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
		    RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, Packet Error\n", __FUNCTION__);
			return;
		}

        const SKeyFieldValue &kfv = vecKFV[i];
		int resultLen = totalStructFieldLen;
		
		vector<string> vecValue;
		CRedisHelper::StringSplit(kfv.strValue, VALUE_SPLITTER, vecValue);
        if(vecValue.size() == 1)
	    {
           if(lastFieldLen < 0)
           {
        	    resultLen += vecValue[0].length();
           }
		   int code = vecConfigField.back().nCode;
		   mapCodeValue[code] = vecValue[0];
        }
		else
		{
			for(unsigned int j = 0; j < vecValue.size(); j++)
			{
				string &str = vecValue[j];
			    unsigned int pos = str.find('#');
			    if(pos != string::npos)
			    {
				    string strCode = str.substr(0, pos);
				    nCode = atoi(strCode.c_str());
					string strValue = str.substr(pos+(unsigned int)1, str.length()-pos-(unsigned int)1);
					mapCodeValue[nCode] = strValue;
				}
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
        for(;configField < vecConfigField.size(); configField++)
		{
		    SConfigField &field = vecConfigField[configField];
			map<unsigned int, string>::iterator iter = mapCodeValue.find(field.nCode);	
			if(iter != mapCodeValue.end() && !(iter->second.empty()))
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
			          memcpy(resultLoc, strValue.c_str(), copyLen);
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

        sapEncoder.SetValue(structCode, resultBuffer, resultLen);
        free(resultBuffer);
		mapCodeValue.clear();
		ptrLoc += nFactLen;
		i++;
	}
	
	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void IRedisTypeMsgOperator::Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);
	
	SERVICE_CODE_TYPE_MAP &mapCodeTypeByService = m_pRedisVirtualService->GetServiceCodeTypeMap();
	CODE_TYPE_MAP &mapTypeByCode = mapCodeTypeByService[dwServiceId];

    vector<string> vecValue;
	CRedisHelper::StringSplit(strValue, VALUE_SPLITTER, vecValue);
	vector<string>::const_iterator iterVec;
	for(iterVec = vecValue.begin(); iterVec != vecValue.end(); iterVec++)
	{
		 string strValue = *iterVec;
		 RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, value[%s]\n", __FUNCTION__, strValue.c_str());
	     unsigned int pos = strValue.find('#');
		 if(pos == string::npos)
		 {
		     continue;
		 }
		 string strCode = strValue.substr(0, pos);
		 if(strCode.empty() || !IsStrDigit(strCode))
		 {
		 	continue;
		 }
	     unsigned int dwCode = atoi(strCode.c_str());
	     strValue = strValue.substr(pos+(unsigned int)1, strValue.length()-pos-(unsigned int)1);
		 RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, split  value[%s]\n", __FUNCTION__, strValue.c_str());
	     if(mapTypeByCode[dwCode] == MSG_FIELD_INT)
		 {
			 sapEncoder.SetValue(dwCode, atoi(strValue.c_str()));
		 }
		 else
		 {
			 sapEncoder.SetValue(dwCode, strValue);
		 }
	}

	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

void IRedisTypeMsgOperator::Response(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const void *pBuffer, unsigned int dwLen, const vector<SKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
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
	
	int totalStructFieldLen = 0;
    int valueBeginPos = -1;
	int totalKeyLen = 0;
	RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, lastFieldLen[%d]\n", __FUNCTION__, lastFieldLen);
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
	while(i < vecKeyValue.size() && ptrLoc < ((unsigned char *)pBuffer + dwLen))
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen = ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
		    RVS_XLOG(XLOG_ERROR, "IRedisTypeMsgOperator::%s, Packet Error\n", __FUNCTION__);
			return;
		}

        const SKeyValue &keyValue = vecKeyValue[i];
		vector<RVSKeyValue> vecResult;
		int resultLen = totalStructFieldLen;
		bool valueIsDigit = false;
		if(IsDigit(keyValue.strValue))
		{
			valueIsDigit = true;
			if(lastFieldLen < 0)
			{
				resultLen += keyValue.strValue.length();
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
	
        sapEncoder.SetValue(structCode, resultBuffer, resultLen);
        free(resultBuffer);
		ptrLoc += nFactLen;
		i++;
	}
	
	m_pRedisVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

//获取对应消息号的第一个响应码
int IRedisTypeMsgOperator::GetFirstResponseCodeByMsgId(unsigned int dwServiceId, int operate)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, dwServiceId[%d], MsgId[%d]\n", __FUNCTION__, dwServiceId, operate);
	//获得响应里的Code码
	int dwCode = -1;
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigMessage * messageType;
	serviceConfig->oConfig.GetMessageTypeById(operate, &messageType);
	map<string, SConfigField> &mapConfigField = messageType->oResponseType.mapFieldByName;
	map<string, SConfigField>::const_iterator iterResponseField = mapConfigField.begin();
	if(iterResponseField == mapConfigField.end())
	{
		return -1;
	}
	SConfigType oConfigType;
	serviceConfig->oConfig.GetTypeByName(iterResponseField->second.strTypeName, oConfigType);
	dwCode = oConfigType.nCode;
	return dwCode;
}

int IRedisTypeMsgOperator::GetFirstResponseValueCodeByMsgId(unsigned int dwServiceId, int operate, int minValueCode)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, dwServiceId[%d], MsgId[%d]\n", __FUNCTION__, dwServiceId, operate);
	//获得响应里的Code码
	int dwCode = -1;
	CAvenueServiceConfigs *oServiceConfigs = m_pRedisVirtualService->GetAvenueServiceConfigs();
	SServiceConfig *serviceConfig;
	oServiceConfigs->GetServiceById(dwServiceId, &serviceConfig);
	SConfigMessage * messageType;
	serviceConfig->oConfig.GetMessageTypeById(operate, &messageType);
	map<string, SConfigField> &mapConfigField = messageType->oResponseType.mapFieldByName;
	map<string, SConfigField>::const_iterator iterResponseField = mapConfigField.begin();
	SConfigType oConfigType;
	for(;iterResponseField != mapConfigField.end(); iterResponseField++)
	{
		int success = serviceConfig->oConfig.GetTypeByName(iterResponseField->second.strTypeName, oConfigType);
		if(success != 0)
		{
			continue;
		}
		dwCode = oConfigType.nCode;
		if(dwCode >= minValueCode)
		{
			return dwCode;
		}
	}
	
	return -1;
}

//判断字符是否可见
bool IRedisTypeMsgOperator::IsRedisKeyValid( const string &strKey )
{
	const char *p = strKey.c_str();
	while(*p != '\0')
	{
		if(*p>126 || *p<33)
		{
			return false;
		}
		p++;
	}

	return true;
}

//判断字符串是否是整数，包括正数和负数
bool IRedisTypeMsgOperator::IsStrDigit(const string &str)
{
    if(str.empty())
    {
    	return false;
    }
	
	const char *p = str.c_str();
	if(*p != '\0' && *p == 45)
	{
		p++;
		if(*p == '\0' || *p<49 || *p>57)
		{
			return false;
		}
		p++;
	}
	while(*p != '\0')
	{
		if(*p>57 || *p<48)
		{
			return false;
		}
		p++;
	}

	return true;
}

//检查字符串是否都是阿拉伯数字
bool IRedisTypeMsgOperator::IsDigit(const string &str)
{
    if(str.empty())
    {
    	return false;
    }
	const char *p = str.c_str();
	while(*p != '\0')
	{
		if(!isdigit(*p))
		{
			return false;
		}
		p++;
	}

	return true;
}


//进行Base64编码
string IRedisTypeMsgOperator::MakeRedisKeyValid( const string &strKey )
{
	string strEncodeValue;

	char szBase64[2048] = {0};
	CCipher::Base64Encode(szBase64, strKey.c_str(), strKey.length());
	strEncodeValue = szBase64;

	return strEncodeValue;
}

void IRedisTypeMsgOperator::VectorToString(const vector<string> &vecValue, string &strValue)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s, vecValueSize[%d]\n", __FUNCTION__, vecValue.size());
	
    strValue = "[";
	for(unsigned int i = 0; i < vecValue.size(); i++)
	{
	    if(i == 0)
	    {
	    	strValue += vecValue[i];
	    }
		else
		{
			strValue += " , " + vecValue[i];
		}	
	}
	strValue += "]";
}

string IRedisTypeMsgOperator::VectorToString1(const vector<SKeyValue> &vecKeyValue)
{
	string strValue;
	vector<SKeyValue>::const_iterator iter;
	for (iter = vecKeyValue.begin(); iter != vecKeyValue.end(); iter++)
	{
		const SKeyValue &objKeyValue = *iter;

        if(!strValue.empty())
        {
            strValue += VALUE_SPLITTER;
        }
		strValue += objKeyValue.strKey;
		strValue += ":";
		strValue += objKeyValue.strValue;
	}

	return strValue;
}

void IRedisTypeMsgOperator::MapToString(map<string, string> &mapFieldValue, string &str)
{
    RVS_XLOG(XLOG_DEBUG, "IRedisTypeMsgOperator::%s\n", __FUNCTION__);
	map<string, string>::const_iterator iter;
    str = "";
	for(iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
	{
		str += "[" + iter->first;
		str += " : " + iter->second + "]";
	}
}

