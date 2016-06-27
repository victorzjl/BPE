#ifndef __JOB_TIMER_THREAD_H__
#define __JOB_TIMER_THREAD_H__
#include <map>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <time.h>
#include "ITimerService.h"

class CJobTimerThread:private boost::noncopyable
{
	struct SCallbackMsg
	{
		pFnCallback pFn;
		unsigned interval;	
		SCallbackMsg(pFnCallback pFn_, unsigned interval_):pFn(pFn_),interval(interval_){}
	};
typedef boost::shared_ptr<SCallbackMsg> SCbMsg_ptr;
typedef std::multimap<time_t, SCbMsg_ptr> CallbackCntr;
typedef CallbackCntr::iterator cbIterator;
public:
	static CJobTimerThread* GetInstance();
public:
	void Start();
	void RegisterCallback(pFnCallback fn, unsigned seconds);
private:
	CJobTimerThread(){}
	void Run();
private:
	static CJobTimerThread* pInstance;
	boost::mutex m_mut;
	boost::thread m_thread;	
	CallbackCntr cbCntr;
	CallbackCntr cbAddCntr;
};

#endif

