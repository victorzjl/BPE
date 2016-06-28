#ifndef _I_AYSNC_LOG_MODULE_H_
#define _I_AYSNC_LOG_MODULE_H_
#include "HpsCommonInner.h"

class IAsyncLogModule
{
public:
	virtual void OnHttpConnectInfo(const SHttpConnectInfo &connInfo) = 0;
	virtual void OnReceiveHttpRequest(unsigned int dwServiceId, unsigned int dwMsgId) = 0;
	virtual void OnDetailLog(const SRequestInfo &reqInfo) = 0;
	virtual void OnReportSosStatus(const SSosStatusData &sosStatus) = 0;
	virtual void OnDetailAvenueRequestLog(SAvenueRequestInfo &sAvenueReqInfo) = 0;
	virtual void OnReceiveAvenueRequest(unsigned int dwServiceId, unsigned int dwMsgId) = 0;
	virtual void OnReportNoHttpResNum(int nThreadIndex, unsigned int nCurrentNoResNum) = 0;

	virtual const string & GetSelfCheckData() = 0;
	virtual unsigned int GetNoHttpResponseNum() = 0;

};

IAsyncLogModule * GetAsyncLogModule();

#endif

