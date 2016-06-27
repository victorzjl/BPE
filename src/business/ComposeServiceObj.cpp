#include "ComposeServiceObj.h"
#include "SessionManager.h"
#include "IComposeBaseObj.h"
#include "AvenueMsgHandler.h"
#include "AvenueServiceConfigs.h"
#include "SapLogHelper.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"
#include "MapMemory.h"
#include "ServiceAvailableTrigger.h"
#include "AvsSession.h"
#include "SapStack.h"
#include "LogTrigger.h"
#include "AvenueTlvGroup.h"
#include "StateMachine.h"
void dump_buffer(const void *pBuffer,EVirtualDirectType eType,const string& serviceName,int nNode=0);
void dump_buffer(const string& strPacketInfo, const void *pBuffer);

void GetAllDefValue(map<string, vector<string> >& def_value_all, const map<string, SDefParameter>& def_parameter, const map<string, SDefValue>& def_values);

const int* CComposeServiceObj::GetNodeCode(const vector<SSeqRet>& vecSeqReturn, int nSeq) const
{
    
    vector<SSeqRet>::const_iterator itr = vecSeqReturn.begin(); 
    for(; itr!=vecSeqReturn.end(); ++itr)
    {
        if ( itr->nSeq == nSeq)
        {
            return &(itr->nCode);
        }
    }
    return NULL;
}

void CComposeServiceObj::AddOrUpdateMsg(RspContainer& mapResponse, const SNodeSeq& sNodeSeq, const SResponseRet& sRsp)
{
    RCIterator it = mapResponse.lower_bound(sNodeSeq);
    if (it != mapResponse.end() &&!(sNodeSeq < it->first))
    { 
        SResponseRet & rsp = it->second;
        delete rsp.pHandler;
        rsp = sRsp;
    }
    else
    {
        mapResponse.insert(it,make_pair(sNodeSeq, sRsp));
    } 
}


int CComposeServiceObj::InitSub(const void *pBuffer, int nLen, SReturnMsgData& returnMsg)
{  
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s...\n",__FUNCTION__); 
	int nValidatorCode=0;
    if((nValidatorCode=SaveRequestMsg(pBuffer, nLen, returnMsg))!=0)
    {        
        return nValidatorCode;
    }     
    m_strRemoteIp = m_pParent->m_strRemoteIp;
    m_rootServId = m_pParent->m_rootServId;
    m_rootMsgId = m_pParent->m_rootMsgId;  
    m_rootSeq = m_pParent->m_rootSeq;
    m_level = m_pParent->m_level;    
    m_tmStart = m_pParent->m_tmStart;
	
	STriggerItem * pSTI =  CTriggerConfig::Instance()->GetInfo(1,m_rootServId, m_rootMsgId);    
	if (pSTI!= NULL)
	{
		m_nCsosLog = pSTI->nCsosLog;
	}  
	else
	{
		m_nCsosLog = 0;
	}
    DoStartNodeService(0);
    return 0;
}


int CComposeServiceObj::Init(unsigned dwIndex, int level, 
    const SSRequestMsg &sReqMsg,const void * pBuffer, int nLen, void* handle, SReturnMsgData& returnMsg)
{  
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s...\n",__FUNCTION__); 
	m_handle = handle;
	int nValidatorCode=0;
    if((nValidatorCode=SaveRequestMsg(pBuffer, nLen, returnMsg))!=0)
    {
        SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s decode request error [%d]\n",__FUNCTION__,nValidatorCode);    
        return nValidatorCode;
    }
	
    CLogTrigger::BeginTrigger(m_pReqHandler, m_rootServId,m_rootMsgId, m_logInfo);

    m_dwIndex = dwIndex;  
    m_level = level;
    SaveExHead(sReqMsg, pBuffer);
    m_pBase->OnAddComposeService(dwIndex, this);     
    DoStartNodeService(0); 
    return 0;
}

int CComposeServiceObj::DecodeMsgBody(const void *pBuffer, int nLen,CAvenueMsgHandler* pMsgHandler, SReturnMsgData& returnMsg)
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s, buffer[%x], len[%d]\n",__FUNCTION__, pBuffer, nLen);    
    SSapMsgHeader *pHeader = (SSapMsgHeader *)pBuffer;
    void *pBody = (unsigned char *)pBuffer + pHeader->byHeadLen;
    int nBodyLen = nLen - pHeader->byHeadLen;    
    return pMsgHandler->Decode(pBody, nBodyLen,returnMsg.nReturnMsgType,returnMsg.strReturnMsg);    
}

int CComposeServiceObj::DecodeMsgBody(const void *pBuffer, int nLen,CAvenueMsgHandler* pMsgHandler)
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s, buffer[%x], len[%d]\n",__FUNCTION__, pBuffer, nLen);    
    SSapMsgHeader *pHeader = (SSapMsgHeader *)pBuffer;
    void *pBody = (unsigned char *)pBuffer + pHeader->byHeadLen;
    int nBodyLen = nLen - pHeader->byHeadLen;    
    return pMsgHandler->Decode(pBody, nBodyLen);    
}

CComposeServiceObj::CComposeServiceObj(IComposeBaseObj *pCBObj,const SComposeServiceUnit &rCSUnit,
    CAvenueServiceConfigs *pConfig,CSessionManager *pManager,unsigned dwSeq)
    :m_pParent(NULL),m_pBase(pCBObj),m_oCSUnit(rCSUnit),
    m_pConfig(pConfig),m_pManager(pManager),m_dwSrcSeq(dwSeq),m_dwServiceSeq(0), m_pReqBuffer(NULL), m_dwBufferLen(0), m_rootMsgId(m_oCSUnit.sKey.dwMsgId),
    m_rootServId(m_oCSUnit.sKey.dwServiceId), m_rootSeq(dwSeq), m_pExHead(new CEXHeadHandler())
	{         
        //m_vecNodeRet.reserve(SEQUENCE_LIMITED);
        m_pManager->gettimeofday(&m_tmStart);
		
		STriggerItem * pSTI =  CTriggerConfig::Instance()->GetInfo(1,m_rootServId, m_rootMsgId);    
		if (pSTI!= NULL)
		{
			m_nCsosLog = pSTI->nCsosLog;
		}  
		else
		{
			m_nCsosLog = 0;
		}
    }

CComposeServiceObj::CComposeServiceObj(const SComposeServiceUnit &rCSUnit,
    unsigned dwSeq,	CComposeServiceObj *pParentNode)
    :m_pParent(pParentNode),m_pBase(NULL),m_oCSUnit(rCSUnit),m_pConfig(pParentNode->m_pConfig),
    m_pManager(pParentNode->m_pManager),m_dwSrcSeq(dwSeq),m_dwServiceSeq(0), m_pReqBuffer(NULL), m_dwBufferLen(0), m_pExHead(pParentNode->m_pExHead)
	{         
        //m_vecNodeRet.reserve(SEQUENCE_LIMITED);        

    }



void CComposeServiceObj::SaveExHead(const SSRequestMsg &sReqMsg, const void* pBuffer)
{    
    SSapMsgHeader *pHeader = (SSapMsgHeader *)pBuffer;
    int nExLen = pHeader->byHeadLen - sizeof(SSapMsgHeader);
    m_pExHead->save(static_cast<const char*>(pBuffer)+sizeof(SSapMsgHeader), nExLen, 
        sReqMsg.pExtHead, sReqMsg.nExLen, sReqMsg.remoteAddr.toString());
	m_pExHead->updateServiceId(m_rootServId);
	m_pExHead->updateMessageId(m_rootMsgId);
    m_strRemoteIp = sReqMsg.remoteAddr.ip; 
}



int CComposeServiceObj::SaveRequestMsg(const void* pBuffer, int nLen, SReturnMsgData& returnMsg)
{    
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s,buffer[%x],len[%d]\n",__FUNCTION__,pBuffer,nLen);    

    SSapMsgHeader *pHeader = (SSapMsgHeader *)pBuffer;
    m_dwBufferLen = nLen - pHeader->byHeadLen;
    if (m_dwBufferLen > 0)
    {
        m_pReqBuffer = malloc(m_dwBufferLen);
        memcpy(m_pReqBuffer, (unsigned char *)pBuffer + pHeader->byHeadLen, m_dwBufferLen);
    }

    m_pReqHandler = new CAvenueMsgHandler(m_oCSUnit.strServiceName,true,m_pConfig);	 
    return DecodeMsgBody(pBuffer, nLen, m_pReqHandler,returnMsg);
	
}


int CComposeServiceObj::RequestSubCompose(const void * pBuffer, int nLen, const SServiceLocation& sLoc, SReturnMsgData& returnMsg)
{
    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;    
    SComposeKey sKey(ntohl(pHead->dwServiceId), ntohl(pHead->dwMsgId));
    const SComposeServiceUnit *pCSUnit = m_pManager->GetComposeUnit(sKey); 
    if(NULL == pCSUnit)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, find sub compose service failed.service[%d],msg[%d]\n",
           __FUNCTION__,sKey.dwServiceId, sKey.dwMsgId);   
        return SAP_CODE_NOT_FOUND;
    }   

    CAvailableTrigger oAvail(m_pConfig, m_pManager);
	if(!oAvail.BeginTrigger(sKey.dwServiceId, sKey.dwMsgId))
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, sub service of compose service[%d],msg[%d] not avaiable\n",
			__FUNCTION__,sKey.dwServiceId, sKey.dwMsgId);  
		return SAP_CODE_NOT_FOUND;
	}

    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, this[%x],service[%d],msg[%d], seq[%u]\n",
           __FUNCTION__,this, sKey.dwServiceId, sKey.dwMsgId, pHead->dwSequence); 
    m_mapService.insert( make_pair(pHead->dwSequence, sLoc) );
    
    CComposeServiceObj *pComposeObj = new CComposeServiceObj(*pCSUnit, ntohl(pHead->dwSequence), this);
	dump_buffer(pBuffer, E_COMPOSE_IN, pCSUnit->strServiceName); 

	int nValidatorCode=0;
    if((nValidatorCode= pComposeObj->InitSub(pBuffer,nLen,returnMsg))!=0)
    {
        delete pComposeObj;
        SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, init sub compose service failed.service[%d],msg[%d]\n",
           __FUNCTION__,sKey.dwServiceId, sKey.dwMsgId); 
        m_mapService.erase(pHead->dwSequence);
        return nValidatorCode;
    }
    
   return 0;
}

