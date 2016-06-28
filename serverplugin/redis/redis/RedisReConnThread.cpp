#include "RedisReConnThread.h"

#include "RedisReConnThread.h"
#include "RedisThread.h"
#include "RedisVirtualServiceLog.h"
#include "RedisVirtualService.h"
#include "Common.h"
#include "CommonMacro.h"
#include "WarnCondition.h"
#include "RedisAgent.h"

CRedisReConnThread::CRedisReConnThread():
	m_timerReConn(this, 3, boost::bind(&CRedisReConnThread::DoReConn,this),CThreadTimer::TIMER_CIRCLE),
	m_pRedisVirtualService(NULL)
{
}

void CRedisReConnThread::StartInThread()
{
	m_timerReConn.Start();
}


void CRedisReConnThread::OnReConn(const string &strAddr)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisReConnThread::%s, Addr[%s]\n", __FUNCTION__, strAddr.c_str());

	SRedisReConnMsg *pRedisReConnMsg = new SRedisReConnMsg;
	pRedisReConnMsg->strAddr = strAddr;
	PutQ(pRedisReConnMsg);
}

void CRedisReConnThread::Deal( void *pData )
{
	SRedisReConnMsg *pRedisReConnMsg = (SRedisReConnMsg *)pData;
	set<string>::const_iterator iter = m_setAddrFilter.find(pRedisReConnMsg->strAddr);
	if(iter != m_setAddrFilter.end())
	{
		delete pRedisReConnMsg;
		return;
	}
	
	Warn(pRedisReConnMsg->strAddr);
	gettimeofday_a(&(pRedisReConnMsg->tmLastWarn), 0);

	m_vecReConnMsg.push_back(pRedisReConnMsg);
	m_setAddrFilter.insert(pRedisReConnMsg->strAddr);
}

void CRedisReConnThread::DoReConn()
{
	vector<SRedisReConnMsg *>::iterator iter = m_vecReConnMsg.begin();
	while (iter != m_vecReConnMsg.end())
	{
		SRedisReConnMsg *pRedisReConnMsg = *iter;
		RVS_XLOG(XLOG_DEBUG, "CRedisReConnThread::%s, Addr[%s]\n", __FUNCTION__, pRedisReConnMsg->strAddr.c_str());

        CRedisAgent *pRedisAgent = new CRedisAgent;
		if(pRedisAgent->Initialize(pRedisReConnMsg->strAddr) == 0 && pRedisAgent->Ping()== 0)
		{
		    RVS_XLOG(XLOG_WARNING, "CRedisReConnThread::%s, ReConnect RedisServer[%s] Sucess\n", __FUNCTION__, pRedisReConnMsg->strAddr.c_str());
			m_pRedisVirtualService->AddRedisServer(pRedisReConnMsg->strAddr);

			iter = m_vecReConnMsg.erase(iter);
			delete pRedisReConnMsg;

			set<string>::const_iterator setIter = m_setAddrFilter.find(pRedisReConnMsg->strAddr);
	        if(setIter != m_setAddrFilter.end())
	        {
		        m_setAddrFilter.erase(setIter);
	        }
		}
		else
		{
			RVS_XLOG(XLOG_ERROR, "CRedisReConnThread::%s, ReConnect RedisServer[%s] Error\n", __FUNCTION__, pRedisReConnMsg->strAddr.c_str());
			if(GetTimeInterval(pRedisReConnMsg->tmLastWarn)/1000 > m_dwAlarmFrequency)
			{
				Warn(pRedisReConnMsg->strAddr);
				gettimeofday_a(&(pRedisReConnMsg->tmLastWarn), 0);
			}
			iter++;
		}
		
		delete pRedisAgent;
	}
}

void CRedisReConnThread::Warn(const string &strAddr)
{
	char szWarnInfo[256] = {0};
	snprintf(szWarnInfo, sizeof(szWarnInfo), "Redis connect failed, ip address : %s", strAddr.c_str());
	m_pRedisVirtualService->Warn(szWarnInfo);
}

int CRedisReConnThread::Start( CRedisVirtualService *pRedisVirtualService )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisReConnThread::%s\n", __FUNCTION__);
	m_pRedisVirtualService = pRedisVirtualService;

	SDependencyWarnCondition objDependencyWarnCondition = pRedisVirtualService->GetWarnCondition()->GetDependencyWarnCondition();
	m_dwAlarmFrequency = objDependencyWarnCondition.dwAlarmFrequency;

	CMsgTimerThread::Start();
	return 0;
}

