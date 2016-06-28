#include "DbThreadGroup.h"
#include "DbAngent.h"

#include "DbReconnThread.h"
#include "DbpService.h"
#include "AsyncLogThread.h"
#include "dbserviceconfig.h"
#include "SapTLVBody.h"
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/ioctl.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <net/if_arp.h> 
#include <arpa/inet.h> 
#include "SmartThread.h"

using namespace sdo::sap;

static int get_eth_address (struct in_addr *addr)
{
	struct ifreq req;
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);

    int i=0;
    for(i=0;i<6;++i)
    {
        char szEth[5]={0};
        sprintf(szEth,"eth%d",i);
        strncpy (req.ifr_name, szEth, IFNAMSIZ);
        if ( ioctl(sock, SIOCGIFADDR, &req)==0)break;
    }
    close(sock);
	if ( i==6)
	{
		return -1;
	}
	memcpy (addr, &((struct sockaddr_in *) &req.ifr_addr)->sin_addr, sizeof (struct in_addr));
	return 0;
}

CDbThreadGroup * CDbThreadGroup::sm_instance=NULL;
CDbThreadGroup * CDbThreadGroup::GetInstance()
{
	//static CDbThreadGroup gThreadGroup;
    if(sm_instance==NULL)
        sm_instance=new CDbThreadGroup;
    return sm_instance;
}

void CDbWorkThread::DoSelfCheck()
{
	m_isAlive = true;
}


void CDbWorkThread::Deal(void *pData)
{
	m_pGroup->Deal(pData);
}

