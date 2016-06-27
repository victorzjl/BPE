#ifndef _SOC_SESSION_H_
#define _SOC_SESSION_H_
#include "SapConnection.h"
#include "ISapSession.h"
#include "SocPrivilege.h"
#include "SessionManager.h"
#include "ThreadTimer.h"
#include "SapMessage.h"
#include "SosSession.h"
#include "SapRecord.h"
#include "ITransferObj.h"
#include "IComposeBaseObj.h"
#include <arpa/inet.h>

#define PUBLIC_CONFIG_ENCKEY "ave@)!))&@#nue00"

class CSessionManager;
class CSosSession;
class CSocSession:public ISapSession,public ITransferObj,public IComposeBaseObj
{
public:
	typedef enum enSocType
	{
		E_UNREGISTED_SOC=0,
		E_REGISTED_SOC=1,
		E_SUPER_SOC=2,
		E_SOC_TYPE
	}ESocType;
	
	typedef enum enSocState
	{
		E_SOC_UNREGISTED=1,
		E_SOC_RECEIVED_REGISTER=2,
		E_SOC_RECEIVED_REREGISTER=3,
		E_SOC_REGISTED
	}ESocState;

	CSocSession(unsigned int dwIndex,const string &strAddr,SapConnection_ptr ptrConn,CSessionManager *pManager);
	virtual ~CSocSession();
	void Init(bool isSuper);
	unsigned int Index()const{return m_dwId;}
	const string & UserName()const{return m_strUserName;}
	ESocType LoginType()const{return m_dwLoginType;}
	const string &SdkVersion()const{return m_strVersion;}
	const string &SdkBuildTime()const{return m_strBuildTime;}
	bool IsRegisted()const{return m_state==E_SOC_REGISTED;}
	
	void DoGetSocConfigResponse(const void *pBuffer,unsigned int nLen);
	void DoDeleteSos(CSosSession * pSos);
	
public:
	virtual void WriteData(const void * pBuffer, int nLen){m_ptrConn->WriteData(pBuffer,nLen);}
	virtual void WriteData(const void * pBuffer, int nLen, const void* pExhead, int nExlen){m_ptrConn->WriteData(pBuffer,nLen,pExhead,nExlen);}
	void WriteComposeData(void* handle, const void * pBuffer, int nLen){m_ptrConn->WriteData(pBuffer,nLen);}
	virtual void OnConnectResult(int nResult){}
	virtual void OnReadCompleted(const void * pBuffer, int nLen);
	virtual void OnPeerClose();
	void Dump();
private:
	void OnReceiveGetConfig(const void * pBuffer, int nLen);
	void OnReceiveRegister(const void * pBuffer, int nLen);
	void OnReceiveOtherRequest(const void * pBuffer, int nLen);
	
private:
	void FastResponse(unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwSequence,unsigned int dwCode);
	void DetectIsResponse();
	int  OnRegisted(const string & strName,ESocType logintype);	
	void Close();
private:
	bool m_isSuper;
	unsigned int m_dwId;
	unsigned int m_seq;
	
	CSessionManager * m_pManager;
	SapConnection_ptr m_ptrConn;
	ESocState m_state;
	bool isCompletedGetConfig; //whether have getted Config
	bool isGetConfig;     // if received getconfig, true, if received response, false again
	unsigned int m_seqGetconfig; //if received getconfig, save request sequence
	CThreadTimer m_tmConnected;
	
	string m_strUserName;
	
	CSocPrivilege m_privilege;

	unsigned int m_dwLastSeq;

	unsigned char m_arNounce[16];

	unsigned char m_arDegest[16];
	string m_strPass;
	string m_strKey;
	string m_strIp;
	unsigned int m_dwPort;

	unsigned char m_arAesKey[16];

	ESocType m_dwLoginType;
	
	map<unsigned int ,CSosSession *> m_mapLastSos;

	in_addr_t m_peerAddr;
	int m_netPort;
	char m_exHeader[256];
	int m_exHeadLen;

	string m_strVersion;
	string m_strBuildTime;
	unsigned int m_dwAreaId;
	unsigned int m_dwGroupId;
	unsigned int m_dwHostId;
	unsigned int m_dwAppId;
	unsigned int m_dwSpId; 
};

#endif

