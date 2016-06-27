#include "ITransferObj.h"
#include "ComposeServiceObj.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"
#include "SessionManager.h"
#include "SapLogHelper.h"
#include "SapRecord.h"
#include "MapMemory.h"
#include "SapMessage.h"
#include "SapStack.h"

void InitSapMsg(SSapMsgHeader &rem, int nCode);


void RecordStoreForwardMsg(const SPeerSeqPtr& obj, const string& desAddr,int nCode, CSessionManager* pManager);


ITransferObj::ITransferObj(const string & strModule,int level,int nIndex,const string &strAddr,CSessionManager *pManager):
    m_nSendErrorCount(0),m_seq(0),m_strModule(strModule),m_nIndex(nIndex),m_strAddr(strAddr),m_pManager(pManager),
    m_tmTimeout(pManager,ASC_TIMEOUT_CHECK,boost::bind(&ITransferObj::OnTransferTimeout,this),CThreadTimer::TIMER_CIRCLE,CThreadTimer::TIMER_UNIT_MILLI_SECOND),
   m_level(level)
{
    m_tmTimeout.Start();
}
    
ITransferObj::~ITransferObj()
{
    map<unsigned int,SPeerSeqPtr>::iterator itr;    
    SSapMsgHeader response;
    InitSapMsg(response,SAP_CODE_NOT_FOUND); 

    for(itr=m_SocSeqPtr.begin();itr!=m_SocSeqPtr.end();++itr)
    {
        SPeerSeqPtr & obj=itr->second;
        response.dwServiceId=htonl(obj.dwServiceId);
        response.dwMsgId=htonl(obj.dwMsgId);
        response.dwSequence=obj.dwSeq;
        switch(obj.eReqSource)
        {
            case SAP_Request_Remote:                 
                m_pManager->GetLogTrigger()->EndTrigger(&response,sizeof(SSapMsgHeader),obj.sLog);            
                obj.ptrConn->WriteData(&response,sizeof(SSapMsgHeader));
                break;
			case SAP_Request_VirtualClient:
				m_pManager->GetLogTrigger()->EndTrigger(&response,sizeof(SSapMsgHeader),obj.sLog);
				((CSessionManager *)m_pManager)->DoTransferVirtualClientResponse(obj.handle,&response,sizeof(SSapMsgHeader));				
				break;
            case SAP_Request_Local:
                m_pManager->GetLogTrigger()->EndTrigger(&response,sizeof(SSapMsgHeader),obj.sLog);            
                ((CSessionManager *)m_pManager)->OnManagerResponse(&response,sizeof(SSapMsgHeader));
                break;
            case SAP_Request_Compose_Service:               
				if (obj.sAckInfo.bAckMsg)
				{
					void *pExBuffer = CMapMemory::Instance().Allocate(obj.sAckInfo.nExLen);
					SS_XLOG(XLOG_NOTICE,"%s::%s alloc map memory[%x] len[%d] serviceId[%d] msgId[%d]\n",
						m_strModule.c_str(), __FUNCTION__, pExBuffer, obj.sAckInfo.nExLen, obj.dwServiceId, obj.dwMsgId);
					if (pExBuffer != NULL)
					{                        
						memcpy(pExBuffer, obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
						((CSessionManager *)m_pManager)->DoAckRequestEnqueue(pExBuffer, obj.sAckInfo.nExLen);
						response.dwCode = htonl(SAP_CODE_SUCC);
					}
					else
					{
						SS_XLOG(XLOG_NOTICE,"%s::%s alloc map memory failed, len[%d] serviceId[%d] msgId[%d]\n",
							m_strModule.c_str(),__FUNCTION__, obj.sAckInfo.nExLen, obj.dwServiceId, obj.dwMsgId);
					}
					free(obj.sAckInfo.pExBuffer);
				}
				obj.sCompose.pObj->OnReceiveResponse(&response,sizeof(SSapMsgHeader), false, obj.tmReceived);
               break;
			case SAP_Request_MapQ:
				((CSessionManager *)m_pManager)->DoAckRequestEnqueue(obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
				break;
            default:
                break;
        }

        RecordOperate(obj, response.dwCode);
    }
    m_SocSeqPtr.clear();
    m_tmTimeout.Stop();
}


int ITransferObj::OnTransferAVSRequest(const void *pBuffer, int nLen,
    const SComposeTransfer& sCompose, 
    const STransferData& sData)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;    

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;
    int nRet = WriteAvs(pBuffer,nLen);
    if (nRet == 0)
    {
        SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];    
        s.dwSeq=dwOldSeq;    
        s.dwServiceId = ntohl(pHead->dwServiceId);
        s.dwMsgId = ntohl(pHead->dwMsgId);
        s.eReqSource = SAP_Request_Compose_Service;
        s.sCompose = sCompose;
               
        m_pManager->gettimeofday(&(s.tmReceived), false);
        s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
        s.ullTimeout +=  sData.dwTimeout;
        m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));

        SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u],buffer[%x],nlen[%d], old seq[%d]\n",
            m_strModule.c_str(),__FUNCTION__,dwNewSeq,pBuffer,nLen, dwOldSeq);    
    }
    return nRet;
}

