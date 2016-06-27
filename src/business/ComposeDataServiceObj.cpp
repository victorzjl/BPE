#include "ComposeDataServiceObj.h"
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
#include "AvenueTlvGroup.h"



void dump_buffer(const void *pBuffer,EVirtualDirectType eType,const string& serviceName,int nNode=0);
void dump_buffer(const string& strPacketInfo, const void *pBuffer);

int CComposeDataServiceObj::Init(unsigned dwIndex, int level, const SSRequestMsg &sReqMsg, const SDataMsgRequest& sDataMsgRequest)
{
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s...\n",__FUNCTION__); 
    if(0 != SaveRequestMsg(sDataMsgRequest))
    {
        SS_XLOG(XLOG_ERROR,"CComposeDataServiceObj::%s decode request error\n",__FUNCTION__);    
        return -1;
    }

    m_dwIndex = dwIndex;  
    m_level = level;
    SaveExHead(sReqMsg, sDataMsgRequest.pMsgHead);
    m_pBase->OnAddComposeService(dwIndex, this);

    CAvenueMsgHandler msgHandler(m_oCSUnit.strServiceName,false,m_pConfig);
    if (sDataMsgRequest.sResponseData.nLen > 0)
    {        
        msgHandler.Decode(sDataMsgRequest.sResponseData.pLoc, sDataMsgRequest.sResponseData.nLen);
    }
    CLogTrigger::BeginDataTrigger(m_logInfo, m_pReqHandler, &msgHandler, m_rootServId, m_rootMsgId, &m_reqDefValue);

    DoStartNodeService(0); 
    return 0;
}

CComposeDataServiceObj::CComposeDataServiceObj(IComposeBaseObj *pCBObj,const SComposeServiceUnit &rCSUnit,
                                               CAvenueServiceConfigs *pConfig,CSessionManager* pManager,unsigned dwSeq)
                                       :CComposeServiceObj(pCBObj, rCSUnit, pConfig, pManager, dwSeq), m_pRspHandler(NULL)
{}

CComposeDataServiceObj::~CComposeDataServiceObj()
{
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s this[%x]\n",__FUNCTION__, this);
    if (m_pRspHandler)
    {
        delete m_pRspHandler;
    }
}

int CComposeDataServiceObj::SaveRequestMsg(const SDataMsgRequest& sDataMsgRequest)
{    
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s\n",__FUNCTION__);

    m_reqResultCode = sDataMsgRequest.nResultCode;

    m_pReqHandler = new CAvenueMsgHandler(m_oCSUnit.strServiceName,true,m_pConfig);	 
    if (DecodeDataMsgBody(sDataMsgRequest.sRequestData.pLoc, sDataMsgRequest.sRequestData.nLen, m_pReqHandler) != 0)
    {
        return -1;
    }

    m_pRspHandler = new CAvenueMsgHandler(m_oCSUnit.strServiceName, false, m_pConfig);	 
    if (DecodeDataMsgBody(sDataMsgRequest.sResponseData.pLoc, sDataMsgRequest.sResponseData.nLen, m_pRspHandler) != 0)
    {
        return -1;
    }

    if (DecodeDefValueMsg(sDataMsgRequest.vecDefValue) != 0)
    {
        return -1;
    }

    return 0;
}

