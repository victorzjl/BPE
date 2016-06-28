#ifndef _LOG_Filter_H_
#define _LOG_Filter_H_

#include "CJson.h"

#include<string>
#include<map>


using std::string;
using std::map;

typedef struct stFilterRule
{
	string _strFilter;
	string _strTarget;
	string _strRegex;
	string _strReplace;
}SFilterRule;


typedef map<string, SFilterRule> SFilterRuleSet;


class CLogFilter
{
public:
	~CLogFilter(){}

	static void Dump();
	static bool Initialize(const string &strFile);
	static bool FilterKeyValue(const string &strFilter, const string &strInKeyValue, string &strOutKeyValue);
	static bool FilterJson(const string &strFilter, const string &strInJson, string &strOutJson);

private:
	CLogFilter(){}
	static bool ParseOneFilterConfig(const string &strConfig);
    static bool ParseOneRuleConfig(const string &strConfig, const string &strFilter);
	static bool FilterStringInner(const SFilterRule &sFilterRule, const string &strInValue, string &strOutValue);
	static bool FilterJsonInner(const SFilterRuleSet &mapFilterRule, const CJsonDecoder &oJsonDecoder, 
		                        const string &strParentNode, const string &strParentBrackets, CJsonEncoder &oJsonEncoder);

private:
	static map<string, SFilterRuleSet> s_mapKeyValueRule;
	static map<string, SFilterRuleSet> s_mapJsonRule;
};

#endif


