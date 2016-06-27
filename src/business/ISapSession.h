#ifndef _I_SAP_SESSION_H_
#define _I_SAP_SESSION_H_

class ISapSession
{
public:
	virtual void OnConnectResult(int nResult)=0;
	virtual void OnReadCompleted(const void * pBuffer, int nLen)=0;
	virtual void OnPeerClose()=0;
};
#endif