void CComposeServiceObj::DoStartNodeService(int nCurrNode, int nLastCode)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, service[%s],node[%s],last code[%d]\n",
        __FUNCTION__,m_oCSUnit.strServiceName.c_str(),getNodeName(m_oCSUnit, nCurrNode),nLastCode); 
    if(DoIsDestroy(nCurrNode))
    {        
        delete this;
        return;
    }    
    m_last_code = nLastCode;
	m_vecNodeRet.clear();
	m_mapNodeLoopRet.clear();
	m_mapSeqLoopTimes.clear();
	m_mapSeqSpSosAddr.clear();
	m_nTotalSeqSize=0;
    
    NodeIterator itrNode = m_oCSUnit.mapNodes.find(nCurrNode);
    if(itrNode == m_oCSUnit.mapNodes.end())
    {
        SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s node[%s] not found\n",__FUNCTION__, getNodeName(m_oCSUnit, nCurrNode));
        return;
    }
    const SComposeNode& rNode = itrNode->second; 
	m_strRunPath+=rNode.strNodeName+",";
		
    CSapEncoder oRequest;
	if(rNode.mapSeqs.size()==0)// have no seq
	{
		DoSaveResponseDef(rNode);
		DoStartNodeService(FindGotoNode(rNode.vecResult));
	}
	else
	{
		BuildSequenceLoopTimes(rNode.mapSeqs);
		BuildSequenceSpSosAddr(rNode.mapSeqs);

		for(SeqIterator itrSeq=rNode.mapSeqs.begin(); itrSeq!=rNode.mapSeqs.end(); ++itrSeq)
		{           
			const SComposeSequence& rSeq = itrSeq->second;
			if(rSeq.enSequenceType == Sequence_Type_Response)
			{                  
				int nCode = DoGetCode(rSeq.sReturnCode,nLastCode);       
				string strErrInfo(m_errInfo);
				if (!m_subErrInfo.empty())
				{                
					strErrInfo.append("_");
					strErrInfo.append(m_subErrInfo);                
				}                       
				CAvenueMsgHandler oBodyEncode(m_oCSUnit.strServiceName,false,m_pConfig);
				string rspParams;
				BuildPacketBody(m_oCSUnit.strServiceName,rSeq.vecField,oBodyEncode,false, rspParams); 
				int nValidatorCode = oBodyEncode.Encode();
				if (nValidatorCode!=0 && nCode==0)
				{
					nCode = nValidatorCode;
				}
				if(m_pParent == NULL)
				{    
				
				
					map<string, vector<string> > defValues;
					GetAllDefValue(defValues);
					CAvenueMsgHandler msgHandler(m_oCSUnit.strServiceName,false,m_pConfig);
				//todo
					CLogTrigger::EndTrigger(oBodyEncode.GetBuffer(),oBodyEncode.GetLen(), &msgHandler,m_logInfo, &defValues);  
					RecordComposeService(nCode,strErrInfo);  
					if (m_pBase != NULL)
					{
						SendToClient(nCode,oBodyEncode.GetBuffer(),oBodyEncode.GetLen());
					}

					if (!m_oCSUnit.vecDataServerId.empty())
					{
						CSapEncoder sapEncoder;
						BuildDataPacketBody(sapEncoder, oBodyEncode.GetBuffer(), oBodyEncode.GetLen(), nCode, defValues);
						vector<unsigned>::const_iterator itrDataServerId = m_oCSUnit.vecDataServerId.begin();
						for(; itrDataServerId != m_oCSUnit.vecDataServerId.end(); ++itrDataServerId)
						{
							CSosSession* pSosSession = m_pManager->OnGetSos(*itrDataServerId);
							if (pSosSession)
							{
								pSosSession->OnTransferDataRequest(
									sapEncoder.GetBuffer(),
									sapEncoder.GetLen(),
									SComposeTransfer(this,m_rootMsgId,m_rootServId),
									STransferData(3000, false));      
							}
						}
					}
				}
				else
				{       
					if (nCode != 0)
					{
						m_pParent->SetErrorInfo(strErrInfo);      
					}
					SendToParent(nCode,oBodyEncode.GetBuffer(),oBodyEncode.GetLen()); 
				}

				//response code is always 0
				if ( DoSaveResponseRet(nCurrNode,itrSeq->first,SAP_CODE_SUCC,rNode) )
				{
					return;
				}  
			}
			else
			{
				int nLoopTimes=1;
				map<SequeceIndex,int>::const_iterator seq_loop_itr= m_mapSeqLoopTimes.find(itrSeq->first);
				if(seq_loop_itr!=m_mapSeqLoopTimes.end())
				{
					nLoopTimes=seq_loop_itr->second;
				}
				if(nLoopTimes<=0)
				{
					nLoopTimes=1;
				}
				unsigned int dwServiceId = 0, dwMsgId = 0;             
				if (m_pConfig->GetServiceIdByName(rSeq.strServiceName, &dwServiceId, &dwMsgId) != 0)
				{
					SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s service[%s] not found\n",
									__FUNCTION__, rSeq.strServiceName.c_str());

					//build retcode lengh=nLoopTimes-1
					if(nLoopTimes>1)
					{
						m_vecNodeRet.insert(m_vecNodeRet.end(),nLoopTimes-1,
							SSeqRet(itrSeq->first, SAP_CODE_COMPOSE_SERVICE_NOT_FOUND));
					}

					if(DoSaveResponseRet(nCurrNode,itrSeq->first,SAP_CODE_COMPOSE_SERVICE_NOT_FOUND,rNode))
					{
						return;
					}
					continue;
				}

				for(int currentLoop=0;currentLoop<nLoopTimes;++currentLoop)
				{			
					CAvenueMsgHandler oBodyEncode(rSeq.strServiceName,true,m_pConfig);
					SServiceLocation sLoc;
					sLoc.msgId = dwMsgId;
					sLoc.srvId = dwServiceId;
					sLoc.nNode = nCurrNode;
					sLoc.nSeq = itrSeq->first;
					sLoc.srvName = rSeq.strServiceName;     
					sLoc.nodeName=rNode.strNodeName;
					 sLoc.seqName=rSeq.strSequenceName;
					if(nLoopTimes==1)
						BuildPacketBody(rSeq.strServiceName, rSeq.vecField, oBodyEncode,true, sLoc.reqParas);
					else
						BuildPacketBody(rSeq.strServiceName, rSeq.vecField, oBodyEncode,true, sLoc.reqParas,currentLoop);


					oRequest.Reset();
					oRequest.SetService(SAP_PACKET_REQUEST, dwServiceId, dwMsgId);
					oRequest.SetSequence(ntohl(++m_dwServiceSeq));
					SEXHeadBufAndLen exHeadBuf = m_pExHead->getExhead();
					oRequest.SetExtHeader(exHeadBuf.buffer, exHeadBuf.len);
					oRequest.SetBody(oBodyEncode.GetBuffer(), oBodyEncode.GetLen()); 
					if (m_pManager->IsVirtualService(dwServiceId))
					{
						dump_buffer(oRequest.GetBuffer(), E_VIRTUAL_IN,rSeq.strServiceName, nCurrNode);
		                
						void *pOutPacket = NULL;
						int nOutLen = 0, nError = 0, nCode = 0;
						if((nError=m_pManager->OnRequestVirtuelService(dwServiceId,oRequest.GetBuffer(),
							oRequest.GetLen(),&pOutPacket, &nOutLen))!=0)
						{	
							nCode = nError==-2? SAP_CODE_VIRTUAL_SERVICE_NOT_FOUND
												: SAP_CODE_VIRTUAL_SERVICE_BAD_REQUEST;
						}
						else
						{                       
							dump_buffer(pOutPacket, E_VIRTUAL_OUT, rSeq.strServiceName,nCurrNode);
							SSapMsgHeader *pHead=(SSapMsgHeader *)pOutPacket;
							nCode = ntohl(pHead->dwCode); 
							if(nOutLen > pHead->byHeadLen)
							{
								CAvenueMsgHandler *pResHandler = new CAvenueMsgHandler(rSeq.strServiceName,false,m_pConfig);                        
								AddOrUpdateMsg(m_mapResponse, SNodeSeq(nCurrNode,itrSeq->first), SResponseRet(nCode, pResHandler));  
								int nValidatorCode=0;
								if ((nValidatorCode=DecodeMsgBody(pOutPacket, nOutLen,pResHandler)) != 0)
								{
									if(DoSaveResponseRet(nCurrNode,itrSeq->first,nValidatorCode,rNode))
									{ 
										return;
									}
									continue;
								}
								pResHandler->GetMsgParam(sLoc.rspParas);
							}
		                          
						}
						RecordSyncRequest(sLoc, nCode);

						if(DoSaveResponseRet(nCurrNode,itrSeq->first,nCode,rNode))
						{
							return;
						}
					}
					else if (m_pManager->isInAsyncService(dwServiceId))
					{
						CAVSSession* pAvs = m_pManager->OnGetAvs();
						if (pAvs)
						{
							m_mapService.insert( make_pair(m_dwServiceSeq, sLoc) );
							dump_buffer(oRequest.GetBuffer(), E_AVS_IN,rSeq.strServiceName, nCurrNode);
							int nError = pAvs->OnTransferAVSRequest(
								oRequest.GetBuffer(),
								oRequest.GetLen(),
								SComposeTransfer(this,m_rootMsgId,m_rootServId),
								STransferData(rSeq.nTimeout));
							if (nError != 0)
							{
								if(DoSaveResponseRet(nCurrNode,itrSeq->first,nError,rNode))
        						{
        							return;
        						}
							}
						}
		                                
					}
					else if (m_pManager->isInAsyncVirtualClient(dwServiceId))
					{
						CVirtualClientSession* pVirtualClient = m_pManager->OnGetVirtualClientSession();
						if (pVirtualClient)
						{
							m_mapService.insert( make_pair(m_dwServiceSeq, sLoc) );
							dump_buffer(oRequest.GetBuffer(), E_AVS_IN,rSeq.strServiceName, nCurrNode);
							int nError = pVirtualClient->OnTransferAVSRequest(
								oRequest.GetBuffer(),
								oRequest.GetLen(),
								SComposeTransfer(this,m_rootMsgId,m_rootServId),
								STransferData(rSeq.nTimeout));
							if (nError != 0)
							{
								if(DoSaveResponseRet(nCurrNode,itrSeq->first,nError,rNode))
        						{
        							return;
        						}
							}
						}            
					}
					else
					{  
						bool isSosCfg = false;
						string strSosAddr;
						ITransferObj* pSession=NULL;
						map<SequeceIndex,string>::const_iterator sos_addr_itr=m_mapSeqSpSosAddr.find(itrSeq->first);
						if(sos_addr_itr!=m_mapSeqSpSosAddr.end())
						{
							strSosAddr=sos_addr_itr->second;
							pSession  = m_pManager->OnGetSoc(strSosAddr );//deliver msg
							if (!pSession)
							{
								if(DoSaveResponseRet(nCurrNode,itrSeq->first,SAP_CODE_NOT_FOUND,rNode))
								{
									return;
								}
								continue;
							}
						}
						else
						{
							pSession = m_pManager->OnGetSos(dwServiceId,dwMsgId,&isSosCfg);
						}						
						if(NULL == pSession)
						{
							sLoc.srvAddr = "127.0.0.1";
							int nCode =0;
							SReturnMsgData returnMsg;
							if (isSosCfg || 
								(nCode=RequestSubCompose(oRequest.GetBuffer(),oRequest.GetLen(), sLoc, returnMsg)) != 0)
							{
								if (rSeq.bAckMsg)
								{
									int nExLen = oRequest.GetLen() + sizeof(SForwardMsg);
									SForwardMsg *pForwardMsg = (SForwardMsg*)CMapMemory::Instance().Allocate(nExLen);
									SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s alloc map memory[%x] len[%d] serviceId[%d] msgId[%d]\n",
										__FUNCTION__, pForwardMsg, nExLen, dwServiceId, dwMsgId);
									if (pForwardMsg != NULL)
									{
										pForwardMsg->dwComposeSrvId = m_oCSUnit.sKey.dwServiceId;
										pForwardMsg->dwComposeMsgId = m_oCSUnit.sKey.dwMsgId;
										pForwardMsg->dwOriginalSeq  = m_dwSrcSeq;								
										memcpy(pForwardMsg->pBuffer, oRequest.GetBuffer(), oRequest.GetLen());                                
										m_pManager->DoAckRequestEnqueue(pForwardMsg, nExLen);
										nCode = SAP_CODE_SUCC;
									}
									else
									{
										SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s alloc map memory failed, len[%d]  serviceId[%d] msgId[%d]\n",
											__FUNCTION__, nExLen, dwServiceId, dwMsgId);
									}
								}                        
		                		
								RecordSyncRequest(sLoc, nCode);
								if(DoSaveResponseRet(nCurrNode,itrSeq->first,nCode,rNode))
								{
									return;
								}
							}
						}
						else
						{                    
							sLoc.srvAddr = pSession->Addr();
							m_mapService.insert( make_pair(m_dwServiceSeq, sLoc) );
							pSession->OnTransferRequest(
								oRequest.GetBuffer(),
								oRequest.GetLen(),
								SComposeTransfer(this,m_rootMsgId,m_rootServId),
								STransferData(rSeq.nTimeout,rSeq.bAckMsg));                
						} 
					}
				}
			}
		}
    }  
}

bool CComposeServiceObj::DoSaveResponseRet(int nNode, int nSeq,int nCode, const SComposeNode& rNode)
{    
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,this[%x],node[%s],seq[%s],code[%d]\n",
        __FUNCTION__,this, getNodeName(m_oCSUnit, nNode), getSequenceName(m_oCSUnit, nNode, nSeq), nCode);        
    m_vecNodeRet.push_back(SSeqRet(nSeq, nCode));
	map<int,vector<int> >::iterator loopItr=m_mapNodeLoopRet.find(nSeq);
	if(loopItr==m_mapNodeLoopRet.end())
	{
		vector<int> vecLoopRet;
		vecLoopRet.push_back(nCode);
		m_mapNodeLoopRet.insert(make_pair(nSeq,vecLoopRet));
	}
	else
	{
		loopItr->second.push_back(nCode);
	}
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,this[%x],node[%s],seq[%s],code[%d] nodeRetSize[%d] nodeSeqSize[%d]\n",
		__FUNCTION__,this, getNodeName(m_oCSUnit, nNode), getSequenceName(m_oCSUnit, nNode, nSeq), nCode,m_vecNodeRet.size(),m_nTotalSeqSize);  
    if(m_vecNodeRet.size() >= m_nTotalSeqSize)
    {
        DoSaveResponseDef(rNode);

        m_serviceRet.nNode = nNode;
        int nLastCode = nCode;
        int nSeqMin = SEQUENCE_MAX;
        for(vector<SSeqRet>::iterator itr=m_vecNodeRet.begin(); itr!=m_vecNodeRet.end(); ++itr)
        {                       
            if ((itr->nCode < 0 ||itr->nCode > OLDSERVICE_ERRORCODE_RANGE) && itr->nSeq < nSeqMin)
            {                     
                nSeqMin = itr->nSeq;
                nLastCode = itr->nCode;                
            }
        }
        m_serviceRet.nSeq=  nSeqMin == SEQUENCE_MAX? nSeq: nSeqMin;

        if (nLastCode != 0)
        {
            string nodeName=rNode.strNodeName;
	     string seqName;
	     map<SequeceIndex, SComposeSequence>::const_iterator itrSeq=rNode.mapSeqs.find(nSeq);
		if(itrSeq!=rNode.mapSeqs.end())
		{
			seqName=itrSeq->second.strSequenceName;
		}
		
            char error_str[128] = {0};
            snprintf(error_str, sizeof(error_str), "[%.16s,%.8s]", nodeName.c_str(), seqName.c_str());
            if (!m_errInfo.empty())
            {
                m_errInfo.append("-");
            }
            m_errInfo.append(error_str);
        }

         DoStartNodeService(FindGotoNode(rNode.vecResult),nLastCode);
        // TODO::
        //DoStartNodeService(rNode.GetGotoNode(/*process variable*/),nLastCode); 
        /*if( m_nodeTrail.insert(nGotoNode).second )
        {          
            DoStartNodeService(nGotoNode,nLastCode); 
        }
        else
        { 
            DoStartNodeService(m_oCSUnit.nEndNode,SAP_CODE_COMPOSE_GOTO_ERROR);
        }*/
        return true;
    }
    return false;
}

string GetTargertKey(const string & strTargertKey)
{
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s\n", __FUNCTION__);
	if(boost::icontains(strTargertKey,"."))
	{
		vector<string> vecSubParams;
		boost::split(vecSubParams,strTargertKey,boost::algorithm::is_any_of("."),boost::algorithm::token_compress_on);
		if (vecSubParams.size()==0)
		{
			SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s,NULL line: %d \n", __FUNCTION__,__LINE__);
			return "";
		}
		return vecSubParams[0];
	}
	else
	{
		return strTargertKey;
	}
}

bool GetTargertSecond(const string & strTargertKey,string &strSecond)
{
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s]\n", __FUNCTION__);
	if(boost::icontains(strTargertKey,"."))
	{
		vector<string> vecSubParams;
		boost::split(vecSubParams,strTargertKey,boost::algorithm::is_any_of("."),boost::algorithm::token_compress_on);
		if (vecSubParams.size()<2)
		{
			SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, <2 line: %d \n", __FUNCTION__,__LINE__);
			return "";
		}
		strSecond = vecSubParams[1];
		return true;
	}
	else
	{
		return false;
	}
}

bool CComposeServiceObj::IsMetaKey(const string &strTargetKeyS) const
{
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, \n", __FUNCTION__);
	string strTargetKey = GetTargertKey(strTargetKeyS);
	map<string, SDefParameter>::const_iterator it_defParameters = m_oCSUnit.mapDefValue.find(strTargetKey);
	if ( it_defParameters!=m_oCSUnit.mapDefValue.end() && sdo::commonsdk::MSG_FIELD_META == it_defParameters->second.enType)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, true\n", __FUNCTION__);
		return true;
	}
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, false, %s\n", __FUNCTION__,strTargetKey.c_str());
	return false;
}

void CComposeServiceObj::HandleAfterAssign(const string& strTargetKeyS, SDefValue& def_value)
{
	if (IsMetaKey(strTargetKeyS))
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Meta Found\n", __FUNCTION__);
		string strField = "";
		if (GetTargertSecond(strTargetKeyS,strField))
		{
			//$meta.a = $rsp.this.s1.abc
			//$meta.a = $req.a
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Meta Field\n", __FUNCTION__);
			SValueData value(def_value.enType,def_value.vecValue);
			def_value.sMetaData.SetField(strField,value);
			def_value.enType = sdo::commonsdk::MSG_FIELD_META;
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Meta FieldXX\n", __FUNCTION__);
		}
		else
		{
			//$meta = $rsp.this.s1.abc not support
			//$meta = $req.a not support
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, $meta = $rsp.this.s1.abc||$meta = $req.a\n", __FUNCTION__);
			if (def_value.enType == sdo::commonsdk::MSG_FIELD_STRING && def_value.vecValue.size()>0)
			{
				const char*pszValue = (const char*)def_value.vecValue[0].pValue;
				string strValue;
				if (def_value.vecValue[0].nLen>0)
				{
					strValue.assign(pszValue,def_value.vecValue[0].nLen);
				}
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d] size[%d]\n",__FUNCTION__,__LINE__,strValue.size());
				def_value.sMetaData.LoadPHPValue(strValue.c_str());
				
				def_value.enType = sdo::commonsdk::MSG_FIELD_META;
			}	
		}
	}
}

