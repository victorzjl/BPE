#include "DbReconnThread.h"
#include "DbThreadGroup.h"
#include "DbpService.h"
extern int	bInit;
CDbReconnThread* CDbReconnThread::m_pInstance = NULL;
CDbReconnThread* CDbReconnThread::GetInstance()
{
	//static CDbReconnThread gReconnThread;
	if(NULL == m_pInstance)
	{
		m_pInstance = new CDbReconnThread;
	}
	return m_pInstance;
}

CDbReconnThread::~CDbReconnThread()
{
	m_timer.Stop();
	m_timerPing.Stop();
	m_timerStatics.Stop();
	//Stop();
}
CDbReconnThread::CDbReconnThread(): m_timer(this,30,boost::bind(&CDbReconnThread::DoTimeOut,this),CThreadTimer::TIMER_CIRCLE),
m_timerPing(this,3600,boost::bind(&CDbReconnThread::DoTimeOutPing,this),CThreadTimer::TIMER_CIRCLE),
m_timerStatics(this,60,boost::bind(&CDbReconnThread::DoTimeStatics,this),CThreadTimer::TIMER_CIRCLE)
{

}

void CDbReconnThread::StartInThread()
{
	SV_XLOG(XLOG_DEBUG, "CDbReconnThread::%s\n", __FUNCTION__);

	m_timer.Start();
	m_timerPing.Start();
	m_timerStatics.Start();
	
}

void CDbReconnThread::OnReConnDb(CDbConnection* pDbConn)
{

	string strWarn = "DBbroker DBAddr["+pDbConn->GetAddr()+ "] DISCONNECT SrvIp["+CDbThreadGroup::GetInstance()->GetIp()+"]";
	
	SV_XLOG(XLOG_FATAL, "CDbReconnThread::%s DBbroker DBAddr[%s] SrvIp[%s] 				MysqlPingnIndex[%d],subIndex[%d],nConnectIndex[%d],Master[%d]\n",
		__FUNCTION__,  
		pDbConn->GetAddr().c_str(),
		CDbThreadGroup::GetInstance()->GetIp().c_str(),
		pDbConn->m_nIndex,
		pDbConn->m_nSubIndex,
		pDbConn->m_nConnectIndex,
		(int)pDbConn->m_isMaster
	);
		
	DbpService::GetInstance()->CallExceptionWarn(strWarn);
	
	PutQ(pDbConn);
}
void CDbReconnThread::Deal(void *pData)
{
	CDbConnection* pDbConn = (CDbConnection*)pData;
    {
        boost::unique_lock<boost::mutex> lock(mut);
    	m_listConn.push_back(pDbConn);
    }
	//DoTimeOut();
}

void CDbReconnThread::DoTimeOutPing()
{
	if (bInit!=CONFIG_OK_INIT)
		return ;
	SV_XLOG(XLOG_TRACE,"CDbReconnThread::%s\n",__FUNCTION__);  
	CDbThreadGroup::GetInstance()->DoPing();
}

void CDbReconnThread::DoTimeStatics()
{
	if (bInit!=CONFIG_OK_INIT)
		return ;
	SV_XLOG(XLOG_TRACE,"CDbReconnThread::%s\n",__FUNCTION__); 
	CDbThreadGroup::GetInstance()->QueueStatics();
}

void CDbReconnThread::DoTimeOut()
{
	if (bInit!=CONFIG_OK_INIT)
		return ;
    SV_XLOG(XLOG_TRACE,"CDbReconnThread::%s\n",__FUNCTION__);   
	list<CDbConnection*>::iterator itr;
    boost::unique_lock<boost::mutex> lock(mut);
	for(itr = m_listConn.begin(); itr != m_listConn.end(); )
	{
		CDbConnection *pDbConn = (CDbConnection*)(*itr);
		pDbConn->DoReConnectDb();
		if(pDbConn->m_bIsConnected)
		{
			itr = m_listConn.erase(itr);
			CDbThreadGroup::GetInstance()->OnConnectInfo(pDbConn, pDbConn->m_nIndex);
		}
		else
		{
			itr ++;
		}
	}
}




