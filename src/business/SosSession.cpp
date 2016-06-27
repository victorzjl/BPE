#include "SosSession.h"
#include "SapLogHelper.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"

CSosSession::CSosSession(const string &strAddr,const vector<unsigned int>& vecServiceId,const string &strIP, unsigned int dwPort,CSessionManager *pManager):
    ITransferObj("CSosSession",SOS_AUDIT_MODULE,0,strAddr,pManager),IComposeBaseObj("CSosSession",pManager,SOC_AUDIT_MODULE),m_vecServiceId(vecServiceId),m_pManager(pManager),m_strIp(strIP),m_dwPort(dwPort),m_seq(0),
    m_tmDisconnect(m_pManager,3,boost::bind(&CSosSession::DoConnect,this),CThreadTimer::TIMER_ONCE),
    m_tmConnecting(m_pManager,6,boost::bind(&CSosSession::OnConnectTimeout,this),CThreadTimer::TIMER_ONCE)
{

}
CSosSession::~CSosSession()
{
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);

    m_ptrConn->Close();
    m_ptrConn->SetOwner(NULL);
    m_pManager->OnDeleteSos(this);
}
void CSosSession::Init()
{
    SS_XLOG(XLOG_DEBUG,"CSosSession::%s\n",__FUNCTION__);
    DoConnect();
}

void CSosSession::OnReadCompleted(const void *pBuffer, int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSosSession::%s,buffer[%x],nlen[%d]\n",__FUNCTION__,pBuffer,nLen);
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
     
    if(pHead->byIdentifer==SAP_PACKET_REQUEST)
    {
        if(ProcessComposeRequest(pBuffer,nLen,NULL,0,
            m_ptrConn->GetRemoteIp(), m_ptrConn->GetRemotePort())==-1)
        {
            m_pManager->OnReceiveRequest(m_ptrConn,pBuffer,nLen);
        }
    }
    else 
    {
        OnTransferResponse(pBuffer, nLen);
    }
}

void CSosSession::OnConnectResult(int nResult)
{
    SS_XLOG(XLOG_DEBUG,"CSosSession::%s,result[%d]\n",__FUNCTION__,nResult);
    m_tmConnecting.Stop();
    if(nResult==0)
    {
        m_state=E_SOS_CONNECTED;
        m_pManager->OnConnected(this,m_vecServiceId);
    }
    else
    {
        m_state=E_SOS_DISCONNECTED;
        CAsyncLogThread::GetInstance()->ReportError(m_strAddr);
        m_tmDisconnect.Start();
    }
}
void CSosSession::OnPeerClose()
{
    SS_XLOG(XLOG_DEBUG,"CSosSession::%s\n",__FUNCTION__);
    m_state=E_SOS_DISCONNECTED;
    CAsyncLogThread::GetInstance()->ReportError(m_strAddr);
	m_nSendErrorCount=0;
    m_tmDisconnect.Start();
}
void CSosSession::OnConnectTimeout()
{
    SS_XLOG(XLOG_DEBUG,"CSosSession::%s\n",__FUNCTION__);
    m_ptrConn->Close();
	m_nSendErrorCount=0;
    m_state=E_SOS_DISCONNECTED;
    m_tmDisconnect.Start();
}
void CSosSession::DoConnect()
{
    SS_XLOG(XLOG_DEBUG,"CSosSession::%s\n",__FUNCTION__);
	m_ptrConn.reset(new CSapConnection(m_pManager->GetIoService(),m_pManager));
    m_ptrConn->SetOwner(this);
    m_ptrConn->AsyncConnect(m_strIp,m_dwPort,5);
    m_state=E_SOS_CONNECTING;
    m_tmConnecting.Start();
}

void CSosSession::Dump()
{
    SS_XLOG(XLOG_NOTICE,"#sos#    manager[%x],addr[%s:%d],seq[%u],state[%d]\n",
        m_pManager,m_strIp.c_str(),m_dwPort,m_seq,m_state);
    m_ptrConn->Dump();
    IComposeBaseObj::Dump();

    SS_XLOG(XLOG_NOTICE,"        diconnectTimer:\n");
    //m_tmDisconnect.Dump();
    SS_XLOG(XLOG_NOTICE,"        dconnectingTimer:\n");
//	m_tmConnecting.Dump();
    SS_XLOG(XLOG_NOTICE,"        requestTimer:\n");
//	m_tmRequest.Dump();
}

