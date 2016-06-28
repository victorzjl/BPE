#ifndef _SMART_THREAD_H_
#define _SMART_THREAD_H_
#include "MsgTimerThread.h"
#include "DbAngent.h"
#include "DbMsg.h"
#include "DbConnection.h"
using namespace sdo::dbp;
class CSmartThread: public CMsgTimerThread
{
public:
	
	static CSmartThread * GetInstance();
	virtual void Deal(void *pData);
	virtual void StartInThread();
	void PutSmart(CDbAgent2* pAngent2);
private:
	void DoTimeOut();
	CSmartThread();
	~CSmartThread();
private:
	static CSmartThread* m_pInstance;
	CThreadTimer m_timer;
	vector<CDbAgent2*> m_vecAngent2;
};
#endif