void CComposeDataServiceObj::DoStartNodeService(int nCurrNode, int nLastCode)
{
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, service[%s],node[%s],last code[%d]\n",
        __FUNCTION__,m_oCSUnit.strServiceName.c_str(),getNodeName(m_oCSUnit, nCurrNode),nLastCode); 
    if(DoIsDestroy(nCurrNode))
    {
        map<string, vector<string> > mapDefValue;
        GetAllDefValue(mapDefValue);
        CLogTrigger::EndDataTrigger(m_logInfo, &mapDefValue);
        RecordComposeService(m_reqResultCode);
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
        SS_XLOG(XLOG_ERROR,"CComposeDataServiceObj::%s node[%s] not found\n",__FUNCTION__, getNodeName(m_oCSUnit, nCurrNode));
        return;
    }
    const SComposeNode& rNode = itrNode->second; 
    CSapEncoder oRequest;
    //CAvenueMsgHandler oBodyEncode;

	BuildSequenceLoopTimes(rNode.mapSeqs);

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
            if(m_pParent == NULL)
            {    
                if (m_pBase != NULL)
                {
                    SendToClient(nCode,oBodyEncode.GetBuffer(),oBodyEncode.GetLen());
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
            unsigned int dwServiceId = 0, dwMsgId = 0;             
            if (m_pConfig->GetServiceIdByName(rSeq.strServiceName, &dwServiceId, &dwMsgId) != 0)
            {
                SS_XLOG(XLOG_ERROR,"CComposeDataServiceObj::%s service[%s] not found\n",
                    __FUNCTION__, rSeq.strServiceName.c_str());
                if(DoSaveResponseRet(nCurrNode,itrSeq->first,SAP_CODE_COMPOSE_SERVICE_NOT_FOUND,rNode))
                {
                    return;
                }
                continue;
            }
			CAvenueMsgHandler oBodyEncode(rSeq.strServiceName,true,m_pConfig);
            oBodyEncode.Reset();
            SServiceLocation sLoc;
            sLoc.msgId = dwMsgId;
            sLoc.srvId = dwServiceId;
            sLoc.nNode = nCurrNode;
            sLoc.nSeq = itrSeq->first;
            sLoc.srvName = rSeq.strServiceName;          
			sLoc.nodeName=rNode.strNodeName;
			sLoc.seqName=rSeq.strSequenceName;
            BuildPacketBody(rSeq.strServiceName, rSeq.vecField, oBodyEncode,true, sLoc.reqParas);
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
                        if ((nValidatorCode=DecodeMsgBody(pOutPacket, nOutLen,pResHandler) )!= 0)
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
            else
            {  
                bool isSosCfg = false;
                CSosSession* pSosSession = m_pManager->OnGetSos(dwServiceId,dwMsgId,&isSosCfg);
                if(NULL == pSosSession)
                {
                    sLoc.srvAddr = "127.0.0.1";
					SReturnMsgData returnMsg;
                    if (isSosCfg || 
                        RequestSubCompose(oRequest.GetBuffer(),oRequest.GetLen(), sLoc, returnMsg) == -1)
                    {
                        int nCode = SAP_CODE_NOT_FOUND;
                        if (rSeq.bAckMsg)
                        {
                            int nExLen = oRequest.GetLen() + sizeof(SForwardMsg);
                            SForwardMsg *pForwardMsg = (SForwardMsg*)CMapMemory::Instance().Allocate(nExLen);
                            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s alloc map memory[%x] len[%d] serviceId[%d] msgId[%d]\n",
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
                                SS_XLOG(XLOG_WARNING,"CComposeDataServiceObj::%s alloc map memory failed, len[%d]  serviceId[%d] msgId[%d]\n",
                                    __FUNCTION__, nExLen, dwServiceId, dwMsgId);
                            }
                        }                        

                        RecordSyncRequest(sLoc, SAP_CODE_NOT_FOUND);
                        if(DoSaveResponseRet(nCurrNode,itrSeq->first,nCode,rNode))
                        {
                            return;
                        }
                    }
                }
                else
                {                    
                    sLoc.srvAddr = pSosSession->Addr();
                    m_mapService.insert( make_pair(m_dwServiceSeq, sLoc) );
                    pSosSession->OnTransferRequest(
                        oRequest.GetBuffer(),
                        oRequest.GetLen(),
                        SComposeTransfer(this,m_rootMsgId,m_rootServId),
                        STransferData(rSeq.nTimeout,rSeq.bAckMsg));                
                } 
            }
        }
    }  
}


int CComposeDataServiceObj::DecodeDefValueMsg(const vector<SSapValueNode>& vecDefValue)
{
    vector<SSapValueNode>::const_iterator itr = vecDefValue.begin();
    for (; itr != vecDefValue.end(); ++itr)
    {
        if (!itr->pLoc || itr->nLen == 0)
        {
            continue;
        }
        const char* buffer = (const char*)itr->pLoc;
        const char* des    = buffer;
        unsigned short num = ntohs(*(unsigned short*)des);
        des += sizeof(unsigned short);
        string strValueName = des;
        des += strValueName.size() + 1;

        vector<string>& vecValue = m_reqDefValue[strValueName];
        for (unsigned short i = 0; i < num; ++i)
        {
            unsigned short len = ntohs(*(unsigned short*)des);
            des += sizeof(unsigned short);
            string strValue(des, len);
            des += 1;
            vecValue.push_back(strValue);
        }
    }
    return 0;
}