int ITransferObj::OnTransferAVSRequest(const void *pBuffer, int nLen, void* handle)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;    

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;
    int nRet = WriteAvs(pBuffer,nLen);
    if (nRet == 0)
    {    
        SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];  
		s.handle=handle; 		
        s.dwSeq=dwOldSeq;    
        s.dwServiceId = ntohl(pHead->dwServiceId);
        s.dwMsgId = ntohl(pHead->dwMsgId);
        s.eReqSource = SAP_Request_VirtualClient;
        s.sInfo = GetRequestInfo(pBuffer,nLen);
        s.strAddr = VIRTUAL_CLIENT_ADDRESS;
        m_pManager->gettimeofday(&(s.tmReceived), false);
        s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
        s.ullTimeout += ASC_TIMEOUT_INTERVAL * 1000;        
        m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));
        m_pManager->GetLogTrigger()->BeginTrigger(pBuffer,nLen,s.sLog);

        SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u], old seq[%d]\n",
            m_strModule.c_str(),__FUNCTION__,dwNewSeq, dwOldSeq);    
    }
    return nRet;
}

int ITransferObj::OnTransferAVSRequest(SapConnection_ptr ptrConn, const void *pBuffer, int nLen)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;    

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;
    int nRet = WriteAvs(pBuffer,nLen);
    if (nRet == 0)
    {    
        SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];    
        s.dwSeq=dwOldSeq;    
        s.dwServiceId = ntohl(pHead->dwServiceId);
        s.dwMsgId = ntohl(pHead->dwMsgId);
        s.eReqSource = SAP_Request_Remote;
        s.ptrConn = ptrConn;
        s.sInfo = GetRequestInfo(pBuffer,nLen);
        s.strAddr = GetIpAddr(pBuffer,nLen,NULL,0);
        m_pManager->gettimeofday(&(s.tmReceived), false);
        s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
        s.ullTimeout += ASC_TIMEOUT_INTERVAL * 1000;        
        m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));
        m_pManager->GetLogTrigger()->BeginTrigger(pBuffer,nLen,s.sLog);

        SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u], old seq[%d]\n",
            m_strModule.c_str(),__FUNCTION__,dwNewSeq, dwOldSeq);    
    }
    return nRet;
}


void ITransferObj::OnTransferRequest(const void *pBuffer, int nLen,
     const SComposeTransfer& sCompose,
    const STransferData& sData)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;    

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;
    
    SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];    
    s.dwSeq=dwOldSeq;    
    s.dwServiceId = ntohl(pHead->dwServiceId);
    s.dwMsgId = ntohl(pHead->dwMsgId);
    s.eReqSource = SAP_Request_Compose_Service;
    s.sCompose = sCompose;
	if (sData.bAckMsg)
	{   
		s.sAckInfo.bAckMsg = true;
        int nExLen = nLen + sizeof(SForwardMsg);
        SForwardMsg *pForwardMsg = (SForwardMsg*)malloc(nExLen);
		pForwardMsg->dwComposeSrvId = s.sCompose.dwServiceId;
        pForwardMsg->dwComposeMsgId = s.sCompose.dwMsgId;
        //pForwardMsg->dwOriginalSeq  = s.dwOriginalSeq;		
	    memcpy(pForwardMsg->pBuffer, pBuffer, nLen);
        s.sAckInfo.pExBuffer = (void*)pForwardMsg;
		s.sAckInfo.nExLen = nExLen;
	}
    m_pManager->gettimeofday(&(s.tmReceived), false);
    s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
    s.ullTimeout += sData.dwTimeout;
    m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));

    SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u],buffer[%x],nlen[%d], old seq[%d]\n",
        m_strModule.c_str(),__FUNCTION__,dwNewSeq,pBuffer,nLen, dwOldSeq);    
    WriteData(pBuffer,nLen);
}

void ITransferObj::OnTransferDataRequest(const void *pBuffer, int nLen,
                                     const SComposeTransfer& sCompose,
                                     const STransferData& sData)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;    

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;

    SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];    
    s.dwSeq=dwOldSeq;    
    s.dwServiceId = ntohl(pHead->dwServiceId);
    s.dwMsgId = ntohl(pHead->dwMsgId);
    s.eReqSource = SAP_Request_Data;
    s.sCompose = sCompose;

    SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u],buffer[%x],nlen[%d]\n",
        m_strModule.c_str(),__FUNCTION__, m_seq, pBuffer, nLen);

    WriteData(pBuffer,nLen);
}

