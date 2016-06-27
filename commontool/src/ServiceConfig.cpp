#include "ServiceConfig.h"
#include "SdkLogHelper.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <arpa/inet.h>
using namespace sdo::util;
using namespace sdo::common;
using std::make_pair;


namespace sdo{
namespace commonsdk{

CServiceConfig::CServiceConfig():m_bIsTreeStruct(true),m_bMutilService(false)
{
	m_mapETypeByName["int"] = MSG_FIELD_INT;
	m_mapETypeByName["string"] = MSG_FIELD_STRING;
	m_mapETypeByName["array"] = MSG_FIELD_ARRAY;
	m_mapETypeByName["tlv"] = MSG_FIELD_TLV;
	m_mapETypeByName["struct"] = MSG_FIELD_STRUCT;


	m_mapValidatorFuncName["required"]=CValidator::require_validate;
	m_mapValidatorFuncName["regex"]=CValidator::regex_validate;
	m_mapValidatorFuncName["email"]=CValidator::email_validate;
	m_mapValidatorFuncName["url"]=CValidator::url_validate;
	m_mapValidatorFuncName["numberset"]=CValidator::numberset_validate;
	m_mapValidatorFuncName["numberrange"]=CValidator::number_validate;
	m_mapValidatorFuncName["lengthrange"]=CValidator::stringlength_validate;
	m_mapValidatorFuncName["timerange"]=CValidator::timerange_validate;

	m_mapValidatorFuncName["regex_r"]=CValidator::regex_validate;
	m_mapValidatorFuncName["email_r"]=CValidator::email_validate;
	m_mapValidatorFuncName["url_r"]=CValidator::url_validate;
	m_mapValidatorFuncName["numberset_r"]=CValidator::numberset_validate;
	m_mapValidatorFuncName["numberrange_r"]=CValidator::number_validate;
	m_mapValidatorFuncName["lengthrange_r"]=CValidator::stringlength_validate;
	m_mapValidatorFuncName["timerange_r"]=CValidator::timerange_validate;


	m_mapEncoderFuncName["normalencoder"]=CValueEncoder::normal_encode;
	m_mapEncoderFuncName["htmlencoder"]=CValueEncoder::html_encode;
	m_mapEncoderFuncName["htmlfilter"]=CValueEncoder::html_filter;
	m_mapEncoderFuncName["nocaseencoder"]=CValueEncoder::nocase_encode;
	m_mapEncoderFuncName["attackfilter"]=CValueEncoder::attack_filter;
	m_mapEncoderFuncName["escapeencoder"]=CValueEncoder::escape_encode;
	m_mapEncoderFuncName["sbc2dbcutf8"]=CValueEncoder::sbc2dbc_utf8;
	m_mapEncoderFuncName["sbc2dbcgbk"]=CValueEncoder::sbc2dbc_gbk;
}

int	CServiceConfig::LoadServiceConfig(const string &strConfig)
{
    TiXmlDocument m_xmlDoc;
    TiXmlElement *pService;
    if(!m_xmlDoc.LoadFile(strConfig.c_str()))
    {
       	SDK_XLOG(XLOG_ERROR, "xml load file[%s] failed\n", strConfig.c_str());
        return -1;
    }
	
    if((pService=m_xmlDoc.RootElement())==NULL)
    {
        SDK_XLOG(XLOG_ERROR, "xml no root node\n");
        return -1;
    }

	
	//service
	if((pService->QueryValueAttribute("name", &m_strServiceName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "serice node get name attr failed\n");
    	return -1;
	}
	transform (m_strServiceName.begin(), m_strServiceName.end(), m_strServiceName.begin(), (int(*)(int))tolower);

	char szServiceId[256]={0};
	if((pService->QueryValueAttribute("id", &szServiceId)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "serice node get id attr failed\n");
    	return -1;
	}
	vector<string> vecFields;
	boost::algorithm::split( vecFields,szServiceId,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on); 
	for(vector<string>::iterator itrField=vecFields.begin(); itrField!=vecFields.end();++itrField)
	{
	    string & strTmp=*itrField;
	    int nKey=atoi(strTmp.c_str());
	    if(nKey!=0)
	    {
	        m_vecServiceId.push_back(nKey);
	    }
	}        
	if(m_vecServiceId.size()>0)
		m_dwServiceId = m_vecServiceId[0];
	if(m_vecServiceId.size()>1)
		m_bMutilService = true;

	m_strServiceEncoding = "utf-8";
	pService->QueryValueAttribute("serviceencoding", &m_strServiceEncoding);
	transform (m_strServiceEncoding.begin(), m_strServiceEncoding.end(), m_strServiceEncoding.begin(), (int(*)(int))tolower);

	m_strSdkEncoding = "utf-8";
	pService->QueryValueAttribute("sdkencoding", &m_strSdkEncoding);
	transform (m_strSdkEncoding.begin(), m_strSdkEncoding.end(), m_strSdkEncoding.begin(), (int(*)(int))tolower);
	
	m_bIsTreeStruct = false;
	string strIsTreeStruct="false";
	if((pService->QueryValueAttribute("istreestruct", &strIsTreeStruct)) == TIXML_SUCCESS)
	{
		transform (strIsTreeStruct.begin(), strIsTreeStruct.end(), strIsTreeStruct.begin(), (int(*)(int))tolower);
		if(strcmp(strIsTreeStruct.c_str(), "true") == 0)
		{
			m_bIsTreeStruct = true;
		}
	}
//get inEncoder
	if(pService->QueryValueAttribute("inencoder", &(m_inEncoder.strEncoderName))== TIXML_SUCCESS)
	{
		transform (m_inEncoder.strEncoderName.begin(),m_inEncoder.strEncoderName.end(), m_inEncoder.strEncoderName.begin(), (int(*)(int))tolower);
	}
	if(m_mapEncoderFuncName.find(m_inEncoder.strEncoderName)==m_mapEncoderFuncName.end())
	{
		m_inEncoder.strEncoderName="";
		m_inEncoder.funcEncoder=NULL;
	}
	else
	{
		m_inEncoder.funcEncoder=m_mapEncoderFuncName[m_inEncoder.strEncoderName];
	}
	if(pService->QueryValueAttribute("inencoderparam", &(m_inEncoder.strEncoderParam))== TIXML_SUCCESS)
	{
		transform (m_inEncoder.strEncoderParam.begin(),m_inEncoder.strEncoderParam.end(), m_inEncoder.strEncoderParam.begin(), (int(*)(int))tolower);
	}

	vector<string> vecInEncoderParams;
	boost::algorithm::split( vecInEncoderParams, m_inEncoder.strEncoderParam, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_on); 
	
	for(vector<string>::iterator itr=vecInEncoderParams.begin();itr!=vecInEncoderParams.end();itr++)
	{
		string strItr=*itr;
		vector<string> vecItr;
		boost::algorithm::split( vecItr, strItr, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
		if(vecItr.size()>=2)
		{
			m_inEncoder.mapEncoderParam[vecItr[0]]=vecItr[1];
		}
	}
//get outEncoder
	if(pService->QueryValueAttribute("outencoder", &(m_outEncoder.strEncoderName))== TIXML_SUCCESS)
	{
		transform (m_outEncoder.strEncoderName.begin(),m_outEncoder.strEncoderName.end(), m_outEncoder.strEncoderName.begin(), (int(*)(int))tolower);
	}
	if(m_mapEncoderFuncName.find(m_outEncoder.strEncoderName)==m_mapEncoderFuncName.end())
	{
		m_outEncoder.strEncoderName="";
		m_outEncoder.funcEncoder=NULL;
	}
	else
	{
		m_outEncoder.funcEncoder=m_mapEncoderFuncName[m_outEncoder.strEncoderName];
	}
	if(pService->QueryValueAttribute("outencoderparam", &(m_outEncoder.strEncoderParam))== TIXML_SUCCESS)
	{
		transform (m_outEncoder.strEncoderParam.begin(),m_outEncoder.strEncoderParam.end(), m_outEncoder.strEncoderParam.begin(), (int(*)(int))tolower);
	}

	vector<string> vecOutEncoderParams;
	boost::algorithm::split( vecOutEncoderParams, m_outEncoder.strEncoderParam, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_on); 
	
	for(vector<string>::iterator itr=vecOutEncoderParams.begin();itr!=vecOutEncoderParams.end();itr++)
	{
		string strItr=*itr;
		vector<string> vecItr;
		boost::algorithm::split( vecItr, strItr, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
		if(vecItr.size()>=2)
		{
			m_inEncoder.mapEncoderParam[vecItr[0]]=vecItr[1];
		}
	}
	
	//type
	TiXmlElement * pConfigType = NULL;
	if((pConfigType=pService->FirstChildElement("type"))==NULL)
	{
		SDK_XLOG(XLOG_ERROR, "xml have no type node\n");
		return -1; 
	}
	else
	{	
		if(LoadConfigTypeConfig_(pConfigType) != 0)
		{
			SDK_XLOG(XLOG_ERROR, "LoadConfigTypeConfig_ failed\n");
			return -1; 
		}
	}
	while((pConfigType=pConfigType->NextSiblingElement("type"))!=NULL)
	{
		if(LoadConfigTypeConfig_(pConfigType) != 0)
		{
			SDK_XLOG(XLOG_ERROR, "LoadConfigTypeConfig_ failed\n");
			return -1; 
		}
	}
	//when array, get subItemType
	Qmap<string, SAvenueTlvTypeConfig>::iterator itr,itr2,itrTmp;
	for(itr=m_mapTypeByName.begin();itr!=m_mapTypeByName.end();)
	{
		itrTmp=itr++;
		SAvenueTlvTypeConfig& oConfig=itrTmp->second;
		if(oConfig.isArray)
		{
			itr2=m_mapTypeByName.find(oConfig.strItemType);
			if(itr2!=m_mapTypeByName.end())
			{
				SAvenueTlvTypeConfig& oConfig2=itr2->second;
				oConfig.nType=oConfig2.nType;
				oConfig.emType=oConfig2.emType;
				oConfig.oFeature=oConfig2.oFeature;
				oConfig.mapStructConfig=oConfig2.mapStructConfig;
				oConfig.vecStructName=oConfig2.vecStructName;
				oConfig.nStructAllLen=oConfig2.nStructAllLen;
				oConfig.vecTlvConfig=oConfig2.vecTlvConfig;
				oConfig.pTlvRule=oConfig2.pTlvRule;
			}
			else
			{
				m_mapTypeByName.erase(itrTmp);
				SDK_XLOG(XLOG_ERROR, "LoadConfigTypeConfig_, erase [%s].  Array's  childItem noexist\n",oConfig.strTypeName.c_str());
			}
		}
	}
	//merger service encoder to type
	if(m_inEncoder.funcEncoder!=NULL)
	{
		for(itr=m_mapTypeByName.begin();itr!=m_mapTypeByName.end();)
		{
			itrTmp=itr++;
			SAvenueTlvTypeConfig& oConfig=itrTmp->second;
			if(oConfig.emType==MSG_FIELD_STRING&&oConfig.oFeature.funcEncoder==NULL)
			{
				oConfig.oFeature.strEncoderName=m_inEncoder.strEncoderName;
				oConfig.oFeature.funcEncoder=m_inEncoder.funcEncoder;
				oConfig.oFeature.strEncoderParam=m_inEncoder.strEncoderParam;
				oConfig.oFeature.mapEncoderParam=m_inEncoder.mapEncoderParam;
			}
			if(oConfig.emType==MSG_FIELD_STRUCT)
			{
				map<string,SAvenueStructConfig>::iterator itrStruct;
				for(itrStruct=oConfig.mapStructConfig.begin();itrStruct!=oConfig.mapStructConfig.end();++itrStruct)
				{
					SAvenueStructConfig& oStruct=itrStruct->second;
					if(oStruct.emStructType==MSG_FIELD_STRING&&oStruct.oFeature.funcEncoder==NULL)
					{
						oStruct.oFeature.strEncoderName=m_inEncoder.strEncoderName;
						oStruct.oFeature.funcEncoder=m_inEncoder.funcEncoder;
						oStruct.oFeature.strEncoderParam=m_inEncoder.strEncoderParam;
						oStruct.oFeature.mapEncoderParam=m_inEncoder.mapEncoderParam;

						oConfig.oFeature.strEncoderName=m_inEncoder.strEncoderName;
						oConfig.oFeature.funcEncoder=m_inEncoder.funcEncoder;
						oConfig.oFeature.strEncoderParam=m_inEncoder.strEncoderParam;
						oConfig.oFeature.mapEncoderParam=m_inEncoder.mapEncoderParam;
					}
				}
			}
		}
	}

	//message
	TiXmlElement * pConfigMsg = NULL;
	if((pConfigMsg=pService->FirstChildElement("message"))==NULL)
	{
		SDK_XLOG(XLOG_ERROR, "xml have no message node\n");
		return -1; 
	}
	else
	{	
		if(LoadConfigMsgConfig_(pConfigMsg) != 0)
		{
			SDK_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ failed\n");
			return -1; 
		}
	}
	while((pConfigMsg=pConfigMsg->NextSiblingElement("message"))!=NULL)
	{
		if(LoadConfigMsgConfig_(pConfigMsg) != 0)
		{
			SDK_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ failed\n");
			return -1; 
		}

	}
	//type: create nest tlv rule & create id_map
	Qmap<string, SAvenueTlvTypeConfig>::iterator itrType,itrType2;
	for(itrType=m_mapTypeByName.begin();itrType!=m_mapTypeByName.end();itrType++)
	{
		SAvenueTlvTypeConfig & oConfig=itrType->second;
		//CreateTlvRule(oConfig.vecTlvConfig,oConfig.pTlvRule);
		if(oConfig.isArray==false)
		{
			m_mapTypeByCode[oConfig.nType] = oConfig;
		}
	}
	//message: create nest tlv rule & create id_map
	Qmap<string, SAvenueMessageConfig>::iterator itrMessage;
	for(itrMessage=m_mapMessageByName.begin();itrMessage!=m_mapMessageByName.end();itrMessage++)
	{
		SAvenueMessageConfig & oConfig=itrMessage->second;
		CreateTlvRule(oConfig.vecRequest,m_inEncoder,oConfig.oRequestTlvRule);
		CreateTlvRule(oConfig.vecResponse,m_outEncoder,oConfig.oResponseTlvRule);
	}

	//create id map
	for(itrMessage=m_mapMessageByName.begin();itrMessage!=m_mapMessageByName.end();itrMessage++)
	{
		SAvenueMessageConfig & oConfig=itrMessage->second;
		m_mapMessageById[oConfig.dwId] = oConfig;
	}
	if(IsEnableLevel(SDK_MODULE,XLOG_TRACE))
	{
		Dump();
	}
	return 0;
}
void CServiceConfig::CreateTlvRule(const vector<SAvenueTlvFieldConfig> & vecTlvConfig, const SServiceEncoderConfig & oServiceEncoder,SAvenueTlvGroupRule& oTlvRule)
{
	SDK_XLOG(XLOG_DEBUG, "CServiceConfig::%s\n",__FUNCTION__);
	oTlvRule.nReturnMsgType=0;
	vector<SAvenueTlvFieldConfig>::const_iterator itrTlvConfig;
	for(itrTlvConfig=vecTlvConfig.begin();itrTlvConfig!=vecTlvConfig.end();itrTlvConfig++)
	{
		const SAvenueTlvFieldConfig& oTlvField=*itrTlvConfig;
		Qmap<string, SAvenueTlvTypeConfig>::iterator itrType=m_mapTypeByName.find(oTlvField.strTypeName);
		if(itrType!=m_mapTypeByName.end())
		{
			SAvenueTlvTypeConfig oTypeConfig=itrType->second;
			MergeValueFeatueConfig(oTypeConfig.oFeature,oTlvField.oFeature);
			if(oTypeConfig.emType==MSG_FIELD_STRING)
			{
				MergeEncoderFeatueFromService(oTypeConfig.oFeature,oServiceEncoder);
			}
			oTlvRule.mapTlvType[oTlvField.strName]=oTypeConfig;
			oTlvRule.mapIdTlvType[oTypeConfig.nType]=oTypeConfig;
			if(oTypeConfig.emType==MSG_FIELD_STRING&&
				(oTlvField.strName=="returnmsg"||
				oTlvField.strName=="returnmessage"||
				oTlvField.strName=="return_msg"||
				oTlvField.strName=="return_message"||
				oTlvField.strName=="failreason"||
				oTlvField.strName=="fail_reason"))
			{
				oTlvRule.nReturnMsgType=oTypeConfig.nType;
			}
			SDK_XLOG(XLOG_DEBUG, "CServiceConfig::%s line[%d] oTlvField.strName[%s] [%p]\n",__FUNCTION__,__LINE__,oTlvField.strName.c_str(), oTypeConfig.oFeature.funcValidate);
			if(oTypeConfig.oFeature.funcValidate!=NULL)
			{
				oTlvRule.mapValidateField[oTypeConfig.nType]=oTypeConfig;
			}
			if(oTypeConfig.oFeature.funcValidate==CValidator::require_validate||
				boost::ends_with(oTypeConfig.oFeature.strValidatorName,"_r"))
			{
				oTlvRule.mapRequired[oTypeConfig.nType]=oTypeConfig.oFeature.nReturnCodeIfValidateFail;
				oTlvRule.mapRequiredReturnMsg[oTypeConfig.nType]=oTypeConfig.oFeature.strReturnMsg;
			}
			if(oTypeConfig.oFeature.isHasDefault&&oTypeConfig.emType==MSG_FIELD_STRING)
			{
				oTlvRule.mapStringDefault[oTypeConfig.nType]=oTypeConfig.oFeature.strDefaultValue;
			}
			if(oTypeConfig.oFeature.isHasDefault&&oTypeConfig.emType==MSG_FIELD_INT)
			{
				oTlvRule.mapIntDefault[oTypeConfig.nType]=oTypeConfig.oFeature.nDefaultValue;
			}
		}
	}
}
void CServiceConfig::MergeEncoderFeatueFromService(SValueFeatueConfig& value1,  const SServiceEncoderConfig & oServiceEncoder)
{
	if(value1.funcEncoder==NULL && oServiceEncoder.funcEncoder!=NULL) 
	{
		value1.strEncoderName=oServiceEncoder.strEncoderName;
		value1.strEncoderParam=oServiceEncoder.strEncoderParam;
		value1.funcEncoder=oServiceEncoder.funcEncoder;
		value1.mapEncoderParam=oServiceEncoder.mapEncoderParam;
	}
}
void CServiceConfig::MergeValueFeatueConfig(SValueFeatueConfig& value1, const SValueFeatueConfig & value2)
{
	if(value2.isHasDefault)
	{
		value1.isHasDefault=value2.isHasDefault;
		value1.strDefaultValue=value2.strDefaultValue;
		value1.nDefaultValue=value2.nDefaultValue;
	}
	
	if(value2.strEncoderName!="") 
	{
		value1.strEncoderName=value2.strEncoderName;
		value1.strEncoderParam=value2.strEncoderParam;
		value1.funcEncoder=value2.funcEncoder;
		value1.mapEncoderParam=value2.mapEncoderParam;
	}
		
	if(value2.strValidatorName!="") 
	{
		value1.strValidatorName=value2.strValidatorName;
		value1.strValidatorParam=value2.strValidatorParam;
		value1.nReturnCodeIfValidateFail=value2.nReturnCodeIfValidateFail;
		value1.strReturnMsg=value2.strReturnMsg;
		value1.funcValidate=value2.funcValidate;
	}
}
int CServiceConfig::LoadConfigTypeConfig_(TiXmlElement * pConfigType)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::LoadConfigTypeConfig_...\n");
	SAvenueTlvTypeConfig oConfigType;
	
	if(GetTypeAttr_(pConfigType, oConfigType) < 0)
	{
		SDK_XLOG(XLOG_ERROR, "xml GetTypeAttr failed\n");
    	return -1;
	}
	if(oConfigType.emType == MSG_FIELD_STRUCT)
	{
		TiXmlElement * pConfigField = NULL;
		oConfigType.nStructAllLen=0;
		int nStructItemLoc=0;
		FuncValidator pStructFunc=NULL;
		FuncValueEncoder pStructEncoderFunc=NULL;
		while(NULL!=(pConfigField =(TiXmlElement *) pConfigType->IterateChildren(pConfigField)))
		{
			SAvenueStructConfig oStructConfig;
			if(GetStructFieldAttr_(pConfigField, oStructConfig,&nStructItemLoc) < 0)
			{
				SDK_XLOG(XLOG_ERROR, "xml GetFieldAttr struct failed\n");
				return -1;
			}
			if(oStructConfig.oFeature.funcValidate!=NULL)
			{
				pStructFunc=oStructConfig.oFeature.funcValidate;
			}
			if(oStructConfig.oFeature.funcEncoder!=NULL)
			{
				pStructEncoderFunc=oStructConfig.oFeature.funcEncoder;
			}
			oConfigType.mapStructConfig[oStructConfig.strName]=oStructConfig;
			oConfigType.nStructAllLen+=(oStructConfig.nLen==-1?AVENUE_STRUCT_LASTSTRING_DEFAULT_SIZE:oStructConfig.nLen);
			oConfigType.vecStructName.push_back(oStructConfig.strName);
			SDK_XLOG(XLOG_TRACE, "CServiceConfig::LoadConfigTypeConfig_  vecStructName %s\n", oStructConfig.strName.c_str());
		}
		oConfigType.nStructAllLen=((oConfigType.nStructAllLen&0x03)!=0?((oConfigType.nStructAllLen>>2)+1)<<2:oConfigType.nStructAllLen);
		oConfigType.oFeature.funcValidate=pStructFunc;
		oConfigType.oFeature.funcEncoder=pStructEncoderFunc;
	}
	if(oConfigType.emType == MSG_FIELD_TLV)
	{
		TiXmlElement * pConfigField = NULL;
		while(NULL!=(pConfigField =(TiXmlElement *) pConfigType->IterateChildren(pConfigField)))
		{
			SAvenueTlvFieldConfig oTlvConfig;
			if(GetTlvFieldAttr_(pConfigField, oTlvConfig) < 0)
			{
				SDK_XLOG(XLOG_ERROR, "xml GetFieldAttr tlv failed\n");
				return -1;
			}
			oConfigType.vecTlvConfig.push_back(oTlvConfig);
			oConfigType.pTlvRule=NULL;
		}
	}
	
	m_mapTypeByName[oConfigType.strTypeName] = oConfigType;
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::LoadConfigTypeConfig_ type[%s] success!\n",oConfigType.strTypeName.c_str());
	return 0;
}

int CServiceConfig::GetTypeAttr_(const TiXmlElement * pElement, SAvenueTlvTypeConfig & oConfigType)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetTypeAttr\n");
	if((pElement->QueryValueAttribute("name", &oConfigType.strTypeName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "type node get name attr failed\n");
    		return -1;
	}
	transform (oConfigType.strTypeName.begin(), oConfigType.strTypeName.end(), oConfigType.strTypeName.begin(), (int(*)(int))tolower);

	char szClass[64] = {0};
	if((pElement->QueryValueAttribute("class", &szClass)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "type node get class attr failed\n");
    		return -1;
	}
	string strClass = szClass;
	transform (strClass.begin(), strClass.end(), strClass.begin(), (int(*)(int))tolower);
	if(m_mapETypeByName.find(strClass)==m_mapETypeByName.end())
	{
		SDK_XLOG(XLOG_ERROR, "type node get class uncogonized\n");
		return -1;
	}
	oConfigType.emType = m_mapETypeByName[strClass];

	oConfigType.isArray=false;
	if(oConfigType.emType == MSG_FIELD_ARRAY)
	{
		oConfigType.isArray=true;
		if((pElement->QueryValueAttribute("itemtype", &oConfigType.strItemType)) != TIXML_SUCCESS)
		{
			SDK_XLOG(XLOG_ERROR, "type node get itemType attr failed\n");
	    		return -1;
		}
		transform (oConfigType.strItemType.begin(), oConfigType.strItemType.end(), oConfigType.strItemType.begin(), (int(*)(int))tolower);
	}

	oConfigType.nType=0;
	if((pElement->QueryIntAttribute("code", &(oConfigType.nType))) != TIXML_SUCCESS 
		&& oConfigType.emType != MSG_FIELD_ARRAY)
	{
		SDK_XLOG(XLOG_ERROR, "type node get code attr failed\n");
    		return -1;
	}

	if(oConfigType.emType == MSG_FIELD_INT||oConfigType.emType == MSG_FIELD_STRING)
	{

		GetValueFeatureConfig_(pElement,oConfigType.oFeature);
	}


	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetTypeAttr name[%s] type[%d] code[%d] default[%s] \n",
		oConfigType.strTypeName.c_str(), oConfigType.emType, oConfigType.nType,  oConfigType.oFeature.strDefaultValue.c_str());
	return 0;
	
}
int CServiceConfig::GetStructFieldAttr_(const TiXmlElement* pElement, SAvenueStructConfig& oConfigField, int * pLoc)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetStructFieldAttr_...\n");
	if((pElement->QueryValueAttribute("name", &oConfigField.strName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "field node get name attr failed\n");
    	return -1;
	}
	transform (oConfigField.strName.begin(), oConfigField.strName.end(), oConfigField.strName.begin(), (int(*)(int))tolower);

	char szName[64]={0};
	if((pElement->QueryValueAttribute("type", &szName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "field node get type attr failed\n");
    	return -1;
	}

	if(strcmp(szName,"int")==0)
	{
		oConfigField.emStructType = MSG_FIELD_INT;
		oConfigField.nLoc=*pLoc;
		oConfigField.nLen=4;
		*pLoc+=oConfigField.nLen;
	}
	else
	{
		oConfigField.emStructType = MSG_FIELD_STRING;
		oConfigField.nLoc=*pLoc;
		if((pElement->QueryIntAttribute("len", (int *)&(oConfigField.nLen))) != TIXML_SUCCESS)
		{
			oConfigField.nLen = -1;
		}
		*pLoc+=oConfigField.nLen;
		
		if(strcmp(szName,"systemstring")==0)
		{
			oConfigField.bSystemString = true;
		}
	}
	GetValueFeatureConfig_(pElement,oConfigField.oFeature);
	return 0;
}


int CServiceConfig::LoadConfigMsgConfig_(TiXmlElement * pConfigMsg)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::LoadConfigMsgConfig_\n");
	SAvenueMessageConfig oConfigMessage;
	if(GetMsgAttr_(pConfigMsg, oConfigMessage) < 0)
	{
		SDK_XLOG(XLOG_ERROR, "xml GetTypeAttr failed\n");
    	return -1;
	}
	
	//request
	TiXmlElement * pConfigField = pConfigMsg->FirstChildElement("requestparameter");
	if(pConfigField == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ have no requestParameter node\n");
    	return -1;
	}
	pConfigField=pConfigField->FirstChildElement();
	while(pConfigField)
	{
		SAvenueTlvFieldConfig oConfigField;
		if(GetTlvFieldAttr_(pConfigField, oConfigField) < 0)
		{
			SDK_XLOG(XLOG_ERROR, "xml GetFieldAttr failed\n");
			return -1;
		}
		oConfigMessage.vecRequest.push_back(oConfigField);
		pConfigField = pConfigField->NextSiblingElement();
	}
	//response
	pConfigField= pConfigMsg->FirstChildElement("responseparameter");
	if(pConfigField == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "LoadConfigMsgConfig_ have no responseparameter node\n");
    	return -1;
	}
	pConfigField=pConfigField->FirstChildElement();
	while(pConfigField)
	{
		SAvenueTlvFieldConfig oConfigField;
		if(GetTlvFieldAttr_(pConfigField, oConfigField) < 0)
		{
			SDK_XLOG(XLOG_ERROR, "xml GetFieldAttr failed\n");
			return -1;
		}
		oConfigMessage.vecResponse.push_back(oConfigField);
		pConfigField = pConfigField->NextSiblingElement();
	}
	
	m_mapMessageByName[oConfigMessage.strName] = oConfigMessage;
	return 0;
}
int CServiceConfig::GetMsgAttr_(const TiXmlElement* pElement, SAvenueMessageConfig& oConfigMessage)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetMsgAttr_...\n");
	if((pElement->QueryValueAttribute("name", &oConfigMessage.strName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "message node get name attr failed\n");
    	return -1;
	}
	transform (oConfigMessage.strName.begin(), oConfigMessage.strName.end(), oConfigMessage.strName.begin(), (int(*)(int))tolower);
	
	if((pElement->QueryIntAttribute("id", (int *)&(oConfigMessage.dwId))) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "message node get id attr failed\n");
    		return -1;
	}

