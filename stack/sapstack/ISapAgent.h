#ifndef _I_SDO_SAP_AGENT_H_
#define _I_SDO_SAP_AGENT_H_
#include "ISapCallback.h"

namespace sdo{
namespace sap{
	
class ISapAgent
{
public:
	virtual int StartServer(unsigned int dwPort)=0;
	virtual void StopServer()=0;
	virtual void SetCallback(ISapCallback *pCallback)=0;
	virtual void SetInterval(unsigned int dwRequestTimeout,int nHeartIntervl,int nHeartDisconnectInterval)=0;

	/*return id*/
	virtual int DoConnect(const string &strIp, unsigned int dwPort,int nTimeout,unsigned int dwLocalPort=0)=0;
	virtual int DoSendMessage(int nId,const void *pBuffer, int nLen,unsigned int dwTimeout=0)=0;
	virtual void DoClose(int nId)=0;
	
	virtual ~ISapAgent(){}

	/*for digest & enc*/
	virtual void SetLocalDigest(int nId,const string &strDigest)=0;
	virtual void SetPeerDigest(int nId,const string &strDigest)=0;
	virtual void SetEncKey(int nId,unsigned char arKey[16])=0;
};
ISapAgent * GetSapAgentInstance();
ISapAgent * GetServerAgentInstance();

}
}
#endif

