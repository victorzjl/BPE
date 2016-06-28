#include "ServiceConfig.h"
#include "AsyncVirtualServiceLog.h"
#include <algorithm>

using namespace sdo::common;

CServiceConfig::CServiceConfig():m_bIsTreeStruct(true),m_bIsClearText(false)
{
	m_mapETypeByName["int"] = MSG_FIELD_INT;
	m_mapETypeByName["string"] = MSG_FIELD_STRING;
	m_mapETypeByName["array"] = MSG_FIELD_ARRAY;
	m_mapETypeByName["tlv"] = MSG_FIELD_TLV;
	m_mapETypeByName["struct"] = MSG_FIELD_STRUCT;
}


int CServiceConfig::LoadConfigTypeConfig_(TiXmlElement * pConfigType)
{
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::LoadConfigTypeConfig_\n");
	SConfigType oConfigType;
	
	if(GetTypeAttr_(pConfigType, oConfigType) < 0)
	{
		CVS_XLOG(XLOG_ERROR, "xml GetTypeAttr failed\n");
    	return -1;
	}

	if(oConfigType.eType == MSG_FIELD_TLV || oConfigType.eType == MSG_FIELD_STRUCT)
	{
		TiXmlElement * pConfigField = NULL;
		while(NULL!=(pConfigField =(TiXmlElement *) pConfigType->IterateChildren(pConfigField)))
		{

			SConfigField oConfigField;
			bool bStruct = (oConfigType.eType == MSG_FIELD_STRUCT) ? true : false;
			
			if(GetFieldAttr_(pConfigField, oConfigField, bStruct) < 0)
			{
				CVS_XLOG(XLOG_ERROR, "xml GetFieldAttr failed\n");
				return -1;
			}
			
			(oConfigType.mapFieldByName)[oConfigField.strName] = oConfigField;
			(oConfigType.mapFieldByType)[oConfigField.strTypeName] = oConfigField;

			if(oConfigType.eType == MSG_FIELD_STRUCT) 
			{
				oConfigType.vecConfigField.push_back(oConfigField);
			}
			else
			{
				SConfigTypePair oConfigTypePair;
				oConfigTypePair.strCurrentTypeName= oConfigField.strTypeName;
				oConfigTypePair.strPreviousTypeName = oConfigType.strName;
				
				m_mapTypePairByFieldName.insert(make_pair(oConfigField.strName, oConfigTypePair));
				if(oConfigType.bArrayField)
				{
					oConfigType.strArraryName = oConfigField.strTypeName;
				}
			}
		}
	}
	m_mapTypeByName[oConfigType.strName] = oConfigType;
	if(oConfigType.eType != MSG_FIELD_ARRAY)
	{
		m_mapTypeByCode[oConfigType.nCode] = oConfigType;
	}
	return 0;
}


