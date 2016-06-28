#include "HttpRequestEncoder.h"

CHttpRequestEncoder::CHttpRequestEncoder()
{
	SHttpHeadValue sUserAgent("User-Agent", "Mozilla/5.0 hps1.0.2.0");
	m_vecHeadValue.push_back(sUserAgent);

	SHttpHeadValue sAccept("Accept", "text/html,application/xhtml+xml,application/xml");
	m_vecHeadValue.push_back(sAccept);

	SHttpHeadValue sLanguage("Accept-Language", "zh-cn,zh;q=0.5");
	m_vecHeadValue.push_back(sLanguage);

	SHttpHeadValue sCharset("Accept-Charset", "utf-8,GB2312");
	m_vecHeadValue.push_back(sCharset);
}

void CHttpRequestEncoder::SetHttpVersion(const string &strVersion)
{
	if(0 == strncmp(strVersion.c_str(), "HTTP/1.1", 8))
	{
			m_strHttpVersion = "HTTP/1.1";
	}
	else
	{
			m_strHttpVersion = "HTTP/1.0";
	}
}

void CHttpRequestEncoder::SetMethod(const string &strMethod)
{
	if(0 == strncmp(strMethod.c_str(), "GET", 3))
	{
		m_strMethod = "GET";
	}
	else if(0 == strncmp(strMethod.c_str(), "POST", 4))
	{
		m_strMethod = "POST";
	}
	else if(0 == strncmp(strMethod.c_str(), "HEAD", 4))
	{
		m_strMethod = "HEAD";
	}
	else
	{
		m_strHttpVersion = "GET";
	}
}


void CHttpRequestEncoder::SetXForwardedFor(const string &strForwaredIp)
{
	if(!strForwaredIp.empty())
	{
		SHttpHeadValue sHeadValue("X-Forwarded-For", strForwaredIp);
		m_vecHeadValue.push_back(sHeadValue);
	}
}

void CHttpRequestEncoder::SetConnecionType(const string &strConnType)
{
	if(!strConnType.empty())
	{
		SHttpHeadValue sHeadValue("Content-Type", strConnType);
		m_vecHeadValue.push_back(sHeadValue);
	}
}

void CHttpRequestEncoder::SetHost(const string &strHost)
{
	if(!strHost.empty())
	{
		SHttpHeadValue sHeadValue("Host", strHost);
		m_vecHeadValue.push_back(sHeadValue);
	}
}

void CHttpRequestEncoder::SetUrl(const string &strUrl)
{
	if(!strUrl.empty())
	{
		m_strUrl = strUrl;
	}
}

void CHttpRequestEncoder::SetBody(const string &strBody)
{
	if(!strBody.empty())
	{
		m_strBody = strBody;
	}
}

void CHttpRequestEncoder::SetHeadValue(const string &strHead, const string &strValue)
{
	if((!strHead.empty()) && (!strValue.empty()))
	{
		SHttpHeadValue sHeadValue(strHead, strValue);
		m_vecHeadValue.push_back(sHeadValue);
	}
}


string CHttpRequestEncoder::Encode()
{
	string strRequestMsg;
	bool isPost = false;
	if(0 == strncmp(m_strMethod.c_str(), "POST", 4))
	{
		strRequestMsg = m_strMethod + " " + m_strUrl + " " + m_strHttpVersion + "\r\n";

		if(!m_strBody.empty())
		{
			isPost = true;
			
			char szBodyLen[64] = {0};
			sprintf(szBodyLen,"%d", m_strBody.length());
			SHttpHeadValue sHeadValue("Content-Length", szBodyLen);
			m_vecHeadValue.push_back(sHeadValue);
		}
	}
	else
	{		
		strRequestMsg = m_strMethod + " " + m_strUrl + "?" + m_strBody + " " + m_strHttpVersion + "\r\n";
	}
	
	vector<SHttpHeadValue>::iterator itr;
	for(itr=m_vecHeadValue.begin(); itr!=m_vecHeadValue.end(); ++itr)
	{
		SHttpHeadValue &sHeadValue = *itr;
		strRequestMsg += sHeadValue.strHead + ": " + sHeadValue.strValue + "\r\n";
	}
	strRequestMsg.append("\r\n");

	if(isPost)
	{
		strRequestMsg += m_strBody;
	}

	return strRequestMsg;
	
}

string CHttpRequestEncoder::Encode(const string &strUrl, const string &strBody, const string &strHost,
	const string &strForwaredIp,const string &strMethod, const string &strVersion, const string &strConnType)
{	
	SetMethod(strMethod);
	SetUrl(strUrl);
	SetBody(strBody);
	SetXForwardedFor(strForwaredIp);
	SetConnecionType(strConnType);
	SetHost(strHost);
	SetHttpVersion(strVersion);

	return Encode();
}




