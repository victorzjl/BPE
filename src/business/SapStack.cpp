#include "SapStack.h"
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp> 
#include "SessionManager.h"
#include "AsyncLogThread.h"
#include "SapLogHelper.h"
#include "XmlConfigParser.h"
#include "TriggerConfig.h"

//#include "KeyConfig.h"
#include "CPPFunctionManager.h"
CSapStack * CSapStack::sm_instance=NULL;
int CSapStack::isAsc=1;
int CSapStack::isLocalPrivilege=0;
int CSapStack::isAuthSoc=0;
int CSapStack::isVerifySocSignature=0;
int CSapStack::isSocEnc=0;
string CSapStack::strLocalKey="snda123@sdo456";
int CSapStack::nLogType=0;

CSapStack * CSapStack::Instance()
{
    if(sm_instance==NULL)
        sm_instance=new CSapStack;
    return sm_instance;
}
CSapStack::CSapStack():ppThreads(NULL),m_nThread(1),m_dwIndex(0),m_minDataServiceId(60000)
{
}

void CSapStack::Start(int nThread)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::Start[%d]...\n",nThread);
    m_nThread=nThread;
    ppThreads=new CSessionManager *[m_nThread];
    
    for(int i=0;i<m_nThread;i++)
    {
        unsigned int affinity=0x01;
        CSessionManager * pThread=new  CSessionManager(i);
        ppThreads[i]=pThread;
        //affinity<<i;
        pThread->Start(affinity);
    }
}
void CSapStack::Stop()
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::Stop\n");
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        pThread->OnStop();
        //delete pThread;
    }
}
void CSapStack::Init(int visAuthSoc,int visVerifySocSignature,int visSocEnc,const string &vstrLocalKey)
{
	SS_XLOG(XLOG_TRACE,"CSapStack::Init, isAuthSoc[%d], isVerifySoc[%d],isEnc[%d]\n",visAuthSoc,visVerifySocSignature,visSocEnc);
	isAuthSoc=visAuthSoc;
	isVerifySocSignature=visVerifySocSignature;
    isSocEnc=visSocEnc;
	strLocalKey=vstrLocalKey;
}
int CSapStack::LoadConfig(const char* pConfig)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadConfig\n");
    CXmlConfigParser oConfig;
    if(oConfig.ParseFile(pConfig)!=0)
    {
        SS_XLOG(XLOG_ERROR, "CSapStack::%s,parse config error:%s\n", __FUNCTION__,
                oConfig.GetErrorMessage().c_str());
        return -1;
    }    
	
    /*read soc basic*/
    Init(oConfig.GetParameter("IsAuthSoc", 0),
        oConfig.GetParameter("IsVerifySocSignature", 0),
        oConfig.GetParameter("IsSocEnc", 0),
        oConfig.GetParameter("LocalKey", "snda123@sdo456"));

	/*temp config*/
	isAsc=oConfig.GetParameter("IsAsc", 1);

    nLogType = oConfig.GetParameter("LogType", 0);

    /*read sos config*/
    vector<SSosStruct> vecSos;
    vector<string> vecSosStri=oConfig.GetParameters("SosList");
    vector<string>::iterator itrSosStr;
    for(itrSosStr=vecSosStri.begin();itrSosStr!=vecSosStri.end();++itrSosStr)
    {
        string strDetail=*itrSosStr;
        CXmlConfigParser oDetailConfig;
        oDetailConfig.ParseDetailBuffer(strDetail.c_str());

        SSosStruct oSos;
		string strServiceId=oDetailConfig.GetParameter("ServiceId","0");
		string strServiceIdMessageId=oDetailConfig.GetParameter("ServiceIdMessageId","0");
		vector<string> vecService;
		boost::algorithm::split( vecService, strServiceId, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
		for (vector<string>::iterator istr = vecService.begin(); 
			istr != vecService.end(); ++istr)
		{
            unsigned serviceId = atoi(istr->c_str());
			oSos.vecServiceId.push_back(serviceId);

            m_minDataServiceId = serviceId > m_minDataServiceId ? 
                serviceId + 10000 - ((serviceId + 10000) % 10000) : m_minDataServiceId;
		}
		sort(oSos.vecServiceId.begin(),oSos.vecServiceId.end());
        oSos.vecAddr=oDetailConfig.GetParameters("ServerAddr");
        sort(oSos.vecAddr.begin(),oSos.vecAddr.end());
		if (strcmp(strServiceIdMessageId.c_str(),"0")!=0)
		{
			vector<string> vecServiceMessage;
			boost::algorithm::split( vecServiceMessage, strServiceIdMessageId, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
			for (vector<string>::iterator istr = vecServiceMessage.begin(); istr != vecServiceMessage.end(); ++istr)
			{
				vector<string> vecs;
				boost::algorithm::split( vecs, *istr, boost::algorithm::is_any_of("_"), boost::algorithm::token_compress_on); 
				if (vecs.size()!=2)
					continue;
				unsigned int serviceId = atoi(vecs[0].c_str());
				unsigned int messageId = atoi(vecs[1].c_str());
				TServiceIdMessageId sm;
				sm.dwServiceId = serviceId;
				sm.dwMessageId = messageId;
				oSos.vecServiceIdMessageId.push_back(sm);
			}
		}
        vecSos.push_back(oSos);
    }

    /*read data server sos config*/
    vecSosStri = oConfig.GetParameters("DataServerList");
    for(itrSosStr=vecSosStri.begin();itrSosStr!=vecSosStri.end();++itrSosStr)
    {
        string strDetail=*itrSosStr;
        CXmlConfigParser oDetailConfig;
        oDetailConfig.ParseDetailBuffer(strDetail.c_str());

        SSosStruct oSos;
        string strServiceId=oDetailConfig.GetParameter("DataServerId","0");
        vector<string> vecService;
        boost::algorithm::split( vecService, strServiceId, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
        for (vector<string>::iterator istr = vecService.begin(); 
            istr != vecService.end(); ++istr)
        {
            unsigned serviceId = atoi(istr->c_str()) + m_minDataServiceId;
            oSos.vecServiceId.push_back(serviceId);
        }
        sort(oSos.vecServiceId.begin(),oSos.vecServiceId.end());
        oSos.vecAddr=oDetailConfig.GetParameters("ServerAddr");
        sort(oSos.vecAddr.begin(),oSos.vecAddr.end());
        vecSos.push_back(oSos);
    }

    for (vector<SSosStruct>::const_iterator itr = vecSos.begin(); itr != vecSos.end(); ++itr)
    {
        SS_XLOG(XLOG_DEBUG,"id[%u]  address[%s]\n",itr->vecServiceId[0], itr->vecAddr[0].c_str());
    }

    LoadSos(vecSos);

    /*read soc config*/
    LoadSuperSoc(oConfig.GetParameters("SuperList/Addr"),oConfig.GetParameters("SuperPrivilege/Privilege"));
    /*if (CTriggerConfig::Instance()->LoadConfig("./trigger_config.xml")!=0||
        CKeyConfig::Instance()->LoadConfig("./key_config.xml") !=0)
    {
        SS_XLOG(XLOG_ERROR, "load trigger or key config error\n"); 
        return -1;
    }*/

    if (CTriggerConfig::Instance()->LoadConfig("./trigger_config.xml")!=0)
    {
        SS_XLOG(XLOG_ERROR, "load trigger config error\n"); 
        return -1;
    }
    return 0;
}

int CSapStack::LoadDataServerConfig(map<SComposeKey, vector<unsigned> >& mapDataServerId, const char* pConfig)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadDataServerConfig\n");

    CXmlConfigParser oConfig;

    if(oConfig.ParseFile(pConfig) != 0)
    {
        SS_XLOG(XLOG_DEBUG, "CSapStack::%s,parse config error:%s\n", __FUNCTION__,
                oConfig.GetErrorMessage().c_str());
        return -1;
    }

    /*read data server config*/
    vector<string> vecDataconfigStr = oConfig.GetParameters("DataConfig");
    vector<string>::iterator itr = vecDataconfigStr.begin();
    for(; itr != vecDataconfigStr.end(); ++itr)
    {
        string& strDetail = *itr;
        CXmlConfigParser oDetailConfig;
        oDetailConfig.ParseDetailBuffer(strDetail.c_str());

        string strServiceId     = oDetailConfig.GetParameter("ServiceId","0");
        string strMsgIds        = oDetailConfig.GetParameter("MsgId","0");
        string strDataServerIds = oDetailConfig.GetParameter("DataServerId","0");
        vector<string> vecMsgIdStr;
        vector<string> vecDataServerIdStr;
        boost::algorithm::split(vecMsgIdStr, strMsgIds, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
        boost::algorithm::split(vecDataServerIdStr, strDataServerIds, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 

        vector<unsigned> vecDataServerId;
        vector<string>::iterator itrDataServerid = vecDataServerIdStr.begin();
        for (; itrDataServerid != vecDataServerIdStr.end(); ++itrDataServerid)
        {
            vecDataServerId.push_back(atoi(itrDataServerid->c_str()) + m_minDataServiceId);
        }
        sort(vecDataServerId.begin(), vecDataServerId.end());

        unsigned serviceId = atoi(strServiceId.c_str());
        vector<string>::iterator itrMsgId = vecMsgIdStr.begin();
        for (; itrMsgId != vecMsgIdStr.end(); ++itrMsgId)
        {
            unsigned msgId = atoi(itrMsgId->c_str());
            mapDataServerId.insert(make_pair(SComposeKey(serviceId, msgId), vecDataServerId));
        }
    }

    return 0;
}

void CSapStack::LoadSos(const vector<SSosStruct> & vecSos)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadSos, nthread[%d]\n",m_nThread);
    for(int i=0;i<m_nThread;i++)
    {
	
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadSos, nthread[%d], i[%d]\n",m_nThread,i);
        CSessionManager * pThread=ppThreads[i];
        pThread->OnLoadSos(vecSos);
    }
    //CAsyncLogThread::GetInstance()->OnReloadSos(vecSos);
}
void CSapStack::LoadSuperSoc(const vector<string> & vecSoc,const vector<string> & strPrivelege)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadSuperSoc\n");
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        pThread->OnLoadSuperSoc(vecSoc,strPrivelege);
    }
}

