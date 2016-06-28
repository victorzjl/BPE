#ifndef _SESSION_MANAGER_H_
#define _SESSION_MANAGER_H_
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "TimerManager.h"
#include "ThreadTimer.h"
#include "SapHandlerAlloc.h"
#include "SosSessionContainer.h"
#include "HpsConnection.h"
#include "HpsSessionContainer.h"
#include "HpsCommonInner.h"
#include "HpsCommon.h"
#include "AvenueHttpTransfer.h"
#include "HttpRequestConfigManager.h"
#include "HttpClientContainer.h"
#include "OsapManager.h"
#include "IpLimitManager.h"

#define REQUEST_TIME_INTERVAL 30000
#define OSAP_REQUEST_TIME_INTERVAL 10

const int REPORT_DATA_INTERVAL = 30;
const int SOC_SELFCHECK_INTERVAL = 30;
const int SELF_CHECK_INTERVAL = 10;
const int REQUEST_TIME_OUT = 10;

class CHpsSessionContainer;
class CSosSessionContainer;
class CAvenueHttpTransfer;
class CHpsConnection;
class CResourceAgent;
typedef boost::shared_ptr<CHpsConnection> HpsConnection_ptr;

class CHttpRecVirtualClient;

typedef struct stServiceSosAddr{
	vector<unsigned int> vecServiceId;
	vector<string> vecSosAddr;
}SServiceSosAddr;
class CHpsSessionManager:public CTimerManager
{
	friend class COsapManager;
	typedef multimap<struct timeval_a,SRequestInfo> MMHistoryTimers;
public:
    CHpsSessionManager(CHttpRecVirtualClient *pVirtualClient, int nThreadId);
    ~CHpsSessionManager();
    void Start(unsigned int affinity);
    void Stop();
    boost::asio::io_service &GetIoService(){return m_oIoService;}
    CHpsSessionContainer *GetHpsSessionContainer(){return m_pHpsContainer;}
	
public:
	void AddConnection(HpsConnection_ptr ptrConn);
	void OnLoadTransferObjs(const string &strH2ADir, const string &strH2AFilter, const string &strA2HDir, const string &strA2HFilter);
	void OnLoadSosList(const vector<string> & vecServer);
	void OnUnLoadSharedLibs(const string &strDirPath,const vector<string> &vecSo);
	void OnReLoadSharedLibs(const string & strDirPath,const vector<string> &vecSo);
	void OnLoadRouteConfig(const vector<string> &vecRouteBalance);
	void OnUpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode);
	void OnUpdateIpLimit(const map<string, string> & mapIpLimits);
	
	void OnUpdataUserInfo(const string& strUserName, OsapUserInfo* pUserInfo);
	void OnUpdataOsapUserTime(const string& strUserName, const timeval_a& sTime);
	void OnClearOsapCatch(const string& strMerchant);
	void OnReLoadCommonTransfer(const vector<string>& vecServer, const string& strTransferName);
    void OnResourceRequestCompleted(SRequestInfo &sReq);
	SUrlInfo GetSoNameByUrl(const string& strUrl);
    int FindUrlMapping(const string& strUrl, unsigned int& dwServiceID, unsigned int& dwMsgID);

public:	
	void OnHttpConnected( unsigned int dwId, const string &strRemoteIp,unsigned int dwRemotePort);
	void DoReceiveHttpRequest(SRequestInfo &sReq, const string &strRemoteIp,unsigned int dwRemotePort);

	void DoHttpPeerClose(unsigned int dwId, const string &strRemoteIp,const unsigned int dwRemotePort);



private:
	void DoHttpConnected(unsigned int dwId, const string &strRemoteIp,unsigned int dwRemotePort);
	void DoLoadRouteConfig(const vector<string> &vecRouteBalance);
	void DoUpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode);

