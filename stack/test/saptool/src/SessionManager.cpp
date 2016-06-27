#include "SessionManager.h"
#include "tinyxml/tinyxml.h"
#include "TestLogHelper.h"
#include "SapCommon.h"
#include "SapAgent.h"
using namespace sdo::sap;

static int nBeforeResponse=0;
static int nAfterResponse=0;


const CSessionManager::SessionMsgFunc CSessionManager::m_funcMap[E_SESSION_ALL_MSG]=
{
    NULL,
    &CSessionManager::DoStartClient_,
    &CSessionManager::DoSessionTimeout_,
    &CSessionManager::DoConnectResultMsg_,
    &CSessionManager::DoReceiveMsg_,
    &CSessionManager::DoPeerCloseMsg_,
    &CSessionManager::DoReceiveConnectMsg_,
    &CSessionManager::DoSendCmdMsg_
};
CSessionManager * CSessionManager::sm_pinstance=NULL;
CSessionManager * CSessionManager::Instance()
{
    if(sm_pinstance==NULL)
        sm_pinstance=new CSessionManager;
    return sm_pinstance;
}
CSessionManager::CSessionManager():m_nAppId(0),m_nAreaId(0)
{
}
string CSessionManager::GetUserPass(const string &strName)
{
    boost::unordered_map<string,string>::iterator itr=m_mapUsers.find(strName);
    if(itr!=m_mapUsers.end())
        return itr->second;
    else
        return "";
}
int CSessionManager::StartServer()
{
    return CSapAgent::Instance()->StartServer(m_nListenPort);
}
void CSessionManager::StartClient()
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::StartClient\n");
    
    SSessionStartMsg *pMsg=new SSessionStartMsg;
    PutQ(pMsg);
}
void CSessionManager::OnSessionTimeout(int nId)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::OnSessionTimeout,id[%d]\n",nId);
    
    SSessionTimeoutMsg *pMsg=new SSessionTimeoutMsg;
    pMsg->m_nId=nId;
    PutQ(pMsg);
}
void CSessionManager::OnSendSessionRequest(int nId,const SSapCommandTest *pCmd)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::OnSendSessionRequest,id[%d]\n",nId);
    
    SSessionSendCmdMsg *pMsg=new SSessionSendCmdMsg;
    pMsg->m_nId=nId;
    pMsg->m_pCmd=pCmd;
    
    PutQ(pMsg);
}

void CSessionManager::OnConnectResult(int nId,int nResult)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::OnConnectResult,id[%d],nResult[%d]\n",nId,nResult);
    
    SSapConnectResultMsg *pMsg=new SSapConnectResultMsg;
    pMsg->m_nId=nId;
    pMsg->m_nResult=nResult;
    PutQ(pMsg);
}
void CSessionManager::OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::OnReceiveConnection,id[%d],peer[%s:%d]\n",nId,strRemoteIp.c_str(),dwRemotPort);
    SSapReceiveConnectMsg *pMsg=new SSapReceiveConnectMsg;
    pMsg->m_nId=nId;
    pMsg->m_strIp=strRemoteIp;
    pMsg->m_dwPort=dwRemotPort;
    PutQ(pMsg);
}
void CSessionManager::OnReceiveMessage(int nId,const void *pBuffer, int nLen,const std::string& strRemoteIp, unsigned int dwRemotePort)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::OnReceiveMessage,id[%d]\n",nId);

    TEST_XLOG(XLOG_ERROR,"nBeforeResponse,%d\n",++nBeforeResponse);
    
    SSapReceiveMsg *pMsg=new SSapReceiveMsg;
    pMsg->m_nId=nId;
    pMsg->m_pBuffer=malloc(nLen);
    memcpy(pMsg->m_pBuffer,pBuffer,nLen);
    pMsg->m_nLen=nLen;
    PutQ(pMsg);
}
void CSessionManager::OnPeerClose(int nId)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::OnPeerClose,id[%d]\n",nId);
    
    SSapPeerCloseMsg *pMsg=new SSapPeerCloseMsg;
    pMsg->m_nId=nId;
    PutQ(pMsg);
}

