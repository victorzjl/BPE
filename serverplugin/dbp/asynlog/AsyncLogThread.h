#ifndef _H_ASYNC_LOG_THREAD_H_
#define _H_ASYNC_LOG_THREAD_H_

#include "MsgTimerThread.h"
#include "AsyncLogMsg.h"
#include "AsyncLogHelper.h"
#include <string>
#include <map>
using std::map;

using namespace sdo::dbp;

class CAsyncLogThread:public CMsgTimerThread
{
public:
	typedef void (CAsyncLogThread::*AsyncLogFunc)(SAsyncLogMsg *);
	
	CAsyncLogThread();
	~CAsyncLogThread(){}
	
	static CAsyncLogThread* GetInstance();
	
	void OnAsynLog(SDbQueryMsg*);

	virtual void StartInThread();
	virtual void Deal(void * pData);	
	string DoSelfCheck();
	
private:
	void DoAsynLog(SAsyncLogMsg * pMsg);
	void DoDetailLog();
private:

	char m_szRequestBuf[MAX_BUFFER_SIZE];
	int m_nRequestLoc;
	CThreadTimer m_tmDetailLog;
	AsyncLogFunc m_funMap[10];

};


#endif

