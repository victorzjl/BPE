#ifndef _HPS_VALIDATOR_H_
#define _HPS_VALIDATOR_H_

#include <string>

using std::string;

class CHpsValidator
{
public:
    static bool IsNeedToValidate(const string &strUrlPath);
    static bool ValidateHttpParams(const string &strKeyValues) { return ValidateHttpParams(strKeyValues.c_str()); }
    static bool ValidateHttpParams(const char *pStrKeyValues);
    static bool ValidateHttpParamValue(const string &strValue) { return ValidateHttpParamValue(strValue.c_str()); }
    static bool ValidateHttpParamValue(const char *pStrValue);
};

#endif //_HPS_VALIDATOR_H_
