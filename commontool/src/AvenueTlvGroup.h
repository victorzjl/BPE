#ifndef _AVENUE_TLV_GROUP_H__
#define _AVENUE_TLV_GROUP_H__
#include "AvenueBuffer.h"
#include <string>
#include <vector>
#include <map>
#include "ServiceConfig.h"

using std::string;
using std::vector;
using std::multimap;

namespace sdo{
namespace commonsdk{

typedef struct tagAvenueMsgAttribute
{
	unsigned short wType;
	unsigned short wLength;
	unsigned char acValue[0];
}SAvenueMsgAttribute;

typedef struct stStructValue
{
	stStructValue():pValue(NULL),nLen(0){}
	void *pValue;
	int nLen;
}SStructValue;

class CAvenueTlvGroupEncoder
{
public:
	CAvenueTlvGroupEncoder(const SAvenueTlvGroupRule &oTlvRule,bool bRequest);
	void SetValue(const string& strName,const void* pValue,unsigned int nValueLen);
	const void * GetBuffer()const{return m_buffer.base();}
	int GetLen()const{return m_buffer.len();}

	int GetValidateCode();
	void Reset();
private:
	void SetStructItem(const string &strName,char *szStructField,const SAvenueStructConfig & oConfig,unsigned char *pStructValue,const void* pValue,unsigned int nValueLen);
	void ValidateValue(const SValueFeatueConfig &oFeature,const void* pValue,unsigned int nValueLen);
private:
	const SAvenueTlvGroupRule &m_oTlvRule;
	bool m_bRequest;
	CAvenueBuffer m_buffer;
	unsigned char *pAttriBlock;
	map<string,unsigned char *> m_mapStructLoc;

	map<unsigned int,int>m_mapRequired;
	int m_nValidateCode;
};


class CAvenueTlvGroupDecoder
{
	typedef multimap<unsigned short,SStructValue> TlvValueMultiMap;
public:
	CAvenueTlvGroupDecoder(const SAvenueTlvGroupRule &m_oTlvRule,bool bRequest,const void *pBuffer,unsigned int nLen);
	int GetValue(const string& strName,void **pBuffer, unsigned int *pLen,EValueType &eValueType);
	int GetValues(const string& strName,vector<SStructValue> &vecValues,EValueType &eValueType);
	int GetValidateCode(){return m_nValidateCode;}
	string GetValidateMsg(){return m_strValidateMsg;}
	
private:
	int GetValueFromBuffer(const SAvenueTlvTypeConfig& oConfig,const string & strName,const string & strStructField,const  SStructValue * pBuffer,vector<SStructValue> &vecValues,EValueType &eValueType);
	void ValidateValue(const SAvenueTlvTypeConfig &oFeature,const void* pValue,unsigned int nValueLen);	
	void Dump();
private:
	const SAvenueTlvGroupRule &m_oTlvRule;
	bool m_bRequest;
	TlvValueMultiMap m_mapValueBuffer;
	const void *m_pBuffer;
	unsigned int m_nLen;
	int m_nValidateCode;
	string m_strValidateMsg;

	map<string ,string> m_mapEncoderResult;
	map<string ,vector<string> > m_mapEncoderResults;
};
}
}
#endif
