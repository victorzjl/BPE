#ifndef __AVS_SESSION_H__
#define __AVS_SESSION_H__
#include "ITransferObj.h"
#include "AsyncVirtualService.h"
#include "AsyncLogHelper.h"

class CSessionManager;
class CAVSSession:public ITransferObj
{
public:    
    CAVSSession(CSessionManager *pManager, CAsyncVirtualService& asyncVS)
        :ITransferObj("CAvsSession",SOS_AUDIT_MODULE, 0, "127.0.0.1", pManager),
        m_pManager(pManager),
        m_asyncVS(asyncVS){}
    bool isInService(unsigned serviceId){return m_asyncVS.isInService(serviceId);}
    virtual int WriteAvs(const void * pBuffer, int nLen)
    {return m_asyncVS.OnRequestService(m_pManager, pBuffer, nLen);}
    virtual void WriteData(const void * pBuffer, int nLen){}
	virtual void WriteData(const void * pBuffer, int nLen, const void* pExhead, int nExlen){}
private:
    void * m_pManager;
    CAsyncVirtualService& m_asyncVS;
};

#endif