	char szIsAck[16] = {0};
	oConfigMessage.bAckMsg = false;
	if((pElement->QueryValueAttribute("isack", &szIsAck)) == TIXML_SUCCESS)
	{
		string strIsAck = szIsAck;
		transform (strIsAck.begin(), strIsAck.end(), strIsAck.begin(), (int(*)(int))tolower);
		if(strcmp(strIsAck.c_str(), "true") == 0)
		{
			oConfigMessage.bAckMsg = true;
		}
	}
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetMsgAttr_ name[%s] id[%d] bAck[%d]\n",
		oConfigMessage.strName.c_str(), oConfigMessage.dwId, oConfigMessage.bAckMsg);
	return 0;
}
int CServiceConfig::GetTlvFieldAttr_(const TiXmlElement* pElement, SAvenueTlvFieldConfig& oConfigField)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetTlvFieldAttr_...\n");
	if((pElement->QueryValueAttribute("name", &oConfigField.strName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "tlv field node get name attr failed\n");
    	return -1;
	}
	transform (oConfigField.strName.begin(), oConfigField.strName.end(), oConfigField.strName.begin(), (int(*)(int))tolower);

	if((pElement->QueryValueAttribute("type", &oConfigField.strTypeName)) != TIXML_SUCCESS)
	{
		SDK_XLOG(XLOG_ERROR, "tlv field node get type attr failed\n");
    	return -1;
	}
	transform (oConfigField.strTypeName.begin(), oConfigField.strTypeName.end(), oConfigField.strTypeName.begin(), (int(*)(int))tolower);

	GetValueFeatureConfig_(pElement,oConfigField.oFeature);
	
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetTlvFieldAttr_ name[%s] type[%s] \n",
		oConfigField.strName.c_str(), oConfigField.strTypeName.c_str());
	return 0;
}
void CServiceConfig::GetValueFeatureConfig_(const sdo::util::TiXmlElement* pElement, SValueFeatueConfig& oFeature)
{
	SDK_XLOG(XLOG_TRACE, "CServiceConfig::GetValueFeatureConfig_...\n");
	oFeature.isHasDefault=false;
	if(pElement->QueryValueAttribute("default", &(oFeature.strDefaultValue))== TIXML_SUCCESS)
	{
		oFeature.isHasDefault=true;
		oFeature.nDefaultValue=atoi(oFeature.strDefaultValue.c_str());
	}

	if(pElement->QueryValueAttribute("validator", &(oFeature.strValidatorName))== TIXML_SUCCESS)
	{
		transform (oFeature.strValidatorName.begin(),oFeature.strValidatorName.end(), oFeature.strValidatorName.begin(), (int(*)(int))tolower);
	}
	if(m_mapValidatorFuncName.find(oFeature.strValidatorName)==m_mapValidatorFuncName.end())
	{
		oFeature.strValidatorName="";
		oFeature.funcValidate=NULL;
	}
	else
	{
		oFeature.funcValidate=m_mapValidatorFuncName[oFeature.strValidatorName];
	}

	//if(oFeature.funcValidate==NULL)
	//{
	//	string strRequired;
	//	if((pElement->QueryValueAttribute("required", &strRequired)) == TIXML_SUCCESS)
	//	{
	//		if(strRequired==string("true"))
	//		{
			//	oFeature.strValidatorName="required";
			//	oFeature.funcValidate=m_mapValidatorFuncName[oFeature.strValidatorName];
	//		}
	//	}
	//}

	if(pElement->QueryValueAttribute("validatorparam", &(oFeature.strValidatorParam))== TIXML_SUCCESS)
	{
		transform (oFeature.strValidatorParam.begin(), oFeature.strValidatorParam.end(), oFeature.strValidatorParam.begin(), (int(*)(int))tolower);
	}

	oFeature.nReturnCodeIfValidateFail=-10242400;
	oFeature.strReturnMsg="请求参数格式非法";
	pElement->QueryValueAttribute("returncode", (int *)&(oFeature.nReturnCodeIfValidateFail));
	if(pElement->QueryValueAttribute("returnmsg", &(oFeature.strReturnMsg))!= TIXML_SUCCESS)
	{
		pElement->QueryValueAttribute("returnmessage", &(oFeature.strReturnMsg));
	}
	
	if(pElement->QueryValueAttribute("encoder", &(oFeature.strEncoderName))== TIXML_SUCCESS)
	{
		transform (oFeature.strEncoderName.begin(),oFeature.strEncoderName.end(), oFeature.strEncoderName.begin(), (int(*)(int))tolower);
	}
	if(m_mapEncoderFuncName.find(oFeature.strEncoderName)==m_mapEncoderFuncName.end())
	{
		oFeature.strEncoderName="";
		oFeature.funcEncoder=NULL;
	}
	else
	{
		oFeature.funcEncoder=m_mapEncoderFuncName[oFeature.strEncoderName];
	}
	if(pElement->QueryValueAttribute("encoderparam", &(oFeature.strEncoderParam))== TIXML_SUCCESS)
	{
		transform (oFeature.strEncoderParam.begin(),oFeature.strEncoderParam.end(), oFeature.strEncoderParam.begin(), (int(*)(int))tolower);
	}

	vector<string> vecParams;
	boost::algorithm::split( vecParams, oFeature.strEncoderParam, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_on); 
	
	for(vector<string>::iterator itr=vecParams.begin();itr!=vecParams.end();itr++)
	{
		string strItr=*itr;
		vector<string> vecItr;
		boost::algorithm::split( vecItr, strItr, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
		if(vecItr.size()>=2)
		{
			oFeature.mapEncoderParam[vecItr[0]]=vecItr[1];
		}
	}
}
//type
int CServiceConfig::GetTypeByName(const string &strName, SAvenueTlvTypeConfig** pConfigType)
{
	SDK_XLOG(XLOG_DEBUG, "CServiceConfig::GetTypeByName name[%s]\n", strName.c_str());
	Qmap<string, SAvenueTlvTypeConfig>::iterator itr = m_mapTypeByName.find(strName);
	if(itr != m_mapTypeByName.end())
	{
		*pConfigType = &(itr->second);
		return 0;
	}
	SDK_XLOG(XLOG_ERROR, "CServiceConfig::GetTypeByName name[%s] service[%d] failed\n", strName.c_str(), m_dwServiceId);
	return -1;
}

int CServiceConfig::GetTypeByCode(int nCode, SAvenueTlvTypeConfig** pConfigType)
{
	SDK_XLOG(XLOG_DEBUG, "CServiceConfig::GetTypeByCode code[%d]\n", nCode);
	Qmap<int, SAvenueTlvTypeConfig>::iterator itr = m_mapTypeByCode.find(nCode);
	if(itr != m_mapTypeByCode.end())
	{
		*pConfigType = &(itr->second);
		return 0;
	}
	SDK_XLOG(XLOG_ERROR, "CServiceConfig::GetTypeByCode code[%d] service[%d] failed\n", nCode, m_dwServiceId);
	return -1;
}

//message
int CServiceConfig::GetMessageTypeByName(char * szName, SAvenueMessageConfig ** oMessageType)
{
	SDK_XLOG(XLOG_DEBUG, "CServiceConfig::GetMessageTypeByName name[%s]\n", szName);
	Qmap<string, SAvenueMessageConfig>::iterator itr = m_mapMessageByName.find(szName);
	if(itr != m_mapMessageByName.end())
	{
		*oMessageType = &(itr->second);
		return 0;
	}
	SDK_XLOG(XLOG_ERROR, "CServiceConfig::GetMessageTypeByName name[%s] service[%d] failed\n", szName, m_dwServiceId);
	return -1;
}
int CServiceConfig::GetMessageTypeById(unsigned int dwMessageId, SAvenueMessageConfig ** oMessageType)
{
	SDK_XLOG(XLOG_DEBUG, "CServiceConfig::GetMessageTypeById code[%d]\n", dwMessageId);
	Qmap<unsigned int, SAvenueMessageConfig>::iterator itr = m_mapMessageById.find(dwMessageId);
	if(itr != m_mapMessageById.end())
	{
		*oMessageType = &(itr->second);
		return 0;
	}
	SDK_XLOG(XLOG_ERROR, "CServiceConfig::GetMessageTypeById id[%d] service[%d] failed\n", dwMessageId, m_dwServiceId);
	return -1;
}

void CServiceConfig::Dump() const
{
	SDK_XLOG(XLOG_NOTICE, "############begin dump############\n");

	DumpTypeMap("all type map: ",m_mapTypeByName);

	SDK_XLOG(XLOG_NOTICE, "mapMessageByName,size[%d],mapMessageById,size[%d]\n",m_mapMessageByName.size(),m_mapMessageById.size());
	Qmap<string, SAvenueMessageConfig>::const_iterator itrMessage;
	for(itrMessage = m_mapMessageByName.begin(); itrMessage != m_mapMessageByName.end(); itrMessage++)
	{
		const SAvenueMessageConfig & oConfigMessage = itrMessage->second;
		SDK_XLOG(XLOG_DEBUG, "name[%s] id[%d] bAck[%d]\n", 
			oConfigMessage.strName.c_str(), oConfigMessage.dwId, oConfigMessage.bAckMsg);
		DumpTypeMap("request map: ",oConfigMessage.oRequestTlvRule.mapTlvType);
		DumpTypeMap("response map: ",oConfigMessage.oRequestTlvRule.mapTlvType);
	}

	SDK_XLOG(XLOG_NOTICE, "############end dump############\n");
}

void CServiceConfig::DumpTypeMap(const string &title,const Qmap<string, SAvenueTlvTypeConfig> &mapType) const
{
	SDK_XLOG(XLOG_NOTICE, "	%s,size[%d]\n",title.c_str(),mapType.size());
	Qmap<string, SAvenueTlvTypeConfig>::const_iterator itrType;
	for(itrType = mapType.begin(); itrType != mapType.end(); itrType++)
	{
		const SAvenueTlvTypeConfig & oConfigType = itrType->second;
		SDK_XLOG(XLOG_DEBUG, "		name[%s] code[%d] type[%d]  isArray[%d:%s],default[%s],validator[%s:%s:%d],encod[%s:%s]\n", 
			oConfigType.strTypeName.c_str(), oConfigType.nType, oConfigType.emType, oConfigType.isArray, oConfigType.strItemType.c_str(),
			oConfigType.oFeature.strDefaultValue.c_str(),oConfigType.oFeature.strValidatorName.c_str(),
			oConfigType.oFeature.strValidatorParam.c_str(),oConfigType.oFeature.nReturnCodeIfValidateFail,
			oConfigType.oFeature.strEncoderName.c_str(),oConfigType.oFeature.strEncoderParam.c_str());
		if(oConfigType.emType==MSG_FIELD_STRUCT)
		{
			map<string,SAvenueStructConfig>::const_iterator itrStruct;
			for(itrStruct=oConfigType.mapStructConfig.begin();itrStruct!=oConfigType.mapStructConfig.end();itrStruct++)
			{
				const SAvenueStructConfig & oStruct=itrStruct->second;
				SDK_XLOG(XLOG_DEBUG, "			struct:name[%s] type[%d] len[%d] loc[%d],default[%s],validator[%s:%s:%d],encod[%s:%s]\n", 
			oStruct.strName.c_str(),  oStruct.emStructType, oStruct.nLen,oStruct.nLoc,
			oStruct.oFeature.strDefaultValue.c_str(),oStruct.oFeature.strValidatorName.c_str(),
			oStruct.oFeature.strValidatorParam.c_str(),oStruct.oFeature.nReturnCodeIfValidateFail,
			oStruct.oFeature.strEncoderName.c_str(),oStruct.oFeature.strEncoderParam.c_str());
			}
		}
		if(oConfigType.emType==MSG_FIELD_TLV)
		{
			//DumpTypeMap( "child tlv map",oConfigType.oTlvRule.mapTlvType);
		}
	}
	SDK_XLOG(XLOG_NOTICE, "	end %s,size[%d]\n",title.c_str(),mapType.size());
}
}
}



