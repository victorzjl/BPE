#include "HpsStack.h"
#include <boost/bind.hpp>
#include "BusinessLogHelper.h"
#include <boost/date_time.hpp>
#include "XmlConfigParser.h"
#include "AsyncLogThread.h"
#include "UpdateManager.h"
#include "tinyxml/tinyxml.h"
#include "HttpRecVirtualClient.h"
#include "HttpRecConfig.h"
#include <boost/filesystem.hpp>

CHpsStack * CHpsStack::sm_instance = NULL;
int CHpsStack::nOsapOption = 0;

CHpsStack * CHpsStack::Instance()
{
    if(sm_instance==NULL)
        sm_instance=new CHpsStack;
    return sm_instance;
}

void CHpsStack::Start(CHttpRecVirtualClient *pVirtualClient)
{
    BS_XLOG(XLOG_DEBUG,"CHpsStack::Start...\n");
    HttpRecConfigPtr pConfig = CHttpRecConfig::GetInstance();
    m_pVirtualClient = pVirtualClient;
	nOsapOption = pConfig->IsOsapVaild();
	m_nThread = pConfig->GetThreadNumbers()+ 1;
    ppThreads=new CHpsSessionManager *[m_nThread];
    
    for(int i=0; i<m_nThread; i++)
    {
        unsigned int affinity = 0x01;
        CHpsSessionManager *pThread = new CHpsSessionManager(m_pVirtualClient, i);
        ppThreads[i]= pThread;
        pThread->Start(affinity);
    }


	if (1 == CHpsStack::nOsapOption)
	{
		//connect to osap service
		string osapServerAddrs = pConfig->GetOsapServerAddrs();
		if (!osapServerAddrs.empty())
		{
			string osapSos = "<ServiceId>2</ServiceId><ServerAddr>";
			osapSos += osapServerAddrs + "</ServerAddr>";
			vector<string> vecSosListConfig;
			vecSosListConfig.push_back(osapSos);
			BS_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s load osap sos\n", __FUNCTION__);
			OnLoadSosList(vecSosListConfig);
		}
	}

	OnLoadTransferObjs(pConfig->GetTransferConfig()._strH2ADir,
                       pConfig->GetTransferConfig()._strH2AFilter, 
                       pConfig->GetTransferConfig()._strA2HDir, 
                       pConfig->GetTransferConfig()._strA2HFilter);


    map<string, string> mapIpLimits;
    CIpLimitLoader::GetInstance()->LoadConfig(boost::filesystem::current_path().string() + "/iplimit.xml", mapIpLimits);
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecVirtualClient::%s update ip limit\n", __FUNCTION__);
    OnUpdateIpLimit(mapIpLimits);
}

void CHpsStack::Stop()
{
    BS_XLOG(XLOG_DEBUG,"CHpsStack::Stop\n");
    for(int i=0;i<m_nThread;i++)
    {
        CHpsSessionManager *pThread = ppThreads[i];
        pThread->Stop();
        delete pThread;
    }
}

int CHpsStack::OnLoadTransferObjs(const string &strH2ADir, const string &strH2AFilter, const string &strA2HDir, const string &strA2HFilter)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s, strH2ADir[%s], strH2AFilter[%s], strA2HDir[%s], strA2HFilter[%s]\n", 
		__FUNCTION__,strH2ADir.c_str(), strH2AFilter.c_str(),strA2HDir.c_str(), strA2HFilter.c_str());
	for(int i=0;i<m_nThread;i++)
    {
	
    	BS_XLOG(XLOG_DEBUG,"CHpsStack::OnLoadTransferObjs, nthread[%d], i[%d]\n",m_nThread,i);
        CHpsSessionManager * pThread=ppThreads[i];
        pThread->OnLoadTransferObjs(strH2ADir, strH2AFilter, strA2HDir, strA2HFilter);
    }

	return 0;
}

void CHpsStack::OnLoadSosList(const vector<string> & vecServer)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s..\n", __FUNCTION__);
	for(int i=0;i<m_nThread;i++)
    {
	
    	BS_XLOG(XLOG_DEBUG,"CSapStack::OnLoadSosList, nthread[%d], i[%d]\n",m_nThread,i);
        CHpsSessionManager * pThread=ppThreads[i];
        pThread->OnLoadSosList(vecServer);
    }
}

