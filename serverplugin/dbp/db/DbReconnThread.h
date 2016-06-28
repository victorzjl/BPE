#ifndef _DB_RECONN_THREAD_H_
#define _DB_RECOON_THREAD_H_
#include "MsgTimerThread.h"

#include "DbMsg.h"
#include "DbConnection.h"
using namespace sdo::dbp;
class CDbReconnThread: public CMsgTimerThread
{
public:
	
	static CDbReconnThread * GetInstance();
	void OnReConnDb(CDbConnection* pDbConn);
	virtual void Deal(void *pData);
	virtual void StartInThread();
private:
	void DoTimeOut();
	void DoTimeOutPing();
	void DoTimeStatics();
	CDbReconnThread();
	~CDbReconnThread();
private:
	list<CDbConnection*> m_listConn;
	static CDbReconnThread* m_pInstance;
	CThreadTimer m_timer;
	CThreadTimer m_timerPing;
	CThreadTimer m_timerStatics;
	boost::mutex mut;
};
#endif



