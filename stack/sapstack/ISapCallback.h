#ifndef _SDO_SAPAGENTCALLBACK_H_
#define _SDO_SAPAGENTCALLBACK_H_
#include <string>
using std::string;

namespace sdo{
namespace sap{
class ISapCallback
{
public:
	virtual void OnConnectResult(int nId,int nResult)=0;
	virtual void OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort)=0;
	virtual void OnReceiveMessage(int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotPort)=0;
	virtual void OnPeerClose(int nId)=0;
	virtual ~ISapCallback(){}
};
}
}
#endif

