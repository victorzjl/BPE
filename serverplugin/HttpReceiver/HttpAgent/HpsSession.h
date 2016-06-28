#ifndef _HPS_SESSION_H_
#define _HPS_SESSION_H_

//#include "HpsSessionContainer.h"
#include "HpsCommonInner.h"
#include "HpsConnection.h"
#include "SapBatResponseHandle.h"
#include "RequestDecoder.h"

#include <string>
#include <deque>
using std::deque;
using std::string;

typedef enum
{
	CONNECT_CREATED_FIRST = 0,
	CONNECT_CONNECTED = 1,
	CONNECT_TIME_OUT = 2
}EConnectStatus;

class CHpsConnection;
class CHpsSessionContainer;
typedef boost::shared_ptr<CHpsConnection> HpsConnection_ptr;

class CHpsSession
{
public:	
	void OnReceiveHttpRequest(char * pBuff, int nLen,const string & strRemoteIp,unsigned int dwRemotePort);
	void OnHttpPeerClose(const string &strRemoteIp,const unsigned int dwRemotePort);
	void DoSendResponse(const SRequestInfo &sReq);
	void Close();
	
public:
	CHpsSession(){}
	CHpsSession(unsigned int dwId, HpsConnection_ptr ptrConn, CHpsSessionContainer *pContainer);
	virtual ~CHpsSession();

	unsigned int GetId(){return m_dwId;}
	string GetRemoteIp(){return m_ptrConn->GetRemoteIp();}
	unsigned int GetRemotePort(){return m_ptrConn->GetRemotePort();}
	EConnectStatus GetConnectStatus(){return m_eConnectStatus;}
	void SetConnectStatus(EConnectStatus nStatus){m_eConnectStatus = nStatus;}

	void Dump();
private:
    void HandleRequest(SRequestInfo &sReq);
	void SendAllResponse();
    void SendErrorResponse(SRequestInfo &sReq, ERequestType type);
    void SendErrorResponse(SDecodeResult &sResult);
    
        
private:
	unsigned int m_dwId;
	EConnectStatus m_eConnectStatus; //0 -- first created, 1 -- connected, 2 -- timeout
	
	HpsConnection_ptr m_ptrConn;
	CHpsSessionContainer *m_pContainer;

	int m_nOrder;
	deque<sdo::hps::CSapBatResponseHandle> m_dequeResponse;
        std::map<int, sdo::hps::CSapBatResponseHandle> m_batResponseHandle;
};

#endif