CDbWorkThread::CDbWorkThread(CMsgQueuePrio & oPublicQueue,CDbThreadGroup*pGroup):CChildMsgThread(oPublicQueue),m_pGroup( pGroup),m_timerSelfCheck(this, 20, boost::bind(&CDbWorkThread::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE),m_isAlive(true)
{

}

void CDbWorkThread::StartInThread()
{
	m_tid= pthread_self();
	m_timerSelfCheck.Start();
}

void CDbThreadGroup::QueueStatics()
{
	 string strLog="";
	 char szTmp[128]={0};
	 int dwCount = m_queue.GetCurrentCount();
	 snprintf(szTmp,sizeof(szTmp),"TaskQueue: \nBig  (%d)\n",dwCount);
	 strLog+=szTmp;
	 
	 map<int, CDbAgent2 * >::iterator it_db = m_dbs.begin();
	 for(; it_db!= m_dbs.end(); it_db++)
	 {
		CDbAgent2 * pAgent2 = it_db->second;
		CDbAgent* pAgent = pAgent2->GetCommonAgent();
		int nQuery,nWrite;
		pAgent->GetQueueCount(nQuery,nWrite);
		char szTmp[1024]={0};
		snprintf(szTmp,sizeof(szTmp)," [%s]  {%d,%d}\n",pAgent2->GetServiceIds().c_str(),nQuery,nWrite);
		strLog+=szTmp;
	 }
	 SV_XLOG(XLOG_INFO,"CDbThreadGroup::QueueStatics %s",strLog.c_str());
}
	
void CDbThreadGroup::GetThreadState(unsigned int& nActive,unsigned int& nSum)
{
	 static unsigned int dwCallTick = 0;
	
	 nSum = m_vecChildThread.size();
	 nActive = 0;
	 if (dwCallTick % CDBServiceConfig::GetInstance()->GetBPEThreadsNum() !=0)
	 {
		nActive=nSum;
		return;
	 }
	 dwCallTick++;
	 vector<CDbWorkThread *>::iterator it_thread = m_vecChildThread.begin();
	 for(; it_thread!= m_vecChildThread.end(); it_thread++)
	 {
		CDbWorkThread * pThread = (*it_thread);
	 	if ( pThread->IsActive() )
			nActive++;
	 }
	 
	 if (nActive!=nSum)
	 {
		SV_XLOG(XLOG_WARNING,"CDbThreadGroup::SELFCHECK [%d/%d]\n",nActive,nSum);
	 }
}


void CDbThreadGroup::Start(int nThread)
{
	struct in_addr addr;
	if(get_eth_address(&addr)==0)
	{
		m_srvIp=inet_ntoa(addr);
	}
	else
	{
		m_srvIp="0.0.0.0";
	}
	
	for(int i=0;i<nThread;i++)
	{
		CDbWorkThread *pThread=new CDbWorkThread(m_queue,this);
		m_vecChildThread.push_back(pThread);
		pThread->Start();
	}
}

void CDbThreadGroup::Stop()
{
	CDbWorkThread *pThread=NULL;
	while(!m_vecChildThread.empty())
	{
		pThread=*(m_vecChildThread.rbegin());
		m_vecChildThread.pop_back();
		pThread->Stop();
		delete pThread;
	}
}



string CDbThreadGroup::GetIp()
{
	return m_srvIp;
}

CDbThreadGroup::CDbThreadGroup():m_queue(512,CDBServiceConfig::GetInstance()->GetDBBigQueueNum(),512)
{
    m_funcMap[DB_CONFIG_MSG]=&CDbThreadGroup::DoLoadConfig;
	m_funcMap[DB_QUREY_MSG]=&CDbThreadGroup::DoRequest;
	m_funcMap[DB_CONNECT_MSG] =&CDbThreadGroup::DoConnectInfo;
	
	SV_XLOG(XLOG_INFO,"CDbThreadGroup::%s BigQueueNum[%d]\n",__FUNCTION__,
			CDBServiceConfig::GetInstance()->GetDBBigQueueNum());
	 
}

void CDbThreadGroup::Clear()
{
	map<int, CDbAgent2 * >::iterator it = m_dbs.begin();
	while (it != m_dbs.end())
	{
		delete it->second;
		it++;
	}
	m_dbs.clear();
}

CDbThreadGroup::~CDbThreadGroup()
{
	//Stop();
	Clear();
}



void CDbThreadGroup::OnLoadConfig( )
{
    SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s\n",__FUNCTION__);
	
    SDbConfigMsg *pMsg=new SDbConfigMsg;
   
	gettimeofday_a(&pMsg->m_tStart, 0);

	
	//SDbConfigMsg *pConf = (SDbConfigMsg *)pMsg;
	
	m_dbs.clear();
	m_serviceindexs.clear();
	
	vector<SConnDesc>* vecs = CDBServiceConfig::GetInstance()->GetAllDBConnConfigs();
	
	int nIndex = 0;
	vector<SConnDesc>::iterator iter_desc;
	char buf[100]={0};
	for (iter_desc=vecs->begin(); iter_desc!=vecs->end();++iter_desc)
	{
		SV_XLOG(XLOG_DEBUG,"CDbThreadGroup Init Angent %d\n",nIndex);
		vector<int>::iterator it_appid = iter_desc->vecServiceId.begin();
		string strServiceIds="";
		for(;it_appid!=iter_desc->vecServiceId.end();++it_appid)
		{
			m_serviceindexs[*it_appid]=nIndex;
			snprintf(buf,sizeof(buf),"%d ",*it_appid);
			strServiceIds +=buf;
		}
		SV_XLOG(XLOG_DEBUG,"Contains: %s\n",strServiceIds.c_str());
		CDbAgent2 *pAgent2=new CDbAgent2(nIndex,*iter_desc);
		pAgent2->SetServiceIds(strServiceIds);
		m_dbs[nIndex]=pAgent2;
		if (iter_desc->dwSmart!=0)
		{
			CSmartThread::GetInstance()->PutSmart(pAgent2);
		}
		nIndex++;
	}
	
   // m_queue.PutQ(pMsg);
   DoLoadConfig(pMsg);
}


void CDbThreadGroup::OnRequest(void* sParams,  const void *pBuffer,  int len)
{
 
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s\n",__FUNCTION__);
	if(pBuffer==NULL || len <=0)
	{
		return;
	}
	
    SDbQueryMsg *pMsg=new SDbQueryMsg;
	pMsg->pRequest = malloc(len);
	memcpy(pMsg->pRequest, pBuffer, len);
	pMsg->nReqLen=len;
    pMsg->sapDec.Reset(pMsg->pRequest,pMsg->nReqLen);
	pMsg->sapDec.DecodeBodyAsTLV();
	

	pMsg->sParams = sParams;
	int dwServiceId = pMsg->sapDec.GetServiceId();
	int dwMsgId = pMsg->sapDec.GetMsgId();
	unsigned int dwSequence = pMsg->sapDec.GetSequence();
	
	SAvenueMsgHeader *pSapMsgHeader = (SAvenueMsgHeader *)pBuffer;
		
	//获取GUID
	string strGuid;
	unsigned char *pExtBuffer = (unsigned char *)pBuffer + sizeof(SAvenueMsgHeader);
	int nExtLen = pSapMsgHeader->byHeadLen-sizeof(SAvenueMsgHeader);
	if(nExtLen > 0)
	{
		SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s extlen[%d]\n", __FUNCTION__, nExtLen);
		CSapTLVBodyDecoder objExtHeaderDecoder(pExtBuffer, nExtLen);
		
		if(objExtHeaderDecoder.GetValue(GUID_CODE, strGuid)!=0)
		{
			SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s get guid failed\n", __FUNCTION__);
		}
		
		pMsg->strGuid = strGuid;
	}
	
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s,dwServiceId[%d],dwMsgId[%d],dwSequence[%u],guid[%s]\n",
        __FUNCTION__, dwServiceId,dwMsgId, dwSequence, pMsg->strGuid.c_str());
	
	SMsgAttri *pMsgAttri = CDBServiceConfig::GetInstance()->GetMsgAttriById(dwServiceId, dwMsgId);
	if(pMsgAttri == NULL)
	{
		SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s no msg attri serivceid[%d] msgid[%d]\n",__FUNCTION__, dwServiceId, dwMsgId);
		pMsg->sResponse.nCode = ERROR_NOAPP;
		CDbThreadGroup::GetInstance()->DoResponse(pMsg);
		
		//delete pMsg;
		return;
	}
	pMsg->pMsgAttri = pMsgAttri;
	if(pMsgAttri->eOperType==DBO_WRITE)
	{
		pMsg->bQueryMaster = true;
		SServiceDesc* pServiceDesc = CDBServiceConfig::GetInstance()->GetServiceDesc(dwServiceId);
		
		if (pServiceDesc->bReadOnly)
		{
			SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s ReadOnly serivceid[%d] msgid[%d]\n",__FUNCTION__, dwServiceId, dwMsgId);
			pMsg->sResponse.nCode = ERROR_READONLY;
			CDbThreadGroup::GetInstance()->DoResponse(pMsg);
			return ;
		}
	}
	
	pMsg->nDbIndex=GetDbIndex(dwServiceId);
	
	gettimeofday_a(&pMsg->m_tStart, 0);

    bool bPut = m_queue.TimedPutQ(pMsg,MSG_MEDIUM,0);
	if (!bPut)
	{
		SV_XLOG(XLOG_ERROR,"CDbThreadGroup::%s BIGQUEUE FULL\n",__FUNCTION__);
		pMsg->sResponse.nCode=ERROR_BIGQUEUEFULL;
		CDbThreadGroup::GetInstance()->DoResponse(pMsg);	
		string warnStr = "DBBroker ["+m_srvIp+"] BIGQUEUE FULL";	

		DbpService::GetInstance()->CallExceptionWarn(warnStr);		
	}
}



