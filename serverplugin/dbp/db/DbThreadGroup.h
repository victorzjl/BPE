#ifndef _DB_THREAD_GROUP_H_
#define _DB_THREAD_GROUP_H_
#include "MsgThread.h"
#include "DbMsg.h"
#include "DbAngent.h"
#include "MsgQueuePrio.h"
#include "ChildMsgThread.h"
#include "pthread.h"

class CDbThreadGroup;
using namespace sdo::dbp;

class CDbWorkThread: public CChildMsgThread
{
	
public: 
		CDbWorkThread(CMsgQueuePrio & oPublicQueue,CDbThreadGroup*pGroup);
		virtual void Deal(void *pData);
		bool IsActive(){ bool bIsAlive = m_isAlive; m_isAlive=false; return bIsAlive;}
		void DoSelfCheck();
	 private:
	 	CDbThreadGroup* m_pGroup;
		pthread_t  m_tid;
		virtual void StartInThread();
		CThreadTimer	m_timerSelfCheck;
		volatile bool m_isAlive;
};

	

class CDbThreadGroup  //: public CMsgThread
{
	typedef void (CDbThreadGroup::*DbFunc)(SDbMsg *);

	friend class CDbWorkThread;
	

public:
	void Start(int nThread);
	void Stop();
	void QueueStatics();
public:
	static CDbThreadGroup * GetInstance();
	virtual void Deal(void *pData);
	void Dump();
	void DoPing();
public:
	virtual void OnLoadConfig();
	virtual void OnRequest(void* sParams,  const void *pBuffer,  int len);	
	virtual void OnConnectInfo(void *pDbConn, int nDbIndex);
	void DoResponse(SDbQueryMsg* pQueryMsg);
	string GetIp();
	void Clear();	
	void GetThreadState(unsigned int& nActive,unsigned  int& nSum);
private:
	void DoLoadConfig(SDbMsg * pMsg);
	void DoRequest(SDbMsg * pMsg);
	void DoConnectInfo(SDbMsg * pMsg);

	int  GetDbIndex(int dwappId);

	void RecordOpr(SDbQueryMsg* pQueryMsg);
private:
	CDbThreadGroup();
	virtual ~CDbThreadGroup();
	void RefineErrorMsg(SDbQueryMsg* pQueryMsg);
	static CDbThreadGroup * sm_instance;

	DbFunc m_funcMap[DB_ALL_MSG];

	map<int, CDbAgent2 * > m_dbs;
	map<int, int> m_serviceindexs;
	boost::mutex m_mutex;

	CMsgQueuePrio m_queue;
	vector<CDbWorkThread*>m_vecChildThread;
	string m_srvIp;

};
#endif