int CServiceConfig::LoadMsgField_(SConfigMessage & oConfigMessage, TiXmlElement * pConfigFields, bool bRequest)
{
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::LoadMsgField_\n");
	TiXmlElement * pConfigField = NULL;
	pConfigField=pConfigFields->FirstChildElement();
	while(pConfigField)
	{
		SConfigField oConfigField;
		if(GetFieldAttr_(pConfigField, oConfigField) < 0)
		{
			CVS_XLOG(XLOG_ERROR, "xml GetFieldAttr failed\n");
			return -1;
		}
		SConfigTypePair oConfigTypePair;
		oConfigTypePair.strCurrentTypeName= oConfigField.strTypeName;
		if(bRequest == true)
		{
			(oConfigMessage.oRequestType.mapFieldByName)[oConfigField.strName] = oConfigField;
			(oConfigMessage.oRequestType.mapFieldByType)[oConfigField.strTypeName] = oConfigField;
			oConfigTypePair.strPreviousTypeName = oConfigMessage.oRequestType.strName;
		}
		else 
		{
			(oConfigMessage.oResponseType.mapFieldByName)[oConfigField.strName] = oConfigField;
			(oConfigMessage.oResponseType.mapFieldByType)[oConfigField.strTypeName] = oConfigField;
			oConfigTypePair.strPreviousTypeName= oConfigMessage.oResponseType.strName;
		}

		//check if is array
		map<string, SConfigType>::iterator itrMapByTypeName = m_mapTypeByName.find(oConfigField.strTypeName);
		if(itrMapByTypeName != m_mapTypeByName.end())
		{
			SConfigType & oItemType = itrMapByTypeName->second;
			if(oItemType.eType == MSG_FIELD_ARRAY)
			{
				itrMapByTypeName = m_mapTypeByName.find(oItemType.strItemType);
				if(itrMapByTypeName != m_mapTypeByName.end())
				{
					SConfigType & oItemElementType = itrMapByTypeName->second;
					oConfigMessage.mapArraryTypeNameByCode[oItemElementType.nCode] = oItemType.strName;
					CVS_XLOG(XLOG_TRACE, "set the array element code[%d] type[%s]\n", oItemElementType.nCode,
						oItemType.strName.c_str());
				}
				else 
				{
					CVS_XLOG(XLOG_ERROR, "can not find the array element type[%s]\n", oItemType.strItemType.c_str());
    				return -1;
				}
			}
		}
		else 
		{
			CVS_XLOG(XLOG_ERROR, "can not find the type[%s]\n", oConfigField.strTypeName.c_str());
			return -1;
		}
	
		
		m_mapTypePairByFieldName.insert(make_pair(oConfigField.strName, oConfigTypePair));
		pConfigField = pConfigField->NextSiblingElement();
	}
	return 0;

}

int CServiceConfig::LoadConfigMsgConfig_(TiXmlElement * pConfigMsg)
{
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::LoadConfigMsgConfig_\n");
	SConfigMessage oConfigMessage;
	if(GetMsgAttr_(pConfigMsg, oConfigMessage) < 0)
	{
		CVS_XLOG(XLOG_ERROR, "xml GetTypeAttr failed\n");
    	return -1;
	}
	
	//request
	char szTypeName[128] = {0};
	int nLen = sprintf(szTypeName, "%s_Req", oConfigMessage.strName.c_str());
	oConfigMessage.oRequestType.strName = string(szTypeName, nLen);
	oConfigMessage.oRequestType.nCode = oConfigMessage.dwId;
	oConfigMessage.oRequestType.eType = MSG_FIELD_TLV;
	m_mapTypeByName[oConfigMessage.oRequestType.strName] = oConfigMessage.oRequestType;
	TiXmlElement * pConfigFields = pConfigMsg->FirstChildElement("requestParameter");
	if(pConfigFields == NULL)
	{
		CVS_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ have no requestParameter node\n");
    	return -1;
	}
	if(LoadMsgField_(oConfigMessage, pConfigFields) != 0)
	{
		CVS_XLOG(XLOG_ERROR, "Load Msg Request Field failed failed\n");
    	return -1;
	}
	
	//response
	nLen = sprintf(szTypeName, "%s_Res", oConfigMessage.strName.c_str());
	oConfigMessage.oResponseType.strName = string(szTypeName, nLen);
	oConfigMessage.oResponseType.nCode = oConfigMessage.dwId;
	oConfigMessage.oResponseType.eType = MSG_FIELD_TLV;
	m_mapTypeByName[oConfigMessage.oResponseType.strName] = oConfigMessage.oResponseType;
	pConfigFields = pConfigMsg->FirstChildElement("responseParameter");
	if(pConfigFields == NULL)
	{
		CVS_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ have no responseparameter node\n");
    	return -1;
	}
	if(LoadMsgField_(oConfigMessage, pConfigFields, false) != 0)
	{
		CVS_XLOG(XLOG_ERROR, "Load Msg Response Field failed\n");
    	return -1;
	}

	m_mapMessageByName[oConfigMessage.strName] = oConfigMessage;
	m_mapMessageById[oConfigMessage.dwId] = oConfigMessage;
	return 0;
}


