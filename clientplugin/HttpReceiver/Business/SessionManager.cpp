#include "SessionManager.h"
#include "BusinessLogHelper.h"
#include "IAsyncLogModule.h"
#include "SapMessage.h"
#include "SapTLVBody.h"
#include "HpsResponseMsg.h"
#include "XmlConfigParser.h"
#include <algorithm>
#include "HpsStack.h"
#include <boost/algorithm/string.hpp>
#include "UrlCode.h"
#include "HpsCommonFunc.h"
#include "SapMessage.h"
#include "HpsRequestMsg.h"
#include "HttpRecVirtualClient.h"
#include "Cipher.h"
#include "AsyncVirtualClientLog.h"
#include "ResourceAgent.h"

#ifndef WIN32
#include <arpa/inet.h>
#endif

using namespace sdo::hps;

const unsigned int LSC_SERVICE_ID = 55101;
const unsigned int RSC_SERVICE_ID = 55201;

CHpsSessionManager::CHpsSessionManager(CHttpRecVirtualClient *pVirtualClient, int nThreadId)
    : m_nThreadId(nThreadId),
      m_pHpsContainer(NULL),
      m_pTransfer(NULL),
      m_dwSeq(0),
      m_isAlive(false),
      m_tmTimeOut(m_oIoService),
      m_nLen(0),
      m_dwSosContainerCount(0),
      m_tmSelfCheck(this, SELF_CHECK_INTERVAL, boost::bind(&CHpsSessionManager::DoSelfCheck, this), CThreadTimer::TIMER_CIRCLE),
      m_timerSocSelfCheck(this, SOC_SELFCHECK_INTERVAL ,boost::bind(&CHpsSessionManager::DoReportSosStatusCheckData, this), CThreadTimer::TIMER_CIRCLE),
      //m_timerRequest(this, REQUEST_TIME_OUT, boost::bind(&CHpsSessionManager::DetectRequestTimerList, this), CThreadTimer::TIMER_CIRCLE),
      m_pHttpClientContainer(NULL),
      m_pVirtualClient(pVirtualClient)
{
	m_pHpsContainer = new CHpsSessionContainer(this);
	m_pTransfer = new CAvenueHttpTransfer(this);
	m_pHttpClientContainer = new CHttpClientContainer(this);
	m_osapManager.SetSessionManager(this);
    m_pResourceAgent = new CResourceAgent(this);
	memset(m_szTransferData.arrTransferData, 0, MAXBUFLEN);
}

CHpsSessionManager::~CHpsSessionManager()
{
	if(m_pHpsContainer != NULL)
	{
		delete m_pHpsContainer;
	}
	if(m_pTransfer != NULL)
	{
		delete m_pTransfer;
	}
	if(m_pHttpClientContainer != NULL)
	{
		delete m_pHttpClientContainer;
	}

    m_mapSosSessionContainerByServiceId.clear();
    //OnClearCache();
}

void CHpsSessionManager::Start(unsigned int affinity)
{    
    BS_XLOG(XLOG_DEBUG, "CHpsSessionManager::%s, affinity[%d]\n", __FUNCTION__, affinity);
    m_oWork = new boost::asio::io_service::work(m_oIoService);
    m_thread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_oIoService)); 
    m_oIoService.post(boost::bind(&CHpsSessionManager::StartInThread, this));
	m_nTimeCount = 0;

    m_isAlive=true;
	m_tmTimeOut.expires_from_now(boost::posix_time::microseconds(300));
	m_tmTimeOut.async_wait(
        MakeSapAllocHandler(m_allocTimeOut,boost::bind(&CHpsSessionManager::DoTimer, this)));
}

void CHpsSessionManager::StartInThread()
{
    BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s...\n",__FUNCTION__);
    Reset();
    /* start the timers */
	m_tmSelfCheck.Start();
	m_timerSocSelfCheck.Start();
	//m_timerRequest.Start();
	m_pHpsContainer->StartInThread();
	m_pTransfer->StartInThread();
	m_pHttpClientContainer->StartInThread();
}

void CHpsSessionManager::DoTimer()
{
    //BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s\n",__FUNCTION__);
	if (m_nTimeCount++ == 6)
	{
		DetectTimerList();
		m_nTimeCount = 0;
	}
	DetectRequestTimerList();
    m_isAlive = true;
    if (m_isAlive)
    {
        m_tmTimeOut.expires_from_now(boost::posix_time::microseconds(300));
        m_tmTimeOut.async_wait(
                MakeSapAllocHandler(m_allocTimeOut,boost::bind(&CHpsSessionManager::DoTimer, this)));
    }
}

void CHpsSessionManager::StopInThread()
{
    //BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s\n",__FUNCTION__);
    m_tmSelfCheck.Stop();
    m_timerSocSelfCheck.Stop();
    m_pHpsContainer->StopInThread();
    m_pTransfer->StopInThread();
    m_pHttpClientContainer->StopInThread();
    m_oIoService.stop();
}


void CHpsSessionManager::Stop()
{
    BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n",__FUNCTION__);
    m_isAlive=false;
    delete m_oWork;
    m_oIoService.post(boost::bind(&CHpsSessionManager::StopInThread, this));
    m_thread.join();
    m_oIoService.reset();
}
void CHpsSessionManager::AddConnection(HpsConnection_ptr ptrConn)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::AddConnectionInner,this,ptrConn));
}

void CHpsSessionManager::AddConnectionInner(HpsConnection_ptr ptrConn)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_pHpsContainer->AddConnection(ptrConn);
}

void CHpsSessionManager::OnLoadTransferObjs(const string &strH2ADir, const string &strH2AFilter, const string &strA2HDir, const string &strA2HFilter)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, strH2ADir[%s], strH2AFilter[%s], strA2HDir[%s], strA2HFilter[%s]\n", 
		__FUNCTION__,strH2ADir.c_str(), strH2AFilter.c_str(),strA2HDir.c_str(), strA2HFilter.c_str());
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoLoadTransferObjs,this,strH2ADir,strH2AFilter,strA2HDir,strA2HFilter));
}

void CHpsSessionManager::OnLoadSosList(const vector<string> & vecServer)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoLoadSosList,this,vecServer));
}

void CHpsSessionManager::OnUnLoadSharedLibs(const string &strDirPath,const vector<string> &vecSo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoUnLoadSharedLibs,this,strDirPath,vecSo));
}

void CHpsSessionManager::OnReLoadSharedLibs(const string & strDirPath,const vector<string> &vecSo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoReLoadSharedLibs,this,strDirPath,vecSo));
}

void CHpsSessionManager::OnLoadRouteConfig(const vector<string> &vecRouteBalance)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoLoadRouteConfig,this,vecRouteBalance));
}

void CHpsSessionManager::DoLoadRouteConfig(const vector<string> &vecRouteBalance)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n", __FUNCTION__);
	map<unsigned int, map<int, string> > mapSosPriAddrsByServiceId;
	map<unsigned int, CSosSessionContainer*>::iterator itr;
	for(itr=m_mapSosSessionContainerByServiceId.begin(); itr!=m_mapSosSessionContainerByServiceId.end(); ++itr)
	{
		unsigned int dwServiceId = itr->first;
		CSosSessionContainer *pContainer = itr->second;
		map<int, string> mapTemp;
		pContainer->GetMapPriSosList(mapTemp);
		mapSosPriAddrsByServiceId.insert(make_pair(dwServiceId, mapTemp));
	}

	m_oHttpReqConfigManager.LoadConfig(vecRouteBalance,mapSosPriAddrsByServiceId);
}

