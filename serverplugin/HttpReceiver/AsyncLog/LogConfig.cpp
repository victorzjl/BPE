#include "LogConfig.h"
#include "tinyxml/tinyxml.h"
#include "BusinessLogHelper.h"
#include <algorithm>
#include "DirReader.h"
#include <boost/algorithm/string.hpp>

int LogConfig::Load()
{
	BS_XLOG(XLOG_DEBUG,"LogConfig::%s...\n",__FUNCTION__);
	nsHpsTransfer::CDirReader oDirReader("*.so");
	if (!oDirReader.OpenDir("./plugin"))
	{
		BS_XLOG(XLOG_WARNING,"LogConfig::%s,open dir error\n",__FUNCTION__);
		return -1;
	}

	char szFileName[MAX_PATH] = {0};
	if (!oDirReader.GetFirstFilePath(szFileName))
	{
		BS_XLOG(XLOG_WARNING,"LogConfig::%s,GetFirstFilePath error\n",__FUNCTION__);
		return -2;
	}
	m_mapKeys.clear();
	do
	{
		string strFileName = szFileName;
		size_t posFind = strFileName.find_last_of('/');
		if (posFind != string::npos)
		{
			string strFile = strFileName.substr(0, posFind);
			strFile  = strFile + "/UrlMapping.xml";
			BS_XLOG(XLOG_DEBUG,"LogConfig::%s [%s]\n",__FUNCTION__, strFile.c_str());
			ReadOneFile(strFile);
		}
	}while(oDirReader.GetNextFilePath(szFileName));
	
	return 0;
}

int LogConfig::ReadOneFile(const string& strFile)
{
	BS_XLOG(XLOG_DEBUG,"LogConfig::%s...\n",__FUNCTION__);
	TiXmlDocument xmlDoc;
	TiXmlElement* xmlRoot;
	TiXmlElement* xmlElement;
	if (!xmlDoc.LoadFile(strFile.c_str()))
	{
		BS_XLOG(XLOG_DEBUG,"LogConfig::%s LoadFile file failed\n",__FUNCTION__);
		return -1;
	}

	if ((xmlRoot = xmlDoc.RootElement()) == NULL)
	{
		BS_XLOG(XLOG_DEBUG,"LogConfig::%s LoadFile file failed no root\n",__FUNCTION__);
		return -2;
	}

	if ((xmlElement = xmlRoot->FirstChildElement("UrlMapping")) == NULL)
	{
		BS_XLOG(XLOG_DEBUG,"LogConfig::%s LoadFile file failed no UrlMapping\n",__FUNCTION__);
		return -3;
	}

	for (TiXmlElement *xmlKey = xmlElement->FirstChildElement("Item");
		xmlKey != NULL; xmlKey = xmlKey->NextSiblingElement("Item"))
	{
		char buff[1024] = {0};
		LagKeys keys;
		if (xmlKey->QueryValueAttribute("logkey1", &buff) == TIXML_SUCCESS)
		{
			keys.strKey1 = buff;
			transform(keys.strKey1.begin(), keys.strKey1.end(), keys.strKey1.begin(), (int(*)(int))tolower);
		}
		if (xmlKey->QueryValueAttribute("logkey2", &buff) == TIXML_SUCCESS)
		{
			keys.strKey2 = buff;
			transform(keys.strKey2.begin(), keys.strKey2.end(), keys.strKey2.begin(), (int(*)(int))tolower);
		}

		const char* pValue = xmlKey->GetText();
		if ((!keys.strKey1.empty() || !keys.strKey2.empty()) &&
			NULL != pValue)
		{
			vector<string> vecSplit;
			boost::split(vecSplit, pValue, boost::is_any_of(","),boost::token_compress_on);  
			if (vecSplit.size() > 0)
			{
				string strUrl = vecSplit[0];
				transform(strUrl.begin(), strUrl.end(),strUrl.begin(), (int(*)(int))tolower);
				m_mapKeys.insert(make_pair(strUrl, keys));
				BS_XLOG(XLOG_DEBUG,"LogConfig::%s, url:%s, %s, %s\n",__FUNCTION__,
					strUrl.c_str(),keys.strKey1.c_str(),keys.strKey2.c_str());
			}
		}
	}
	return 0;
}

int LogConfig::GetLogParam(const string& strUrl, string& strKey1, string& strKey2)
{
	string strUrl2 = strUrl;
	transform(strUrl2.begin(), strUrl2.end(), strUrl2.begin(), (int(*)(int))tolower);
	map<string, LagKeys>::iterator iter = m_mapKeys.find(strUrl2);
	if (iter != m_mapKeys.end())
	{
		strKey1 = iter->second.strKey1;
		strKey2 = iter->second.strKey2;
		return 0;
	}
	return -1;
}