void CDbThreadGroup::OnConnectInfo(void *pDbConn, int nDbIndex)
{
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s,pDbConn[%d] nDbIndex[%d]\n", __FUNCTION__, 
		pDbConn, nDbIndex);
	
	SDbConnectMsg *pMsg=new SDbConnectMsg;
	pMsg->nDbIndex = nDbIndex;
	pMsg->pConn = pDbConn;
	gettimeofday_a(&pMsg->m_tStart, 0);
	
	bool bPut = m_queue.TimedPutQ(pMsg,MSG_MEDIUM,0);
	if (!bPut)
	{
		SV_XLOG(XLOG_ERROR,"CDbThreadGroup::%s BIGQUEUE FULL*\n",__FUNCTION__);
		string warnStr = "DBbroker ["+m_srvIp+"] BIGQUEUE FULL";
		DbpService::GetInstance()->CallExceptionWarn(warnStr);
	}
}

void CDbThreadGroup::Deal(void *pData)
{	
    SDbMsg *pDbMsg=(SDbMsg *)pData;
	(this->*(m_funcMap[pDbMsg->m_enType]))(pDbMsg);
}

void CDbThreadGroup::DoPing()
{
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s\n",__FUNCTION__);
	map<int, CDbAgent2 * >::iterator itAngent2 = m_dbs.begin();
	for(;itAngent2!=m_dbs.end();itAngent2++)
	{
		CDbAgent2* pAngent2 = (CDbAgent2*)(itAngent2->second);
		pAngent2->DoPing();
	}
}

void CDbThreadGroup::DoLoadConfig(SDbMsg * pMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s\n",__FUNCTION__);

	SDbConfigMsg *pConf = (SDbConfigMsg *)pMsg;

	map<int, CDbAgent2 * >::iterator itAngent2 = m_dbs.begin();
	for(;itAngent2!=m_dbs.end();itAngent2++)
	{
		CDbAgent2* pAngent2 = (CDbAgent2*)(itAngent2->second);
		pAngent2->DoInit();
	}
		
	delete pConf;
	
}

