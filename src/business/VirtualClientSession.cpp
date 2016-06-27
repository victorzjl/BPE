#include "VirtualClientSession.h"
#include "SessionManager.h"
void CVirtualClientSession::WriteComposeData(void* handle,const void * pBuffer, int nLen)  //组合流程的响应
{
	SS_XLOG(XLOG_DEBUG,"CVirtualClientSession::%s ...\n",__FUNCTION__);  
	m_pManager->DoTransferVirtualClientResponse(handle,pBuffer,nLen);
}

