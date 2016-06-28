#include "ThreadTimer.h"

namespace sdo{
	namespace dbp{
		CThreadTimer::CThreadTimer(CTimerManager * pOwner,unsigned int dwInterval,void (*pfn)(void *),void *pData,ETimerType enType,ETimerUnit enUnit):
			m_pOwner(pOwner),m_dwInterval(dwInterval),m_enState(TIMER_IDLE),m_enType(enType),m_pfn(pfn),m_pData(pData)
        {
            if(enUnit==TIMER_UNIT_SECOND)
    		{
    			m_dwInterval=dwInterval*1000;
    		}
    		else
    		{
    			m_dwInterval=dwInterval;
    		}
            m_f=boost::bind(pfn,pData);
        }
		void CThreadTimer::Start()
		{
		    if(m_enState==TIMER_IDLE)
            {      
			    m_pOwner->AddTimer(this);
                m_enState=TIMER_ALIVE;
            }
		}
		void CThreadTimer::Stop()
		{
			m_pOwner->RemoveTimer(this);
            m_enState=TIMER_IDLE;
		}
		void CThreadTimer::Restart(unsigned int dwInterval,ETimerUnit enUnit)
		{
			m_pOwner->RemoveTimer(this);
			if(dwInterval!=0)
			{
				if(enUnit==TIMER_UNIT_SECOND)
        		{
        			m_dwInterval=dwInterval*1000;
        		}
        		else
        		{
        			m_dwInterval=dwInterval;
        		}
			}
			m_pOwner->AddTimer(this);
            m_enState=TIMER_ALIVE;
		}
		CThreadTimer::~CThreadTimer()
		{
			if(m_enState==TIMER_ALIVE)
				Stop();
		}
	}
}

