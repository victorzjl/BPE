#ifndef ISAP_SESSION_H
#define ISAP_SESSION_H

#include <string>

using namespace std;

struct ISapSession
{
	virtual ~ISapSession(){}
	virtual void OnConnectResult(unsigned int dwConnId,int nResult) = 0;
	virtual void OnReceiveSosAvenueResponse(unsigned int nId, const void *pBuffer,
		int nLen,const string &strRemoteIp,unsigned int dwRemotePort) = 0;
	virtual void OnReceiveAvenueRequest(unsigned int dwConnId, const void *pBuffer,
		int nLen,const string &strRemoteIp,unsigned int dwRemotePort) = 0;
	virtual void OnPeerClose(unsigned int nId) = 0;
};

#endif // ISAP_SESSION_H
