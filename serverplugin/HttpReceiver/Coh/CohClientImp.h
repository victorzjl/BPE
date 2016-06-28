#ifndef _COH_CLIENT_IMP_H_
#define _COH_CLIENT_IMP_H_

#include "CohClient.h"
#include "HpsCommonInner.h"
#include <string>

using namespace sdo::coh;
using std::string;

class CHpsCohClientImp:public ICohClient
{
public:
	static CHpsCohClientImp * GetInstance();
	void Init(const string &strMonitorUrl, const string &strDetailMonitorUrl, const string &strDgsUrl, const string &strErrorUrl);
	void SendReportDataToMonitor(const string &strContent);
	void SendDetailReportDataToMonitor(const string &strContent);
	void SendToDgs(const string &strContent);
	void SendErrorDataToMonitor(const string &strContent);

public:
	virtual void OnReceiveResponse(const string &strResponse);

private:
	CHpsCohClientImp(){}
	void ParseHttpUrl(const string &strUrl,SHttpServerInfo & oServerInfo);

private:	
	static CHpsCohClientImp * sm_instance;
	SHttpServerInfo m_oMonitorServerInfo;
	SHttpServerInfo m_oDetailMonitorServerInfo;
	SHttpServerInfo m_oDgsServerInfo;
	SHttpServerInfo m_oErrorServerInfo;
};
#endif

