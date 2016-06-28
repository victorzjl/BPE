#ifndef _MQS_THREAD_H_
#define _MQS_THREAD_H_

#include <vector>
#include <ChildMsgThread.h>

#include "MQSMsg.h"
#include "KafkaCommon.h"
#include "ReConnThread.h"

#include "KafkaProducer.h"
#include <boost/thread/tss.hpp>

using namespace sdo::common;

class CMQSReConnHelperImpl : public IReConnHelper, public IKafkaErrorCallBack
{
public:
	virtual int TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3);
	virtual int ReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3) { return 0; }
	int OnKafkaError(IKafka *, const std::string&) { return 0; }
};

typedef std::pair< SKafkaBTP, vector<unsigned int> >  PairServiceInfo;

class CMQSThreadGroup;
class CMQSThread : public CChildMsgThread, public IKafkaErrorCallBack, public IReConnOwner
{
	typedef void (CMQSThread::*MQSFunc)(SMQSMsg *pMQSMsg);
public:
	CMQSThread( CMsgQueuePrio & oPublicQueue, CMQSThreadGroup *pMQSThreadGroup );
	virtual ~CMQSThread();
	
	bool GetSelfCheck();
	
	int Start(const vector<PairServiceInfo>& vecServiceInfo);
	virtual void StartInThread();
	virtual void Stop();
	bool IsStopped() { return m_bIsSelfCheckTimerStopped; }
	
	virtual void Deal(void *pData);
	int OnAddService(const vector<PairServiceInfo>& vecServiceInfo);
	int OnReConn(void *pContext);
	int OnKafkaError(IKafka *pSrc, const std::string& msg);
	
	void Dump();
	
private:
	void DoProduce(SMQSMsg *pMQSMsg);
	void DoAddService(SMQSMsg *pMQSMsg);
	void DoStop(SMQSMsg *pMQSMsg);
	void DoReConn(SMQSMsg *pMQSMsg);
	void DoSelfCheck();

private:
	MQSFunc m_MQSFunc[MQS_ALL];
	bool			 m_isAlive;
	unsigned int	 m_dwTimeoutNum;
	timeval_a		 m_tmLastTimeoutWarn;
	CMQSThreadGroup *m_pMQSThreadGroup;
	CThreadTimer	 m_timerSelfCheck;
	bool             m_bIsSelfCheckTimerStopped;

	map<unsigned int, CKafkaProducer*>   m_mapServiceAndProducer;
	map<SKafkaBTP, CKafkaProducer*>      m_mapBTPAndProducer;
	CMQSReConnHelperImpl                 m_MQSReConnHelper;
};

#endif //_MQS_THREAD_H_