int	CServiceConfig::LoadServiceConfig(const string &strConfig)
{
    TiXmlDocument m_xmlDoc;
    TiXmlElement *pService;
    if(!m_xmlDoc.LoadFile(strConfig.c_str()))
    {
       	CVS_XLOG(XLOG_ERROR, "xml load file[%s] failed\n", strConfig.c_str());
        return -1;
    }
	
    if((pService=m_xmlDoc.RootElement())==NULL)
    {
        CVS_XLOG(XLOG_ERROR, "xml no root node\n");
        return -1;
    }

	
	//service
	
	char szName[64] = {0};
	int nRet = pService->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "serice node get name attr failed\n");
    	return -1;
	}
	m_strServiceName = szName;
	transform (m_strServiceName.begin(), m_strServiceName.end(), m_strServiceName.begin(), (int(*)(int))tolower);
	
    nRet = pService->QueryIntAttribute("id", (int *)&m_dwServiceId);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "serice node get id attr failed\n");
    	return -1;
	}
	
	char szClear[64] = {0};
	nRet = pService->QueryValueAttribute("cleartext", &szClear);
	if(nRet == TIXML_SUCCESS)
	{
		if (strcmp(szClear,"true")==0)
			m_bIsClearText = true;
	}
	
	char szEncode[64] = {0};
	nRet = pService->QueryValueAttribute("serviceencoding", &szEncode);
	if(nRet != TIXML_SUCCESS)
	{
		m_strServiceEncoding = "utf-8";
	}
	m_strServiceEncoding = szEncode;
	transform (m_strServiceEncoding.begin(), m_strServiceEncoding.end(), m_strServiceEncoding.begin(), (int(*)(int))tolower);

	nRet = pService->QueryValueAttribute("sdkencoding", &szEncode);
	if(nRet != TIXML_SUCCESS)
	{
		m_strSdkEncoding = "utf-8";
	}
	m_strSdkEncoding = szEncode;
	transform (m_strSdkEncoding.begin(), m_strSdkEncoding.end(), m_strSdkEncoding.begin(), (int(*)(int))tolower);
	
	nRet = pService->QueryValueAttribute("IsTreeStruct", &szEncode);
	if(nRet == TIXML_SUCCESS)
	{
		string strIsTreeStruct = szEncode;
		transform (strIsTreeStruct.begin(), strIsTreeStruct.end(), strIsTreeStruct.begin(), (int(*)(int))tolower);
		if(strcmp(strIsTreeStruct.c_str(), "true") == 0)
		{
			m_bIsTreeStruct = true;
		}
		else 
		{
			m_bIsTreeStruct = false;
		}
	}
	m_bIsTreeStruct = true;	//add by zpt 2011.12.7 使所有的配置文件中的IsTreeStruct均为true

	char szPrefix[64] = {0};
	nRet = pService->QueryValueAttribute("KeyPrefix", &szPrefix);
	if (nRet != TIXML_SUCCESS)
	{
		m_strKeyPrefix = "";
	}
	m_strKeyPrefix = szPrefix;
	//transform(m_strKeyPrefix.begin(), m_strKeyPrefix.end(), m_strKeyPrefix.begin(), (int(*)(int))tolower);

	//type
	TiXmlElement * pConfigType = NULL;
	if((pConfigType=pService->FirstChildElement("type"))==NULL)
	{
		CVS_XLOG(XLOG_ERROR, "xml have no type node\n");
		return -1; 
	}
	else
	{	
		if(LoadConfigTypeConfig_(pConfigType) != 0)
		{
			CVS_XLOG(XLOG_ERROR, "LoadConfigTypeConfig_ failed\n");
			return -1; 
		}
	}
	while((pConfigType=pConfigType->NextSiblingElement("type"))!=NULL)
	{
		if(LoadConfigTypeConfig_(pConfigType) != 0)
		{
			CVS_XLOG(XLOG_ERROR, "LoadConfigTypeConfig_ failed\n");
			return -1; 
		}
	}

	//message
	TiXmlElement * pConfigMsg = NULL;
	if((pConfigMsg=pService->FirstChildElement("message"))==NULL)
	{
		CVS_XLOG(XLOG_ERROR, "xml have no message node\n");
		return -1; 
	}
	else
	{	
		if(LoadConfigMsgConfig_(pConfigMsg) != 0)
		{
			CVS_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ failed\n");
			return -1; 
		}
	}
	while((pConfigMsg=pConfigMsg->NextSiblingElement("message"))!=NULL)
	{
		if(LoadConfigMsgConfig_(pConfigMsg) != 0)
		{
			CVS_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ failed\n");
			return -1; 
		}

	}
	return 0;
}