void CHpsStack::OnUpdateIpLimit(const map<string, string> & mapIpLimits)
{
	for(int i=0;i<m_nThread;i++)
    {
        CHpsSessionManager * pThread=ppThreads[i];
        pThread->OnUpdateIpLimit(mapIpLimits);
    }
}


void CHpsStack::OnUnLoadSharedLibs(const string &strDirPath,const vector<string> &vecSo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s..\n", __FUNCTION__);
	for(int i=0;i<m_nThread;i++)
	{
		
		BS_XLOG(XLOG_DEBUG,"CSapStack::OnUnLoadSharedLibs, nthread[%d], i[%d]\n",m_nThread,i);
		CHpsSessionManager * pThread=ppThreads[i];
		pThread->OnUnLoadSharedLibs(strDirPath,vecSo);
	}
}

void CHpsStack::OnReLoadTransferObjs(const string &strDirPath, const vector<string> &vecSoName)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s, strDirPath[%s]\n", __FUNCTION__,strDirPath.c_str());
	for(int i=0;i<m_nThread;i++)
	{
		BS_XLOG(XLOG_DEBUG,"CSapStack::OnReLoadTransferObjs, nthread[%d], i[%d]\n",m_nThread,i);
		CHpsSessionManager * pThread=ppThreads[i];
		pThread->OnReLoadSharedLibs(strDirPath,vecSoName);
	}
}

int CHpsStack::OnLoadRouteConfig(const vector<string> &vecRouteBalance)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s\n", __FUNCTION__);
	for(int i=0;i<m_nThread;i++)
	{
		BS_XLOG(XLOG_DEBUG,"CSapStack::OnReLoadTransferObjs, nthread[%d], i[%d]\n",m_nThread,i);
		CHpsSessionManager * pThread=ppThreads[i];
		pThread->OnLoadRouteConfig(vecRouteBalance);
	}

	return 0;
}

void CHpsStack::OnUpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s\n", __FUNCTION__);
	for(int i=0;i<m_nThread;i++)
	{
		BS_XLOG(XLOG_DEBUG,"CSapStack::OnUpdateRouteConfig, nthread[%d], i[%d]\n",m_nThread,i);
		CHpsSessionManager * pThread=ppThreads[i];
		pThread->OnUpdateRouteConfig(dwServiceId,dwMsgId,nErrCode);
	}
}

CHpsSessionManager * CHpsStack::GetAcceptThread()
{
    return ppThreads[m_nThread-1];
}


CHpsSessionManager * CHpsStack::GetThread()
{
    int i=(m_dwIndex++)%(m_nThread-1);
    return ppThreads[i];
}
CHpsSessionManager * CHpsStack::GetThread(int n)
{
	int i=(n)%(m_nThread-1);
	return ppThreads[i];
}
string CHpsStack::GetPluginSoInfo()
{	
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s..\n", __FUNCTION__);
	CHpsSessionManager *pThread = GetThread();
	if(pThread)
	{
		return pThread->GetPluginSoInfo();
	}
	return "";
}

void CHpsStack::GetThreadStatus(int &nThreads, int &nDeadThreads)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s..\n", __FUNCTION__);
	nThreads=m_nThread;
	for(int i=0;i<m_nThread;i++)
	{
		CHpsSessionManager * pThread=ppThreads[i];
		bool isAlive;
		pThread->GetSelfCheck(isAlive);
		if(!isAlive)
			nDeadThreads++;
	}
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s, nThreads[%d], nDeadThreads[%d]\n", __FUNCTION__,nThreads, nDeadThreads);

}

void CHpsStack::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum)
{
    BS_XLOG(XLOG_DEBUG,"CHpsStack::%s..\n", __FUNCTION__);
    dwAllNum = m_nThread;
    for(int i=0; i<m_nThread; i++)
    {
        bool isAlive;
        ppThreads[i]->GetSelfCheck(isAlive);
        if(isAlive)
        {
            dwAliveNum++;
        }
    }
}

