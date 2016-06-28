#include "CJson.h"

CJsonEncoder::CJsonEncoder(EJsonTextType type)
    : m_jsonTextType(type)
{ 
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        m_root = (void *)json_object_new_array();
    }
    else
    {
        m_root = (void *)json_object_new_object();
    }
}

CJsonEncoder::~CJsonEncoder()
{
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const int nValue, int nIndex)
{
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        return SetValue_Array(strParentKey, strKey, nValue, nIndex);
    }
    else
    {
        return SetValue_Object(strParentKey, strKey, nValue, nIndex);
    }
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const string& strValue, int nIndex)
{
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        return SetValue_Array(strParentKey, strKey, strValue, nIndex);
    }
    else
    {
        return SetValue_Object(strParentKey, strKey, strValue, nIndex);
    }
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const vector<int>& vecIntValue, int nIndex)
{
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        return SetValue_Array(strParentKey, strKey, vecIntValue, nIndex);
    }
    else
    {
        return SetValue_Object(strParentKey, strKey, vecIntValue, nIndex);
    }
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue, int nIndex)
{
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        return SetValue_Array(strParentKey, strKey, vecStrValue, nIndex);
    }
    else
    {
        return SetValue_Object(strParentKey, strKey, vecStrValue, nIndex);
    }
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder, int nIndex)
{
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        return SetValue_Array(strParentKey, strKey, encoder, nIndex);
    }
    else
    {
        return SetValue_Object(strParentKey, strKey, encoder, nIndex);
    }
}

int CJsonEncoder::SetValue(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder, int nIndex)
{
    if (JSON_TEXT_ARRAY == m_jsonTextType)
    {
        return SetValue_Array(strParentKey, strKey, vecEncoder, nIndex);
    }
    else
    {
        return SetValue_Object(strParentKey, strKey, vecEncoder, nIndex);
    }
}

void *CJsonEncoder::GetParentJsonObj(const string& strParentKey) 
{
    json_object* pParent = (json_object*)m_root;

    if (!strParentKey.empty())
    {
        pParent = json_object_object_get(pParent, strParentKey.c_str());
    }
    
    return (void *)pParent;
}

int CJsonEncoder::SetElement(const string& strKey, int nIndex, void *pParent, void *pObj)
{
    json_object *pParentJson = (json_object*)pParent;
    json_object *pObjJson = (json_object*)pObj;
    json_type type = json_object_get_type(pParentJson);
    if (json_type_object == type)
    {
        json_object_object_add(pParentJson, strKey.c_str(), pObjJson);
    }
    else if (json_type_array == type)
    {
        SetArrayElement(nIndex, pParent, pObj);
    }

    return 0;
}

int CJsonEncoder::SetArrayElement(int nIndex, void *pParent, void *pObj)
{
    json_object *pParentJson = (json_object*)pParent;
    json_object *pObjJson = (json_object*)pObj;
    if (-1 == nIndex)
    {
        json_object_array_add(pParentJson, pObjJson);
    }
    else
    {
        json_object_array_put_idx(pParentJson, nIndex, pObjJson);
    }
    return 0;
}

int CJsonEncoder::SetValue_Object(const string& strParentKey, const string& strKey, const int nValue, int nIndex)
{
    int nRet = -1;

    json_object* pParent = (json_object*)GetParentJsonObj(strParentKey);
    if (NULL != pParent)
    {
        nRet = SetElement(strKey, nIndex, pParent, json_object_new_int(nValue));
    }
    return nRet;
}



int CJsonEncoder::SetValue_Object(const string& strParentKey, const string& strKey, const string& strValue, int nIndex)
{
    int nRet = -1;

    json_object* pParent = (json_object*)GetParentJsonObj(strParentKey);
    if (NULL != pParent)
    {
        nRet = SetElement(strKey, nIndex, pParent, json_object_new_string(strValue.c_str()));
    }

    return nRet;
}

int CJsonEncoder::SetValue_Object(const string& strParentKey, const string& strKey, const vector<int>& vecIntValue, int nIndex)
{
    int nRet = -1;

    json_object* pParent = (json_object*)GetParentJsonObj(strParentKey);
    if (NULL != pParent)
    {
        json_object* pArray = json_object_new_array();

        vector<int>::const_iterator iter;
        for (iter = vecIntValue.begin(); iter != vecIntValue.end(); iter++)
        {
            int nTemp = *iter;
            json_object_array_add(pArray, json_object_new_int(nTemp));
        }

        nRet = SetElement(strKey, nIndex, pParent, pArray);
    }

    return nRet;
}


