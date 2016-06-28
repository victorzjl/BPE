#include "SmartThread.h"
#include "DbThreadGroup.h"
#include "DbpService.h"

CSmartThread* CSmartThread::m_pInstance = NULL;
CSmartThread* CSmartThread::GetInstance()
{
	//static CSmartThread gReconnThread;
	if(NULL == m_pInstance)
	{
		m_pInstance = new CSmartThread;
	}
	return m_pInstance;
}

CSmartThread::~CSmartThread()
{
	m_timer.Stop();
	//Stop();
}

CSmartThread::CSmartThread():m_timer(this,30,boost::bind(&CSmartThread::DoTimeOut,this),CThreadTimer::TIMER_CIRCLE)
{

}

void CSmartThread::StartInThread()
{
	SV_XLOG(XLOG_DEBUG, "CSmartThread::%s\n", __FUNCTION__);
	m_timer.Start();
}

void CSmartThread::Deal(void *pData)
{

}

void CSmartThread::PutSmart(CDbAgent2* pAngent2)
{
	m_vecAngent2.push_back(pAngent2);
}

void CSmartThread::DoTimeOut()
{
    SV_XLOG(XLOG_TRACE,"CSmartThread::%s\n",__FUNCTION__);   
	vector<CDbAgent2*>::iterator itr;
	for(itr = m_vecAngent2.begin(); itr != m_vecAngent2.end(); )
	{
		CDbAgent2 *pAngent2 = (CDbAgent2*)(*itr);
		pAngent2->HandleSmartCommit();
		itr ++;

	}
}