void CComposeServiceObj::DoSaveResponseDef(const SComposeNode& rNode)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, this[%x]\n", __FUNCTION__, this);

    vector<SValueInCompose>::const_iterator itr = rNode.vecResultValues.begin();
    for (; itr != rNode.vecResultValues.end(); ++itr)
    {
		SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, AAA[%d] [%d]\n", __FUNCTION__, itr->enTargetType, itr->enValueType);
	
        if (itr->enTargetType == VTYPE_XHEAD)
        {
            DoSaveResponseXhead(*itr);
            continue;
        }

        if (itr->enValueType == VTYPE_RESPONSE)
        {
            RCIterator itrRes = m_mapResponse.find(SNodeSeq(itr->sNode.nIndex,itr->sSeq.nIndex));
            if(itrRes == m_mapResponse.end())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s line[%d] get response failed, target[%s],"\
                    "source[%s] node[%s], seq[%s]\n", __FUNCTION__,__LINE__,
                    itr->strTargetKey.c_str(), itr->strSourceKey.c_str(), itr->sNode.strIndex.c_str(), itr->sSeq.strIndex.c_str());
                continue;
            }
			
			string strTargetKey = GetTargertKey(itr->strTargetKey);
            SDefValue& def_value = _def_values[strTargetKey];
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, defValueenType[%d]\n", __FUNCTION__,  def_value.enType);
            def_value.enSourceType = itr->enValueType;
            def_value.vecValue.clear();
            SResponseRet& sRsp = itrRes->second;
			if (strcmp(itr->strSourceKey.c_str(),"*")==0)
			{
				if (IsMetaKey(strTargetKey))
				{
					string strField = "";
					if (GetTargertSecond(itr->strTargetKey,strField))
					{
						//$meta.a = $rsp.this.s1.* not support
						SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, $meta.a = $rsp.this.s1.* not support\n", __FUNCTION__);
					}
					else
					{
						//$meta = $rsp.this.s1.*
						SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, ssss $meta = $rsp.this.s1.* not support\n", __FUNCTION__);
						
						def_value.sMetaData.php.SetTlvGroup(sRsp.pHandler);
						
						
						/*
						Qmap<string, sdo::commonsdk::SConfigField> &mResponseName = sRsp.pHandler->GetConfigMessage()->oResponseType.mapFieldByName; 
						Qmap<string, sdo::commonsdk::SConfigField>::iterator it_name = mResponseName.begin();
						for(;it_name!=mResponseName.end();it_name++)
						{
							SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, request field name: %s\n", __FUNCTION__,  it_name->first.c_str());
							EValueType        enType;
							vector<sdo::commonsdk::SStructValue> vecValue;
							sRsp.pHandler->GetValues(it_name->first.c_str(), vecValue, enType);
							
							SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, size: %d\n", __FUNCTION__,  vecValue.size());
							
							if (vecValue.size()>0)
							{
								SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, size: %d (1)\n", __FUNCTION__,  vecValue.size());
								SValueData value(enType,vecValue);
								def_value.sMetaData.SetField(it_name->first.c_str(),value);
								SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, size: %d (2)\n", __FUNCTION__,  vecValue.size());
							}
						}
						def_value.enType = sdo::commonsdk::MSG_FIELD_META;
						*/
					}
				}
			}
			else
			{
				sRsp.pHandler->GetValues(itr->strSourceKey.c_str(), def_value.vecValue, def_value.enType);
			}
			
			HandleAfterAssign(itr->strTargetKey,def_value);
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from response value. targetkey[%s] value size[%u] value type[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.vecValue.size(), def_value.enType);
        }
        else if (itr->enValueType == VTYPE_REQUEST)
        {
			string strTargetKey = GetTargertKey(itr->strTargetKey);
            SDefValue& def_value = _def_values[strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.vecValue.clear();
            m_pReqHandler->GetValues(itr->strSourceKey.c_str(), def_value.vecValue, def_value.enType);
			
			HandleAfterAssign(itr->strTargetKey,def_value);
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from request value. targetkey[%s] value size[%u] value type[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.vecValue.size(), def_value.enType);
        }
        else if (itr->enValueType == VTYPE_DEF_VALUE)
        {
			string strTargetKey = GetTargertKey(itr->strTargetKey);
			string strSourceKey = GetTargertKey(itr->strSourceKey);
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s >>>>>>>>>>>>>>>>>>>>>>., source: %s targert: %s\n",__FUNCTION__,strSourceKey.c_str(),strTargetKey.c_str());
            map<string, SDefValue>::iterator itr_def_value = _def_values.find(strSourceKey);
			
			if (itr_def_value == _def_values.end())			
			{
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s not found source key %s\n",__FUNCTION__,strSourceKey.c_str());
				map<string, SDefParameter>::const_iterator itr_param = m_oCSUnit.mapDefValue.find(strSourceKey);
				if (itr_param == m_oCSUnit.mapDefValue.end() || !itr_param->second.bHasDefaultValue)
				{
					SS_XLOG(XLOG_DEBUG, "%s, the def value has no default value. def value[%s]\n", __FUNCTION__, strSourceKey.c_str());
				}
				else if (itr_param->second.enType == sdo::commonsdk::MSG_FIELD_INT)
				{
					SDefValue& df = _def_values[strSourceKey];
					df.enSourceType = VTYPE_LASTCODE;
					df.enType = itr_param->second.enType;
					df.nLastCode = itr_param->second.nDefaultValue;
					itr_def_value = _def_values.find(strSourceKey);
				}
				else if (itr_param->second.enType == sdo::commonsdk::MSG_FIELD_STRING)
				{
					SDefValue& df = _def_values[strSourceKey];
					df.enType = itr_param->second.enType;
					sdo::commonsdk::SStructValue ssv;
					ssv.pValue = (void*)itr_param->second.strDefaultValue.c_str();
					ssv.nLen = itr_param->second.strDefaultValue.size();
					df.vecValue.push_back(ssv);
					itr_def_value = _def_values.find(strSourceKey);
				}
			}
			if (itr_def_value != _def_values.end())		 
            {
				SDefValue& sourceValue = itr_def_value->second;
				if (IsMetaKey(itr->strSourceKey))		// $* = $meta
				{
					string strField;
					if (GetTargertSecond(itr->strSourceKey,strField))  
					{
						if (IsMetaKey(itr->strTargetKey))   
						{
							string strTargertField;
							if (GetTargertSecond(itr->strTargetKey,strTargertField))
							{
								//$meta.b = $meta.a
								_def_values[strTargetKey].LoadValueFromMetaField(sourceValue,strField,strTargertField);		
							}
						    else
							{
								//$meta = $meta.a not support
								SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, $meta = $meta.a not support\n",__FUNCTION__);
							}
						}
						else    
						{
							////$def = $meta.a
							_def_values[strTargetKey].LoadValueFromMetaField(sourceValue,strField);			
							SS_XLOG(XLOG_DEBUG,
							"CComposeServiceObj::%s, targert:%s  strField:%s line[%d]\n",
							__FUNCTION__,strTargetKey.c_str(),strField.c_str(),__LINE__);
						}
					}
					else   
					{
						string strTargertField;
						if (GetTargertSecond(itr->strTargetKey,strTargertField))
						{
							//$meta.a = $meta not support
							SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, $meta.a = $meta not support\n",__FUNCTION__);
						}
						else
						{
							if (_def_values[strTargetKey].enType!=sdo::commonsdk::MSG_FIELD_META)
							{
								//$def = $meta
								SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n",__FUNCTION__,__LINE__);
								 SValueData &valueData = sourceValue.sMetaData.GetPHPValue();
								 _def_values[strTargetKey].enType = valueData.enType;
								 _def_values[strTargetKey].vecValue = valueData.vecValue;
							}
							else
							{
								//$meta = $meta
								_def_values[strTargetKey] = sourceValue;
							}
						}
					}
				}
				else								//$* = $def
				{
					if (IsMetaKey(itr->strTargetKey))  
					{
						string strTargertField;
						if (GetTargertSecond(itr->strTargetKey,strTargertField))
						{
							//$meta.b = $def
							SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n",__FUNCTION__,__LINE__);
							_def_values[strTargetKey].SaveValueToMetaField(sourceValue,strTargertField);
						}
						else
						{
							//$meta = $def 
							if (sourceValue.vecValue.size()>0)
							{
								const char*pszValue = (const char*)sourceValue.vecValue[0].pValue;
								string strValue;
								if (sourceValue.vecValue[0].nLen>0)
								{
									strValue.assign(pszValue,sourceValue.vecValue[0].nLen);
								}
								SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d] size[%d]\n",__FUNCTION__,__LINE__,strValue.size());
								_def_values[strTargetKey].sMetaData.LoadPHPValue(strValue.c_str());
								_def_values[strTargetKey].enType = sdo::commonsdk::MSG_FIELD_META;
							}
							
						}
					}
					else
					{	
						//$def = $def
						_def_values[strTargetKey] = sourceValue;	
					}
				}					
                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from def value. targetkey[%s] value size[%u] value type[%d]\n",
                    __FUNCTION__, itr->strTargetKey.c_str(), itr_def_value->second.vecValue.size(), itr_def_value->second.enType);
            }

			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s <<<<<<<<<<<<<<< source: %s targert: %s\n",__FUNCTION__,strSourceKey.c_str(),strTargetKey.c_str());
        }
        else if (itr->enValueType == VTYPE_NOCASE_VALUE)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            map<string, SDefParameter>::const_iterator itr_defvalue = m_oCSUnit.mapDefValue.find(itr->strTargetKey);
            if (itr_defvalue != m_oCSUnit.mapDefValue.end() &&
                itr_defvalue->second.enType == sdo::commonsdk::MSG_FIELD_INT)
            {
                def_value.enSourceType = VTYPE_LASTCODE;
                def_value.nLastCode    = itr->strSourceKey.size() > 0 ? atoi(itr->strSourceKey.c_str()) : 0;
                def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;
				
				 SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from no case value. targetkey[%s] value[%s] MSG_FIELD_INT\n",
                __FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
            }
            else
            {
                def_value.vecValue.clear();
                def_value.vecValue.push_back(SStructValue());
                def_value.enSourceType = itr->enValueType;
                def_value.enType       = sdo::commonsdk::MSG_FIELD_STRING;
                SStructValue& value_struct = def_value.vecValue[0];
                value_struct.pValue    = (void*)itr->strSourceKey.c_str();
                value_struct.nLen      = itr->strSourceKey.size();
				
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from no case value. targetkey[%s] value[%s] MSG_FIELD_STRING\n",
                __FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
            }

           
        }
        else if (itr->enValueType == VTYPE_LASTCODE)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.nLastCode    = m_last_code;
            def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;

            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from last code. targetkey[%s] value[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.nLastCode);
        }
        else if (itr->enValueType == VTYPE_REMOTEIP)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.strRemoteIp  = m_strRemoteIp;
            def_value.enType       = sdo::commonsdk::MSG_FIELD_STRING;

            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from remote ip. targetkey[%s] value[%s]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.strRemoteIp.c_str());
        }
        else if (itr->enValueType == VTYPE_RETURN_CODE)
        {
            const int* code = GetNodeCode(m_vecNodeRet, itr->sSeq.nIndex);
            if (code)
            {
                SDefValue& def_value   = _def_values[itr->strTargetKey];
                def_value.enSourceType = VTYPE_LASTCODE;
                def_value.nLastCode    = *code;
                def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;

                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from code. targetkey[%s] value[%d]\n",
                    __FUNCTION__, itr->strTargetKey.c_str(), def_value.nLastCode);
            }
		}
		else  if ((itr->enValueType == VTYPE_BUILTINFUNCTION) && (strcasecmp(itr->strSourceKey.c_str(),"ExcuteStateMachine")==0)) //ExcuteStateMachine.
		{
			SServiceLocation sLoc;
			sLoc.msgId = 0;
			sLoc.srvId = 0;
			sLoc.nNode = 0;
			sLoc.nSeq = 0;
			sLoc.srvName = GetFunctionFullSourcekey(itr->enValueType,itr->strSourceKey);
			string strMachineName;
			int nState;
			int nEvent;
			int bResult= -1;
			if (itr->sFunctionParams.size()!=3)
			{
				bResult = SAP_CODE_FUNCALLERROR;
			}
			else
			{
				inst_value instValue1 = GetFunctionParamValue(itr->sFunctionParams[0]);
				inst_value instValue2 = GetFunctionParamValue(itr->sFunctionParams[1]);
				inst_value instValue3 = GetFunctionParamValue(itr->sFunctionParams[2]);
				if (instValue1.m_type!=eiv_string||instValue2.m_type!=eiv_int||instValue3.m_type!=eiv_int)
				{
					bResult = SAP_CODE_FUNCALLERROR;
				}
				else
				{
					strMachineName = instValue1.to_string();
					nState = instValue2.m_int_value;
					nEvent = instValue3.m_int_value;
				}
			}
			BuildStateMachineFunctionLogRequest(strMachineName,nState,nEvent,sLoc);
			map<string, SDefParameter>::const_iterator it_para = m_oCSUnit.mapDefValue.find(itr->strTargetKey);
			if (it_para != m_oCSUnit.mapDefValue.end())
			{
				if (it_para->second.enType != sdo::commonsdk::MSG_FIELD_INT)
				{
					bResult = SAP_CODE_FUNCALLERROR;
				}
				else
				{
					nState = CStateMachine::Instance()->ExcuteStateMachine(strMachineName, nState, nEvent); 
					bResult = 0;
				}
			}
			else
			{
				SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error miss def type when calling lua function\n",__FUNCTION__);
			}
			if(bResult==0)
			{
				BuildStateMachineFunctionLogResponse(nState,sLoc);
				RecordSyncRequest(sLoc, bResult);
				SDefValue& def_value   = _def_values[itr->strTargetKey];
				def_value.vecValue.clear();
				def_value.enSourceType =VTYPE_LASTCODE;
				def_value.nLastCode = nState;
				def_value.enType= sdo::commonsdk::MSG_FIELD_INT;
				
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, %s=%d \n",__FUNCTION__, itr->strTargetKey.c_str(), nState);
					
			}
			else
			{
				RecordSyncRequest(sLoc, bResult);
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result. targetkey[%s] funcstr[%s]\n",
					__FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
			}
		}
		else  if ((itr->enValueType == VTYPE_BUILTINFUNCTION) && (strcasecmp(itr->strSourceKey.c_str(),"GetMachineStateName")==0)) //GetMachineStateName.
		{
			SServiceLocation sLoc;
			sLoc.msgId = 0;
			sLoc.srvId = 0;
			sLoc.nNode = 0;
			sLoc.nSeq = 0;
			sLoc.srvName = GetFunctionFullSourcekey(itr->enValueType,itr->strSourceKey);
			string strMachineName;
			int nState;
			int nEvent;
			string strStateName;
			int bResult= -1;
			if (itr->sFunctionParams.size()!=2)
			{
				bResult = SAP_CODE_FUNCALLERROR;
				SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, builtin para- error\n",__FUNCTION__);
			}
			else
			{
				inst_value instValue1 = GetFunctionParamValue(itr->sFunctionParams[0]);
				inst_value instValue2 = GetFunctionParamValue(itr->sFunctionParams[1]);
				if (instValue1.m_type!=eiv_string||instValue2.m_type!=eiv_int)
				{
					bResult = SAP_CODE_FUNCALLERROR;
					SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, builtin input- error\n",__FUNCTION__);
				}
				else
				{
					strMachineName = instValue1.to_string();
					nState = instValue2.m_int_value;
					nEvent = 0;
				}
			}
			BuildStateMachineFunctionLogRequest(strMachineName,nState,nEvent,sLoc);
			map<string, SDefParameter>::const_iterator it_para = m_oCSUnit.mapDefValue.find(itr->strTargetKey);
			if (it_para != m_oCSUnit.mapDefValue.end())
			{
				strStateName = CStateMachine::Instance()->GetMachineStateName(strMachineName, nState); 
				bResult = 0;
			}
			else
			{
				SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error miss def type when calling builtin function\n",__FUNCTION__);
			}
			if(bResult==0)
			{
				BuildStateMachineFunctionLogResponse(strStateName,sLoc);
				RecordSyncRequest(sLoc, bResult);
				SDefValue& def_value   = _def_values[itr->strTargetKey];
				
				def_value.enSourceType =itr->enValueType;
				def_value.vecValue.clear();
				def_value.vecValue.push_back(SStructValue());
				def_value.enType       = sdo::commonsdk::MSG_FIELD_STRING;
				(string&)(itr->sFunctionRet) = strStateName;
				string & sRetString = (string&)itr->sFunctionRet;
				SStructValue& value_struct = def_value.vecValue[0];
				value_struct.pValue    = (void*)itr->sFunctionRet.c_str();
				value_struct.nLen      = itr->sFunctionRet.size();
			}
			else
			{
				RecordSyncRequest(sLoc, bResult);
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result. targetkey[%s] funcstr[%s]\n",
					__FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
			}
		}
		else if (itr->enValueType == VTYPE_FUNCTION || itr->enValueType == VTYPE_LUAFUNCTION)
		{
			SServiceLocation sLoc;
			sLoc.msgId = 0;
			sLoc.srvId = 0;
			sLoc.nNode = 0;
			sLoc.nSeq = 0;
			sLoc.srvName = GetFunctionFullSourcekey(itr->enValueType,itr->strSourceKey);
			vector<inst_value> stk;
			for(vector<SFunctionParam>::const_iterator itrFuncParam=itr->sFunctionParams.begin();
			itrFuncParam!=itr->sFunctionParams.end();++itrFuncParam)
			{
				stk.push_back(GetFunctionParamValue(*itrFuncParam));
			}
			BuildFunctionLogRequest(stk,sLoc);
			
			int bResult= -1;
			if (itr->enValueType == VTYPE_FUNCTION)
				bResult = extern_fun_manager::GetInstance()->call(itr->strSourceKey,stk.size(),stk,0);
			else if (itr->enValueType == VTYPE_LUAFUNCTION)
			{
				map<string, SDefParameter>::const_iterator it_para = m_oCSUnit.mapDefValue.find(itr->strTargetKey);
				if (it_para != m_oCSUnit.mapDefValue.end())
				{
					bResult = extern_fun_lua_manager::GetInstance()->call(itr->strSourceKey,stk.size(),stk,0,it_para->second.enType);
				}
				else
				{
					SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error miss def type when calling lua function\n",__FUNCTION__);
				}
			}
			if(bResult==0)
			{
				BuildFunctionLogResponse(stk,sLoc);
				RecordSyncRequest(sLoc, bResult);
				SDefValue& def_value   = _def_values[itr->strTargetKey];
				if(stk[0].m_type==eiv_int)
				{
					def_value.enSourceType =VTYPE_LASTCODE;
					def_value.nLastCode=stk[0].to_int();
					def_value.enType= sdo::commonsdk::MSG_FIELD_INT;
				}
				else
				{
					def_value.enSourceType =itr->enValueType;
					def_value.vecValue.clear();
					def_value.vecValue.push_back(SStructValue());
					def_value.enType       = sdo::commonsdk::MSG_FIELD_STRING;
					string & sRetString = (string&)itr->sFunctionRet;
					sRetString = stk[0].to_string();
					SStructValue& value_struct = def_value.vecValue[0];
					value_struct.pValue    = (void*)itr->sFunctionRet.c_str();
					value_struct.nLen      = itr->sFunctionRet.size();
				}
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, execCppFunc Set def string value which is from function result. targetkey[%s] funcstr[%s] value[%s]\n",
					__FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str(),stk[0].to_string().c_str());
			}
			else
			{
				RecordSyncRequest(sLoc, bResult);
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result. targetkey[%s] funcstr[%s]\n",
					__FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
			}
		}
		else if (itr->enValueType == VTYPE_LOOP_RETURN_CODE)
		{
			map<int,vector<int> >::const_iterator loopCodeItr = m_mapNodeLoopRet.find(itr->sSeq.nIndex);
			if(loopCodeItr!=m_mapNodeLoopRet.end())
			{
					SDefValue& def_value   = _def_values[itr->strTargetKey];
					def_value.enSourceType = itr->enValueType;
					def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;
					def_value.vecLoopRetCode=loopCodeItr->second;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from loopcode. targetkey[%s]  value size[%u] value type[%d]\n",
						__FUNCTION__, itr->strTargetKey.c_str(), def_value.vecLoopRetCode.size(), def_value.enType);
			}
		}
        else
        {
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, unknow value type. It's not a def value. source key[%s], target key[%s]\n",
                __FUNCTION__, itr->strSourceKey.c_str(), itr->strTargetKey.c_str());
            continue;
        }
    }
}

