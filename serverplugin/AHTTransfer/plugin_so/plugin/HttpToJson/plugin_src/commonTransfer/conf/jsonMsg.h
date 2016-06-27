#ifndef _JSON_MSG_H_
#define _JSON_MSG_H_
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
	int SetValue(const string& strParentKey, const string& strKey, const string& strValue);

	int SetValue(const string& strParentKey, const string& strKey, double dValue);

	int SetValue(const string& strParentKey, const string& strKey, const vector<string>& vecStrValue);

	int SetValue(const string& strParentKey, const string& strKey, const CJsonEncoder& encoder);
	
	int SetValue(const string& strParentKey, const string& strKey, const vector<CJsonEncoder>& vecEncoder);

	void* GetJsonObj() const {return m_root;}
	void GetJsonString(string &strJson);
	
	void Destroy();
private:
	void *m_root;
};

class CJsonDecoder
{
public:
	CJsonDecoder();
	CJsonDecoder(const void * pBuffer, unsigned int nLen);
	~CJsonDecoder();
	void Decoder(const void * pBuffer, unsigned int nLen);
	int GetValue(const string& strKey, int &nValue);
	int GetValue(const string& strKey, string& strValue);
	int GetValue(const string& strKey, vector<string>& vecValues);
	int GetValue(const string& strKey, CJsonDecoder& decoder);
	
	int GetValue(const string& strKey, vector<CJsonDecoder>& vecDecoder);
	
	void SetJsonObj(void *pObj) {m_root = pObj;}

	void Destroy();
private:
	void *m_root;
};


#endif