int CComposeDataServiceObj::BuildPacketBody(const string& strName, const vector<SValueInCompose>& vecAttri,CAvenueMsgHandler& msgHandler,bool bIsRequest, string &params)
{
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, service[%s], attribute[%d]\n",__FUNCTION__, strName.c_str(), vecAttri.size()); 
    vector<SValueInCompose>::const_iterator itrVec;
    for(itrVec=vecAttri.begin(); itrVec!=vecAttri.end(); ++itrVec)
    {        
        const SValueInCompose& sValue = *itrVec;

        if(sValue.enValueType == VTYPE_REQUEST)
        {
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Can't get value which is from date request. targetkey[%s] sourcekey[%s]\n",
                __FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
        }

        if (sValue.enTargetType == VTYPE_XHEAD)
        {
            EValueType eValueType;
            vector<SStructValue> values;
            if (sValue.enValueType == VTYPE_RESPONSE)
            {
                RCIterator itrRes = m_mapResponse.find(SNodeSeq(sValue.sNode.nIndex,sValue.sSeq.nIndex));
                if(itrRes == m_mapResponse.end())
                {
                    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s get response failed,target[%s],"\
                        "source[%s] node[%s], seq[%s]\n", __FUNCTION__,
                        sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str());
                    continue;
                }
                SResponseRet& sRsp = itrRes->second;
                sRsp.pHandler->GetValues(sValue.strSourceKey.c_str(), values, eValueType);
            }
            else if (sValue.enValueType == VTYPE_DATA_REQUEST)
            {
                m_pReqHandler->GetValues(sValue.strSourceKey.c_str(), values, eValueType);
            }
            else if (sValue.enValueType == VTYPE_DATA_RESPONSE)
            {
                m_pRspHandler->GetValues(sValue.strSourceKey.c_str(), values, eValueType);
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


        if(sValue.enValueType == VTYPE_RESPONSE)
        {
            RCIterator itrRes = m_mapResponse.find(SNodeSeq(sValue.sNode.nIndex,sValue.sSeq.nIndex));
            if(itrRes == m_mapResponse.end())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s get response failed,target[%s],"\
                    "source[%s] node[%s], seq[%s]\n", __FUNCTION__,
                    sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(),sValue.sSeq.strIndex.c_str());
                continue;
            }
            SResponseRet& sRsp = itrRes->second;
            if(sRsp.pHandler!= NULL)
            {
                BuildField(sRsp.pHandler, msgHandler, sValue);
            }
        }
        else if (sValue.enValueType == VTYPE_DEF_VALUE)
        {
            BuildDefField(m_oCSUnit.mapDefValue, _def_values, msgHandler, sValue);
        }
        else if (sValue.enValueType == VTYPE_LASTCODE)
        {
            msgHandler.SetValue(sValue.strTargetKey.c_str(), &m_last_code, sizeof(int));
        }
        else if (sValue.enValueType == VTYPE_REMOTEIP)
        {
            msgHandler.SetValue(sValue.strTargetKey.c_str(), m_strRemoteIp.c_str());
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, strTargetKey[%s], m_strRemoteIp[%s]\n",
                __FUNCTION__, sValue.strTargetKey.c_str(), m_strRemoteIp.c_str()); 
        }
        else if (sValue.enValueType == VTYPE_NOCASE_VALUE)
        {
            msgHandler.SetValueNoCaseType(sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str());
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, strTargetKey[%s], no case value[%s]\n",
                __FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str()); 
        }
        else if (sValue.enValueType == VTYPE_DATA_REQUEST)
        {
            BuildField(m_pReqHandler, msgHandler, sValue);
        }
        else if (sValue.enValueType == VTYPE_DATA_RESPONSE)
        {
            BuildField(m_pRspHandler, msgHandler, sValue);
        }
        else if (sValue.enValueType == VTYPE_DATA_CODE)
        {
            msgHandler.SetValue(sValue.strTargetKey.c_str(), &m_reqResultCode, sizeof(int));
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, strTargetKey[%s], data code[%d]\n",
                __FUNCTION__, sValue.strTargetKey.c_str(), m_reqResultCode); 
        }
        else if (sValue.enValueType == VTYPE_DATA_DEFVALUE)
        {
            map<string, vector<string> >::const_iterator itrReqDefValue = m_reqDefValue.find(sValue.strSourceKey);
            if (itrReqDefValue == m_reqDefValue.end())
            {
                SS_XLOG(XLOG_WARNING,"CComposeDataServiceObj::%s, can't get data def value[%s]\n",
                    __FUNCTION__, sValue.strSourceKey.c_str()); 
            }
            else
            {
                vector<string>::const_iterator itrValue = itrReqDefValue->second.begin();
                for (; itrValue != itrReqDefValue->second.end(); ++itrValue)
                {
                    msgHandler.SetValue(sValue.strTargetKey.c_str(), itrValue->c_str(), itrValue->size());
                }

                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, strTargetKey[%s], data def value[%s] size[%u]\n",
                    __FUNCTION__, sValue.strTargetKey.c_str(), sValue.strSourceKey.c_str(), itrReqDefValue->second.size()); 
            }
        }
        else
        {
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s don't know the type of value in compose.\n", __FUNCTION__);
        }
    }

   // pMsgEncode->EncodeMsg(&msgHandler);

    if (bIsRequest)
    {
        msgHandler.GetMsgParam(params);
    }
    return 0;
}

