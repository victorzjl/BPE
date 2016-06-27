#ifndef __I_TRANSFER_OBJ__H_
#define __I_TRANSFER_OBJ__H_

#include "SapConnection.h"
#include "Utility.h"
#include "LogTrigger.h"
#include <string>
#include "TimerManager.h"
#include "ThreadTimer.h"

using std::string;
using namespace sdo::common;


typedef enum
{
	SAP_Request_Remote = 0,
	SAP_Request_Local = 1,
	SAP_Request_Compose_Service = 2,
	SAP_Request_MapQ = 3,
	SAP_Request_AsyncVS = 4,
    SAP_Request_Data = 5,
	SAP_Request_VirtualClient = 6,
	SAP_Request_ALL
}ERequestSource;

typedef struct stAckInfo
{
	bool bAckMsg;
	void *pExBuffer;
	int nExLen;
	stAckInfo()
		: bAckMsg(false), pExBuffer(NULL), nExLen(0)
	{}
}SAckInfo;


class CComposeServiceObj;
class CSessionManager;

typedef struct stComposeTransfer
{
    CComposeServiceObj* pObj;
	unsigned int dwMsgId;
	unsigned int dwServiceId;
    //stComposeTransfer():pObj(NULL){}
    stComposeTransfer(CComposeServiceObj* pObj_=NULL,
        unsigned msgId_=0, 
        unsigned serviceId_=0)
        :pObj(pObj_),dwMsgId(msgId_),dwServiceId(serviceId_){}
}SComposeTransfer;

typedef struct stSTransferData
{
    unsigned dwTimeout;
    bool bAckMsg;
    stSTransferData(unsigned dwTimeout_, bool bAckMsg_=false)
        :dwTimeout(dwTimeout_),bAckMsg(bAckMsg_)
        {}
}STransferData;


typedef struct stPeerSeqPtr
{
	unsigned int dwSeq;
	unsigned int dwMsgId;
	unsigned int dwServiceId;
    string strAddr;
    SComposeTransfer sCompose;
	SapConnection_ptr ptrConn;
	ERequestSource eReqSource;
    
	SAckInfo sAckInfo;
	SRequestInfo sInfo;
	SLogInfo sLog;
	struct timeval_a tmReceived;
    unsigned long long ullTimeout;
	void* handle;
	stPeerSeqPtr():handle(NULL){}
}SPeerSeqPtr;


class ITransferObj
{
    typedef multimap<unsigned long long, unsigned int> TimeStampMMap;
public:
	ITransferObj(const string & strModule,int level,int nIndex,const string &strAddr,CSessionManager *pManager);
	void SetIndex(int nIndex){m_nIndex=nIndex;}
    void OnTransferDataRequest(const void *pBuffer, int nLen,
        const SComposeTransfer& sCompose,
        const STransferData& sData);
    int OnTransferAVSRequest(const void *pBuffer, int nLen, 
        const SComposeTransfer& sCompose,
        const STransferData& sData);
    int OnTransferAVSRequest(SapConnection_ptr ptrConn, const void *pBuffer, int nLen);
	int OnTransferAVSRequest(const void *pBuffer, int nLen, void* handle=NULL);
	void OnTransferRequest(const void *pBuffer, int nLen,
        const SComposeTransfer& sCompose,
        const STransferData& sData);
	void OnTransferRequest(ERequestSource eReqFrom,const void *pBuffer, int nLen, void* handle=NULL);
	void OnTransferRequest(ERequestSource eReqFrom,SapConnection_ptr ptrConn,const void *pBuffer, int nLen);
	void OnTransferRequest(ERequestSource eReqFrom, SapConnection_ptr ptrConn,const void *pBuffer, int nLen,const void* pExhead, int nExlen);
	void OnTransferMapQRequest(void *pExBuffer, int nExLen);
	void OnTransferResponse(const void * pBuffer, int nLen);
	void OnStopComposeService();
	unsigned GetSendErrorCount(){return m_nSendErrorCount;}
	const string &Addr() const{return m_strAddr;}
    virtual int WriteAvs(const void * pBuffer, int nLen){return 0;}
	virtual void WriteData(const void * pBuffer, int nLen)=0;
	virtual void WriteData(const void * pBuffer, int nLen, const void* pExhead, int nExlen)=0;
	virtual ~ITransferObj();
private:
	void OnTransferTimeout();
	void RecordOperate(const SPeerSeqPtr& oSeq, int nCode);

protected:
	unsigned m_nSendErrorCount;
private:
	unsigned int m_seq;
	string m_strModule;
	int m_nIndex;	
protected:
	string m_strAddr;
private:
	CSessionManager * m_pManager;	
	CThreadTimer m_tmTimeout;
	map<unsigned int,SPeerSeqPtr> m_SocSeqPtr;
    TimeStampMMap m_TimeList;
	int m_level;
};
#endif
