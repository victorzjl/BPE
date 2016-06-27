#include "SessionManager.h"
#include "SapLogHelper.h"
#include "SapCommon.h"
#include "SapStack.h"
#include <string.h>
#include <sched.h>
#include <boost/algorithm/string.hpp> 
#include <algorithm>
#include "AsyncLogHelper.h"
#include "Utility.h"
#include "AsyncLogThread.h"
#include "ComposeServiceObj.h"
#include "AvsSession.h"
#include "SosSessionContainer.h"
#include "SosSession.h"

const string GetAddr(SapConnection_ptr ptrConn)
{
    char szPort[16]={0};
	sprintf(szPort,"%d",ptrConn->GetRemotePort());
    return ptrConn->GetRemoteIp()+":"+szPort;
}

void dump_buffer(const string& strPacketInfo, const void *pBuffer);


CSessionManager::CSessionManager(unsigned int dwId):
    m_dwSocIndex(0),m_avs(NULL),m_tmTimeoOut(m_oIoService),bStopped(false), bRunning(false),m_isAlive(true), m_dwId(dwId),m_dwComposeServiceObjAlive(0),
	m_tmSendAckRequest(this,SEND_ACK_REQUEST_WAIT_SECOND,boost::bind(&CSessionManager::DoSendAckRequest,this),CThreadTimer::TIMER_CIRCLE),
    m_tmSelfCheck(this,MANAGER_SELFCHECK_INTERVAL,boost::bind(&CSessionManager::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE)
{
    gettimeofday_a(&m_timeReceive, NULL);
}
void CSessionManager::Start(unsigned int affinity)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    m_oWork=new boost::asio::io_service::work(m_oIoService);
    m_thread=boost::thread(boost::bind(&boost::asio::io_service::run,&m_oIoService)); 
    m_oIoService.post(boost::bind(&CSessionManager::StartInThread, this));

    bRunning=true;
    m_tmTimeoOut.expires_from_now(boost::posix_time::millisec(30));
	m_tmTimeoOut.async_wait(
        MakeSapAllocHandler(m_allocTimeOut,boost::bind(&CSessionManager::DoTimer, this)));
}

void CSessionManager::StartInThread()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    Reset();
    m_tmSelfCheck.Start();
	m_tmSendAckRequest.Start();
}
void CSessionManager::OnLoadSos(const vector<SSosStruct> & vecSos)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
     m_oIoService.post(boost::bind(&CSessionManager::DoLoadSos,this,vecSos));
}
void CSessionManager::OnLoadSuperSoc(const vector<string> & vecSoc,const vector<string> & strPrivelege)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSessionManager::DoLoadSuperSoc,this,vecSoc,strPrivelege));
}
void CSessionManager::OnReceiveSoc(SapConnection_ptr ptrConn)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSessionManager::DoReceiveSoc,this,ptrConn));
}
int CSessionManager::OnLoadConfig(const CComposeServiceContainer& oComposeConfig,
    const CAvenueServiceConfigs& oServiceConfig)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__); 
    m_oIoService.post(boost::bind(&CSessionManager::DoLoadConfig,this,oComposeConfig,oServiceConfig));
    return 0;
}

void CSessionManager::OnLoadVirtualService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSessionManager::DoLoadVirtualService,this));
}

void CSessionManager::OnLoadAsyncVirtualService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSessionManager::DoLoadAsyncVirtualService,this));
}

void CSessionManager::OnLoadAsyncVirtualClientService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    m_oIoService.post(boost::bind(&CSessionManager::DoLoadAsyncVirtualClientService,this));
}

void CSessionManager::OnRecvVirtualClientRequest(void* handle, const void * buffer, unsigned int len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    void * pBuffer = malloc(len);
    memcpy(pBuffer, buffer, len);
    m_oIoService.post(boost::bind(&CSessionManager::DoRecvVirtualClientRequest,this, handle, pBuffer, len));
}

void CSessionManager::OnRecvAVS(const void * buffer, unsigned int len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    void * pBuffer = malloc(len);
    memcpy(pBuffer, buffer, len);
    m_oIoService.post(boost::bind(&CSessionManager::DoRecvAVS,this, pBuffer, len));
}

void CSessionManager::OnRecvVirtualClient(const void * buffer, unsigned int len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    void * pBuffer = malloc(len);
    memcpy(pBuffer, buffer, len);
    m_oIoService.post(boost::bind(&CSessionManager::DoRecvVirtualClient,this, pBuffer, len));
}

void CSessionManager::OnSetConfig(const void* buffer, const unsigned len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s buff[%x], len[%u]\n",
        __FUNCTION__, buffer, len);
    void * pBuffer = malloc(len);
    memcpy(pBuffer, buffer, len);
    m_oIoService.post(boost::bind(&CSessionManager::DoSetConfig, this, pBuffer, len));
}

void CSessionManager::OnInstallSOSync(const string& strName, int &nRet)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);
    boost::mutex mut;
    boost::condition_variable cond;
    boost::unique_lock<boost::mutex> lock(mut);
    SVSReload *pVSReolad = new SVSReload(mut,cond);  
    m_oIoService.post(boost::bind(&CSessionManager::DoInstallSOSync,this,pVSReolad,strName));
    if(!cond.timed_wait<boost::posix_time::seconds>(lock, boost::posix_time::seconds(RELOAD_TIMEWAIT))) 
    {
        pVSReolad->isTimeout = true;          
	}    
    nRet = pVSReolad->nRet;
    delete pVSReolad;
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s end...\n",__FUNCTION__);    
}

void CSessionManager::OnUnInstallSOSync(const string& strName, int &nRet)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s start\n",__FUNCTION__);
    boost::mutex mut;
    boost::condition_variable cond;
    boost::unique_lock<boost::mutex> lock(mut);    
    SVSReload *pVSReolad = new SVSReload(mut,cond);    
    m_oIoService.post(boost::bind(&CSessionManager::DoUnInstallSOSync,this,pVSReolad,strName));
    if(!cond.timed_wait<boost::posix_time::seconds>(lock, boost::posix_time::seconds(RELOAD_TIMEWAIT))) 
    {
        pVSReolad->isTimeout = true;         
	} 
    nRet = pVSReolad->nRet;
    delete pVSReolad;
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s end\n",__FUNCTION__);
}