string CComposeServiceObj::GetFunctionFullSourcekey(int type, const string& strSourcekey)
{
	if (type == VTYPE_FUNCTION)
	{
		return "cpp_function."+strSourcekey;
	}
	else if (type == VTYPE_LUAFUNCTION)
	{
		return "lua_function."+strSourcekey;
	}
	else if (type == VTYPE_BUILTINFUNCTION)
	{
		return "builtin_function."+strSourcekey;
	}
	return strSourcekey;
}

void CComposeServiceObj::DoSaveResponseXhead(const SValueInCompose& value_in_compose)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, value_node[%s][%d], value_seq[%s][%d]\n",
        __FUNCTION__, value_in_compose.sNode.strIndex.c_str(), value_in_compose.sNode.nIndex, value_in_compose.sSeq.strIndex.c_str(), value_in_compose.sSeq.nIndex);

    int* appid  = NULL;
    int* areaid = NULL;
    const char* name = NULL;
    const char* socid = NULL;
    int id = -1;
    string strName;
	int nEnType;
    if (value_in_compose.strTargetKey.find("appid") == 0)
    {
        appid = &id;
		nEnType = eiv_int;
    }
    else if (value_in_compose.strTargetKey.find("areaid") == 0)
    {
        areaid = &id;
		nEnType = eiv_int;
    }
    else if (value_in_compose.strTargetKey.find("name") == 0)
    {
        name = strName.c_str();
		nEnType = eiv_string;
    }
    else if (value_in_compose.strTargetKey.find("socid") == 0)
    {
        socid = strName.c_str();
		nEnType = eiv_string;
    }
    else
    {
        SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Update xhead faild. target key[%s]\n",
            __FUNCTION__, value_in_compose.strTargetKey.c_str());
        return;
    }

    if (value_in_compose.enValueType  == VTYPE_RESPONSE)
    {
        RCIterator itrRes = m_mapResponse.find(SNodeSeq(value_in_compose.sNode.nIndex,value_in_compose.sSeq.nIndex));
        if(itrRes != m_mapResponse.end())
        {
            SDefValue def_value;
            SResponseRet& sRsp = itrRes->second;
            sRsp.pHandler->GetValues(value_in_compose.strSourceKey.c_str(), def_value.vecValue, def_value.enType);
            if (def_value.vecValue.size() > 0)
            {
                if (appid || areaid)
                {
                    id = *(int*)def_value.vecValue[0].pValue;
                }
                else
                {
                    strName.assign((char*)def_value.vecValue[0].pValue, def_value.vecValue[0].nLen);
                }
            }
        }
    }
    else if (value_in_compose.enValueType == VTYPE_REQUEST)
    {
       SDefValue  def_value;
       m_pReqHandler->GetValues(value_in_compose.strSourceKey.c_str(), def_value.vecValue, def_value.enType);
       if (def_value.vecValue.size() > 0)
       {
            if (appid || areaid)
            {
                id = *(int*)def_value.vecValue[0].pValue;
            }
            else
            {
               strName.assign((char*)def_value.vecValue[0].pValue, def_value.vecValue[0].nLen);
            }
       }
    }
    else if (value_in_compose.enValueType == VTYPE_DEF_VALUE)
    {
        map<string, SDefValue>::iterator itr = _def_values.find(value_in_compose.strSourceKey);
        if (itr != _def_values.end() && !itr->second.vecValue.empty())
        {
            if (appid || areaid)
            {
                id = *(int*)itr->second.vecValue[0].pValue;
            }
            else
            {
                strName.assign((char*)itr->second.vecValue[0].pValue, itr->second.vecValue[0].nLen);
            }
        }
    }
    else if (value_in_compose.enValueType == VTYPE_NOCASE_VALUE)
    {
        if (appid || areaid)
        {
            id = atoi(value_in_compose.strSourceKey.c_str());
        }
        else
        {
            strName = value_in_compose.strSourceKey;
        }
	}
	else if (value_in_compose.enValueType == VTYPE_FUNCTION || value_in_compose.enValueType == VTYPE_LUAFUNCTION)
	{
		SServiceLocation sLoc;
		sLoc.msgId = 0;
		sLoc.srvId = 0;
		sLoc.nNode = 0;
		sLoc.nSeq = 0;
		sLoc.srvName = GetFunctionFullSourcekey(value_in_compose.enValueType,value_in_compose.strSourceKey);
		vector<inst_value> stk;
		for(vector<SFunctionParam>::const_iterator itrFuncParam=value_in_compose.sFunctionParams.begin();
			itrFuncParam!=value_in_compose.sFunctionParams.end();++itrFuncParam)
		{
			stk.push_back(GetFunctionParamValue(*itrFuncParam));
		}
		BuildFunctionLogRequest(stk,sLoc);
		int bResult=-1;
		if (value_in_compose.enValueType == VTYPE_FUNCTION)
		{
			bResult = extern_fun_manager::GetInstance()->call(value_in_compose.strSourceKey,stk.size(),stk,0);
		}
		else if (value_in_compose.enValueType == VTYPE_LUAFUNCTION)
		{
			bResult = extern_fun_lua_manager::GetInstance()->call(value_in_compose.strSourceKey,stk.size(),stk,0,nEnType);
		}
		
		if(bResult==0)
		{
			BuildFunctionLogResponse(stk,sLoc);
			RecordSyncRequest(sLoc, bResult);
			if ((appid || areaid)&&stk[0].m_type==eiv_int)
			{
				id=stk[0].to_int();
			}
			else
			{
				strName=stk[0].to_string();
			}
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, execCppFunc Set def string value which is from function result.target[%s],  funcstr[%s] value[%s]\n",
				__FUNCTION__, value_in_compose.strTargetKey.c_str(),value_in_compose.strSourceKey.c_str(),stk[0].to_string().c_str());
		}
		else
		{
			RecordSyncRequest(sLoc, bResult);
			SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result.target[%s],  funcstr[%s]\n",
				__FUNCTION__, value_in_compose.strTargetKey.c_str(),value_in_compose.strSourceKey.c_str());
		}
	}
    else
    {
        SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, unknow value type. target[%s], source[%s], type[%d], node[%s][%d], seq[%s][%d], isdef[%s] .\n",
            __FUNCTION__, value_in_compose.strTargetKey.c_str(), value_in_compose.strSourceKey.c_str(), value_in_compose.enValueType,
            value_in_compose.sNode.strIndex.c_str(), value_in_compose.sNode.nIndex, value_in_compose.sSeq.strIndex.c_str(), value_in_compose.sSeq.nIndex);
        return;
    }

    if (appid)
    {
        m_pExHead->updateAppId(*appid);        
    }
    if (areaid)
    {
        m_pExHead->updateAreaId(*areaid);
    }
    if (name)
    {
        m_pExHead->updateName(strName);
    }
    if (socid)
    {
        m_pExHead->updateSocId(strName);
    }
}

void CComposeServiceObj::OnReceiveResponse(const void *pBuffer, int nLen, bool bToParent, const timeval_a& timeStart)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, pBuffer[%x], len[%d]\n",__FUNCTION__, pBuffer, nLen);
    
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;   
    SrvLocContainer::iterator itr = m_mapService.find(pHead->dwSequence);
    if(itr != m_mapService.end())
    {         
        SServiceLocation& sLocation = itr->second;         
        sLocation.nCode = static_cast<int>(ntohl(pHead->dwCode)); 
       
        //save response
        //CAvenueMsgHandler *pResHandler = NULL;
        if(nLen > pHead->byHeadLen)
        {
            CAvenueMsgHandler *pResHandler = new CAvenueMsgHandler(sLocation.srvName,false,m_pConfig);
		int nValidatorCode=0;
            if((nValidatorCode= DecodeMsgBody(pBuffer, nLen, pResHandler))!=0)
            {
                sLocation.nCode = nValidatorCode;               
                SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s decode atom service response failed, node[%s] seq[%s]\n",
				__FUNCTION__, getNodeName(m_oCSUnit, sLocation.nNode), getSequenceName(m_oCSUnit, sLocation.nNode, sLocation.nSeq));
            }
            pResHandler->GetMsgParam(sLocation.rspParas);

            AddOrUpdateMsg(m_mapResponse, SNodeSeq(sLocation.nNode,sLocation.nSeq), SResponseRet(sLocation.nCode, pResHandler));                       
        }
        else
        {
             sLocation.rspParas = "[NULL]";
        }
        NodeIterator itrNode =  m_oCSUnit.mapNodes.find(sLocation.nNode);
        const SComposeNode &rNode = itrNode->second;

        if (!bToParent)
        {
			//leaf 
            RecordSequence(sLocation, timeStart,true);
        }
        else if (GetParentObejct()==NULL)
		{
			//sub composed process 
			RecordSequence(sLocation, timeStart,false);
		}
        //if (sLocation.nCode == 0)
        //{
            //DoSaveResponseDef(sLocation.nNode, sLocation.nNodeSeq, pResHandler, rNode);
        //}
        DoSaveResponseRet(sLocation.nNode,sLocation.nSeq,sLocation.nCode,rNode);
    }
    else
    {        
        SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s,can't find route, this[%x],seq[%u]\n",
            __FUNCTION__, this, pHead->dwSequence);
    }
    
}

void CComposeServiceObj::SendToClient(int nCode,const void *pBuffer, int nLen)const
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,code[%d]\n",__FUNCTION__,nCode);
    CSapEncoder oResponse(SAP_PACKET_RESPONSE,m_oCSUnit.sKey.dwServiceId,m_oCSUnit.sKey.dwMsgId,nCode);   
    oResponse.SetSequence(m_dwSrcSeq);
    oResponse.SetBody(pBuffer, nLen);
    m_pBase->WriteComposeData(m_handle,oResponse.GetBuffer(),oResponse.GetLen()); 
}

void CComposeServiceObj::SendToParent(int nCode,const void *pBuffer, int nLen)const
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,code[%d]\n",__FUNCTION__,nCode);
    CSapEncoder oResponse(SAP_PACKET_RESPONSE,m_oCSUnit.sKey.dwServiceId,m_oCSUnit.sKey.dwMsgId,nCode);   
    oResponse.SetSequence(m_dwSrcSeq);
    oResponse.SetBody(pBuffer, nLen);
	dump_buffer(oResponse.GetBuffer(), E_COMPOSE_OUT,m_oCSUnit.strServiceName, 0);
	m_pParent->OnReceiveResponse(oResponse.GetBuffer(),oResponse.GetLen(), true, m_tmStart);
}


/* verify current node*/
bool CComposeServiceObj::DoIsDestroy(int nCurrNode)const
{ 
    if(nCurrNode == Compose_Node_End)
    {
        if (m_pParent == NULL)
        {
            if(m_pBase != NULL)
            {
                m_pBase->OnDeleteComposeService(m_dwIndex);
            }
            else if (m_pManager != NULL)
            {
                m_pManager->OnDecreaseComposeObjNum();
            }
        }

        return true;
    }
    return false;
}

int CComposeServiceObj::DoGetCode(const SReturnCode& rCode, const int nLastCode)const
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s,return type[%d]\n",__FUNCTION__,rCode.enReturnType);
    
    if(rCode.enReturnType == Return_Type_LastNode)
    {
        return nLastCode;
    }

    if(rCode.enReturnType == Return_Type_Code)
    {
        if (!rCode.bIsDefValue)
        {
            return rCode.nReturnCode;
        }

        map<string, SDefValue>::const_iterator itr_def_value = _def_values.find(rCode.strValueName);
        if (itr_def_value != _def_values.end())
        {
            if (itr_def_value->second.enSourceType == VTYPE_LASTCODE)
            {
                return itr_def_value->second.nLastCode;
            }

            if (itr_def_value->second.vecValue.size() > 0)
            {
                if (itr_def_value->second.enType == sdo::commonsdk::MSG_FIELD_STRING )
                {
                    return atoi((const char*)itr_def_value->second.vecValue[0].pValue);
                }
                return *(int*)(itr_def_value->second.vecValue[0].pValue);
            }

            return DEFAULT_RESPONSE_CODE;
        }

        map<string, SDefParameter>::const_iterator itr_def_param = m_oCSUnit.mapDefValue.find(rCode.strValueName);
        if (itr_def_param != m_oCSUnit.mapDefValue.end() && itr_def_param->second.bHasDefaultValue)
        {
            return itr_def_param->second.nDefaultValue;
        }

        return DEFAULT_RESPONSE_CODE;
    }
    RspContainer::const_iterator it = m_mapResponse.find(SNodeSeq(rCode.sNodeId.nIndex, rCode.sSequenceId.nIndex));
    if (it != m_mapResponse.end())
    {
        return it->second.nCode;
    }    
    return DEFAULT_RESPONSE_CODE;
}

/*loading process has guaranteed the validity of goto node, so it is not necessary to check it any more*/
int CComposeServiceObj::FindGotoNode(const vector<SComposeGoto> &vecRet)const
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s,vecRet size[%u]\n", __FUNCTION__, vecRet.size());
    vector<SComposeGoto>::const_iterator itrVec;
    for(itrVec=vecRet.begin(); itrVec!=vecRet.end(); ++itrVec)
    {
        if(IsRuleMatch(*itrVec))
        {
            return itrVec->sTargetNode.nIndex;
        }        
    }    
    return m_oCSUnit.nEndNode;
}

