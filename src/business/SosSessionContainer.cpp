#include "SosSessionContainer.h"
#include "SapLogHelper.h"
#include <algorithm>
#include "SessionManager.h"

CSosSessionContainer::CSosSessionContainer(const vector<unsigned int> &vecServiceId)
: m_vecServiceId(vecServiceId)
{
}

CSosSessionContainer::~CSosSessionContainer()
{
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);
    boost::unordered_map<string,CSosSession *>::iterator itrSos;
    for(itrSos=m_mapSession.begin();itrSos!=m_mapSession.end();)
    {
        boost::unordered_map<string,CSosSession *>::iterator itrTmp=itrSos++;
        CSosSession * pSession=itrTmp->second;
        m_mapSession.erase(itrTmp->first);
        delete pSession;
    }
}

void CSosSessionContainer::AddSos(const string &strAddr,CSosSession *pSos, int &nIndex)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);
	m_mapSession[strAddr]=pSos;
	pSos->SetIndex(nIndex++);
	m_nSos=m_mapSession.size();
	m_itr=m_mapSession.begin();
}

void CSosSessionContainer::GetAllSos(boost::unordered_map<string,CSosSession *> &mapSos)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);
	mapSos.insert(m_mapSession.begin(), m_mapSession.end());
	m_mapSession.clear();
}

void CSosSessionContainer::ReloadSos(const vector<unsigned int> vecServiceId,const vector<string> & vecAddr, CSessionManager *pManager,int &nIndex)
{
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);
	 m_vecServiceId = vecServiceId;
	boost::unordered_map<string,CSosSession *>::iterator itrSos;
	for(itrSos=m_mapSession.begin();itrSos!=m_mapSession.end();)
	{
		boost::unordered_map<string,CSosSession *>::iterator itrTmp=itrSos++;
		if(std::binary_search(vecAddr.begin(),vecAddr.end(),itrTmp->first)==false)
		{
			SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s,delete serviceid[%s], addr[%s]\n",__FUNCTION__,SSosStruct::FormatServiceId(m_vecServiceId).c_str(),itrTmp->first.c_str());
			CSosSession * pSession=itrTmp->second;
			m_mapSession.erase(itrTmp);
			delete pSession;
		}
	}

	vector<string>::const_iterator itrAddr;
	for(itrAddr=vecAddr.begin();itrAddr!=vecAddr.end();++itrAddr)
	{
		string strAddr=*itrAddr;
		if(m_mapSession.find(strAddr)==m_mapSession.end())
		{
			SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s,add serviceid[%s], addr[%s]\n",__FUNCTION__,SSosStruct::FormatServiceId(m_vecServiceId).c_str(),strAddr.c_str());

			char szIp[128]={0};
			unsigned int dwPort;
			sscanf(strAddr.c_str(),"%127[^:]:%u",szIp,&dwPort);
			CSosSession *pSos=new CSosSession(strAddr,vecServiceId,szIp,dwPort,pManager);
			pSos->Init();
			m_mapSession[strAddr]=pSos;
		}
	}
    for(boost::unordered_map<string,CSosSession *>::iterator itrSos=
		m_mapSession.begin(); itrSos != m_mapSession.end(); ++itrSos)
    {
        CSosSession * pSession=itrSos->second;
        pSession->SetIndex(nIndex++);
    }
    
    m_nSos=m_mapSession.size();
    m_itr=m_mapSession.begin();
}

CSosSession * CSosSessionContainer::GetSos()
{
    CSosSession * pSos=NULL;
    for(int i=0;i<m_nSos;++i)
    {
        if(m_itr==m_mapSession.end())
        {
            m_itr=m_mapSession.begin();
        }
        pSos=m_itr->second;
        ++m_itr;
        if(pSos->State()==CSosSession::E_SOS_CONNECTED)
        {
            SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, return[%x]\n",__FUNCTION__,pSos);
            return pSos;
        }
    }
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, return[NULL]\n",__FUNCTION__);
    return NULL;
}