void CSessionManager::OnGetPluginInfo(string &strInfo)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    boost::mutex mut;
    boost::condition_variable cond;
    boost::unique_lock<boost::mutex> lock(mut);    
    SPlugInfo *pPlugInfo = new SPlugInfo(mut,cond); 
    m_oIoService.post(boost::bind(&CSessionManager::DoGetPluginInfo,this,pPlugInfo)); 
    if(!cond.timed_wait<boost::posix_time::seconds>(lock, boost::posix_time::seconds(GETINFO_TIMEWARI))) 
    {
        pPlugInfo->isTimeout = true;         
	} 
    strInfo = pPlugInfo->strInfo;
    delete pPlugInfo;    
}

void CSessionManager::OnGetConfigInfo(string &strInfo)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    boost::mutex mut;
    boost::condition_variable cond;
    boost::unique_lock<boost::mutex> lock(mut);    
    DoGetConfigInfo(strInfo);
}

void CSessionManager::DoTimer()
{
    //SS_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    gettimeofday(NULL, false);
    DetectTimerList();
    m_isAlive = true;
    if(bRunning)
    {
        m_tmTimeoOut.expires_from_now(boost::posix_time::millisec(30));
        m_tmTimeoOut.async_wait(
                MakeSapAllocHandler(m_allocTimeOut,boost::bind(&CSessionManager::DoTimer, this)));
    }
}

void CSessionManager::OnAckRequestEnqueue(void *pBuffer, int nLen)
{
	m_oIoService.post(boost::bind(&CSessionManager::DoAckRequestEnqueue,this,pBuffer,nLen));
}

void CSessionManager::DoAckRequestEnqueue(void *pBuffer, int nLen)
{
	SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
	SSapLenMsg msg;
	msg.pBuffer = pBuffer;
	msg.nLen = nLen;
	m_ackRequestQueue.push_back(msg);
}

void CSessionManager::DoSendAckRequest()
{	
	if(bRunning)
	{
        int count = 0;
		for (SapLenMsgQueue::iterator iter = m_ackRequestQueue.begin();
			count < 500 && iter != m_ackRequestQueue.end(); )
		{
            SForwardMsg *pForwardMsg = (SForwardMsg*)iter->pBuffer;            
            SSapMsgHeader *pHead=(SSapMsgHeader *)pForwardMsg->pBuffer;
			CSosSession *pSos = OnGetSos(ntohl(pHead->dwServiceId),ntohl(pHead->dwMsgId));			
			if (pSos != NULL)
			{
				pSos->OnTransferMapQRequest((void *)iter->pBuffer, iter->nLen);
				iter = m_ackRequestQueue.erase(iter);
                ++count;
			}
			else
			{
				++iter;                
			}
		}
	}
}

void CSessionManager::DoLoadSos(const vector<SSosStruct> &vecSos)
{
	SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);

	boost::unordered_map<string,CSosSession *> mapSos;
	for(vector<CSosSessionContainer *>::iterator iter = m_vecSos.begin();
		iter != m_vecSos.end(); ++iter)
	{
		(*iter)->GetAllSos(mapSos);
		delete (*iter);
	}
	m_vecSos.clear();
	m_mapSos.clear();

	for(boost::unordered_map<string,CSosSession *>::iterator itrSos=mapSos.begin();
		itrSos!=mapSos.end();)
	{
		boost::unordered_map<string,CSosSession *>::iterator itrTmp=itrSos++;
		vector<SSosStruct>::const_iterator itrConfig;
		for(itrConfig=vecSos.begin();itrConfig!=vecSos.end();++itrConfig)
		{
			const vector<string> &vecAddr = itrConfig->vecAddr;
			if(binary_search(vecAddr.begin(), vecAddr.end(), itrTmp->first))
			{
				break;
			}
		}
		if(itrConfig==vecSos.end())
		{
			SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,delete sos, addr[%s]\n",__FUNCTION__,itrTmp->first.c_str());
			CSosSession * pSession=itrTmp->second;
			mapSos.erase(itrTmp);
			delete pSession;
		}
	}

	int nIndex=1;
	for(vector<SSosStruct>::const_iterator itrConfig=vecSos.begin();
		itrConfig!=vecSos.end();++itrConfig)
	{
		const vector<string> &vecAddr = itrConfig->vecAddr;
		const vector<unsigned int> &vecServiceId = itrConfig->vecServiceId;
		const vector<TServiceIdMessageId> &vecServiceIdMessageId = itrConfig->vecServiceIdMessageId;
		CSosSessionContainer *pSosContainer = new CSosSessionContainer(vecServiceId);
		for(vector<string>::const_iterator itrAddr = vecAddr.begin();
			itrAddr!=vecAddr.end();++itrAddr)
		{
			string strAddr=*itrAddr;
			if(mapSos.find(strAddr) == mapSos.end())
			{
				SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,add sos, addr[%s]\n",__FUNCTION__,strAddr.c_str());

				char szIp[128] = {0};
				unsigned int dwPort;
				sscanf(strAddr.c_str(),"%127[^:]:%u",szIp,&dwPort);
				CSosSession *pSos = new CSosSession(strAddr,vecServiceId,szIp,dwPort,this);
				pSos->Init();
				mapSos[strAddr] = pSos;
			}
			pSosContainer->AddSos(strAddr, mapSos[strAddr], nIndex);
			//mapSos.erase(strAddr);
		}
		m_vecSos.push_back(pSosContainer);
		for(vector<unsigned int>::const_iterator itrSvrId=vecServiceId.begin();
			itrSvrId!=vecServiceId.end();++itrSvrId)
		{
			TServiceIdMessageId sm;
			sm.dwServiceId=*itrSvrId;
			sm.dwMessageId=ALL_MSG;
			m_mapSos[sm] = pSosContainer;
		}
		for(vector<TServiceIdMessageId>::const_iterator itrSvrMsgId=vecServiceIdMessageId.begin();
			itrSvrMsgId!=vecServiceIdMessageId.end();++itrSvrMsgId)
		{
			m_mapSos[*itrSvrMsgId] = pSosContainer;
		}
	}
}