public:
	void OnConnected(unsigned int dwSosContainerId, unsigned int dwSosSessionId, unsigned int dwConnId);
	void OnReceiveSosAvenueResponse(unsigned int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	void OnPeerClose(unsigned int nId);

	void OnReceiveAvenueRequest(unsigned dwSosContainerId, unsigned int dwSosSessionId, unsigned int dwConnId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	void OnReceiveHttpResponse(SAvenueRequestInfo &sAvenueReqInfo);
        
    void OnReceiveServiceResponse(const void *pBuffer, int nLen);
	
public:
	string GetPluginSoInfo();
	void GetSelfCheck(bool &isRun);
	static void RecordeHttpRequest(SRequestInfo & sReq);
	void Dump();

	void GetUrlMapping(vector<string> &vecUrlMappin);
	
/*************************************************************************************8*/
private:
	void DoTimer();
    void StopInThread();
	void StartInThread();
	void AddConnectionInner(HpsConnection_ptr ptrConn);
	void DoLoadTransferObjs(const string &strH2ADir, const string &strH2AFilter, const string &strA2HDir, const string &strA2HFilter);
	void DoLoadSosList(const vector<string> & vecServer);
	void DoUnLoadSharedLibs(const string &strDirPath,const vector<string> &vecSo);
	void DoReLoadSharedLibs(const string & strDirPath,const vector<string> &vecSo);
	void DoReLoadCommonTransfer(const vector<string>& vecServer, const string& strTransferName);
	void DoUpdateIpLimit(const map<string, string> & mapIpLimits);

	CSosSessionContainer* GetContainerPtrBySosAddr(const vector<string> &vecSosAddr);
private:
	void AddHistoryRequest(SRequestInfo &sReq);
	void RemoveHistory(SRequestInfo &sReq);
	void DetectRequestTimerList();
	void DoSelfCheck();
	void DoReportSosStatusCheckData();

	int DoSendAvenueRequest(SRequestInfo &sReq, int nCode, OsapUserInfo* pUserInfo);
	int SendHttpResponse(SRequestInfo &sReq);

	int SendOsapRequest(const void* pBuffer, int nLength);
	void DoUpdataUserInfo(const string& strUserName, OsapUserInfo* pUserInfo);
	void DoUpdataOsapUserTime(const string& strUserName, const timeval_a& sTime);
	void DoClearOsapCatch(const string& strMerchant);

	void DoSendHttpRequest( SAvenueRequestInfo &sAvenueReqInfo, const void * pBuffer, int nLen, OsapUserInfo* pUserInfo);
	void AvenueRequestFail(SAvenueRequestInfo &sAvenueReqInfo);
	int SendAvenueResponse(SAvenueRequestInfo &sAvenueReqInfo, const void *pBuffer, int nLen);

	void RecordAvenueRequest(SAvenueRequestInfo &sAvenueReqInfo);
        
        int DoHandleResourceRequest(SRequestInfo &sReq);
        void DoReceiveServiceResponse(const void *pBuffer, int nLen);

private:
	int m_nThreadId;
	map<unsigned int, CSosSessionContainer*> m_mapSosSessionContainerByContainerId;
	map<unsigned int, CSosSessionContainer*> m_mapSosSessionContainerByServiceId; // by serviceId

	CHpsSessionContainer *m_pHpsContainer;
	
	CAvenueHttpTransfer *m_pTransfer;

	boost::asio::io_service m_oIoService;
	boost::asio::io_service::work *m_oWork;
	boost::thread m_thread;

	map<unsigned int, SRequestInfo> m_mapSeqReqInfo;  //key-sequence value-reqInfo
	//multimap<unsigned int, unsigned int >  m_multimapIdSeq;
	MMHistoryTimers m_mapEndtimeReqInfo;
	
	unsigned int m_dwSeq;

	bool m_isAlive;
	boost::asio::deadline_timer m_tmTimeOut;
	int m_nTimeCount;
	CSapHandlerAllocator m_allocTimeOut;

        STransferDataStruct m_szTransferData;
	int m_nLen;

	unsigned int m_dwSosContainerCount; //used when create a new SosSessionContainer ptr index

	CThreadTimer m_tmSelfCheck;
	CThreadTimer m_timerSocSelfCheck;
	//CThreadTimer m_timerRequest;

	CHttpRequestConfigManager m_oHttpReqConfigManager;
	CHttpClientContainer *m_pHttpClientContainer;
	COsapManager m_osapManager;

	vector<string> m_vecUrlMappingForCheck;

    CIpLimitCheckor m_ipLimitCheckor;
    
    CHttpRecVirtualClient *m_pVirtualClient;
    
    CResourceAgent *m_pResourceAgent;
};

#endif

