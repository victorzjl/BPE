#include "LogFilter.h"
#include "XmlConfigParser.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <vector>

using namespace sdo::common;
using std::vector;


map<string, SFilterRuleSet> CLogFilter::s_mapKeyValueRule;
map<string, SFilterRuleSet> CLogFilter::s_mapJsonRule;

void CLogFilter::Dump()
{}

bool CLogFilter::Initialize(const string &strFile)
{
	bool bRet = false;

	s_mapKeyValueRule.clear();
	s_mapJsonRule.clear();

	CXmlConfigParser oConfig;
	if(0 == oConfig.ParseFile(strFile.c_str()))
	{
        bRet = true;
		vector<string> vecFilterConfig = oConfig.GetParameters("filter_config");
		vector<string>::iterator iter = vecFilterConfig.begin();
		while ((iter!=vecFilterConfig.end()) && bRet)
		{
			bRet = ParseOneFilterConfig(*iter);
			++iter;
		}
	}
	Dump();
	return bRet;
}

bool CLogFilter::FilterStringInner(const SFilterRule &sFilterRule, const string &strInValue, string &strOutValue)
{
	boost::regex reg(sFilterRule._strRegex, boost::regex::icase|boost::regex::perl);
	strOutValue = boost::regex_replace(strInValue, reg, sFilterRule._strReplace); 
	return true;
}

bool CLogFilter::FilterKeyValue(const string &strFilter, const string &strInKeyValue, string &strOutKeyValue)
{
	bool bRet = false;
	map<string, SFilterRuleSet>::iterator iter = s_mapKeyValueRule.find(strFilter);
	if (iter != s_mapKeyValueRule.end())
	{
		bRet = true;
		vector<string> vecKeyValue;
    	boost::split(vecKeyValue, strInKeyValue, boost::is_any_of("&"));
    	vector<string>::iterator iterKeyValue = vecKeyValue.begin();
    	while (bRet &&(iterKeyValue != vecKeyValue.end()))
        {
        	string strKey;
        	string strFilterValue;
			vector<string> vecPart;
			boost::split(vecPart, *iterKeyValue, boost::is_any_of("="));         
			if (vecPart.size() == 2)
			{
				strKey = vecPart[0];
				SFilterRuleSet::iterator iterRule = iter->second.find(strKey);
				if (iterRule != iter->second.end())
				{
					bRet = FilterStringInner(iterRule->second, vecPart[1], strFilterValue); 
				}
				else
				{
					strFilterValue = vecPart[1];
				}
			}
			else
			{
				strKey = *iterKeyValue;
			}

			strOutKeyValue += (strOutKeyValue.empty()?"":"&");
			strOutKeyValue += strKey + "=" + strFilterValue;
			++iterKeyValue;
		}
	}

	return bRet;
}

bool CLogFilter::FilterJson(const string &strFilter, const string &strInJson, string &strOutJson)
{
	bool bRet = false;
    map<string, SFilterRuleSet>::const_iterator iter = s_mapJsonRule.find(strFilter);
    if (iter != s_mapJsonRule.end())
	{
		CJsonDecoder oJsonDecoder(strInJson.c_str(), strInJson.length());
		if (JSONTYPE_OBJECT == oJsonDecoder.GetType())
		{
			CJsonEncoder oJsonEncoder;
			bRet = FilterJsonInner(iter->second, oJsonDecoder, "", "", oJsonEncoder);
			strOutJson = (char *)oJsonEncoder.GetJsonString();
			oJsonEncoder.DestroyJsonEncoder();
		}
		else if (JSONTYPE_ARRAY == oJsonDecoder.GetType())
		{
			CJsonEncoder oJsonEncoder(JSON_TEXT_ARRAY);
			bRet = FilterJsonInner(iter->second, oJsonDecoder, "[", "]", oJsonEncoder);
			strOutJson = (char *)oJsonEncoder.GetJsonString();
			oJsonEncoder.DestroyJsonEncoder();
		}
		oJsonDecoder.DestroyJsonDecoder();
	}
	return bRet;
}

