#ifndef _SAP_REQUEST_HISTORY_H_
#define _SAP_REQUEST_HISTORY_H_
#include "SmallObject.h"
#include "SapCommon.h"
#include "SapMessage.h"
#include <map>
#include "detail/_time.h"
#include <boost/thread/mutex.hpp>

using std::map;
using std::multimap;
namespace sdo{
namespace sap{
typedef struct tagSapRequestHistory
{
	struct timeval_a m_stEndtime;

	int nId;
	unsigned int dwSequence;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	struct timeval_a starttime;
	bool Equal(const struct tagSapRequestHistory &obj)
	{
		return (nId==obj.nId?
			(dwSequence==obj.dwSequence?
			(dwServiceId==obj.dwServiceId?
			(dwMsgId==obj.dwMsgId?
			(starttime==obj.starttime?1:0):0):0):0):0);
	}
}SSapRequestHistory;
class CSapMsgTimerList
{
	typedef map<unsigned int, SSapRequestHistory > MHistory;
	typedef multimap<struct timeval_a,SSapRequestHistory> MMHistoryTimers;
public:
	CSapMsgTimerList();
	~CSapMsgTimerList();
	static void SetRequestTimeout(unsigned int timeout);

	
	void AddHistory(int nId,unsigned int dwSequence,unsigned int dwServiceId,unsigned int dwMsgId,unsigned int timeout);
	bool RemoveHistoryBySequence(unsigned int dwSequence,unsigned int dwCode);
	void DetectTimerList();

	bool IsExistSequence(unsigned int dwSequence);

private:
	static unsigned int sm_nRequestTimeout;
	MHistory m_history;
	MMHistoryTimers m_timers;

	SSapMsgHeader m_stTimeoutResponse;
};
}
}
#endif