int CServiceConfig::GetTypeAttr_(const TiXmlElement * pElement, SConfigType & oConfigType)
{
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::GetTypeAttr\n");
	char szName[64] = {0};
	int nRet = pElement->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "type node get name attr failed\n");
    	return -1;
	}
	oConfigType.strName = szName;
	transform (oConfigType.strName.begin(), oConfigType.strName.end(), oConfigType.strName.begin(), (int(*)(int))tolower);

	char szClass[16] = {0};
	nRet = pElement->QueryValueAttribute("class", &szClass);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "type node get class attr failed\n");
    	return -1;
	}
	string strClass = szClass;
	transform (strClass.begin(), strClass.end(), strClass.begin(), (int(*)(int))tolower);
	oConfigType.eType = m_mapETypeByName[strClass];
	
	if(oConfigType.eType == MSG_FIELD_ARRAY)
	{
		nRet = pElement->QueryValueAttribute("itemType", &szName);
		if(nRet != TIXML_SUCCESS)
		{
			CVS_XLOG(XLOG_ERROR, "type node get itemType attr failed\n");
	    	return -1;
		}
		string strItemType = szName;
		transform (strItemType.begin(), strItemType.end(), strItemType.begin(), (int(*)(int))tolower);
		oConfigType.strItemType = strItemType;
	}

	nRet = pElement->QueryIntAttribute("code", &(oConfigType.nCode));
	if(nRet != TIXML_SUCCESS && oConfigType.eType != MSG_FIELD_ARRAY)
	{
		CVS_XLOG(XLOG_ERROR, "type node get code attr failed\n");
    	return -1;
	}

	char szArrayField[16]= {0};
	nRet = pElement->QueryValueAttribute("arrayfield", &szArrayField);
	if(nRet == TIXML_SUCCESS)
	{
		string strArrayField = szArrayField;
		transform (strArrayField.begin(), strArrayField.end(), strArrayField.begin(), (int(*)(int))tolower);
		if(strcmp(strArrayField.c_str(), "true") == 0)
		{
			oConfigType.bArrayField = true;
		}
		else 
		{
			oConfigType.bArrayField = false;
		}
	}
	else 
	{
		oConfigType.bArrayField = false;
	}

	char szDefaultValue[128] = {0};
	nRet = pElement->QueryValueAttribute("default", &szDefaultValue);
	if(nRet == TIXML_SUCCESS)
	{
		oConfigType.bHasDefault = true;
		if(oConfigType.eType == MSG_FIELD_INT)
		{
			oConfigType.nDefaultValue = atoi(szDefaultValue);
		}
		else 
		{
			oConfigType.strDefaultValue = szDefaultValue;
		}
		
	}
	else 
	{
		oConfigType.bHasDefault = false;
		oConfigType.nDefaultValue = 0;
	}

	CVS_XLOG(XLOG_TRACE, "CServiceConfig::GetTypeAttr name[%s] type[%d] code[%d] default[%d] \n",
		oConfigType.strName.c_str(), oConfigType.eType, oConfigType.nCode,  oConfigType.bHasDefault);
	return 0;
	
}


