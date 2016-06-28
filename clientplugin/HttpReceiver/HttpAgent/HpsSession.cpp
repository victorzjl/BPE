#include "HpsSession.h"
#include "HpsRequestMsg.h"
#include "HpsCommon.h"
#include "HpsLogHelper.h"
#include "HpsStack.h"
#include "HpsCommonFunc.h"
#include "HpsValidator.h"

using namespace sdo::hps;

CHpsSession::CHpsSession(unsigned int dwId,HpsConnection_ptr ptrConn,CHpsSessionContainer *pContainer):
	m_dwId(dwId),m_eConnectStatus(CONNECT_CREATED_FIRST), m_ptrConn(ptrConn), m_pContainer(pContainer),m_nOrder(0)
{
	HP_XLOG(XLOG_DEBUG,"---------CHpsSession::%s-----------\n",__FUNCTION__);
}

CHpsSession::~CHpsSession()
{
	HP_XLOG(XLOG_DEBUG,"---------CHpsSession::%s-----------\n",__FUNCTION__);
	m_ptrConn->SetOwner(NULL);
	m_ptrConn->OnPeerClose();
	m_dequeResponse.clear();
}

void CHpsSession::HandleRequest(SRequestInfo &sReq)
{
    HP_XLOG(XLOG_DEBUG,"CHpsSession::%s\n", __FUNCTION__);
    
    m_pContainer->RequestPrePrecess(sReq);

    /* for apipool, add the method to url to get the serviceId msgId*/
    string strUrlTmp = sReq.strUriCommand;
    string strMethod = sdo::hps::CHpsRequestMsg::GetValueByKey(sReq.strUriAttribute, "method");
    if (strMethod.length() > 0)
    {
        if (strUrlTmp.length() > 0 && strUrlTmp[strUrlTmp.length()-1] != '/')
        {
                strUrlTmp += '/';
        }
        strUrlTmp += strMethod;
    }
    
    unsigned int nServiceID,nMsgID;
    transform(strUrlTmp.begin(), strUrlTmp.end(), strUrlTmp.begin(), (int(*)(int))tolower);
    int nResult = m_pContainer->FindUrlMapping(strUrlTmp,nServiceID,nMsgID);
    if(nResult != 0)
    {
        strUrlTmp = sReq.strUriCommand;
        transform(strUrlTmp.begin(), strUrlTmp.end(), strUrlTmp.begin(), (int(*)(int))tolower);
        nResult = m_pContainer->FindUrlMapping(strUrlTmp,nServiceID,nMsgID);
        if (nResult != 0)
        {
            sReq.isNomalBpeRequest = false;
        }
//        if(nResult != 0)
//        {
//            EErrorCode errCode = ERROR_URL_NOT_SUPPORT;
//            if (type == E_JsonRpc_Request)
//            {
//                errCode = ERROR_JSONRPC_METHOD_NOT_FOUND;
//            }
//
//            char szErrorCodeNeg[16] = {0};
//            snprintf(szErrorCodeNeg, 15, "%d", errCode);
//            sReq.strResponse = string("{\"return_code\":") + szErrorCodeNeg 
//                                  + ",\"return_message\":\"" +  CErrorCode::GetInstance()->GetErrorMessage(errCode) 
//                                   + "\",\"data\":{}}";
//            sReq.isVaild = false;
//            sReq.nCode = errCode;
//            return;
//        }
    }
    sReq.strUrlIdentify = strUrlTmp;

    sReq.nServiceId = nServiceID;
    sReq.nMsgId = nMsgID;

    if( MAX_SIZE_OF_DEQUE <= m_dequeResponse.size())
    { 
        char szErrorCodeNeg[16] = {0};
        snprintf(szErrorCodeNeg, 15, "%d", ERROR_HPS_OUTOF_QUEUE_MAXSIZE);
        sReq.strResponse = string("{\"return_code\":") + szErrorCodeNeg 
                                  + ",\"return_message\":\"" +  CErrorCode::GetInstance()->GetErrorMessage(ERROR_HPS_OUTOF_QUEUE_MAXSIZE) 
                                  + "\",\"data\":{}}";
        sReq.isVaild = false;
        sReq.nCode = ERROR_HPS_OUTOF_QUEUE_MAXSIZE;
    }	
}

