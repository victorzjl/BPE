#include "CacheReConnThread.h"
#include "CacheThread.h"
#include "AsyncVirtualServiceLog.h"
#include "CacheVirtualService.h"
#include "Common.h"
#include "CommonMacro.h"
#include "WarnCondition.h"

CCacheReConnThread::CCacheReConnThread():
	m_timerReConn(this, 3, boost::bind(&CCacheReConnThread::DoReConn,this),CThreadTimer::TIMER_CIRCLE),
	m_pCacheVirtualService(NULL)
{
}

void CCacheReConnThread::StartInThread()
{
	m_timerReConn.Start();
}




void CCacheReConnThread::OnReConn( vector<unsigned int> dwVecServiceId, const string &strAddr, CCacheThread *pCacheThread)
{
	string szServiceId = GetServiceIdString(dwVecServiceId);
	CVS_XLOG(XLOG_ERROR, "CCacheReConnThread::%s, ServiceId[%s], Addr[%s]\n", __FUNCTION__, szServiceId.c_str(), strAddr.c_str());

	SCacheReConnMsg *pCacheReConnMsg = new SCacheReConnMsg;
	pCacheReConnMsg->dwVecServiceId = dwVecServiceId;
	pCacheReConnMsg->strAddr = strAddr;
	pCacheReConnMsg->pCacheThread = pCacheThread;
	PutQ(pCacheReConnMsg);
}


void CCacheReConnThread::Deal( void *pData )
{
	SCacheReConnMsg *pCacheReConnMsg = (SCacheReConnMsg *)pData;
	Warn(pCacheReConnMsg->strAddr);
	gettimeofday_a(&(pCacheReConnMsg->tmLastWarn), 0);

	pCacheReConnMsg->pCacheAgent = new CCacheAgent;
	if(pCacheReConnMsg->pCacheAgent->Initialize(pCacheReConnMsg->strAddr) == 0)
	{
		pCacheReConnMsg->bCacheAgentInitSucc = true;
	}

	m_vecReConnMsg.push_back(pCacheReConnMsg);
}

void CCacheReConnThread::DoReConn()
{
	vector<SCacheReConnMsg *>::iterator iter = m_vecReConnMsg.begin();
	while (iter != m_vecReConnMsg.end())
	{
		SCacheReConnMsg *pCacheReConnMsg = *iter;
		CVS_XLOG(XLOG_DEBUG, "CCacheReConnThread::%s, Addr[%s]\n", __FUNCTION__, pCacheReConnMsg->strAddr.c_str());

		if((pCacheReConnMsg->bCacheAgentInitSucc || pCacheReConnMsg->pCacheAgent->Initialize(pCacheReConnMsg->strAddr) == 0)
			&& pCacheReConnMsg->pCacheAgent->Set("Test", "Test") == 0)
		{
			pCacheReConnMsg->pCacheThread->OnAddCacheServer(pCacheReConnMsg->dwVecServiceId, pCacheReConnMsg->strAddr);
			iter = m_vecReConnMsg.erase(iter);
			
			delete pCacheReConnMsg->pCacheAgent;
			pCacheReConnMsg->pCacheAgent = NULL;
			
			delete pCacheReConnMsg;
		}
		else
		{
			CVS_XLOG(XLOG_ERROR, "CCacheReConnThread::%s, ReConnect CacheServer[%s] Error\n", __FUNCTION__, pCacheReConnMsg->strAddr.c_str());
			if(GetTimeInterval(pCacheReConnMsg->tmLastWarn)/1000 > m_dwAlarmFrequency)
			{
				Warn(pCacheReConnMsg->strAddr);
				gettimeofday_a(&(pCacheReConnMsg->tmLastWarn), 0);
			}
			iter++;
		}
	}
}

void CCacheReConnThread::Warn(const string &strAddr)
{
	char szWarnInfo[256] = {0};
	snprintf(szWarnInfo, sizeof(szWarnInfo)-1, "cache connect failed, ip address : %s", strAddr.c_str());
	m_pCacheVirtualService->Warn(szWarnInfo);
}

int CCacheReConnThread::Start( CCacheVirtualService *pCacheVirtualService )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheReConnThread::%s\n", __FUNCTION__);
	m_pCacheVirtualService = pCacheVirtualService;

	SDependencyWarnCondition objDependencyWarnCondition = pCacheVirtualService->GetWarnCondition()->GetDependencyWarnCondition();
	m_dwAlarmFrequency = objDependencyWarnCondition.dwAlarmFrequency;

	CMsgTimerThread::Start();
	return 0;
}