int CServiceConfig::GetFieldAttr_(const TiXmlElement* pElement, SConfigField& oConfigField,  bool bStruct)
{
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::GetFieldAttr\n");
	char szName[64] = {0};
	int nRet = pElement->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "field node get name attr failed\n");
    	return -1;
	}
	oConfigField.strName = szName;
	oConfigField.strOriName = szName;
	transform (oConfigField.strName.begin(), oConfigField.strName.end(), oConfigField.strName.begin(), (int(*)(int))tolower);

	nRet = pElement->QueryValueAttribute("type", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "field node get type attr failed\n");
    	return -1;
	}
	oConfigField.bStruct = bStruct;
	if(oConfigField.bStruct) 
	{
		oConfigField.eStructFieldType = m_mapETypeByName[szName];
		nRet = pElement->QueryIntAttribute("len", &(oConfigField.nLen));
		if(nRet != TIXML_SUCCESS)
		{
			oConfigField.nLen = -1;
		}

		char szDefaultValue[128] = {0};
		nRet = pElement->QueryValueAttribute("default", &szDefaultValue);
		if(nRet == TIXML_SUCCESS)
		{
			oConfigField.bHasDefault = true;
			if(oConfigField.eStructFieldType == MSG_FIELD_INT)
			{
				oConfigField.nDefaultValue = atoi(szDefaultValue);
			}
			else 
			{
				oConfigField.strDefaultValue = szDefaultValue;
			}
		}
		else 
		{
			oConfigField.bHasDefault = false;
		}
	}
	else 
	{
		oConfigField.strTypeName = szName;
		transform (oConfigField.strTypeName.begin(), oConfigField.strTypeName.end(), oConfigField.strTypeName.begin(), (int(*)(int))tolower);
	}

	nRet = pElement->QueryIntAttribute("maxlength", &(oConfigField.nMaxLen));
	if(nRet != TIXML_SUCCESS)
	{
		oConfigField.nMaxLen = -1;
	}
	nRet = pElement->QueryIntAttribute("minlength", &(oConfigField.nMinLen));
	if(nRet != TIXML_SUCCESS)
	{
		oConfigField.nMinLen = -1;
	}
	
	char szRequest[16]= {0};
	nRet = pElement->QueryValueAttribute("required", &szRequest);
	if(nRet != TIXML_SUCCESS)
	{
		oConfigField.bRequested = false;
	}
	else 
	{
		string strRequest = szRequest;
		transform (strRequest.begin(), strRequest.end(), strRequest.begin(), (int(*)(int))tolower);
		if(strcmp(strRequest.c_str(), "true") == 0)
		{
			oConfigField.bRequested = true;
		}
		else 
		{
			oConfigField.bRequested = false;
		}
	}

	char szAutoSet[16]= {0};
	nRet = pElement->QueryValueAttribute("autoSet", &szAutoSet);
	if(nRet != TIXML_SUCCESS)
	{
		
		oConfigField.bAutoSet= false;
	}
	else 
	{

		string strAutoSet = szAutoSet;
		transform (strAutoSet.begin(), strAutoSet.end(), strAutoSet.begin(), (int(*)(int))tolower);
		if(strcmp(strAutoSet.c_str(), "true") == 0)
		{
			oConfigField.bAutoSet = true;
		}
		else 
		{
			oConfigField.bAutoSet = false;
		}
	}

	
	
	char szConfigField[64]= {0};
	nRet = pElement->QueryValueAttribute("configField", &szConfigField);
	if(nRet != TIXML_SUCCESS)
	{
		oConfigField.strConfigField = "";
	}
	else 
	{

		oConfigField.strConfigField = szConfigField;
		transform (oConfigField.strConfigField.begin(), oConfigField.strConfigField.end(), oConfigField.strConfigField.begin(), (int(*)(int))tolower);
	}

	
	char szRegex[64] = {0};
	nRet = pElement->QueryValueAttribute("validatorregex", &szRegex);
	if(nRet != TIXML_SUCCESS)
	{
		oConfigField.strRegex = "";
	}
	else 
	{
		oConfigField.strRegex = szRegex;
		transform (oConfigField.strRegex.begin(), oConfigField.strRegex.end(), oConfigField.strRegex.begin(), (int(*)(int))tolower);
	}

	CVS_XLOG(XLOG_TRACE, "CServiceConfig::GetFieldAttr name[%s] type[%s] \n",
		oConfigField.strName.c_str(), oConfigField.strTypeName.c_str());
	return 0;
}

