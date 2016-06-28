#include "SosSession.h"
#include "SapLogHelper.h"
#include "SosSessionContainer.h"
#include "AsyncLogThread.h"
#include <boost/lexical_cast.hpp>

using boost::lexical_cast;


CSosSession::CSosSession(unsigned int dwIndex,CHpsSessionManager *pManager,CSosSessionContainer *pSosSessionContainer,const string& strIp, unsigned int dwPort):
	m_bReConnectFlag(false),m_dwId(dwIndex),m_pSessionManager(pManager),m_pSosSessionContainer(pSosSessionContainer), m_strIp(strIp), m_dwPort(dwPort),m_eConnectStatus(E_SOS_DISCONNECTED),
	m_tmReconnect(m_pSessionManager,3,boost::bind(&CSosSession::DoConnect,this),CThreadTimer::TIMER_ONCE),
	m_tmConnTimeout(m_pSessionManager,6,boost::bind(&CSosSession::OnConnectTimeout,this),CThreadTimer::TIMER_ONCE)
{
}

CSosSession::~CSosSession()
{
	SS_XLOG(XLOG_DEBUG,"----------CSosSession::%s-----------\n",__FUNCTION__);
	m_ptrConn->Close();
	m_ptrConn->SetOwner(NULL);
}


void CSosSession::DoConnect()
{
	SS_XLOG(XLOG_DEBUG,"CSosSession::%s\n",__FUNCTION__);
    m_ptrConn.reset(new CSapConnection(m_pSessionManager->GetIoService(),m_pSessionManager,this));
    m_ptrConn->AsyncConnect(m_strIp,m_dwPort,5);

	if(m_bReConnectFlag)
	{
		SS_XLOG(XLOG_ERROR,"CSosSession::%s, ip[%s:%d], connect error, reconnecting ....\n",__FUNCTION__, m_strIp.c_str(), m_dwPort);
		CHpsAsyncLogThread::GetInstance()->OnReConnWarn(m_strIp + ":" + lexical_cast<string>(m_dwPort));
		m_bReConnectFlag = false;
	}

	m_eConnectStatus = E_SOS_CONNECTING;
	m_tmConnTimeout.Start();
}

void CSosSession::OnConnectResult(unsigned int dwConnId,int nResult)
{
	SS_XLOG(XLOG_DEBUG,"CSosSession::%s,nId[%d],nResult[%d]\n",__FUNCTION__,dwConnId,nResult);
	m_tmConnTimeout.Stop();
	m_dwConnId=dwConnId;
	if(nResult != 0)
	{
		SS_XLOG(XLOG_WARNING,"CSosSession::%s,connect faild. addr[%s:%u]\n",__FUNCTION__,m_strIp.c_str(),m_dwPort);
		m_eConnectStatus = E_SOS_DISCONNECTED;
		m_bReConnectFlag = true;
		m_tmReconnect.Start();
	}
	else
	{
		m_eConnectStatus = E_SOS_CONNECTED;
		m_pSosSessionContainer->OnConnected(m_dwId, dwConnId);
	}
}

void CSosSession::OnConnectTimeout()
{
	m_ptrConn->Close();
	m_ptrConn->SetOwner(NULL);
	
	m_eConnectStatus = E_SOS_DISCONNECTED;
	m_bReConnectFlag = true;
	m_tmReconnect.Start();
}


void CSosSession::OnPeerClose(unsigned int nId)
{
	SS_XLOG(XLOG_DEBUG,"CSosSession::%s,nId[%d]\n",__FUNCTION__,nId);
	m_pSessionManager->OnPeerClose(nId);
	m_eConnectStatus = E_SOS_DISCONNECTED;
	m_bReConnectFlag = true;
	m_tmReconnect.Start();
}


void CSosSession::OnReceiveSosAvenueResponse(unsigned int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort)
{
	SS_XLOG(XLOG_DEBUG,"CSosSession::%s,nId[%d]\n",__FUNCTION__,nId);
	m_pSessionManager->OnReceiveSosAvenueResponse(nId,pBuffer,nLen,strRemoteIp,dwRemotePort);
}

void CSosSession::OnReceiveAvenueRequest(unsigned int dwConnId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort)
{
	SS_XLOG(XLOG_DEBUG,"CSosSession::%s,connId[%u] remoteIp[%s], remotePort[%u]\n",__FUNCTION__,dwConnId,strRemoteIp.c_str(),dwRemotePort);
	m_pSosSessionContainer->OnReceiveAvenueRequest(m_dwId,dwConnId,pBuffer,nLen,strRemoteIp,dwRemotePort);
}


void CSosSession::Close()
{
	SS_XLOG(XLOG_DEBUG,"CSosSession::%s\n",__FUNCTION__);
	m_eConnectStatus = E_SOS_DISCONNECTED;
	m_ptrConn->Close();
	m_ptrConn->SetOwner(NULL);
}

void CSosSession::DoSendRequest(const void *pBuffer,int nLen,const void * exHeader,int nExLen,const char* pSignatureKey,unsigned int dwTimeout)
{
	if(nExLen > 0)
	{
		m_ptrConn->WriteDataEx(pBuffer, nLen, exHeader, nExLen, pSignatureKey, dwTimeout);
	}
	else
	{
		m_ptrConn->WriteData(pBuffer, nLen, pSignatureKey,dwTimeout);
	}
}

bool CSosSession::IsConnected()
{
	if(m_eConnectStatus == E_SOS_CONNECTED)
		return true;
	else
		return false;
}

void CSosSession::Dump()
{	
	SS_XLOG(XLOG_NOTICE,"========CSosSession::%s, sosAddr[%s:%u]=====m_eConnectStatus[%d]=======\n",
		__FUNCTION__,m_strIp.c_str(),m_dwPort,m_eConnectStatus);
}

	

