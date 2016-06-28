#ifndef _SOS_SESSION_H_
#define _SOS_SESSION_H_

#include <string>
#include "HpsCommonInner.h"
#include "ThreadTimer.h"
#include "SapConnection.h"
#include "SessionManager.h"
#include "ISapSession.h"

using namespace sdo::common;
using std::string;

class CHpsSessionManager;
class CSapConnection;

typedef boost::shared_ptr<CSapConnection> SapConnection_ptr;

class CSosSessionContainer;
class CSosSession : public ISapSession
{
public:
	CSosSession(){}
	CSosSession(unsigned int dwIndex,CHpsSessionManager *pManager, CSosSessionContainer *pSosSessionContainer,const string& strIp, unsigned int dwPort);
	virtual ~CSosSession();

public:
	void OnConnectResult(unsigned int dwConnId,int nResult);
	void OnReceiveSosAvenueResponse(unsigned int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	void OnReceiveAvenueRequest(unsigned int dwConnId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	void OnPeerClose(unsigned int nId);

	void DoConnect();
	void Close();
	void DoSendRequest(const void *pBuffer,int nLen,const void * exHeader,int nExLen,const char* pSignatureKey=NULL,unsigned int dwTimeout=0);
	
public:	
	bool IsConnected();
	unsigned int GetSosSessionId(){return m_dwId;}
	unsigned int GetConnId(){return m_dwConnId;}
	const string GetRemoteIp() const{return m_strIp;}
	const unsigned int GetRemotePort() const {return m_dwPort;}

	void Dump();

private:
	void OnConnectTimeout();
	
private:	
	bool m_bReConnectFlag;
	unsigned int m_dwId;
	unsigned int m_dwConnId;
	CHpsSessionManager *m_pSessionManager;
	CSosSessionContainer *m_pSosSessionContainer;

	SapConnection_ptr m_ptrConn;
	string m_strIp;
	unsigned int m_dwPort;

	ESosConnectState m_eConnectStatus;

	CThreadTimer m_tmReconnect;
	CThreadTimer m_tmConnTimeout;
};

#endif