void CHpsSessionManager::OnUpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, serviceId[%u], msgId[%u], nErrCode[%d]\n", __FUNCTION__,dwServiceId,dwMsgId,nErrCode);

	m_oIoService.post(boost::bind(&CHpsSessionManager::DoUpdateRouteConfig,this,dwServiceId,dwMsgId,nErrCode));
}

void CHpsSessionManager::DoUpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, serviceId[%u], msgId[%u], nErrCode[%d]\n", __FUNCTION__,dwServiceId,dwMsgId,nErrCode);

	m_oHttpReqConfigManager.UpdateRouteConfig(dwServiceId,dwMsgId,nErrCode);
}

void CHpsSessionManager::OnReLoadCommonTransfer(const vector<string>& vecServer, const string& strTransferName)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s vecsize[%d]\n", __FUNCTION__,vecServer.size());
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoReLoadCommonTransfer,this,vecServer,strTransferName));
}

void CHpsSessionManager::OnResourceRequestCompleted(SRequestInfo &sReq)
{
    BS_XLOG(XLOG_DEBUG, "CHpsSessionManager::%s\n", __FUNCTION__);
    SendHttpResponse(sReq);
    RecordeHttpRequest(sReq);
}

SUrlInfo CHpsSessionManager::GetSoNameByUrl(const string& strUrl)
{
	if (m_pTransfer != NULL)
	{
		return m_pTransfer->GetSoNameByUrl(strUrl);
	}
	return SUrlInfo();
}

int CHpsSessionManager::FindUrlMapping(const string& strUrl, unsigned int& dwServiceID, unsigned int& dwMsgID)
{
    if (NULL != m_pHpsContainer)
    {
        return m_pHpsContainer->FindUrlMapping(strUrl, dwServiceID, dwMsgID);
    }
    
    return -1;
}

/*****************************************************/

void CHpsSessionManager::DoLoadTransferObjs(const string &strH2ADir, const string &strH2AFilter, const string &strA2HDir, const string &strA2HFilter)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, strH2ADir[%s], strH2AFilter[%s], strA2HDir[%s], strA2HFilter[%s]\n", 
		__FUNCTION__,strH2ADir.c_str(), strH2AFilter.c_str(),strA2HDir.c_str(), strA2HFilter.c_str());
	if(0 == m_pTransfer->DoLoadTransferObj(strH2ADir, strH2AFilter, strA2HDir, strA2HFilter))
	{
		vector<string> vecUrlMapping;
		m_pTransfer->GetUriMapping(vecUrlMapping);
		m_pHpsContainer ->LoadMethodMapping(vecUrlMapping);
		m_osapManager.LoadConfig();

		m_vecUrlMappingForCheck = vecUrlMapping;
	}
}

void CHpsSessionManager::DoUnLoadSharedLibs(const string &strDirPath,const vector<string> &vecSo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n",	__FUNCTION__);
	m_pTransfer->UnLoadTransferObj(strDirPath,vecSo);

	vector<string> vecUrlMapping;
	m_pTransfer->GetUriMapping(vecUrlMapping);
	m_pHpsContainer ->LoadMethodMapping(vecUrlMapping);
}

void CHpsSessionManager::DoReLoadSharedLibs(const string & strDirPath,const vector<string> &vecSo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n",	__FUNCTION__);
	if(0 == m_pTransfer->ReLoadTransferObj(strDirPath,vecSo))
	{
		vector<string> vecUrlMapping;
		m_pTransfer->GetUriMapping(vecUrlMapping);
		m_pHpsContainer ->LoadMethodMapping(vecUrlMapping);
		m_osapManager.LoadConfig();
	}
}

void CHpsSessionManager::DoLoadSosList(const vector<string> & vecServer)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s.,vecServer.size[%d]\n", __FUNCTION__,vecServer.size());
	
	vector<SServiceSosAddr> vecServiceIdSosAdds;

	vector<unsigned int> vecNewServiceId;
	vector<string>::const_iterator iter;
	for(iter = vecServer.begin(); iter != vecServer.end(); ++iter)
	{
		SServiceSosAddr sSrvSosAddr;
		
		string strSosInfo = *iter;
		CXmlConfigParser oConfig;
		oConfig.ParseDetailBuffer(strSosInfo.c_str());
		string strServiceIds = oConfig.GetParameter("ServiceId","0");
		sSrvSosAddr.vecSosAddr = oConfig.GetParameters("ServerAddr");
		vector<unsigned int> vecIntServiceId;
		vector<string> vecStrServiceId;
		boost::algorithm::split( vecStrServiceId, strServiceIds, boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on); 
		vector<string>::iterator iterVec;
		for(iterVec=vecStrServiceId.begin(); iterVec!=vecStrServiceId.end(); ++iterVec)
		{
			unsigned int dwServiceId = atoi((*iterVec).c_str());
			sSrvSosAddr.vecServiceId.push_back(dwServiceId);
			vecNewServiceId.push_back(dwServiceId);
		}
		vecServiceIdSosAdds.push_back(sSrvSosAddr);
	}
	
	sort(vecNewServiceId.begin(), vecNewServiceId.end());

	/*step1, travers the old m_mapSosSessionContainerByServiceId, delete the deleted service*/
	map<unsigned int, CSosSessionContainer*>::iterator iterOldMap = m_mapSosSessionContainerByServiceId.begin();;
	while(iterOldMap != m_mapSosSessionContainerByServiceId.end())
	{
		map<unsigned int, CSosSessionContainer*>::iterator itrTmp = iterOldMap++;
		unsigned int dwOldServiceId = itrTmp->first;
		if(!binary_search(vecNewServiceId.begin(), vecNewServiceId.end(),dwOldServiceId ))
		{
			BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s, erase map-serviceId[%u]\n", __FUNCTION__,dwOldServiceId);
			m_mapSosSessionContainerByServiceId.erase(itrTmp);
		}
	}
	
	/* set the sosSessionContainer's status to false*/
	map<unsigned int, CSosSessionContainer*>::iterator itrContainer = m_mapSosSessionContainerByContainerId.begin();
	for(;itrContainer != m_mapSosSessionContainerByContainerId.end(); ++itrContainer)
	{
		CSosSessionContainer* pContainer = itrContainer->second;
		pContainer->SetUsableStatus(false);
	}
	
	/*step2, tranvers the new mapNewSosList, add the load the services' sossession*/
	vector<SServiceSosAddr>::iterator itrVec = vecServiceIdSosAdds.begin();
	for(; itrVec != vecServiceIdSosAdds.end(); ++itrVec)
	{
		vector<unsigned int> &vecServiceId = itrVec->vecServiceId;
		vector<string> &vecSosAddr = itrVec->vecSosAddr;

		CSosSessionContainer* pContainer = GetContainerPtrBySosAddr(vecSosAddr);
		if(pContainer == NULL)
		{
			BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s, no pContainer\n", __FUNCTION__);
			pContainer = new CSosSessionContainer(++m_dwSosContainerCount,this,vecServiceId);
			pContainer->LoadSosList(vecSosAddr, this);
			m_mapSosSessionContainerByContainerId.insert(make_pair(pContainer->GetContainerId(),pContainer));
		}
		else
		{
			BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s, pContainer has exist\n", __FUNCTION__);
			pContainer->LoadSosList(vecSosAddr, this);
		}
		
		pContainer->SetServiceIds(vecServiceId);
		pContainer->SetUsableStatus(true);

		vector<unsigned int>::iterator iterSrvIdVec;
		for(iterSrvIdVec = vecServiceId.begin(); iterSrvIdVec != vecServiceId.end(); ++iterSrvIdVec)
		{
			
			unsigned int dwServiceId = *iterSrvIdVec;
			//m_mapSosSessionContainerByServiceId.insert(make_pair(dwServiceId, pContainer));
			m_mapSosSessionContainerByServiceId[dwServiceId] = pContainer;

			BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s, dwServiceId[%u], pContainerId[%u]\n", __FUNCTION__,dwServiceId,pContainer->GetContainerId());
		}
	}
	
	/* delete the sosSessionContainer*/
	itrContainer = m_mapSosSessionContainerByContainerId.begin();
	while(itrContainer != m_mapSosSessionContainerByContainerId.end())
	{
		map<unsigned int, CSosSessionContainer*>::iterator itrContainerIdTmp = itrContainer++;
		CSosSessionContainer* pContainerTmp = itrContainerIdTmp->second;
		if(pContainerTmp->GetUsableStatus() == false)
		{
			m_mapSosSessionContainerByContainerId.erase(itrContainerIdTmp);
			delete pContainerTmp;
		}
	}
}

