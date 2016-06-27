#ifndef _SESSION_MANAGER_H_
#define _SESSION_MANAGER_H_
#include "SapCommon.h"
#include "SapHandlerAlloc.h"
#include "SocSession.h"


#include "TimerManager.h"
#include "ThreadTimer.h"
#include "SapRecord.h"
#include "ComposeService.h"
#include <boost/thread.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/unordered_map.hpp>
#include "VirtualServiceManager.h"
#include "LogTrigger.h"
#include "AsyncVirtualService.h"
#include "CVirtualClientLoader.h"
#include "CPPFunctionManager.h"
#include "VirtualClientSession.h"

#define MAX_EXT_SOC_BUFFER  1024*1024*2
const int MANAGER_SELFCHECK_INTERVAL= 30;
const int SEND_ACK_REQUEST_WAIT_SECOND = 5;
class CSosSessionContainer;
class CSocSession;
class CSosSession;
//class CComposeServiceObj;
class CAVSSession;

//const string GetAddr(SapConnection_ptr ptrConn);
#define ALL_MSG 0xFFFFFFFF

class CSessionManager:public CTimerManager
{
public:
	CSessionManager(unsigned int dwId=0);
	void Start(unsigned int affinity);
	void OnLoadSos(const vector<SSosStruct> & vecSos);
	void OnLoadSuperSoc(const vector<string> & vecSoc,const vector<string> & strPrivelege);
	void OnReceiveSoc(SapConnection_ptr ptrConn);
	void OnLoadVirtualService();
    void OnLoadAsyncVirtualService();
	void OnLoadAsyncVirtualClientService();
	void OnRecvAVS(const void * buffer, unsigned int len);
	void OnRecvVirtualClient(const void * buffer, unsigned int len);
	void DoRecvVirtualClient(void *pBuffer, unsigned len);
	void OnRecvVirtualClientRequest(void* handel, const void * buffer, unsigned int len);
	int OnLoadConfig(const CComposeServiceContainer& composeConfig,const CAvenueServiceConfigs& serviceConfig);	
	void OnInstallSOSync(const string& strName, int &nRet);
	void OnUnInstallSOSync(const string& strName, int &nRet);
    void OnStop();
    void OnAddComposeObjNum(unsigned dwSize);
    void OnDecreaseComposeObjNum();

	const SComposeServiceUnit* GetComposeUnit(const SComposeKey& sKey)const
		{return m_oComposeConfig.FindComposeServiceUnit(sKey);}
	CAvenueServiceConfigs* GetServiceConfiger() {return &m_oServiceConfig;}
	CLogTrigger* GetLogTrigger(){return &m_oLogTrigger;}
	const unsigned int GetId()const{return m_dwId;}
	void OnGetPluginInfo(string &strInfo);
    void OnGetConfigInfo(string &strInfo);
    void OnSetConfig(const void* buffer, const unsigned len);

public:
	boost::asio::io_service & GetIoService(){return m_oIoService;}
	void OnConnected(CSosSession *pSos,const vector<unsigned int> &vecServiceId);
	bool IsSuperSoc(const string & strIp);	
	bool IsCanRequest(const string & strIp,unsigned int dwServiceId, unsigned int dwMsgId);	
	int OnRegistedSoc(CSocSession * pSoc);
	void OnReportRegisterSoc(CSocSession * pSoc);
	void OnDeleteSoc(CSocSession * pSoc);
	void OnDeleteSos(CSosSession * pSos);
	CSocSession* CSessionManager::OnGetSoc( const string& socName);
	CSosSession * OnReceiveRequest(SapConnection_ptr ptrConn,const void *pBuffer, int nLen, const void* pExhead=NULL, int nExlen=0);
	void OnManagerResponse(const void *pBuffer, int nLen);

