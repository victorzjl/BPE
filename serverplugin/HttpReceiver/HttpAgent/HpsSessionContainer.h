#ifndef _HPS_SESSION_CONTAINER_H_
#define _HPS_SESSION_CONTAINER_H_

#include "HpsConnection.h"
#include "HpsCommonInner.h"
#include "ThreadTimer.h"
#include "PreProcess.h"
#include "HpsSession.h"
#include <boost/unordered_map.hpp>
#include <map>
#include <string>
#include <algorithm>

using namespace sdo::common;


const int HTTP_CONNECTION_DETECT_INTERVAL = 30;

class CHpsSession;
class CHpsSessionManager;
class CHpsConnection;
struct SRegistInfo;
typedef boost::shared_ptr<CHpsConnection> HpsConnection_ptr;

class CHpsSessionContainer
{
public:
	CHpsSessionContainer(CHpsSessionManager *pManager);
	~CHpsSessionContainer();

	void StartInThread();
    void StopInThread();

	void OnHttpConnected(unsigned int dwId);
	void OnReceiveHttpRequest( SRequestInfo &sReq, const string &strRemoteIp,unsigned int dwRemotePort);
	void OnHttpPeerClose( unsigned int dwId, const string &strRemoteIp,const unsigned int dwRemotePort);
	
	void DoSendResponse(const SRequestInfo &sReq);

	void LoadMethodMapping(const vector<string> &vecUrlMapping);
	int FindUrlMapping(const string& strUrl,unsigned int& dwServiceID,unsigned int& dwMsgID);
	int RequestPrePrecess(SRequestInfo& sReq);
	void AddConnection(HpsConnection_ptr ptrConn);
	CHpsSession * GetConnection(unsigned int nId);
	void DelConnection(unsigned int nId);

	void DoCheckHttpConnection();

	void Dump();

private:
	//map<unsigned int, HpsConnection_ptr> m_mapHpsConn;
	map<unsigned int, CHpsSession*> m_mapHpsSession;
	map<string,SServiceMsgId> m_mapUrlMsg;
	CHpsSessionManager *m_pSessionManager;
	PreProcess m_preProcess;

	CThreadTimer m_tmConnCheck;
};

#endif