int CJsonEncoder::SetValue_Object(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue, int nIndex)
{
    int nRet = -1;

    json_object* pParent = (json_object*)GetParentJsonObj(strParentKey);
    if (NULL != pParent)
    {
        json_object* pArray = json_object_new_array();

        vector<string>::const_iterator iter;
        for (iter = vecStrValue.begin(); iter != vecStrValue.end(); iter++)
        {
            const string& strTemp = *iter;
            json_object_array_add(pArray, json_object_new_string(strTemp.c_str()));
        }

        nRet = SetElement(strKey, nIndex, pParent, pArray);
    }

    return nRet;
}

int CJsonEncoder::SetValue_Object(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder, int nIndex)
{

    int nRet = -1;

    json_object* pParent = (json_object*)GetParentJsonObj(strParentKey);
    if (NULL != pParent)
    {
        nRet = SetElement(strKey, nIndex, pParent, encoder.GetJsonObj());
    }

    return nRet;
}

int CJsonEncoder::SetValue_Object(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder, int nIndex)
{

    int nRet = -1;

    json_object* pParent = (json_object*)GetParentJsonObj(strParentKey);
    if (NULL != pParent)
    {
        json_object* pArray = json_object_new_array();

        vector<CJsonEncoder>::const_iterator iter;
        for (iter = vecEncoder.begin(); iter != vecEncoder.end(); iter++)
        {
            const CJsonEncoder &encoder = *iter;
            json_object_array_add(pArray, (json_object*)encoder.GetJsonObj());
        }

        nRet = SetElement(strKey, nIndex, pParent, pArray);
    }

    return nRet;
}

int CJsonEncoder::SetValue_Array(const string& strParentKey, const string& strKey, const int nValue, int nIndex)
{
    int nRet = 0;

    if (NULL != m_root)
    {
        nRet = SetArrayElement(nIndex, m_root, json_object_new_int(nValue));
    }
    
    return nRet;
}

int CJsonEncoder::SetValue_Array(const string& strParentKey, const string& strKey, const string& strValue, int nIndex)
{
    int nRet = 0;
    if (NULL != m_root)
    {
        nRet = SetArrayElement(nIndex, m_root, json_object_new_string(strValue.c_str()));
    }
    return nRet;
}

int CJsonEncoder::SetValue_Array(const string& strParentKey, const string& strKey, const vector<int>& vecIntValue, int nIndex)
{
    int nRet = 0;

    if (NULL != m_root)
    {
        json_object* pArray = json_object_new_array();

        vector<int>::const_iterator iter;
        for (iter = vecIntValue.begin(); iter != vecIntValue.end(); iter++)
        {
            int nTemp = *iter;
            json_object_array_add(pArray, json_object_new_int(nTemp));
        }

        nRet = SetElement(strKey, nIndex, m_root, pArray);
    }

    return nRet;
}

int CJsonEncoder::SetValue_Array(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue, int nIndex)
{
    int nRet = 0;

    if (NULL != m_root)
    {
        json_object* pArray = json_object_new_array();

        vector<string>::const_iterator iter;
        for (iter = vecStrValue.begin(); iter != vecStrValue.end(); iter++)
        {
            const string& strTemp = *iter;
            json_object_array_add(pArray, json_object_new_string(strTemp.c_str()));
        }

        nRet = SetElement(strKey, nIndex, m_root, pArray);
    }

    return nRet;
}

int CJsonEncoder::SetValue_Array(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder, int nIndex)
{
    int nRet = 0;

    if (NULL != m_root)
    {
        nRet = SetElement(strKey, nIndex, m_root, encoder.GetJsonObj());
    }

    return nRet;
}

int CJsonEncoder::SetValue_Array(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder, int nIndex)
{
    int nRet = 0;

    if (NULL != m_root)
    {
        json_object* pArray = json_object_new_array();

        vector<CJsonEncoder>::const_iterator iter;
        for (iter = vecEncoder.begin(); iter != vecEncoder.end(); iter++)
        {
            const CJsonEncoder &encoder = *iter;
            json_object_array_add(pArray, (json_object*)encoder.GetJsonObj());
        }

        nRet = SetElement(strKey, nIndex, m_root, pArray);
    }

    return nRet;
}

