#ifndef __META_DATA_H_
#define __META_DATA_H_

#include "ComposeService.h"
#include "../common/detail/_time.h"
#include "SapRecord.h"
#include "SapCommon.h"
#include "Utility.h"
#include "LogTrigger.h"
#include "SapTLVBody.h"
#include "ExtendHead.h"
#include "SapLogHelper.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"
#include "PhpSerializer.h"
namespace sdo{
namespace commonsdk{
    class CAvenueMessageEncode;	
	class CAvenueServiceConfigs;
	class CAvenueMsgHandler;
	class CPhpSerializer;
}
}
string GetTargertKey(const string & strTargertKey);
bool GetTargertSecond(const string & strTargertKey,string &strSecond);

typedef struct stValueData
{
	EValueType        enType;
    vector<sdo::commonsdk::SStructValue> vecValue;
	vector<string> vecStoreValue;
	stValueData(){}
	stValueData(EValueType  enType,vector<sdo::commonsdk::SStructValue>& vecValue)
	{
		Init(enType,vecValue);
	}
	void Clear()
	{
		vecValue.clear();
		vecStoreValue.clear();
	}
	void Init(EValueType  enType,vector<sdo::commonsdk::SStructValue>& vecValue)
	{
		this->enType = enType;
		for (vector<sdo::commonsdk::SStructValue>::iterator it=vecValue.begin();it!=vecValue.end(); it++)
		{
			string str;
			str.assign((const char*)(it->pValue),it->nLen);
			this->vecStoreValue.push_back(str);
			sdo::commonsdk::SStructValue ssv;
			ssv.pValue = (void*)(this->vecStoreValue[this->vecStoreValue.size()-1].c_str());
			ssv.nLen = it->nLen;
			this->vecValue.push_back(ssv);
		}
	}
	stValueData(const stValueData& other)
	{
		(*this)=other;
	}
	stValueData& operator=(const stValueData& other)
	{
		this->enType = other.enType;
		this->vecStoreValue = other.vecStoreValue;
		this->vecValue = other.vecValue;
		vector<string>::iterator it_value = this->vecStoreValue.begin();
		for(vector<sdo::commonsdk::SStructValue>::iterator it = this->vecValue.begin(); it!=this->vecValue.end(); it++,it_value++)
		{
			it->pValue = (void*)it_value->c_str();
		}
		return *this;
	}
	void Dump();
}SValueData;

typedef struct stMetaData
{
	vector<SValueData> vecValues;
	sdo::commonsdk::CPhpSerializer	php;
	SValueData& IsHasChanged()
	{
		vecValues.push_back(stValueData());
		stValueData	&phpValue = vecValues[vecValues.size()-1];
		EValueType  enType = sdo::commonsdk::MSG_FIELD_INT;
		int &dwHasChanged=php.IsHasChanged();
		SS_XLOG(XLOG_DEBUG,"SMetaData::%s [%d]\n", __FUNCTION__,dwHasChanged);
		sdo::commonsdk::SStructValue stv;
		stv.pValue = (void*)&dwHasChanged;
		stv.nLen = sizeof(int);
		vector<sdo::commonsdk::SStructValue> vecValue;
		vecValue.push_back(stv);
		phpValue.Init(enType,vecValue);
		return phpValue;
	}
	void LoadPHPValue(const char* pszValue)
	{
		php.FromString(pszValue);
	}
	SValueData& GetPHPValue()
	{
		vecValues.push_back(stValueData());
		stValueData	&phpValue = vecValues[vecValues.size()-1];
		string &strPHPValue = php.ToString();
		SS_XLOG(XLOG_DEBUG,"SMetaData::%s GetPHPValue[%s]\n", __FUNCTION__,strPHPValue.c_str());
		EValueType  enType = sdo::commonsdk::MSG_FIELD_STRING;
		sdo::commonsdk::SStructValue stv;
		stv.pValue = (void*)strPHPValue.c_str();
		stv.nLen = strPHPValue.size();
		vector<sdo::commonsdk::SStructValue> vecValue;
		vecValue.push_back(stv);
		phpValue.Init(enType,vecValue);
		return phpValue;
	}
	void SetField(const string & strField, SValueData & value)
	{
		SS_XLOG(XLOG_DEBUG,"SMetaData::%s size[%d]\n", __FUNCTION__,value.vecValue.size());
		if (value.enType == sdo::commonsdk::MSG_FIELD_INT)
		{
			SS_XLOG(XLOG_DEBUG,"SMetaData::%s MSG_FIELD_INT\n", __FUNCTION__);
			if (value.vecValue.size()>0)
			{
				php.SetValue(strField,*(int*)value.vecValue[0].pValue);
				SS_XLOG(XLOG_DEBUG,"SMetaData::%s MSG_FIELD_INT len[%d] val[%d]\n", __FUNCTION__,4, *(int*)value.vecValue[0].pValue);
			}
			else
			{
				php.RemoveValue(strField);
			}
		}
		else
		{
			SS_XLOG(XLOG_DEBUG,"SMetaData::%s MSG_FIELD_STRING size[%d]\n", __FUNCTION__,value.vecValue.size());
			if (value.vecValue.size()>0)
			{
				string strValue;
				strValue.assign((const char*)(value.vecValue[0].pValue),value.vecValue[0].nLen);
				php.SetValue(strField,strValue);
				SS_XLOG(XLOG_DEBUG,"SMetaData::%s MSG_FIELD_STRING len[%d] val[%s]\n", __FUNCTION__,value.vecValue[0].nLen,
				strValue.c_str());
			}
			else
			{
				php.RemoveValue(strField);
			}
		}
	}
	SValueData & GetField(const string& strField)
	{
		if (strcasecmp(strField.c_str(),"IsHasChanged()")==0)
		{
			return IsHasChanged();
		}
		vecValues.push_back(stValueData());
		stValueData	&value = vecValues[vecValues.size()-1];
		sdo::commonsdk::SStructValue stv;
		EValueType  enType;
		int nResult = php.GetValue(strField,&stv.pValue,(unsigned int*)&stv.nLen,enType);
		vector<sdo::commonsdk::SStructValue> vecValue;
		vecValue.push_back(stv);
		value.Init(enType,vecValue);
		SS_XLOG(XLOG_DEBUG,"SMetaData::%s key:%s result[%d] stv.nLen[%d]\n", __FUNCTION__,strField.c_str(),nResult,stv.nLen);
		return value;
	}
	void Dump()
	{
		SS_XLOG(XLOG_DEBUG,"SMetaData::%s >>>>\n", __FUNCTION__);
		SS_XLOG(XLOG_DEBUG,"SMetaData::%s <<<<\n", __FUNCTION__);
	}
}SMetaData;


#endif