void CSessionManager::Deal(void *pData)
{
    SSessionMsg *pMsg=(SSessionMsg *)pData;
    (this->*(m_funcMap[pMsg->m_enType]))(pMsg);
    delete pMsg;
}
void CSessionManager::DoStartClient_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    vector<SSocConfUnit *>::const_iterator itr;
    for(itr=m_vecUnits.begin();itr!=m_vecUnits.end();++itr)
    {
        SSocConfUnit *pTmp=*itr;
        for(int i=0;i<pTmp->m_nTimes;++i)
        {
            int nId=CSapAgent::Instance()->DoConnect(m_strServerIp,m_nServerPort,5,pTmp->m_nPort);
            CreateSession(nId,m_strServerIp,m_nServerPort,pTmp,m_mapResponse);
        }
    }
}
void CSessionManager::DoSessionTimeout_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    SSessionTimeoutMsg *pDetail=(SSessionTimeoutMsg *)pMsg;
    CSessionAdapter *pSession=GetSession(pDetail->m_nId);
    if(pSession!=NULL)
        pSession->OnSessionTimeout();
}
void CSessionManager::DoConnectResultMsg_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    SSapConnectResultMsg *pDetail=(SSapConnectResultMsg *)pMsg;
    if(pDetail->m_nResult!=0)
    {
        CSessionAdapter *pSession=GetSession(pDetail->m_nId);
        if(pSession!=NULL)
        {
            SSocConfUnit *pUnit=pSession->ConfUnit();
            DelSession(pDetail->m_nId);
            TEST_XLOG(XLOG_TRACE,"CSessionManager::%s,punit[%0x:%d]\n",__FUNCTION__,pUnit,pUnit!=NULL?pUnit->m_type:0);
            if(pUnit!=NULL&&pUnit->m_type==E_EXECUTE_CIRCLE)
            {
                int nId=CSapAgent::Instance()->DoConnect(m_strServerIp,m_nServerPort,5,pUnit->m_nPort);
                CreateSession(nId,m_strServerIp,m_nServerPort,pUnit,m_mapResponse);
            }
            else
            {
                DelSession(pDetail->m_nId);
            }
        }
        
    }
    else
    {
        CSessionAdapter *pSession=GetSession(pDetail->m_nId);
        if(pSession!=NULL)
            pSession->OnConnectSucc();
    }
}
void CSessionManager::DoReceiveMsg_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);

    TEST_XLOG(XLOG_ERROR,"nAfterResponse,%d\n",++nAfterResponse);
    
    SSapReceiveMsg *pDetail=(SSapReceiveMsg *)pMsg;
    CSessionAdapter *pSession=GetSession(pDetail->m_nId);
    if(pSession!=NULL)
        pSession->OnReceiveMsg(pDetail->m_pBuffer,pDetail->m_nLen);

    free(pDetail->m_pBuffer);
    
}
void CSessionManager::DoPeerCloseMsg_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    SSapPeerCloseMsg *pDetail=(SSapPeerCloseMsg *)pMsg;
    CSessionAdapter *pSession=GetSession(pDetail->m_nId);
    if(pSession!=NULL)
    {
        SSocConfUnit *pUnit=pSession->ConfUnit();
        DelSession(pDetail->m_nId);
        TEST_XLOG(XLOG_TRACE,"CSessionManager::%s,punit[%0x:%d]\n",__FUNCTION__,pUnit,pUnit!=NULL?pUnit->m_type:0);
        if(pUnit!=NULL&&pUnit->m_type==E_EXECUTE_CIRCLE)
        {
            int nId=CSapAgent::Instance()->DoConnect(m_strServerIp,m_nServerPort,5,pUnit->m_nPort);
            CreateSession(nId,m_strServerIp,m_nServerPort,pUnit,m_mapResponse);
        }
    }
}
void CSessionManager::DoReceiveConnectMsg_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    SSapReceiveConnectMsg *pDetail=(SSapReceiveConnectMsg *)pMsg;
    CreateSession(pDetail->m_nId,pDetail->m_strIp,pDetail->m_dwPort,m_mapResponse);
}
void CSessionManager::DoSendCmdMsg_(SSessionMsg * pMsg)
{
    TEST_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    SSessionSendCmdMsg *pDetail=(SSessionSendCmdMsg *)pMsg;
    CSessionAdapter *pSession=GetSession(pDetail->m_nId);
    if(pSession!=NULL)
        pSession->OnSendMsg(pDetail->m_pCmd);
}

/*map operator*/
CSessionAdapter * CSessionManager::CreateSession(int nId,const string &strIp,unsigned int dwPort,const map<int,SSapCommandResponseTest *> &mapResponse)
{
    CSessionAdapter *pSession=new CSessionAdapter(nId,strIp,dwPort,mapResponse);
    m_mapSession[nId]=pSession;
    return pSession;
}

CSessionAdapter * CSessionManager::CreateSession(int nId,const string &strIp,unsigned int dwPort,SSocConfUnit * pConfUnit,const map<int,SSapCommandResponseTest *> &mapResponse)
{
    CSessionAdapter *pSession=new CSessionAdapter(nId,strIp,dwPort,pConfUnit,mapResponse);
    m_mapSession[nId]=pSession;
    return pSession;
}
CSessionAdapter * CSessionManager::GetSession(int nId)
{
    CSessionAdapter *pSession=NULL;
    map<int,CSessionAdapter *>::iterator itr=m_mapSession.find(nId);
    if(itr!=m_mapSession.end())
    {
        pSession=itr->second;
    }
    return pSession;
}
void CSessionManager::DelSession(int nId)
{
    map<int,CSessionAdapter *>::iterator itr=m_mapSession.find(nId);
    if(itr!=m_mapSession.end())
    {
        CSessionAdapter *pSession=itr->second;
        delete pSession;
    }
    m_mapSession.erase(nId);
}
void CSessionManager::DelSession(CSessionAdapter *pSession)
{
    m_mapSession.erase(pSession->Id());
    delete pSession;
}
string CSessionManager::GetAllSessions()const
{
    string strResult="  sessionId  type    addr\n";
    map<int,CSessionAdapter *>::const_iterator itr;
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();itr++)
    {
        CSessionAdapter *pSession=itr->second;
        char szBuf[128]={0};
        sprintf(szBuf,"    %2d        %s    %s:%d\n",
            pSession->Id(),pSession->GetType()==E_SESSION_SOC?"out":"in ",pSession->GetPeerIp().c_str(),pSession->GetPeerPort());
        strResult+=szBuf;
    }
    return strResult;    
}

void CSessionManager::Dump()const
{
    TEST_XLOG(XLOG_INFO,"----------CSessionManager Begin-----------\n");
    TEST_XLOG(XLOG_INFO,"    server[%s:%d],unitNums[%d],session[%d]\n",m_strServerIp.c_str(),m_nServerPort,m_vecUnits.size(),m_mapSession.size());
    map<int,CSessionAdapter *>::const_iterator itr;
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();itr++)
    {
        CSessionAdapter *pSession=itr->second;
        pSession->Dump();
    }
    TEST_XLOG(XLOG_INFO,"----------CSessionManager End-----------\n");
}