	int OnRequestVirtuelService(unsigned int nServiceId, const void *pInPacket, unsigned int nLen,
			void **ppOutPacket, int *pLen);
	bool IsVirtualService(unsigned int nServiceId){ return m_virtualServiceManager.IsVirtualService(nServiceId);}
	void GetSocConfig(unsigned int dwIndex,const string & strName);
	void GetSocConfig(unsigned int dwIndex,const string &strIp,unsigned int port);
	void GetSocConfig(unsigned int dwIndex,const string & strName,const string &strIp,unsigned int port);
	void GetThreadSelfCheck(bool& isRun){ isRun = m_isAlive; m_isAlive = false;}
	CSosSession* OnGetSos(const unsigned int dwServiceId,const unsigned int dwMessageId = ALL_MSG, bool *isSosCfg=NULL)const;
    CAVSSession* OnGetAvs(){return m_avs;}
	CVirtualClientSession* OnGetVirtualClientSession(){return m_virtualClientSession;}
	bool isInAsyncService(unsigned serviceId);
	bool isInAsyncVirtualClient(unsigned serviceId);
	void OnAckRequestEnqueue(void *pBuffer, int nLen);
	void DoAckRequestEnqueue(void *pBuffer, int nLen);
	void RecordBusinessReq(const string& strAddr, const void* pBuffer, int nLen, int nCode,
		int nLevel, bool isCompose=false);
	void RecordRequest(const string& strAddr,int nServiceId, int nMsgId, int nCode);
    void RecordDataReq(const string& strIp, unsigned dwServiceId, unsigned dwMsgid, int nCode);
	const SConnectData GetConnData() {return m_connData;}
	void Dump();

    void gettimeofday(struct timeval_a* tm, bool bUseCache = true);

    bool IsAllComposeServiceObjClosed();
	void DoTransferVirtualClientResponse(void* handle, const void * pBuffer, int nLen);
	//SReportData m_oReportCompose;
private:
	void DoTimer();
	void DoSendAckRequest();
	void DoLoadSos(const vector<SSosStruct> & vecSos);
	void DoLoadSuperSoc(const vector<string> & vecSoc,const vector<string> & strPrivelege);
	void DoReceiveSoc(SapConnection_ptr ptrConn);
	void DoLoadConfig(CComposeServiceContainer oComposeConfig,CAvenueServiceConfigs oServiceConfig);
	void DoLoadVirtualService();
    void DoLoadAsyncVirtualService();
	void DoLoadAsyncVirtualClientService();
    void DoRecvAVS(void *buffer, unsigned len);
	void DoRecvVirtualClientRequest(void* handle, void *buffer, unsigned len);
    void DoSetConfig(void *buffer, unsigned len);
	void DoInstallSOSync(SVSReload *pReload, const string &strName);
	void DoUnInstallSOSync(SVSReload *pReload, const string &strName);
	void DoGetPluginInfo(SPlugInfo *pPlugInfo);
    void DoGetConfigInfo(string& config_info_);
	void DoSelfCheck();	
	const string DoGetAddr();
	void DoStopComposeService();
	void StartInThread();
    void Stop();
    void DoDecreaseComposeObjNum();
    void DoAddComposeObjNum(unsigned dwSize);
private:
	unsigned int m_dwSocIndex;
	vector<CSosSessionContainer *> m_vecSos;
	map<TServiceIdMessageId,CSosSessionContainer *> m_mapSos;
	map<unsigned int,CSocSession * > m_mapSoc;
	CVirtualServiceManager m_virtualServiceManager;
	CCPPFunctionManager m_cppFunctionManager;
    CAsyncVirtualService m_asyncVS;
	CVirtualClientLoader m_virtualClientLoader;
	CAVSSession *m_avs;
	CVirtualClientSession *m_virtualClientSession;
	boost::unordered_map<string,CSocSession * > m_mapRegistedSoc;

	boost::asio::io_service m_oIoService;
	boost::asio::io_service::work *m_oWork;
	boost::thread m_thread;


	boost::asio::deadline_timer m_tmTimeoOut;
	CSapHandlerAllocator m_allocTimeOut;
    bool bStopped;
	bool bRunning;
	bool m_isAlive;
	unsigned int m_dwId;
    unsigned m_dwComposeServiceObjAlive;
	SapLenMsgQueue m_ackRequestQueue;
	CThreadTimer m_tmSendAckRequest;
	CThreadTimer m_tmSelfCheck;
	CSapHandlerAllocator m_allocSelfCheck;

	vector<string> m_vecSuperSoc;
	vector<SServicePrivilege> m_vecSuperPrivilege;

	CComposeServiceContainer m_oComposeConfig;
	CAvenueServiceConfigs m_oServiceConfig;	
	CLogTrigger m_oLogTrigger;
	SConnectData m_connData;
	static const int RELOAD_TIMEWAIT = 30;
	static const int GETINFO_TIMEWARI = 10;

    struct timeval_a m_timeReceive;
};
#endif