bool CLogFilter::ParseOneFilterConfig(const string &strConfig)
{
	bool bRet = false;
	CXmlConfigParser oConfig;
    if (0 == oConfig.ParseDetailBuffer(strConfig.c_str()))
	{
        bRet = true;
        string strFilter = oConfig.GetParameter("filter");
        vector<string> vecRule = oConfig.GetParameters("rule");
        vector<string>::const_iterator iter = vecRule.begin();
        while (bRet && (iter != vecRule.end()))
        {
            bRet = ParseOneRuleConfig(*iter, strFilter);
            ++iter;
        }
	}
	return bRet;
}

bool CLogFilter::ParseOneRuleConfig(const string &strConfig, const string &strFilter)
{
    bool bRet = false;
    CXmlConfigParser oConfig;
    if (0 == oConfig.ParseDetailBuffer(strConfig.c_str()))
    {
        bRet = true;
        SFilterRule sFilterRule;
        sFilterRule._strFilter = strFilter;
        sFilterRule._strTarget = oConfig.GetParameter("target");
        sFilterRule._strRegex = oConfig.GetParameter("regex");
        sFilterRule._strReplace = oConfig.GetParameter("replace");

        int iType = oConfig.GetParameter("type", 0);
        switch (iType)
        {
        case 0:
            s_mapKeyValueRule[sFilterRule._strFilter][sFilterRule._strTarget] = sFilterRule;
            break;
        case 1:
            s_mapJsonRule[sFilterRule._strFilter][sFilterRule._strTarget] = sFilterRule;
            break;
        default:
            bRet = false;
            break;
        }
    }
    return bRet;
}

bool CLogFilter::FilterJsonInner(const SFilterRuleSet &mapFilterRule, const CJsonDecoder &oJsonDecoder, 
								 const string &strParentNode, const string &strParentBrackets, CJsonEncoder &oJsonEncoder)
{
	bool bRet = true;
	vector<CJsonDecoder> vecDecoder;
	oJsonDecoder.GetSubDecoder(vecDecoder);
	vector<CJsonDecoder>::iterator iter = vecDecoder.begin();
	while ((iter != vecDecoder.end()) && bRet)
	{
		switch(iter->GetType())
		{
			case JSONTYPE_NULL:
				break;

			case JSONTYPE_OBJECT:
			{
                string strNode = strParentNode + (iter->GetKey().empty() ? "" : "/") 
                                 + boost::algorithm::to_lower_copy(iter->GetKey());
				CJsonEncoder oEncoder;
				bRet = FilterJsonInner(mapFilterRule, *iter, strNode, strParentBrackets, oEncoder); 
				oJsonEncoder.SetValue("", iter->GetKey(), oEncoder);
				break;
			}
			case JSONTYPE_ARRAY:
			{
                string strNode = strParentNode + (iter->GetKey().empty() ? "" : "/") 
                                 + boost::algorithm::to_lower_copy(iter->GetKey()) + "[";
				string strBrackets = strParentBrackets + "]";
				CJsonEncoder oEncoder(JSON_TEXT_ARRAY);
				bRet = FilterJsonInner(mapFilterRule, *iter, strNode, strBrackets, oEncoder); 
				oJsonEncoder.SetValue("", iter->GetKey(), oEncoder);
				break;
			}
			default:
			{
				string strFilter = strParentNode + (iter->GetKey().empty()?"":"/") 
                                   + boost::algorithm::to_lower_copy(iter->GetKey()) + strParentBrackets;
				SFilterRuleSet::const_iterator iterRule = mapFilterRule.find(strFilter);
                boost::algorithm::to_lower(strFilter);
                string strFilterValue;
				if (iterRule != mapFilterRule.end())
				{
                    bRet = FilterStringInner(iterRule->second, iter->GetString(), strFilterValue);
				}
                else
                {
                    strFilterValue = iter->GetString();
                }
                oJsonEncoder.SetValue("", iter->GetKey(), strFilterValue);
				break;
			}
		}

		++iter;
	}

	return bRet;
}
