#include "AvenueMsgHandler.h"
#include "SdkLogHelper.h"
#include "CodeConverter.h"
#include <boost/algorithm/string.hpp> 
#include <string>
#include <boost/asio.hpp>

using std::make_pair;
using std::string;
 
namespace sdo{
namespace commonsdk{

SNDAAPI(ISDOCommonMsg*) sdoCreateRequestMsg(IN char * szServiceName)
{
		return new CAvenueMsgHandler(szServiceName);
}
SNDAAPI(ISDOCommonMsg*) sdoCreateResponseMsg(IN char * szServiceName)
{
		return new CAvenueMsgHandler(szServiceName, false);
}


CAvenueMsgHandler::CAvenueMsgHandler(const string & strServiceName, bool bReuqest,CAvenueServiceConfigs * pConfig)
	:m_strSericeName(strServiceName), m_oServiceConfig(NULL), m_oMessage(NULL),
	m_bRequest(bReuqest),m_pMsgEnCoder(NULL),m_pMsgDeCoder(NULL),
	m_pBuffer(NULL),m_nBufferLen(0),m_nLen(0)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	char szServiceName[64] = {0};
	char szMsgName[64] = {0};
	transform(m_strSericeName.begin(), m_strSericeName.end(), m_strSericeName.begin(), (int(*)(int))tolower);
	sscanf(m_strSericeName.c_str(), "%[^.].%s", szServiceName, szMsgName);
	if (pConfig==NULL)
	{
	    CAvenueServiceConfigs::Instance()->GetServiceByName(szServiceName, &m_oServiceConfig);
	}
	else
	{
	    pConfig->GetServiceByName(szServiceName, &m_oServiceConfig);
	}
	if(m_oServiceConfig != NULL)
		m_oServiceConfig->oConfig.GetMessageTypeByName(szMsgName,&m_oMessage);

	if(m_bRequest) 
	{
		m_pMsgEnCoder=new CAvenueTlvGroupEncoder(m_oMessage->oRequestTlvRule,m_bRequest);
	}
	else 
	{
		m_pMsgEnCoder=new CAvenueTlvGroupEncoder(m_oMessage->oResponseTlvRule,m_bRequest);
	}

	m_pExtHeadBuf = malloc(1024);
	m_nExtHeadBufLen = 1024;
	m_nExtHeadLen = 0;
		
}
int CAvenueMsgHandler::Decode(const void *pBuffer,int nLen)
{
    SDK_XLOG(XLOG_NOTICE, "CAvenueMsgHandler::%s [%s] Decode Message\n", m_strSericeName.c_str(),__FUNCTION__);	
	if(m_nBufferLen > 0)
	{
		free(m_pBuffer);
	}
	if(m_pMsgDeCoder!=NULL)
	{
		delete m_pMsgDeCoder;
	}
	if(pBuffer != NULL && nLen > 0)
	{
		int nAllocSize = SDO_ALIGN(nLen, 8192);
		m_nBufferLen += nAllocSize;
		m_pBuffer = malloc(m_nBufferLen);
		memcpy(m_pBuffer, pBuffer, nLen);
		m_nLen = nLen;
	}
	if(m_bRequest) 
	{
		m_pMsgDeCoder=new CAvenueTlvGroupDecoder(m_oMessage->oRequestTlvRule,m_bRequest,m_pBuffer,nLen);
	}
	else 
	{
		m_pMsgDeCoder=new CAvenueTlvGroupDecoder(m_oMessage->oResponseTlvRule,m_bRequest,m_pBuffer,nLen);
	}
	return m_pMsgDeCoder->GetValidateCode();
}
int CAvenueMsgHandler::Decode(const void *pBuffer,int nLen,int & nReturnMsgType,string &strReturnMsg)
{
	int nReturnCode=Decode(pBuffer,nLen);
	if(nReturnCode!=0)
	{
		strReturnMsg=m_pMsgDeCoder->GetValidateMsg();
		if(m_bRequest) 
		{
			nReturnMsgType=m_oMessage->oRequestTlvRule.nReturnMsgType;
		}
		else 
		{
			nReturnMsgType=m_oMessage->oResponseTlvRule.nReturnMsgType;
		}
	}
	return nReturnCode;
}
int  CAvenueMsgHandler::GetValue(IN const char* pKey, OUT void ** pValue, OUT int * pLen)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	EValueType eValueType;
	return GetValue(pKey, pValue, pLen,eValueType);
}