int CSapStack::LoadServiceConfig()
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadServiceConfig\n"); 

    map<SComposeKey, vector<unsigned> > mapDataServerId;
    if (LoadDataServerConfig(mapDataServerId) != 0)
    {
        SS_XLOG(XLOG_DEBUG, "CSapStack::%s, BPE will be running without data server config\n", __FUNCTION__);
    }

    CComposeServiceContainer oComposeConfig;
    CAvenueServiceConfigs oServiceConfig;
    if(oServiceConfig.LoadAvenueServiceConfigs("./avenue_conf/") != 0 || 
        oComposeConfig.LoadComposeServiceConfigs("./compose_conf", "./data_compose_conf", oServiceConfig, mapDataServerId)!= 0)
    {
        return -1;
    }
	
	//oServiceConfig.Dump();
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        pThread->OnLoadConfig(oComposeConfig,oServiceConfig);        
    }
    return 0;
}

int CSapStack::LoadVirtualService()
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadVirtualService\n");    
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        pThread->OnLoadVirtualService();        
    }
    return 0;
}

int CSapStack::LoadAsyncVirtualService()
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadAsyncVirtualService\n");    
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        pThread->OnLoadAsyncVirtualService();
    }
    return 0;
}

int CSapStack::LoadAsyncVirtualClientService()
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::LoadAsyncVirtualClientService\n");    
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        pThread->OnLoadAsyncVirtualClientService();
    }
    return 0;
}