bool CComposeServiceObj::IsRuleMatch(const SComposeGoto& goto_node_)const
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s, condition_param size[%u]\n", __FUNCTION__, goto_node_.vecConditionParam.size());

    vector<ParameterValue> values;
    vector<SValueInCompose>::const_iterator itr_condition_param = goto_node_.vecConditionParam.begin();
    for (; itr_condition_param != goto_node_.vecConditionParam.end(); ++itr_condition_param)
    {
        ParameterValue param_value;
        if (itr_condition_param->enValueType == VTYPE_RESPONSE)
        {
            SNodeSeq node_seq;
            node_seq.nNode = itr_condition_param->sNode.nIndex;
            node_seq.nSeq  = itr_condition_param->sSeq.nIndex;

            RspContainer::const_iterator itr = m_mapResponse.find(node_seq);
            if (itr != m_mapResponse.end())
            {
                SResponseRet* pHandler = const_cast<SResponseRet*>(&(itr->second));
                vector<sdo::commonsdk::SStructValue> values;
                EValueType type;
                pHandler->pHandler->GetValues(itr_condition_param->strSourceKey.c_str(), values, type);
                if (values.empty())
                {
                    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. Can't get value from response. source key[%s]. It will be NULL\n", 
                        __FUNCTION__, itr_condition_param->strSourceKey.c_str());
                }
                else if (type == sdo::commonsdk::MSG_FIELD_INT)
                {
                    param_value.type  = VALUETYPE_INT;
                    param_value.value = values[0].pValue;
                    param_value.len   = sizeof(int);
                }
                else if (type == sdo::commonsdk::MSG_FIELD_STRING)
                {
                    param_value.type  = VALUETYPE_STRING;
                    param_value.value = values[0].pValue;
                    param_value.len   = values[0].nLen;
                }
                else
                {
                    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. Can't use this type value to do comparing operation, source[%s] type[%d] value[%p] len[%d]\n",
                        __FUNCTION__, itr_condition_param->strSourceKey.c_str(), type, values[0].pValue, values[0].nLen);
                    return false;
                }
            }
        }
        else if (itr_condition_param->enValueType == VTYPE_REQUEST)
        {
            vector<sdo::commonsdk::SStructValue> values;
            EValueType type;
            m_pReqHandler->GetValues(itr_condition_param->strSourceKey.c_str(), values, type);
            if (values.empty())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. Can't get value from request. source key[%s]. It will be NULL.\n", 
                    __FUNCTION__, itr_condition_param->strSourceKey.c_str());
            }
            else if (type == sdo::commonsdk::MSG_FIELD_INT)
            {
                param_value.type  = VALUETYPE_INT;
                param_value.value = values[0].pValue;
                param_value.len   = sizeof(int);
            }
            else if (type == sdo::commonsdk::MSG_FIELD_STRING)
            {
                param_value.type  = VALUETYPE_STRING;
                param_value.value = values[0].pValue;
                param_value.len   = values[0].nLen;
            }
            else
            {
                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. Can't use this type value to do comparing operation, source[%s] type[%d] value[%p] len[%d]\n",
                    __FUNCTION__, itr_condition_param->strSourceKey.c_str(), type, values[0].pValue, values[0].nLen);
                return false;
            }
            //SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,param request value[%s]=[%s]\n", __FUNCTION__, itr_condition_param->strSourceKey.c_str(), value_str.c_str());
        }
        else if (itr_condition_param->enValueType == VTYPE_DEF_VALUE)
        {
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. [%s]\n",  __FUNCTION__,itr_condition_param->strSourceKey.c_str());
			string strSourceKey = GetTargertKey(itr_condition_param->strSourceKey);			
            map<string, SDefValue>::const_iterator itr = _def_values.find(strSourceKey);
            if (itr != _def_values.end())
            {
				SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. line[%d]\n",  __FUNCTION__,__LINE__);
			    const SDefValue& def_value = itr->second;
				if (IsMetaKey(itr_condition_param->strSourceKey))   
				{
					SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. line[%d]\n",  __FUNCTION__,__LINE__);
					string strField;
					if (GetTargertSecond(itr_condition_param->strSourceKey,strField))
					{
						SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. line[%d]\n",  __FUNCTION__,__LINE__);
						SValueData &value = ((SDefValue&)(def_value)).sMetaData.GetField(strField);
						SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. line[%d]\n",  __FUNCTION__,__LINE__);
						if (value.vecValue.size()>0)
						{
							SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. line[%d]\n",  __FUNCTION__,__LINE__);
							if (value.enType == sdo::commonsdk::MSG_FIELD_INT)
							{
								param_value.type  = VALUETYPE_INT;
								param_value.value = value.vecValue[0].pValue;
								param_value.len   = sizeof(int);
								SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. value[%d] line[%d]\n",  __FUNCTION__,*(int*)(param_value.value),__LINE__);
							}
							else if (value.enType == sdo::commonsdk::MSG_FIELD_STRING)
							{
								param_value.type  = VALUETYPE_STRING;
								param_value.value = value.vecValue[0].pValue;
								param_value.len   = value.vecValue[0].nLen;
								SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s. line[%d]\n",  __FUNCTION__,__LINE__);
							}
						}
					}
					else
					{
						//not support $meta compare
						SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, $meta compare not support\n",__FUNCTION__);
					}
				}
                else if (def_value.enSourceType == VTYPE_LASTCODE)
                {
                    param_value.type  = VALUETYPE_INT;
                    param_value.value = &def_value.nLastCode;
                    param_value.len   = sizeof(int);
                }
                else if (def_value.enSourceType == VTYPE_REMOTEIP)
                {
                    param_value.type  = VALUETYPE_STRING;
                    param_value.value = m_strRemoteIp.c_str();
                    param_value.len   = m_strRemoteIp.length();
                }
                else
                {
                    if (def_value.vecValue.empty())
                    {
                        SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. Can't get def value. source key[%s]. It will be NULL.\n", 
                            __FUNCTION__, itr_condition_param->strSourceKey.c_str());
                    }
                    else
                    {
                        if (def_value.enType == sdo::commonsdk::MSG_FIELD_INT)
                        {
                            param_value.type  = VALUETYPE_INT;
                            param_value.value = def_value.vecValue[0].pValue;
                            param_value.len   = sizeof(int);
                        }
                        else if (def_value.enType == sdo::commonsdk::MSG_FIELD_STRING)
                        {
                            param_value.type  = VALUETYPE_STRING;
                            param_value.value = def_value.vecValue[0].pValue;
                            param_value.len   = def_value.vecValue[0].nLen;
                        }
                        else
                        {
                            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. Can't use this type def value to do comparing operation, source[%s] type[%d]\n",
                                __FUNCTION__, itr_condition_param->strSourceKey.c_str(), def_value.enType);
                            return false;
                        }
                    }
                }
            }
            else
            {
                map<string, SDefParameter>::const_iterator itr_def_param = m_oCSUnit.mapDefValue.find(strSourceKey);
                if (itr_def_param != m_oCSUnit.mapDefValue.end() && itr_def_param->second.bHasDefaultValue)
                {
                    if (itr_def_param->second.enType == sdo::commonsdk::MSG_FIELD_INT)
                    {
                        param_value.type  = VALUETYPE_INT;
                        param_value.value = &itr_def_param->second.nDefaultValue;
                        param_value.len   = sizeof(int);
                    }
                    else
                    {
                        param_value.type  = VALUETYPE_STRING;
                        param_value.value = itr_def_param->second.strDefaultValue.c_str();
                        param_value.len   = itr_def_param->second.strDefaultValue.length();
                    }
                }
            }
        }
        else if (itr_condition_param->enValueType == VTYPE_LASTCODE)
        {
            param_value.type  = VALUETYPE_INT;
            param_value.value = &m_last_code;
            param_value.len   = sizeof(int);
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,param last code[%d]\n", __FUNCTION__, *(int*)param_value.value);
        }
        else if (itr_condition_param->enValueType == VTYPE_REMOTEIP)
        {
            param_value.type  = VALUETYPE_STRING;
            param_value.value = m_strRemoteIp.c_str();
            param_value.len   = m_strRemoteIp.length();
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,param remote ip[%s]\n", __FUNCTION__, (char*)param_value.value);
        }
        else if (itr_condition_param->enValueType == VTYPE_RETURN_CODE)
        {
            param_value.type  = VALUETYPE_INT;
            param_value.value = GetNodeCode(m_vecNodeRet, itr_condition_param->sSeq.nIndex);
            param_value.len   = sizeof(int);
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,param code=[%d]\n", __FUNCTION__, param_value.value ? *(int*)param_value.value : -1);
        }

        SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s,param type[%d] value[%p], len[%d]\n", 
            __FUNCTION__, param_value.type, param_value.value, param_value.len);
        values.push_back(param_value);
    }

    return goto_node_.cCondition.getResult(values);
}

void CComposeServiceObj::BuildStateMachineFunctionLogRequest(const string &strMachineName, int nState, int nEvent, SServiceLocation &location)
{
	char szBuf[100]={0};
	snprintf(szBuf,sizeof(szBuf),"%s^_^%d^_^%d",strMachineName.c_str(),nState,nEvent);
	location.reqParas = szBuf;
}

void CComposeServiceObj::BuildStateMachineFunctionLogResponse(int nResult, SServiceLocation &location)
{
	char szBuf[20];
	snprintf(szBuf,sizeof(szBuf),"%d",nResult);
	location.rspParas = szBuf;
}

void CComposeServiceObj::BuildStateMachineFunctionLogResponse(string strName, SServiceLocation &location)
{
	char szBuf[20];
	snprintf(szBuf,sizeof(szBuf),"%s",strName.c_str());
	location.rspParas = szBuf;
}

void CComposeServiceObj::BuildFunctionLogRequest(vector<inst_value> &stk, SServiceLocation &location)
{
	int  i=0;
	vector<inst_value>::iterator it = stk.begin();
	for(;it!=stk.end();it++)
	{
		string strPara;
		if(it->m_type==eiv_int)
		{
			char szBuf[20];
			snprintf(szBuf,sizeof(szBuf),"%d",it->m_int_value);
			strPara = szBuf;
		}
		else
		{
			strPara = it->to_string();
		}
		if (stk.size()>1 && i< stk.size()-1)
		{
			location.reqParas +=strPara + "^_^";
		}
		else
		{
			location.reqParas +=strPara;
		}
		i++;
	}
}