CSosSessionContainer* CHpsSessionManager::GetContainerPtrBySosAddr(const vector<string> &vecSosAddr)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s.  vecSosAddr.size[%d]\n", __FUNCTION__,vecSosAddr.size());
	
	map<unsigned int, CSosSessionContainer*>::iterator itrContainer = m_mapSosSessionContainerByContainerId.begin();
	for(;itrContainer != m_mapSosSessionContainerByContainerId.end(); ++itrContainer)
	{
		CSosSessionContainer* pContainer = itrContainer->second;
		if(true == pContainer->ContainSosAddr(vecSosAddr))
		{
			return pContainer;
		}
	}
	return NULL;
}



void CHpsSessionManager::DoReLoadCommonTransfer(const vector<string>& vecServer, const string& strTransferName)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n",	__FUNCTION__);
	DoLoadSosList(vecServer);
	if(0 == m_pTransfer->ReLoadCommonTransfer(strTransferName))
	{
		vector<string> vecUrlMapping;
		m_pTransfer->GetUriMapping(vecUrlMapping);
		m_pHpsContainer->LoadMethodMapping(vecUrlMapping);

		m_vecUrlMappingForCheck = vecUrlMapping;
	}
}

void CHpsSessionManager::OnHttpConnected(unsigned int dwId, const string &strRemoteIp,unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, dwId[%u], strRemoteIp[%s], dwRemotePort[%d]\n",
		__FUNCTION__,dwId,strRemoteIp.c_str(),dwRemotePort);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoHttpConnected,this,dwId, strRemoteIp, dwRemotePort));
}


void CHpsSessionManager::DoHttpConnected(unsigned int dwId, const string &strRemoteIp,unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, dwId[%u], strRemoteIp[%s], dwRemotePort[%d]\n",
		__FUNCTION__,dwId, strRemoteIp.c_str(),dwRemotePort);

	SHttpConnectInfo connInfo(E_Http_Connect,dwId,strRemoteIp,dwRemotePort);
	GetAsyncLogModule()->OnHttpConnectInfo(connInfo);

	m_pHpsContainer->OnHttpConnected(dwId);
}



void CHpsSessionManager::DoReceiveHttpRequest(SRequestInfo &sReq, const string &strRemoteIp,unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, dwId[%u], nServiceId[%d], nMsgId[%d], strUriCommand[%s], strUriAttribute[%s], sReq.strRemoteIp[%s], sReq.dwRemotePort[%u], strRemoteIp[%s], dwRemotePort[%d]\n",
		__FUNCTION__, sReq.dwId,sReq.nServiceId, sReq.nMsgId, sReq.strUriCommand.c_str(),  sReq.strUriAttribute.c_str(), sReq.strRemoteIp.c_str(),sReq.dwRemotePort,strRemoteIp.c_str(),dwRemotePort);

	GetAsyncLogModule()->OnReceiveHttpRequest(sReq.nServiceId,sReq.nMsgId);
	
	sReq.dwSequence = ++m_dwSeq;
	

	if(0 == m_ipLimitCheckor.CheckIpLimit(sReq.strUrlIdentify,sReq.strRemoteIp))
	{
		sReq.inWhiteList = true;
	}
	
	OsapUserInfo* pUserInfo = NULL;
	int nCode = m_osapManager.GetUserInfo(sReq, &pUserInfo);
	if (nCode > 0)
	{
		return; // waiting for osap response
	}
        
        if (!sReq.isNomalBpeRequest)
        {
            DoHandleResourceRequest(sReq);
        }
        else
        {
            DoSendAvenueRequest(sReq, nCode, pUserInfo);
        }
}