void CHpsSession::OnReceiveHttpRequest(char * pBuff, int nLen,const string & strRemoteIp,unsigned int dwRemotePort)
{
    HP_XLOG(XLOG_DEBUG,"CHpsSession::%s,bufLen[%d],strRemoteIp[%s],dwRemotePort[%u]\n",__FUNCTION__,
            nLen, strRemoteIp.c_str(), dwRemotePort);

    m_eConnectStatus = CONNECT_CONNECTED;

    SDecodeResult result;
    CRequestDecoder::GetInstance()->Decode(result, pBuff, nLen, strRemoteIp, dwRemotePort);

    if (!result.isSuccessful)
    {
        HP_XLOG(XLOG_DEBUG,"CHpsSession::%s, parse http content fail\n", __FUNCTION__);
        SendErrorResponse(result);
        m_ptrConn->OnPeerClose();
        return;
    }

    sdo::hps::CSapBatResponseHandle batResponseHandle;
    int orderNo = m_nOrder++;
    batResponseHandle.requestType = result.requestType;
    batResponseHandle.orderNo = orderNo;
    batResponseHandle.headerInfo = result.headerInfo;

    //check all requests to find out if method is supported and if request can be handled. 
    std::vector<SRequestInfo>::iterator iter = result.requests.begin();
    while (iter != result.requests.end())
    {
        if (CHpsValidator::IsNeedToValidate(iter->strUrlIdentify)
            && !CHpsValidator::ValidateHttpParams(iter->strUriAttribute))
        {
            HP_XLOG(XLOG_DEBUG, "CHpsSession::%s, Http request may be a attack.\n", __FUNCTION__);
            result.errorCode = ERROR_HPS_ERROR_ATTACK;
            SendErrorResponse(result);
            m_ptrConn->OnPeerClose();
            return;
        }

        iter->nOrder = orderNo;
        iter->dwId = m_dwId;
        
        SHpsResponseHandle sHandle;
        sHandle.nOrder = iter->nOrder;
        sHandle.sequenceNo = iter->sequenceNo;
        sHandle.isResponseReady = false;
        sHandle.nCode = 0;
        
        if (iter->isVaild)
        {
            HandleRequest(*iter);
            if (iter->isVaild)
            {
                sHandle.isKeepAlive = true;
                sHandle.nHttpVersion = iter->m_nHttpVersion;
            }
        }
        else
        {
            char szErrorCodeNeg[16] = {0};
            snprintf(szErrorCodeNeg, 15, "%d", iter->nCode);
            iter->strResponse = string("{\"return_code\":") + szErrorCodeNeg 
                                  + ",\"return_message\":\"" +  CErrorCode::GetInstance()->GetErrorMessage(iter->nCode) 
                                   + "\",\"data\":{}}";
        }

        batResponseHandle.AddHandle(sHandle);

        ++iter;
    }
    
    m_dequeResponse.push_back(batResponseHandle);
    

    std::pair<std::map<int, sdo::hps::CSapBatResponseHandle>::iterator, bool> insertResult = 
                                            m_batResponseHandle.insert(std::make_pair(orderNo, batResponseHandle));

    //check all requests, if it is vaild, send it. 
    iter = result.requests.begin();
    while (iter != result.requests.end())
    {
        if (iter->isVaild)
        { 
            m_pContainer->OnReceiveHttpRequest(*iter, strRemoteIp, dwRemotePort);
        }
        else
        {
            DoSendResponse(*iter);
            CHpsSessionManager::RecordeHttpRequest(*iter);
        }
        ++iter;
    }

    return;
}

void CHpsSession::OnHttpPeerClose(const string & strRemoteIp,const unsigned int dwRemotePort)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSession::%s,strRemoteIp[%s],dwRemotePort[%u]\n",__FUNCTION__,strRemoteIp.c_str(),dwRemotePort);
	m_pContainer->OnHttpPeerClose(m_dwId,strRemoteIp,dwRemotePort);
	m_pContainer->DelConnection(m_dwId);
}