void CSessionManager::DoLoadSuperSoc(const vector<string> & vecSoc,const vector<string> & vecPrivilege)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);
    m_vecSuperSoc.clear();
    m_vecSuperPrivilege.clear();
    
    m_vecSuperSoc=vecSoc;
    sort(m_vecSuperSoc.begin(),m_vecSuperSoc.end());
 
    vector<string>::const_iterator itrService;
    for(itrService=vecPrivilege.begin();itrService!=vecPrivilege.end();++itrService)
    {
        const string & strService=*itrService;
        SServicePrivilege obj;
        char szMsg[1024]={0};
        if(sscanf(strService.c_str(),"%u{%1023[^}]}",&(obj.dwServiceId),szMsg)==2)
        {
            vector<string> vecMsgId;
            boost::algorithm::split( vecMsgId, szMsg,boost::algorithm::is_any_of("/"),boost::algorithm::token_compress_on); 

            vector<string>::const_iterator itrMsgId;
            for(itrMsgId=vecMsgId.begin();itrMsgId!=vecMsgId.end();++itrMsgId)
            {
                const string & strMsgId=*itrMsgId;
                obj.vecMsgId.push_back(atoi(strMsgId.c_str()));
            }
            sort(obj.vecMsgId.begin(),obj.vecMsgId.end());
        }
        m_vecSuperPrivilege.push_back(obj);
    }
    sort(m_vecSuperPrivilege.begin(),m_vecSuperPrivilege.end());
}

void CSessionManager::DoReceiveSoc(SapConnection_ptr ptrConn)
{
    unsigned int dwIndex=m_dwSocIndex++;
    char szPort[16]={0};
    sprintf(szPort,"%d",ptrConn->GetRemotePort());
    string strAddr=ptrConn->GetRemoteIp()+":"+szPort;
    
    CSocSession * pSoc=new CSocSession(dwIndex,strAddr,ptrConn,this);
    pSoc->Init(IsSuperSoc(ptrConn->GetRemoteIp()));
    m_mapSoc[dwIndex]=pSoc;
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,index[%d], soc in[%x]\n",__FUNCTION__,dwIndex,pSoc);

    m_connData.dwConnect++;
    m_connData.dwClient++;
}

void CSessionManager::DoLoadConfig(CComposeServiceContainer oComposeConfig,CAvenueServiceConfigs oServiceConfig)
{    
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    DoStopComposeService();
    m_oComposeConfig = oComposeConfig;
    m_oServiceConfig = oServiceConfig;
    m_oLogTrigger.SetConfig(&m_oServiceConfig);
}

void CSessionManager::DoLoadVirtualService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    m_virtualServiceManager.LoadVirtualService();
	m_cppFunctionManager.LoadExecFunction();
}

void CSessionManager::DoLoadAsyncVirtualService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    m_asyncVS.Load();
    m_avs = new CAVSSession(this, m_asyncVS);
}

void CSessionManager::DoLoadAsyncVirtualClientService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    m_virtualClientLoader.Load();
	m_virtualClientSession = new CVirtualClientSession(this,m_virtualClientLoader);
}

void CSessionManager::DoTransferVirtualClientResponse(void* handle, const void * pBuffer, int nLen)
{
	SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
	m_virtualClientSession->DoTransferVirtualClientResponse(handle,pBuffer,nLen);
}

/****
function: DoRecvVirtualClientRequest
description: recv virtual client request from plugin
****/
void CSessionManager::DoRecvVirtualClientRequest(void* handle,void *pBuffer, unsigned nLen) 
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    dump_buffer("Read Sap Command from async virtual client\n", pBuffer);

    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwSeviceId=ntohl(pHead->dwServiceId);
	unsigned int dwMsgId=ntohl(pHead->dwMsgId);
	if (isInAsyncService(dwSeviceId))  /*deal with async message*/
	{       
        if (m_avs->OnTransferAVSRequest(pBuffer, nLen,handle) != 0)
        {
        	CSapEncoder oResponse;
	        oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_NOT_FOUND);
	        oResponse.SetSequence(ntohl(pHead->dwSequence));
			RecordRequest(VIRTUAL_CLIENT_ADDRESS,dwSeviceId,dwMsgId,SAP_CODE_NOT_FOUND);
            DoTransferVirtualClientResponse(handle,oResponse.GetBuffer(), oResponse.GetLen());
        }
	}
	else  if(m_virtualClientSession->ProcessComposeRequest(pBuffer,nLen,NULL,0,VIRTUAL_CLIENT_ADDRESS, 0,handle)==-1)  /*deal with compose message*/
	{
		CSosSession * pSosSession=NULL;
		/*deal with sos message*/
		pSosSession = OnGetSos(dwSeviceId,dwMsgId,NULL);
		if (pSosSession!=NULL)
		{
			pSosSession->OnTransferRequest(SAP_Request_VirtualClient,pBuffer,nLen,handle);
		}
		else
		{
			CSapEncoder oResponse;
			oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_NOT_FOUND);
			oResponse.SetSequence(ntohl(pHead->dwSequence));
			RecordRequest(VIRTUAL_CLIENT_ADDRESS,dwSeviceId,dwMsgId,SAP_CODE_NOT_FOUND);
			DoTransferVirtualClientResponse(handle,oResponse.GetBuffer(), oResponse.GetLen());
		}
	}
    free(pBuffer);
}

void CSessionManager::DoRecvVirtualClient(void *pBuffer, unsigned len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    dump_buffer("Read Sap Command from async virtual service\n", pBuffer);
    //gettimeofday(NULL, false);
    m_virtualClientSession->OnTransferResponse(pBuffer, len);
    free(pBuffer);
}

void CSessionManager::DoRecvAVS(void *pBuffer, unsigned len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    dump_buffer("Read Sap Command from async virtual service\n", pBuffer);
    //gettimeofday(NULL, false);
    m_avs->OnTransferResponse(pBuffer, len);
    free(pBuffer);
}

void CSessionManager::DoSetConfig(void *buffer, unsigned len)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    m_virtualServiceManager.SetData(buffer, len);
    free(buffer);
}

void CSessionManager::DoInstallSOSync(SVSReload *pReload,const string& strName)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s ...\n",__FUNCTION__);
    pReload->nRet = m_virtualServiceManager.LoadServiceByName(strName);    
    if(!pReload->isTimeout)
    {        
        boost::unique_lock<boost::mutex> lock(pReload->mut);
	    pReload->cond.notify_one();       
    }        
}