int CServiceConfig::GetMsgAttr_(const TiXmlElement* pElement, SConfigMessage& oConfigMessage)
{
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::GetMsgAttr_\n");
	char szName[64] = {0};
	int nRet = pElement->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "message node get name attr failed\n");
    	return -1;
	}
	oConfigMessage.strName = szName;
	transform (oConfigMessage.strName.begin(), oConfigMessage.strName.end(), oConfigMessage.strName.begin(), (int(*)(int))tolower);
	
    nRet = pElement->QueryIntAttribute("id", (int *)&(oConfigMessage.dwId));
	if(nRet != TIXML_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "message node get id attr failed\n");
    	return -1;
	}

	char szIsAck[16] = {0};
	nRet = pElement->QueryValueAttribute("isAck", &szIsAck);
	if(nRet != TIXML_SUCCESS)
	{
		oConfigMessage.bAckMsg = false;
	}
	else 
	{
		string strIsAck = szIsAck;
		transform (strIsAck.begin(), strIsAck.end(), strIsAck.begin(), (int(*)(int))tolower);
		if(strcmp(strIsAck.c_str(), "true") == 0)
		{
			oConfigMessage.bAckMsg = true;
		}
		else 
		{
			oConfigMessage.bAckMsg = false;
		}
	}
	CVS_XLOG(XLOG_TRACE, "CServiceConfig::GetMsgAttr_ name[%s] id[%d] bAck[%d]\n",
		oConfigMessage.strName.c_str(), oConfigMessage.dwId, oConfigMessage.bAckMsg);
	return 0;
}



int CServiceConfig::GetTypeByName(const string &strName, SConfigType& oConfigType)
{
	CVS_XLOG(XLOG_DEBUG, "CServiceConfig::GetTypeByName name[%s]\n", strName.c_str());
	map<string, SConfigType>::iterator itr = m_mapTypeByName.find(strName);
	if(itr != m_mapTypeByName.end())
	{
		oConfigType = itr->second;
		return 0;
	}
	CVS_XLOG(XLOG_ERROR, "CServiceConfig::GetTypeByName name[%s] service[%d] failed\n", strName.c_str(), m_dwServiceId);
	return -1;
}

int CServiceConfig::GetTypeByCode(int nCode, SConfigType& oConfigType)
{
	CVS_XLOG(XLOG_DEBUG, "CServiceConfig::GetTypeByCode code[%d]\n", nCode);
	map<int, SConfigType>::iterator itr = m_mapTypeByCode.find(nCode);
	if(itr != m_mapTypeByCode.end())
	{
		oConfigType = itr->second;
		return 0;
	}
	CVS_XLOG(XLOG_ERROR, "CServiceConfig::GetTypeByCode code[%d] service[%d] failed\n", nCode, m_dwServiceId);
	return -1;
}

//message
int CServiceConfig::GetMessageTypeByName(const string &strName, SConfigMessage ** oMessageType)
{
	CVS_XLOG(XLOG_DEBUG, "CServiceConfig::GetMessageTypeByName name[%s]\n", strName.c_str());
	map<string, SConfigMessage>::iterator itr = m_mapMessageByName.find(strName);
	if(itr != m_mapMessageByName.end())
	{
		*oMessageType = &(itr->second);
		return 0;
	}
	CVS_XLOG(XLOG_ERROR, "CServiceConfig::GetMessageTypeByName name[%s] service[%d] failed\n", strName.c_str(), m_dwServiceId);
	return -1;
}
int CServiceConfig::GetMessageTypeById(unsigned int dwMessageId, SConfigMessage ** oMessageType)
{
	CVS_XLOG(XLOG_DEBUG, "CServiceConfig::GetMessageTypeById code[%d]\n", dwMessageId);
	map<unsigned int, SConfigMessage>::iterator itr = m_mapMessageById.find(dwMessageId);
	if(itr != m_mapMessageById.end())
	{
		*oMessageType = &(itr->second);
		return 0;
	}
	CVS_XLOG(XLOG_ERROR, "CServiceConfig::GetMessageTypeById id[%d] service[%d] failed\n", dwMessageId, m_dwServiceId);
	return -1;
}


