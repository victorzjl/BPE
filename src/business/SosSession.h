#ifndef _SOS_SESSION_H_
#define _SOS_SESSION_H_
#include "SapConnection.h"
#include "SessionManager.h"
#include "ISapSession.h"
#include "ThreadTimer.h"
#include "SapMessage.h"
#include "SapRecord.h"
#include "ITransferObj.h"
#include "IComposeBaseObj.h"

class CSessionManager;
class CSosSession:public ISapSession,public ITransferObj,public IComposeBaseObj
{
public:
	typedef enum emSosState
	{
		E_SOS_DISCONNECTED=1,
		E_SOS_CONNECTING=2,
		E_SOS_CONNECTED=3
		
	}ESosState;
	CSosSession(const string &strAddr,const vector<unsigned int>& vecServiceId,const string &strIP, unsigned int dwPort,CSessionManager *pManager);
	void Init();
	void SetIndex(int nIndex){m_nIndex=nIndex;ITransferObj::SetIndex(nIndex);}
	int Index()const{return m_nIndex;}
	ESosState State()const{return m_state;}
	virtual ~CSosSession();
public:
	virtual void WriteData(const void * pBuffer, int nLen){m_ptrConn->WriteData(pBuffer,nLen);}
	virtual void WriteData(const void * pBuffer, int nLen, const void* pExhead, int nExlen){m_ptrConn->WriteData(pBuffer,nLen,pExhead,nExlen);}
	void WriteComposeData(void* handle, const void * pBuffer, int nLen){m_ptrConn->WriteData(pBuffer,nLen);}

	virtual void OnConnectResult(int nResult);
	virtual void OnReadCompleted(const void * pBuffer, int nLen);
	virtual void OnPeerClose();

	void OnConnectTimeout();
	void Dump();
private:
	void DoConnect();
private:
	vector<unsigned int> m_vecServiceId;
	CSessionManager * m_pManager;
	string m_strIp;
	unsigned int m_dwPort;
	unsigned int m_seq;
	SapConnection_ptr m_ptrConn;
	ESosState m_state;
	
	
	CThreadTimer m_tmDisconnect;
	CThreadTimer m_tmConnecting;

	//SStatisticData m_sStatData;	
	int m_nIndex;
};


#endif