void CHpsStack::UpdataUserInfo(const CHpsSessionManager* pManager,
					const string& strUserName, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s.. %s \n", __FUNCTION__, strUserName.c_str());
	for(int i=0;i<m_nThread;i++)
	{
		CHpsSessionManager * pThread=ppThreads[i];
		if (pThread != pManager)
		{
			OsapUserInfo* pNew = new OsapUserInfo;
			*pNew = *pUserInfo;
			pThread->OnUpdataUserInfo(strUserName, pNew);
		}
	}
}

void CHpsStack::UpdataOsapUserTime(const CHpsSessionManager* pManager,
						const string& strUserName, const timeval_a& sTime)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s.. %s \n", __FUNCTION__, strUserName.c_str());
	for(int i=0;i<m_nThread;i++)
	{
		CHpsSessionManager * pThread=ppThreads[i];
		if (pThread != pManager)
		{
			pThread->OnUpdataOsapUserTime(strUserName, sTime);
		}
	}
}

void CHpsStack::ClearOsapCatch(const string& strMerchant)
{
	BS_XLOG(XLOG_DEBUG,"CHpsStack::%s..\n", __FUNCTION__);
	for(int i=0;i<m_nThread;i++)
	{
		CHpsSessionManager * pThread=ppThreads[i];
		pThread->OnClearOsapCatch(strMerchant);
	}
}

void CHpsStack::GetUrlMapping(vector<string> &vecUrlMapping)
{
	CHpsSessionManager *pThread = GetThread();
	if(pThread)
	{
		pThread->GetUrlMapping(vecUrlMapping);
	}
}

int CHpsStack::OnUpdateConfigFromOsap(bool bWait/* = true*/)
{
	if (!m_bUpdate && CUpdateManager::sm_nUpdateOption != 0)
	{
		m_bUpdate = true;
		static CUpdateManager *pUM;
		if (pUM != NULL)
		{
			pUM->Stop();
			delete pUM;
		}
		pUM = new CUpdateManager;
		boost::unique_lock<boost::mutex> lock(CUpdateManager::sm_mut);   	
		pUM->Start();
		if (bWait)
		{
			CUpdateManager::sm_cond.timed_wait<boost::posix_time::seconds>(lock,
				boost::posix_time::seconds(20));
		}
		return CUpdateManager::sm_nUpateResult;
	}
	return -20;
}

void CHpsStack::UpdateFinished(int nUpdateStat)
{
	if (nUpdateStat == 0)
	{
		CXmlConfigParser oConfig;
		if(oConfig.ParseFile("./config.xml")!=0)
		{
			BS_XLOG(XLOG_ERROR, "parse config.xml error:%s\n",oConfig.GetErrorMessage().c_str());
			CUpdateManager::sm_nUpateResult = -23;
			CUpdateManager::sm_cond.notify_one();
			m_bUpdate = false;
			return;
		}
		vector<string> vecSosListConfig = oConfig.GetParameters("SosList");
		CXmlConfigParser oServer;
		if(oServer.ParseFile("./server.xml")==0)
		{
			vector<string> vecSosListServer = oServer.GetParameters("SosList");
			vecSosListConfig.insert(vecSosListConfig.begin(), vecSosListServer.begin(), vecSosListServer.end());
		}
		for(int i=0;i<m_nThread;i++)
		{
			CHpsSessionManager * pThread=ppThreads[i];
			pThread->OnReLoadCommonTransfer(vecSosListConfig,
				"./plugin/HpsCommonTransfer/HpsCommonTransfer.so");	
		}
		string strReloadErorMsg;
		int nRet = CAvenueHttpTransfer::WaitForReload(1,strReloadErorMsg);
		CUpdateManager::sm_nUpateResult = nRet;
	}
	else
	{
		CUpdateManager::sm_nUpateResult = nUpdateStat;
	}
	CUpdateManager::sm_cond.notify_one();
	m_bUpdate = false;
}

SUrlInfo CHpsStack::GetSoNameByUrl(const string& strUrl)
{
	CHpsSessionManager *pThread = GetThread();
	if(pThread)
	{
		return pThread->GetSoNameByUrl(strUrl);
	}
	return SUrlInfo();
}

void CHpsStack::Dump()
{
    for(int i=0;i<m_nThread;i++)
    {
        CHpsSessionManager * pThread=ppThreads[i];
        BS_XLOG(XLOG_NOTICE,"\n\n\n=====DUMP[%d], manager[%x]\n",i,pThread);
        pThread->Dump();
    }
}


