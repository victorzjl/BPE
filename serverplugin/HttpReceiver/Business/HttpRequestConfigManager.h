#ifndef _HTTP_REQUEST_CONFIG_MANAGER_H_
#define _HTTP_REQUEST_CONFIG_MANAGER_H_

#include "HpsCommonInner.h"
#include "ErrorCode.h"
#include "CodeConvert.h"
#include<string>
#include<map>
using std::string;
using std::map;
using std::make_pair;

typedef struct stPriorityReqThreshold
{
	int nSosPriority;
	int nReqThreshold;

	stPriorityReqThreshold():nSosPriority(0),nReqThreshold(0){}
	stPriorityReqThreshold(int nPri, int nThreshold):nSosPriority(nPri),nReqThreshold(nThreshold){}
}SPriorityReqThreshold;

typedef struct stRoutePolicy
{
	int nErrorCode;
	
	map<int, SPriorityReqThreshold> mapReqThresholdByPri;
}SRoutePolicy;

typedef struct stRouteConfig
{	
	int nCurrentSosPriority;  //default to the high sos priority
	int nLastErrorCode;
	int nFailCount;

	map<int, string> mapSosPriorityAddr; //used to update the nCurrentSosPriority	
	map<int, SRoutePolicy> mapRoutePolicyByErrCode;

	stRouteConfig():nCurrentSosPriority(0),nLastErrorCode(0),nFailCount(0){}
}SRouteConfig;

typedef enum
{
	E_ErrorCode_Neg = 1,
	E_ErrorCode_Pos = 2,
	E_ErrorMsg_String = 3
}EFieldType;
typedef struct stHttpResponseFormat
{
	vector<EFieldType> vecFields;
	string strFormat;
	string strCharSet;
	string strContentType;
}SHttpResponseFormat;

typedef struct stHttpRequestConfig
{
	string strUrl;
	SServiceMsgId sServiceMsgId;

	string strSignature;

	SRouteConfig sRouteConfig;

	SHttpResponseFormat sHttpResponseFormat;	
}SHttpRequestConfig;

struct STimeoutConfig
{
	string strParam;
	unsigned int nDefTimeout;
};


class CHttpRequestConfigManager
{
public:
	CHttpRequestConfigManager(){}
	~CHttpRequestConfigManager(){}

	int LoadConfig(const vector<string> &vecRouteBalance, const map<unsigned int, map<int, string> > &mapSosPriAddrsByServiceId);

	int GetCurrentSosPri(unsigned int dwServiceId, unsigned int dwMsgId);
	int UpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode);

	string GetResponseString(unsigned int dwServiceId, unsigned int dwMsgId, int nErrorCode, const string& strIp, const string &strUrl);
	void GetResponseOption(unsigned int dwServiceId, unsigned int dwMsgId, const string &strUrl,
		string &strCharSet, string &strContentType);

	string GetSignatureKey(unsigned int dwServiceId, unsigned int dwMsgId, const string &strUrl);

	bool FindTimeoutConfig(const string& strUrl, string& strParam, unsigned int nTimeout);
	
public:
	void Dump();

private:
	int LoadHttpResponseFormat(const string &strFields, const string &strFormat, const string &strCharSet, string &strContentType, SHttpResponseFormat &sHttpResponseFormat);
	int LoadRouteBalanceConfig(const string &strRouteBalance, const map<unsigned int, map<int, string> > &mapSosPriAddrsByServiceId, 
		unsigned int dwServiceId, unsigned int dwMsgId, SRouteConfig &sRouteConfig);
	int LoadTimeoutConfig();
	int ResetFailMsg(SRouteConfig &sConfig);

private:
	//map<SServiceMsgId, SRouteConfig> m_mapRouteConfigByServiceMsgId;
	map<SServiceMsgId, SHttpRequestConfig> m_mapHttpReqConfByServiceMsgId;
	map<string, SHttpRequestConfig> m_mapHttpReqConfByUrl;
	map<string, STimeoutConfig> m_mapTimeoutConfig;

	CCodeConverter m_oCodeConvert;

};

#endif

