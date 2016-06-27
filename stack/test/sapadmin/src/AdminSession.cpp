#include "stdafx.h"
#include "AdminSession.h"
#include "SapAgent.h"
#include "AdminLogHelper.h"
#include "SapMessage.h"
#include "MemoryPool.h"
#include "Cipher.h"
#include "SapCommon.h"
using namespace sdo::common;
using namespace sdo::sap;

CAdminSession::CAdminSession()
{
    m_funcMap[ADMIN_LOGIN_MSG]=&CAdminSession::DoAdminLoginMsg;
    m_funcMap[ADMIN_CONNECT_RESULT_MSG]=&CAdminSession::DoConnectResultMsg;
    m_funcMap[ADMIN_RECEIVE_MESSAGE_MSG]=&CAdminSession::DoReceiveMsg;
    m_funcMap[ADMIN_PEER_CLOSE_MSG]=&CAdminSession::DoPeerCloseMsg;
    m_funcMap[ADMIN_SEND_REQUEST_MSG]=&CAdminSession::DoRequestMsg;
}

void CAdminSession::OnLoginServer(const string &strIp,int nPort,const string &strName,const string &strPass)
{
    A_XLOG(XLOG_DEBUG,"CAdminSession::OnLoginServer,addr[%s:%d],login[%s:%s]\n",
        strIp.c_str(),nPort,strName.c_str(),strPass.c_str());
    
    SAdminLoginMsg *pMsg=new SAdminLoginMsg;
    pMsg->m_strIp=strIp;
    pMsg->m_nPort=nPort;
    pMsg->m_strName=strName;
    pMsg->m_strPass=strPass;

    PutQ(pMsg);
}
void CAdminSession::OnSendRequest(const string &strRequest)
{
    A_XLOG(XLOG_DEBUG,"CAdminSession::OnSendRequest,request[%s]\n",strRequest.c_str());
    SAdminRequestMsg *pMsg=new SAdminRequestMsg;
    pMsg->m_strRequest=strRequest;

    PutQ(pMsg);
}

void CAdminSession::OnConnectResult(int nId,int nResult)
{
    
    A_XLOG(XLOG_DEBUG,"CAdminSession::OnConnectResult,id[%d],result[%d]\n",
            nId,nResult);

    SAdminConnectResultMsg *pMsg=new SAdminConnectResultMsg;
    pMsg->m_nId=nId;
    pMsg->m_nResult=nResult;

    PutQ(pMsg);
}
void CAdminSession::OnReceiveMessage(int nId,const void *pBuffer, int nLen)
{
    A_XLOG(XLOG_DEBUG,"CAdminSession::OnReceiveMessage,id[%d]\n",nId);
    
    SAdminReceiveMsg *pMsg=new SAdminReceiveMsg;
    pMsg->m_nId=nId;
    pMsg->m_pBuffer=CMemoryPool::Instance()->Malloc(nLen);
    memcpy(pMsg->m_pBuffer,pBuffer,nLen);
    pMsg->m_nLen=nLen;

    PutQ(pMsg);
}
void CAdminSession::OnPeerClose(int nId)
{
    A_XLOG(XLOG_DEBUG,"CAdminSession::OnPeerClose,id[%d]\n",nId);
    SdminPeerCloseMsg *pMsg=new SdminPeerCloseMsg;
    pMsg->m_nId=nId;
    PutQ(pMsg);
}