void ITransferObj::OnTransferRequest(ERequestSource eReqFrom,SapConnection_ptr ptrConn,
    const void *pBuffer, int nLen)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;    

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;

    SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];    
    s.dwSeq=dwOldSeq;    
    s.ptrConn=ptrConn;
    s.dwServiceId = ntohl(pHead->dwServiceId);
    s.dwMsgId = ntohl(pHead->dwMsgId);
    s.eReqSource = eReqFrom;
    s.sInfo = GetRequestInfo(pBuffer,nLen);
    s.strAddr = GetIpAddr(pBuffer,nLen,NULL,0);
    m_pManager->gettimeofday(&(s.tmReceived), false);
    s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
    s.ullTimeout += ASC_TIMEOUT_INTERVAL * 1000;
    m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));

    SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u],buffer[%x],nlen[%d], rquest from[%d], old seq[%d]\n",
        m_strModule.c_str(),__FUNCTION__,dwNewSeq,pBuffer,nLen, eReqFrom, dwOldSeq);    
    WriteData(pBuffer,nLen);
}

void ITransferObj::OnTransferRequest(ERequestSource eReqFrom, const void *pBuffer, int nLen, void* handle)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;
    
    SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];
	s.handle=handle;
    s.dwSeq=dwOldSeq;
    s.dwServiceId = ntohl(pHead->dwServiceId);
    s.dwMsgId = ntohl(pHead->dwMsgId);
    s.eReqSource = eReqFrom;
    //s.pCSObj = NULL;
    s.sInfo = GetRequestInfo(pBuffer,nLen);
    s.strAddr = VIRTUAL_CLIENT_ADDRESS;
    m_pManager->GetLogTrigger()->BeginTrigger(pBuffer,nLen,s.sLog);
    m_pManager->gettimeofday(&(s.tmReceived), false);
    s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
    s.ullTimeout += ASC_TIMEOUT_INTERVAL * 1000;
    m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));

    SS_XLOG(XLOG_DEBUG,"ITransferObj::%s, seq[%u],buffer[%x],nlen[%d], rquest from[%d], old seq[%d]\n",
        __FUNCTION__,dwNewSeq,pBuffer,nLen, eReqFrom, dwOldSeq);
    WriteData(pBuffer,nLen);
}

void ITransferObj::OnTransferRequest(ERequestSource eReqFrom, SapConnection_ptr ptrConn, const void *pBuffer, int nLen,const void*pExhead, int nExlen)
{
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwOldSeq=pHead->dwSequence;

    unsigned int dwNewSeq=m_seq++;
    pHead->dwSequence=dwNewSeq;
    
    SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];
    s.dwSeq=dwOldSeq;
    s.ptrConn=ptrConn;
    s.dwServiceId = ntohl(pHead->dwServiceId);
    s.dwMsgId = ntohl(pHead->dwMsgId);
    s.eReqSource = eReqFrom;
    //s.pCSObj = NULL;
    s.sInfo = GetRequestInfo(pBuffer,nLen);
    s.strAddr = GetIpAddr(pBuffer,nLen,pExhead,nExlen);
    m_pManager->GetLogTrigger()->BeginTrigger(pBuffer,nLen,s.sLog);
    m_pManager->gettimeofday(&(s.tmReceived), false);
    s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
    s.ullTimeout += ASC_TIMEOUT_INTERVAL * 1000;
    m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));

    SS_XLOG(XLOG_DEBUG,"ITransferObj::%s, seq[%u],buffer[%x],nlen[%d], rquest from[%d], old seq[%d]\n",
        __FUNCTION__,dwNewSeq,pBuffer,nLen, eReqFrom, dwOldSeq);
    WriteData(pBuffer,nLen, pExhead, nExlen);
}