void CDbThreadGroup::DoRequest(SDbMsg * pMsg)
{
    
	SDbQueryMsg *pQuery = (SDbQueryMsg *)pMsg;
	
	gettimeofday_a(&pQuery->m_tPopMainQueue, 0);
	//pQuery->m_tPopAgentQueue = pQuery->m_tPopMainQueue;
	

	
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s nDbIndex: %d\n",__FUNCTION__,pQuery->nDbIndex);

	struct timeval tInterval;
	timersub(&pQuery->m_tPopMainQueue, &pMsg->m_tStart, &tInterval);
	
	if(tInterval.tv_sec + tInterval.tv_usec / 1.0e6 > 3)
	{
	  //主队列超时
	   pQuery->sResponse.nCode = ERROR_TIMEOUT_MAINQUEUE;
	   DoResponse(pQuery);	
	   return;
	}
	
    map<int, CDbAgent2 * >::iterator itr=m_dbs.find(pQuery->nDbIndex);
    if(itr!=m_dbs.end())
    {
        CDbAgent2 *pAgent2=itr->second;
		CDbAgent* pAgent = pAgent2->GetAngent(pQuery);
		if(pQuery->bQueryMaster)
		{
			pAgent->DoWriteRequest(pQuery);
		}
        else 
		{
			pAgent->DoReadRequest(pQuery);
        }
    }
    else
    {
       //没找到对应的angent  出错
       pQuery->sResponse.nCode = ERROR_NOAPP;
	   DoResponse(pQuery);
    }
}

void CDbThreadGroup::DoConnectInfo(SDbMsg * pMsg)
{
	 SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s\n",__FUNCTION__);
	 SDbConnectMsg *pConnMsg = (SDbConnectMsg*)pMsg;
	 
	 CDbConnection *pDbConn = (CDbConnection*)pConnMsg->pConn;
 
	 CDbAgent *pAgent=pDbConn->GetDbAngent();
	 pAgent->DoConnectInfo(pConnMsg);
	 delete pConnMsg;
	
}


int  CDbThreadGroup::GetDbIndex(int dwAppId)
{
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s\n",__FUNCTION__);
	map<int,int>::iterator it_appid = m_serviceindexs.find(dwAppId);
	if (it_appid==m_serviceindexs.end())
		return -1;
	else
		return it_appid->second;
}

void CDbThreadGroup::Dump()
{
	
}

void CDbThreadGroup::RecordOpr(SDbQueryMsg* pQueryMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s >>>>\n",__FUNCTION__);
	char szBuf[3072]={0};
    struct tm tm_start;
    struct timeval_a now;
    struct timeval_a interval;
    struct timeval_a interval_from_queue;
	struct timeval_a interval_from_mainqueue;

    gettimeofday_a(&now,0);

#ifdef WIN32
	 time_t tt = (time_t)(pQueryMsg->m_tStart.tv_sec);
	 localtime_r(&tt, &tm_start);
#else
	 localtime_r(&(pQueryMsg->m_tStart.tv_sec), &tm_start);
#endif

    timersub(&now, &(pQueryMsg->m_tStart), &interval);

    timersub(&now, &(pQueryMsg->sResponse.m_tStartDB), &interval_from_queue);

	timersub(&now, &(pQueryMsg->m_tPopMainQueue), &interval_from_mainqueue);

    //operatetype, start, time,index,llsndaId_appid,code
	
	string strTip="";
	if (interval_from_queue.tv_sec *1000+ interval_from_queue.tv_usec / 1.0e3 > 200)
	{
		strTip = "[LongCall]";
	}	
	
    snprintf(szBuf,3071,
        "%s\t%s %04u-%02u-%02u %02u:%02u:%02u\t%ld.%03ld\t%ld.%03ld\t%ld.%03ld\tServiceId[%d]\tMsgId[%d]\t%s\tcode[%d]\tDBCode[%d]\tErrMsg[%s]\n",
		pQueryMsg->strGuid.c_str(),
		strTip.c_str(),
		tm_start.tm_year+1900,
        tm_start.tm_mon+1,
        tm_start.tm_mday,
        tm_start.tm_hour,
        tm_start.tm_min,
        tm_start.tm_sec,
        interval.tv_sec * 1000 + interval.tv_usec / 1000, 
        interval.tv_usec % 1000,
        interval_from_mainqueue.tv_sec * 1000 + interval_from_mainqueue.tv_usec / 1000, 
        interval_from_mainqueue.tv_usec % 1000,
        interval_from_queue.tv_sec * 1000 + interval_from_queue.tv_usec / 1000, 
        interval_from_queue.tv_usec % 1000,
        pQueryMsg->sapDec.GetServiceId(),
		pQueryMsg->sapDec.GetMsgId(),
        pQueryMsg->sResponse.strSql.c_str(),
        pQueryMsg->sResponse.nCode,
        pQueryMsg->errDesc.nErrNo,
        pQueryMsg->errDesc.sErrMsg.c_str()
        );
	
	
	
	DbpService::GetInstance()->CallAsyncLog(
	ASYNC_VIRTUAL_MODULE,
	XLOG_INFO,
	interval.tv_sec * 1000 + interval.tv_usec / 1000,
	szBuf,
	pQueryMsg->sResponse.nCode,
	"ip",
	pQueryMsg->sapDec.GetServiceId(),
	pQueryMsg->sapDec.GetMsgId());

	//if(pQueryMsg->pRequest!=NULL&&pQueryMsg->nReqLen>0)
	//{
	//	free(pQueryMsg->pRequest);
	//	pQueryMsg->pRequest = NULL;
	//}
	delete pQueryMsg;
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s <<<<\n",__FUNCTION__);
}