int  CAvenueMsgHandler::GetValue(IN const char* pKey, OUT void ** pValue, OUT int * pLen, OUT EValueType &eValueType)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::GetValue message[%s] field[%s]\n", m_strSericeName.c_str(),pKey);

	if(m_oServiceConfig == NULL || m_oMessage == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "CAvenueMsgHandler::GetValue can not find the service or msg[%s]\n", m_strSericeName.c_str());
		return -1;
	}
	string strName=pKey;
	transform (strName.begin(), strName.end(), strName.begin(), (int(*)(int))tolower);
	return m_pMsgDeCoder->GetValue(strName, pValue, (unsigned int *)pLen,eValueType);

}

int CAvenueMsgHandler::GetValue(IN const char* pKey, OUT char ** ppValue)
{
		SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	void *pData;
	int nLen;
	int nRet = GetValue((char *)pKey, &pData, &nLen);
	if(nRet == 0) 
	{
		*((char *)pData + nLen) = '\0';
		*ppValue = (char *)pData;
	}
	return nRet;
	
}

int CAvenueMsgHandler::GetValue(IN const char* pKey, OUT int *pValue)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	void *pData;
	int nLen;
	int nRet = GetValue((char *)pKey, &pData, &nLen);
	if(nRet == 0 && nLen == 4) 
	{
		*pValue =*(int *)pData;
	}
	return nRet;
}
int CAvenueMsgHandler::GetValues(IN const char* pKey, OUT vector<SStructValue> & vecValue)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::GetValues name[%s] value[%d]\n", pKey);
	if(m_oServiceConfig == NULL || m_oMessage == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "CAvenueMsgHandler::GetValues can not find the service or msg[%s]\n", m_strSericeName.c_str());
		return -1;
	}
	EValueType eValueType;
	string strName=pKey;
	transform (strName.begin(), strName.end(), strName.begin(), (int(*)(int))tolower);
	return m_pMsgDeCoder->GetValues(strName,vecValue,eValueType);
}

int CAvenueMsgHandler::GetValues(IN const char* pKey, OUT vector<SStructValue> & vecValue, EValueType &eValueType)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::GetValues name[%s] \n", pKey);
	if(m_oServiceConfig == NULL || m_oMessage == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "CAvenueMsgHandler::GetValues can not find the service or msg[%s]\n", m_strSericeName.c_str());
		return -1;
	}
	string strName=pKey;
	transform (strName.begin(), strName.end(), strName.begin(), (int(*)(int))tolower);
	return m_pMsgDeCoder->GetValues(strName,vecValue,eValueType);
}

int CAvenueMsgHandler::Encode()
{
	SDK_XLOG(XLOG_NOTICE, "CAvenueMsgHandler::%s [%s] Encode Message\n", __FUNCTION__,m_strSericeName.c_str());	
	return m_pMsgEnCoder->GetValidateCode();
}
void CAvenueMsgHandler::Reset()
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	m_pMsgEnCoder->Reset();
}
void CAvenueMsgHandler::SetValue(IN const char* pKey, IN const void * pValue, IN int nLen)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::SetValue message[%s] field[%s]\n", m_strSericeName.c_str(),pKey);
	if(pKey==NULL||pValue == NULL)
	{
		SDK_XLOG(XLOG_WARNING, "SetValue [%s]=[%x,%d] failed: no buffer\n", pKey,pValue, nLen);
		return;
	}
	else if(nLen == 0)
	{
		SDK_XLOG(XLOG_TRACE, "SetValue [%s]=[%x,%d] len=0: no buffer\n", pKey,pValue, nLen);
		return;
	}
	
	if(m_oServiceConfig == NULL || m_oMessage == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "CAvenueMsgHandler::SetValue can not find the service or msg[%s]\n", m_strSericeName.c_str());
		return;
	}
	string strName=pKey;
	transform (strName.begin(), strName.end(), strName.begin(), (int(*)(int))tolower);
	m_pMsgEnCoder->SetValue(strName, pValue, nLen);
}


