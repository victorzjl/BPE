#ifndef _I_SESSION_H_
#define _I_SESSION_H_
#include "SocConfUnit.h"

class ISession
{
public:
	virtual void OnSessionTimeout()=0;
	virtual void OnConnectSucc()=0;
	virtual void OnReceiveMsg(void *pBuffer, int nLen)=0;
	virtual void OnSendMsg(const SSapCommandTest * pCmd)=0;
	virtual void Dump()const=0;
	virtual ~ISession(){}
};
#endif