int CServiceConfig::GetTypeNameByFieldName(const string &strPreviousTypeName, const string & strFieldName, string & strTypeName, bool bFirst)
{
	CVS_XLOG(XLOG_DEBUG, "CServiceConfig::GetTypeNameByFieldName PreviousTypeName[%s] fieldname[%s] len[%d]\n", 
				strPreviousTypeName.c_str(),strFieldName.c_str(), strFieldName.length());
	SConfigType oConfigParentType;
	string strParentTypeName;
	if(GetTypeByName(strPreviousTypeName, oConfigParentType) < 0)
	{
		CVS_XLOG(XLOG_ERROR, "get ParentType failed\n");
		return -1;
	}
	if(bFirst == false && oConfigParentType.eType == MSG_FIELD_ARRAY)
	{
		strParentTypeName = oConfigParentType.strItemType;
	}
	else 
	{
		strParentTypeName = oConfigParentType.strName;
	}
	
	CVS_XLOG(XLOG_DEBUG, "CServiceConfig::GetTypeNameByFieldName ParentTypeName[%s]\n", strParentTypeName.c_str());
	std::pair<multimap<string, SConfigTypePair>::iterator, multimap<string, SConfigTypePair>::iterator> mapPair = m_mapTypePairByFieldName.equal_range(strFieldName);
	multimap<string, SConfigTypePair>::iterator itr;
	for(itr = mapPair.first; itr != mapPair.second; itr++) 
	{
		SConfigTypePair & oConfigTypePair = itr->second;
		
		if(strParentTypeName == oConfigTypePair.strPreviousTypeName) 
		{
			strTypeName = oConfigTypePair.strCurrentTypeName;
			return 0;
		}
	}
	return -1;
}


void CServiceConfig::Dump() const
{
	CVS_XLOG(XLOG_NOTICE, "############begin dump############\n");

	CVS_XLOG(XLOG_NOTICE, "mapTypeByName,size[%d]\n",m_mapTypeByName.size());
	CVS_XLOG(XLOG_NOTICE, "mapTypeByCode,size[%d]\n",m_mapTypeByCode.size());
	map<string, SConfigType>::const_iterator itrType;
	for(itrType = m_mapTypeByName.begin(); itrType != m_mapTypeByName.end(); itrType++)
	{
		const SConfigType & oConfigType = itrType->second;
		CVS_XLOG(XLOG_DEBUG, "name[%s] code[%d] type[%d] itemType[%s] fieldSize[%d]\n", 
			oConfigType.strName.c_str(), oConfigType.nCode, oConfigType.eType, 
			oConfigType.strItemType.c_str(), oConfigType.mapFieldByType.size());
	}


	CVS_XLOG(XLOG_NOTICE, "mapMessageByName,size[%d]\n",m_mapMessageByName.size());
	CVS_XLOG(XLOG_NOTICE, "mapMessageById,size[%d]\n",m_mapMessageById.size());
	map<string, SConfigMessage>::const_iterator itrMessage;
	for(itrMessage = m_mapMessageByName.begin(); itrMessage != m_mapMessageByName.end(); itrMessage++)
	{
		const SConfigMessage & oConfigMessage = itrMessage->second;
		CVS_XLOG(XLOG_DEBUG, "name[%s] id[%d] request[%d] res[%d] array[%d]\n", 
			oConfigMessage.strName.c_str(), oConfigMessage.dwId, 
			oConfigMessage.oRequestType.mapFieldByName.size(), oConfigMessage.oResponseType.mapFieldByType.size(),
			oConfigMessage.mapArraryTypeNameByCode.size());
	}


	CVS_XLOG(XLOG_NOTICE, "mapTypeNameByFieldName,size[%d]\n",m_mapTypePairByFieldName.size());
	multimap<string, SConfigTypePair>::const_iterator itrMap;
	for(itrMap = m_mapTypePairByFieldName.begin(); itrMap != m_mapTypePairByFieldName.end(); itrMap++)
	{
		const string & strFieldName = itrMap->first;
		const SConfigTypePair & oConfigTypePair = itrMap->second;
		CVS_XLOG(XLOG_DEBUG, "fieldname[%s] fieldlen[%d] typename[%s] pretypename[%s]\n", 
			strFieldName.c_str(), strFieldName.length(), oConfigTypePair.strCurrentTypeName.c_str(), oConfigTypePair.strPreviousTypeName.c_str());
	}
	CVS_XLOG(XLOG_NOTICE, "############end dump############\n");
}

bool CServiceConfig::IsExist( unsigned int dwMsgId )
{
	map<unsigned int, SConfigMessage>::iterator iter = m_mapMessageById.find(dwMsgId);
	if(iter == m_mapMessageById.end())
	{
		return false;
	}

	return true;
}