void CAvenueMsgHandler::SetValue(const char* pKey, const char* pValue)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	if(pKey==NULL||pValue == NULL)
	{
		SDK_XLOG(XLOG_WARNING, "SetValue [%s]=[%s] failed: no buffer\n", pKey,pValue);
		return;
	}
	if(m_oServiceConfig == NULL || m_oMessage == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "CAvenueMsgHandler::SetValue can not find the service or msg[%s]\n", m_strSericeName.c_str());
		return;
	}
	const string & strServiceEncoding = m_oServiceConfig->oConfig.GetServiceEncoding();
	const string & strSdkEncoding = m_oServiceConfig->oConfig.GetSdkEncoding();
	if(strcmp(strServiceEncoding.c_str(), strSdkEncoding.c_str()) != 0)
	{
		string strValue;
		if(m_bRequest)
		{
			ConverterCoder_(strSdkEncoding, pValue, strServiceEncoding, strValue);
		}
		else 
		{
			ConverterCoder_(strServiceEncoding, pValue, strSdkEncoding, strValue);
		}
		SetValue(pKey, (void *)strValue.c_str(), strValue.length());
	}
	else 
	{
		SetValue(pKey, (void *)pValue, strlen(pValue));
	}
}


void CAvenueMsgHandler::SetValue(const char* pKey, int nValue)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	if(pKey==NULL)
	{
		SDK_XLOG(XLOG_WARNING, "SetValue [%s]=[%d] failed: no buffer\n", pKey,nValue);
		return;
	}
	if(m_oServiceConfig == NULL || m_oMessage == NULL)
	{
		SDK_XLOG(XLOG_ERROR, "CAvenueMsgHandler::SetValue can not find the service or msg[%s]\n", m_strSericeName.c_str());
		return;
	}
	
	SetValue(pKey, &nValue, 4);

}
void CAvenueMsgHandler::SetValues(IN const char* pKey, IN const vector<SStructValue> & vecValue)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::SetValues line[%d] name[%s] value[%d]\n",__LINE__, pKey, vecValue.size());
	if(vecValue.size()==0) return;
	if(vecValue.size()==1) 
	{
		SetValue(pKey, vecValue[0].pValue, vecValue[0].nLen);
		return;
	}
	char szName[128]={0};
	vector<SStructValue>::const_iterator itrVec;
	int i=0;
	for(itrVec = vecValue.begin(); itrVec != vecValue.end(); ++itrVec,++i)
	{
		snprintf(szName,128,"%s[%d]",pKey,i);
		const SStructValue & oValue = *itrVec;
		SetValue(szName, oValue.pValue, oValue.nLen);
	}
}
void CAvenueMsgHandler::SetTlvGroup(CAvenueMsgHandler *pHandler)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	map<string, SAvenueTlvTypeConfig> &mapDestMap=GetTlvMap();
	map<string, SAvenueTlvTypeConfig> &mapSrcMap=pHandler->GetTlvMap();
	map<string, SAvenueTlvTypeConfig> ::iterator itrDest,itrSrc;
	for(itrDest=mapDestMap.begin();itrDest!=mapDestMap.end();itrDest++)
	{
		const string & strName=itrDest->first;
		SAvenueTlvTypeConfig & oDestConfig=itrDest->second;
		itrSrc=mapSrcMap.find(strName);
		if(itrSrc!=mapSrcMap.end())
		{
			SAvenueTlvTypeConfig & oSrcConfig=itrSrc->second;
			if(oSrcConfig==oDestConfig)
			{
				vector<SStructValue> vecValue;
				if(pHandler->GetValues(strName.c_str(),vecValue)==0)
				{
					SetValues(strName.c_str(),vecValue);
				}
			}
		}
	}
}
void CAvenueMsgHandler::SetPhpSerializer(CPhpSerializer *pPhpSerializer)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	map<string, SAvenueTlvTypeConfig> * pMapTlvType;
	if(m_bRequest) 
	{
		pMapTlvType=&(m_oMessage->oRequestTlvRule.mapTlvType);
	}
	else 
	{
		pMapTlvType=&(m_oMessage->oResponseTlvRule.mapTlvType);
	}
	map<string, SAvenueTlvTypeConfig> ::iterator itrEncoder;
	for(itrEncoder=pMapTlvType->begin();itrEncoder!=pMapTlvType->end();itrEncoder++)
	{
		const string & strName=itrEncoder->first;
		SAvenueTlvTypeConfig & oDestConfig=itrEncoder->second;
		int nValue=0;
		string strValue;
		if(oDestConfig.emType==MSG_FIELD_INT
			&&!(oDestConfig.isArray)
			&&pPhpSerializer->GetValue(strName,nValue)==0)
		{
			SetValue(strName.c_str(),nValue);
		}
		else if(oDestConfig.emType==MSG_FIELD_STRING
			&&!(oDestConfig.isArray)
			&&pPhpSerializer->GetValue(strName,strValue)==0)
		{
			SetValue(strName.c_str(),strValue.c_str());
		}
		else if(oDestConfig.emType==MSG_FIELD_STRUCT
			&&!(oDestConfig.isArray))
		{
			map<string,SAvenueStructConfig>::iterator itrStruct;
			for(itrStruct=oDestConfig.mapStructConfig.begin();itrStruct!=oDestConfig.mapStructConfig.end();itrStruct++)
			{
				char szPath[128]={0};
				SAvenueStructConfig & oStructConfig=itrStruct->second;
				snprintf(szPath,128,"%s['%s']",strName.c_str(),oStructConfig.strName.c_str());
				if(oStructConfig.emStructType==MSG_FIELD_INT
					&&pPhpSerializer->GetValue(szPath,nValue)==0)
				{
					SetValue(szPath,nValue);
				}
				else if(oStructConfig.emStructType==MSG_FIELD_STRING
					&&pPhpSerializer->GetValue(szPath,strValue)==0)
				{
					SetValue(szPath,strValue.c_str());
				}
			}
		}
		else if(oDestConfig.emType==MSG_FIELD_INT
			&&oDestConfig.isArray)
		{
			int i=0;
			char szPath[128]={0};
			while(1)
			{
				snprintf(szPath,128,"%s[%d]",strName.c_str(),i);
				if(pPhpSerializer->GetValue(szPath,nValue)==0)
				{
					SetValue(strName.c_str(),nValue);
					i++;
				}
				else break;
			}
		}
		else if(oDestConfig.emType==MSG_FIELD_STRING
			&&oDestConfig.isArray)
		{
			int i=0;
			char szPath[128]={0};
			while(1)
			{
				snprintf(szPath,128,"%s[%d]",strName.c_str(),i);
				if(pPhpSerializer->GetValue(szPath,strValue)==0)
				{
					SetValue(strName.c_str(),strValue.c_str());
					i++;
				}
				else break;
			}
		}
		else if(oDestConfig.emType==MSG_FIELD_STRUCT
			&&oDestConfig.isArray)
		{
			int i=0;
			char szPath[128]={0};
			bool isFind=false;
			while(1)
			{
				isFind=false;
				map<string,SAvenueStructConfig>::iterator itrStruct;
				for(itrStruct=oDestConfig.mapStructConfig.begin();itrStruct!=oDestConfig.mapStructConfig.end();itrStruct++)
				{
					char szPath[128]={0};
					
					SAvenueStructConfig & oStructConfig=itrStruct->second;
					snprintf(szPath,128,"%s[%d]['%s']",strName.c_str(),i,oStructConfig.strName.c_str());
					if(oStructConfig.emStructType==MSG_FIELD_INT
						&&pPhpSerializer->GetValue(szPath,nValue)==0)
					{
						SetValue(szPath,nValue);
						isFind=true;
					}
					else if(oStructConfig.emStructType==MSG_FIELD_STRING
						&&pPhpSerializer->GetValue(szPath,strValue)==0)
					{
						SetValue(szPath,strValue.c_str());
						isFind=true;
					}
				}
				if(isFind)
				{
					i++;
				}
				else break;
			}
			
		}
	}
}
CAvenueMsgHandler::~CAvenueMsgHandler()
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	if(m_nBufferLen > 0)
	{
		free(m_pBuffer);
	}

	if(m_nExtHeadBufLen > 0)
	{
		free(m_pExtHeadBuf);
	}
	if(m_pMsgDeCoder!=NULL)
	{
		delete m_pMsgDeCoder;
	}
	if(m_pMsgEnCoder!=NULL)
	{
		delete m_pMsgEnCoder;
	}
}