int CHpsSessionManager::DoHandleResourceRequest(SRequestInfo &sReq)
{
    BS_XLOG(XLOG_DEBUG, "CHpsSessionManager::%s\n", __FUNCTION__);
    bool isFind = false, isLoad = false;
    string strContentType;
    m_pResourceAgent->OnResourceRequest(sReq, isFind, isLoad, strContentType);
    if (!isFind)
    {
        BS_XLOG(XLOG_WARNING, "CHpsSessionManager::%s the resource [%s] is not found\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
        EErrorCode errCode = ERROR_URL_NOT_SUPPORT;
        if (sReq.requestType == E_JsonRpc_Request)
        {
            errCode = ERROR_JSONRPC_METHOD_NOT_FOUND;
        }

        char szErrorCodeNeg[16] = {0};
        snprintf(szErrorCodeNeg, 15, "%d", errCode);
        sReq.strResponse = string("{\"return_code\":") + szErrorCodeNeg 
                              + ",\"return_message\":\"" +  CErrorCode::GetInstance()->GetErrorMessage(errCode) 
                               + "\",\"data\":{}}";
        sReq.isVaild = false;
        sReq.nCode = errCode;
        sReq.nHttpCode = 404;
        SendHttpResponse(sReq);
        RecordeHttpRequest(sReq);
    } 
    else if (isLoad)
    {
        BS_XLOG(XLOG_DEBUG, "CHpsSessionManager::%s the resource [%s] is static and loaded\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
        sReq.nCode = 0;
        sReq.strContentType = strContentType;
        SendHttpResponse(sReq);
        RecordeHttpRequest(sReq);
    }
    else
    {
        BS_XLOG(XLOG_DEBUG, "CHpsSessionManager::%s the resource [%s] is dynamic and loaded\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
        //m_mapSeqReqInfo[sReq.dwSequence] = sReq;
    }
    return 0;
}

int CHpsSessionManager::DoSendAvenueRequest(SRequestInfo &sReq, int nCode, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s username[%s]\n", __FUNCTION__,sReq.strUserName.c_str());

	CSapTLVBodyEncoder exHeaderEncoder;
	if (nCode == 0 || sReq.nServiceId == RSC_SERVICE_ID)
	{
		if (nCode == 0)
		{
			nCode = m_osapManager.CheckAuthenAndSignature(sReq, pUserInfo);
		}
		char szAddr[16] = {0};
#if defined(_WIN32)||defined(_WIN64)
        char szPeerAddr[4] = { 0 };
        unsigned int segment[4];
        sscanf(sReq.strRemoteIp.c_str(), "%u.%u.%u.%u", &segment[0], &segment[1], &segment[2], &segment[3]);
        szPeerAddr[0] = segment[0];
        szPeerAddr[1] = segment[1];
        szPeerAddr[2] = segment[2];
        szPeerAddr[3] = segment[3];
#else
        in_addr_t peerAddr = inet_addr(sReq.strRemoteIp.c_str());
        memcpy(szAddr, &peerAddr, 4);
#endif
        int netPort = htonl(sReq.dwRemotePort);
        memcpy(szAddr + 4, &netPort, 4);
		exHeaderEncoder.SetValue(SAP_HEADEX_CODE_USER, sReq.strUserName);
		exHeaderEncoder.SetValue(SAP_HEADEX_CODE_ADDR, szAddr, 8);
		if (!m_osapManager.IsUrlInNoSaagList(sReq.strUrlIdentify))
		{
			exHeaderEncoder.SetValue(SAP_HEADEX_CODE_APPID, sReq.nAppid);
			exHeaderEncoder.SetValue(SAP_HEADEX_CODE_AREAID, sReq.nAreaId);
			exHeaderEncoder.SetValue(SAP_HEADEX_CODE_GROUPID, sReq.nGroupId);
			exHeaderEncoder.SetValue(SAP_HEADEX_CODE_HOSTID, sReq.nHostid);
			exHeaderEncoder.SetValue(SAP_HEADEX_CODE_SPID, sReq.nSpid);
		}
		if (sReq.nServiceId == RSC_SERVICE_ID)
		{
			sReq.strGuid = "RSC" + sReq.strGuid;
		}
		exHeaderEncoder.SetValue(SAP_HEADEX_CODE_GUID, sReq.strGuid);
		exHeaderEncoder.SetValue(SAP_HEADEX_CODE_COMMENT, sReq.strComment);

		memcpy(m_szTransferData.arrTransferData, exHeaderEncoder.GetBuffer(), exHeaderEncoder.GetLen());
		m_nLen = exHeaderEncoder.GetLen();
		
		if (sReq.nServiceId == RSC_SERVICE_ID)
		{
			if (nCode == 0)
			{
				unsigned osapcheck = 1;
				int RSC_SIG_FLAG = 100;
				CSapTLVBodyEncoder::SetValue(RSC_SIG_FLAG, 
					&osapcheck, 4, m_szTransferData.arrTransferData+m_nLen, (unsigned&)m_nLen);
			}
			else
			{
				nCode = 0; // "RegistTransfer" will check authen
			}
		}
	}

	if (nCode == 0)
	{
		if (!sReq.strCookie.empty())
		{
			CSapTLVBodyEncoder::SetValue(TRANSEX_CODE_COOKIE,
				sReq.strCookie.c_str(), sReq.strCookie.length(),
				m_szTransferData.arrTransferData+m_nLen, (unsigned&)m_nLen);
		}
                
                m_szTransferData.pReq = &sReq;
		nCode = m_pTransfer->DoTransferRequestMsg(sReq.strUrlIdentify, sReq.nServiceId, sReq.nMsgId,
			sReq.dwSequence, sReq.strUriCommand.c_str(), sReq.strUriAttribute.c_str(),
			sReq.strRemoteIp.c_str(),sReq.dwRemotePort, (void*)&m_szTransferData, &m_nLen);
		sReq.headers.clear();
	}

	if (nCode == 0)
	{
        void *pBuffer = m_szTransferData.arrTransferData;
		SSapMsgHeader* pHeader = (SSapMsgHeader*)pBuffer;
		sReq.nServiceId = htonl(pHeader->dwServiceId);
		sReq.nMsgId = htonl(pHeader->dwMsgId);

        const void* pExHeader = exHeaderEncoder.GetBuffer();
        int nExLen = exHeaderEncoder.GetLen();
        void *wrBuffer = NULL;
        int nFactLen = m_nLen;
        if (nExLen>0)
        {
            nFactLen += nExLen;
            wrBuffer = malloc(nFactLen);
            memcpy(wrBuffer, pBuffer, pHeader->byHeadLen);
            memcpy((char *)wrBuffer + pHeader->byHeadLen, pExHeader, nExLen);
            memcpy((char *)wrBuffer + pHeader->byHeadLen + nExLen, (char *)pBuffer + pHeader->byHeadLen, m_nLen - pHeader->byHeadLen);

            pHeader = (SSapMsgHeader *)wrBuffer;
            pHeader->byHeadLen += nExLen;
            pHeader->dwPacketLen = htonl(ntohl(pHeader->dwPacketLen) + nExLen);
        }
        else
        {
            wrBuffer = malloc(nFactLen);
            memcpy(wrBuffer, pBuffer, m_nLen);
        }
                
        string strSosSignatueKey = m_oHttpReqConfigManager.GetSignatureKey(sReq.nServiceId, sReq.nMsgId, sReq.strUrlIdentify);
        if(!strSosSignatueKey.empty())
        {
            int nDigestLen = strSosSignatueKey.length();
            int nMin = (nDigestLen<16? nDigestLen:16);
            memcpy(pHeader->bySignature, strSosSignatueKey.c_str(), nMin);
            CCipher::Md5((unsigned char *)wrBuffer, nFactLen, pHeader->bySignature);
        }

        BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s SendRequest\n", __FUNCTION__);
        m_pVirtualClient->SendRequest(this, wrBuffer, nFactLen);
        BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s AddHistoryRequest\n", __FUNCTION__);
        AddHistoryRequest(sReq);
        BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s insert into map\n", __FUNCTION__);
        m_mapSeqReqInfo[sReq.dwSequence] = sReq;	
                
        free(wrBuffer);
	}

	if(nCode != 0)  //error happened, response and record the request info
	{		
		BS_XLOG(XLOG_TRACE,"CHpsSessionManager::%s::%d, nCode[%d]\n",__FUNCTION__,__LINE__,nCode);
		sReq.nCode = nCode;
		sReq.strSosIp = "";
		sReq.dwSosPort = 0;
		sReq.headers.clear();
		if(sReq.nCode == ERROR_BAD_REQUEST_INNER)
		{
			sReq.strResponse = string(m_szTransferData.arrTransferData,m_nLen);
		}
		SendHttpResponse(sReq);
		//m_pHpsContainer->DoSendResponse(sReq);
		RecordeHttpRequest(sReq);
	}
	return nCode;
}

void CHpsSessionManager::DoHttpPeerClose(unsigned int dwId, const string &strRemoteIp,const unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, dwId[%u], strRemoteIp[%s], dwRemotePort[%d]\n",
		__FUNCTION__,dwId,strRemoteIp.c_str(),dwRemotePort);

	/*erase the m_mapSeqReqInfo and m_mapQueue according to the pIdentifier*/
	//pair<multimap<unsigned int, unsigned int>::iterator, multimap<unsigned int, unsigned int>::iterator> ii;
	//ii = m_multimapIdSeq.equal_range(dwId);
	//multimap<unsigned int, unsigned int>::iterator itermmap;
	//for( itermmap = ii.first; itermmap != ii.second; ++itermmap ) 
	//{
	//	map<unsigned int, SRequestInfo>::iterator iter = m_mapSeqReqInfo.find(itermmap->second);
	//	if(iter != m_mapSeqReqInfo.end())
	//	{
	//		SRequestInfo &sReq = iter->second;
	//		sReq.nCode = ERROR_CLIENT_CANCEL;
	//		RecordeHttpRequest(sReq);
	//		m_mapSeqReqInfo.erase(iter);
	//		RemoveHistory(sReq);
	//	}
	//}
	//m_multimapIdSeq.erase(dwId);


	SHttpConnectInfo connInfo(E_Http_Close,dwId,strRemoteIp,dwRemotePort);
	GetAsyncLogModule()->OnHttpConnectInfo(connInfo);
}

void CHpsSessionManager::OnConnected(unsigned int dwSosContainerId, unsigned int dwSosSessionId, unsigned int dwConnId)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, containerId[%u], sosSessionId[%u], sosConnId[%u]\n",__FUNCTION__,
		dwSosContainerId, dwSosSessionId, dwConnId);
}

