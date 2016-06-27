#ifndef _COH_CLIENT_IMP_H_
#define _COH_CLIENT_IMP_H_
#include "CohClient.h"
#include <string>

using namespace sdo::coh;
using std::string;

typedef struct stHttpServerInfo
{
	string strIp;
	unsigned int dwPort;
	string strPath;
}SHttpServerInfo;


class CCohClientImp:public ICohClient
{
public:
	static CCohClientImp * GetInstance();
	void Init(const string &strMonitorUrl, 
		const string &strDetailMonitorUrl, 
		const string &strDgsUrl,
		const string &strErrUrl);
	void SendDataToMonitor(const string &strContent);
	void SendDetailDataToMonitor(const string &strContent);
	void SendToDgs(const string &strContent);
	void SendErrRequest(const string &strContent);

public:
	virtual void OnReceiveResponse(const string &strResponse);

private:
	CCohClientImp(){}
	void ParseHttpUrl(const string &strUrl,SHttpServerInfo & oServerInfo);
	
private:	
	static CCohClientImp * sm_instance;
	SHttpServerInfo m_oMonitorServerInfo;
	SHttpServerInfo m_oDetailMonitorServerInfo;
	SHttpServerInfo m_oDgsServerInfo;
	SHttpServerInfo m_oErrUrl;
};
#endif