void CAdminSession::Deal(void *pMsg)
{
    SAdminMsg *pAdminMsg=(SAdminMsg *)pMsg;
    (this->*(m_funcMap[pAdminMsg->m_enType]))(pAdminMsg);
    delete pAdminMsg;
}
void CAdminSession::DoAdminLoginMsg(SAdminMsg * pMsg)
{
    SAdminLoginMsg *pDetail=(SAdminLoginMsg *)pMsg;
    A_XLOG(XLOG_TRACE,"CAdminSession::DoAdminLoginMsg,addr[%s:%d],login[%s:%s]\n",
        pDetail->m_strIp.c_str(),pDetail->m_nPort,pDetail->m_strName.c_str(),pDetail->m_strPass.c_str());

    m_strName=pDetail->m_strName;
    m_strPass=pDetail->m_strPass;
    m_nId=CSapAgent::Instance()->DoConnect(pDetail->m_strIp,pDetail->m_nPort,5);
}
void CAdminSession::DoConnectResultMsg(SAdminMsg * pMsg)
{
    SAdminConnectResultMsg *pDetail=(SAdminConnectResultMsg *)pMsg;
    A_XLOG(XLOG_TRACE,"CAdminSession::DoConnectResultMsg,id[%d],result[%d]\n",
        m_nId,pDetail->m_nResult);

    if(pDetail->m_nResult!=0)
    {
        A_XLOG(XLOG_ERROR,"connect refused!\n");
        exit(-1);
    }

    CSapEncodeMsg msg;
    msg.m_pHeader->byIdentifer=SAP_PACKET_REQUEST;
    msg.m_pHeader->byServiceId=SAP_PLATFORM_SERVICE_ID;
    msg.m_pHeader->byMsgId=SAP_MSG_AdminRegister;
    msg.m_pHeader->dwSequence=CSapEncodeMsg::CreateSequence();
    msg.SetStrAttribute(SAP_Attri_userName,m_strName);
    msg.Encode();

    CSapAgent::Instance()->DoSendMessage(m_nId,msg.GetBuffer(),msg.GetLen());
}
void CAdminSession::DoReceiveMsg(SAdminMsg * pMsg)
{
    SAdminReceiveMsg *pDetail=(SAdminReceiveMsg *)pMsg;
    A_XLOG(XLOG_TRACE,"CAdminSession::DoReceiveMsg,id[%d]\n",m_nId);

    CSapDecodeMsg msgDecode(pDetail->m_pBuffer,pDetail->m_nLen);
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pDetail->m_pBuffer;
    
    if(pHeader->byIdentifer==SAP_PACKET_RESPONSE
        &&pHeader->byServiceId==SAP_PLATFORM_SERVICE_ID
        &&pHeader->byMsgId==SAP_MSG_AdminRegister
        &&pHeader->byCode==SAP_CODE_SUCC)
    {
        void *pEncKey;
        int nLen;
        if(msgDecode.Decode()!=0
            ||msgDecode.GetAttribute(SAP_Attri_encKey,&pEncKey,&nLen)!=0
            ||nLen!=16)
        {
            A_XLOG(XLOG_ERROR,"register,unregenize packet format!\n");
            exit(-1);
        }
        unsigned char szKey[16];
        memset(szKey,0,16);
        strncpy((char *)szKey,m_strPass.c_str(),16);
        CCipher::AesDec(szKey,(unsigned char *)pEncKey,nLen,m_szSessionKey);
        
        CSapEncodeMsg msg;
        msg.m_pHeader->byIdentifer=SAP_PACKET_REQUEST;
        msg.m_pHeader->byServiceId=SAP_PLATFORM_SERVICE_ID;
        msg.m_pHeader->byMsgId=SAP_MSG_AdminCommand;
        msg.m_pHeader->dwSequence=CSapEncodeMsg::CreateSequence();
        msg.SetStrAttribute(SAP_Attri_adminRequest,"help");
        msg.Encode(SAP_ENC_AES,m_szSessionKey);

        CSapAgent::Instance()->DoSendMessage(m_nId,msg.GetBuffer(),msg.GetLen());
    }
    else if(pHeader->byIdentifer==SAP_PACKET_RESPONSE
        &&pHeader->byServiceId==SAP_PLATFORM_SERVICE_ID
        &&pHeader->byMsgId==SAP_MSG_AdminCommand
        &&pHeader->byCode==SAP_CODE_SUCC)
    {
        string strResult;
        if(msgDecode.Decode(SAP_ENC_AES,m_szSessionKey)!=0
            ||msgDecode.GetStrAttribute(SAP_Attri_adminResponse,strResult)!=0)
        {
            A_XLOG(XLOG_ERROR,"request,unregenize packet format!\n");
            exit(-1);
        }
        
        if(pHeader->b==0)
        {
            printf("%s\n",strResult.c_str());
            printf("Sap>: ");
        }
        else
        {
            printf("%s",strResult.c_str());
        }
        fflush( stdout );
    }
    else
    {
        A_XLOG(XLOG_ERROR,"normal,unregenize packet format!,indentifer[%0x],service[%d],msg[%d],code[%d]\n",
            pHeader->byIdentifer,pHeader->byServiceId,pHeader->byMsgId,pHeader->byCode);
        exit(-1);
    }
    CMemoryPool::Instance()->Free(pDetail->m_pBuffer,pDetail->m_nLen);
}
void CAdminSession::DoPeerCloseMsg(SAdminMsg * pMsg)
{
    SdminPeerCloseMsg *pDetail=(SdminPeerCloseMsg *)pMsg;
    A_XLOG(XLOG_TRACE,"CAdminSession::DoPeerCloseMsg,id[%d]\n",m_nId);
    
    A_XLOG(XLOG_ERROR,"peer close connection!\n");
    exit(-1);
}
void CAdminSession::DoRequestMsg(SAdminMsg * pMsg)
{
    SAdminRequestMsg *pDetail=(SAdminRequestMsg *)pMsg;
    A_XLOG(XLOG_TRACE,"CAdminSession::DoRequestMsg,id[%d]\n",m_nId);

    CSapEncodeMsg msg;
    msg.m_pHeader->byIdentifer=SAP_PACKET_REQUEST;
    msg.m_pHeader->byServiceId=SAP_PLATFORM_SERVICE_ID;
    msg.m_pHeader->byMsgId=SAP_MSG_AdminCommand;
    msg.m_pHeader->dwSequence=CSapEncodeMsg::CreateSequence();
    msg.SetStrAttribute(SAP_Attri_adminRequest,pDetail->m_strRequest);
    msg.Encode(SAP_ENC_AES,m_szSessionKey);

    CSapAgent::Instance()->DoSendMessage(m_nId,msg.GetBuffer(),msg.GetLen());
}

