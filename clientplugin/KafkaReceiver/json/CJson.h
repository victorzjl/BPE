#ifndef _JSON__H_
#define _JSON__H_
#include <string>
#include <vector>
using std::string;
using std::vector;

class CJsonEncoder
{
public:
	CJsonEncoder();
	~CJsonEncoder();
	
	//if there is no parent , strParentKey should pass ""
	int SetValue(const string& strParentKey, const string& strKey,const int nValue);

	int SetValue(const string& strParentKey, const string& strKey,const string& strValue);

	int SetValue(const string& strParentKey, const string& strKey,const vector<int>& vecIntValue);

	int SetValue(const string& strParentKey, const string& strKey,const vector<string>& vecStrValue);

	int SetValue(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder);
	
	int SetValue(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder);

	void* GetJsonObj() const {return m_root;}
	void* GetJsonString();
	int GetJsonStringLen();

	void DestroyJsonEncoder();
private:
	void *m_root;
};

class CJsonDecoder
{
public:
	CJsonDecoder();
	CJsonDecoder(const void * pBuffer, unsigned int nLen);
	~CJsonDecoder();
	int GetValue(const string& strKey, int& nValue);
	int GetValue(const string& strKey, string& strValue);
	int GetValue(const string& strKey, vector<string>& vecValues);
	int GetValue(const string& strKey, CJsonDecoder& decoder);
	
	int GetValue(const string& strKey, vector<CJsonDecoder>& vecDecoder);
	int GetValue(const string& strKey, vector<int>& vecValues);
	
	void SetJsonObj(void *pObj) {m_root = pObj;}

	void DestroyJsonDecoder();
private:
	void *m_root;
};

#endif


