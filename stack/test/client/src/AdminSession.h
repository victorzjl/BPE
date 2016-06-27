#ifndef _ADMIN_SESSION_H_
#define _ADMIN_SESSION_H_
#include "MsgThread.h"
#include "AdminMsg.h"
#include "ISapCallback.h"
using namespace sdo::common;
using namespace sdo::sap;
class CAdminSession:public CMsgThread,public ISapCallback
{

	typedef void (CAdminSession::*AdminFunc)(SAdminMsg *);	
public:
	CAdminSession();
	void OnLoginServer(const string &strIp,int nPort);
	void OnSendRequest(const string &strRequest);
public:
	virtual void OnConnectResult(int nId,int nResult);
	virtual void OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort){}
	virtual void OnReceiveMessage(int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotPort);
	virtual void OnPeerClose(int nId);

	virtual void Deal(void *pMsg);
private:
	void DoAdminLoginMsg(SAdminMsg * pMsg);
	void DoConnectResultMsg(SAdminMsg * pMsg);
	void DoReceiveMsg(SAdminMsg * pMsg);
	void DoPeerCloseMsg(SAdminMsg * pMsg);
	void DoRequestMsg(SAdminMsg * pMsg);

private:
	AdminFunc m_funcMap[ADMIN_ALL_MSG];
	string m_strName;
	string m_strPass;
	int m_nId;
	unsigned char m_szSessionKey[16];
};
#endif