void ITransferObj::OnTransferMapQRequest(void *pExBuffer, int nExLen)
{
    SForwardMsg *pForwardMsg = (SForwardMsg*)pExBuffer;
    int nLen = nExLen - sizeof(SForwardMsg);
	SSapMsgHeader *pHead = (SSapMsgHeader *)(pForwardMsg->pBuffer);
	unsigned int dwOldSeq = pHead->dwSequence;
	unsigned int dwNewSeq = m_seq++;
	pHead->dwSequence=dwNewSeq;

	SPeerSeqPtr & s=m_SocSeqPtr[dwNewSeq];
	s.dwSeq=dwOldSeq;
	s.dwServiceId = ntohl(pHead->dwServiceId);
	s.dwMsgId = ntohl(pHead->dwMsgId);    
	s.eReqSource = SAP_Request_MapQ;
	//s.pCSObj = NULL;
	s.sAckInfo.bAckMsg = true;
	s.sAckInfo.pExBuffer = pExBuffer;
	s.sAckInfo.nExLen = nExLen;
    s.strAddr = GetIpAddr(pForwardMsg->pBuffer,nLen,NULL,0);
    s.sCompose.dwServiceId = pForwardMsg->dwComposeSrvId;
    s.sCompose.dwMsgId = pForwardMsg->dwComposeMsgId;
    //s.dwOriginalSeq  = pForwardMsg->dwOriginalSeq;
    s.sInfo = GetRequestInfo(pForwardMsg->pBuffer,nLen);

    m_pManager->gettimeofday(&(s.tmReceived), false);
    s.ullTimeout = s.tmReceived.tv_sec * 1000LL + s.tmReceived.tv_usec / 1000;
    s.ullTimeout += ASC_TIMEOUT_INTERVAL * 1000;
    m_TimeList.insert(make_pair(s.ullTimeout, dwNewSeq));

	SS_XLOG(XLOG_DEBUG,"%s::%s, seq[%u],buffer[%x],nlen[%d], rquest from[%d], old seq[%d]\n",
		m_strModule.c_str(),__FUNCTION__,dwNewSeq,pExBuffer,nExLen, SAP_Request_MapQ, dwOldSeq);

	//((CSessionManager *)m_pManager)->m_oReportData.dwRequest++;

	WriteData(pForwardMsg->pBuffer,nLen);
}

