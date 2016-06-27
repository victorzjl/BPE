#include "SessionAdapter.h"
#include "SessionSoc.h"
#include "SessionSos.h"
#include "TestLogHelper.h"

CSessionAdapter::CSessionAdapter(int nId,const string &strIp,unsigned int dwPort,SSocConfUnit * pConfUnit,const map<int,SSapCommandResponseTest *> &mapResponse):
    m_nId(nId),m_strIp(strIp),m_dwPort(dwPort),m_type(E_SESSION_SOC)
{   
    m_pSession=new CSessionSoc(nId,pConfUnit,mapResponse);
}
CSessionAdapter::CSessionAdapter(int nId,const string &strIp,unsigned int dwPort,const map<int,SSapCommandResponseTest *> &mapResponse):
    m_nId(nId),m_strIp(strIp),m_dwPort(dwPort),m_type(E_SESSION_SOS)
{
    m_pSession=new CSessionSos(nId,mapResponse);
}
SSocConfUnit *CSessionAdapter::ConfUnit()const
{
    if(m_type==E_SESSION_SOC)
        return ((CSessionSoc *)m_pSession)->ConfUnit();
    else
        return NULL;
}
CSessionAdapter::~CSessionAdapter()
{
    delete m_pSession;
}

void CSessionAdapter::OnSessionTimeout()
{
    if(m_pSession)
        m_pSession->OnSessionTimeout();
}
void CSessionAdapter::OnConnectSucc()
{
    if(m_pSession)
        m_pSession->OnConnectSucc();
}
void CSessionAdapter::OnReceiveMsg(void *pBuffer, int nLen)
{
    if(m_pSession)
        m_pSession->OnReceiveMsg(pBuffer,nLen);
}
void CSessionAdapter::OnSendMsg(const SSapCommandTest * pCmd)
{
    if(m_pSession)
        m_pSession->OnSendMsg(pCmd);
}

void CSessionAdapter::Dump()
{
    TEST_XLOG(XLOG_INFO,"id[%d],type[%d],peer[%s:%d]\n",m_nId,m_type,m_strIp.c_str(),m_dwPort);
    if(m_pSession)
        m_pSession->Dump();
}

