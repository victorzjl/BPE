#ifndef _SESSION_SOS_H_
#define _SESSION_SOS_H_
#include "ISession.h"
#include "SocConfUnit.h"
#include <map>
using std::map;
class CSessionSos:public ISession
{
public:
	CSessionSos(int nId,const map<int,SSapCommandResponseTest *> &mapResponse);
	
	virtual void OnSessionTimeout(){}
	virtual void OnConnectSucc(){}
	virtual void OnReceiveMsg(void *pBuffer, int nLen);
	virtual void OnSendMsg(const SSapCommandTest * pCmd);
	virtual ~CSessionSos(){}

	virtual void Dump()const;
private:
	int DoAdminRequest_(void *pBuffer,int nLen);
	
	int m_nId;
	const map<int,SSapCommandResponseTest *> &m_mapResponse;

	bool m_bAesTransfer;
	int m_type;
	unsigned char m_szNonce[16];
	unsigned char m_szAesKey[16];

	string m_strPass;

	static string sm_strUser;
	static string sm_strPass;
	unsigned char m_szKey[16];
};
#endif
