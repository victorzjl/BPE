#include "PhpSerializer.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <vector>
#include <arpa/inet.h>
#include "SdkLogHelper.h"

#define PHP_SERIALIZER_MAXLEN 4096
using std::vector;
namespace sdo{
namespace commonsdk{

void CopyPhpSerializerUnit(stPhpSerializerUnit *pDest, stPhpSerializerUnit *pSrc)
{
	pDest->strName=pSrc->strName;
	pDest->eType=pSrc->eType;
	pDest->nValue=pSrc->nValue;
	pDest->strValue=pSrc->strValue;

	map<string, stPhpSerializerUnit * >::const_iterator itr;
	for(itr=pSrc->mapArray.begin();itr!=pSrc->mapArray.end();itr++)
	{
		const string& strArayName=itr->first;
		stPhpSerializerUnit *pNewElement=new stPhpSerializerUnit;
		pDest->mapArray[strArayName]=pNewElement;
		CopyPhpSerializerUnit(pNewElement,itr->second);
	}
}
void Destroy(stPhpSerializerUnit *pUnit)
{
	SDK_XLOG(XLOG_DEBUG, "Destroy stPhpSerializerUnit[%s] type[%d]\n",pUnit->strName.c_str(), pUnit->eType);
	map<string, stPhpSerializerUnit * >::iterator itr,itrTmp;
	for(itr=pUnit->mapArray.begin();itr!=pUnit->mapArray.end();)
	{
		itrTmp=itr++;
		Destroy(itrTmp->second);
		pUnit->mapArray.erase(itrTmp);
	}
	delete pUnit;
}
CPhpSerializer::CPhpSerializer():m_isHasChanged(false)
{
}
CPhpSerializer::~CPhpSerializer()
{
	SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::~CPhpSerializer\n");
	map<string, stPhpSerializerUnit * >::iterator itr;
	for(itr=m_oElements.begin();itr!=m_oElements.end();itr++)
	{
		Destroy(itr->second);
	}
	m_oElements.clear();
}
CPhpSerializer::CPhpSerializer(const string & strSerializer)
{
	FromString(strSerializer);
}
void CPhpSerializer::FromString(const string & strSerializer)
{
	map<string, stPhpSerializerUnit * >::iterator itr;
	for(itr=m_oElements.begin();itr!=m_oElements.end();itr++)
	{
		Destroy(itr->second);
	}
	m_oElements.clear();
	
	m_nLoc=0;
	const char *pLoc=strSerializer.c_str();  //
	while(ReadRootElement(&pLoc)!=-1);
	if(IsEnableLevel(SDK_MODULE,XLOG_NOTICE))
	{
		Dump();
	}
	m_isHasChanged=false;
}

string& CPhpSerializer::ToString()
{
	char szBuffer[PHP_SERIALIZER_MAXLEN]={0};
	char *pLoc=szBuffer;
	int nLoc=0;
	m_nLoc=0;
	map<string, stPhpSerializerUnit * >::iterator itr;
	for(itr=m_oElements.begin();itr!=m_oElements.end();itr++)
	{
		const string & strKey=itr->first;
		stPhpSerializerUnit * pUnit=itr->second;
		nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-nLoc,"%s|",strKey.c_str());
		SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ToString key[%s] nLoc[%d]\n",strKey.c_str(),nLoc);
		pLoc+=nLoc;
		m_nLoc+=nLoc;
		ElementToString(pUnit,&pLoc);
		if (pLoc<szBuffer)
		{
			SDK_XLOG(XLOG_ERROR, "CPhpSerializer::Wrong \n");
		}
	}
		
	string strValue = szBuffer;
	boost::replace_all(strValue,"_login_info","_LOGIN_INFO");
	m_vecStrGetReference.push_back(strValue);
	SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ToString [%s]\n",strValue.c_str());
	
	string &strRefValue=*(m_vecStrGetReference.rbegin());
	
	SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ToString2 [%s]\n",strRefValue.c_str());
	return strRefValue;
}

stPhpSerializerUnit * CPhpSerializer::GetElement(const string& strName)
{
	SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::GetElement [%s]\n",strName.c_str());
	vector<string> vecParams;
	boost::algorithm::split( vecParams, strName, boost::algorithm::is_any_of("["), boost::algorithm::token_compress_on); 
	if(vecParams.size()<1) 
	{
		SDK_XLOG(XLOG_WARNING, "GetElement [%s]  failed: invalid input\n",strName.c_str());
		return NULL;
	}

	map<string, stPhpSerializerUnit * >::iterator itrElement= m_oElements.find(vecParams[0]);
	if(itrElement==m_oElements.end())
	{
		SDK_XLOG(XLOG_WARNING, "GetElement [%s]  failed: no element\n",strName.c_str());
		return NULL;
	}
	
	stPhpSerializerUnit *pElement=itrElement->second;
	
	SDK_XLOG(XLOG_TRACE, "GetElement parasize[%d] \n", vecParams.size());
	vector<string>::iterator itrParam=vecParams.begin();
	for(++itrParam;itrParam!=vecParams.end();itrParam++)
	{
		if(pElement->eType==MSG_FIELD_INT||pElement->eType==MSG_FIELD_STRING)
		{
			SDK_XLOG(XLOG_TRACE, "GetElement line[%d] \n", __LINE__);
			return pElement;
		}
		else if(pElement->eType==MSG_FIELD_ARRAY)
		{
			int nIndex=-1;
			char szField[64]={0};
			char szFieldName[128]={0};
			string strElement=*itrParam;
			if(sscanf(strElement.c_str(),"%d",&nIndex)!=1&&sscanf(strElement.c_str(),"'%63[^''']']",szField)!=1)
			{
				SDK_XLOG(XLOG_WARNING, "GetElement [%s]  failed: no element\n",strName.c_str());
				return NULL;
			}
			if(nIndex!=-1)
			{
				snprintf(szFieldName,128,"i_%d",nIndex);
			}
			else
			{
				snprintf(szFieldName,128,"s_%s",szField);
			}
			SDK_XLOG(XLOG_TRACE, "GetElement [%s] szFieldName:%s\n",szField,szFieldName);
			map<string, stPhpSerializerUnit * > ::iterator itrArray=pElement->mapArray.find(szFieldName);
			if(itrArray==pElement->mapArray.end())
			{
				SDK_XLOG(XLOG_WARNING, "GetElement line[%d] \n", __LINE__);
				
				map<string, stPhpSerializerUnit * >::iterator it = pElement->mapArray.begin();
				for(;it!=pElement->mapArray.end();it++)
				{
					SDK_XLOG(XLOG_WARNING, "GetElement key[%s] \n", it->first.c_str());
				}
				return NULL;
			}
			pElement=itrArray->second;
		}
		else 
		{
			SDK_XLOG(XLOG_WARNING, "GetElement [%s]  failed: invalid type\n",strName.c_str());
			return NULL;
		}
	}
	return pElement;
}
int CPhpSerializer::GetValue(const string& strName, string & strValue)
{
	stPhpSerializerUnit * pUnit=GetElement(strName);
	if(pUnit!=NULL&&pUnit->eType==MSG_FIELD_STRING)
	{
		strValue=pUnit->strValue;
		return 0;
	}
	return -1;
}
int CPhpSerializer::GetValue(const string& strName, int & nValue)
{
	stPhpSerializerUnit * pUnit=GetElement(strName);
	if(pUnit!=NULL&&pUnit->eType==MSG_FIELD_INT)
	{
		nValue=pUnit->nValue;
		return 0;
	}
	return -1;
}
int CPhpSerializer::GetValue(const string& strName, stPhpSerializerUnit **pPhpSerializerUnit)
{
	stPhpSerializerUnit * pUnit=GetElement(strName);
	if(pUnit!=NULL)
	{
		*pPhpSerializerUnit=pUnit;
		return 0;
	}
	return -1;
}
int CPhpSerializer::GetValue(const string& strName,void **pBuffer, unsigned int *pLen)
{
	EValueType eValueType;
	string strNameLower=strName;
	SDK_XLOG(XLOG_TRACE, "GetValue strNameLower:%s  ..\n",strNameLower.c_str());
	boost::replace_all(strNameLower,"_LOGIN_INFO","_login_info");
	SDK_XLOG(XLOG_TRACE, "GetValue strNameLower:%s  ....\n",strNameLower.c_str());
	return GetValue(strNameLower,pBuffer, pLen,eValueType);
}
int CPhpSerializer::GetValue(const string& strNameOri,void **pBuffer, unsigned int *pLen,EValueType &eValueType)
{
	string strName = strNameOri;
	boost::replace_all(strName,"_LOGIN_INFO","_login_info");
	
	stPhpSerializerUnit * pUnit=GetElement(strName);
	if(pUnit!=NULL&&pUnit->eType==MSG_FIELD_INT)
	{
		m_vecNGetReference.push_back(pUnit->nValue);
		int & nValue=*(m_vecNGetReference.rbegin());
		*pBuffer=&nValue;
		*pLen=4;
		eValueType=MSG_FIELD_INT;
		return 0;
	}
	else if(pUnit!=NULL&&pUnit->eType==MSG_FIELD_STRING)
	{
		m_vecStrGetReference.push_back(pUnit->strValue);
		string & strValue=*(m_vecStrGetReference.rbegin());
		*pBuffer=(void *)strValue.data();
		*pLen=strValue.length();
		eValueType=MSG_FIELD_STRING;
		return 0;
	}
	else
	{
		SDK_XLOG(XLOG_WARNING, "GetValue line[%d] \n", __LINE__);
		return -1;
	}
}
void CPhpSerializer::SetValue(const string& strName, const string & strValue)
{
	string strNameLower = strName;
	boost::replace_all(strNameLower,"_LOGIN_INFO","_login_info");
	stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
	pUnit->strName=strNameLower;
	pUnit->eType=MSG_FIELD_STRING;
	pUnit->strValue=strValue;
	if(SetValueNoCopy(strNameLower,pUnit)==0)
	{
		m_isHasChanged=true;
	}
	else
	{
		delete pUnit;
	}
}
void CPhpSerializer::SetValue(const string& strName, int nValue)
{
	stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
	string strNameLower = strName;
	boost::replace_all(strNameLower,"_LOGIN_INFO","_login_info");
	pUnit->strName=strNameLower;
	pUnit->eType=MSG_FIELD_INT;
	pUnit->nValue=nValue;
	if(SetValueNoCopy(strNameLower,pUnit)==0)
	{
		m_isHasChanged=true;
	}
	else
	{
		delete pUnit;
	}
}
void CPhpSerializer::SetValue(const string& strName, stPhpSerializerUnit *pPhpSerializerUnit)
{
	stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
	CopyPhpSerializerUnit(pUnit,pPhpSerializerUnit);
	if(SetValueNoCopy(strName,pUnit)==0)
	{
		m_isHasChanged=true;
	}
	else
	{
		delete pUnit;
	}
}
int CPhpSerializer::SetValueNoCopy(const string& strName, stPhpSerializerUnit *pPhpSerializerUnit)
{
	if(strName.length()==0)
	{
		SDK_XLOG(XLOG_WARNING, "SetElement [%s]  failed: invalid input\n",strName.c_str());
		return -1;
	}
	
	vector<string> vecParams;
	boost::algorithm::split( vecParams, strName, boost::algorithm::is_any_of("["), boost::algorithm::token_compress_on);

	int nTotalPath=vecParams.size();
	int nLoc=0;
	if(nTotalPath<1)
	{
		SDK_XLOG(XLOG_WARNING, "SetElement [%s]  failed: invalid input\n",strName.c_str());
		return -1;
	}
	
	nLoc=1;
	stPhpSerializerUnit *pElement=NULL;
	map<string, stPhpSerializerUnit * >::iterator itrElement= m_oElements.find(vecParams[0]);
	SDK_XLOG(XLOG_DEBUG, "[%d]CPhpSerializer::SetValueNoCopy  [%s] size[%d], nLoc[%d]\n", __LINE__,strName.c_str(),nTotalPath,nLoc);
	if(nLoc==nTotalPath)
	{
		if(itrElement==m_oElements.end())
		{
			m_oElements[vecParams[0] ]=pPhpSerializerUnit;
			return 0;
		}
		else
		{
			pElement=itrElement->second;
			Destroy(pElement);
			m_oElements[vecParams[0] ]=pPhpSerializerUnit;
			return 0;
		}
	}
	else
	{
		if(itrElement==m_oElements.end())
		{
			stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
			pUnit->strName=vecParams[0] ;
			pUnit->eType=MSG_FIELD_ARRAY;
			m_oElements[vecParams[0] ]=pUnit;
			pElement=pUnit;
		}
		else
		{
			pElement=itrElement->second;
		}
	}
	
	vector<string>::iterator itrParam=vecParams.begin();
	for(++itrParam,++nLoc;itrParam!=vecParams.end();itrParam++,nLoc++)
	{
		string strElement=*itrParam;
		if(pElement->eType==MSG_FIELD_ARRAY)
		{
			int nIndex=-1;
			char szField[64]={0};
			char szFieldName[128]={0};
			
			if(sscanf(strElement.c_str(),"%d",&nIndex)!=1&&sscanf(strElement.c_str(),"'%63[^''']']",szField)!=1)
			{
				SDK_XLOG(XLOG_WARNING, "SetElement [%s]  failed: invalid array name\n",strName.c_str());
				return -1;
			}
			if(nIndex!=-1)
			{
				snprintf(szFieldName,128,"i_%d",nIndex);
			}
			else
			{
				snprintf(szFieldName,128,"s_%s",szField);
			}
			
			map<string, stPhpSerializerUnit * > ::iterator itrArray=pElement->mapArray.find(szFieldName);
			if(nLoc==nTotalPath)
			{
				if(itrArray==pElement->mapArray.end())
				{
					pElement->mapArray[szFieldName]=pPhpSerializerUnit;
					return 0;
				}
				else
				{
					stPhpSerializerUnit *pTmpElement=itrArray->second;
					Destroy(pTmpElement);
					pElement->mapArray[szFieldName]=pPhpSerializerUnit;
					return 0;
				}
			}
			else
			{
				if(itrArray==pElement->mapArray.end())
				{
					stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
					pUnit->strName=szFieldName ;
					pUnit->eType=MSG_FIELD_ARRAY;
					pElement->mapArray[szFieldName ]=pUnit;
					pElement=pUnit;
				}
				else
				{
					pElement=itrArray->second;
				}
			}
		}
		else 
		{
			SDK_XLOG(XLOG_WARNING, "SetElement [%s]  failed: [%s] itype unmatched array\n",strName.c_str(),strElement.c_str());
			return -1;
		}
	}
	SDK_XLOG(XLOG_WARNING, "SetElement [%s]  failed: never print\n",strName.c_str());
	return -1;
}
void CPhpSerializer::SetTlvGroup(CAvenueMsgHandler *pHandler)
{
	m_isHasChanged=true;
	
	map<string, SAvenueTlvTypeConfig> &mapSrcMap=pHandler->GetTlvMap();
	map<string, SAvenueTlvTypeConfig> ::iterator itrSrc;
	for(itrSrc=mapSrcMap.begin();itrSrc!=mapSrcMap.end();itrSrc++)
	{
		const string & strName=itrSrc->first;
		SAvenueTlvTypeConfig & oSrcConfig=itrSrc->second;
		int nValue=0;
		string strValue;
		char *pStrValue;
		if(oSrcConfig.emType==MSG_FIELD_INT
			&&!(oSrcConfig.isArray)
			&&pHandler->GetValue(strName.c_str(),&nValue)==0)
		{
			SetValue(strName,nValue);
		}
		else if(oSrcConfig.emType==MSG_FIELD_STRING
			&&!(oSrcConfig.isArray)
			&&pHandler->GetValue(strName.c_str(),&pStrValue)==0)
		{
			SetValue(strName,pStrValue);
		}
		else if(oSrcConfig.emType==MSG_FIELD_STRUCT
			&&!(oSrcConfig.isArray))
		{
			map<string,SAvenueStructConfig>::iterator itrStruct;
			for(itrStruct=oSrcConfig.mapStructConfig.begin();itrStruct!=oSrcConfig.mapStructConfig.end();itrStruct++)
			{
				char szPath[128]={0};
				SAvenueStructConfig & oStructConfig=itrStruct->second;
				snprintf(szPath,128,"%s['%s']",strName.c_str(),oStructConfig.strName.c_str());
				if(oStructConfig.emStructType==MSG_FIELD_INT
					&&pHandler->GetValue(szPath,&nValue)==0)
				{
					SetValue(szPath,nValue);
				}
				else if(oStructConfig.emStructType==MSG_FIELD_STRING
					&&pHandler->GetValue(szPath,&pStrValue)==0)
				{
					SetValue(szPath,pStrValue);
				}
			}
		}
		else if(oSrcConfig.emType==MSG_FIELD_INT
			&&oSrcConfig.isArray)
		{
			vector<SStructValue> vecValue;
			if(pHandler->GetValues(strName.c_str(),vecValue)==0)
			{
				int i=0;
				char szPath[128]={0};
				vector<SStructValue>::iterator itr;
				for(itr=vecValue.begin();itr!=vecValue.end();itr++)
				{
					SStructValue &oValue=*itr;
					snprintf(szPath,128,"%s[%d]",strName.c_str(),i);
					SetValue(szPath,*(int *)oValue.pValue);
					i++;
				}
			}
		}
		else if(oSrcConfig.emType==MSG_FIELD_STRING
			&&oSrcConfig.isArray)
		{
			vector<SStructValue> vecValue;
			if(pHandler->GetValues(strName.c_str(),vecValue)==0)
			{
				int i=0;
				char szPath[128]={0};
				vector<SStructValue>::iterator itr;
				for(itr=vecValue.begin();itr!=vecValue.end();itr++)
				{
					SStructValue &oValue=*itr;
					snprintf(szPath,128,"%s[%d]",strName.c_str(),i);
					SetValue(szPath,string((const char *)oValue.pValue,oValue.nLen));
					i++;
				}
			}
		}
		else if(oSrcConfig.emType==MSG_FIELD_STRUCT
			&&oSrcConfig.isArray)
		{
			vector<SStructValue> vecValue;
			if(pHandler->GetValues(strName.c_str(),vecValue)==0)
			{
				int i=0;
				char szPath[128]={0};
				vector<SStructValue>::iterator itr;
				for(itr=vecValue.begin();itr!=vecValue.end();itr++)
				{
					SStructValue &oValue=*itr;
					map<string,SAvenueStructConfig>::iterator itrStruct;
					for(itrStruct=oSrcConfig.mapStructConfig.begin();itrStruct!=oSrcConfig.mapStructConfig.end();itrStruct++)
					{
						char szPath[128]={0};
						SAvenueStructConfig & oStructConfig=itrStruct->second;
						snprintf(szPath,128,"%s[%d]['%s']",strName.c_str(),i,oStructConfig.strName.c_str());
						if(oStructConfig.emStructType==MSG_FIELD_INT)
						{
							nValue=*(int *)((const char *)oValue.pValue+oStructConfig.nLoc);
							SetValue(szPath,nValue);
						}
						else if(oStructConfig.emStructType==MSG_FIELD_STRING)
						{
							if(oStructConfig.nLen==-1)
							{
								strValue=string((const char *)oValue.pValue+oStructConfig.nLoc,oValue.nLen-oStructConfig.nLoc);
							}
							else
							{
								strValue=string((const char *)oValue.pValue+oStructConfig.nLoc,oStructConfig.nLen);
							}
							SetValue(szPath,strValue);
						}
					}
					i++;
				}
			}
		}
	}
} 
int CPhpSerializer::ReadRootElement(const char **ppLoc)
{
	const char *pLoc=*ppLoc;
	if(*pLoc=='\0'||*pLoc=='|') 
	{
		SDK_XLOG(XLOG_DEBUG, "FromString[%d]  ended\n",__LINE__);
		return -1;
	}
	
	string strName;
	if(ReadStrByEnd(&pLoc,'|',strName)==-1) return -1;
	
	stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
	m_oElements[strName]=pUnit;

	SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ReadRootElement name[%s]  type[%c]\n",strName.c_str(),*(pLoc+1));
	
	int nStrLen=0;
	int nArrayLen=0;
	switch (*(++pLoc))
	{
		case 'b':
		case 'i':
			pUnit->strName=strName;
			pUnit->eType=MSG_FIELD_INT;
			if(*(++pLoc)!=':')
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
				return -1;
			}
			if(ReadInt(&(++pLoc),';',&(pUnit->nValue))==-1) return -1;
			pLoc++;
			break;
		case 's':
			pUnit->strName=strName;
			pUnit->eType=MSG_FIELD_STRING;
			if(*(++pLoc)!=':') 
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
				return -1;
			}
			if(ReadInt(&(++pLoc),':',&nStrLen)==-1) return -1;
			if(ReadStr(&(++pLoc),nStrLen,pUnit->strValue)==-1) return -1;
			break;
		case 'a':
			pUnit->strName=strName;
			pUnit->eType=MSG_FIELD_ARRAY;
			if(*(++pLoc)!=':')
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
				return -1;
			}
			if(ReadInt(&(++pLoc),':',&nArrayLen)==-1) return -1;
			if(ReadArray(&(++pLoc),nArrayLen,pUnit->mapArray)==-1) return -1;
			break;
		case 'N':
			pUnit->strName=strName;
			pUnit->eType=MSG_FIELD_NULL;
			if(*(++pLoc)!=';')
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
				return -1;
			}
			break;
		default:
			pUnit->strName=strName;
			pUnit->eType=MSG_FIELD_NULL;
			return -1;
	}
	*ppLoc=pLoc;
	return 0;
}
int CPhpSerializer::ReadInt(const char **ppLoc,char cEnd,int *pValue)
{
	const char *pLoc=*ppLoc;
	while(pLoc++)
	{
		if(*pLoc==cEnd)
		{
			sscanf(*ppLoc,"%d",pValue);
			*ppLoc=pLoc;
			SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ReadInt cEnd[%c] nVaue=[%d]\n",cEnd,*pValue);
			return 0;
		}
		else if(*pLoc=='\0')
		{
			SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: string end\n",__LINE__);
			return -1;
		}
	}
}
int CPhpSerializer::ReadStrByEnd(const char **ppLoc,char cEnd,string &strValue)
{
	const char *pLoc=*ppLoc;
	while(pLoc++)
	{
		if(*pLoc==cEnd)
		{
			strValue=string(*ppLoc,pLoc-*ppLoc);
			*ppLoc=pLoc;
			SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ReadStrByEnd cEnd[%c] strVaue=[%s]\n",cEnd,strValue.c_str());
			return 0;
		}
		else if(*pLoc=='\0')
		{
			SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: string end\n",__LINE__);
			return -1;
		}
	}
}
int CPhpSerializer::ReadStr(const char **ppLoc,int nLen,string &strValue)
{
	const char *pLoc=*ppLoc;
	if(strlen(pLoc)<nLen+2)
	{
		SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %s  is  shorter  than [%d]\n",__LINE__,pLoc,nLen);
		return -1;
	}
	if(*(pLoc++)!='\"')
	{
		SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
		return -1;
	}
	strValue=string(pLoc,nLen);
	pLoc+=nLen;
	SDK_XLOG(XLOG_DEBUG, "CPhpSerializer::ReadStr nLen[%d] strVaue=[%s]\n",nLen,strValue.c_str());
	if(*(pLoc++)!='\"')
	{
		SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
		return -1;
	}
	if(*(pLoc++)!=';') 
	{
		SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
		return -1;
	}
	*ppLoc=pLoc;
	return 0;
}
int CPhpSerializer::ReadArrayKey(const char **ppLoc,string &strKey)
{
	const char *pLoc=*ppLoc;
	int nIntValue=0;
	int nStrLen=0;
	string strValue;
	char szKey[128]={0};
	switch (*pLoc)
	{
		case 'i':
			if(*(++pLoc)!=':')
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
				return -1;
			}
			if(ReadInt(&(++pLoc),';',&(nIntValue))==-1) return -1;
			pLoc++;
			snprintf(szKey,128,"i_%d",nIntValue);
			break;
		case 's':
			if(*(++pLoc)!=':')
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
				return -1;
			}
			if(ReadInt(&(++pLoc),':',&nStrLen)==-1) return -1;
			if(ReadStr(&(++pLoc),nStrLen,strValue)==-1) return -1;
			if(strcmp(strValue.c_str(),"_LOGIN_INFO")==0)
			{
				std::transform(strValue.begin(), strValue.end(), strValue.begin(), (int(*)(int))tolower);
			}
			snprintf(szKey,128,"s_%s",strValue.c_str());
			break;
		default:
			{
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid type\n",__LINE__,*pLoc);
				return -1;
			}
	}
	strKey=szKey;
	*ppLoc=pLoc;
	return 0;
}
int CPhpSerializer::ReadArray(const char **ppLoc,int nArrayNum,map<string, stPhpSerializerUnit * > &mapArray)
{
	const char *pLoc=*ppLoc;
	if(*(pLoc++)!='{')
	{
		SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
		return -1;
	}
	
	for(int i=0;i<nArrayNum;i++)
	{
		string strName;
		int nStrLen=0;
		int nArrayLen=0;
		ReadArrayKey(&pLoc,strName);
		stPhpSerializerUnit *pUnit=new stPhpSerializerUnit;
		pUnit->strName=strName ;
		mapArray[strName]=pUnit;
		switch (*pLoc)
		{
			case 'b':
			case 'i':
				pUnit->eType=MSG_FIELD_INT;
				if(*(++pLoc)!=':')
				{
					SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
					return -1;
				}
				if(ReadInt(&(++pLoc),';',&(pUnit->nValue))==-1) return -1;
				pLoc++;
				break;
			case 's':
				pUnit->eType=MSG_FIELD_STRING;
				if(*(++pLoc)!=':') 
				{
					SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
					return -1;
				}
				if(ReadInt(&(++pLoc),':',&nStrLen)==-1) return -1;
				if(ReadStr(&(++pLoc),nStrLen,pUnit->strValue)==-1) return -1;
				break;
			case 'a':
				pUnit->eType=MSG_FIELD_ARRAY;
				if(*(++pLoc)!=':') return -1;
				if(ReadInt(&(++pLoc),':',&nArrayLen)==-1) return -1;
				if(ReadArray(&(++pLoc),nArrayLen,pUnit->mapArray)==-1) return -1;
				break;
			case 'N':
				pUnit->eType=MSG_FIELD_NULL;
				if(*(++pLoc)!=';')
				{
					SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
					return -1;
				}
				break;
			default:
				pUnit->eType=MSG_FIELD_NULL;
				SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid type\n",__LINE__,*pLoc);
				return -1;	
		}
	}
	if(*(pLoc++)!='}') 
	{
		SDK_XLOG(XLOG_WARNING, "FromString[%d]  failed: %c is invalid\n",__LINE__,*(pLoc-1));
		return -1;
	}
	*ppLoc=pLoc;
	return 0;
}
int CPhpSerializer::ElementToString(stPhpSerializerUnit * pUnit, char **ppLoc)
{
	char *pLoc=*ppLoc;
	
	int nLoc=0;
	switch (pUnit->eType)
	{
		case Php_Serializer_INT:
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"i:%d;",pUnit->nValue);
			pLoc+=nLoc;
			m_nLoc+=nLoc;
			break;
		case Php_Serializer_STRING:
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"s:%d:\"%s\";",pUnit->strValue.length(),pUnit->strValue.c_str());
			pLoc+=nLoc;
			m_nLoc+=nLoc;
			break;
		case Php_Serializer_ARRAY:
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"a:%d:{",pUnit->mapArray.size());
			pLoc+=nLoc;
			m_nLoc+=nLoc;
			ArrayToString(pUnit->mapArray,&pLoc);
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"}");
			pLoc+=nLoc;
			m_nLoc+=nLoc;
			break;
		default:
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"NULL;");
			pLoc+=nLoc;
			m_nLoc+=nLoc;
			break;
	}
	*ppLoc=pLoc;
	return 0;
}
int CPhpSerializer::ArrayToString(map<string, stPhpSerializerUnit * > &mapArray,char **ppLoc)
{
	char *pLoc=*ppLoc;
	map<string, stPhpSerializerUnit * >::iterator itr;
	int nKey=0;
	char szKey[64]={0};
	int nLoc=0;
	for(itr=mapArray.begin();itr!=mapArray.end();itr++)
	{
		const string& strKey=itr->first;
		stPhpSerializerUnit *pUnit=itr->second;
		if(sscanf(strKey.c_str(),"i_%d",&nKey)==1)
		{
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"i:%d;",nKey);
			pLoc+=nLoc;
			m_nLoc+=nLoc;
		}
		else if(sscanf(strKey.c_str(),"s_%s",&szKey)==1)
		{
		//	if(strcmp(szKey,"_login_info")==0)
		//	{
		//		std::transform(szKey, szKey+strlen(szKey), szKey, (int(*)(int))toupper);
		//	}
			nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"s:%d:\"%s\";",strlen(szKey),szKey);
			pLoc+=nLoc;
			m_nLoc+=nLoc;
		}
		else
		{
			SDK_XLOG(XLOG_WARNING, "ToString[%d]  failed: name[%s] invalid format\n",__LINE__,strKey.c_str());
			return -1;
		}
		
		switch (pUnit->eType)
		{
			case MSG_FIELD_INT:
				nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"i:%d;",pUnit->nValue);
				pLoc+=nLoc;
				m_nLoc+=nLoc;
				break;
			case MSG_FIELD_STRING:
				nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"s:%d:\"%s\";",pUnit->strValue.length(),pUnit->strValue.c_str());
				pLoc+=nLoc;
				m_nLoc+=nLoc;
				break;
			case MSG_FIELD_ARRAY:
				nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"a:%d:{",pUnit->mapArray.size());
				pLoc+=nLoc;
				m_nLoc+=nLoc;
				ArrayToString(pUnit->mapArray,&pLoc);
				nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"}");
				pLoc+=nLoc;
				m_nLoc+=nLoc;
				break;
			default:
				nLoc=snprintf(pLoc,PHP_SERIALIZER_MAXLEN-m_nLoc,"NULL;");
				pLoc+=nLoc;
				m_nLoc+=nLoc;
				break;
		}
	}
	*ppLoc=pLoc;
	return 0;
}
void CPhpSerializer::Dump()
{
	SDK_XLOG(XLOG_DEBUG, "#######CPhpSerializer::Dump [%d]########\n",m_oElements.size());
	map<string, stPhpSerializerUnit * >::iterator itr;
	for(itr=m_oElements.begin();itr!=m_oElements.end();++itr)
	{
		const string & strName=itr->first;
		stPhpSerializerUnit * pUnit=itr->second;
		SDK_XLOG(XLOG_DEBUG, "%s:\n",strName.c_str());
		DumpUnit(pUnit);
	}
}
void CPhpSerializer::DumpUnit(stPhpSerializerUnit * pUnit)
{
	
	if(pUnit->eType==MSG_FIELD_NULL)
		SDK_XLOG(XLOG_DEBUG, "\t\t%s  NULL\n",pUnit->strName.c_str());
	else if(pUnit->eType==MSG_FIELD_INT)
		SDK_XLOG(XLOG_DEBUG, "\t\t%s  %d\n",pUnit->strName.c_str(),pUnit->nValue);
	else if(pUnit->eType==MSG_FIELD_STRING)
		SDK_XLOG(XLOG_DEBUG, "\t\t%s  %s\n",pUnit->strName.c_str(),pUnit->strValue.c_str());
	else if(pUnit->eType==MSG_FIELD_ARRAY)
	{
		SDK_XLOG(XLOG_DEBUG, "\t\t#####Php_Serializer_ARRAY::Dump [%d]########\n",pUnit->mapArray.size());
		map<string, stPhpSerializerUnit * >::iterator itr;
		for(itr=pUnit->mapArray.begin();itr!=pUnit->mapArray.end();++itr)
		{
			const string & strName=itr->first;
			stPhpSerializerUnit * pUnit=itr->second;
			SDK_XLOG(XLOG_DEBUG, "\t\t%s:\n",strName.c_str());
			DumpUnit(pUnit);
		}
	}
}
}
}

