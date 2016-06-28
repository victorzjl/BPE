#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "SapConnection.h"
#include "SapHandlerAlloc.h"
#include "TimerManager.h"
#include "ThreadTimer.h"
#include "ISapSession.h"

using namespace std;
using namespace sdo::hps;
using boost::asio::ip::tcp;
using namespace sdo::common;

class CUpdateManager : public CTimerManager,
						public ISapSession
{
public:
	CUpdateManager();
	void Start();
	void Stop();

	static void GetAllServerInfo(string& strInfo);
	static string GetErrorString(int nError);

private:
	void StopThread(int nUpdataStat);
	void StartInThread();
	void DoTimer();
	void OnConnectTimeout();
	void OnRequestTimeout();

	void LoadConfig();
	void DoConnect();

	void DownLoadServerInfo();
	int ParseServerInfoResponse(CSapDecoder& oSapDecoder);
	void DownLoadUrlMapping();
	int ParseUrlMappingResponse(CSapDecoder& oSapDecoder);
	void DownLoadServiceFiles();
	int ParseServiceFilesResponse(CSapDecoder& oSapDecoder);

	void Update();
	int CmpUpdateFiles();
	int CmpFile(const string& strFile1, const string& strFile2);
	void GetCurrentServiceFiles(vector<string>& vecFiles);

protected:
	// impl of ISapSession
	virtual void OnConnectResult(unsigned int dwConnId,int nResult);
	virtual void OnReceiveSosAvenueResponse(unsigned int nId, const void *pBuffer,
		int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	virtual void OnReceiveAvenueRequest(unsigned int dwConnId, const void *pBuffer,
		int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	virtual void OnPeerClose(unsigned int nId);

private:
	boost::asio::io_service m_oIoService;
	boost::asio::io_service::work *m_oWork;
	boost::thread m_thread;
	boost::asio::deadline_timer m_timer;
	CSapHandlerAllocator m_allocTimeOut;
	bool m_bRunning;

	int m_nSequence;
	SapConnection_ptr m_ptrConn;
	CThreadTimer m_tmConnTimeout;
	CThreadTimer m_tmRequestTimeout;

	vector<string> m_vecOsapAddr;
	vector<string>::iterator m_iterOsapAddr;

	vector<string> m_vecServiceFiles;

public:
	static int sm_nUpdateOption;
	static boost::mutex sm_mut;
	static boost::condition_variable  sm_cond;
	static int sm_nUpateResult;
};

#endif // UPDATE_MANAGER_H
