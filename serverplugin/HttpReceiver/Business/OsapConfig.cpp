#include "OsapConfig.h"
#include "tinyxml/tinyxml.h"
#include "BusinessLogHelper.h"
#include <algorithm>

int COsapConfig::Load()
{
	BS_XLOG(XLOG_DEBUG,"COsapConfig::%s...\n",__FUNCTION__);
	TiXmlDocument xmlDoc;
	TiXmlElement* xmlRoot;
	TiXmlElement* xmlElement;
	if (!xmlDoc.LoadFile("./osap.xml"))
	{
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	if ((xmlRoot = xmlDoc.RootElement()) == NULL)
	{
		return -2;
	}

	m_mapUrlToDefautUser.clear();
	if ((xmlElement = xmlRoot->FirstChildElement("defautUserList")) != NULL)
	{
		for (TiXmlElement *xmlUrl = xmlElement->FirstChildElement("url");
			xmlUrl != NULL; xmlUrl = xmlUrl->NextSiblingElement("url"))
		{
			char buff[1024] = {0};
			string strUserName;
			if (xmlUrl->QueryValueAttribute("username", &buff) != TIXML_SUCCESS)
			{
				continue;
			}
			strUserName = buff;

			const char* pValue = xmlUrl->GetText();
			if (NULL != pValue)
			{
				string strUrl = pValue;
				transform(strUrl.begin(), strUrl.end(), strUrl.begin(), (int(*)(int))tolower);
				m_mapUrlToDefautUser.insert(make_pair(strUrl, strUserName));
				BS_XLOG(XLOG_DEBUG,"COsapConfig::%s, defautUserList:%s, %s\n",__FUNCTION__,strUrl.c_str(),strUserName.c_str());
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	m_vecWhiteUrl.clear();
	if ((xmlElement = xmlRoot->FirstChildElement("whiteUrlList")) != NULL)
	{
		for (TiXmlElement *xmlUrl = xmlElement->FirstChildElement("url");
			xmlUrl != NULL; xmlUrl = xmlUrl->NextSiblingElement("url"))
		{
			const char* pValue = xmlUrl->GetText();
			if (NULL != pValue)
			{
				string strUrl = pValue;
				transform(strUrl.begin(), strUrl.end(), strUrl.begin(), (int(*)(int))tolower);
				m_vecWhiteUrl.push_back(strUrl);
				BS_XLOG(XLOG_DEBUG,"COsapConfig::%s, whiteUrlList:%s \n",__FUNCTION__,strUrl.c_str());
			}
		}
		sort(m_vecWhiteUrl.begin(), m_vecWhiteUrl.end());
	}

	//////////////////////////////////////////////////////////////////////////
	m_vecNoSaagUrl.clear();
	if ((xmlElement = xmlRoot->FirstChildElement("noSaagList")) != NULL)
	{
		for (TiXmlElement *xmlUrl = xmlElement->FirstChildElement("url");
			xmlUrl != NULL; xmlUrl = xmlUrl->NextSiblingElement("url"))
		{
			const char* pValue = xmlUrl->GetText();
			if (NULL != pValue)
			{
				string strUrl = pValue;
				transform(strUrl.begin(), strUrl.end(), strUrl.begin(), (int(*)(int))tolower);
				m_vecNoSaagUrl.push_back(strUrl);
				BS_XLOG(XLOG_DEBUG,"COsapConfig::%s, whiteUrlList:%s \n",__FUNCTION__,strUrl.c_str());
			}
		}
		sort(m_vecNoSaagUrl.begin(), m_vecNoSaagUrl.end());
	}
    LoadTmpWhiteList();
	return 0;
}

bool COsapConfig::IsUrlInWhiteList(const string& strUrl)
{
	//string strUrl2 = strUrl;
	//transform(strUrl2.begin(), strUrl2.end(), strUrl2.begin(), (int(*)(int))tolower);
	return binary_search(m_vecWhiteUrl.begin(), m_vecWhiteUrl.end(), strUrl);
}

bool COsapConfig::IsUrlInNoSaagList(const string& strUrl)
{
	//string strUrl2 = strUrl;
	//transform(strUrl2.begin(), strUrl2.end(), strUrl2.begin(), (int(*)(int))tolower);
	return binary_search(m_vecNoSaagUrl.begin(), m_vecNoSaagUrl.end(), strUrl);
}

string COsapConfig::GetDefaultUserName(const string& strUrl)
{
	//string strUrl2 = strUrl;
	//transform(strUrl2.begin(), strUrl2.end(), strUrl2.begin(), (int(*)(int))tolower);
	map<string, string>::iterator iter = m_mapUrlToDefautUser.find(strUrl);
	if (iter != m_mapUrlToDefautUser.end())
	{
		return iter->second;
	}
	return "";
}

int COsapConfig::LoadTmpWhiteList()
{
	BS_XLOG(XLOG_DEBUG,"COsapConfig::%s...\n",__FUNCTION__);
	TiXmlDocument xmlDoc;
	TiXmlElement* xmlRoot;
	TiXmlElement* xmlElement;
	if (!xmlDoc.LoadFile("./tmpWhiteUrlList.xml"))
	{
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	if ((xmlRoot = xmlDoc.RootElement()) == NULL)
	{
		return -2;
	}
    m_vecTmpWhiteUrl.clear();
	if ((xmlElement = xmlRoot->FirstChildElement("whiteUrlList")) != NULL)
	{
		for (TiXmlElement *xmlUrl = xmlElement->FirstChildElement("url");
			xmlUrl != NULL; xmlUrl = xmlUrl->NextSiblingElement("url"))
		{
			const char* pValue = xmlUrl->GetText();
			if (NULL != pValue)
			{
				string strUrl = pValue;
				transform(strUrl.begin(), strUrl.end(), strUrl.begin(), (int(*)(int))tolower);
				m_vecTmpWhiteUrl.push_back(strUrl);
				BS_XLOG(XLOG_DEBUG,"COsapConfig::%s, whiteUrlList:%s \n",__FUNCTION__,strUrl.c_str());
			}
		}
		sort(m_vecTmpWhiteUrl.begin(), m_vecTmpWhiteUrl.end());
	}
	return 0;
}
bool COsapConfig::IsUrlInTmpWhiteList(const string& strUrl)
{
    return binary_search(m_vecTmpWhiteUrl.begin(),m_vecTmpWhiteUrl.end(),strUrl);
}