void CSosSessionContainer::ReportSocOnline(const string &strName,const string &strAddr,
    unsigned int dwType,const string & strVersion, const string & strBuildTime)
{
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s,strName[%s],strAddr[%s],deType[%d],sdkversion[%s],buildtime[%s]\n",
        __FUNCTION__,strName.c_str(),strAddr.c_str(),dwType,strVersion.c_str(),strBuildTime.c_str());
    boost::unordered_map<string,CSosSession *>::iterator itr;
    CSosSession * pSos=NULL;
    SapConnection_ptr ptrConn;
    CSapEncoder oRequest;
    oRequest.SetService(SAP_PACKET_REQUEST,SAP_PLATFORM_SERVICE_ID,SAP_MSG_ReportSocState);
    oRequest.SetValue(SAP_Attri_userName,strName);
    oRequest.SetValue(SAP_Attri_online,1);
    oRequest.SetValue(SAP_Attri_addr,strAddr);
    oRequest.SetValue(SAP_Attri_state,dwType);
    oRequest.SetValue(SAP_Attri_version,strVersion);
    oRequest.SetValue(SAP_Attri_buildTime,strBuildTime);
    
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();++itr)
    {
        pSos=itr->second;
        if(pSos->State()==CSosSession::E_SOS_CONNECTED)
        {
            pSos->OnTransferRequest(SAP_Request_Local,ptrConn,oRequest.GetBuffer(),oRequest.GetLen());
        }
    }
}

void CSosSessionContainer::ReportSocDownline(const string &strName,const string &strAddr,unsigned int dwType)
{
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s,strName[%s],strAddr[%s],deType[%d]\n",__FUNCTION__,
        strName.c_str(),strAddr.c_str(),dwType);
    boost::unordered_map<string,CSosSession *>::iterator itr;
    CSosSession * pSos=NULL;
    SapConnection_ptr ptrConn;
    CSapEncoder oRequest;
    oRequest.SetService(SAP_PACKET_REQUEST,SAP_PLATFORM_SERVICE_ID,SAP_MSG_ReportSocState);
    oRequest.SetValue(SAP_Attri_userName,strName);
    oRequest.SetValue(SAP_Attri_online,0);
    oRequest.SetValue(SAP_Attri_addr,strAddr);
    oRequest.SetValue(SAP_Attri_state,dwType);
    
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();++itr)
    {
        pSos=itr->second;
        if(pSos->State()==CSosSession::E_SOS_CONNECTED)
        {
            pSos->OnTransferRequest(SAP_Request_Local,ptrConn,oRequest.GetBuffer(),oRequest.GetLen());
        }
    }
}

void CSosSessionContainer::OnStopComposeService()
{
    SS_XLOG(XLOG_TRACE,"CSosSessionContainer::%s...\n",__FUNCTION__);    
    boost::unordered_map<string,CSosSession *>::iterator itr;
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();++itr)
    {
        CSosSession *pSos=itr->second;
        pSos->OnStopComposeService();
    }        
}

void CSosSessionContainer::DoSelfCheck(vector<SSosStatusData> &vecDeadSos, int & nDeadSos,int &nAliveSos)
{
    SS_XLOG(XLOG_TRACE,"CSosSessionContainer::%s,this[%x]\n",__FUNCTION__,this);
    CSosSession * pSos=NULL;
    boost::unordered_map<string,CSosSession *>::iterator itr;
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();++itr)
    {
        pSos=itr->second;
        if(pSos->State()==CSosSession::E_SOS_CONNECTED && 
			pSos->GetSendErrorCount() <= MAX_ERRORS)
        {
            nAliveSos++;
        }
        else
        {
            SSosStatusData tmp;
            tmp.vecServiceId=m_vecServiceId;
            tmp.nIndex=pSos->Index();
            tmp.strAddr=pSos->Addr();
            vecDeadSos.push_back(tmp);
            nDeadSos++;
        }
    }        
}


void CSosSessionContainer::Dump()
{
    SS_XLOG(XLOG_NOTICE,"===========CSosSessionContainer::Dump [%d] serviceId[%s]=========\n",
		m_mapSession.size(), SSosStruct::FormatServiceId(m_vecServiceId).c_str());
    boost::unordered_map<string,CSosSession *>::const_iterator itr;
    for(itr=m_mapSession.begin();itr!=m_mapSession.end();++itr)
    {
        CSosSession *pSos=itr->second;
        pSos->Dump();
        
    }
}

