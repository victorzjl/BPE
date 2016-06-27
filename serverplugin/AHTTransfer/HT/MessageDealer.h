#ifndef _MESSAGE_DEALER_H_
#define _MESSAGE_DEALER_H_

#include <MsgTimerThread.h>
#include "HTMsg.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include "AHTTransfer.h"
#include "Common.h"
#include "MessageReceive.h"
using namespace sdo::common;
using std::vector;
using namespace HT;

class CMessageDealer: public CMsgTimerThread
{
	typedef void (CMessageDealer::*HTFunc)(SHTTypeMsg *pSHTTypeMsg);
public:
	CMessageDealer();
    virtual ~CMessageDealer();
	virtual void StartInThread();
	virtual void Deal(void *pData);
	int Start();
	//int Stop(){CMsgTimerThread::Stop();}
	bool GetSelfCheck();
	
	void OnSend(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, timeval_a tStart,
				void *pHandler, const void *pBuffer, int nLen, CHTDealer *htd);
	void OnReceive(int nCohClientId, const string &strResponse, int nProtocol, const string  & ip, const string & port );	

private:
	void DoSend(SHTTypeMsg *pSHTTypeMsg);
	void DoReceive(SHTTypeMsg *pSHTTypeMsg);
private:	
	void DoSelfCheck();
	void SetIPPORT( const string  ip, const string port, int m_BId);
	
private:
	HTFunc m_HTFunc[E_MSG_MAX];
	int m_nBusinessClientId;
	
	unsigned int	m_dwTimeoutNum;
	bool			m_isAlive;
	unsigned int	m_dwTimeoutInterval;
	unsigned int	m_dwConditionTimes;
	unsigned int	m_dwTimeoutAlarmFrequency;
	
	timeval_a		m_tmLastTimeoutWarn;
	
	map<int, SSessionInfo> m_mapReqs;
	CThreadTimer	m_timerSelfCheck;
	CAHTTransfer *m_aht;
	
};
#endif