int CSapStack::InstallVirtualService(const string& strName)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::%s\n",__FUNCTION__);    
    for(int i=0;i<m_nThread;i++)
    {
        int nCode = 504;
        CSessionManager * pThread=ppThreads[i];
        pThread->OnInstallSOSync(strName, nCode);		
        if(0 != nCode)
        {
            return nCode;
        }		
    }
    return 0;
}


int CSapStack::UnInstallVirtualService(const string& strName)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::%s\n",__FUNCTION__);  
    for(int i=0;i<m_nThread;i++)
    {
        int nCode = 504;
        CSessionManager * pThread=ppThreads[i];
        pThread->OnUnInstallSOSync(strName, nCode);
        if(0 != nCode)
        {
            return nCode;
        }
    }
    return 0;
}

CSessionManager * CSapStack::GetThread()
{
    int i=(m_dwIndex++)%m_nThread;
    return ppThreads[i];
}

void CSapStack::HandleRequest(void *pBuffer, int nLen)
{
	SS_XLOG(XLOG_DEBUG,"CSapStack::HandleRequest\n");
	GetThread()->OnAckRequestEnqueue(pBuffer, nLen);
}

void CSapStack::GetThreadStatus(int &nThreads,int &nDeadThread)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::GetThreadStatus\n");
    nThreads=m_nThread;
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        bool isAlive=false;
        pThread->GetThreadSelfCheck(isAlive);
        if(!isAlive)
            nDeadThread++;            
    }
}

void CSapStack::GetConnStatus(SConnectData &oData)
{
    SS_XLOG(XLOG_DEBUG,"CSapStack::%s\n",__FUNCTION__); 
    for(int i=0;i<m_nThread;i++)
    {
        oData += ppThreads[i]->GetConnData();              
    }
}


const string CSapStack::GetPluginInfo()
{    
    string strInfo;
    ppThreads[0]->OnGetPluginInfo(strInfo);
    return strInfo;
}

const string CSapStack::GetConfigInfo()
{    
    string strInfo;
    ppThreads[0]->OnGetConfigInfo(strInfo);
    return strInfo;
}

void CSapStack::SetAllConfig(const void* buffer, const int len)
{
    //TODO
    for(int i=0;i<m_nThread;i++)
    {
        ppThreads[i]->OnSetConfig(buffer, len);
    }
}


void CSapStack::Dump()
{
    CTriggerConfig::Instance()->Dump();
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        SS_XLOG(XLOG_NOTICE,"\n\n\n=====DUMP[%d], manager[%x]\n",i,pThread);
        pThread->Dump();
    }    
}

bool CSapStack::IsClosed()
{
    for(int i=0;i<m_nThread;i++)
    {
        CSessionManager * pThread=ppThreads[i];
        if (!pThread->IsAllComposeServiceObjClosed())
        {
            return false;
        }
    }

    return true;
}


