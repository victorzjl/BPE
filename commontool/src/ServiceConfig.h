#ifndef _SERVICE_CONFIG_H_
#define _SERVICE_CONFIG_H_
#include "QStl.h"

#include <map>
#include <vector>
#include <string>
#include <set>
#include "tinyxml/ex_tinyxml.h"
#include "Validator.h"
#include "ValueEncoder.h"

#define AVENUE_STRUCT_LASTSTRING_DEFAULT_SIZE 64

using std::string;
using std::multimap;
using std::vector;
using std::set;
namespace sdo{
namespace commonsdk{

typedef enum
{
	MSG_FIELD_NULL=0,
	MSG_FIELD_INT=1,
	MSG_FIELD_STRING=2,
	MSG_FIELD_ARRAY=3,
	MSG_FIELD_TLV=4,
	MSG_FIELD_STRUCT=5,
	MSG_FIELD_META=6, //only used in compose def value
	MSG_FIELD_ALL
}EValueType;
typedef struct stServiceEncoderConfig
{
	string strEncoderName;
	FuncValueEncoder funcEncoder;
	string strEncoderParam;	
	map<string,string> mapEncoderParam;
}SServiceEncoderConfig;

typedef struct stValueFeatueConfig
{
	stValueFeatueConfig():isHasDefault(false),funcValidate(NULL),funcEncoder(NULL){}
	bool isHasDefault;
	string strDefaultValue;
	int nDefaultValue;

	string strValidatorName;
	FuncValidator funcValidate;
	string strValidatorParam;
	int nReturnCodeIfValidateFail;
	string strReturnMsg;

	string strEncoderName;
	FuncValueEncoder funcEncoder;
	string strEncoderParam;	
	map<string,string> mapEncoderParam;
}SValueFeatueConfig;
typedef struct stAvenueStructConfig
{
	stAvenueStructConfig():emStructType(MSG_FIELD_STRING),nLoc(0),nLen(0),bSystemString(false){}
	string strName;
	EValueType emStructType;
	unsigned int nLoc;	
	int nLen;
	SValueFeatueConfig oFeature;
	bool bSystemString;
	bool operator==(const stAvenueStructConfig &obj)
	{
		return this->strName==obj.strName &&
			this->emStructType==obj.emStructType &&
			this->nLoc==obj.nLoc &&
			this->nLen==obj.nLen;
	}
}SAvenueStructConfig;
typedef struct stAvenueTlvFieldConfig
{
	string strName;
	string strTypeName;
	SValueFeatueConfig oFeature;
}SAvenueTlvFieldConfig;

struct stAvenueTlvGroupRule;
typedef struct stAvenueTlvTypeConfig
{
	int nType;
	string strTypeName;
	EValueType	emType;
	bool isArray;
	string strItemType;
	SValueFeatueConfig oFeature;
	map<string,SAvenueStructConfig> mapStructConfig;
	vector<string> 	vecStructName;
	unsigned int nStructAllLen;

	vector<SAvenueTlvFieldConfig> vecTlvConfig;
	stAvenueTlvGroupRule  *pTlvRule;
	bool operator==(const stAvenueTlvTypeConfig &obj)
	{
		if(this->mapStructConfig.size()!=obj.mapStructConfig.size()) return false;

		map<string,SAvenueStructConfig>::iterator itr;
		map<string,SAvenueStructConfig>::const_iterator itr2;
		for(itr=this->mapStructConfig.begin();itr!=this->mapStructConfig.end();itr++)
		{
			const string & strName=itr->first;
			SAvenueStructConfig & oConfig=itr->second;
			if((itr2=obj.mapStructConfig.find(strName))==obj.mapStructConfig.end()) return false;
			const SAvenueStructConfig & oConfig2=itr2->second;
			if(!(oConfig==oConfig2)) return false;
		}
		return this->strTypeName==obj.strTypeName &&
			this->emType==obj.emType &&
			this->isArray==obj.isArray &&
			this->strItemType==obj.strItemType;
	}
}SAvenueTlvTypeConfig;

typedef struct stAvenueTlvGroupRule
{
	map<string, SAvenueTlvTypeConfig> mapTlvType;
	map<unsigned int, SAvenueTlvTypeConfig> mapIdTlvType;
	map<unsigned int, SAvenueTlvTypeConfig> mapValidateField;
	map<unsigned int,string> mapStringDefault;
	map<unsigned int,int> mapIntDefault;
	map<unsigned int,int> mapRequired;
	map<unsigned int ,string> mapRequiredReturnMsg;
	unsigned int nReturnMsgType;
}SAvenueTlvGroupRule;

//message 
typedef struct stAvenueMessageConfig
{
	string strName;
	unsigned int dwId;
	bool bAckMsg;
	vector<SAvenueTlvFieldConfig> vecRequest;
	vector<SAvenueTlvFieldConfig> vecResponse;

	SAvenueTlvGroupRule oRequestTlvRule;
	SAvenueTlvGroupRule oResponseTlvRule;
}SAvenueMessageConfig;


class CServiceConfig
{
public:
	CServiceConfig();
	~CServiceConfig(){}
	int	LoadServiceConfig(const string &strConfig);
	
