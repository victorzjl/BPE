#ifndef _SESSION_MANAGER_H_
#define _SESSION_MANAGER_H_
#include <string>
#include "MsgTimerThread.h"
#include <vector>
#include "SocConfUnit.h"
#include "SessionAdapter.h"
#include "SessionManagerMsg.h"
#include "ISapCallback.h"
#include <boost/unordered_map.hpp>

#include <map>
using std::string;
using std::vector;
using std::map;
using namespace sdo::common;
using namespace sdo::sap;
class CSessionManager:public CMsgTimerThread,public ISapCallback
{
	typedef void (CSessionManager::*SessionMsgFunc)(SSessionMsg *);	
public:
	static CSessionManager * Instance();
	
	int LoadClientSetting(const string & strPath);
	int LoadServerSetting(const string & strPath);
	string GetUserPass(const string &strName);

	
	void StartClient();
	int StartServer();
	void OnSessionTimeout(int nId);
	void OnSendSessionRequest(int nId,const SSapCommandTest *pCmd);

	virtual void Deal(void *pData);
	
	virtual void OnConnectResult(int nId,int nResult);
	virtual void OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort);
	virtual void OnReceiveMessage(int nId,const void *pBuffer, int nLen,const std::string& strRemoteIp, unsigned int dwRemotePort);
	virtual void OnPeerClose(int nId);

	string GetAllSessions()const;
	void Dump()const;
private:
	
	/*message mapping function*/
	void DoStartClient_(SSessionMsg * pMsg);
	void DoSessionTimeout_(SSessionMsg * pMsg);
	void DoConnectResultMsg_(SSessionMsg * pMsg);
	void DoReceiveMsg_(SSessionMsg * pMsg);
	void DoPeerCloseMsg_(SSessionMsg * pMsg);
	void DoReceiveConnectMsg_(SSessionMsg * pMsg);
	void DoSendCmdMsg_(SSessionMsg * pMsg);

	/*map operator*/
	CSessionAdapter * CreateSession(int nId,const string &strIp,unsigned int dwPort,SSocConfUnit * pConfUnit,const map<int,SSapCommandResponseTest *> &mapResponse);
	CSessionAdapter * CreateSession(int nId,const string &strIp,unsigned int dwPort,const map<int,SSapCommandResponseTest *> &mapResponse);
	CSessionAdapter * GetSession(int nId);
	void DelSession(int nId);
	void DelSession(CSessionAdapter *pSession);
private:
	CSessionManager();
	static CSessionManager * sm_pinstance;
	const static SessionMsgFunc m_funcMap[E_SESSION_ALL_MSG];	
	string m_strServerIp;
	int m_nServerPort;

	int m_nAppId;
	int m_nAreaId;
	
	vector<SSocConfUnit *> m_vecUnits;

	int m_nListenPort;
	map<int,SSapCommandResponseTest *> m_mapResponse;
	boost::unordered_map<string,string> m_mapUsers;

	map<int,CSessionAdapter *> m_mapSession;
};
#endif

