#ifndef _JSON__H_
#define _JSON__H_
#include <string>
#include <vector>

#include <stdint.h>
#include "json.h"

using std::string;
using std::vector;

typedef enum etJsonValueType 
{
  JSONTYPE_NULL = json_type_null,
  JSONTYPE_BOOLEAN = json_type_boolean,
  JSONTYPE_DOUBLE = json_type_double,
  JSONTYPE_INT = json_type_int,
  JSONTYPE_OBJECT = json_type_object,
  JSONTYPE_ARRAY = json_type_array,
  JSONTYPE_STRING = json_type_string
} EJsonValueType;

typedef enum etJsonTextType
{
    JSON_TEXT_OBJECT = 0,
    JSON_TEXT_ARRAY = 1
}EJsonTextType;


class CJsonEncoder
{
public:
    explicit CJsonEncoder(EJsonTextType type = JSON_TEXT_OBJECT);
    ~CJsonEncoder();
    
    EJsonTextType GetJsonTextType() const { return m_jsonTextType; }

    /////////////////////////////////////////////////////////////
    //   @param strParentKey: if there is no parent , strParentKey should pass ""
    /////////////////////////////////////////////////////////////
    int SetValue(const string& strParentKey, const string& strKey, const int nValue, int nIndex = -1);
    int SetValue(const string& strParentKey, const string& strKey, const string& strValue, int nIndex = -1);
    int SetValue(const string& strParentKey, const string& strKey, const vector<int>& vecIntValue, int nIndex = -1);
    int SetValue(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue, int nIndex = -1);
    int SetValue(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder, int nIndex = -1);
    int SetValue(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder, int nIndex = -1);

    void* GetJsonObj() const { return m_root; }
    void* GetJsonString() const;
    int GetJsonStringLen() const;
    void DestroyJsonEncoder();

private:
    void *GetParentJsonObj(const string& strParentKey);
    int SetElement(const string& strKey, int nIndex, void *pParent, void *pObj);
    int SetArrayElement(int nIndex, void *pParent, void *pObj);

    int SetValue_Object(const string& strParentKey, const string& strKey, const int nValue, int nIndex);
    int SetValue_Object(const string& strParentKey, const string& strKey, const string& strValue, int nIndex);
    int SetValue_Object(const string& strParentKey, const string& strKey, const vector<int>& vecIntValue, int nIndex);
    int SetValue_Object(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue, int nIndex);
    int SetValue_Object(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder, int nIndex);
    int SetValue_Object(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder, int nIndex);

    int SetValue_Array(const string& strParentKey, const string& strKey, const int nValue, int nIndex);
    int SetValue_Array(const string& strParentKey, const string& strKey, const string& strValue, int nIndex);
    int SetValue_Array(const string& strParentKey, const string& strKey, const vector<int>& vecIntValue, int nIndex);
    int SetValue_Array(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue, int nIndex);
    int SetValue_Array(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder, int nIndex);
    int SetValue_Array(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder, int nIndex);

private:
    void *m_root;
    EJsonTextType m_jsonTextType;
};

class CJsonDecoder
{
public:
    CJsonDecoder();
    CJsonDecoder(const void * pBuffer, unsigned int nLen);
    ~CJsonDecoder(); 
    EJsonValueType GetType() const;
    const string &GetKey() const { return m_strKey; }
    int GetValue(const string& strKey, int& nValue) const;
    int GetValue(const string& strKey, string& strValue) const;
    int GetValue(const string& strKey, vector<string>& vecValues) const;
    int GetValue(const string& strKey, CJsonDecoder& decoder) const;
    int GetValue(vector<CJsonDecoder>& vecDecoder) const;
    int GetValue(const string& strKey, vector<CJsonDecoder>& vecDecoder) const;   
    int GetValueType(const string& strKey, EJsonValueType& type) const;
    int GetSubDecoder(vector<CJsonDecoder>& vecDecoder) const;
     
    //The difference between function 'ToJsonString' and 'GetString' is that
    //when the json object type is json_type_string, using function 'ToJsonString' will
    //get value in quotes and using function 'ToJsonString' will get value not in quotes.
    //example:
    //      if CJsonDecoder instance variable named 'oDecoder' is json_type_string, and its value is 'nothing',
    //      and var1=oDecoder.ToJsonString(), var2 =oDecoder.ToJsonString(),
    //      then var1's value is '"nothing"' but var2's value is 'nothing'
    const char *ToJsonString() const;
    const char *GetString() const;

    bool ToKeyValueString(string &strKeyVlaue) const;
    
    void SetJsonObj(void *pObj, const string &strKey = "") { m_root = pObj; m_strKey = strKey; }

    void DestroyJsonDecoder();

private:
    int GetArraySubDecoder(vector<CJsonDecoder>& vecDecoder) const;
    int GetObjectSubDecoder(vector<CJsonDecoder>& vecDecoder) const;

private:
    void *m_root;
    string m_strKey;
};


#endif