	//type and field
	int GetTypeByName(const string &strName, SAvenueTlvTypeConfig **pConfigType);
	int GetTypeByCode(int nCode, SAvenueTlvTypeConfig **pConfigType);

	//message
	int GetMessageTypeByName(char *szName, SAvenueMessageConfig ** oMessageType);
	int GetMessageTypeById(unsigned int dwMessageId, SAvenueMessageConfig ** oMessageType);

	//
	const string & GetServiceName() {return m_strServiceName;}
	unsigned int GetServiceId() {return m_dwServiceId;}
	const string & GetServiceEncoding(){return m_strServiceEncoding;}
	const string & GetSdkEncoding(){return m_strSdkEncoding;}
	bool GetIsTreeStruct(){return m_bIsTreeStruct;}
	bool GetIsMutilService() {return m_bMutilService;}
	const vector<unsigned int>& GetMutilService() {return m_vecServiceId;}
	void Dump()const;
	void DumpTypeMap(const string &title,const Qmap<string, SAvenueTlvTypeConfig> &mapType)const;
	
private:
    int LoadConfigTypeConfig_(sdo::util::TiXmlElement * pConfigType);
	int GetTypeAttr_(const sdo::util::TiXmlElement* pElement, SAvenueTlvTypeConfig& oConfigType);
	int GetStructFieldAttr_(const sdo::util::TiXmlElement* pElement, SAvenueStructConfig& oConfigField, int * pLoc);

	int LoadConfigMsgConfig_(sdo::util::TiXmlElement * pConfigMsg);
	int GetMsgAttr_(const sdo::util::TiXmlElement* pElement, SAvenueMessageConfig& oConfigMessage);
	int GetTlvFieldAttr_(const sdo::util::TiXmlElement* pElement, SAvenueTlvFieldConfig& oConfigField);

	void GetValueFeatureConfig_(const sdo::util::TiXmlElement* pElement, SValueFeatueConfig& oFeature);
	void MergeValueFeatueConfig(SValueFeatueConfig& value1, const SValueFeatueConfig & value2);
	void CreateTlvRule(const vector<SAvenueTlvFieldConfig> & vecTlvConfig, const SServiceEncoderConfig & oServiceEncoder,SAvenueTlvGroupRule& oTlvRule);
	void MergeEncoderFeatueFromService(SValueFeatueConfig& value1,  const SServiceEncoderConfig & oServiceEncoder);

	

private:

	//type config
	Qmap<string, SAvenueTlvTypeConfig> m_mapTypeByName;
	Qmap<int, SAvenueTlvTypeConfig> m_mapTypeByCode;

	//message config
	Qmap<string, SAvenueMessageConfig> m_mapMessageByName;
	Qmap<unsigned int, SAvenueMessageConfig> m_mapMessageById;

	//define const map 
	Qmap<string, EValueType> m_mapETypeByName;
	map<string, FuncValidator> m_mapValidatorFuncName;
	map<string, FuncValueEncoder> m_mapEncoderFuncName;

	string m_strServiceName;
	unsigned int m_dwServiceId;
	bool m_bMutilService;
	vector<unsigned int> m_vecServiceId;
	string m_strServiceEncoding;
	string m_strSdkEncoding;
	bool m_bIsTreeStruct;

	SServiceEncoderConfig m_inEncoder;
	SServiceEncoderConfig m_outEncoder;
	
};
}
}
#endif


