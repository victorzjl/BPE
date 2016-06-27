#include "cJSON.h"
#include "jsonMsg.h"
#include "stdio.h"

CJsonEncoder::CJsonEncoder( )
{ 
	m_root = (void *)cJSON_CreateObject();
}

CJsonEncoder::~CJsonEncoder()
{
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey,const string& strValue)
{
   int nRet = 0;
   if(strParentKey.empty())
   {
	   cJSON_AddStringToObject((cJSON *)m_root, strKey.c_str(), strValue.c_str());
   }
   else 
   {
		cJSON * pParent = cJSON_GetObjectItem((cJSON *)m_root, strParentKey.c_str());
		if(pParent)
		{
			cJSON_AddStringToObject(pParent, strKey.c_str(), strValue.c_str());
		}
		else 
		{
			nRet = -1;
		}
   }
   return nRet;
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey,const vector<string>& vecStrValue)
{
	char **pStrArray = new char*[vecStrValue.size()];
	int i=0;
	vector<string>::const_iterator iter;
	for(iter = vecStrValue.begin(); iter != vecStrValue.end(); iter++)
	{
		pStrArray[i++] = (char *)iter->c_str();
	}
	cJSON *pArray = cJSON_CreateStringArray((const char **)pStrArray, vecStrValue.size());
	
    int nRet = 0;
	if(strParentKey.empty())
	{
		cJSON_AddItemToObject((cJSON *)m_root, strKey.c_str(), pArray);
	}
	else 
	{
		cJSON * pParent = cJSON_GetObjectItem((cJSON *)m_root, strParentKey.c_str());
		if(pParent)
		{
			cJSON_AddItemToObject(pParent, strKey.c_str(), pArray);
		}
		else 
		{
			nRet = -1;
		}
	}
	delete[] pStrArray;
	return nRet;
}


int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, double dValue)
{
	int nRet = 0;
	if(strParentKey.empty())
	{
		cJSON_AddNumberToObject((cJSON *)m_root, strKey.c_str(), dValue);
	}
	else 
	{
		cJSON * pParent = cJSON_GetObjectItem((cJSON *)m_root, strParentKey.c_str());
		if(pParent)
		{
			cJSON_AddNumberToObject(pParent, strKey.c_str(), dValue);
		}
		else 
		{
			nRet = -1;
		}
	}
	return nRet;
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder)
{
	int nRet = 0;
	cJSON* pJson =(cJSON*)encoder.GetJsonObj();
	if(strParentKey.empty())
	{
		cJSON_AddItemToObject((cJSON *)m_root, strKey.c_str(), pJson);
	}
	else 
	{
		cJSON * pParent = cJSON_GetObjectItem((cJSON *)m_root, strParentKey.c_str());
		if(pParent)
		{
			cJSON_AddItemToObject(pParent, strKey.c_str(), pJson);
		}
		else 
		{
			nRet = -1;
		}
	}
	return nRet;
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder)
{
	cJSON *pJson = cJSON_CreateArray();
	vector<CJsonEncoder>::const_iterator iter;
	for(iter = vecEncoder.begin(); iter != vecEncoder.end(); iter++)
	{
		const CJsonEncoder &encoder = *iter;
		cJSON_AddItemToArray(pJson, (cJSON *)encoder.GetJsonObj());
	}

    int nRet = 0;
	if(strParentKey.empty())
	{
		cJSON_AddItemToObject((cJSON *)m_root, strKey.c_str(), pJson);
	}
	else 
	{
		cJSON * pParent = cJSON_GetObjectItem((cJSON *)m_root, strParentKey.c_str());
		if(pParent)
		{
			cJSON_AddItemToObject(pParent, strKey.c_str(), pJson);
		}
		else 
		{
			nRet = -1;
		}
	}
	return nRet;
}

void CJsonEncoder::GetJsonString(string &strJson)
{
	char *jsonStr = cJSON_PrintUnformatted((cJSON *)m_root);
	strJson = jsonStr;
	free(jsonStr);
}

void CJsonEncoder::Destroy()
{
	if(m_root)
	{
		cJSON_Delete((cJSON *)m_root);
	}
}
////////////////////////////////////////////////////////





CJsonDecoder::CJsonDecoder()
{
   m_root = NULL;
}

CJsonDecoder::CJsonDecoder(const void * pBuffer, unsigned int nLen)
{
	Decoder(pBuffer, nLen);
}