void CComposeServiceObj::BuildFunctionLogResponse(vector<inst_value> &stk, SServiceLocation &location)
{
	if(stk[0].m_type==eiv_int)
	{
		char szBuf[20];
		snprintf(szBuf,sizeof(szBuf),"%d",stk[0].m_int_value);
		location.rspParas = szBuf;
	}
	else
	{
		location.rspParas = stk[0].to_string();
	}
}
int CComposeServiceObj::BuildPacketBody(const string& strName, const vector<SValueInCompose>& vecAttri,CAvenueMsgHandler &msgHandler,bool bIsRequest, string &params,int seqLoopIndex)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, service[%s], attribute[%d]\n",__FUNCTION__, strName.c_str(), vecAttri.size()); 
    vector<SValueInCompose>::const_iterator itrVec;
    for(itrVec=vecAttri.begin(); itrVec!=vecAttri.end(); ++itrVec)
    {
        const SValueInCompose& sValue = *itrVec;
        if (sValue.enTargetType == VTYPE_XHEAD)
        {
            EValueType eValueType;
            vector<SStructValue> values;
            if (sValue.enValueType == VTYPE_RESPONSE)
            {
                RCIterator itrRes = m_mapResponse.find(SNodeSeq(sValue.sNode.nIndex,sValue.sSeq.nIndex));
                if(itrRes == m_mapResponse.end())
                {
                    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s line[%d] get response failed,target[%s],"\
                        "source[%s] node[%s], seq[%s]\n", __FUNCTION__,__LINE__,
                        sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str());
                    continue;
                }
                SResponseRet& sRsp = itrRes->second;
                sRsp.pHandler->GetValues(sValue.strSourceKey.c_str(), values, eValueType);
            }
            else if (sValue.enValueType == VTYPE_REQUEST)
            {
                m_pReqHandler->GetValues(sValue.strSourceKey.c_str(), values, eValueType);
            }
            else
            {
                SStructValue sStructValue;
                sStructValue.pValue = (void*)sValue.strSourceKey.c_str();
                sStructValue.nLen   = sValue.strSourceKey.size();
                values.push_back(sStructValue);
            }

            if (values.empty())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s the value for xhead %s is empty.\n", __FUNCTION__, sValue.strTargetKey.c_str());
                continue;
            }

            string strTargetKey(sValue.strTargetKey);
            transform (strTargetKey.begin(), strTargetKey.end(), strTargetKey.begin(), (int(*)(int))tolower);
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s strTargetKey[%s].\n", __FUNCTION__, strTargetKey.c_str());
            if (strTargetKey.find("appid") != string::npos)
            {
                m_pExHead->updateAppId(*(int *)values[0].pValue);
            }
            else if (strTargetKey.find("areaid") != string::npos)
            {
                m_pExHead->updateAreaId(*(int *)values[0].pValue);
            }
            else if (strTargetKey.find("name") != string::npos)
            {
                m_pExHead->updateName(string((char*)values[0].pValue, values[0].nLen));
            }
            else if (strTargetKey.find("socid") != string::npos)
            {
                m_pExHead->updateSocId(string((char*)values[0].pValue, values[0].nLen));
            }

            continue;
        }

        if(sValue.enValueType == VTYPE_REQUEST)
        {
			if(sValue.isLoop&&seqLoopIndex>=0)
				BuildField(m_pReqHandler, msgHandler, sValue,seqLoopIndex);
			else
				BuildField(m_pReqHandler, msgHandler, sValue);
        }
        else if(sValue.enValueType == VTYPE_RESPONSE)
        {
            RCIterator itrRes = m_mapResponse.find(SNodeSeq(sValue.sNode.nIndex,sValue.sSeq.nIndex));
            if(itrRes == m_mapResponse.end())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s line[%d] get response failed,target[%s],"\
                    "source[%s] node[%s], seq[%s]\n", __FUNCTION__,__LINE__,
                    sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str());
                continue;
            }
            SResponseRet& sRsp = itrRes->second;
            if(sRsp.pHandler!= NULL)
			{
				if(sValue.isLoop&&seqLoopIndex>=0)
					BuildField(sRsp.pHandler, msgHandler, sValue,seqLoopIndex);
				else
					BuildField(sRsp.pHandler, msgHandler, sValue);
            }
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s,execCppPlus read from response. target[%s],source[%s] node[%s], seq[%s] intnode[%d] intseq[%d]\n",__FUNCTION__, sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str(),sValue.sNode.nIndex,sValue.sSeq.nIndex);
        }
        else if (sValue.enValueType == VTYPE_DEF_VALUE)
		{
			if(sValue.isLoop&&seqLoopIndex>=0)
				BuildDefField(m_oCSUnit.mapDefValue, _def_values, msgHandler, sValue,seqLoopIndex);
			else
				BuildDefField(m_oCSUnit.mapDefValue, _def_values, msgHandler, sValue);
        }
        else if (sValue.enValueType == VTYPE_LASTCODE)
        {
            msgHandler.SetValue(sValue.strTargetKey.c_str(), &m_last_code, sizeof(int));
		}
		else if (sValue.enValueType == VTYPE_LOOP_RETURN_CODE)
		{
			map<string, SDefValue>::iterator itr_value = _def_values.find(sValue.strSourceKey);
			if (itr_value != _def_values.end())
			{
				if(sValue.isLoop&&seqLoopIndex>=0)
				{
					if(itr_value->second.vecLoopRetCode.size()>(unsigned)seqLoopIndex)
					{
						msgHandler.SetValue(sValue.strTargetKey.c_str(), itr_value->second.vecLoopRetCode[seqLoopIndex]);
						SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, set loopRetCode strTargetKey[%s], value[%d]\n",
							__FUNCTION__, sValue.strTargetKey.c_str(), itr_value->second.vecLoopRetCode[seqLoopIndex]); 
					}
					else
					{
						SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, can't get source value in loopIndex[%d].strTargetKey[%s], value size[%d] source key[%s]\n",
							__FUNCTION__,seqLoopIndex, sValue.strTargetKey.c_str(), itr_value->second.vecLoopRetCode.size(),sValue.strSourceKey.c_str());
					}
				}
				else
				{
					vector<int>::const_iterator loopRetCodeItr=itr_value->second.vecLoopRetCode.begin();
					for(;loopRetCodeItr!=itr_value->second.vecLoopRetCode.end();++loopRetCodeItr)
					{
						msgHandler.SetValue(sValue.strTargetKey.c_str(), *loopRetCodeItr);
						SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, set loopRetCode strTargetKey[%s], value[%d]\n",
							__FUNCTION__, sValue.strTargetKey.c_str(), *loopRetCodeItr); 
					}
				}
			}
			else
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, can't find sourcekey in defvalues.strTargetKey[%s], source key[%s]\n",
					__FUNCTION__,sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
			}
		}
        else if (sValue.enValueType == VTYPE_REMOTEIP)
        {
            msgHandler.SetValue(sValue.strTargetKey.c_str(), m_strRemoteIp.c_str());
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, strTargetKey[%s], m_strRemoteIp[%s]\n",
                __FUNCTION__, sValue.strTargetKey.c_str(), m_strRemoteIp.c_str()); 
        }
        else if (sValue.enValueType == VTYPE_NOCASE_VALUE)
        {
            msgHandler.SetValueNoCaseType(sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, strTargetKey[%s], no case value[%s]\n",
                __FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str()); 
			
		}
		else  if ((sValue.enValueType == VTYPE_BUILTINFUNCTION) && (strcasecmp(sValue.strSourceKey.c_str(),"ExcuteStateMachine")==0)) //ExcuteStateMachine.
		{
			SServiceLocation sLoc;
			sLoc.msgId = 0;
			sLoc.srvId = 0;
			sLoc.nNode = 0;
			sLoc.nSeq = 0;
			sLoc.srvName = GetFunctionFullSourcekey(sValue.enValueType,sValue.strSourceKey);
			string strMachineName;
			int nState;
			int nEvent;
			int bResult= -1;
			if (sValue.sFunctionParams.size()!=3)
			{
				bResult = SAP_CODE_FUNCALLERROR;
			}
			else
			{
				inst_value instValue1 = GetFunctionParamValue(sValue.sFunctionParams[0]);
				inst_value instValue2 = GetFunctionParamValue(sValue.sFunctionParams[1]);
				inst_value instValue3 = GetFunctionParamValue(sValue.sFunctionParams[2]);
				if (instValue1.m_type!=eiv_string||instValue2.m_type!=eiv_int||instValue3.m_type!=eiv_int)
				{
					bResult = SAP_CODE_FUNCALLERROR;
				}
				else
				{
					strMachineName = instValue1.to_string();
					nState = instValue2.m_int_value;
					nEvent = instValue3.m_int_value;
				}
			}
			BuildStateMachineFunctionLogRequest(strMachineName,nState,nEvent,sLoc);
			map<string, SDefParameter>::const_iterator it_para = m_oCSUnit.mapDefValue.find(sValue.strTargetKey);
			if (it_para != m_oCSUnit.mapDefValue.end())
			{
				if (it_para->second.enType != sdo::commonsdk::MSG_FIELD_INT)
				{
					bResult = SAP_CODE_FUNCALLERROR;
				}
				else
				{
					nState = CStateMachine::Instance()->ExcuteStateMachine(strMachineName, nState, nEvent); 
					bResult = 0;
				}
			}
			else
			{
				SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error miss def type when calling lua function\n",__FUNCTION__);
			}
			if(bResult==0)
			{
				BuildStateMachineFunctionLogResponse(nState,sLoc);
				RecordSyncRequest(sLoc, bResult);
				SDefValue& def_value   = _def_values[sValue.strTargetKey];
				def_value.vecValue.clear();
				def_value.enSourceType =VTYPE_LASTCODE;
				def_value.nLastCode = nState;
				def_value.enType= sdo::commonsdk::MSG_FIELD_INT;
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, %s=%d \n",__FUNCTION__, sValue.strTargetKey.c_str(), nState);
			}
			else
			{
				RecordSyncRequest(sLoc, bResult);
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result. targetkey[%s] funcstr[%s]\n",
					__FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
			}
		}
		else  if ((sValue.enValueType == VTYPE_BUILTINFUNCTION) && (strcasecmp(sValue.strSourceKey.c_str(),"GetMachineStateName")==0)) //GetMachineStateName.
		{
			SServiceLocation sLoc;
			sLoc.msgId = 0;
			sLoc.srvId = 0;
			sLoc.nNode = 0;
			sLoc.nSeq = 0;
			sLoc.srvName = GetFunctionFullSourcekey(sValue.enValueType,sValue.strSourceKey);
			string strMachineName;
			int nState;
			int nEvent;
			string strStateName;
			int bResult= -1;
			if (sValue.sFunctionParams.size()!=2)
			{
				bResult = SAP_CODE_FUNCALLERROR;
				SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, buitin para error\n",__FUNCTION__);
			}
			else
			{
				inst_value instValue1 = GetFunctionParamValue(sValue.sFunctionParams[0]);
				inst_value instValue2 = GetFunctionParamValue(sValue.sFunctionParams[1]);
				if (instValue1.m_type!=eiv_string||instValue2.m_type!=eiv_int)
				{
					bResult = SAP_CODE_FUNCALLERROR;
					SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, buitin type error\n",__FUNCTION__);
				}
				else
				{
					strMachineName = instValue1.to_string();
					nState = instValue2.m_int_value;
				}
			}
			BuildStateMachineFunctionLogRequest(strMachineName,nState,nEvent,sLoc);
			map<string, SDefParameter>::const_iterator it_para = m_oCSUnit.mapDefValue.find(sValue.strTargetKey);
			if (it_para != m_oCSUnit.mapDefValue.end())
			{
					strStateName = CStateMachine::Instance()->GetMachineStateName(strMachineName, nState); 
					bResult = 0;
			}
			else
			{
				SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error miss def type when calling lua function\n",__FUNCTION__);
			}
			if(bResult==0)
			{
				BuildStateMachineFunctionLogResponse(strStateName,sLoc);
				RecordSyncRequest(sLoc, bResult);
				SDefValue& def_value   = _def_values[sValue.strTargetKey];
				
				def_value.enSourceType =sValue.enValueType;
				def_value.vecValue.clear();
				def_value.vecValue.push_back(SStructValue());
				def_value.enType       = sdo::commonsdk::MSG_FIELD_STRING;
				(string&)(sValue.sFunctionRet) = strStateName;
				string & sRetString = (string&)sValue.sFunctionRet;
				SStructValue& value_struct = def_value.vecValue[0];
				value_struct.pValue    = (void*)sValue.sFunctionRet.c_str();
				value_struct.nLen      = sValue.sFunctionRet.size();
			}
			else
			{
				RecordSyncRequest(sLoc, bResult);
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result. targetkey[%s] funcstr[%s]\n",
					__FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
			}
		}
		else if (sValue.enValueType == VTYPE_FUNCTION || sValue.enValueType == VTYPE_LUAFUNCTION)
		{
			SServiceLocation sLoc;
			sLoc.msgId = 0;
			sLoc.srvId = 0;
			sLoc.nNode = 0;
			sLoc.nSeq = 0;
			sLoc.srvName = GetFunctionFullSourcekey(sValue.enValueType,sValue.strSourceKey);

			vector<inst_value> stk;
			for(vector<SFunctionParam>::const_iterator itrFuncParam=sValue.sFunctionParams.begin();
				itrFuncParam!=sValue.sFunctionParams.end();++itrFuncParam)
			{
				stk.push_back(GetFunctionParamValue(*itrFuncParam));
			}
			BuildFunctionLogRequest(stk,sLoc);
			int bResult = 0;
			if (sValue.enValueType == VTYPE_FUNCTION)
			{
				bResult=extern_fun_manager::GetInstance()->call(sValue.strSourceKey,stk.size(),stk,0);
			}
			else if (sValue.enValueType == VTYPE_LUAFUNCTION)
			{
				int nRetType = msgHandler.GetType(sValue.strTargetKey.c_str());
				if (nRetType!=-1)
				{
					int nEnType = -1;
					if (nRetType == sdo::commonsdk::MSG_FIELD_INT) 
					{
						nEnType = eiv_int;			
					}
					else if (nRetType == sdo::commonsdk::MSG_FIELD_STRING)
					{
						nEnType = eiv_string;
					}
					else
					{
						SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error increctt type when calling lua function %d\n",__FUNCTION__,nRetType);
					}
					bResult = extern_fun_lua_manager::GetInstance()->call(sValue.strSourceKey,stk.size(),stk,0,nEnType);
				}
				else
				{
					SS_XLOG(XLOG_ERROR,"CComposeServiceObj::%s, error miss type when calling lua function\n",__FUNCTION__);
				}
			}
			if(bResult==0)
			{
				BuildFunctionLogResponse(stk,sLoc);
				RecordSyncRequest(sLoc, bResult);
				if(stk[0].m_type==eiv_int)
					msgHandler.SetValue(sValue.strTargetKey.c_str(), &stk[0].m_int_value, sizeof(int));
				else
					msgHandler.SetValue(sValue.strTargetKey.c_str(), stk[0].to_string().c_str());
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, strTargetKey[%s],execCppFunc  func_str[%s], func_result[%s]\n",
					__FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str(),stk[0].to_string().c_str()); 
			}
			else
			{
				RecordSyncRequest(sLoc, bResult);
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s, execCppFunc Can`t set def string value which is from function result. targetkey[%s] funcstr[%s]\n",
					__FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
			}
		}
        else
        {
            SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s don't know the type of value in compose.\n", __FUNCTION__);
        }
       
    }

  //  pMsgEncode->EncodeMsg(&msgHandler);

    if (bIsRequest)
    {
        msgHandler.GetMsgParam(params);        
    }
    return 0;
}

int CComposeServiceObj::BuildDataPacketBody(CSapEncoder& sapEncoder, const void* rspBuffer, unsigned rspLen, int nResult, const map<string, vector<string> >& defValues)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s\n",__FUNCTION__);

    sapEncoder.Reset();
    sapEncoder.SetService(SAP_PACKET_REQUEST, DATASERVICE_SERVICEID, DATASERVICE_MSGID);
    sapEncoder.SetSequence(ntohl(++m_dwServiceSeq));
    SEXHeadBufAndLen exHeadBuf = m_pExHead->getExhead();
    sapEncoder.SetExtHeader(exHeadBuf.buffer, exHeadBuf.len);
    sapEncoder.SetValue(DATA_ATTR_SERVICE_ID,    m_rootServId);
    sapEncoder.SetValue(DATA_ATTR_MSG_ID,        m_rootMsgId);
    sapEncoder.SetValue(DATA_ATTR_RESULT_CODE,   nResult);
    sapEncoder.SetValue(DATA_ATTR_REQUEST_DATA,  m_pReqBuffer, m_dwBufferLen);
    sapEncoder.SetValue(DATA_ATTR_RESPONSE_DATA, rspBuffer, rspLen);

    char buffer[4096] = {0};
    char* des;
    char* end = buffer + 4094;
    map<string, vector<string> >::const_iterator itrDefValue = defValues.begin();
    for (; itrDefValue != defValues.end(); ++itrDefValue)
    {
        des = buffer;
        memset(buffer, 0, 4096);
        *(unsigned short*)des = htons((unsigned short)itrDefValue->second.size());
        des += sizeof(unsigned short);

        strcpy(des, itrDefValue->first.c_str());
        des += itrDefValue->first.length();
        *des++ = '\0';

        vector<string>::const_iterator itrValue = itrDefValue->second.begin();
        for (; itrValue != itrDefValue->second.end(); ++itrValue)
        {
            if (end - des < int(itrValue->size()))
            {
                SS_XLOG(XLOG_WARNING,"CComposeDataServiceObj::%s def value size is too big. def value[%s] size[%u]\n",
                    __FUNCTION__, itrDefValue->first.c_str(), itrValue->size());
                continue;
            }

            *(unsigned short*)des = htons((unsigned short)itrValue->size());
            des += sizeof(unsigned short);
            memcpy(des, itrValue->c_str(), itrValue->size());
            des += itrValue->size();
            *des++ = '\0';
        }

        sapEncoder.SetValue(DATA_ATTR_DEF_VALUE,  buffer, des - buffer);
    }

    return 0;
}


void CComposeServiceObj::RecordComposeService(int nCode, const string &strErrInfo)const
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s\n",__FUNCTION__);  
    if (IsEnableLevel(m_level, XLOG_INFO))
	{
        struct tm tmStart, tmNow;
        struct timeval_a now,interval;
        m_pManager->gettimeofday(&now);
        localtime_r(&(m_tmStart.tv_sec), &tmStart);
        localtime_r(&(now.tv_sec), &tmNow);
        timersub(&now,&m_tmStart, &interval);    
        unsigned dwInterMS = interval.tv_sec*1000+interval.tv_usec/1000; 
        const string strStartTm = TimeToString(tmStart,m_tmStart.tv_usec/1000);
        const string strNowTm   = TimeToString(tmNow,now.tv_usec/1000);

	SComposeLog oCompose;
	oCompose.isBNB=m_logInfo.isBNB;
	oCompose.strStartTm=strStartTm;
	oCompose.strExHeadLogMsg=m_pExHead->getLogMsg();
	oCompose.strExHeadLogId=m_pExHead->getLogId();
	oCompose.dwPacketSequence=m_dwServiceSeq;
	oCompose.nServiceId=m_rootServId;
	oCompose.nMsgId=m_rootMsgId;
	
	oCompose.strNowTm=strNowTm;
	oCompose.dwInterMS=dwInterMS;
	oCompose.dwusec=interval.tv_usec;
	
	oCompose.strErrorInfo=strErrInfo;
	oCompose.strRunPath=m_strRunPath;
	oCompose.nReponseCode=nCode;

	oCompose.strLogInfo=m_logInfo.logInfo;
	oCompose.plusLogInfo=m_logInfo.plusLogInfo;
	oCompose.logInfo2=m_logInfo.logInfo2;
	oCompose.strComments=m_pExHead->getComment();

	oCompose.strAddr=m_pExHead->getIp();

        CAsyncLogThread::GetInstance()->OnComposeAsyncLog(m_level,XLOG_INFO,oCompose);
    }
}

string CComposeServiceObj::GetLevelLog(bool isLeaf)
{
	string strLog = "";
	CComposeServiceObj* pParentObject = this->GetParentObejct();
	if (pParentObject!=NULL)
		strLog = GetServiceMsgName();	
	while (pParentObject!=NULL)
	{
		if (pParentObject->GetParentObejct()!=NULL)
		{
			strLog = pParentObject->GetServiceMsgName()+"->"+strLog;
		}
		pParentObject = pParentObject->GetParentObejct();
	}
	if (strLog.size()==0)
	{
		if (isLeaf)
		{
			strLog = "MainProcess";
		}
		else
		{
			strLog = "MainProcess_Result";
		}
	}
	return strLog;
}

void CComposeServiceObj::RecordSequence(const SServiceLocation& rLoc, const timeval_a& timeStart, bool isLeaf)
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s\n",__FUNCTION__);
    if(m_nCsosLog||IsEnableLevel(SCS_AUDIT_MODULE, XLOG_INFO))
	{
        struct tm tmStart, tmNow;
		struct timeval_a now,interval;
		m_pManager->gettimeofday(&now);
		localtime_r(&(timeStart.tv_sec), &tmStart);
        localtime_r(&(now.tv_sec), &tmNow);
        timersub(&now,&timeStart, &interval);
        unsigned dwInterMS = interval.tv_sec*1000+interval.tv_usec/1000; 

        const string strStartTm = TimeToString(tmStart,timeStart.tv_usec/1000);
        const string strNowTm = TimeToString(tmNow,now.tv_usec/1000);

        char szBuf[MAX_LOG_BUFFER]={0};
        if (CSapStack::nLogType == 0)
        {
            snprintf(szBuf,sizeof(szBuf)-1,"%s,  %s,  %d,  %d,  %d,  %d,  %s,  %s,  %s.%s,  "\
                "%s,  %s,  %s,  %d.%03ld,  %d\n",
                strStartTm.c_str(), m_pExHead->getGuid(), m_rootServId, m_rootMsgId,
                rLoc.srvId, rLoc.msgId, rLoc.srvAddr.c_str(),GetLevelLog(isLeaf).c_str(),rLoc.nodeName.c_str(), rLoc.seqName.c_str(),
                rLoc.srvName.c_str(), rLoc.reqParas.c_str(), rLoc.rspParas.c_str(),
                dwInterMS,interval.tv_usec%1000, rLoc.nCode);
        }
        else
        {
            snprintf(szBuf,sizeof(szBuf)-1,"%s,  %s,  %d,  %d,  %s,  %s.%s,  "\
                "%s,  %s,  %s,  %d.%03ld,  %d\n",
                strStartTm.c_str(), m_pExHead->getGuid(),
                rLoc.srvId, rLoc.msgId, rLoc.srvAddr.c_str(), rLoc.nodeName.c_str(), rLoc.seqName.c_str(),
                rLoc.srvName.c_str(), rLoc.reqParas.c_str(), rLoc.rspParas.c_str(),
                dwInterMS,interval.tv_usec%1000, rLoc.nCode);
        }

        szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
        CAsyncLogThread::GetInstance()->OnAsyncLog(SCS_AUDIT_MODULE, XLOG_INFO, 0,
            dwInterMS,szBuf,rLoc.nCode, rLoc.srvAddr, rLoc.srvId, rLoc.msgId);
    }
}



void CComposeServiceObj::RecordSyncRequest(const SServiceLocation& sParas, int nCode)
{
    SS_XLOG(XLOG_TRACE,"CComposeServiceObj::%s\n",__FUNCTION__);
    if(m_nCsosLog||IsEnableLevel(SCS_AUDIT_MODULE, XLOG_DEBUG)||(nCode>=SAP_CODE_KEY_ERROR && nCode<=SAP_CODE_BAD_REQUEST))
	{
        struct tm tmNow;
		struct timeval_a now;
		m_pManager->gettimeofday(&now);		
        localtime_r(&(now.tv_sec), &tmNow);
        const string strNowTm = TimeToString(tmNow,now.tv_usec/1000);

        char szBuf[MAX_LOG_BUFFER]={0};
        snprintf(szBuf,sizeof(szBuf)-1,"%s,  %s,  %d,  %d,  %d,  %d,  127.0.0.1,  %s,  %s.%s,  "\
           "%s,  %s,  %s,  0.000,  %d\n",
           strNowTm.c_str(), m_pExHead->getGuid(), m_rootServId, m_rootMsgId,
           sParas.srvId, sParas.msgId,  GetLevelLog(true).c_str(), sParas.nodeName.c_str(), sParas.seqName.c_str(),
           sParas.srvName.c_str(), sParas.reqParas.c_str(), sParas.rspParas.c_str(),
           nCode);

        szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
        CAsyncLogThread::GetInstance()->OnAsyncLog(SCS_AUDIT_MODULE, XLOG_INFO, 0,
            0,szBuf,nCode, "127.0.0.1:0", sParas.srvId, sParas.msgId);
    }
}



CComposeServiceObj::~CComposeServiceObj()
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s this[%x]\n",__FUNCTION__, this);
    for(RCIterator itRsp=m_mapResponse.begin(); itRsp!=m_mapResponse.end(); ++itRsp)
    {
        delete itRsp->second.pHandler;
    }
    delete m_pReqHandler;    
    if (m_pReqBuffer)
    {
        free(m_pReqBuffer);
    }
}


void CComposeServiceObj::Dump()
{
	SS_XLOG(XLOG_NOTICE, "  IComposeBase[%x],source seq[%d]\n",m_pBase,m_dwSrcSeq);
    if(m_pReqHandler != NULL)
    {
        m_pReqHandler->Dump();
    }
    RCIterator itrResponse;
    for(itrResponse=m_mapResponse.begin(); itrResponse!=m_mapResponse.end(); ++itrResponse)
    {
        const SNodeSeq& sNode= itrResponse->first;
        SS_XLOG(XLOG_NOTICE, "    node[%d],nodeSeq[%d]\n", getNodeName(m_oCSUnit, sNode.nNode),getSequenceName(m_oCSUnit, sNode.nNode,sNode.nSeq)); 
        SResponseRet& rRsp = itrResponse->second;
        if (rRsp.pHandler != NULL)
        {
            rRsp.pHandler->Dump();
        }
    }

    vector<SSeqRet>::iterator itrNodeRet;
    for(itrNodeRet = m_vecNodeRet.begin(); itrNodeRet != m_vecNodeRet.end(); ++itrNodeRet)
    {   
        SS_XLOG(XLOG_NOTICE, "    seq[%d], code[%d]\n", itrNodeRet->nSeq, itrNodeRet->nCode);
    }
 
	SS_XLOG(XLOG_NOTICE, "  m_mapSeqService,size[%d]\n",m_mapService.size());
    map<unsigned int, SServiceLocation>::iterator itrSeq;
    for(itrSeq = m_mapService.begin(); itrSeq != m_mapService.end(); ++itrSeq)
    {
        unsigned int dwSeq = itrSeq->first;
        SServiceLocation & sLocation = itrSeq->second;
        SS_XLOG(XLOG_NOTICE, "    SourceSeq[%d], node[%s], nodeSeq[%s], name[%s]\n",
			dwSeq, getNodeName(m_oCSUnit, sLocation.nNode), getSequenceName(m_oCSUnit, sLocation.nNode, sLocation.nSeq), sLocation.srvName.c_str());
    }	
}

void CComposeServiceObj::BuildField(CAvenueMsgHandler *pGet, CAvenueMsgHandler &msgSet, const SValueInCompose& sValue,int seqLoopIndex)
{
    //SS_XLOG(XLOG_TRACE,"%s, pGet[%p]\n",__FUNCTION__, pGet);
    vector<SStructValue> values;
    EValueType type;
    pGet->GetValues(sValue.strSourceKey.c_str(), values, type);
    if(!values.empty())
    {
		if(seqLoopIndex>=0)
		{
			if(values.size()>(unsigned)seqLoopIndex)
			{
				msgSet.SetValue(sValue.strTargetKey.c_str(),values[seqLoopIndex].pValue,values[seqLoopIndex].nLen);
				SS_XLOG(XLOG_DEBUG, "%s, set value from handle in loopIndex[%d]. source key[%s], target key[%s]\n", __FUNCTION__,seqLoopIndex, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str());
			}
			else
			{
				SS_XLOG(XLOG_DEBUG, "%s, can't get source value in loopIndex[%d]. value size[%d] source key[%s] \n", __FUNCTION__, seqLoopIndex,values.size(),sValue.strSourceKey.c_str());
			}
		}
		else
		{
			vector<SStructValue>::const_iterator itr = values.begin();
			for (; itr != values.end(); ++itr)
			{
				msgSet.SetValue(sValue.strTargetKey.c_str(), itr->pValue, itr->nLen);
				SS_XLOG(XLOG_DEBUG, "%s, set value from handle. source key[%s], target key[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str());
			}
		}
    }
    else
    {
        SS_XLOG(XLOG_DEBUG, "%s, can't get source value. source key[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
    }
}

void CComposeServiceObj::BuildDefField(const map<string, SDefParameter>& def_parameter, map<string, SDefValue>& def_values, CAvenueMsgHandler &msgSet, const SValueInCompose& sValue,int seqLoopIndex)
{
    //SS_XLOG(XLOG_TRACE,"%s, def_parameter size[%u], def_values size[%u]\n",__FUNCTION__, def_parameter.size(), def_values.size());
	string strSourceKey = GetTargertKey(sValue.strSourceKey);
    map<string, SDefValue>::iterator itr_value = def_values.find(strSourceKey);
    if (itr_value != def_values.end())
    {
		if (IsMetaKey(sValue.strSourceKey))
		{
			string strField;
			if (GetTargertSecond(sValue.strSourceKey,strField))
			{
				//$this.a = $meta.a
				SValueData& value = _def_values[strSourceKey].sMetaData.GetField(strField);
				vector<sdo::commonsdk::SStructValue>::iterator it = value.vecValue.begin();
				for(;it != value.vecValue.end(); it++)
				{
					msgSet.SetValue(sValue.strTargetKey.c_str(), it->pValue, it->nLen);
				}
			}
			else if (strcmp(sValue.strTargetKey.c_str(),"*")==0)
			{
				//$this.* = $meta
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, ssss $this.* = $meta \n", __FUNCTION__);

				vector<sdo::commonsdk::SAvenueTlvFieldConfig> &mRequestName = msgSet.GetTlvFieldConfig(); 
				vector<sdo::commonsdk::SAvenueTlvFieldConfig>::iterator it_name = mRequestName.begin();
				for(;it_name!=mRequestName.end();it_name++)
				{
					EValueType  enType;
					int nResult = _def_values[strSourceKey].sMetaData.php.GetValue(it_name->strName,enType,&msgSet);
					
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, ssss request field name: %s nResult[%d]\n", __FUNCTION__,  it_name->strName.c_str(),nResult);
				}
			}
			else
			{
				//$this.a = $meta 
				SValueData& value = _def_values[strSourceKey].sMetaData.GetPHPValue();
				vector<sdo::commonsdk::SStructValue>::iterator it = value.vecValue.begin();
				for(;it != value.vecValue.end(); it++)
				{
					msgSet.SetValue(sValue.strTargetKey.c_str(), it->pValue, it->nLen);
				}
			}
		}
        else if (itr_value->second.enSourceType == VTYPE_LASTCODE)
        {
            msgSet.SetValue(sValue.strTargetKey.c_str(), &itr_value->second.nLastCode, sizeof(int));
            SS_XLOG(XLOG_DEBUG, "%s, set value as last code. target key[%s], lastcode[%d]\n", __FUNCTION__, sValue.strTargetKey.c_str(), itr_value->second.nLastCode);
        }
        else if (itr_value->second.enSourceType == VTYPE_REMOTEIP)
        {
            msgSet.SetValue(sValue.strTargetKey.c_str(), itr_value->second.strRemoteIp.c_str());
            SS_XLOG(XLOG_DEBUG, "%s, set value as remote ip. target key[%s], remoteip[%s]\n", __FUNCTION__, sValue.strTargetKey.c_str(), itr_value->second.strRemoteIp.c_str());
        }
        else if (itr_value->second.enSourceType == VTYPE_NOCASE_VALUE)
        {
            if (itr_value->second.vecValue.size() > 0)
            {
               msgSet.SetValueNoCaseType(sValue.strTargetKey.c_str(), (const char*)itr_value->second.vecValue[0].pValue);
               SS_XLOG(XLOG_DEBUG, "%s, set value as no case value. target key[%s], no case value[%s]\n", __FUNCTION__, sValue.strTargetKey.c_str(), (const char*)itr_value->second.vecValue[0].pValue);
            }
		}
		else if (itr_value->second.enSourceType == VTYPE_LOOP_RETURN_CODE)
		{
			if(seqLoopIndex>=0)
			{
				if(itr_value->second.vecLoopRetCode.size()>(unsigned)seqLoopIndex)
				{
					msgSet.SetValue(sValue.strTargetKey.c_str(), itr_value->second.vecLoopRetCode[seqLoopIndex]);
					SS_XLOG(XLOG_DEBUG, "%s, set value from def. source key[%s], target key[%s], loopIndex[%d] value[%d]\n", __FUNCTION__, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str(),seqLoopIndex,itr_value->second.vecLoopRetCode[seqLoopIndex]);
				}
			}
			else
			{
				vector<int>::const_iterator itr_def_value = itr_value->second.vecLoopRetCode.begin();
				for (; itr_def_value != itr_value->second.vecLoopRetCode.end(); ++itr_def_value)
				{
					msgSet.SetValue(sValue.strTargetKey.c_str(), *itr_def_value);
					SS_XLOG(XLOG_DEBUG, "%s, set value from def. source key[%s], target key[%s], value[%d]\n", __FUNCTION__, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str(), *itr_def_value);
				}
			}
		}
        else
        {
			if(seqLoopIndex>=0)
			{
				if(itr_value->second.vecValue.size()>(unsigned)seqLoopIndex)
				{
					msgSet.SetValue(sValue.strTargetKey.c_str(), itr_value->second.vecValue[seqLoopIndex].pValue, itr_value->second.vecValue[seqLoopIndex].nLen);
					SS_XLOG(XLOG_DEBUG, "%s, set value from def. source key[%s], target key[%s], value[%s] size[%d]\n", __FUNCTION__, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str(),(const char*)itr_value->second.vecValue[0].pValue, itr_value->second.vecValue[seqLoopIndex].nLen);
				}
			}
			else
			{
				vector<sdo::commonsdk::SStructValue>::const_iterator itr_def_value = itr_value->second.vecValue.begin();
				for (; itr_def_value != itr_value->second.vecValue.end(); ++itr_def_value)
				{
					msgSet.SetValue(sValue.strTargetKey.c_str(), itr_def_value->pValue, itr_def_value->nLen);
					SS_XLOG(XLOG_DEBUG, "%s, set value from def. source key[%s], target key[%s], value len[%d]\n", __FUNCTION__, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str(), itr_def_value->nLen);
				}
			}
        }
    }
    else
    {
        map<string, SDefParameter>::const_iterator itr_param = def_parameter.find(strSourceKey);
        if (itr_param == def_parameter.end() || !itr_param->second.bHasDefaultValue)
        {
            SS_XLOG(XLOG_DEBUG, "%s, the def value has no default value. def value[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
            return;
        }

        if (itr_param->second.enType == sdo::commonsdk::MSG_FIELD_INT)
        {
            msgSet.SetValue(sValue.strTargetKey.c_str(), itr_param->second.nDefaultValue);
        }
        else
        {
            msgSet.SetValue(sValue.strTargetKey.c_str(), 
                itr_param->second.strDefaultValue.c_str(), itr_param->second.strDefaultValue.length());
        }

        SS_XLOG(XLOG_DEBUG, "%s, set value from default. source key[%s], target key[%s], value[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str(), sValue.strTargetKey.c_str(), itr_param->second.strDefaultValue.c_str());
    }
}

void CComposeServiceObj::GetAllDefValue(map<string, vector<string> >& mapDefValue)
{
    map<string, SDefValue>::const_iterator itrDefValue;
    map<string, SDefParameter>::const_iterator itrParameter = m_oCSUnit.mapDefValue.begin();
    for (; itrParameter != m_oCSUnit.mapDefValue.end(); ++itrParameter)
    {
        vector<string>& vecDefValue = mapDefValue[itrParameter->first];

        itrDefValue = _def_values.find(itrParameter->first);
        if (itrDefValue != _def_values.end())
        {
            if (itrDefValue->second.enSourceType == VTYPE_LASTCODE)
            {
                string strValue((const char*)&itrDefValue->second.nLastCode, sizeof(int));
                vecDefValue.push_back(strValue);
            }
            else if (itrDefValue->second.enSourceType == VTYPE_REMOTEIP)
            {
                vecDefValue.push_back(itrDefValue->second.strRemoteIp);
            }
            else if (itrDefValue->second.enSourceType == VTYPE_NOCASE_VALUE)
            {
                if (itrDefValue->second.vecValue.size() > 0)
                {
                    if (itrParameter->second.enType == sdo::commonsdk::MSG_FIELD_INT)
                    {
                        int tmpInt = atoi((const char*)itrDefValue->second.vecValue[0].pValue);
                        string strValue((const char*)&tmpInt, sizeof(int));
                        vecDefValue.push_back(strValue);
                    }
                    else
                    {
                        vecDefValue.push_back((const char*)itrDefValue->second.vecValue[0].pValue);
                    }
                }
            }
            else
            {
                vector<sdo::commonsdk::SStructValue>::const_iterator itrValue = itrDefValue->second.vecValue.begin();
                for (; itrValue != itrDefValue->second.vecValue.end(); ++itrValue)
                {
                    string strValue((const char*)itrValue->pValue, itrValue->nLen);
                    vecDefValue.push_back(strValue);
                }
            }
        }
        else if (!itrParameter->second.strDefaultValue.empty())
        {
            if (itrParameter->second.enType == sdo::commonsdk::MSG_FIELD_INT)
            {
                string strValue((const char*)&itrParameter->second.nDefaultValue, sizeof(int));
                vecDefValue.push_back(strValue);
            }
            else
            {
                vecDefValue.push_back(itrParameter->second.strDefaultValue);
            }
        }
    }
}

void CComposeServiceObj::BuildSequenceLoopTimes(const map<SequeceIndex, SComposeSequence>& mapSeqs)
{
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, seqCount[%d]\n",__FUNCTION__, mapSeqs.size());
	m_nTotalSeqSize=0;
	vector<SStructValue> values;
	EValueType type;
	for(SeqIterator itrSeq=mapSeqs.begin(); itrSeq!=mapSeqs.end(); ++itrSeq)
	{
		int& times=m_mapSeqLoopTimes[itrSeq->first];
		times=1;
		const SComposeSequence& rSeq = itrSeq->second;
		const SValueInCompose& sValue=rSeq.sLoopTimes;
		if(sValue.enValueType == VTYPE_UNKNOW)
		{
			times=1;
		}
		else if(sValue.enValueType == VTYPE_REQUEST)
		{
			m_pReqHandler->GetValues(sValue.strSourceKey.c_str(), values, type);
			if(!values.empty()&&type==sdo::commonsdk::MSG_FIELD_INT)
			{
				times= *(int*)values[0].pValue;
				SS_XLOG(XLOG_DEBUG, "%s, set value from request. source key[%s], value[%d]\n",
					__FUNCTION__, sValue.strSourceKey.c_str(), times);
			}
			else
			{
				SS_XLOG(XLOG_WARNING, "CComposeServiceObj::%s, can't get source value from request. source key[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
			}
		}
		else if(sValue.enValueType == VTYPE_RESPONSE)
		{
			RCIterator itrRes = m_mapResponse.find(SNodeSeq(sValue.sNode.nIndex,sValue.sSeq.nIndex));
			if(itrRes == m_mapResponse.end())
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s line[%d] get response failed,source[%s] node[%s], seq[%s]\n", __FUNCTION__,__LINE__,
					sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str());
			}
			else
			{
				SResponseRet& sRsp = itrRes->second;
				if(sRsp.pHandler!= NULL)
				{
					sRsp.pHandler->GetValues(sValue.strSourceKey.c_str(), values, type);
					if(!values.empty()&&type==sdo::commonsdk::MSG_FIELD_INT)
					{
						times= *(int*)values[0].pValue;
						SS_XLOG(XLOG_DEBUG, "%s, set value from response. source key[%s], value[%d]\n",
							__FUNCTION__, sValue.strSourceKey.c_str(), times);
					}
					else
					{
						SS_XLOG(XLOG_WARNING, "CComposeServiceObj::%s, can't get source value from response. source key[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
					}
				}
			}
		}
		else if (sValue.enValueType == VTYPE_DEF_VALUE)
		{
			map<string, SDefValue>::iterator itr_value = _def_values.find(sValue.strSourceKey);
			if (itr_value != _def_values.end())
			{
				if (itr_value->second.enSourceType == VTYPE_LASTCODE)
				{
					times=itr_value->second.nLastCode;
					SS_XLOG(XLOG_DEBUG, "%s, set value from last code. sourcekey[%s], lastcode[%d]\n", __FUNCTION__, sValue.strSourceKey.c_str(), times);
				}
				else
				{
					if (itr_value->second.vecValue.size() > 0)
					{
						times= *(int*)itr_value->second.vecValue[0].pValue;
						SS_XLOG(XLOG_DEBUG, "%s, set value from no case value. sourcekey[%s], no case value[%d]\n", __FUNCTION__, sValue.strSourceKey.c_str(), times);
					}
				}
			}
			else
			{
				map<string, SDefParameter>::const_iterator itr_param = m_oCSUnit.mapDefValue.find(sValue.strSourceKey);
				if (itr_param == m_oCSUnit.mapDefValue.end() || !itr_param->second.bHasDefaultValue)
				{
					SS_XLOG(XLOG_DEBUG, "%s, the def value has no default value. def value[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
				}
				else if (itr_param->second.enType == sdo::commonsdk::MSG_FIELD_INT)
				{
					times=itr_param->second.nDefaultValue;
					SS_XLOG(XLOG_DEBUG, "%s, set value from default. source key[%s], value[%d]\n",
						__FUNCTION__, sValue.strSourceKey.c_str(),  times);
				}
			}
		}
		else if (sValue.enValueType == VTYPE_NOCASE_VALUE)
		{
			times=atoi(sValue.strSourceKey.c_str());
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, no case value[%s]\n",
				__FUNCTION__, sValue.strSourceKey.c_str()); 
		}
		else
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s don't know the type of value in compose.\n", __FUNCTION__);
		}
		//SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s nodeindex[%s] looptimes[%d].\n", __FUNCTION__,itrSeq->first,times);
		m_nTotalSeqSize+=times;
	}
}
void CComposeServiceObj::BuildSequenceSpSosAddr(const map<SequeceIndex, SComposeSequence>& mapSeqs)
{
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, seqCount[%d]\n",__FUNCTION__, mapSeqs.size());
	vector<SStructValue> values;
	EValueType type;
	for(SeqIterator itrSeq=mapSeqs.begin(); itrSeq!=mapSeqs.end(); ++itrSeq)
	{
		if(itrSeq->second.sSpSosAddr.enValueType==VTYPE_UNKNOW)
		{
			continue;
		}
		string& strSpSosAddr=m_mapSeqSpSosAddr[itrSeq->first];
		const SComposeSequence& rSeq = itrSeq->second;
		const SValueInCompose& sValue=rSeq.sSpSosAddr;
		if(sValue.enValueType == VTYPE_REQUEST)
		{
			m_pReqHandler->GetValues(sValue.strSourceKey.c_str(), values, type);
			if(!values.empty()&&type==sdo::commonsdk::MSG_FIELD_STRING)
			{
				strSpSosAddr.assign((char*)values[0].pValue, values[0].nLen);
				SS_XLOG(XLOG_DEBUG, "%s, set value from request. source key[%s], value[%s]\n",
					__FUNCTION__, sValue.strSourceKey.c_str(), strSpSosAddr.c_str());
			}
			else
			{
				SS_XLOG(XLOG_WARNING, "CComposeServiceObj::%s, can't get source value from request. source key[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
			}
		}
		else if(sValue.enValueType == VTYPE_RESPONSE)
		{
			RCIterator itrRes = m_mapResponse.find(SNodeSeq(sValue.sNode.nIndex,sValue.sSeq.nIndex));
			if(itrRes == m_mapResponse.end())
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s line[%d] get response failed,source[%s] node[%s], seq[%s]\n", __FUNCTION__,__LINE__,
					sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str());
			}
			else
			{
				SResponseRet& sRsp = itrRes->second;
				if(sRsp.pHandler!= NULL)
				{
					sRsp.pHandler->GetValues(sValue.strSourceKey.c_str(), values, type);
					if(!values.empty()&&type==sdo::commonsdk::MSG_FIELD_STRING)
					{
						strSpSosAddr.assign((char*)values[0].pValue, values[0].nLen);
						SS_XLOG(XLOG_DEBUG, "%s, set value from response. source key[%s], value[%s]\n",
							__FUNCTION__, sValue.strSourceKey.c_str(), strSpSosAddr.c_str());
					}
					else
					{
						SS_XLOG(XLOG_WARNING, "CComposeServiceObj::%s, can't get source value from response. source key[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
					}
				}
			}
		}
		else if (sValue.enValueType == VTYPE_DEF_VALUE)
		{
			map<string, SDefValue>::iterator itr_value = _def_values.find(sValue.strSourceKey);
			if (itr_value != _def_values.end())
			{
				if (itr_value->second.vecValue.size() > 0)
				{
					strSpSosAddr.assign((char*)itr_value->second.vecValue[0].pValue, itr_value->second.vecValue[0].nLen);
					SS_XLOG(XLOG_DEBUG, "%s, set value from def value. sourcekey[%s], def value[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str(), strSpSosAddr.c_str());
				}
			}
			else
			{
				map<string, SDefParameter>::const_iterator itr_param = m_oCSUnit.mapDefValue.find(sValue.strSourceKey);
				if (itr_param == m_oCSUnit.mapDefValue.end() || !itr_param->second.bHasDefaultValue)
				{
					SS_XLOG(XLOG_DEBUG, "%s, the def value has no default value. def value[%s]\n", __FUNCTION__, sValue.strSourceKey.c_str());
				}
				else if (itr_param->second.enType == sdo::commonsdk::MSG_FIELD_STRING)
				{
					strSpSosAddr=itr_param->second.strDefaultValue;
					SS_XLOG(XLOG_DEBUG, "%s, set value from default. source key[%s], value[%s]\n",
						__FUNCTION__, sValue.strSourceKey.c_str(),  strSpSosAddr.c_str());
				}
			}
		}
		else if (sValue.enValueType == VTYPE_NOCASE_VALUE)
		{
			strSpSosAddr=sValue.strSourceKey;
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, no case value[%s]\n",
				__FUNCTION__, strSpSosAddr.c_str()); 
		}
		else
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s don't know the type of value in compose.\n", __FUNCTION__);
		}
	}
}
inst_value CComposeServiceObj::GetFunctionParamValue(const SFunctionParam& sFunctionParam)
{
	SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, this[%x] sourcekey[%s] >>>\n", __FUNCTION__, this, sFunctionParam.strSourceKey.c_str());
	inst_value eValue;
	SDefValue def_value;
		if (sFunctionParam.enValueType == VTYPE_RESPONSE)
		{
			RCIterator itrRes = m_mapResponse.find(SNodeSeq(sFunctionParam.sNode.nIndex,sFunctionParam.sSeq.nIndex));
			if(itrRes == m_mapResponse.end())
			{
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s line[%d] get response failed, source[%s] node[%s], seq[%s] intnode[%d] intseq[%d] rspSize[%d]\n", __FUNCTION__,__LINE__,
					sFunctionParam.strSourceKey.c_str(), sFunctionParam.sNode.strIndex.c_str(), sFunctionParam.sSeq.strIndex.c_str(),sFunctionParam.sNode.nIndex, sFunctionParam.sSeq.nIndex,m_mapResponse.size());
				return eValue;
			}
			SResponseRet& sRsp = itrRes->second;
			sRsp.pHandler->GetValues(sFunctionParam.strSourceKey.c_str(), def_value.vecValue, def_value.enType);
			if(def_value.enType==sdo::commonsdk::MSG_FIELD_INT)
			{
				eValue.m_type=eiv_int;
				if (def_value.vecValue.size()==0)
				{
					eValue.m_int_value=0;
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from response value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__, def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}
				else
				{
					eValue.m_int_value=*(int*)def_value.vecValue[0].pValue;
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from response value. value size[%u] value type[%d] value[%s]\n",__FUNCTION__, __LINE__, def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}
			}
			else
			{
				eValue.m_type=eiv_string;
				if (def_value.vecValue.size()==0)
				{
					eValue.m_string_value="";
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from response value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__, def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}
				else
				{
					eValue.m_string_value.assign((char*)def_value.vecValue[0].pValue, def_value.vecValue[0].nLen);
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from response value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__, def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}

			}
		}
		else if (sFunctionParam.enValueType == VTYPE_REQUEST)
		{
			def_value.enSourceType = sFunctionParam.enValueType;
			def_value.vecValue.clear();
			m_pReqHandler->GetValues(sFunctionParam.strSourceKey.c_str(), def_value.vecValue, def_value.enType);
			if(def_value.enType==sdo::commonsdk::MSG_FIELD_INT)
			{
				eValue.m_type=eiv_int;
				if (def_value.vecValue.size()==0)
				{
					eValue.m_int_value=0;
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from request value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__,def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}
				else
				{
					eValue.m_int_value=*(int*)def_value.vecValue[0].pValue;
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from request value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__,def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}

			}
			else
			{
				eValue.m_type=eiv_string;
				if (def_value.vecValue.size()==0)
				{
					eValue.m_string_value="";
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from request value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__,def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}
				else
				{
					eValue.m_string_value.assign((char*)def_value.vecValue[0].pValue, def_value.vecValue[0].nLen);
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from request value. value size[%u] value type[%d] value[%s]\n",
				__FUNCTION__, __LINE__,def_value.vecValue.size(), def_value.enType,eValue.to_string().c_str());
				}
			}
		}
		else if (sFunctionParam.enValueType == VTYPE_DEF_VALUE)
		{
			string strSourceKey = GetTargertKey(sFunctionParam.strSourceKey);	
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n", __FUNCTION__,__LINE__);
			map<string, SDefValue>::const_iterator itr_def_value = _def_values.find(strSourceKey);
			if (itr_def_value != _def_values.end())
			{
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n", __FUNCTION__,__LINE__);
				def_value = itr_def_value->second;
				if (IsMetaKey(sFunctionParam.strSourceKey))   
				{
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n", __FUNCTION__,__LINE__);
					string strField;
					if (GetTargertSecond(sFunctionParam.strSourceKey,strField))
					{
						SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n", __FUNCTION__,__LINE__);
						SValueData &value = ((SDefValue&)(def_value)).sMetaData.GetField(strField);
						if (value.vecValue.size()>0)
						{
							SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line[%d]\n", __FUNCTION__,__LINE__);
							if (value.enType == sdo::commonsdk::MSG_FIELD_INT)
							{
								eValue.m_type=eiv_int;
								eValue.m_int_value=*(int*)value.vecValue[0].pValue;
							}
							else if (value.enType == sdo::commonsdk::MSG_FIELD_STRING)
							{
								eValue.m_type=eiv_string;
								eValue.m_string_value.assign((char*)value.vecValue[0].pValue, value.vecValue[0].nLen);
							}
						}
					}
				}						
				else if (def_value.enSourceType == VTYPE_LASTCODE)
				{
					eValue.m_type=eiv_int;
					eValue.m_int_value=def_value.nLastCode;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d.\n", __FUNCTION__,__LINE__);
				}
				else if (def_value.enSourceType == VTYPE_REMOTEIP)
				{
					eValue.m_type=eiv_string;
					eValue.m_string_value=m_strRemoteIp;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d.\n", __FUNCTION__,__LINE__);
				}
				else
				{
					if (def_value.vecValue.empty())
					{
						SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d Can't get def value. source key[%s]. It will be NULL.\n", 
							__FUNCTION__,__LINE__, sFunctionParam.strSourceKey.c_str());
					}
					else
					{
						if (def_value.enType == sdo::commonsdk::MSG_FIELD_INT)
						{
							eValue.m_type=eiv_int;
							eValue.m_int_value=*(int*)def_value.vecValue[0].pValue;
							SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d.\n", __FUNCTION__,__LINE__);
						}
						else if (def_value.enType == sdo::commonsdk::MSG_FIELD_STRING)
						{
							eValue.m_type=eiv_string;
							eValue.m_string_value.assign((char*)def_value.vecValue[0].pValue, def_value.vecValue[0].nLen);
							SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d.\n", __FUNCTION__,__LINE__);
						}
						else
						{
							SS_XLOG(XLOG_WARNING,"CComposeServiceObj::%s. Can't use this type def value to do comparing operation, source[%s] type[%d]\n",
								__FUNCTION__, sFunctionParam.strSourceKey.c_str(), def_value.enType);
						}
					}
				}
			}
			else
			{
				map<string, SDefParameter>::const_iterator itr_def_param = m_oCSUnit.mapDefValue.find(strSourceKey);
				if (itr_def_param != m_oCSUnit.mapDefValue.end())
				{
					if (itr_def_param->second.bHasDefaultValue)
					{
						if (itr_def_param->second.enType == sdo::commonsdk::MSG_FIELD_INT)
						{
							eValue.m_type=eiv_int;
							eValue.m_int_value=itr_def_param->second.nDefaultValue;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from def value. value[%s]\n",
					__FUNCTION__, __LINE__, eValue.to_string().c_str());
						}
						else
						{
							eValue.m_type=eiv_string;
							eValue.m_string_value=itr_def_param->second.strDefaultValue;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from def value. value[%s]\n",
					__FUNCTION__, __LINE__,  eValue.to_string().c_str());
						}
					}
					else
					{
						if (itr_def_param->second.enType == sdo::commonsdk::MSG_FIELD_INT)
						{
							eValue.m_type=eiv_int;
							eValue.m_int_value=0;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from def value. value[%s]\n",
					__FUNCTION__, __LINE__, eValue.to_string().c_str());
						}
						else
						{
							eValue.m_type=eiv_string;
							eValue.m_string_value="";
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from def value. value[%s]\n",
					__FUNCTION__, __LINE__,  eValue.to_string().c_str());
						}
					}
				}
				else
				{
					SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d.\n", __FUNCTION__,__LINE__);
				}
			}
		}
		else if (sFunctionParam.enValueType == VTYPE_NOCASE_VALUE)
		{
		//	map<string, SDefParameter>::const_iterator itr_defvalue = m_oCSUnit.mapDefValue.find(sFunctionParam.strSourceKey);
			if (sFunctionParam.strSourceKey.find("\"")==0  
				&& sFunctionParam.strSourceKey.rfind("\"")==sFunctionParam.strSourceKey.size()-1)
			{
				eValue.m_type=eiv_string;
				eValue.m_string_value=sFunctionParam.strSourceKey.substr(1,sFunctionParam.strSourceKey.size()-2);
			}
			else
			{
				eValue.m_type=eiv_int;
				eValue.m_int_value=atoi(sFunctionParam.strSourceKey.c_str());
			}

			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from no case value. value[%s]\n",
				__FUNCTION__,  eValue.to_string().c_str());
		}
		else if (sFunctionParam.enValueType == VTYPE_LASTCODE)
		{
			eValue.m_type=eiv_int;
			eValue.m_int_value=m_last_code;

			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from last code. value[%d]\n",
				__FUNCTION__, eValue.m_int_value);
		}
		else if (sFunctionParam.enValueType == VTYPE_REMOTEIP)
		{
			eValue.m_type=eiv_string;
			eValue.m_string_value=m_strRemoteIp;

			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, Set def value which is from remote ip. value[%s]\n",
				__FUNCTION__,eValue.m_string_value.c_str());
		}
		else if (sFunctionParam.enValueType == VTYPE_RETURN_CODE)
		{
			const int* code = GetNodeCode(m_vecNodeRet, sFunctionParam.sSeq.nIndex);
			if (code)
			{
				eValue.m_type=eiv_int;
				eValue.m_int_value=*code;

				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, line: %d Set def value which is from code. value[%d]\n",
					__FUNCTION__,  __LINE__, eValue.m_int_value);
			}
			else
			{
				SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s. line: %d.\n", __FUNCTION__,__LINE__);
			}
		}
		else
		{
			SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s, unknow value type. It's not a def value. source key[%s]\n",
				__FUNCTION__, sFunctionParam.strSourceKey.c_str());
		}
		return eValue;
}

void dump_buffer(const string& strPacketInfo, const void *pBuffer)
{
    if(IsEnableLevel(SAP_STACK_MODULE,XLOG_NOTICE))
    {
        string strPacket(strPacketInfo);
        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;        
        unsigned char *pChar=(unsigned char *)pBuffer;
        unsigned int nPacketlen=ntohl(pHeader->dwPacketLen);
        int nLine=nPacketlen>>3;
        int nLast=(nPacketlen&0x7);
        int i=0;
        char szBuf[128]={0};
        for(i=0;(i<nLine)&&(i<40);i++)
        {            
            unsigned char * pBase=pChar+(i<<3);
            snprintf(szBuf, sizeof(szBuf),"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
                i,*(pBase),*(pBase+1),*(pBase+2),*(pBase+3),*(pBase+4),*(pBase+5),*(pBase+6),*(pBase+7));
            strPacket.append(szBuf);
        }
        
        unsigned char * pBase=pChar+(i<<3);
        if(nLast>0)
        {
            for(int j=0;j<8;j++)
            {
                char szBuf[128]={0};
                if(j==0)
                {
                    sprintf(szBuf,"                [%2d]    %02x ",i,*(pBase+j));
                    strPacket+=szBuf;
                }
                else if(j<nLast&&j==3)
                {
                    sprintf(szBuf,"%02x    ",*(pBase+j));
                    strPacket+=szBuf;
                }
                else if(j>=nLast&&j==3)
                {
                    strPacket+="      ";
                }
                else if(j<nLast)
                {
                    sprintf(szBuf,"%02x ",*(pBase+j));
                    strPacket+=szBuf;
                }
                else
                {
                    strPacket+="   ";
                }
                
            }
            strPacket+="   ......\n";
        }
        SS_XLOG(XLOG_NOTICE,strPacket.c_str());
    }
}

void dump_buffer(const void *pBuffer,EVirtualDirectType eType,const string& serviceName, int nNode)
{
    if(IsEnableLevel(SAP_STACK_MODULE,XLOG_NOTICE))
    {        
        static const char* DUMP_TYPE[E_ALL] = {
            "Write Sap Command to virtual service [",
            "Read Sap Command from virtual service [",
            "Write Sap Command to compose service [",
            "Read Sap Command from compose service [",
            "Write Sap Command to async virtual service ["
        };
        string strPacket;
        if (eType < E_ALL)
        {
            strPacket = DUMP_TYPE[eType];
        }
        strPacket.append(serviceName);
        char szNode[16]={0};
        sprintf(szNode,"] node[%d]\n",nNode);
        strPacket.append(szNode);
        dump_buffer(strPacket, pBuffer);
    }
}



