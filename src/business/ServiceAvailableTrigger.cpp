#include "ServiceAvailableTrigger.h"
#include "SessionManager.h"
#include "SapLogHelper.h"

CAvailableTrigger::CAvailableTrigger(CAvenueServiceConfigs *pConfig, CSessionManager *pManager)
    :m_pConfig(pConfig),m_pManager(pManager)
{}

bool CAvailableTrigger::BeginTrigger(int nServId, int nMsgId)
{ 
    const STriggerItem *pItem = CTriggerConfig::Instance()->GetInfo(AVAIABLE_TRIGGERTYPE,nServId, nMsgId);
    if (pItem == NULL || pItem->vecReq.empty())
    {
        return true;
    }

    unsigned int dwSubServId = 0; 
    for (vecTriggerItem::const_iterator itr = pItem->vecReq.begin();
        itr != pItem->vecReq.end();
        ++itr)
    {                
        if (0 != m_pConfig->GetServiceIdByName(itr->strName, &dwSubServId))
        {            
            SS_XLOG(XLOG_WARNING,"CAvailableTrigger::%s get service[%s] id failed\n",
                __FUNCTION__,itr->strName.c_str());
            continue;
        }        
        
        SS_XLOG(XLOG_DEBUG,"CAvailableTrigger::%s serviceid[%d]\n",__FUNCTION__,dwSubServId);
		if (m_pManager->OnGetSos(dwSubServId) == NULL)
		{            
            SS_XLOG(XLOG_WARNING,"CAvailableTrigger::%s service[%d] not avaiable\n",
                __FUNCTION__,dwSubServId);
            return false;
		}               
    }     
    return true;
}