void CSessionManager::DoUnInstallSOSync(SVSReload* pReload,const string& strName)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);   
    pReload->nRet = m_virtualServiceManager.ReleaseServiceByName(strName);   
    if(!pReload->isTimeout)
    {
        boost::unique_lock<boost::mutex> lock(pReload->mut);
	    pReload->cond.notify_one();
    }  
}

void CSessionManager::DoGetPluginInfo(SPlugInfo *pPlugInfo)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);   
    pPlugInfo->strInfo = m_virtualServiceManager.OnGetPluginInfo();
    pPlugInfo->strInfo += m_asyncVS.GetPluginInfo();
    if(!pPlugInfo->isTimeout)
    {
        boost::unique_lock<boost::mutex> lock(pPlugInfo->mut);
	    pPlugInfo->cond.notify_one();
    }  
}

void CSessionManager::DoGetConfigInfo(string& config_info_)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);
    config_info_ = m_oComposeConfig.OnGetConfigInfo();
}

bool CSessionManager::IsSuperSoc(const string & strIp)
{
    return std::binary_search( m_vecSuperSoc.begin(), m_vecSuperSoc.end(), strIp );
}

bool CSessionManager::IsCanRequest(const string & strIp,unsigned int dwServiceId, unsigned int dwMsgId)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s, strip[%s],seviceid[%u],msgid[%u]\n",__FUNCTION__,strIp.c_str(),dwServiceId,dwMsgId);
    //if(std::binary_search( m_vecSuperSoc.begin(), m_vecSuperSoc.end(), strIp ) == true)
    //    return true;    

    vector<SServicePrivilege>::iterator itr=std::find( m_vecSuperPrivilege.begin(), m_vecSuperPrivilege.end(), dwServiceId );
    if(itr==m_vecSuperPrivilege.end())
        return true;

    SServicePrivilege &obj=*itr;
    if(obj.vecMsgId.size()!=0 && std::binary_search( obj.vecMsgId.begin(), obj.vecMsgId.end(), dwMsgId )==false) 
        return true;

    return false;
}

int CSessionManager::OnRegistedSoc(CSocSession * pSoc)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,index[%u],name[%s]\n",__FUNCTION__,pSoc->Index(),pSoc->UserName().c_str());
    boost::unordered_map<string,CSocSession * >::iterator itrSocName=m_mapRegistedSoc.find(pSoc->UserName());
    if(itrSocName!=m_mapRegistedSoc.end())
    {
        if(pSoc->LoginType()==CSocSession::E_REGISTED_SOC||pSoc->LoginType()==CSocSession::E_SUPER_SOC)
        {
            CSocSession * pLastSoc=itrSocName->second;
            OnDeleteSoc(pLastSoc);
            return 0;
        }
        else
        {
            SS_XLOG(XLOG_WARNING,"CSessionManager::%s,index[%u],name[%s],already registered, register failed\n",
                __FUNCTION__, pSoc->Index(),pSoc->UserName().c_str());
            return -1;
        }
    }
    else
    {
        return 0;
    }
}
void CSessionManager::OnReportRegisterSoc(CSocSession * pSoc)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,index[%u],name[%s]\n",__FUNCTION__,pSoc->Index(),pSoc->UserName().c_str());
    m_mapRegistedSoc[pSoc->UserName()]=pSoc;

	if(CSapStack::isAsc==0)
	{
		TServiceIdMessageId sm;
		sm.dwServiceId = SAP_SLS_SERVICE_ID;
		sm.dwMessageId = ALL_MSG;
		map<TServiceIdMessageId,CSosSessionContainer *>::iterator itr= m_mapSos.find(sm);
		if(itr!=m_mapSos.end())
		{
			CSosSessionContainer *pContainer=itr->second;
			pContainer->ReportSocOnline(pSoc->UserName(),pSoc->Addr(),pSoc->LoginType(),pSoc->SdkVersion(),pSoc->SdkBuildTime());
		}
	}
}

void CSessionManager::OnDeleteSoc(CSocSession * pSoc)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,index[%u],name[%s]\n",__FUNCTION__,pSoc->Index(),pSoc->UserName().c_str());
    
    m_mapSoc.erase(pSoc->Index());    
    if(pSoc->IsRegisted())
    {
        m_mapRegistedSoc.erase(pSoc->UserName());
		if(CSapStack::isAsc==0)
		{
			TServiceIdMessageId sm;
			sm.dwServiceId = SAP_SLS_SERVICE_ID;
			sm.dwMessageId = ALL_MSG;
			map<TServiceIdMessageId,CSosSessionContainer *>::iterator itr= m_mapSos.find(sm);
			if(itr!=m_mapSos.end())
			{
				CSosSessionContainer *pContainer=itr->second;
				pContainer->ReportSocDownline(pSoc->UserName(),pSoc->Addr(),pSoc->LoginType());
			}
		}
    }
    
    delete pSoc;
    m_connData.dwDisConnect++;
    m_connData.dwClient--;
}

void CSessionManager::OnDeleteSos(CSosSession * pSos)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,index[%u],addr[%s]\n",__FUNCTION__,pSos->Index(),pSos->Addr().c_str());
    map<unsigned int,CSocSession * >::iterator itr;
    for(itr=m_mapSoc.begin();itr!=m_mapSoc.end();++itr)
    {
        CSocSession * pSoc=itr->second;
        pSoc->DoDeleteSos(pSos);
    }
}

CSocSession* CSessionManager::OnGetSoc( const string& socName)
{
	SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,request soc,name[%s]\n",__FUNCTION__,socName.c_str());
	boost::unordered_map<string,CSocSession * >::iterator itr=m_mapRegistedSoc.find(socName);
	if(itr!=m_mapRegistedSoc.end())
	{
		return itr->second;
	}
	return NULL;
}

