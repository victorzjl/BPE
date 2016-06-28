#ifndef _AYNC_LOG_THREAD_H_
#define _AYNC_LOG_THREAD_H_
#include "AsyncLogMsg.h"
#include "MsgTimerThread.h"
#include "IAsyncLogModule.h"
#include "HpsCommonInner.h"
#include "AsyncLogCommon.h"
#include "LogManager.h"
#include "AsyncLogMsg.h"
#include "LogConfig.h"
#include <set>

using std::set;
using namespace sdo::common;

#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 102400
#endif

const int DETAIL_LOG_INTERVAL = 10;
const int SELFCHECK_INTERVAL = 50;
const int REPORT_INTERVAL = 50;
const int RE_CONN_INTERVAL = 10;

class CHpsAsyncLogThread: public CMsgTimerThread, public IAsyncLogModule
{
	typedef void (CHpsAsyncLogThread::*AsyncLogFunc)(SAsyncLogMsg *);
	
public:
	static CHpsAsyncLogThread * GetInstance();
	virtual void StartInThread();
	virtual void Deal(void * pData);

    void RegisterLogger();

public:
	virtual void OnHttpConnectInfo(const SHttpConnectInfo &connInfo);
	virtual void OnReceiveHttpRequest(unsigned int dwServiceId, unsigned int dwMsgId);
	virtual void OnDetailLog(const SRequestInfo &reqInfo);
	virtual void OnReportSosStatus(const SSosStatusData &sosStatus);
	
	virtual void OnDetailAvenueRequestLog(SAvenueRequestInfo &sAvenueReqInfo);
	virtual void OnReceiveAvenueRequest(unsigned int dwServiceId, unsigned int dwMsgId);

	virtual void OnReportNoHttpResNum(int nThreadIndex, unsigned int nCurrentNoResNum);
	void OnReConnWarn(const string &strReConnIp);

	
	virtual const string & GetSelfCheckData();
	virtual unsigned int GetNoHttpResponseNum();

	void OnStop();


private:
	void DoHttpConnectInfo(SAsyncLogMsg * pMsg);
	void DoReceiveHttpRequest(SAsyncLogMsg * pMsg);
	void DoDetailLog(SAsyncLogMsg * pMsg);
	void DoReportSosStatus(SAsyncLogMsg * pMsg);
	void DoStop(SAsyncLogMsg * pMsg);
	void DoDetailAvenueRequestLog(SAsyncLogMsg * pMsg);
	void DoReceiveAvenueRequest(SAsyncLogMsg * pMsg);
	void DoReportNoHttpResNum(SAsyncLogMsg * pMsg);
	void DoReConnWarn(SAsyncLogMsg *pMsg);

private:
	CHpsAsyncLogThread();
	void DoDetailLogTimeout();
	void DoSelfCheck();
	void DoReport();
	void DoWarn();
	void ReportAvenueReqStatisticData();

private:
	static CHpsAsyncLogThread * sm_pInstance;

	string m_strIp;
	boost::mutex m_mutex;
	string m_strSelfCheck;

	char m_szHttpConnInfoBuf[MAX_BUFFER_SIZE];
	int m_nConnInfoLoc;
	char m_szRequestBuf[MAX_BUFFER_SIZE];
	int m_nRequestLoc;

	char m_szAvenueRequestBuf[MAX_BUFFER_SIZE];
	int m_nAvenueRequestLoc;
	
	map<unsigned int, SSosStatisticData> m_mapStatisticDataByServiceId;
	map<unsigned int, SSosStatisticData> m_mapStatisticOldDataByServiceId;

	SHPSStatisticData m_sHpsStatisticData;
	SHPSStatisticData m_sHpsStatisticOldData;

	map<SServiceMsgId, SSosStatisticData> m_mapStatisticDataBySrvMsgId;
	map<SServiceMsgId, SSosStatisticData> m_mapStatisticOldDataBySrvMsgId;

	SSosStatusData m_sSosStatusData;


	SAvenueReqStatisticData m_sAvenueReqStatisticDataAll;
	SAvenueReqStatisticData m_sOldAvenueReqStatisticDataAll;
	
	map<unsigned int, SAvenueReqStatisticData> m_mapAvnReqStatisticDataByServiceId;
	map<unsigned int, SAvenueReqStatisticData> m_mapAvnReqStatisticOldDataByServiceId;
	
	map<SServiceMsgId, SAvenueReqStatisticData> m_mapAvnReqStatisticDataBySrvMsgId;
	map<SServiceMsgId, SAvenueReqStatisticData> m_mapAvnReqStatisticOldDataBySrvMsgId;

	CThreadTimer m_tmDetailLog;
	CThreadTimer m_tmSelfcheck;
	CThreadTimer m_tmReport;
	CThreadTimer m_tmConnectWarn;
	set<string> m_setReConnIps;

	boost::mutex m_mutexNoResNum;
	map<int, unsigned int> m_mapNoHttpResNumByThreadId;

	AsyncLogFunc m_funMap[ASYNCLOG_ALL_MSG];

	LogConfig m_logConfg;
	string m_defautLogKey1;
	string m_defautLogKey2;
};
#endif