void CJsonDecoder::Decoder(const void * pBuffer, unsigned int nLen)
{
	m_root = (cJSON *)cJSON_Parse((const char *)pBuffer);
}

CJsonDecoder::~CJsonDecoder()
{
}

/*Get attribute*/
int CJsonDecoder::GetValue(const string& strKey, int& nValue) 
{
	if (NULL == m_root)
	{
		return -1;
	}

 	cJSON *pJson = cJSON_GetObjectItem((cJSON *)m_root, strKey.c_str());

	if(!pJson)
	{
		return -2;
	}


	if ( cJSON_Number == pJson->type)
	{
		nValue = pJson->valueint;
	}
	else if(cJSON_False == pJson->type)
	{
		nValue = 0;
	}
	else if(cJSON_True == pJson->type)
	{
		nValue = 1;
	}
	else
	{
		return -3;
	}
    return 0;
}


int CJsonDecoder::GetValue(const string& strKey, string& strValuen) 
{
	if (NULL == m_root)
	{
		return -1;
	}

 	cJSON *pJson = cJSON_GetObjectItem((cJSON *)m_root, strKey.c_str());

	if(!pJson)
	{
		return -2;
	}

	switch(pJson->type)
	{
	case cJSON_False:
		strValuen = "0";
		break;
	case cJSON_True:
		strValuen = "1";
		break;
	case cJSON_String:
		strValuen = pJson->valuestring;
		break;
		
	case cJSON_Array:
		{
			int nSize = cJSON_GetArraySize(pJson);
			//cJSON *pTmp=cJSON_GetArrayItem(pJson,0);
			char* szTmp=cJSON_PrintUnformatted(pJson);
			strValuen = szTmp;
			free(szTmp);
			//printf("szie = %d %s\n", nSize,strValuen.c_str());
		}
		break;
		
	case cJSON_Number:
		{
			char szValue[32] = {0};
			snprintf(szValue, sizeof(szValue)-1, "%lld", static_cast<long long>(pJson->valuedouble));
			strValuen = szValue;
		}
		break;
	default:
		return -3;
	}
	
    return 0;
}

int CJsonDecoder::GetValue(const string& strKey, vector<string> &vecValues)
{
	if (NULL == m_root)
	{
		return -1;
	}

    int nRet = 0;
 	cJSON *pJson = cJSON_GetObjectItem((cJSON *)m_root, strKey.c_str());
	if(pJson)
	{
		vecValues.clear();
		for(int i=0; i < cJSON_GetArraySize(pJson); i++) 
		{    
			cJSON *pJsonArray = cJSON_GetArrayItem(pJson, i);
			if (pJsonArray)
			{
				vecValues.push_back(pJsonArray->valuestring);
			}
		}
	}
	else 
	{
		nRet = -2;
	}
    return nRet;
}
int CJsonDecoder::GetValue(const string& strKey, CJsonDecoder& decoder) 
{
	if (NULL == m_root)
	{
		return -1;
	}

	int nRet = 0;
	cJSON *pJson = cJSON_GetObjectItem((cJSON *)m_root, strKey.c_str());
	if(pJson)
	{
		//if (pJson->type == cJSON_Array)
		//{
			//string strValuen = cJSON_PrintUnformatted(pJson);
			//printf("json %p %s\n", pJson, strValuen.c_str());
			//cJSON *pTmp=cJSON_GetArrayItem(pJson,0);
			//if (pTmp!=NULL)
			//	pJson = pTmp;
		//}
		decoder.SetJsonObj((void *)pJson);
	}
	else 
	{
		nRet = -2;
	}
    return 0;
}

int CJsonDecoder::GetValue(const string& strKey, vector<CJsonDecoder>& vecDecoder)
{
	if (NULL == m_root)
	{
		return -1;
	}

	int nRet = 0;
	cJSON *pJson = cJSON_GetObjectItem((cJSON *)m_root, strKey.c_str());
	if(pJson)
	{
		vecDecoder.clear();
		for(int i=0; i < cJSON_GetArraySize(pJson); i++) 
		{    
			cJSON *pJsonArray = cJSON_GetArrayItem(pJson, i);
			CJsonDecoder decoder;
			decoder.SetJsonObj((void *)pJsonArray);
			vecDecoder.push_back(decoder);
		}
	}
	else 
	{
		nRet = -2;
	}
    return nRet;
}

void CJsonDecoder::Destroy()
{
	if(m_root)
	{
		cJSON_Delete((cJSON *)m_root);
	}
}


