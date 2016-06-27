#ifndef _SDO_SAP_AGENT_H_
#define _SDO_SAP_AGENT_H_
#include "ISapCallback.h"
#include "SapConnection.h"
#include "SmallObject.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include "SapHandlerAlloc.h"
#include "ISapAgent.h"
using std::string;
using std::map;

namespace sdo{
namespace sap{

class CSapConnection;	
typedef boost::shared_ptr<CSapConnection> SapConnection_ptr;
class CSapAgent:public ISapAgent
{
public:
	CSapAgent();
	~CSapAgent(){}
	static CSapAgent *Instance();
	static CSapAgent *Instance2();
	virtual int StartServer(unsigned int dwPort);
	virtual void StopServer();
	virtual void SetCallback(ISapCallback *pCallback){m_pCallback=pCallback;}
	virtual void SetInterval(unsigned int dwRequestTimeout,int nHeartIntervl,int nHeartDisconnectInterval);
	//int do_get_sps_addr(const string &sag_ip,unsigned int sag_port,string &sps_ip,unsigned int *sps_port);

	/*return id*/
	virtual int DoConnect(const string &strIp, unsigned int dwPort,int nTimeout,unsigned int dwLocalPort=0);
	virtual int DoSendMessage(int nId,const void *pBuffer, int nLen,unsigned int dwTimeout=0);
	virtual void DoClose(int nId);

	/*for digest & enc*/
	virtual void SetLocalDigest(int nId,const string &strDigest);
	virtual void SetPeerDigest(int nId,const string &strDigest);
	virtual void SetEncKey(int nId,unsigned char arKey[16]);
	
	
	void OnConnectResult(int nId,int nResult);
	void OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort);
	void OnReceiveMessage(int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotPort);
	void OnPeerClose(int nId);

private:
	void StartAccept();
	void HandleStop();
	void HandleAccept(SapConnection_ptr new_connection,const boost::system::error_code &e);
private:
	static CSapAgent *sm_pInstance;
	static CSapAgent *sm_pInstance2;
	ISapCallback *m_pCallback;

	boost::asio::io_service &m_oIoService;
	boost::asio::ip::tcp::acceptor m_oAcceptor;

	CSapHandlerAllocator m_allocator;
	bool bRunning;

	
};
}
}
#endif

