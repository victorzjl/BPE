#ifndef _MQR_RECONN_THREAD_H_
#define _MQR_RECONN_THREAD_H_

#include "MsgTimerThread.h"
#include "IReConnHelp.h"

#include <vector>

using namespace sdo::common;

/////////////////////////////////////////////
/////// Message Definition
/////////////////////////////////////////////
typedef enum enReConnOperator
{
	RECONN_SERVICE = 1,
	RECONN_STOP = 2,
	RECONN_ALL
}EReConnOperator;

typedef struct stReConnMsg
{
	EReConnOperator m_enReConnOperator;
	stReConnMsg(EReConnOperator enReConnOperator):m_enReConnOperator(enReConnOperator){}
	virtual ~stReConnMsg(){}
}SReConnMsg;

typedef struct stReConnServiceMsg : public stReConnMsg
{
	IReConnOwner *pOwner;
	IReConnHelper *pHelper;
	void *pContext;
	
	timeval_a tmLastWarn;
	
	stReConnServiceMsg():stReConnMsg(RECONN_SERVICE){}
	virtual ~stReConnServiceMsg(){}
}SReConnServiceMsg;

typedef struct stReConnStopMsg : public stReConnMsg
{
	stReConnStopMsg():stReConnMsg(RECONN_STOP){}
	virtual ~stReConnStopMsg(){}
}SReConnStopMsg;



/////////////////////////////////////////////
/////// ReConn Thread Class Declaration
/////////////////////////////////////////////
class CReConnThread : public CMsgTimerThread
{
public:
	static CReConnThread *GetReConnThread();
	
	virtual ~CReConnThread();
	
	int Initialize(IReConnWarnRecevier *pRecevier, unsigned int alarmFrequency);
	int Start();
	void StartInThread();
	int Stop();
	
	void OnReConn(IReConnOwner *pOwner, IReConnHelper *pHelper, void *context);
	virtual void Deal(void *pData);
	
	void Dump();

private:
	CReConnThread();
	
private:
	void DoReConn();
	void DoStop();
	void Warn(const std::string& warnning);
	
private:
	static CReConnThread                 *s_pReConnThread;
	std::vector<SReConnServiceMsg *>      m_vecReConnServiceMsg;
	CThreadTimer                          m_timerReConn;
	IReConnWarnRecevier                  *m_pRecevier;
	unsigned int                          m_alarmFrequency;
	bool                                  m_bIsReConnTimerStopped;
};

#endif //_MQR_RECONN_THREAD_H_
