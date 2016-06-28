#ifndef _SERVICE_CONFIG_H_
#define _SERVICE_CONFIG_H_

#include <map>
#include <vector>
#include <string>
#include "tinyxml/tinyxml.h"

using std::string;
using std::map;
using std::multimap;
using std::make_pair;
using std::vector;
using std::pair;

typedef enum
{
	MSG_FIELD_INT=1,
	MSG_FIELD_STRING=2,
	MSG_FIELD_ARRAY=3,
	MSG_FIELD_TLV=4,
	MSG_FIELD_STRUCT=5,
	MSG_FIELD_ALL
}EValueType;

//field 
typedef struct stConfigField
{
	string strName;
	string strOriName;
	string strTypeName;
	bool   bRequested;
	bool   bAutoSet;
	string strConfigField;
	int    nMaxLen;
	int    nMinLen;
	string strRegex;

	//for struct
	bool bStruct;
	EValueType eStructFieldType;
	int nLen;

	//for struct's default value
	bool bHasDefault;
	int nDefaultValue;
	string strDefaultValue;
}SConfigField;

//type
typedef struct stConfigType
{
	string strName;										//参数名
	EValueType eType;									//参数类型
	int    nCode;										//参数代码
	string strItemType;									//如果该参数为数组，则表示数组中个体的类型,每个个体均为type
	map<string, SConfigField> mapFieldByName;			//该参数中的field by name,针对非struct
	map<string, SConfigField> mapFieldByType;			//该参数中的filed by code,针对非struct
	vector<SConfigField> vecConfigField;				//如果该参数为struct,则表示struct中所有的field
	int operator <(const stConfigType &p)const
	{
		 return ( this->nCode< p.nCode);
	}

	//for array
	bool bArrayField;									//是否为数组
	string strArraryName;								//数组中个体名

	//for int or string default value
	bool bHasDefault;									//是否有缺省值
	int nDefaultValue;									//int的缺省值
	string strDefaultValue;								//string的缺省值
}SConfigType;


//message 
typedef struct stConfigMessage
{
	string strName;										//消息名
	unsigned int dwId;									//消息ID
	bool bAckMsg;										//是否必答消息
	SConfigType oRequestType;							//请求参数
	SConfigType oResponseType;							//应答参数
	map<unsigned int, string> mapArraryTypeNameByCode;
}SConfigMessage;


typedef struct stConfigTypePair
{
	string strPreviousTypeName;							//当前字段类型
	string strCurrentTypeName;							//当前字段所属参数名
}SConfigTypePair;

class CServiceConfig
{
public:
	CServiceConfig();
	~CServiceConfig(){}
	int	LoadServiceConfig(const string &strConfig);
	
	//type and field
	int GetTypeByName(const string &strName, SConfigType& oConfigType);
	int GetTypeByCode(int nCode, SConfigType& oConfigType);
	int GetTypeNameByFieldName(const string &strPreviousTypeName, const string & strFieldName, string & strTypeName, bool bFirst = false);

	map<int, SConfigType>& GetTypeByCodeMap(){return m_mapTypeByCode;}
	bool IsExist(unsigned int dwMsgId);
	//message
	int GetMessageTypeByName(const string &strName, SConfigMessage ** oMessageType);
	int GetMessageTypeById(unsigned int dwMessageId, SConfigMessage ** oMessageType);
	
	bool IsClearText(){return m_bIsClearText;}
	const string & GetServiceName() {return m_strServiceName;}
	unsigned int GetServiceId() {return m_dwServiceId;}
	const string & GetServiceEncoding(){return m_strServiceEncoding;}
	const string & GetSdkEncoding(){return m_strSdkEncoding;}
	const string & GetKeyPrefix(){return m_strKeyPrefix;}
	bool GetIsTreeStruct(){return m_bIsTreeStruct;}
	void Dump()const;
	
private:
	
	int LoadConfigTypeConfig_(TiXmlElement * pConfigType);
	int LoadConfigMsgConfig_(TiXmlElement * pConfigMsg);
	int LoadMsgField_(SConfigMessage & oConfigMessage, TiXmlElement * pConfigFields, bool bRequest = true);
	
	int GetTypeAttr_(const TiXmlElement* pElement, SConfigType& oConfigType);
	int GetFieldAttr_(const TiXmlElement* pElement, SConfigField& oConfigField, bool bStruct = false);
	
	int GetMsgAttr_(const TiXmlElement* pElement, SConfigMessage& oConfigMessage);
	int GetMsgFieldAttr_(const TiXmlElement* pElement, SConfigField& oConfigField);
private:

	//type config
	map<string, SConfigType> m_mapTypeByName;		
	map<int, SConfigType> m_mapTypeByCode;

	//message config
	map<string, SConfigMessage> m_mapMessageByName;
	map<unsigned int, SConfigMessage> m_mapMessageById;

	//type map by field
	multimap<string, SConfigTypePair> m_mapTypePairByFieldName;

	//type map by class
	map<string, EValueType> m_mapETypeByName;

	string m_strServiceName;
	unsigned int m_dwServiceId;
	bool   m_bIsClearText; 
	string m_strServiceEncoding;
	string m_strSdkEncoding;
	bool m_bIsTreeStruct;
	string m_strKeyPrefix;
};

#endif


