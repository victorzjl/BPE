#ifndef _THRAD_TIMER_H_
#define _THRAD_TIMER_H_
#include "detail/_time.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "TimerManager.h"

namespace sdo{
namespace dbp{
class CTimerManager;
class CThreadTimer
{
public:
	friend class CTimerManager;
	typedef enum
	{
		TIMER_IDLE=0,
		TIMER_ALIVE
	}ETimerState;

	typedef enum
	{
		TIMER_UNIT_SECOND=0,
		TIMER_UNIT_MILLI_SECOND
	}ETimerUnit;
	
	typedef enum
	{
		TIMER_ONCE=0,
		TIMER_CIRCLE
	}ETimerType;
	CThreadTimer(){}

	CThreadTimer(CTimerManager * pOwner,unsigned int dwInterval=0,void (*pfn)(void *)=0,void *pData=0,
		ETimerType enType=TIMER_ONCE,ETimerUnit enUnit=TIMER_UNIT_SECOND);
	void Init(unsigned int dwInterval=0,void (*pfn)(void *)=0,void *pData=0,
		ETimerType enType=TIMER_ONCE,ETimerUnit enUnit=TIMER_UNIT_SECOND)
	{
		if(enUnit==TIMER_UNIT_SECOND)
		{
			m_dwInterval=dwInterval*1000;
		}
		else
		{
			m_dwInterval=dwInterval;
		}
		
		m_pfn=pfn;
		m_pData=pData;
		m_f=boost::bind(pfn,pData);
		m_enType=enType;
	}
	template <typename Func>
	CThreadTimer(CTimerManager * pOwner,unsigned int dwInterval,Func func,ETimerType enType,ETimerUnit enUnit=TIMER_UNIT_SECOND):
		m_pOwner(pOwner),m_enState(TIMER_IDLE),m_enType(enType),m_f(func)
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
	template <typename Func>
	void Init(CTimerManager * pOwner,unsigned int dwInterval,Func func,ETimerType enType,ETimerUnit enUnit=TIMER_UNIT_SECOND)
	{
		if(enUnit==TIMER_UNIT_SECOND)
		{
			m_dwInterval=dwInterval*1000;
		}
		else
		{
			m_dwInterval=dwInterval;
		}
		m_pOwner=pOwner;
		m_f=func;
		m_enType=enType;
	}
	
	void Start();
	void Stop();
	void OnCallBack(){m_f();}
	void Restart(unsigned int dwInterval=0,ETimerUnit enUnit=TIMER_UNIT_SECOND);
	void SetInterval(unsigned int dwInterval,ETimerUnit enUnit=TIMER_UNIT_SECOND)
	{
		m_dwInterval=dwInterval;
		if(enUnit==TIMER_UNIT_SECOND)
		{
			m_dwInterval=dwInterval*1000;
		}
		else
		{
			m_dwInterval=dwInterval;
		}
	}
	ETimerState State()const{return m_enState;}
	~CThreadTimer();
private:
	CTimerManager * m_pOwner;
	unsigned int m_dwInterval;//unit : ms
	struct timeval_a m_stEndtime;
	ETimerState m_enState;
	ETimerType m_enType;
	void (*m_pfn)(void *);
	void * m_pData;
	boost::function<void()> m_f;
};
}
}
#endif