void ITransferObj::OnTransferResponse(const void * pBuffer, int nLen)
{    
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwNewSeq=pHead->dwSequence;
    unsigned int dwCode=ntohl(pHead->dwCode);       
	if (dwCode==(unsigned)SAP_CODE_QUEUE_FULL || 
		dwCode==(unsigned)SAP_CODE_SEND_FAIL)
	{
		m_nSendErrorCount++;
	}
	else
	{
		m_nSendErrorCount=0;
	}

    map<unsigned int,SPeerSeqPtr>::iterator itr=m_SocSeqPtr.find(dwNewSeq);
    if(itr!=m_SocSeqPtr.end())
    {
        SPeerSeqPtr &obj=itr->second;
        pHead->dwSequence = obj.dwSeq;
        SS_XLOG(XLOG_DEBUG,"%s::%s,buffer[%x],nlen[%d],seq[%d],old seq[%d],code[%d],src[%d]\n",
            m_strModule.c_str(),__FUNCTION__,pBuffer,nLen,dwNewSeq,obj.dwSeq,dwCode,obj.eReqSource);


        if(obj.eReqSource == SAP_Request_Remote)
        {
            m_pManager->GetLogTrigger()->EndTrigger(pBuffer,nLen,obj.sLog);    
            obj.ptrConn->WriteData(pBuffer,nLen);
        }
        else if(obj.eReqSource == SAP_Request_Local)
        {            
            m_pManager->GetLogTrigger()->EndTrigger(pBuffer,nLen,obj.sLog);
            ((CSessionManager *)m_pManager)->OnManagerResponse(pBuffer,nLen);
        }
		else if(obj.eReqSource == SAP_Request_VirtualClient)
		{
			m_pManager->GetLogTrigger()->EndTrigger(pBuffer,nLen,obj.sLog); 
			((CSessionManager *)m_pManager)->DoTransferVirtualClientResponse(obj.handle,pBuffer,nLen);
		}
        else if(obj.eReqSource == SAP_Request_Data)
        {
            m_SocSeqPtr.erase(itr);
            return;
            ;// to do record
        }
        if(dwCode<100||dwCode>=200)
        {            
            if(obj.eReqSource == SAP_Request_Compose_Service)
            {                
				if (obj.sAckInfo.bAckMsg)
				{
					if (dwCode == (unsigned)SAP_CODE_QUEUE_FULL || 
						dwCode == (unsigned)SAP_CODE_SEND_FAIL)
					{
						void *pMapBuffer = CMapMemory::Instance().Allocate(obj.sAckInfo.nExLen);						
						if (pMapBuffer != NULL)
						{
							memcpy(pMapBuffer, obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
							m_pManager->DoAckRequestEnqueue(pMapBuffer, obj.sAckInfo.nExLen);
							pHead->dwCode = htonl(SAP_CODE_SUCC);
						}
						else
						{
							SS_XLOG(XLOG_ERROR,"%s::%s alloc map memory failed, len[%d] atom[%d:%d]\n",
								m_strModule.c_str(),__FUNCTION__, obj.sAckInfo.nExLen, 
								obj.dwServiceId, obj.dwMsgId);
						}
					}
					else if (dwCode != SAP_CODE_SUCC)
					{
                       RecordStoreForwardMsg(obj,m_strAddr,dwCode, m_pManager);
					}
					free(obj.sAckInfo.pExBuffer);
				}				

				obj.sCompose.pObj->OnReceiveResponse(pBuffer,nLen, false, obj.tmReceived);
			}
			else if (obj.eReqSource == SAP_Request_MapQ)
			{
				if (dwCode == (unsigned)SAP_CODE_QUEUE_FULL || 
					dwCode == (unsigned)SAP_CODE_SEND_FAIL)
				{
					m_pManager->DoAckRequestEnqueue(obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
				}
				else
				{
					SS_XLOG(XLOG_NOTICE,"%s::%s dealloc map memory[%x] len[%d] atom[%d:%d]\n",
						m_strModule.c_str(), __FUNCTION__, obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen, 
						obj.dwServiceId, obj.dwMsgId);
					CMapMemory::Instance().Deallocate(obj.sAckInfo.pExBuffer);
					if (dwCode != SAP_CODE_SUCC)
					{
						RecordStoreForwardMsg(obj,m_strAddr,dwCode, m_pManager);		
					}
				}
			}

            RecordOperate(obj, dwCode);
            
            std::pair<TimeStampMMap::iterator, TimeStampMMap::iterator> it_pair 
                = m_TimeList.equal_range(obj.ullTimeout);
            for (TimeStampMMap::iterator cit=it_pair.first; 
                cit!=it_pair.second;
                ++cit)
            {
                if (cit->second == dwNewSeq)
                {
                    m_TimeList.erase(cit);
                    break;
                }
            }
            m_SocSeqPtr.erase(itr);
        }
    }
    else
    {
        SS_XLOG(XLOG_DEBUG,"%s::%s,can't find route, seq[%u]\n",m_strModule.c_str(),__FUNCTION__,dwNewSeq);
    }
}

void ITransferObj::OnTransferTimeout()
{
    //SS_XLOG(XLOG_TRACE,"%s::%s\n",m_strModule.c_str(),__FUNCTION__);    
    struct timeval_a now;
    m_pManager->gettimeofday(&now, false);
    unsigned long long ullTimeNow = now.tv_sec * 1000LL + now.tv_usec / 1000;

    SSapMsgHeader resTm;
    InitSapMsg(resTm,SAP_CODE_TIMEOUT); 

    multimap<unsigned long long, unsigned int>::iterator itrTimeOut;
    multimap<unsigned long long, unsigned int>::iterator itrTimeOutTmp;
    for(itrTimeOut = m_TimeList.begin(); itrTimeOut != m_TimeList.end();)
    {
        itrTimeOutTmp = itrTimeOut++;

        if (ullTimeNow < itrTimeOutTmp->first)
        {
            break;
        }

        map<unsigned int,SPeerSeqPtr>::iterator itrSocSeqPtr = m_SocSeqPtr.find(itrTimeOutTmp->second);
        if (itrSocSeqPtr == m_SocSeqPtr.end())
        {
            m_TimeList.erase(itrTimeOutTmp);
            continue;
        }

        SPeerSeqPtr & obj=itrSocSeqPtr->second;

		m_nSendErrorCount++;
        resTm.dwServiceId=htonl(obj.dwServiceId);
        resTm.dwMsgId=htonl(obj.dwMsgId);            
        resTm.dwSequence=obj.dwSeq;            
        switch(obj.eReqSource)
        {
            case SAP_Request_Remote:                    
                m_pManager->GetLogTrigger()->EndTrigger(&resTm,sizeof(SSapMsgHeader),obj.sLog);            
                obj.ptrConn->WriteData(&resTm,sizeof(SSapMsgHeader));
                break;
			case SAP_Request_VirtualClient:
				m_pManager->GetLogTrigger()->EndTrigger(&resTm,sizeof(SSapMsgHeader),obj.sLog);
				((CSessionManager *)m_pManager)->DoTransferVirtualClientResponse(obj.handle,&resTm,sizeof(SSapMsgHeader));				
				break;
            case SAP_Request_Local:                    
                m_pManager->GetLogTrigger()->EndTrigger(&resTm,sizeof(SSapMsgHeader),obj.sLog);            
                ((CSessionManager *)m_pManager)->OnManagerResponse(&resTm,sizeof(SSapMsgHeader));                    
                break;
            case SAP_Request_Compose_Service:                    
				if (obj.sAckInfo.bAckMsg)
				{
					void *pExBuffer = CMapMemory::Instance().Allocate(obj.sAckInfo.nExLen);
					SS_XLOG(XLOG_NOTICE,"%s::%s alloc map memory[%x] len[%d] atom[%d:%d]\n",
						m_strModule.c_str(), __FUNCTION__, pExBuffer, obj.sAckInfo.nExLen, 
						obj.dwServiceId, obj.dwMsgId);
					if (pExBuffer != NULL)
					{
						memcpy(pExBuffer, obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
						((CSessionManager *)m_pManager)->DoAckRequestEnqueue(pExBuffer, obj.sAckInfo.nExLen);
						resTm.dwCode = htonl(SAP_CODE_SUCC);
					}
					else
					{
						SS_XLOG(XLOG_NOTICE,"%s::%s alloc map memory failed, len[%d] atom[%d:%d]\n",
							m_strModule.c_str(),__FUNCTION__, obj.sAckInfo.nExLen, 
							obj.dwServiceId, obj.dwMsgId);
					}
					free(obj.sAckInfo.pExBuffer);
				}
				obj.sCompose.pObj->OnReceiveResponse(&resTm,sizeof(SSapMsgHeader), false, obj.tmReceived);
                resTm.dwCode = htonl(SAP_CODE_TIMEOUT);
                break;
			case SAP_Request_MapQ:
				((CSessionManager *)m_pManager)->DoAckRequestEnqueue(obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
				break;
            default:
                break;
        }     
        RecordOperate(obj, SAP_CODE_TIMEOUT);   
        m_SocSeqPtr.erase(itrSocSeqPtr); 
        m_TimeList.erase(itrTimeOutTmp);
    }
}

void ITransferObj::OnStopComposeService()
{
    SS_XLOG(XLOG_DEBUG,"%s::%s [%u]\n",m_strModule.c_str(),__FUNCTION__, m_SocSeqPtr.size()); 
    SSapMsgHeader resTm;
    InitSapMsg(resTm,SAP_CODE_CONFIG_RELOAD);    
    map<unsigned int,SPeerSeqPtr>::iterator itr;
    map<unsigned int,SPeerSeqPtr>::iterator itrTmp;
    for(itr=m_SocSeqPtr.begin();itr!=m_SocSeqPtr.end();)
    {
        itrTmp = itr++;
        SPeerSeqPtr & obj = itrTmp->second; 
        if(obj.sCompose.pObj != NULL)
        {                 
            resTm.dwServiceId = htonl(obj.dwServiceId);
            resTm.dwMsgId = htonl(obj.dwMsgId);
            resTm.dwSequence = obj.dwSeq;
			if (obj.sAckInfo.bAckMsg)
			{
				void *pExBuffer = CMapMemory::Instance().Allocate(obj.sAckInfo.nExLen);
				SS_XLOG(XLOG_DEBUG,"%s::%s alloc map memory[%x] len[%d] atom[%d:%d]\n",
					m_strModule.c_str(), __FUNCTION__, pExBuffer, obj.sAckInfo.nExLen, 
					obj.dwServiceId, obj.dwMsgId);
				if (pExBuffer != NULL)
				{
					memcpy(pExBuffer, obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
					((CSessionManager *)m_pManager)->DoAckRequestEnqueue(pExBuffer, obj.sAckInfo.nExLen);
					resTm.dwCode = htonl(SAP_CODE_SUCC);
				}
				else
				{
					SS_XLOG(XLOG_NOTICE,"%s::%s alloc map memory failed, len[%d] atom[%d:%d]\n",
						m_strModule.c_str(),__FUNCTION__, obj.sAckInfo.nExLen, 
						obj.dwServiceId, obj.dwMsgId);
				}
				free(obj.sAckInfo.pExBuffer);
			}
			obj.sCompose.pObj->OnReceiveResponse(&resTm,sizeof(SSapMsgHeader), false, obj.tmReceived);
            RecordOperate(obj, SAP_CODE_CONFIG_RELOAD);  
            m_SocSeqPtr.erase(itrTmp);             
        }
		if (itrTmp->second.eReqSource == SAP_Request_MapQ)
		{
			((CSessionManager *)m_pManager)->DoAckRequestEnqueue(obj.sAckInfo.pExBuffer, obj.sAckInfo.nExLen);
		}
    }
}

void ITransferObj::RecordOperate(const SPeerSeqPtr& oSeq, int nCode)
{       
    if(IsEnableLevel(m_level, XLOG_INFO) && oSeq.eReqSource != SAP_Request_Compose_Service)
	{
        SS_XLOG(XLOG_TRACE,"%s::%s\n",m_strModule.c_str(),__FUNCTION__);
        struct tm tmStart, tmNow;
		struct timeval_a now,interval;
        m_pManager->gettimeofday(&now);
		localtime_r(&(oSeq.tmReceived.tv_sec), &tmStart);
        localtime_r(&(now.tv_sec), &tmNow);
        timersub(&now,&(oSeq.tmReceived), &interval);
        unsigned dwInterMS = interval.tv_sec*1000+interval.tv_usec/1000; 

        const string strStartTm = TimeToString(tmStart,oSeq.tmReceived.tv_usec/1000);
        const string strNowTm = TimeToString(tmNow,now.tv_usec/1000);

        char szBuf[MAX_LOG_BUFFER]={0};
        if (oSeq.eReqSource == SAP_Request_MapQ)
        {                    
           // request time, source ip, appid, areaid, username,old seq,new seq,
           //compose serviceid, msgid, sub serviceid,msgid,destination addr, response time, type, interval, code  
           //snprintf(szBuf,sizeof(szBuf)-1,"%-23s, %-20s, %4d, %4d, %-20s, %-9u, %-9u, %5d, %4d, "
           //"%5d, %4d, %-20s, %-23s, %4d.%03ld, %-9d\n",
           
            if (CSapStack::nLogType == 0)
            {
               snprintf(szBuf,sizeof(szBuf)-1,"%s,  %s,  %d,  %d,  %s,  %s,  %u,  %d,  %d,  "\
               "%d,  %d,  %s,  %s,  %d,  %d.%03ld,  %d\n",        
               strStartTm.c_str(), oSeq.strAddr.c_str(), oSeq.sInfo.nAppId,oSeq.sInfo.nAreaId,
               oSeq.sInfo.strUserName.c_str(),oSeq.sInfo.strGuid.c_str(),oSeq.dwSeq,oSeq.sCompose.dwServiceId,oSeq.sCompose.dwMsgId,
               oSeq.dwServiceId, oSeq.dwMsgId, m_strAddr.c_str(),
               strNowTm.c_str(),oSeq.eReqSource, dwInterMS,interval.tv_usec%1000, nCode);
            }
            else
            {
                snprintf(szBuf,sizeof(szBuf)-1,"%s,  %s,  %d,  %d,  %s,  ,  ,  "\
                    "[NULL],  [NULL],  %d.%03ld,  %d\n",
                    strStartTm.c_str(), oSeq.sInfo.strGuid.c_str(),
                    oSeq.dwServiceId, oSeq.dwMsgId, m_strAddr.c_str(),
                    dwInterMS,interval.tv_usec % 1000, nCode);
            }
            szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
            CAsyncLogThread::GetInstance()->OnAsyncLog(SCS_AUDIT_MODULE, XLOG_INFO, m_nIndex,
                dwInterMS,szBuf,nCode, oSeq.strAddr,oSeq.dwServiceId,oSeq.dwMsgId);
        }
        else
        {
           //forward message              
           //request time, source ip,appid,areaid,username,old seq,new seq,
           //serviceid, msgid,destination addr, response time, interval, error info,code   
           //snprintf(szBuf,sizeof(szBuf)-1,"%-23s, %-20s, %4d, %4d, %-20s,"
           //" %-9u, %-9u, %5d, %4d, %-20s, %-23s, %4d.%03ld, %-10s, %-9d\n",            
           if (CSapStack::nLogType == 0)
           {
                snprintf(szBuf,sizeof(szBuf)-1,"%s,  %s,  %d,  %d,  %s,"\
               "  %s,  %u,  %d,  %d,  %s,  %s,  %d.%03ld,  0-0,  %s,  %d\n",        
               strStartTm.c_str(), oSeq.strAddr.c_str(), oSeq.sInfo.nAppId,oSeq.sInfo.nAreaId,
               oSeq.sInfo.strUserName.c_str(),oSeq.sInfo.strGuid.c_str(),oSeq.dwSeq,
               oSeq.dwServiceId, oSeq.dwMsgId, m_strAddr.c_str(), 
               strNowTm.c_str(),dwInterMS,interval.tv_usec%1000,oSeq.sLog.logInfo.c_str(), nCode);
           }
           else
           {
               snprintf(szBuf, sizeof(szBuf)-1, "%s,  %s,  %d,  %d,  %s,  "\
                   "%s,  %s,  %d,  %d,  %s,  "\
                   "%s,  %d.%03ld,  %s,  %s,  %d,  %s,  %s\n",
                   strStartTm.c_str(),
                   oSeq.strAddr.c_str(),
                   oSeq.sInfo.nAppId,
                   oSeq.sInfo.nAreaId,
                   oSeq.sInfo.strUserName.c_str(),
                   oSeq.sInfo.strGuid.c_str(),
                   oSeq.sInfo.strLogId.c_str(),
                   oSeq.dwServiceId,
                   oSeq.dwMsgId,
                   m_strAddr.c_str(),
                   strNowTm.c_str(),
                   dwInterMS,
                   interval.tv_usec % 1000,
                   "0-0", /*strErrInfo.c_str()*/
                   oSeq.sLog.logInfo.c_str(),
                   nCode,
                   oSeq.sLog.logInfo2.c_str(),
                   oSeq.sInfo.strComment.c_str());
           }
           szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
           CAsyncLogThread::GetInstance()->OnAsyncLog(m_level, XLOG_INFO, m_nIndex,
            dwInterMS,szBuf,nCode,oSeq.strAddr,oSeq.dwServiceId,oSeq.dwMsgId);
        }
        
	}
}

void RecordStoreForwardMsg(const SPeerSeqPtr& obj, const string& desAddr,int nCode, CSessionManager* pManager)
{
    struct tm tmStart, tmNow;
    struct timeval_a now,interval;
    pManager->gettimeofday(&now);
    localtime_r(&(obj.tmReceived.tv_sec), &tmStart);
    localtime_r(&(now.tv_sec), &tmNow);
    timersub(&now,&(obj.tmReceived), &interval);
    unsigned dwInterMS = interval.tv_sec*1000+interval.tv_usec/1000;
    //source ip, request time,original seq, compose serviceid:msgid, atom serviceid:msgid,destination addr
    // response time, interval,code     
    //SF_XLOG(XLOG_FATAL,"%-20s, %04u-%02u-%02u %02u:%02u:%02u.%03ld, %9d, %4d,%4d, %4d,%4d, %-20s, "
//        "%04u-%02u-%02u %02u:%02u:%02u.%03ld, %4d.%03ld, %9d\n",
    if (CSapStack::nLogType == 0)
    {
        SF_XLOG(XLOG_FATAL,"%s,  %04u-%02u-%02u %02u:%02u:%02u.%03ld,  %s,  %d,  %d,  %d,  %d,  %s,  "\
            "%04u-%02u-%02u %02u:%02u:%02u.%03ld,  %d.%03ld,  %d\n",
            obj.strAddr.c_str(), tmStart.tm_year+1900,tmStart.tm_mon+1,tmStart.tm_mday,tmStart.tm_hour,tmStart.tm_min,tmStart.tm_sec,obj.tmReceived.tv_usec/1000, 
            obj.sInfo.strGuid.c_str(), obj.sCompose.dwServiceId,obj.sCompose.dwMsgId,obj.dwServiceId, obj.dwMsgId, desAddr.c_str(), 
            tmNow.tm_year+1900,tmNow.tm_mon+1,tmNow.tm_mday,tmNow.tm_hour,tmNow.tm_min,tmNow.tm_sec,now.tv_usec/1000,
            dwInterMS,interval.tv_usec%1000,nCode);
    }
    else
    {
        SF_XLOG(XLOG_FATAL,"%s,  %04u-%02u-%02u %02u:%02u:%02u.%03ld,  %s,  %s,  %s,  %d,  %d,  %d,  %d,  %s,  "\
            "%04u-%02u-%02u %02u:%02u:%02u.%03ld,  %d.%03ld,  %d\n",
            obj.strAddr.c_str(), tmStart.tm_year+1900,tmStart.tm_mon+1,tmStart.tm_mday,tmStart.tm_hour,tmStart.tm_min,tmStart.tm_sec,obj.tmReceived.tv_usec/1000, 
            obj.sInfo.strGuid.c_str(), obj.sInfo.strLogId.c_str(), obj.sInfo.strComment.c_str(), obj.sCompose.dwServiceId,obj.sCompose.dwMsgId,obj.dwServiceId, obj.dwMsgId, desAddr.c_str(), 
            tmNow.tm_year+1900,tmNow.tm_mon+1,tmNow.tm_mday,tmNow.tm_hour,tmNow.tm_min,tmNow.tm_sec,now.tv_usec/1000,
            dwInterMS,interval.tv_usec%1000,nCode);	
    }
}


void InitSapMsg(SSapMsgHeader &response, int nCode)
{
    response.byIdentifer=SAP_PACKET_RESPONSE;
    response.byContext=0;
    response.byPrio=0;
    response.byBodyType=0;
    response.byCharset=0;
    response.byHeadLen=sizeof(SSapMsgHeader);
    response.dwPacketLen=htonl(sizeof(SSapMsgHeader));
    response.byVersion=0x01;
    response.byM=0xFF;
    response.dwCode=htonl(nCode);
    memset(response.bySignature,0,16);
}