void CHpsSession::Close()
{
	HP_XLOG(XLOG_DEBUG,"CHpsSession::%s\n",__FUNCTION__);
	m_pContainer->OnHttpPeerClose(m_dwId,m_ptrConn->GetRemoteIp(),m_ptrConn->GetRemotePort());
	m_ptrConn->SetOwner(NULL);
	m_ptrConn->OnPeerClose();
}

void CHpsSession::DoSendResponse(const SRequestInfo &sReq)
{ 
    HP_XLOG(XLOG_DEBUG,"CHpsSession::%s,nCode[%d]\n",__FUNCTION__,sReq.nCode);
    if(!m_dequeResponse.empty())
    {
        string strContentType = sReq.strContentType;
        if (sReq.nCode == 0 && sReq.nServiceId ==41641 && sReq.nMsgId==1)
        {
                strContentType = "image/jpeg";   //图片验证码GET需返回 图片类型
        }
        deque<sdo::hps::CSapBatResponseHandle>::iterator iter = m_dequeResponse.begin();
        if((*iter).orderNo == sReq.nOrder)
        {
            CSapBatResponseHandle& batResponseHandle = *iter;
            iter->HandleResponse(sReq);
            if (iter->IsCompleted())
            {
                //HP_XLOG(XLOG_DEBUG,"========>CHpsSession -- DoSendResponse - completed\n");
                m_ptrConn->DoSendResponse(batResponseHandle.headerInfo.isKeepAlive, 
                                          batResponseHandle.headerInfo.nHttpVersion, 
                                          batResponseHandle.headerInfo.strResCharSet, 
                                          strContentType,
                                          batResponseHandle.GenerateResponse(), 
                                          batResponseHandle.headerInfo.nHttpCode, 
                                          batResponseHandle.headerInfo.vecSetCookie, 
                                          batResponseHandle.headerInfo.strLocationAddr, 
                                          batResponseHandle.headerInfo.strHeaders);
                m_dequeResponse.pop_front();
            }


            while( !m_dequeResponse.empty())
            {
                iter =  m_dequeResponse.begin();
                batResponseHandle = *iter;
                if (iter->IsCompleted())
                {
                    //HP_XLOG(XLOG_DEBUG,"========>CHpsSession -- DoSendResponse - completed\n");
                    m_ptrConn->DoSendResponse(batResponseHandle.headerInfo.isKeepAlive, 
                                              batResponseHandle.headerInfo.nHttpVersion, 
                                              batResponseHandle.headerInfo.strResCharSet, 
                                              strContentType,           
                                              batResponseHandle.GenerateResponse(), 
                                              batResponseHandle.headerInfo.nHttpCode, 
                                              batResponseHandle.headerInfo.vecSetCookie, 
                                              batResponseHandle.headerInfo.strLocationAddr, 
                                              batResponseHandle.headerInfo.strHeaders);
                    m_dequeResponse.pop_front();
                }
                else
                {
                        break;
                }
            }
        }
        else
        {
            for(++iter; iter != m_dequeResponse.end(); ++iter)
            {
                if((*iter).orderNo == sReq.nOrder)
                {
                    iter->HandleResponse(sReq);    
                    break;
                }
            }
        }
    }

    return;
}
void CHpsSession::SendAllResponse()
{
    HP_XLOG(XLOG_DEBUG,"CHpsSession::%s\n",__FUNCTION__);
    while (!m_dequeResponse.empty())
    {
        deque<sdo::hps::CSapBatResponseHandle>::iterator iter = m_dequeResponse.begin();
        sdo::hps::CSapBatResponseHandle &batResponseHandle = *iter;
        if (!batResponseHandle.IsCompleted())
        {
            batResponseHandle.Interrupt(); 
        }
        
        m_ptrConn->DoSendResponse(batResponseHandle.headerInfo.isKeepAlive, batResponseHandle.headerInfo.nHttpVersion, batResponseHandle.headerInfo.strResCharSet, batResponseHandle.headerInfo.strContentType, 
                                        batResponseHandle.GenerateResponse(), batResponseHandle.headerInfo.nHttpCode, batResponseHandle.headerInfo.vecSetCookie, batResponseHandle.headerInfo.strLocationAddr, batResponseHandle.headerInfo.strHeaders);
        m_dequeResponse.pop_front();
    }
}
void CHpsSession::SendErrorResponse( SRequestInfo &sReq, ERequestType type)
{
    HP_XLOG(XLOG_DEBUG,"CHpsSession::%s,nCode[%d]\n",__FUNCTION__,sReq.nCode);
    string strErrorMsg = CErrorCode::GetInstance()->GetErrorMessage(sReq.nCode);

    char szErrorCodeNeg[16] = {0};
    snprintf(szErrorCodeNeg, 15, "%d", sReq.nCode);
    sReq.strResponse = string("{\"return_code\":") + szErrorCodeNeg + ",\"return_msg\":\"" + strErrorMsg + "\",\"data\":{}}";

    HP_XLOG(XLOG_ERROR,"CHpsConnection::%s::%d, nCode[%d] strerrmsg[%s]\n",__FUNCTION__,__LINE__,sReq.nCode, strErrorMsg.c_str());


    sdo::hps::CSapBatResponseHandle batResponseHandle;
    batResponseHandle.orderNo = sReq.nOrder;
    batResponseHandle.requestType = type;
        
    SHpsResponseHandle sHandle;
    sHandle.nOrder = sReq.nOrder;
    sHandle.strResponse = sReq.strResponse;
    sHandle.isResponseReady = true;
    sHandle.nCode = sReq.nCode;
    sHandle.nHttpVersion = sReq.m_nHttpVersion;
    sHandle.isKeepAlive = sReq.isKeepAlive;
    sHandle.strResCharSet = sReq.strResCharSet;
    sHandle.nHttpCode = sReq.nHttpCode;
    sHandle.strContentType = sReq.strContentType;


    batResponseHandle.AddHandle(sHandle);
    m_dequeResponse.push_back(batResponseHandle);
    DoSendResponse(sReq);
	CHpsSessionManager::RecordeHttpRequest(sReq);

}

