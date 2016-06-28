#ifndef _SDO_COMMON_TIMER_MANAGER_H_
#define _SDO_COMMON_TIMER_MANAGER_H_
#include "ThreadTimer.h"
#include "detail/_time.h"
#include <boost/thread/tss.hpp>
#include <map>

using std::multimap;
using std::map;

namespace sdo{
namespace dbp{
class CThreadTimer;
class CTimerManager
{
typedef multimap<struct timeval_a,CThreadTimer *> MMTimer;
public:
	void Reset();
	void DetectTimerList();
	void AddTimer(CThreadTimer *pTimer);
	void RemoveTimer(CThreadTimer *pTimer);
	void Dump();
private:
	boost::thread_specific_ptr<MMTimer> m_times;
};
}
}
#endif