void CAvenueMsgHandler::Release()
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	delete this;
}

void CAvenueMsgHandler::ConverterCoder_(const string & strSrcEncoding, const string & strSrc, const string & strDstEncoding, string &strDst)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::ConverterCoder_ fromcode[%s] from[%s] destcode[%s]\n",
			strSrcEncoding.c_str(), strSrc.c_str(), strDstEncoding.c_str());
	
#ifdef WIN32
	if(strSrc.length() > 0)
	{
		int iDstLen = strSrc.length() * 6 + 1;
		WCHAR *pwch = (WCHAR *)malloc(iDstLen);
		MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(), -1, pwch, iDstLen);

		char *pDest = (char *)malloc(iDstLen);
		WideCharToMultiByte(CP_UTF8, 0, pwch, -1, pDest, iDstLen, 0, 0);
		strDst = pDest;

		free(pwch);
		free(pDest);
	}
	else 
	{
		strDst = "";
	}
#else
	if(strSrc.length() > 0)
	{
		int iDstLen = strSrc.length() * 6 + 1;
		char *pDest = (char *)malloc(iDstLen);
		CodeConverter code = CodeConverter(strSrcEncoding.c_str(), strDstEncoding.c_str());

		int nResult = code.Convert((char*)(strSrc.c_str()), strSrc.length(), pDest, &iDstLen);
		if(nResult != -1)
		{
			strDst = string(pDest, iDstLen);	
		}
		else if(nResult == -1 && strSrcEncoding.compare("gbk") == 0)
		{
			int iDstLen = strSrc.length() * 6 + 1;
			CodeConverter tmp = CodeConverter("GB18030", strDstEncoding.c_str());
			nResult = tmp.Convert((char*)(strSrc.c_str()), strSrc.length(), pDest, &iDstLen);
			if(nResult != -1)
			{
				strDst = string(pDest, iDstLen);	
			}
			else 
			{
				Utf82Ascii oUtoG;
				int nLen = oUtoG.Convert((char*)(strSrc.c_str()), strSrc.length(), pDest, iDstLen);
				strDst = string(pDest, nLen);
			}
		}
		else 
		{
			strDst = "";
		}
		free(pDest);
	}
	else 
	{
		strDst = "";
	}