void CHpsSession::SendErrorResponse(SDecodeResult &sResult)
{
    HP_XLOG(XLOG_DEBUG, "CHpsSession::%s\n", __FUNCTION__);
    if (E_JsonRpc_Request == sResult.requestType)
    {
        char szErrorCodeMsg[16] = { 0 };
        snprintf(szErrorCodeMsg, 15, "%d", sResult.errorCode);
        m_ptrConn->DoSendResponse(false, sResult.headerInfo.nHttpVersion, sResult.headerInfo.strResCharSet, sResult.headerInfo.strContentType,
            string("{\"jsonrpc\": \"2.0\", \"error\": {\"return_code\":") + szErrorCodeMsg + ", \"return_message\": \"" + CErrorCode::GetInstance()->GetErrorMessage(sResult.errorCode) + "\"}, \"id\": null}",
            200, sResult.headerInfo.vecSetCookie, sResult.headerInfo.strLocationAddr, sResult.headerInfo.strHeaders);
    }
    else
    {
        m_ptrConn->DoSendResponse(false, sResult.headerInfo.nHttpVersion, sResult.errorCode);
    }
}

void CHpsSession::Dump()
{
	HP_XLOG(XLOG_NOTICE,"  ==============CHpsSession::%s,m_dwId[%u]]=========\n",__FUNCTION__,m_dwId);
	deque<sdo::hps::CSapBatResponseHandle>::iterator iter;
	HP_XLOG(XLOG_NOTICE,"           m_dequeResponse.size[%d]\n", m_dequeResponse.size());
	for(iter = m_dequeResponse.begin(); iter != m_dequeResponse.end(); ++iter)
	{
		HP_XLOG(XLOG_NOTICE,"           order[%d], IsCompleted\n",(*iter).orderNo);
	}
}



