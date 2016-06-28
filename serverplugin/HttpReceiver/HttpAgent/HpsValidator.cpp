#include "HpsValidator.h"
#include "HttpRecConfig.h"
#include "HpsLogHelper.h"

#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using std::vector;


bool CHpsValidator::IsNeedToValidate(const string &strUrlPath)
{
    HP_XLOG(XLOG_DEBUG, "CHpsValidator::%s urlPath[%s]\n", __FUNCTION__, strUrlPath.c_str());
    const vector<string> vecWhiteValidateList = CHttpRecConfig::GetInstance()->GetWhiteValidateList();
    return !std::binary_search(vecWhiteValidateList.begin(), vecWhiteValidateList.end(), strUrlPath);
}

bool CHpsValidator::ValidateHttpParams(const char *pStrKeyValues)
{
    HP_XLOG(XLOG_DEBUG, "CHpsValidator::%s StrKeyValues[%s]\n", __FUNCTION__, pStrKeyValues);
    if (NULL != pStrKeyValues)
    {
        vector<string> vecKeyValue;
        boost::algorithm::split(vecKeyValue, pStrKeyValues, boost::algorithm::is_any_of("&"), boost::algorithm::token_compress_on);
        for (vector<string>::iterator iter = vecKeyValue.begin(); iter != vecKeyValue.end(); ++iter)
        {
            const char *pEqualitySign = strstr(iter->c_str(), "=");
            if (NULL != pEqualitySign)
            {
                if (!ValidateHttpParamValue(pEqualitySign + 1))
                {
                    return false;
                }
            }
        }
    }
    

    return true;
}

bool CHpsValidator::ValidateHttpParamValue(const char *pStrValue)
{
    HP_XLOG(XLOG_DEBUG, "CHpsValidator::%s Value[%s]\n", __FUNCTION__, pStrValue);
    //Validate for injection attack
    const static boost::regex reg("\\b(and|or)\\b.{1,6}?(=|>|<|\\bin\\b|\\blike\\b)|\\/\\*.+?\\*\\/|<\\s*script\\b|\\bEXEC\\b|UNION.+?Select|Update.+?SET|Insert\\s+INTO.+?VALUES|(Select|Delete).+?FROM|(Create|Alter|Drop|TRUNCATE)\\s+(TABLE|DATABASE)");
    if (NULL != pStrValue)
    {
        if (boost::regex_match(pStrValue, reg))
        {
            return false;
        }
    }

    return true;
}