int CComposeDataServiceObj::DecodeDataMsgBody(const void *pBuffer, int nLen,CAvenueMsgHandler* pMsgHandler)
{
    SS_XLOG(XLOG_TRACE,"CComposeDataServiceObj::%s, buffer[%x], len[%d]\n",__FUNCTION__, pBuffer, nLen);    
    return pMsgHandler->Decode(pBuffer, nLen);    
}

bool CComposeDataServiceObj::IsRuleMatch(const SComposeGoto& goto_node_)const
{
    SS_XLOG(XLOG_TRACE,"CComposeDataServiceObj::%s, condition_param size[%u]\n", __FUNCTION__, goto_node_.vecConditionParam.size());

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
                    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't get value from response. source key[%s]. It will be NULL\n", 
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
                    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't use this type value to do comparing operation, source[%s] type[%d] value[%p] len[%d]\n",
                        __FUNCTION__, itr_condition_param->strSourceKey.c_str(), type, values[0].pValue, values[0].nLen);
                    return false;
                }
            }
        }
        else if (itr_condition_param->enValueType == VTYPE_REQUEST)
        {
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Can't get def value which is from date request. targetkey[%s] sourcekey[%s]\n",
                __FUNCTION__, itr_condition_param->strTargetKey.c_str(), itr_condition_param->strSourceKey.c_str());
        }
        else if (itr_condition_param->enValueType == VTYPE_DEF_VALUE)
        {
            map<string, SDefValue>::const_iterator itr = _def_values.find(itr_condition_param->strSourceKey);
            if (itr != _def_values.end())
            {
                const SDefValue& def_value = itr->second;

                if (def_value.enSourceType == VTYPE_LASTCODE)
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
                        SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't get def value. source key[%s]. It will be NULL.\n", 
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
                            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't use this type def value to do comparing operation, source[%s] type[%d]\n",
                                __FUNCTION__, itr_condition_param->strSourceKey.c_str(), def_value.enType);
                            return false;
                        }
                    }
                }
            }
            else
            {
                map<string, SDefParameter>::const_iterator itr_def_param = m_oCSUnit.mapDefValue.find(itr_condition_param->strSourceKey);
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
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s,param last code[%d]\n", __FUNCTION__, *(int*)param_value.value);
        }
        else if (itr_condition_param->enValueType == VTYPE_REMOTEIP)
        {
            param_value.type  = VALUETYPE_STRING;
            param_value.value = m_strRemoteIp.c_str();
            param_value.len   = m_strRemoteIp.length();
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s,param remote ip[%s]\n", __FUNCTION__, (char*)param_value.value);
        }
        else if (itr_condition_param->enValueType == VTYPE_RETURN_CODE)
        {
            param_value.type  = VALUETYPE_INT;
            param_value.value = GetNodeCode(m_vecNodeRet, itr_condition_param->sSeq.nIndex);
            param_value.len   = sizeof(int);
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s,param code=[%d]\n", __FUNCTION__, param_value.value ? *(int*)param_value.value : -1);
        }
        else if (itr_condition_param->enValueType == VTYPE_DATA_REQUEST)
        {
            vector<sdo::commonsdk::SStructValue> values;
            EValueType type;
            m_pReqHandler->GetValues(itr_condition_param->strSourceKey.c_str(), values, type);
            if (values.empty())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't get value from date request. source key[%s]. It will be NULL.\n", 
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
                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't use this type value to do comparing operation, source[%s] type[%d] value[%p] len[%d]\n",
                    __FUNCTION__, itr_condition_param->strSourceKey.c_str(), type, values[0].pValue, values[0].nLen);
                return false;
            }
        }
        else if (itr_condition_param->enValueType == VTYPE_DATA_RESPONSE)
        {
            vector<sdo::commonsdk::SStructValue> values;
            EValueType type;
            m_pRspHandler->GetValues(itr_condition_param->strSourceKey.c_str(), values, type);
            if (values.empty())
            {
                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't get value from date response. source key[%s]. It will be NULL.\n", 
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
                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't use this type value to do comparing operation, source[%s] type[%d] value[%p] len[%d]\n",
                    __FUNCTION__, itr_condition_param->strSourceKey.c_str(), type, values[0].pValue, values[0].nLen);
                return false;
            }
        }
        else if (itr_condition_param->enValueType == VTYPE_DATA_CODE)
        {
            param_value.type  = VALUETYPE_INT;
            param_value.value = &m_reqResultCode;
            param_value.len   = sizeof(int);
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s,param date return code[%d]\n", __FUNCTION__, *(int*)param_value.value);
        }
        else if (itr_condition_param->enValueType == VTYPE_DATA_DEFVALUE)
        {
            map<string, vector<string> >::const_iterator itrReqDefValue = m_reqDefValue.find(itr_condition_param->strSourceKey);
            if (itrReqDefValue == m_reqDefValue.end())
            {
                SS_XLOG(XLOG_WARNING,"CComposeDataServiceObj::%s, can't get data def value[%s]\n",
                    __FUNCTION__, itr_condition_param->strSourceKey.c_str()); 
            }
            else
            {
                if (itrReqDefValue->second.empty())
                {
                    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s. Can't get value from date def value. source key[%s]. It will be NULL.\n", 
                        __FUNCTION__, itr_condition_param->strSourceKey.c_str());
                }
                else if (itrReqDefValue->second[0].size() == sizeof(int))
                {
                    param_value.type  = VALUETYPE_INT;
                    param_value.value = itrReqDefValue->second[0].c_str();
                    param_value.len   = sizeof(int);
                }
                else 
                {
                    param_value.type  = VALUETYPE_STRING;
                    param_value.value = itrReqDefValue->second[0].c_str();
                    param_value.len   = itrReqDefValue->second[0].size();
                }
            }
        }

        SS_XLOG(XLOG_TRACE,"CComposeDataServiceObj::%s,param type[%d] value[%p], len[%d]\n", 
            __FUNCTION__, param_value.type, param_value.value, param_value.len);
        values.push_back(param_value);
    }

    return goto_node_.cCondition.getResult(values);
}

