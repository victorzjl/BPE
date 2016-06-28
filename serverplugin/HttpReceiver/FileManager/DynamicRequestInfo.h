#ifndef _DYNAMIC_REQUEST_INFO_H_
#define _DYNAMIC_REQUEST_INFO_H_

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

typedef struct stURLInfo
{
    bool _bIsRemote;
    bool _bIsHttps;
    string _strHost;
    unsigned int _dwPort;
    string _strPath;

    string _strFormatParams;
    string _strFixedParams;
    map<string, string> _mapFixedParam;
    map<string, string> _mapVarParam;
    vector<string> _vecVarParamName;
    vector<string> _vecVarParamReference;

    stURLInfo() : _bIsRemote(false), _bIsHttps(false), _dwPort(0) {}
    void ConstructParams(const vector<string> &vecValue, string &strParams) const;
}SURLInfo;

typedef struct stDynamicRequestInfo
{
    bool _bIsHandled;
    string _strResponse;
    SURLInfo _sUrlInfo;
}SDynamicRequestInfo;

#endif //_DYNAMIC_REQUEST_INFO_H_