/* 以响应包中的serviceId为准*/
void CHpsSessionManager::OnReceiveSosAvenueResponse(unsigned int dwId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, dwId[%u], strRemoteAddr[%s:%u], bufferLen[%d]\n",
		__FUNCTION__, dwId, strRemoteIp.c_str(), dwRemotePort, nLen);
	
	CSapDecoder sapDecoderObj(pBuffer, nLen);
	unsigned int dwServiceId = sapDecoderObj.GetServiceId();
	unsigned int dwMsgId = sapDecoderObj.GetMsgId();
	unsigned int nSeq = sapDecoderObj.GetSequence();
	unsigned int dwCode = sapDecoderObj.GetCode();

	if (sapDecoderObj.GetServiceId() == SAP_SMS_SERVICE_ID)
	{
            BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, SAP_SMS_SERVICE_ID\n", __FUNCTION__);
		m_osapManager.OnOsapResponse(pBuffer, nLen);
		return;
	}

	if(dwCode >= 100 && dwCode < 200)
	{
		BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, template data packet\n",__FUNCTION__);
		return;
	}	

	if(dwServiceId==SAP_PLATFORM_SERVICE_ID)
	{
		BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s.  Register Sls succ. remoteIp[%s], port[%u]\n", __FUNCTION__,strRemoteIp.c_str(), dwRemotePort);
		return;
	}

        BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, Code[%d]\n", __FUNCTION__, dwCode);
	if(dwCode != 0)
	{
		CHpsStack::Instance()->OnUpdateRouteConfig(dwServiceId,dwMsgId,(int)dwCode);		
	}

	/* judge here can avoid call the transfer method when the request has been cancled before reveiving response
	The method call metho which in shared libs is inefficient */
	map<unsigned int, SRequestInfo>::iterator itr = m_mapSeqReqInfo.find(nSeq);
	if(itr != m_mapSeqReqInfo.end())
	{
        BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, find record\n", __FUNCTION__);
		SRequestInfo sReq = itr->second;
        m_szTransferData.pReq = &sReq;
		sReq.nCode = m_pTransfer->DoTransferResponseMsg(sReq.strUrlIdentify, dwServiceId, dwMsgId, pBuffer,nLen, (void*)m_szTransferData.arrTransferData, &m_nLen);
		if(0 == sReq.nCode)//transfer the packet sucessfully
		{
            BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, transfer the packet sucessfully\n", __FUNCTION__);
			if (VER2SO_PRECODE == *(unsigned*)m_szTransferData.arrTransferData)
			{
                            BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, VER2SO_PRECODE\n", __FUNCTION__);
				unsigned tlvLen = *(unsigned*)(m_szTransferData.arrTransferData+4);
				sReq.strResponse.assign((char*)m_szTransferData.arrTransferData+8+tlvLen, m_nLen-8-tlvLen);
				CSapTLVBodyDecoder tlvDecoder(m_szTransferData.arrTransferData+8, tlvLen);
				tlvDecoder.GetValue(TRANSEX_CODE_HTTPCODE, (unsigned*)&sReq.nHttpCode);
				tlvDecoder.GetValue(TRANSEX_CODE_LOCATION, sReq.strLocationAddr);
				tlvDecoder.GetValues(TRANSEX_CODE_COOKIE, sReq.vecSetCookie);
				tlvDecoder.GetValue(TRANSEX_CODE_CONTTYPE, sReq.strContentType);
			}
			else 
			{
				sReq.strResponse.assign((char*)m_szTransferData.arrTransferData, m_nLen);
			}
		}
		else if(0 < sReq.nCode)
		{
                    BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, fail to transfer\n", __FUNCTION__);
			sReq.nHttpCode = sReq.nCode;
			sReq.nCode = 0;

			char *szSplit = strstr(m_szTransferData.arrTransferData, "\r\n\r\n");
			if(szSplit!=NULL && szSplit>m_szTransferData.arrTransferData && szSplit<m_szTransferData.arrTransferData+m_nLen)
			{
				sReq.strHeaders.assign((char*)m_szTransferData.arrTransferData,szSplit+2-m_szTransferData.arrTransferData);
				sReq.strResponse.assign((char*)(szSplit+4),m_szTransferData.arrTransferData+m_nLen-szSplit-4);
			}
			else
			{
				sReq.strResponse.assign((char*)m_szTransferData.arrTransferData, m_nLen);
			}
			
		}

		sReq.strSosIp = strRemoteIp;
		sReq.dwSosPort = dwRemotePort;
		RemoveHistory(sReq);
		
		m_mapSeqReqInfo.erase(itr);	
		/*must erase the itr before DoSendResponse because the OnHttpPeerClose will be called if the 
		http connection is non-persistant and it will erase the map in DoHttpClose function*/
                SendHttpResponse(sReq);
		RecordeHttpRequest(sReq);
		//m_pHpsContainer->DoSendResponse(sReq);	
	}
	else
	{
		/*the client close the connect before receive response
			do nothing*/
		/*nCode = ERROR_CLIENT_CLOSE_BEFORE_RESPONSE;
			remove the request info in DoHttpPeerClose function*/			
	}
}

void CHpsSessionManager::OnPeerClose(unsigned int dwId)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, dwId[%u]\n",__FUNCTION__, dwId);
	map<unsigned int, SRequestInfo>::iterator itr=m_mapSeqReqInfo.begin();
	while(itr!=m_mapSeqReqInfo.end())
	{
		SRequestInfo sReq = itr->second;
		if(sReq.dwSosId==dwId)
		{
			sReq.nCode=ERROR_SOS_DISCONNECTED;
			RemoveHistory(sReq);
			m_mapSeqReqInfo.erase(itr++);	
			SendHttpResponse(sReq);
			RecordeHttpRequest(sReq);
		}
		else
		{
			itr++;
		}
		
	}
}




int CHpsSessionManager::SendHttpResponse(SRequestInfo &sReq)
{	
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s\n",__FUNCTION__);
	if(sReq.nCode != 0 && sReq.nCode != ERROR_BAD_REQUEST_INNER)
	{
		sReq.strResponse = m_oHttpReqConfigManager.GetResponseString(sReq.nServiceId,sReq.nMsgId,sReq.nCode,sReq.strRemoteIp,sReq.strUrlIdentify);
	}

	if (!sReq.strFnCallback.empty())
	{
		sReq.strResponse = sReq.strFnCallback + "(" + sReq.strResponse + ")";
		sReq.strContentType = "application/javascript";
	}

	string strContentType;
	m_oHttpReqConfigManager.GetResponseOption(sReq.nServiceId,sReq.nMsgId,sReq.strUrlIdentify,
		 sReq.strResCharSet, strContentType);
	if (sReq.strContentType.empty())
		sReq.strContentType = strContentType;
	m_pHpsContainer->DoSendResponse(sReq);

	return 0;
}