void CComposeDataServiceObj::DoSaveResponseDef(const SComposeNode& rNode)
{
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, this[%x]\n", __FUNCTION__, this);

    vector<SValueInCompose>::const_iterator itr = rNode.vecResultValues.begin();
    for (; itr != rNode.vecResultValues.end(); ++itr)
    {
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
                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s get response failed, target[%s],"\
                    "source[%s] node[%s], seq[%s]\n", __FUNCTION__,
                    itr->strTargetKey.c_str(), itr->strSourceKey.c_str(), itr->sNode.strIndex.c_str(), itr->sSeq.strIndex.c_str());
                continue;
            }

            SDefValue& def_value = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.vecValue.clear();
            SResponseRet& sRsp = itrRes->second;
            sRsp.pHandler->GetValues(itr->strSourceKey.c_str(), def_value.vecValue, def_value.enType);

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from response value. targetkey[%s] value size[%u] value type[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.vecValue.size(), def_value.enType);
        }
        else if (itr->enValueType == VTYPE_REQUEST)
        {
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Can't get def value which is from date request. targetkey[%s] sourcekey[%s]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
        }
        else if (itr->enValueType == VTYPE_DEF_VALUE)
        {
            map<string, SDefValue>::const_iterator itr_def_value = _def_values.find(itr->strSourceKey);
            if (itr_def_value != _def_values.end())
            {
                _def_values[itr->strTargetKey] = itr_def_value->second;

                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from def value. targetkey[%s] value size[%u] value type[%d]\n",
                    __FUNCTION__, itr->strTargetKey.c_str(), itr_def_value->second.vecValue.size(), itr_def_value->second.enType);
            }
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
            }

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from no case value. targetkey[%s] value[%s]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), itr->strSourceKey.c_str());
        }
        else if (itr->enValueType == VTYPE_LASTCODE)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.nLastCode    = m_last_code;
            def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from last code. targetkey[%s] value[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.nLastCode);
        }
        else if (itr->enValueType == VTYPE_REMOTEIP)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.strRemoteIp  = m_strRemoteIp;
            def_value.enType       = sdo::commonsdk::MSG_FIELD_STRING;

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from remote ip. targetkey[%s] value[%s]\n",
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

                SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from code. targetkey[%s] value[%d]\n",
                    __FUNCTION__, itr->strTargetKey.c_str(), def_value.nLastCode);
            }
        }
        else if (itr->enValueType == VTYPE_DATA_REQUEST)
        {
            SDefValue& def_value = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.vecValue.clear();
            m_pReqHandler->GetValues(itr->strSourceKey.c_str(), def_value.vecValue, def_value.enType);

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from date request value. targetkey[%s] value size[%u] value type[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.vecValue.size(), def_value.enType);
        }
        else if (itr->enValueType == VTYPE_DATA_RESPONSE)
        {
            SDefValue& def_value = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.vecValue.clear();
            m_pRspHandler->GetValues(itr->strSourceKey.c_str(), def_value.vecValue, def_value.enType);

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from date response value. targetkey[%s] value size[%u] value type[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.vecValue.size(), def_value.enType);
        }
        else if (itr->enValueType == VTYPE_DATA_CODE)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            def_value.enSourceType = VTYPE_LASTCODE;
            def_value.nLastCode    = m_reqResultCode;
            def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from date request code. targetkey[%s] value[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.nLastCode);
        }
        else if (itr->enValueType == VTYPE_DATA_DEFVALUE)
        {
            SDefValue& def_value   = _def_values[itr->strTargetKey];
            def_value.enSourceType = itr->enValueType;
            def_value.enType       = sdo::commonsdk::MSG_FIELD_INT;
            def_value.vecValue.clear();

            map<string, vector<string> >::const_iterator itrDefValue =  m_reqDefValue.find(itr->strSourceKey);
            if (itrDefValue != m_reqDefValue.end())
            {
                vector<string>::const_iterator itrValue = itrDefValue->second.begin();
                for (; itrValue != itrDefValue->second.end(); ++itrValue)
                {
                    SStructValue sStructValue;
                    sStructValue.pValue = (void*)(itrValue->c_str());
                    sStructValue.nLen   = itrValue->size();
                    def_value.vecValue.push_back(sStructValue);
                    def_value.enType = sStructValue.nLen == sizeof(int) ? def_value.enType : sdo::commonsdk::MSG_FIELD_STRING;
                }
            }

            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Set def value which is from data def value. targetkey[%s] value size[%u] value type[%d]\n",
                __FUNCTION__, itr->strTargetKey.c_str(), def_value.vecValue.size(), def_value.enType);
        }
        else
        {
            SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, unknow value type. It's not a def value. source key[%s], target key[%s]\n",
                __FUNCTION__, itr->strSourceKey.c_str(), itr->strTargetKey.c_str());
            continue;
        }
    }
}

