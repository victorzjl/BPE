#ifndef _HTTP_REQUEST_ENCODER_H_
#define _HTTP_REQUEST_ENCODER_H_

#include <vector>
#include <string>
#include <map>
using std::string;
using std::vector;
using std::map;
using std::make_pair;

typedef struct stHttpHeadValue
{
	string strHead;
	string strValue;

	stHttpHeadValue(){}
	stHttpHeadValue(const string &strHead_,const string &strValue_):strHead(strHead_),strValue(strValue_){}
}SHttpHeadValue;

class CHttpRequestEncoder
{
public:
	CHttpRequestEncoder();
	~CHttpRequestEncoder(){}
	void SetHttpVersion(const string &strVersion);
	void SetMethod(const string &strMethod);
	void SetXForwardedFor(const string &strForwaredIp);
	void SetConnecionType(const string &strConnType);
	void SetHost(const string &strHost);
	void SetUrl(const string &strUrl);
	void SetBody(const string &strBody);
	void SetHeadValue(const string &strHead, const string &strValue);
	
	string Encode();
	string Encode(const string &strUrl, const string &strBody,const string &strHost,const string &strForwaredIp="",
		const string &strMethod="GET", const string &strVersion="HTTP/1.0", const string &strConnType="close");


private:
	string m_strHttpVersion;
	string m_strMethod;
	vector<SHttpHeadValue> m_vecHeadValue;
	string m_strUrl;
	string m_strBody;
	
};

#endif