CSosSession * CSessionManager::OnReceiveRequest(SapConnection_ptr ptrConn,const void *pBuffer, int nLen, const void* pExhead, int nExlen)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s, pBuffer[%x], len[%d]\n",__FUNCTION__, pBuffer, nLen);
    CSosSession * pSosSession=NULL;
    
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwSeviceId=ntohl(pHead->dwServiceId);
	unsigned int dwMsgId=ntohl(pHead->dwMsgId);
    /*deal with platform request*/
    if(dwSeviceId==SAP_PLATFORM_SERVICE_ID)
    {
        CSapDecoder decode(pBuffer,nLen);
        decode.DecodeBodyAsTLV();
        
        string strUserName;
        if(dwMsgId==SAP_MSG_Kicksoc&&decode.GetValue(SMS_Attri_userName,strUserName)==0)
        {
            //kick soc & response ok
            boost::unordered_map<string,CSocSession * >::iterator itr=m_mapRegistedSoc.find(strUserName);
            if(itr!=m_mapRegistedSoc.end())
            {          
                CSocSession *pSoc=itr->second;
                SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,KickSoc,index[%u],name[%s]\n",__FUNCTION__,pSoc->Index(),pSoc->UserName().c_str());                
                m_mapRegistedSoc.erase(itr);
                m_mapSoc.erase(pSoc->Index());
                delete pSoc;                
            }
            
            CSapEncoder oResponse;
            oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,dwMsgId,SAP_CODE_SUCC);
            oResponse.SetSequence(ntohl(pHead->dwSequence));
            RecordRequest(GetAddr(ptrConn),dwSeviceId,dwMsgId,SAP_CODE_SUCC);
            ptrConn->WriteData(oResponse.GetBuffer(),oResponse.GetLen());
        }
        else
        {
            SS_XLOG(XLOG_WARNING,"CSessionManager::%s,not support, serviceid[%u], msgid[%u],strUserName[%s]\n",__FUNCTION__,dwSeviceId,dwMsgId,strUserName.c_str());
            CSapEncoder oResponse;
            oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,dwMsgId,SAP_CODE_NOT_SUPPORT);
            RecordRequest(GetAddr(ptrConn),dwSeviceId,dwMsgId,SAP_CODE_NOT_SUPPORT);
            ptrConn->WriteData(oResponse.GetBuffer(),oResponse.GetLen());
        }
        return NULL;
    }
    
    /*deal with delivering message*/
    if(dwSeviceId==SAP_SLS_SERVICE_ID)
    {
        if(pHead->byHeadLen < sizeof(SSapMsgHeader)+32)
        {
            SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,ext header len not less than 32,headlen[%d], should be[%d] \n",__FUNCTION__,pHead->byHeadLen,sizeof(SSapMsgHeader)+32);
            CSapEncoder oResponse;
            oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_BAD_REQUEST);
            RecordRequest(GetAddr(ptrConn),dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_BAD_REQUEST);
            ptrConn->WriteData(oResponse.GetBuffer(),oResponse.GetLen());
            return NULL;
        }
        else
        {
			if(CSapStack::isAsc==0)
			{
				char *szName=(char *)pBuffer+sizeof(SSapMsgHeader);
				CSocSession* pSoc = OnGetSoc(szName);
				if (NULL != pSoc)
				{
					pSoc->OnTransferRequest(SAP_Request_Remote, ptrConn, pBuffer,nLen,pExhead,nExlen);
					return NULL;
				}				
			}
        }
    }

	if (IsVirtualService(dwSeviceId))
	{           
        void *pOutPacket = NULL;
        int nOutLen = 0, nCode = 0;
        if(OnRequestVirtuelService(dwSeviceId,pBuffer,nLen,&pOutPacket, &nOutLen)!=0)
        {	
            nCode = SAP_CODE_VIRTUAL_SERVICE_BAD_REQUEST;
        }          
    	RecordBusinessReq(GetAddr(ptrConn),pBuffer,nLen, nCode,SOS_AUDIT_MODULE);
        ptrConn->WriteData(pOutPacket, nOutLen);                  
        return NULL;
	}
	if (isInAsyncService(dwSeviceId))
	{       
        if (m_avs->OnTransferAVSRequest(ptrConn, pBuffer, nLen) != 0)
        {
        	CSapEncoder oResponse;
	        oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_NOT_FOUND);
	        oResponse.SetSequence(ntohl(pHead->dwSequence));
            ptrConn->WriteData(oResponse.GetBuffer(), oResponse.GetLen());
        }
        return NULL;
        
	}
	if (isInAsyncVirtualClient(dwSeviceId))
	{       
        if (m_virtualClientSession->OnTransferAVSRequest(ptrConn, pBuffer, nLen) != 0)
        {
        	CSapEncoder oResponse;
	        oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_NOT_FOUND);
	        oResponse.SetSequence(ntohl(pHead->dwSequence));
            ptrConn->WriteData(oResponse.GetBuffer(), oResponse.GetLen());
        }
        return NULL;
	}
    /*deal with normal message*/
	pSosSession = OnGetSos(dwSeviceId,dwMsgId,NULL);

    if(pSosSession!=NULL)
    {
        if(dwSeviceId==SAP_SMS_SERVICE_ID&&dwMsgId==SMS_GET_CFG_PRIV)
        {
            pSosSession->OnTransferRequest(SAP_Request_Local,ptrConn,pBuffer,nLen);
        }
        else
        {
            pSosSession->OnTransferRequest(SAP_Request_Remote,ptrConn,pBuffer,nLen,pExhead,nExlen);
        }
    }
    else
    {   
        SS_XLOG(XLOG_WARNING,"CSessionManager::%s,can't find server, serviceid[%u]\n",__FUNCTION__,dwSeviceId);
        CSapEncoder oResponse;
        oResponse.SetService(SAP_PACKET_RESPONSE,dwSeviceId,ntohl(pHead->dwMsgId),SAP_CODE_NOT_FOUND);
        oResponse.SetSequence(ntohl(pHead->dwSequence));
    	if(ptrConn.get()!=NULL)
    	{
            RecordBusinessReq(GetAddr(ptrConn),pBuffer,nLen,SAP_CODE_NOT_FOUND,SOS_AUDIT_MODULE);
            ptrConn->WriteData(oResponse.GetBuffer(),oResponse.GetLen());
    	}
    	else
    	{
    		OnManagerResponse(oResponse.GetBuffer(),oResponse.GetLen());
    	}
    }
    return pSosSession;
}

//response msg sent by sps
void CSessionManager::OnManagerResponse(const void *pBuffer, int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,buffer[%x],len[%d]\n",__FUNCTION__,pBuffer,nLen);
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwSeviceId=ntohl(pHead->dwServiceId);
    unsigned int dwMsgId=ntohl(pHead->dwMsgId);
    unsigned int dwSequence=ntohl(pHead->dwSequence);   
    
    if(dwSeviceId==SAP_SMS_SERVICE_ID&&dwMsgId==SMS_GET_CFG_PRIV)
    {
        map<unsigned int,CSocSession * >::iterator itr=m_mapSoc.find(dwSequence);
        if(itr!=m_mapSoc.end())
        {
            CSocSession *pSoc=itr->second;
            pSoc->DoGetSocConfigResponse(pBuffer, nLen);
        }
        else
        {
            SS_XLOG(XLOG_WARNING,"CSessionManager::%s,no soc find\n",__FUNCTION__);
        }
    }
}