int CHpsSessionManager::SendOsapRequest(const void* pBuffer, int nLength)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s...\n",__FUNCTION__);
	CSosSessionContainer* pSosContainer;
	map<unsigned int, CSosSessionContainer*>::iterator itermap = m_mapSosSessionContainerByServiceId.find(SAP_SMS_SERVICE_ID);
	if(itermap != m_mapSosSessionContainerByServiceId.end())
	{
		int nFactPri = 0;
		pSosContainer = itermap->second;
		if(0 != pSosContainer->DoSendMessage(pBuffer, nLength, NULL,0,0,&nFactPri))	
		{
			return ERROR_SOS_CAN_NOT_REACH;
		}
	}
	else
	{
		return ERROR_NO_SOS_AVAILABLE;				
	}
	return 0;
}

void CHpsSessionManager::OnUpdataUserInfo(const string& strUserName, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoUpdataUserInfo,this,strUserName,pUserInfo));
}

void CHpsSessionManager::DoUpdataUserInfo(const string& strUserName, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_osapManager.UpdataUserInfo(strUserName, pUserInfo);
}

void CHpsSessionManager::OnUpdataOsapUserTime(const string& strUserName, const timeval_a& sTime)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoUpdataOsapUserTime,this,strUserName,sTime));
}

void CHpsSessionManager::DoUpdataOsapUserTime(const string& strUserName, const timeval_a& sTime)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_osapManager.UpdataUserReqeustTime(strUserName, sTime);
}

void CHpsSessionManager::OnClearOsapCatch(const string& strMerchant)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoClearOsapCatch,this,strMerchant));
}
void CHpsSessionManager::DoClearOsapCatch(const string& strMerchant)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_osapManager.ClearCatch(strMerchant);
}

void CHpsSessionManager::OnUpdateIpLimit(const map<string, string> & mapIpLimits)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_oIoService.post(boost::bind(&CHpsSessionManager::DoUpdateIpLimit,this,mapIpLimits));
}
void CHpsSessionManager::DoUpdateIpLimit(const map<string, string> & mapIpLimits)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s..\n", __FUNCTION__);
	m_ipLimitCheckor.UpdateIps(mapIpLimits);
}



void CHpsSessionManager::OnReceiveAvenueRequest(unsigned dwSosContainerId,
					unsigned int dwSosSessionId, unsigned int dwConnId,
					const void *pBuffer, int nLen,
					const string &strRemoteIp,unsigned int dwRemotePort)
{	
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, containerId[%u], sosSessionId[%u], connId[%u], remoteIp[%s], remotePort[%u], bufferLen[%d]\n",__FUNCTION__,dwSosContainerId,dwSosSessionId, dwConnId,strRemoteIp.c_str(),dwRemotePort,nLen);

	SAvenueRequestInfo sAvenueReqInfo;

	gettimeofday_a(&(sAvenueReqInfo.tStart),0);
	
	sAvenueReqInfo.dwSosSessionContainerId = dwSosContainerId;
	sAvenueReqInfo.dwSosSessionId = dwSosSessionId;
	sAvenueReqInfo.dwSosConnectionId = dwConnId;
	sAvenueReqInfo.strSosIp = strRemoteIp;
	sAvenueReqInfo.dwSosPort = dwRemotePort;

	CSapDecoder oSapDecoder(pBuffer, nLen);
	sAvenueReqInfo.dwServiceId = oSapDecoder.GetServiceId();
	sAvenueReqInfo.dwMsgId = oSapDecoder.GetMsgId();
	sAvenueReqInfo.dwAvenueSequence= oSapDecoder.GetSequence();

	if (oSapDecoder.GetServiceId() != SAP_SLS_SERVICE_ID)
	{
		CSapEncoder oSapEncoder(SAP_PACKET_RESPONSE, oSapDecoder.GetServiceId(), oSapDecoder.GetMsgId(), ERROR_SOS_REQUEST_NOT_SUPPORT);
		oSapEncoder.SetSequence(oSapDecoder.GetSequence());
		SendAvenueResponse(sAvenueReqInfo,oSapEncoder.GetBuffer(),oSapEncoder.GetLen());
		return;
	}

	GetAsyncLogModule()->OnReceiveAvenueRequest(sAvenueReqInfo.dwServiceId,sAvenueReqInfo.dwMsgId);

	char *exheader;
	int exheader_len;
	oSapDecoder.GetExtHeader((void**)&exheader, &exheader_len);
	CSapTLVBodyDecoder exDecoder(exheader,exheader_len);
	exDecoder.GetValue(SAP_HEADEX_CODE_USER, sAvenueReqInfo.strUsername);

	OsapUserInfo* pUserInfo = NULL;
	if (!sAvenueReqInfo.strUsername.empty())
	{
		int nCode = m_osapManager.GetUserInfo(sAvenueReqInfo, pBuffer, nLen, &pUserInfo);
		if (nCode > 0)
		{
			return; //waiting for osap response
		}
	}

	DoSendHttpRequest(sAvenueReqInfo, pBuffer, nLen, pUserInfo);

}

void CHpsSessionManager::DoSendHttpRequest( SAvenueRequestInfo &sAvenueReqInfo,
						const void * pBuffer, int nLen, OsapUserInfo* pUserInfo)
{
	CSapTLVBodyEncoder exEncoder;
	exEncoder.SetValue(HTTPTRAN_CODE_USERNAME, sAvenueReqInfo.strUsername);
	if (pUserInfo != NULL)
	{
		timeval_a now;
		gettimeofday_a(&now,NULL);
		for (vector<OsapPassword>::const_iterator iter = pUserInfo->vecPassword.begin();
			iter != pUserInfo->vecPassword.end(); ++iter)
		{
			if (now.tv_sec >= iter->nBeginTime && now.tv_sec <= iter->nEndTime)
			{
				exEncoder.SetValue(HTTPTRAN_CODE_USERKEY, iter->strKey);
				break;
			}
		}
		for (vector<OsapKeyValue>::iterator iter = pUserInfo->vecKeyValue.begin();
			iter != pUserInfo->vecKeyValue.end(); ++iter)
		{
			exEncoder.SetValue(iter->nKey, iter->strValue);
		}
	}
	sAvenueReqInfo.nAvenueRetCode = m_pTransfer->DoTransferRequestAvenue2Http(sAvenueReqInfo.dwServiceId,
		sAvenueReqInfo.dwMsgId, pBuffer, nLen,sAvenueReqInfo.strSosIp.c_str(),sAvenueReqInfo.dwSosPort,sAvenueReqInfo.strHttpUrl,
		(void*)m_szTransferData.arrTransferData, &m_nLen, sAvenueReqInfo.strHttpMethod,exEncoder.GetBuffer(), exEncoder.GetLen());
	if(0 != sAvenueReqInfo.nAvenueRetCode)
	{
		AvenueRequestFail(sAvenueReqInfo);
	}
	else
	{
		sAvenueReqInfo.strHttpRequestBody.assign((char*)m_szTransferData.arrTransferData, m_nLen);	
		BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, containerId[%u], sosSessionId[%u], connId[%u], remoteIp[%s], remotePort[%u], url[%s], body[%s]\n",
			__FUNCTION__,sAvenueReqInfo.dwSosSessionContainerId,sAvenueReqInfo.dwSosSessionId,
			sAvenueReqInfo.dwSosConnectionId,sAvenueReqInfo.strSosIp.c_str(),sAvenueReqInfo.dwSosPort,sAvenueReqInfo.strHttpUrl.c_str(),sAvenueReqInfo.strHttpRequestBody.c_str());

		m_pHttpClientContainer->SendHttpRequest(sAvenueReqInfo);
	}
}