void CComposeDataServiceObj::DoSaveResponseXhead(const SValueInCompose& value_in_compose)
{
    SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, value_node[%s][%d], value_seq[%s][%d]\n",
        __FUNCTION__, value_in_compose.sNode.strIndex.c_str(), value_in_compose.sNode.nIndex, value_in_compose.sSeq.strIndex.c_str(), value_in_compose.sSeq.nIndex);

    int* appid  = NULL;
    int* areaid = NULL;
    const char* name = NULL;
    const char* socid = NULL;
    int id = -1;
    string strName;
    if (value_in_compose.strTargetKey.find("appid") == 0)
    {
        appid = &id;
    }
    else if (value_in_compose.strTargetKey.find("areaid") == 0)
    {
        areaid = &id;
    }
    else if (value_in_compose.strTargetKey.find("name") == 0)
    {
        name = strName.c_str();
    }
    else if (value_in_compose.strTargetKey.find("socid") == 0)
    {
        socid = strName.c_str();
    }
    else
    {
        SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, Update xhead faild. target key[%s]\n",
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
    else if (value_in_compose.enValueType == VTYPE_DATA_REQUEST)
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
    else if (value_in_compose.enValueType == VTYPE_DATA_RESPONSE)
    {
        SDefValue  def_value;
        m_pRspHandler->GetValues(value_in_compose.strSourceKey.c_str(), def_value.vecValue, def_value.enType);
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
    else if (value_in_compose.enValueType == VTYPE_DATA_DEFVALUE)
    {
        map<string, vector<string> >::const_iterator itrDefValue =  m_reqDefValue.find(value_in_compose.strSourceKey);
        if (itrDefValue != m_reqDefValue.end() && !itrDefValue->second.empty())
        {
            if (appid || areaid)
            {
                id = *(int*)itrDefValue->second[0].c_str();
            }
            else
            {
                strName = itrDefValue->second[0];
            }
        }
    }
    else
    {
        SS_XLOG(XLOG_DEBUG,"CComposeDataServiceObj::%s, unknow value type. target[%s], source[%s], type[%d], node[%s][%d], seq[%s][%d], isdef[%s] .\n",
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

void CComposeDataServiceObj::RecordComposeService(int nCode) const
{
    SS_XLOG(XLOG_TRACE,"CComposeDataServiceObj::%s\n",__FUNCTION__);  
    if (!IsEnableLevel(SOS_DATA_MODULE, XLOG_INFO))
    {
        return;
    }

    struct tm tmStart, tmNow;
    struct timeval_a now,interval;
    m_pManager->gettimeofday(&now);
    localtime_r(&(m_tmStart.tv_sec), &tmStart);
    localtime_r(&(now.tv_sec), &tmNow);
    timersub(&now,&m_tmStart, &interval);    
    unsigned dwInterMS = interval.tv_sec*1000+interval.tv_usec/1000; 
    const string strStartTm = TimeToString(tmStart,m_tmStart.tv_usec/1000);
    const string strNowTm   = TimeToString(tmNow,now.tv_usec/1000);

    char szBuf[MAX_LOG_BUFFER] = {0};

    snprintf(szBuf, sizeof(szBuf)-1, "%s,  %s,  "\
        "%s,  %d,  %d,  %s,  "\
        "%s,  %d.%03ld,  %d\n",		
        strStartTm.c_str(),
        m_pExHead->getIp().c_str(),
        m_pExHead->getGuid(),
        //m_dwServiceSeq,
        m_rootServId,
        m_rootMsgId,
        CAsyncLogThread::GetInstance()->GetLocalIP().c_str(),
        //strNowTm.c_str(),
        //"",
        m_logInfo.logInfo.c_str(),
        dwInterMS,
        interval.tv_usec%1000,
        nCode);
    szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
    CAsyncLogThread::GetInstance()->OnAsyncLog(SOS_DATA_MODULE,XLOG_INFO,0,dwInterMS,szBuf,0,
        m_pExHead->getIp(),m_rootServId,m_rootMsgId,true);
}