int CSessionManager::OnRequestVirtuelService(unsigned int nServiceId,
	const void *pInPacket, unsigned int nLen, void **ppOutPacket, int *pLen)
{
	return m_virtualServiceManager.OnRequestService(
		nServiceId, pInPacket, nLen, ppOutPacket, pLen);
}

void CSessionManager::GetSocConfig(unsigned int dwIndex,const string & strName)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s, index[%u],socName[%s]\n",__FUNCTION__,dwIndex,strName.c_str());
    CSapEncoder request;
    request.SetService(SAP_PACKET_REQUEST,SAP_SMS_SERVICE_ID,SMS_GET_CFG_PRIV);
    request.SetSequence(dwIndex);
    request.SetValue(SMS_Attri_userName,strName);
    SapConnection_ptr ptrConn;
    OnReceiveRequest(ptrConn,request.GetBuffer(),request.GetLen());
}
void CSessionManager::GetSocConfig(unsigned int dwIndex,const string &strIp,unsigned int port)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s, index[%u],addr[%s:%u]\n",__FUNCTION__,dwIndex,strIp.c_str(),port);
    CSapEncoder request;
    request.SetService(SAP_PACKET_REQUEST,SAP_SMS_SERVICE_ID,SMS_GET_CFG_PRIV);
    request.SetSequence(dwIndex);
    request.SetValue(SMS_Attri_ip,strIp);
    request.SetValue(SMS_Attri_port,port);
    SapConnection_ptr ptrConn;
    OnReceiveRequest(ptrConn,request.GetBuffer(),request.GetLen());
}
void CSessionManager::GetSocConfig(unsigned int dwIndex,const string & strName,const string &strIp,unsigned int port)
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s, index[%u],socName[%s],addr[%s:%u]\n",__FUNCTION__,dwIndex,strName.c_str(),strIp.c_str(),port);
    CSapEncoder request;
    request.SetService(SAP_PACKET_REQUEST,SAP_SMS_SERVICE_ID,SMS_GET_CFG_PRIV);
    request.SetSequence(dwIndex);
    request.SetValue(SMS_Attri_userName,strName);
    request.SetValue(SMS_Attri_ip,strIp);
    request.SetValue(SMS_Attri_port,port);
    SapConnection_ptr ptrConn;
    OnReceiveRequest(ptrConn,request.GetBuffer(),request.GetLen());
}
void CSessionManager::Stop()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s\n",__FUNCTION__);
    /*bRunning=false;
    boost::system::error_code ignore_ec;
    m_tmTimeoOut.cancel(ignore_ec);
    
    delete m_oWork;
    m_thread.join();
    m_oIoService.reset();*/

    map<unsigned int,CSocSession * >::iterator itr = m_mapSoc.begin();
    for (; itr != m_mapSoc.end(); ++itr)
    {
        itr->second->NotifyOffLine();
        delete itr->second;
    }
    m_mapSoc.clear();
    bStopped = true;
}

void CSessionManager::OnStop()
{
    m_oIoService.post(boost::bind(&CSessionManager::Stop, this));
}

void CSessionManager::OnConnected(CSosSession *pSos,const vector<unsigned int> &vecServiceId)
{   
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,index[%u],addr[%s],serviceid[%s]\n",
		__FUNCTION__,pSos->Index(),pSos->Addr().c_str(),SSosStruct::FormatServiceId(vecServiceId).c_str());

	if(CSapStack::isAsc==0)
	{
		if(std::binary_search(vecServiceId.begin(), vecServiceId.end(), SAP_SLS_SERVICE_ID))
		{
			SapConnection_ptr ptrConn;
			CSapEncoder oRequest;

			boost::unordered_map<string,CSocSession * >::iterator itr;
			CSocSession *pSoc=NULL;
			for(itr=m_mapRegistedSoc.begin();itr!=m_mapRegistedSoc.end();++itr)
			{   
				pSoc=itr->second;
				oRequest.Reset();
				oRequest.SetService(SAP_PACKET_REQUEST,SAP_PLATFORM_SERVICE_ID,SAP_MSG_ReportSocState);
				oRequest.SetValue(SAP_Attri_userName,pSoc->UserName());
				oRequest.SetValue(SAP_Attri_online,1);
				oRequest.SetValue(SAP_Attri_addr,pSoc->Addr());
				oRequest.SetValue(SAP_Attri_state,pSoc->LoginType());
				oRequest.SetValue(SAP_Attri_version, pSoc->SdkVersion());
				oRequest.SetValue(SAP_Attri_buildTime,pSoc->SdkBuildTime());

				pSos->OnTransferRequest(SAP_Request_Local,ptrConn,oRequest.GetBuffer(),oRequest.GetLen());
			}
		}
	}
}
bool CSessionManager::isInAsyncService(unsigned serviceId)
{
    return m_avs->isInService(serviceId);
};
bool CSessionManager::isInAsyncVirtualClient(unsigned serviceId)
{
	return m_virtualClientSession->isInService(serviceId);
}
CSosSession* CSessionManager::OnGetSos(const unsigned int dwServiceId,const unsigned int dwMessageId, bool *isSosCfg)const
{   
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s,dwServiceId[%d]\n",__FUNCTION__, dwServiceId);    
    
    CSosSession *pSosSession = NULL;
	TServiceIdMessageId sm;
	sm.dwServiceId=dwServiceId;
	sm.dwMessageId=dwMessageId;
	map<TServiceIdMessageId,CSosSessionContainer *>::const_iterator itr = m_mapSos.find(sm);
	if(itr != m_mapSos.end())
	{
		if (isSosCfg!=NULL)
		{
			*isSosCfg=true;
		}
		CSosSessionContainer *pContainer = itr->second;
		pSosSession = pContainer->GetSos();
	}
	else  if (dwMessageId!=ALL_MSG)
	{
		sm.dwMessageId=ALL_MSG;
		itr = m_mapSos.find(sm);
		if(itr != m_mapSos.end())
		{
			if (isSosCfg!=NULL)
			{
				*isSosCfg=true;
			}
			CSosSessionContainer *pContainer = itr->second;
			pSosSession = pContainer->GetSos();
		}
	}
    return pSosSession;
}

