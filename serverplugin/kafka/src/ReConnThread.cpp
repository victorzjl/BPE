#include "ReConnThread.h"
#include "Common.h"

CReConnThread *CReConnThread::s_pReConnThread = NULL;

CReConnThread *CReConnThread::GetReConnThread()
{
	if (NULL == s_pReConnThread)
	{
		s_pReConnThread = new CReConnThread();
	}
	
	return s_pReConnThread;
}

CReConnThread::CReConnThread()
	: m_timerReConn(this, 3, boost::bind(&CReConnThread::DoReConn,this),CThreadTimer::TIMER_CIRCLE),
	  m_pRecevier(NULL),
	  m_alarmFrequency(3),
	  m_bIsReConnTimerStopped(true)
{
	
}

CReConnThread::~CReConnThread()
{
	Stop();
}

int CReConnThread::Initialize(IReConnWarnRecevier *pRecevier, unsigned int alarmFrequency)
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s. pRecevier[%p], AlarmFrequency[%u]\n", __FUNCTION__, pRecevier, alarmFrequency);
	m_pRecevier = pRecevier;
	m_alarmFrequency = alarmFrequency;
	return 0;
}

int CReConnThread::Start()
{
	CMsgTimerThread::Start();
	return 0;
}

void CReConnThread::StartInThread()
{
	m_timerReConn.Start();
	m_bIsReConnTimerStopped = false;
}

int CReConnThread::Stop()
{
	SReConnStopMsg *pReConnStopMsg = new SReConnStopMsg;
	PutQ(pReConnStopMsg);
	
	do 
	{
		sleep(1);
	}while(!m_bIsReConnTimerStopped);
	
	CMsgTimerThread::Stop();
	
	std::vector<SReConnServiceMsg *>::iterator iter = m_vecReConnServiceMsg.begin();
	while (iter != m_vecReConnServiceMsg.end())
	{
		delete *iter;
		*iter = NULL;
		++iter;
	}
	m_vecReConnServiceMsg.clear();
	return 0;
}

void CReConnThread::OnReConn(IReConnOwner *pOwner, IReConnHelper *pHelper, void *context)
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s. pOwner[%p], pHelper[%p], context[%p]\n", __FUNCTION__, pOwner, pHelper, context);
	SReConnServiceMsg *pReConnServiceMsg = new SReConnServiceMsg;
	pReConnServiceMsg->pOwner = pOwner;
	pReConnServiceMsg->pHelper = pHelper;
	pReConnServiceMsg->pContext = context;
	PutQ(pReConnServiceMsg);
}

void CReConnThread::Deal(void *pData)
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s.\n", __FUNCTION__);
	SReConnMsg *pReConnMsg = (SReConnMsg *)pData;
	if (RECONN_SERVICE == pReConnMsg->m_enReConnOperator)
	{
		SReConnServiceMsg *pReConnServiceMsg = (SReConnServiceMsg *)pReConnMsg;

		std::string outMsg;
		if (0 == pReConnServiceMsg->pHelper->TryReConnect(pReConnServiceMsg->pContext, outMsg))
		{
			pReConnServiceMsg->pOwner->OnReConn(pReConnServiceMsg->pContext);
			delete pReConnMsg;
			pReConnServiceMsg = NULL;
		}
		else
		{
			gettimeofday_a(&(pReConnServiceMsg->tmLastWarn), 0);
			m_vecReConnServiceMsg.push_back(pReConnServiceMsg);
		}
	}
	else if (RECONN_STOP == pReConnMsg->m_enReConnOperator)
	{
		DoStop();
		delete pReConnMsg;
	}
	pReConnMsg = NULL;
}

void CReConnThread::DoReConn()
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s.\n", __FUNCTION__);
	std::vector<SReConnServiceMsg *>::iterator iter = m_vecReConnServiceMsg.begin();
	while (iter != m_vecReConnServiceMsg.end())
	{
		SReConnServiceMsg *pReConnServiceMsg = *iter;
		std::string outMsg;
		if(0 == pReConnServiceMsg->pHelper->TryReConnect(pReConnServiceMsg->pContext, outMsg))
		{
			if (0 == pReConnServiceMsg->pOwner->OnReConn(pReConnServiceMsg->pContext))
			{
				iter = m_vecReConnServiceMsg.erase(iter);
				delete pReConnServiceMsg;
				pReConnServiceMsg = NULL;
				continue;
			}
		}
		
		//Failed to reconnect
		if(GetTimeInterval(pReConnServiceMsg->tmLastWarn)/1000 > m_alarmFrequency)
		{
			Warn(outMsg);
			gettimeofday_a(&(pReConnServiceMsg->tmLastWarn), 0);
		}
		iter++;
	}
}

void CReConnThread::DoStop()
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s.\n", __FUNCTION__);
	m_timerReConn.Stop();
	m_bIsReConnTimerStopped = true;
}

void CReConnThread::Warn(const std::string& warnning)
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s. Warning[%s], Recevier[%p]\n", __FUNCTION__, warnning.c_str(), m_pRecevier);
	if (!m_pRecevier)
	{
		m_pRecevier->ReConnWarn(warnning);
	}
}

void CReConnThread::Dump()
{
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s\n", __FUNCTION__);
	RECONN_XLOG(XLOG_DEBUG, "CReConnThread::%s < AlarmFrequency[%u] >\n", __FUNCTION__, m_alarmFrequency);
}

