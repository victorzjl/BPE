#ifndef _VIRTUAL_CLIENT_SESSION_H_
#define _VIRTUAL_CLIENT_SESSION_H_
#include "SapConnection.h"
#include "ISapSession.h"
#include "ThreadTimer.h"
#include "ITransferObj.h"
#include "SapMessage.h"
#include "SapRecord.h"
#include "IComposeBaseObj.h"
#include "CVirtualClientLoader.h"
#include "AsyncVirtualService.h"
#include "AsyncLogHelper.h"

class CSessionManager;
class CVirtualClientSession:public ITransferObj,public IComposeBaseObj
{
public:    
    CVirtualClientSession(CSessionManager *pManager, CVirtualClientLoader& virtualClientLoader)
	:ITransferObj("CVirtualClientSession",SOS_AUDIT_MODULE, 0, VIRTUAL_CLIENT_ADDRESS, pManager),
	IComposeBaseObj("CSosSessionVirtualClient",pManager,SOS_AUDIT_MODULE),m_pManager(pManager),
    m_virtualClientLoader(virtualClientLoader){}
	virtual int WriteAvs(const void * pBuffer, int nLen)
    {return m_virtualClientLoader.OnRequestService(m_pManager, pBuffer, nLen);}
	
    bool isInService(unsigned serviceId){return m_virtualClientLoader.isInService(serviceId);}
    virtual void WriteData(const void * pBuffer, int nLen){}
	virtual void WriteData(const void * pBuffer, int nLen, const void* pExhead, int nExlen){}
	virtual void DoTransferVirtualClientResponse(void* handle, const void * pBuffer, int nLen) //所有的响应都会走到这里
	{ 
		SS_XLOG(XLOG_DEBUG,"CVirtualClientSession::%s ...\n",__FUNCTION__);
		m_virtualClientLoader.OnResponseService(handle,pBuffer,nLen);
	}
	void WriteComposeData(void* handle, const void * pBuffer, int nLen) ; //组合流程的响应
	
private:
    CSessionManager* m_pManager;
	CVirtualClientLoader &m_virtualClientLoader;
};
    
	
#endif

