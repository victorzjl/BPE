#include "PreProcess.h"
#include "XmlConfigParser.h"
#include "HpsLogHelper.h"
#include <boost/algorithm/string.hpp>

using namespace sdo::common;

int PreProcess::LoadConfig(const string& strConfig)
{
	HP_XLOG(XLOG_DEBUG,"PreProcess::%s\n", __FUNCTION__);
	CXmlConfigParser oConfig;
	if(oConfig.ParseFile(strConfig)!=0)
	{
		return -1;
	}

	m_vecUrlList.clear();
	vector<string> vecUrlList = oConfig.GetParameters("PreProcessUrlList/Item");
	for (vector<string>::iterator iter = vecUrlList.begin();
		iter != vecUrlList.end(); ++iter)
	{
		string& str = *iter;
		vector<string> vecUrlInfo;
		boost::algorithm::split(vecUrlInfo, str,
			boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on); 
		if (vecUrlInfo.size() == 4)
		{
			PreInfo info;
			transform(vecUrlInfo[0].begin(), vecUrlInfo[0].end(), vecUrlInfo[0].begin(), (int(*)(int))tolower);
			info.strUrl = vecUrlInfo[0];
			info.nPathCount = atoi(vecUrlInfo[1].c_str());
			info.nParamIndex = atoi(vecUrlInfo[2].c_str());
			info.strParamName = vecUrlInfo[3];
			m_vecUrlList.push_back(info);
		}
        else
        {
            HP_XLOG(XLOG_WARNING, "PreProcess::%s Incorrect config.[%s]\n", __FUNCTION__, str.c_str());
        }
	}

	return 0;
}

int PreProcess::Process(SRequestInfo& sReq)
{
	HP_XLOG(XLOG_DEBUG,"PreProcess::%s url[%s] [%d]\n", __FUNCTION__, sReq.strUriCommand.c_str(), m_vecUrlList.size());
	if (m_vecUrlList.size() == 0)
	{
		return 0;
	}

	string strUrl = sReq.strUriCommand;
	string strUrlLowcase = strUrl;
	string strJson;
	transform(strUrlLowcase.begin(), strUrlLowcase.end(), strUrlLowcase.begin(), (int(*)(int))tolower);
	size_t nPos = strUrlLowcase.rfind(".json");
	if (nPos != string::npos)
	{
		strJson = strUrl.substr(nPos, strUrl.length()-nPos);
		strUrl = strUrl.substr(0, nPos);
	}
	for (vector<PreInfo>::iterator iter = m_vecUrlList.begin();
		iter != m_vecUrlList.end(); ++iter)
	{
		if (strUrlLowcase.find(iter->strUrl) != string::npos)
		{
			vector<string> vecUrlPath;
			boost::algorithm::split(vecUrlPath, strUrl,
				boost::algorithm::is_any_of("/"),boost::algorithm::token_compress_on);
			if ((int)vecUrlPath.size() == iter->nPathCount)
			{
				int index = 0;
				string strNewUrl;
				string strParam;
				for (vector<string>::iterator iterPath = vecUrlPath.begin(); 
					iterPath != vecUrlPath.end(); ++iterPath)
				{
					if (index == iter->nParamIndex)
					{
						strParam = string("&") + iter->strParamName + "=" + *iterPath;
					}
					else
					{
						if (!strNewUrl.empty())
							strNewUrl += "/";
						strNewUrl += *iterPath;
					}
					index++;
				}
				sReq.strUriCommand = strNewUrl + strJson;
				sReq.strUriAttribute += strParam;
			}
			break;
		}
	}

	return 0;
}