#endif
	SDK_XLOG(XLOG_TRACE, "dest[%s] len[%d]\n", strDst.c_str(), strDst.length());
}


void CAvenueMsgHandler::SetExtHead(IN const void * pExtHead, IN int nLen)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s head[%x], len[%d]\n", __FUNCTION__, pExtHead, nLen);
	if(pExtHead == NULL || nLen == 0)
	{
		return;
	} 

	
	if(m_nExtHeadBufLen - m_nExtHeadLen < nLen)
	{
		int nAllocSize = SDO_ALIGN(nLen, 1024);
		m_nExtHeadBufLen += nAllocSize;
		m_pExtHeadBuf = realloc(m_pExtHeadBuf, m_nExtHeadBufLen);
	}

	memcpy((char *)m_pExtHeadBuf + m_nExtHeadLen, pExtHead, nLen);
	m_nExtHeadLen += nLen;
}

void CAvenueMsgHandler::GetExtHead(OUT void ** pExtHead, OUT int * pLen)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s\n", __FUNCTION__);
	*pExtHead = (void *)m_pMsgEnCoder->GetBuffer();
	*pLen = m_pMsgEnCoder->GetLen();

}

char * CAvenueMsgHandler::GetServiceName()
{
	return (char  *)m_strSericeName.c_str();
}
EValueType CAvenueMsgHandler::GetType(IN const char* pKey)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	char szName[64] = {0};
	char szStructField[64] = {0};
	int nArrayIndex=-1;
	int nNum = sscanf(pKey,"%63[^'['][%d]['%63[^''']']",szName,&nArrayIndex,szStructField);
	if(nNum != 3) 
	{
		sscanf(pKey,"%63[^'[']['%63[^''']']",szName,szStructField);
	}
	SAvenueTlvGroupRule *pTlvRule;
	if(m_bRequest)
	{
		pTlvRule=&(m_oMessage->oRequestTlvRule);
	}
	else
	{
		pTlvRule=&(m_oMessage->oResponseTlvRule);
	}

	map<string,SAvenueTlvTypeConfig>::const_iterator itr=pTlvRule->mapTlvType.find(szName);
	if(itr==pTlvRule->mapTlvType.end())
	{
		SDK_XLOG(XLOG_WARNING, "CAvenueMsgHandler::GetType [%s]  failed: no typecode\n", pKey);
		return MSG_FIELD_NULL;
	}
	const SAvenueTlvTypeConfig &oConfig=itr->second;
	if(szStructField[0]!='\0'&&oConfig.emType==MSG_FIELD_STRUCT)
	{
		map<string,SAvenueStructConfig>::const_iterator itrStruct=oConfig.mapStructConfig.find(szStructField);
		if(itrStruct==oConfig.mapStructConfig.end())
		{
			SDK_XLOG(XLOG_WARNING, "CAvenueMsgHandler::GetType [%s]  failed: buffer no this struct field \n",pKey);
			return MSG_FIELD_NULL;
		}

		const SAvenueStructConfig &oStructConfig=itrStruct->second;
		return oStructConfig.emStructType;
	}
	else
	{
		return oConfig.emType;
	}
}
void CAvenueMsgHandler::SetValueNoCaseType(IN const char * pKey, IN const char *pValue)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s line[%d]\n", __FUNCTION__,__LINE__);
	if(pKey == NULL || pValue == NULL)
	{
		return;
	}
	string szKey=pKey;
	boost::to_lower(szKey); 
	if (GetType(szKey.c_str())==MSG_FIELD_INT)
	{
		int nValue = atoi(pValue);
		SetValue(pKey,nValue);
	}
	else
	{
		SetValue(pKey,pValue);
	}
}

