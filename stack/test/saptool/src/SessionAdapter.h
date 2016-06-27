#ifndef _SESSION_ADAPTER_H_
#define _SESSION_ADAPTER_H_
#include "ISession.h"
#include "SocConfUnit.h"
#include <map>

using std::map;
typedef enum
{
	E_SESSION_SOC,
	E_SESSION_SOS
}ESessionType;
class CSessionAdapter
{
public:
	CSessionAdapter(int nId,const string &strIp,unsigned int dwPort,SSocConfUnit * pConfUnit,const map<int,SSapCommandResponseTest *> &mapResponse);
	CSessionAdapter(int nId,const string &strIp,unsigned int dwPort,const map<int,SSapCommandResponseTest *> &mapResponse);
	~CSessionAdapter();
	int Id()const{return m_nId;}
	ESessionType GetType()const{return m_type;}
	string GetPeerIp()const{return m_strIp;}
	unsigned int GetPeerPort()const{return m_dwPort;}
	SSocConfUnit *ConfUnit()const;

	void OnSessionTimeout();
	void OnConnectSucc();
	void OnReceiveMsg(void *pBuffer, int nLen);
	void OnSendMsg(const SSapCommandTest * pCmd);
	
	void Dump();
private:
	int m_nId;
	ESessionType m_type;
	string m_strIp;
	unsigned int m_dwPort;
	
	ISession * m_pSession;
};
#endif