void* CJsonEncoder::GetJsonString() const
{ 
    return (void *)json_object_to_json_string((json_object *)m_root);
}

int CJsonEncoder::GetJsonStringLen() const
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
    if (NULL != m_root)
    {
        json_object_put((json_object *)m_root);
        m_root = NULL;
    }
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
    if (NULL != m_root)
    {
        json_object_put((json_object *)m_root);
        m_root = NULL;
    }
}

EJsonValueType CJsonDecoder::GetType() const
{
    EJsonValueType eType = JSONTYPE_NULL;
    if (NULL != m_root)
    {
        eType = (EJsonValueType)json_object_get_type((json_object *)m_root);
    }
    return eType;
}

int CJsonDecoder::GetValue(const string& strKey, int& nValue) const
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
int CJsonDecoder::GetValue(const string& strKey, string& strValuen) const
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

int CJsonDecoder::GetValue(const string& strKey, vector<string> &vecValues) const
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
int CJsonDecoder::GetValue(const string& strKey, CJsonDecoder& decoder) const
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

int CJsonDecoder::GetValue(const string& strKey, vector<CJsonDecoder>& vecDecoder) const
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

int CJsonDecoder::GetValue(vector<CJsonDecoder>& vecDecoder) const
{
    int nRet = 0;
    array_list* array = json_object_get_array((json_object *)m_root);
    if(array) 
    {
        for (int i=0; i<array->length; ++i)
        {
            json_object *pTemp = (json_object *)array_list_get_idx(array, i);
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

int CJsonDecoder::GetValueType(const string& strKey, EJsonValueType& type) const
{
    int nRet = 0;
    json_object *pJson = json_object_object_get((json_object *)m_root, strKey.c_str());
    if(pJson) 
    {
        type = (EJsonValueType)json_object_get_type(pJson);
    }
    else
    {
        nRet = -1;
    }
    
    return nRet;
}

int CJsonDecoder::GetSubDecoder(vector<CJsonDecoder>& vecDecoder) const
{
    int nRet = 0;
    if (NULL != m_root)
    {
        EJsonValueType eType = (EJsonValueType)json_object_get_type((json_object *)m_root);
        switch (eType)
        {
        case JSONTYPE_OBJECT:
            nRet = GetObjectSubDecoder(vecDecoder);
            break;
        case JSONTYPE_ARRAY:
            nRet = GetArraySubDecoder(vecDecoder);
            break;
        default:
            break;
        }
    }
    
    return nRet;
}

int CJsonDecoder::GetArraySubDecoder(vector<CJsonDecoder>& vecDecoder) const
{
    json_object *root = (json_object *)m_root;
 
    int len = json_object_array_length(root);
    for (int i = 0; i < len; ++i)
    {
        CJsonDecoder oJsonDecoder;
        oJsonDecoder.SetJsonObj(json_object_array_get_idx(root, i));
        vecDecoder.push_back(oJsonDecoder);
    }
    return 0;
}

int CJsonDecoder::GetObjectSubDecoder(vector<CJsonDecoder>& vecDecoder) const
{
    json_object *root = (json_object *)m_root;
    json_object_object_foreach(root, key, val)
    {
        CJsonDecoder oJsonDecoder;
        oJsonDecoder.SetJsonObj(val, key);
        vecDecoder.push_back(oJsonDecoder);
    }
    return 0;
}

const char* CJsonDecoder::ToJsonString() const
{
    return json_object_to_json_string((json_object *)m_root);
}

const char* CJsonDecoder::GetString() const
{
    return json_object_get_string((json_object *)m_root);
}

bool CJsonDecoder::ToKeyValueString(string &strKeyValue) const
{
    strKeyValue = "";
    json_object *root = (json_object *)m_root;
    json_object_object_foreach(root, key, val)
    {
        if (!strKeyValue.empty())
        {
            strKeyValue += "&";
        }

        strKeyValue += key;
        strKeyValue += "="; 
        strKeyValue += json_object_get_string(val);
    }
    json_object_put(root);
    return true;
}


