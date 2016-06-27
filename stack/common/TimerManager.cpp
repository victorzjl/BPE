#include "TimerManager.h"
#ifndef WIN32
#include <unistd.h>
#endif
namespace sdo{
namespace common{
void CTimerManager::Reset()
{
    m_times.reset(new MMTimer);
}
void CTimerManager::DetectTimerList()
{
	struct timeval_a now;
    gettimeofday_a(&now,0);
    MMTimer * pMapThisThread=m_times.get();
    while(!pMapThisThread->empty())
    {
    	struct timeval_a curTimerEnd=pMapThisThread->begin()->first;
    	if(curTimerEnd<now)
    	{
    		CThreadTimer *pTimer=pMapThisThread->begin()->second;
    		pMapThisThread->erase(pMapThisThread->begin());
    		if(pTimer->m_enType==CThreadTimer::TIMER_CIRCLE)
    		{
    			struct timeval_a interval;
    			interval.tv_sec=pTimer->m_dwInterval/1000;
    			interval.tv_usec=(pTimer->m_dwInterval%1000)*1000;
    			timeradd(&now,&interval,&pTimer->m_stEndtime);
    			pMapThisThread->insert(MMTimer::value_type(pTimer->m_stEndtime,pTimer));
    		}
    		else
    		{
    			pTimer->m_enState=CThreadTimer::TIMER_IDLE;
    		}
    		pTimer->OnCallBack();
    	}
    	else
    	{
    		break;
    	}
    }
}
void CTimerManager::AddTimer(CThreadTimer *pTimer)
{
    MMTimer * pMapThisThread=m_times.get();

	struct timeval_a now;
	gettimeofday_a(&now,0);
	struct timeval_a interval;
	interval.tv_sec=pTimer->m_dwInterval/1000;
    interval.tv_usec=(pTimer->m_dwInterval%1000)*1000;
	timeradd(&now,&interval,&pTimer->m_stEndtime);
	pMapThisThread->insert(MMTimer::value_type(pTimer->m_stEndtime,pTimer));
}
void CTimerManager::RemoveTimer(CThreadTimer *pTimer)
{
    MMTimer * pMapThisThread=m_times.get();

	std::pair<MMTimer::iterator, MMTimer::iterator> itr_pair = pMapThisThread->equal_range(pTimer->m_stEndtime);
	MMTimer::iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
		CThreadTimer *pQueryTimer=itr->second;
		if(pQueryTimer==pTimer)
		{
			pMapThisThread->erase(itr);
			return;
		}
	}
}
void CTimerManager::Dump()
{
    MMTimer * pMapThisThread=m_times.get();
    multimap<struct timeval_a,CThreadTimer *>::iterator itr;
    struct timeval_a now;
	gettimeofday_a(&now,0);
    printf("=====TimerManager[%d], now[%ld.%ld]===\n",pMapThisThread->size(),now.tv_sec,now.tv_usec);
    for(itr=pMapThisThread->begin();itr!=pMapThisThread->end();++itr)
    {
        const struct timeval_a &end=itr->first;
        CThreadTimer *pTimer=itr->second;
        printf("timer end[%ld.%ld], state[%d]\n",end.tv_sec,end.tv_usec,pTimer->m_enType);
    }
}

}
}