void CAvenueMsgHandler::Dump()
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s\n",__FUNCTION__);
}

void CAvenueMsgHandler::GetMsgParam(string &strParam)
{
	SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s bReuqest[%d]\n",__FUNCTION__, (int)m_bRequest);
	void *pBuffer=NULL;
	int   nLen = 0;
	SAvenueTlvGroupRule *pTlvRule;
	vector<SAvenueTlvFieldConfig>* vecParas;
	if(m_bRequest)
	{
		pTlvRule=&(m_oMessage->oRequestTlvRule);
		vecParas=&(m_oMessage->vecRequest);
	}
	else
	{
		pTlvRule=&(m_oMessage->oResponseTlvRule);
		vecParas=&(m_oMessage->vecResponse);
	}
	
	if(m_bRequest==true)
	{
		pBuffer = (void *)GetBuffer();
		nLen = GetLen();
		Decode(pBuffer,nLen);
	}
	vector<SAvenueTlvFieldConfig>::iterator itField = vecParas->begin();
	for(; itField!=vecParas->end();itField++)
	{
		map<string,SAvenueTlvTypeConfig>::const_iterator itr = pTlvRule->mapTlvType.find(itField->strName);
		if (itr==pTlvRule->mapTlvType.end())
		{
			SDK_XLOG(XLOG_WARNING, "CAvenueMsgHandler::%s no %s\n",__FUNCTION__,itField->strName.c_str());
			strParam+="^_^";
			continue;
		}
		
		const SAvenueTlvTypeConfig &oConfig=itr->second;
		void *pValue;
		unsigned int nLen=0;
		EValueType valueType;
		if(oConfig.emType == MSG_FIELD_INT)
		{
			//SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s MSG_FIELD_INT\n",__FUNCTION__);
			if(m_pMsgDeCoder->GetValue(itField->strName, &pValue, &nLen,valueType)==0&&nLen==4)
			{
				char szTmp[32];
				snprintf(szTmp, 32, "%d", *(int *)pValue);
				strParam+=string(szTmp)+"^_^";
			}
			else 
			{
				strParam+="^_^";
			}
		}
		else if(oConfig.emType == MSG_FIELD_STRING)
		{	
			vector<SStructValue> vecValues;
			//SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s MSG_FIELD_STRING\n",__FUNCTION__);
			if(m_pMsgDeCoder->GetValues(itField->strName, vecValues, valueType)==0)
			{
				strParam +=GetStringArrayLog(vecValues)+"^_^";
			}
			else 
			{
				strParam+="^_^";
			}
		}
		else if(oConfig.emType == MSG_FIELD_STRUCT)
		{
			//SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s MSG_FIELD_STRUCT\n",__FUNCTION__);
			vector<SStructValue> vecValues;
			
			if(m_pMsgDeCoder->GetValues(itField->strName, vecValues, valueType)==0)
			{
				//SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s size[%d]\n",__FUNCTION__,vecValues.size());
				for(vector<SStructValue>::iterator itrVec=vecValues.begin();itrVec!=vecValues.end();itrVec++)
				{
					SStructValue& sv = *itrVec;
					string strLog="";
					//SDK_XLOG(XLOG_TRACE, "CAvenueMsgHandler::%s [%d]\n",__FUNCTION__,sv.nLen);
					if (sv.nLen>0)
					{
						//SDK_XLOG(XLOG_TRACE, "\tCAvenueMsgHandler::%s size[%d]\n",__FUNCTION__,oConfig.mapStructConfig.size());
						map<string,SAvenueStructConfig>::const_iterator itrStruct=oConfig.mapStructConfig.begin();
						while(itrStruct!=oConfig.mapStructConfig.end())
						{
							const SAvenueStructConfig &oStructConfig=itrStruct->second;
							int eValueType=oStructConfig.emStructType;
							char* pBuffer=(char *)(sv.pValue)+oStructConfig.nLoc;
							int nStructItemLen;
							if(oStructConfig.nLen==-1)
							{
								nStructItemLen=sv.nLen-oStructConfig.nLoc;
							}
							else
							{
								nStructItemLen=oStructConfig.nLen;
							}
							//SDK_XLOG(XLOG_TRACE, "\tCAvenueMsgHandler::%s type[%d] length[%d]\n",__FUNCTION__,eValueType,nStructItemLen);
							if (eValueType == MSG_FIELD_INT)
							{
								char szTmp[20]={0};
								snprintf(szTmp,sizeof(szTmp),"%d",*(int*)(pBuffer));
								strLog+=szTmp;
							}
							else if (eValueType == MSG_FIELD_STRING)
							{
								if (nStructItemLen!=0)
								{
									string strTmp=string(pBuffer,nStructItemLen);
									strLog+=strTmp.c_str();
								}
							}
							itrStruct++;
							if (itrStruct!=oConfig.mapStructConfig.end())
							{
								strLog +=",";
							}
						}
					}
					strParam +=strLog;
					if (vecValues.size()!=1)
						strParam+="|";
				}
			}
			strParam+="^_^";
		}
	}
}

string CAvenueMsgHandler::GetStringArrayLog(vector<SStructValue>& vecValues)
{
	string strLog="";
	if (vecValues.size()>1)
	{
		for(vector<SStructValue>::iterator itr=vecValues.begin();itr!=vecValues.end();itr++)
		{
			SStructValue& sv = *itr;
			if (sv.nLen>0)
			{
				string strTmp=string((char *)sv.pValue, sv.nLen);
				strLog+=strTmp.c_str();
			}
			strLog+="|";
		}
	}
	else if (vecValues.size()==1)
	{
		vector<SStructValue>::iterator itr=vecValues.begin();
		SStructValue& sv = *itr;
		if (sv.nLen>0)
		{
			string strTmp=string((char *)sv.pValue, sv.nLen);
			strLog+=strTmp.c_str();
		}
	}
	return strLog;
}
}
}