void CHpsSessionManager::OnReceiveHttpResponse(SAvenueRequestInfo &sAvenueReqInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, serviceId[%u], msgId[%u], avenueCode[%d], httpCode[%d], httpResponse[%s]\n",__FUNCTION__,
			sAvenueReqInfo.dwServiceId, sAvenueReqInfo.dwMsgId, sAvenueReqInfo.nAvenueRetCode,sAvenueReqInfo.nHttpCode,sAvenueReqInfo.strHttpResponse.c_str());
	if(0 != sAvenueReqInfo.nAvenueRetCode)
	{
		AvenueRequestFail(sAvenueReqInfo);
		return;
	}

	sAvenueReqInfo.nAvenueRetCode = m_pTransfer->DoTransferResponseHttp2Avenue(sAvenueReqInfo.dwServiceId,
		sAvenueReqInfo.dwMsgId,sAvenueReqInfo.dwAvenueSequence,sAvenueReqInfo.nAvenueRetCode,sAvenueReqInfo.nHttpCode,sAvenueReqInfo.strHttpResponse,
    	(void*)m_szTransferData.arrTransferData, &m_nLen);
	
	if(0 != sAvenueReqInfo.nAvenueRetCode)
	{
		AvenueRequestFail(sAvenueReqInfo);
		return;
	}

	SendAvenueResponse(sAvenueReqInfo,m_szTransferData.arrTransferData,m_nLen);
}

void CHpsSessionManager::OnReceiveServiceResponse(const void *pBuffer, int nLen)
{
    BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, pBuffer[%p], nLen[%d]\n", __FUNCTION__, pBuffer, nLen);
    void *pBufferCopy = new char[nLen];
    memcpy(pBufferCopy, pBuffer, nLen);
    m_oIoService.post(boost::bind(&CHpsSessionManager::DoReceiveServiceResponse, this, pBufferCopy, nLen));
}

void CHpsSessionManager::DoReceiveServiceResponse(const void *pBuffer, int nLen)
{
    BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, pBuffer[%p], nLen[%d]\n", __FUNCTION__, pBuffer, nLen);
    OnReceiveSosAvenueResponse(0, pBuffer, nLen, "127.0.0.1", 0);
    delete [](const char *)pBuffer;
}

void CHpsSessionManager::AvenueRequestFail(SAvenueRequestInfo &sAvenueReqInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, serviceId[%u], msgId[%u], avenueCode[%d]\n",__FUNCTION__,sAvenueReqInfo.dwServiceId, sAvenueReqInfo.dwMsgId, sAvenueReqInfo.nAvenueRetCode);
	CSapEncoder oSapEncoder(SAP_PACKET_RESPONSE, sAvenueReqInfo.dwServiceId, sAvenueReqInfo.dwMsgId, sAvenueReqInfo.nAvenueRetCode);
	oSapEncoder.SetSequence(sAvenueReqInfo.dwAvenueSequence);
	SendAvenueResponse(sAvenueReqInfo,oSapEncoder.GetBuffer(),oSapEncoder.GetLen());
}

int CHpsSessionManager::SendAvenueResponse(SAvenueRequestInfo &sAvenueReqInfo, const void *pBuffer, int nLen)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, serviceId[%u], msgId[%u], avenueCode[%d]\n",__FUNCTION__,sAvenueReqInfo.dwServiceId, sAvenueReqInfo.dwMsgId, sAvenueReqInfo.nAvenueRetCode);

	RecordAvenueRequest(sAvenueReqInfo);

	map<unsigned int, CSosSessionContainer*>::iterator itr = m_mapSosSessionContainerByContainerId.find(sAvenueReqInfo.dwSosSessionContainerId);
	if(itr != m_mapSosSessionContainerByContainerId.end())
	{
		CSosSessionContainer* pContainer = itr->second;
		sAvenueReqInfo.nAvenueRetCode = pContainer->DoSendAvenueResponse(sAvenueReqInfo.dwSosSessionId,sAvenueReqInfo.dwSosConnectionId,pBuffer,nLen);
		if(sAvenueReqInfo.nAvenueRetCode != 0)
		{			
			BS_XLOG(XLOG_ERROR,"CHpsSessionManager::%s, DoSendAvenueResponse fail. containerId[%u], sosSessionId[%u], connId[%u], remoteIp[%s], remotePort[%u], bufferLen[%d], ret[%d]\n",	__FUNCTION__,
				sAvenueReqInfo.dwSosSessionContainerId,sAvenueReqInfo.dwSosSessionId,sAvenueReqInfo.dwSosConnectionId,sAvenueReqInfo.strSosIp.c_str(),sAvenueReqInfo.dwSosPort,nLen,sAvenueReqInfo.nAvenueRetCode);
		}
	}
	else
	{
		BS_XLOG(XLOG_ERROR,"CHpsSessionManager::%s, DoSendAvenueResponse fail. no containerId. containerId[%u], sosSessionId[%u], connId[%u], remoteIp[%s], remotePort[%u], bufferLen[%d]\n", __FUNCTION__,
			sAvenueReqInfo.dwSosSessionContainerId,sAvenueReqInfo.dwSosSessionId,sAvenueReqInfo.dwSosConnectionId,sAvenueReqInfo.strSosIp.c_str(),sAvenueReqInfo.dwSosPort,nLen);		
	}

	
	return 0;
}

void CHpsSessionManager::AddHistoryRequest(SRequestInfo &sReq)
{
	string strTimeoutParam;
	unsigned nTimeout = REQUEST_TIME_INTERVAL;
	if (m_oHttpReqConfigManager.FindTimeoutConfig(sReq.strUrlIdentify, strTimeoutParam, nTimeout))
	{
		string strTimeOutType = sdo::hps::CHpsRequestMsg::GetValueByKey(sReq.strUriAttribute, strTimeoutParam);
		if (strTimeoutParam == "1")
		{
			nTimeout = 500;
		}
		else if (strTimeoutParam == "2")
		{
			nTimeout = 3000;
		}
	}
	struct timeval_a interval;
	interval.tv_sec = nTimeout / 1000;
	interval.tv_usec = (nTimeout % 1000) * 1000;

    timeradd(&(sReq.tStart),&interval,&(sReq.tEnd));

    m_mapEndtimeReqInfo.insert(MMHistoryTimers::value_type(sReq.tEnd,sReq));
}

void CHpsSessionManager::RemoveHistory(SRequestInfo &sReq)
{
	std::pair<MMHistoryTimers::iterator, MMHistoryTimers::iterator> itr_pair = m_mapEndtimeReqInfo.equal_range(sReq.tEnd);
    MMHistoryTimers::iterator itr;
    for(itr=itr_pair.first; itr!=itr_pair.second; ++itr)
    {
        SRequestInfo & oQueryRequest = itr->second;
        if(oQueryRequest.dwSequence == sReq.dwSequence)
        {
            m_mapEndtimeReqInfo.erase(itr);
            break;
        }
    }
}