void CSessionManager::DoStopComposeService()
{
    SS_XLOG(XLOG_DEBUG,"CSessionManager::%s...\n",__FUNCTION__);
    boost::unordered_map<string,CSocSession * >::iterator itrSoc;
    for(itrSoc=m_mapRegistedSoc.begin(); itrSoc!=m_mapRegistedSoc.end(); ++itrSoc)
    {
        CSocSession *pSoc = itrSoc->second;
        pSoc->OnStopComposeService();
    }

	for (vector<CSosSessionContainer *>::iterator iter = m_vecSos.begin();
		iter != m_vecSos.end(); ++iter)
	{
		(*iter)->OnStopComposeService();
	}
}

void CSessionManager::DoSelfCheck()
{
    SS_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    vector<SSosStatusData> vecDeadSos;
    int nDeadSos=0;
    int nAliveSos=0;
    vector<CSosSessionContainer *>::const_iterator itrSos;
    for(itrSos=m_vecSos.begin();itrSos!=m_vecSos.end();++itrSos)
    {               
        (*itrSos)->DoSelfCheck(vecDeadSos,nDeadSos,nAliveSos);
    }
    //get cache info
    vector < SCacheSelfCheck > vecInfo;
    m_asyncVS.GetSelfCheck(vecInfo);
    
    CAsyncLogThread::GetInstance()->OnReportSosStatus(vecDeadSos,nDeadSos,nAliveSos,vecInfo,m_dwId);    
}


void CSessionManager::RecordBusinessReq(const string& strAddr, const void* pBuffer, int nLen, int nCode,
     int nLevel, bool isCompose)
{
    SS_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);
    if(IsEnableLevel(nLevel, XLOG_INFO))
	{
        struct tm tmNow;
    	struct timeval_a now;
        gettimeofday(&now);
        localtime_r(&(now.tv_sec), &tmNow);
        const string strNowTm = TimeToString(tmNow,now.tv_usec/1000);
        const SRequestInfo sInfo = GetRequestInfo(pBuffer,nLen);

        SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
        int nServId          = ntohl(pHead->dwServiceId);
        int nMsgId           = ntohl(pHead->dwMsgId);
        unsigned dwSeq       = ntohl(pHead->dwSequence);
        
        SLogInfo sLog;
        m_oLogTrigger.BeginTrigger(pBuffer,nLen,sLog);
        m_oLogTrigger.EndTrigger(NULL, 0, sLog);
        
        char szBuf[MAX_LOG_BUFFER]={0};
        //request time, sourceip,appid, areaid, username,old seq,new seq, 
    	//serviceid, msgid, response time, interval,node, seq, code
    	//snprintf(szBuf,sizeof(szBuf)-1,	"%-23s, %-20s, %4d, %4d, %-20s, %-9u, %-9u, "
    		//"%5d, %4d, %-20s, %-23s, %4d.%03ld, %-10s, %-9d\n",	

        if (CSapStack::nLogType == 0)
        {
    	    snprintf(szBuf,sizeof(szBuf)-1,	"%s,  %s,  %d,  %d,  %s,  %u,  0,  "\
    		    "%d,  %d,  %s,  %s,  0.0,  0-0,  %s,  %d\n",		
    		    strNowTm.c_str(), strAddr.c_str(),sInfo.nAppId,sInfo.nAreaId,sInfo.strUserName.c_str(),
    		    dwSeq, nServId, nMsgId, CAsyncLogThread::GetInstance()->GetLocalIP().c_str(),
    		    strNowTm.c_str(), sLog.logInfo.c_str(),nCode);    
        }
        else
        {
            snprintf(szBuf, sizeof(szBuf)-1, "%s,  %s,  %d,  %d,  %s,  "\
                "%s,  %s,  %d,  %d,  %s,  "\
                "%s,  %d.%03d,  %s,  %s,  %d,  %s,  %s\n",
                strNowTm.c_str(),
                strAddr.c_str(),
                sInfo.nAppId,
                sInfo.nAreaId,
                sInfo.strUserName.c_str(),
                "", /* sInfo.strGuid.c_str() */
                sInfo.strLogId.c_str(),
                nServId,
                nMsgId,
                CAsyncLogThread::GetInstance()->GetLocalIP().c_str(),
                strNowTm.c_str(),
                0, /* dwInterMS */
                0, /* interval.tv_usec % 1000 */
                "0-0", /*strErrInfo.c_str()*/
                sLog.logInfo.c_str(),
                nCode,
                sLog.logInfo2.c_str(),
                sInfo.strComment.c_str());
        }

        szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
        CAsyncLogThread::GetInstance()->OnAsyncLog(nLevel,XLOG_INFO,0,0,szBuf,
            nCode,strAddr,nServId,nMsgId,isCompose);
    }
}


void CSessionManager::RecordRequest(const string& strAddr,int nServiceId, int nMsgId, int nCode)
{
    if(IsEnableLevel(SOC_PLATFORM_MODULE, XLOG_INFO))
	{
        struct tm tmNow;
    	struct timeval_a now;
    	gettimeofday(&now);     		
        localtime_r(&(now.tv_sec), &tmNow);
        const string strNowTm = TimeToString(tmNow,now.tv_usec/1000);
        
        char szBuf[MAX_LOG_BUFFER]={0};
        //record time, thread_id, source ip,  serviceid, msgid, code
        // snprintf(szBuf,sizeof(szBuf)-1,"%-24s, %-2d,%-20s, %4d, %4d, %9d\n",
        snprintf(szBuf,sizeof(szBuf)-1,"%s,  %d,  %s,  %d,  %d,  %d\n",	        
        strNowTm.c_str(), GetId(),strAddr.c_str(),nServiceId,nMsgId,nCode);
        szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
        CAsyncLogThread::GetInstance()->OnAsyncLog(SOC_PLATFORM_MODULE,XLOG_INFO,
            0,0,szBuf,nCode,strAddr,nServiceId);
    }
}

