#include "jsonlib/json.h"
#include "CJson.h"

CJsonEncoder::CJsonEncoder( )
{ 
	m_root = (void *)json_object_new_object();
}

CJsonEncoder::~CJsonEncoder()
{
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey,const int nValue){
	int nRet = 0;
	if(strParentKey.empty())
	{
		 json_object_object_add((json_object*)m_root, strKey.c_str(), json_object_new_int(nValue));
	}
	else 
	{
		 json_object* pParent = json_object_object_get((json_object*)m_root, strParentKey.c_str());
		 if(pParent)
		 {
			 json_object_object_add(pParent, strKey.c_str(), json_object_new_int(nValue));
		 }
		 else 
		 {
			 nRet = -1;
		 }
	}
	return nRet;
}



int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey,const string& strValue)
{
   int nRet = 0;
   if(strParentKey.empty())
   {
		json_object_object_add((json_object*)m_root, strKey.c_str(), json_object_new_string(strValue.c_str()));
   }
   else 
   {
		json_object* pParent = json_object_object_get((json_object*)m_root, strParentKey.c_str());
		if(pParent)
		{
			json_object_object_add(pParent, strKey.c_str(), json_object_new_string(strValue.c_str()));
		}
		else 
		{
			nRet = -1;
		}
   }
   return nRet;
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey,const vector<int>& vecIntValue)
{
	json_object* pArray = json_object_new_array();  
	
	vector<int>::const_iterator iter;
	for(iter = vecIntValue.begin(); iter != vecIntValue.end(); iter++)
	{
		int nTemp = *iter;
		json_object_array_add(pArray, json_object_new_int(nTemp));  
	}
	
    int nRet = 0;
	if(strParentKey.empty())
	{
		json_object_object_add((json_object*)m_root, strKey.c_str(), pArray);
	}
	else 
	{
		json_object* pParent = json_object_object_get((json_object*)m_root, strParentKey.c_str());
		if(pParent)
		{
			json_object_object_add(pParent, strKey.c_str(), pArray);
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
	json_object* pArray = json_object_new_array();  
	
	vector<string>::const_iterator iter;
	for(iter = vecStrValue.begin(); iter != vecStrValue.end(); iter++)
	{
		const string& strTemp = *iter;
		json_object_array_add(pArray, json_object_new_string(strTemp.c_str()));  
	}
	
    int nRet = 0;
	if(strParentKey.empty())
	{
		json_object_object_add((json_object*)m_root, strKey.c_str(), pArray);
	}
	else 
	{
		json_object* pParent = json_object_object_get((json_object*)m_root, strParentKey.c_str());
		if(pParent)
		{
			json_object_object_add(pParent, strKey.c_str(), pArray);
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
	json_object* pJson =(json_object*)encoder.GetJsonObj();
	if(strParentKey.empty())
	{
		json_object_object_add((json_object*)m_root, strKey.c_str(), pJson);
	}
	else 
	{
		json_object* pParent = json_object_object_get((json_object*)m_root, strParentKey.c_str());
		if(pParent)
		{
			json_object_object_add(pParent, strKey.c_str(), pJson);
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
	json_object* pArray = json_object_new_array();  
	
	vector<CJsonEncoder>::const_iterator iter;
	for(iter = vecEncoder.begin(); iter != vecEncoder.end(); iter++)
	{
		const CJsonEncoder &encoder = *iter;
		json_object_array_add(pArray, (json_object*)encoder.GetJsonObj());  
	}
	
    int nRet = 0;
	if(strParentKey.empty())
	{
		json_object_object_add((json_object*)m_root, strKey.c_str(), pArray);
	}
	else 
	{
		json_object* pParent = json_object_object_get((json_object*)m_root, strParentKey.c_str());
		if(pParent)
		{
			json_object_object_add(pParent, strKey.c_str(), pArray);
		}
		else 
		{
			nRet = -1;
		}
	}
	return nRet;
}

//void* CJsonEncoder::GetJsonString()
//{	
//	const char *t_JsonString = json_object_to_json_string((json_object*)m_root);
//	json_escape_str(m_Buf,t_JsonString,strlen(t_JsonString));
//	return (void *)m_Buf->buf;
//}
void* CJsonEncoder::GetJsonString()
{	
	return (void *)json_object_to_json_string((json_object*)m_root);
}

int CJsonEncoder::GetJsonStringLen()
{
	int nLen = 0;
	const char* pStr = (const char*)GetJsonString();
	if(pStr)
	{
		nLen = strlen(pStr);
	}
	return nLen;
}

void CJsonEncoder::DestroyJsonEncoder()
{
	json_object_put((json_object *)m_root);
}




CJsonDecoder::CJsonDecoder()
{
   m_root = NULL;
}

CJsonDecoder::CJsonDecoder(const void * pBuffer, unsigned int nLen)
{
   m_root = (json_object*)json_tokener_parse((const char *)pBuffer);
}
CJsonDecoder::~CJsonDecoder()
{
}
void CJsonDecoder::DestroyJsonDecoder()
{
	json_object_put((json_object *)m_root);
}

int CJsonDecoder::GetValue(const string& strKey, int& nValue) 
{
	int nRet = 0;
 	json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
	if(pJson)
	{
		nValue = json_object_get_int(pJson);
	}
	else 
	{
		nRet = -1;
	}
    return nRet;
}


/*Get attribute*/
int CJsonDecoder::GetValue(const string& strKey, string& strValuen) 
{
	int nRet = 0;
 	json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
	if(pJson)
	{
		char *pValue = (char *)json_object_get_string(pJson);
		strValuen = pValue;
	}
	else 
	{
		nRet = -1;
	}
    return nRet;
}

int CJsonDecoder::GetValue(const string& strKey, vector<string> &vecValues)
{
    int nRet = 0;
 	json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
	if(pJson)
	{
		for(int i=0; i < json_object_array_length(pJson); i++) 
		{    
			json_object *pTemp = json_object_array_get_idx(pJson, i);    
			char *pValue = (char *)json_object_get_string(pTemp);
			vecValues.push_back(pValue);
		}
	}
	else 
	{
		nRet = -1;
	}
    return nRet;
}

int CJsonDecoder::GetValue(const string& strKey, CJsonDecoder& decoder) 
{
    int nRet = 0;
 	json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
	if(pJson)
	{
		decoder.SetJsonObj((void *)pJson);
	}
	else 
	{
		nRet = -1;
	}
    return 0;
}

int CJsonDecoder::GetValue(const string& strKey, vector<CJsonDecoder>& vecDecoder)
{
    int nRet = 0;
 	json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
	if(pJson)
	{
		for(int i=0; i < json_object_array_length(pJson); i++) 
		{    
			json_object *pTemp = json_object_array_get_idx(pJson, i);    
			CJsonDecoder decoder;
			decoder.SetJsonObj((void *)pTemp);
			vecDecoder.push_back(decoder);
		}
	}
	else 
	{
		nRet = -1;
	}
    return nRet;
}

int CJsonDecoder::GetValue(const string& strKey, vector<int> &vecValues)
{
    int nRet = 0;
 	json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
	printf("--- %p [%s] %p\n", m_root, strKey.c_str(), pJson);
	if(pJson)
	{
		for(int i=0; i < json_object_array_length(pJson); i++) 
		{    
			json_object *pTemp = json_object_array_get_idx(pJson, i);    
			int value = json_object_get_int(pTemp);
			vecValues.push_back(value);
		}
	}
	else 
	{
		nRet = -1;
	}
    return nRet;
}

