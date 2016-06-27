#ifndef __SERVICE_AVAILABLE_TRIGGER_H__
#define __SERVICE_AVAILABLE_TRIGGER_H__
#include <string>
#include "AvenueServiceConfigs.h"


using sdo::commonsdk::CAvenueServiceConfigs;

class CSessionManager;
class CAvailableTrigger
{
public:
	CAvailableTrigger(CAvenueServiceConfigs *pConfig,CSessionManager *pManager);
	~CAvailableTrigger(){}
	bool BeginTrigger(int nServId, int nMsgId);	
private:	
	CAvenueServiceConfigs* m_pConfig;
	CSessionManager* m_pManager;
	static const int AVAIABLE_TRIGGERTYPE = 2;
};

#endif