void CSessionManager::RecordDataReq(const string& strIp, unsigned dwServiceId, unsigned dwMsgid, int nCode)
{
    SS_XLOG(XLOG_TRACE,"CSessionManager::%s\n",__FUNCTION__);  
    if (!IsEnableLevel(SOS_DATA_MODULE, XLOG_INFO))
    {
        return;
    }

    struct tm tmNow;
    struct timeval_a now;
    gettimeofday(&now);
    localtime_r(&(now.tv_sec), &tmNow);
    const string strNowTm   = TimeToString(tmNow,now.tv_usec/1000);

    char szBuf[MAX_LOG_BUFFER] = {0};
    snprintf(szBuf, sizeof(szBuf)-1, "%s,  %s,  "\
        "%s,  %d,  %d,  "\
        "%s,  ,  ,  ,  ,  ,  ,  %d\n",		
        strNowTm.c_str(),
        strIp.c_str(),
        "",
        dwServiceId,
        dwMsgid,
        CAsyncLogThread::GetInstance()->GetLocalIP().c_str(),
        nCode);
    szBuf[MAX_LOG_BUFFER - 1] = szBuf[MAX_LOG_BUFFER - 2] == '\n' ? '\0' : '\n';
    CAsyncLogThread::GetInstance()->OnAsyncLog(SOS_DATA_MODULE,XLOG_INFO,0,0,szBuf,0,
        strIp, dwServiceId, dwMsgid, true);
}

void CSessionManager::Dump()
{
    SS_XLOG(XLOG_NOTICE,"=============CSessionManager::Dump,index[u]==============\n");
    SS_XLOG(XLOG_NOTICE,"=====m_vecSos[%d]\n",m_vecSos.size());
	SS_XLOG(XLOG_NOTICE,"=====m_tmSendAckRequest[%d]\n",m_ackRequestQueue.size());
    vector<CSosSessionContainer *>::const_iterator itrSos;
    for(itrSos=m_vecSos.begin();itrSos!=m_vecSos.end();++itrSos)
    {
        (*itrSos)->Dump();
    }

    SS_XLOG(XLOG_NOTICE,"=====m_mapSoc--1 [%d]\n",m_mapSoc.size());
    map<unsigned int,CSocSession *>::const_iterator itrSoc;
    for(itrSoc=m_mapSoc.begin();itrSoc!=m_mapSoc.end();++itrSoc)
    {
        unsigned int dwServiceId=itrSoc->first;
        CSocSession *pSoc=itrSoc->second;
        SS_XLOG(XLOG_NOTICE,"%u-------\n",dwServiceId);
        pSoc->Dump();
    }

    SS_XLOG(XLOG_NOTICE,"=====m_mapSoc--2 [%d]\n",m_mapRegistedSoc.size());
    boost::unordered_map<string,CSocSession *>::const_iterator itrSoc2;
    for(itrSoc2=m_mapRegistedSoc.begin();itrSoc2!=m_mapRegistedSoc.end();++itrSoc2)
    {
        string strname=itrSoc2->first;
        CSocSession *pSoc=itrSoc2->second;
        SS_XLOG(XLOG_NOTICE,"%s-------\n",strname.c_str());
        pSoc->Dump();
    }

    vector<string>::iterator itrSuperSoc;
    for(itrSuperSoc=m_vecSuperSoc.begin();itrSuperSoc!=m_vecSuperSoc.end();++itrSuperSoc)
    {
        string strSuper=*itrSuperSoc;
        SS_XLOG(XLOG_NOTICE,"---super[%s]\n",strSuper.c_str());
    }

    string strPrivilege;
    vector<SServicePrivilege>::iterator itr;
    for(itr=m_vecSuperPrivilege.begin();itr!=m_vecSuperPrivilege.end();++itr)
    {
        const SServicePrivilege & obj=*itr;
        char szServiceId[16]={0};
        sprintf(szServiceId,"%d",obj.dwServiceId);
        strPrivilege+=szServiceId;
        if(obj.vecMsgId.size()>0)
        {
            strPrivilege+="{";
            vector<unsigned int>::const_iterator itrMsgId;
            for(itrMsgId=obj.vecMsgId.begin();itrMsgId!=obj.vecMsgId.end();++itrMsgId)
            {
                unsigned int dwMsgId=*itrMsgId;
                char szMsgId[16]={0};
                sprintf(szMsgId,"%d/",dwMsgId);
                strPrivilege+=szMsgId;
            }
            strPrivilege+="}";
        }
	strPrivilege+=",";
    }
    SS_XLOG(XLOG_NOTICE,"        privilege:%s\n",strPrivilege.c_str()); 
    m_oComposeConfig.Dump();
    m_oServiceConfig.Dump();
    m_virtualServiceManager.Dump();
}

void CSessionManager::gettimeofday(struct timeval_a* tm, bool bUseCache)
{
    if (!bUseCache)
    {
        gettimeofday_a(&m_timeReceive, NULL);
    }

    if (tm)
    {
        *tm = m_timeReceive;
    }
}

void CSessionManager::DoDecreaseComposeObjNum()
{
    SS_XLOG(XLOG_TRACE,"CSessionManager::%s alive num[%u]\n",__FUNCTION__, m_dwComposeServiceObjAlive);
    if (m_dwComposeServiceObjAlive > 0)
    {
        m_dwComposeServiceObjAlive--;
    }
}

void CSessionManager::DoAddComposeObjNum(unsigned dwSize)
{
    SS_XLOG(XLOG_TRACE,"CSessionManager::%s alive num[%u] add num[%u]\n",
        __FUNCTION__, m_dwComposeServiceObjAlive, dwSize);
    m_dwComposeServiceObjAlive =+ dwSize;
}

bool CSessionManager::IsAllComposeServiceObjClosed()
{
    SS_XLOG(XLOG_TRACE,"CSessionManager::%s stopped[%d] alive num[%u]\n",__FUNCTION__, bStopped ? 1 : 0, m_dwComposeServiceObjAlive);
    return bStopped && m_dwComposeServiceObjAlive == 0;
}

void CSessionManager::OnAddComposeObjNum(unsigned dwSize)
{
    m_oIoService.post(boost::bind(&CSessionManager::DoAddComposeObjNum, this, dwSize));
}

void CSessionManager::OnDecreaseComposeObjNum()
{
    m_oIoService.post(boost::bind(&CSessionManager::DoDecreaseComposeObjNum, this));
}