void CDbThreadGroup::RefineErrorMsg(SDbQueryMsg* pQueryMsg)
{
	switch (pQueryMsg->sResponse.nCode)
	{
		case 0:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ok";
			break;
		case ERROR_TIMEOUT:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_TIMEOUT";
			break;
		case ERROR_QUEUEFULL:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_QUEUEFULL";
			break;
		case ERROR_BINDRESPONSE:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_BINDRESPONSE";
			break;
		case ERROR_READONLY:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_READONLY";
			break;
		case ERROR_NOAPP:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_NOAPP";
			break;
		case ERROR_MISSPARA:
			pQueryMsg->errDesc.nErrNo = ERROR_MISSPARA;
			pQueryMsg->errDesc.sErrMsg = "ERROR_MISSPARA";
			break;
		case ERROR_BIGQUEUEFULL:
			pQueryMsg->errDesc.nErrNo = ERROR_BIGQUEUEFULL;
			pQueryMsg->errDesc.sErrMsg = "ERROR_BIGQUEUEFULL";
			break;
		case ERROR_DB:
			break;
		case ERROR_TIMEOUT_NOCONNECT:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_TIMEOUT_NOCONNECT";
			break;
		case ERROR_BIND_VAR_NOT_MATCH:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_BIND_VAR_NOT_MATCH";
			break;
		case ERROR_NO_DIVIDEKEY:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_NO_DIVIDEKEY";
			break;
		case ERROR_TIMEOUT_MAINQUEUE:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_TIMEOUT_MAINQUEUE";
			break;
		default:
			pQueryMsg->errDesc.nErrNo = pQueryMsg->sResponse.nCode;
			pQueryMsg->errDesc.sErrMsg = "ERROR_UNKNOWN";
			break;
	}
}

void CDbThreadGroup::DoResponse(SDbQueryMsg* pQueryMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbThreadGroup::%s [%d]\n",__FUNCTION__,pQueryMsg->sResponse.nCode);

	gettimeofday_a(&pQueryMsg->m_tEnd, 0);

	pQueryMsg->sResponse.sapEnc.SetService(SAP_PACKET_RESPONSE,
	pQueryMsg->sapDec.GetServiceId(),
	pQueryMsg->sapDec.GetMsgId(),
	pQueryMsg->sResponse.nCode);

	pQueryMsg->sResponse.sapEnc.SetSequence(pQueryMsg->sapDec.GetSequence());

	RefineErrorMsg(pQueryMsg);
	
	if (pQueryMsg->sResponse.nCode !=0)
	{
	//	pQueryMsg->sResponse.sapEnc.SetValue(1,pQueryMsg->errDesc.nErrNo);
	//	pQueryMsg->sResponse.sapEnc.SetValue(2,pQueryMsg->errDesc.sErrMsg.c_str());
	}

	
	DbpService::GetInstance()->CallResponseService(pQueryMsg->sParams, 
		pQueryMsg->sResponse.sapEnc.GetBuffer(), 
		pQueryMsg->sResponse.sapEnc.GetLen());
	

	RecordOpr(pQueryMsg);
	

	
}