void CHpsSessionManager::DetectRequestTimerList()
{
    struct timeval_a now;
	gettimeofday_a(&now,0);
    while(!m_mapEndtimeReqInfo.empty())
    {
        if((m_mapEndtimeReqInfo.begin()->first)<now)
        {
            SRequestInfo & oRequest = m_mapEndtimeReqInfo.begin()->second;
            oRequest.nCode = ERROR_SOS_RESPONSE_TIMEOUT;
			oRequest.strSosIp = "";
			oRequest.dwSosPort = 0;
			SendHttpResponse(oRequest);
			//m_pHpsContainer->DoSendResponse(oRequest);
			RecordeHttpRequest(oRequest);
		
	    	m_mapSeqReqInfo.erase(oRequest.dwSequence);               
            m_mapEndtimeReqInfo.erase(m_mapEndtimeReqInfo.begin());
        }
        else
        {
            break;
        }
    }
	m_osapManager.DetectOsapRequestTimout(now);
}

void CHpsSessionManager::GetSelfCheck(bool &isRun)
{
	isRun = m_isAlive;
	m_isAlive = false;
}

void CHpsSessionManager::DoSelfCheck()
{
	m_isAlive = true;
}

void CHpsSessionManager::DoReportSosStatusCheckData()
{
	if(m_isAlive)
	{
		SSosStatusData sData;
		map<unsigned int, CSosSessionContainer*>::iterator iter;
		for(iter = m_mapSosSessionContainerByContainerId.begin(); iter != m_mapSosSessionContainerByContainerId.end(); ++iter)
		{
			CSosSessionContainer* pContainer = iter->second;
			SSosStatusData sTemp;
			pContainer->GetSosStatusData(sTemp);
			sData = sData + sTemp;			
		}		

		GetAsyncLogModule()->OnReportSosStatus(sData);

		unsigned int dwNum = m_pHttpClientContainer->GetCurrentRequestNum();
		GetAsyncLogModule()->OnReportNoHttpResNum(m_nThreadId, dwNum);
	}
}

void CHpsSessionManager::RecordeHttpRequest(SRequestInfo & sReq)
{	
	struct timeval_a now,interval;
	gettimeofday_a(&now,0); 		
	timersub(&now,&(sReq.tStart),&interval);

	sReq.fSpentTime = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;
	
	GetAsyncLogModule()->OnDetailLog(sReq);
}

void CHpsSessionManager::RecordAvenueRequest(SAvenueRequestInfo &sAvenueReqInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsSessionManager::%s, serviceId[%u], msgId[%u], avenueCode[%d]\n",__FUNCTION__,sAvenueReqInfo.dwServiceId, sAvenueReqInfo.dwMsgId, sAvenueReqInfo.nAvenueRetCode);
	struct timeval_a now,interval;
	gettimeofday_a(&now,0); 		
	timersub(&now,&(sAvenueReqInfo.tStart),&interval);

	sAvenueReqInfo.fAllSpentTime = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;

	GetAsyncLogModule()->OnDetailAvenueRequestLog(sAvenueReqInfo);
}

string CHpsSessionManager::GetPluginSoInfo()
{
	if(m_pTransfer)
		return m_pTransfer->GetPluginSoInfo();
	else
		return "";
}

void CHpsSessionManager::GetUrlMapping(vector<string> &vecUrlMappin)
{
	vecUrlMappin = m_vecUrlMappingForCheck;
}

void CHpsSessionManager::Dump()
{
	BS_XLOG(XLOG_NOTICE,"===========CHpsSessionManager::%s======sm_dwSeq[%u]======\n",__FUNCTION__,m_dwSeq);

	BS_XLOG(XLOG_NOTICE,"\n");
	BS_XLOG(XLOG_NOTICE,"   m_mapSeqReqInfo.size()[ %u ], <dwSequence, addr>:\n", m_mapSeqReqInfo.size());
	map<unsigned int, SRequestInfo>::iterator iter2;
	for(iter2 = m_mapSeqReqInfo.begin(); iter2 != m_mapSeqReqInfo.end(); ++iter2)
	{
		BS_XLOG(XLOG_NOTICE,"	   ---<%u, %s:%u>\n", iter2->first, iter2->second.strRemoteIp.c_str(), iter2->second.dwRemotePort);
	}

	BS_XLOG(XLOG_NOTICE,"\n");
	//BS_XLOG(XLOG_NOTICE,"   m_multimapIdSeq.size()[ %d ], <id, dwSequence>:\n", m_multimapIdSeq.size());
	//multimap<unsigned int, unsigned int >::const_iterator iter3;
	//for(iter3 = m_multimapIdSeq.begin(); iter3 != m_multimapIdSeq.end(); ++iter3)
	//{
	//	BS_XLOG(XLOG_NOTICE,"	  ---id[%u], seq[ %u ]\n", iter3->first, iter3->second);
	//}

	BS_XLOG(XLOG_NOTICE,"\n");
	BS_XLOG(XLOG_NOTICE,"   m_mapEndtimeReqInfo.size()[ %d ], <time, dwSequence>:\n", m_mapEndtimeReqInfo.size());
	MMHistoryTimers::iterator itrTime;
	for(itrTime = m_mapEndtimeReqInfo.begin(); itrTime != m_mapEndtimeReqInfo.end(); ++itrTime)
	{
		BS_XLOG(XLOG_NOTICE,"	  ---time[ %u.%u ], seq[ %u ]\n", itrTime->first.tv_sec, 
			itrTime->first.tv_usec, itrTime->second.dwSequence);
	}


	BS_XLOG(XLOG_NOTICE,"\n");
	BS_XLOG(XLOG_NOTICE,"   m_mapSosSessionContainerByServiceId.size()[ %d ],<serviceId, sosContainerId>:\n", m_mapSosSessionContainerByServiceId.size());
	map<unsigned int, CSosSessionContainer*>::iterator iter4;
	for(iter4 = m_mapSosSessionContainerByServiceId.begin(); iter4 != m_mapSosSessionContainerByServiceId.end(); ++iter4)
	{
		BS_XLOG(XLOG_NOTICE,"      ---serviceId[%u],sosContainerId[%d]\n", iter4->first,iter4->second->GetContainerId());
	}

	BS_XLOG(XLOG_NOTICE,"\n");
	BS_XLOG(XLOG_NOTICE,"   +m_mapSosSessionContainerByContainerId.size[ %d ]\n", m_mapSosSessionContainerByContainerId.size());
	map<unsigned int, CSosSessionContainer*>::iterator iter5 ;
	for(iter5 = m_mapSosSessionContainerByContainerId.begin(); iter5 != m_mapSosSessionContainerByContainerId.end(); ++iter5)
	{
		BS_XLOG(XLOG_NOTICE,"     -m_mapSosSessionContainerByContainerId[%u]\n", iter5->first);
		(iter5->second)->Dump();
		BS_XLOG(XLOG_NOTICE,"\n");
	}

	BS_XLOG(XLOG_NOTICE,"\n");
	m_pHpsContainer->Dump();

	m_pTransfer->Dump();

	m_oHttpReqConfigManager.Dump();

	m_pHttpClientContainer->Dump();
}



