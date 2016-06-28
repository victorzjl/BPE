#include "DbAngent.h"
#include "DbThreadGroup.h"
#include "DbpService.h"
#include "DbReconnThread.h"
#include "hash.h"
#include <algorithm>

#define QUEUE_MAX_DELAYS (CDBServiceConfig::GetInstance()->GetDBDelayQueueNum())

#define DEFAULT_INDEX -111
#define ALL_INDEX 	  -222

#include "dbserviceconfig.h"
#include "util.h"


void CDbAgent::DoInit()
{

	SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, QueryType[%d], nIndex[%d] \n",__FUNCTION__ ,
		nQueryType, nIndex);  

	SConnDesc& dbconf = m_dbConf;
	const int dwSleepTime = 250*1000;
	int subIndex = 0;
	if (nQueryType== QUERY_COMMON)
	{
		{
			SConnItem& item = dbconf.masterConn;
			SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, nIndex[%d] MasterConn[%d] DbFactor[%d]\n",__FUNCTION__ ,nIndex,item.nConnNum,item.nDBFactor);  
			for( subIndex = 0; subIndex < item.nConnNum; subIndex++) 
			{		
				{
					int nConnIndex=0;
					vector<string>::iterator it = item.vecDivideConn.begin();
					for(;it!=item.vecDivideConn.end();++it)
					{
						AddDbConn(true, subIndex, nConnIndex, item.nTBFactor,*it,dbconf.dwSmart);
						usleep(dwSleepTime);
						nConnIndex++;						
					}
					
				}
				if (item.sDefaultConn.size())
				{
					AddDbConn(true, subIndex, DEFAULT_INDEX, item.nTBFactor, item.sDefaultConn,dbconf.dwSmart);	
					usleep(dwSleepTime);
				}
			}
			m_nDBFactor = item.nDBFactor;
		}
		
		{
			SConnItem& item = dbconf.slaveConn;
			 SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, nIndex[%d] SlaveConn[%d] DbFactor[%d]\n",__FUNCTION__ ,nIndex,item.nConnNum,item.nDBFactor); 
			 
			for( subIndex = 0; subIndex < item.nConnNum; subIndex++) 
			{		
				{
					int nConnIndex=0;
					vector<string>::iterator it = item.vecDivideConn.begin();
					for(;it!=item.vecDivideConn.end();++it)
					{
						AddDbConn(false, subIndex,nConnIndex, item.nTBFactor, *it, dbconf.dwSmart);
						usleep(dwSleepTime);
						nConnIndex++;						
					}
					
				}
				
				if (item.sDefaultConn.size())
				{
					AddDbConn(false,subIndex, DEFAULT_INDEX, item.nTBFactor, item.sDefaultConn, dbconf.dwSmart);	
					usleep(dwSleepTime);
				}
			}
		}
		
	}
	else
	{
		if (dbconf.masterConn.vecDivideConn.size())
		{
			AddDbConn(true,0,ALL_INDEX,0, dbconf.masterConn.vecDivideConn,dbconf.dwSmart);
			usleep(dwSleepTime);
		}
			
		if (dbconf.slaveConn.vecDivideConn.size())	
		{
			AddDbConn(false,0,ALL_INDEX,0, dbconf.slaveConn.vecDivideConn,dbconf.dwSmart);
			usleep(dwSleepTime);
		}

	}

}

CDbAgent::CDbAgent(int nIndex, SConnDesc& dbconf,int QueryType)
{
	SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, QueryType[%d], nIndex[%d] \n",__FUNCTION__ ,
		nQueryType, nIndex);  
	m_dbConf = dbconf;
	this->nIndex = nIndex;
	nQueryType = QueryType;
}

CDbAgent::~CDbAgent()
{
	vector<CDbConnection*>::iterator it = m_Conns.begin();
	for(;it!=m_Conns.end();++it)
	{
		delete *it;
	}
	m_Conns.clear();
}

void CDbAgent::DoPing()
{
	SV_XLOG(XLOG_INFO,"CDbAgent::%s [%d]>>>>>>>>>>>>>\n",__FUNCTION__,nIndex);
	m_ConnMutex.lock();
	vector<CDbConnection*>::iterator itr;
	for(itr = m_Conns.begin(); itr != m_Conns.end(); ++itr) 
	{
		CDbConnection* pDbConn = *itr;
		if(pDbConn->m_isFree && pDbConn->m_bIsConnected && pDbConn->m_eDBType==DBT_MYSQL) 
		{
			//MYSQL的需要定时重连 否则会断开
			
			bool bConnect = pDbConn->CheckDbConnect();
			SV_XLOG(XLOG_INFO,"MysqlPingnIndex[%d],subIndex[%d],nConnectIndex[%d],Master[%d],PingResult[%d]\n",
			pDbConn->m_nIndex,
			pDbConn->m_nSubIndex,
			pDbConn->m_nConnectIndex,
			(int)pDbConn->m_isMaster,
			(int)bConnect);
		}
	}
	m_ConnMutex.unlock();
	SV_XLOG(XLOG_INFO,"CDbAgent::%s [%d]<<<<<<<<<<<<\n",__FUNCTION__,nIndex);
}

void CDbAgent::DoWriteRequest(SDbQueryMsg *pQuery)
{
	SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, dividetype[%d], nIndex[%d] \n",__FUNCTION__ ,
		pQuery->divideDesc.eDivideType, nIndex);  
	m_ConnMutex.lock();

	int nConnectIndex = 0;
			
	if(pQuery->divideDesc.eDivideType==EDT_NODIVIDE)
	{
		nConnectIndex = DEFAULT_INDEX;
		vector<CDbConnection*>::iterator itr;
		for(itr = m_Conns.begin(); itr != m_Conns.end(); ++itr) 
		{
			CDbConnection* pDbConn = *itr;
			if(pDbConn->m_isFree && pDbConn->m_isMaster && pDbConn->m_bIsConnected && pDbConn->m_nConnectIndex==DEFAULT_INDEX) 
			{
				pDbConn->m_isFree = false;
				m_ConnMutex.unlock();
				DoWrite(pDbConn, pQuery);
				return ;
			}
		}
	}
	else
	{
		string strValue;
		unsigned int nIntVal;


		
		if (DBP_DT_INT == pQuery->divideDesc.eParamType)
		{
			if (-1==pQuery->sapDec.GetValue(pQuery->divideDesc.nHashParamCode,&nIntVal))
			{
				pQuery->sResponse.nCode = ERROR_NO_DIVIDEKEY;
				m_ConnMutex.unlock();
				CDbThreadGroup::GetInstance()->DoResponse(pQuery);
				return;
			}
		
			char stmp[24]={0};
			snprintf(stmp,sizeof(stmp),"%u",nIntVal);
			unsigned long lhashnum = average_hash(stmp,strlen(stmp),0);
			nConnectIndex = lhashnum % m_nDBFactor;
			SV_XLOG(XLOG_DEBUG,"CDbAgent::%s Int, value[%s] hashnum[%u] ConnectIndex[%d]\n",__FUNCTION__,stmp,lhashnum,nConnectIndex);
		}
		else if (DBP_DT_STRING == pQuery->divideDesc.eParamType)
		{
			if (-1==pQuery->sapDec.GetValue(pQuery->divideDesc.nHashParamCode,strValue))
			{
				pQuery->sResponse.nCode = ERROR_NO_DIVIDEKEY;
				m_ConnMutex.unlock();
				CDbThreadGroup::GetInstance()->DoResponse(pQuery);
				return;
			}
			unsigned long lhashnum = average_hash(strValue.c_str(), strValue.length(),0);
			SV_XLOG(XLOG_DEBUG,"***m_nDBFactor = %d\n",m_nDBFactor);
			nConnectIndex = lhashnum % m_nDBFactor;
			
			SV_XLOG(XLOG_DEBUG,"CDbAgent::%s String, value[%s] hashnum[%u] ConnectIndex[%d]\n",__FUNCTION__,strValue.c_str(),lhashnum,nConnectIndex);
		}
		else if (DBP_DT_ARRAY ==  pQuery->divideDesc.eParamType)
		{
			//nElementCode
			//todo 分库分表尚不支持数组入参
		}
			
		vector<CDbConnection*>::iterator itr;
		for(itr = m_Conns.begin(); itr != m_Conns.end(); ++itr) 
		{
			CDbConnection* pDbConn = *itr;
			if(pDbConn->m_isFree && pDbConn->m_isMaster && pDbConn->m_bIsConnected && nConnectIndex==pDbConn->m_nConnectIndex) 
			{
				pDbConn->m_isFree = false;
				m_ConnMutex.unlock();
				DoWrite(pDbConn, pQuery);
				return ;
			}
		}
	}
	PushWriteDelayMsg(nConnectIndex,pQuery);
	
	m_ConnMutex.unlock();
	return; 

}

void CDbAgent::DoSmartCommit()
{
	SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, nIndex[%d] \n",__FUNCTION__ , nIndex);
	if (nQueryType== QUERY_COMMON)
	{
		m_ConnMutex.lock();
		vector<CDbConnection*>::iterator itr;
		itr = m_Conns.begin();
		while( itr != m_Conns.end() ) 
		{
			CDbConnection* pDbConn = *itr;
			if(pDbConn->m_isFree && pDbConn->m_bIsConnected) 
			{
				pDbConn->m_isFree = false;
				m_ConnMutex.unlock();
				pDbConn->SmartCommitNow();
				m_ConnMutex.lock();
				pDbConn->m_isFree = true;
			}	
			++itr;
		}
		m_ConnMutex.unlock();
	}	
}

void CDbAgent::DoReadRequest(SDbQueryMsg *pQuery)
{
    SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, dividetype[%d], nIndex[%d] \n",__FUNCTION__ ,pQuery->divideDesc.eDivideType, nIndex);  
    m_ConnMutex.lock();
	
	if (nQueryType== QUERY_COMMON)
	{
		int nConnectIndex = 0;
		if(pQuery->divideDesc.eDivideType==EDT_NODIVIDE)
		{
			nConnectIndex = DEFAULT_INDEX;
			vector<CDbConnection*>::iterator itr;
			for(itr = m_Conns.begin(); itr != m_Conns.end(); ++itr) 
			{
				CDbConnection* pDbConn = *itr;
				if(pDbConn->m_isFree && pDbConn->m_bIsConnected && pDbConn->m_nConnectIndex==DEFAULT_INDEX) 
				{
					pDbConn->m_isFree = false;
					m_ConnMutex.unlock();
					DoQuery(pDbConn, pQuery);
					return ;
				}
			}
		}
		else
		{
		 
			string strValue;
			unsigned int nIntVal;
			
			//SV_XLOG(XLOG_DEBUG,"-----   %d %d %d\n",DBP_DT_INT,DBP_DT_STRING,pQuery->divideDesc.eParamType);
			
			if (DBP_DT_INT == pQuery->divideDesc.eParamType)
			{
			
			
				if (-1==pQuery->sapDec.GetValue(pQuery->divideDesc.nHashParamCode,&nIntVal))
				{
					pQuery->sResponse.nCode = ERROR_NO_DIVIDEKEY;
					m_ConnMutex.unlock();
					CDbThreadGroup::GetInstance()->DoResponse(pQuery);
					return;
				}
				char stmp[24]={0};
				snprintf(stmp,sizeof(stmp),"%u",nIntVal);
				unsigned long lhashnum = average_hash(stmp,strlen(stmp),0);
				nConnectIndex = lhashnum % m_nDBFactor;
				SV_XLOG(XLOG_DEBUG,"CDbAgent::%s Int, value[%s] hashnum[%u] ConnectIndex[%d]\n",__FUNCTION__,stmp,lhashnum,nConnectIndex);
			}
			else if (DBP_DT_STRING == pQuery->divideDesc.eParamType)
			{
				if (-1==pQuery->sapDec.GetValue(pQuery->divideDesc.nHashParamCode,strValue))
				{
					pQuery->sResponse.nCode = ERROR_NO_DIVIDEKEY;
					m_ConnMutex.unlock();
					CDbThreadGroup::GetInstance()->DoResponse(pQuery);
					return;
				}
				unsigned long lhashnum = average_hash(strValue.c_str(),strValue.length(),0);
				nConnectIndex = lhashnum % m_nDBFactor;
				
				SV_XLOG(XLOG_DEBUG,"CDbAgent::%s String, value[%s] hashnum[%u] ConnectIndex[%d]\n",__FUNCTION__,strValue.c_str(),lhashnum,nConnectIndex);
			}
			else
			{
				SV_XLOG(XLOG_WARNING,"CDbAgent::%s UnSupportHashType [%d]\n",__FUNCTION__,pQuery->divideDesc.eParamType);
			}
				
			vector<CDbConnection*>::iterator itr;
			for(itr = m_Conns.begin(); itr != m_Conns.end(); ++itr) 
			{
				CDbConnection* pDbConn = *itr;
				if(pDbConn->m_isFree && pDbConn->m_bIsConnected && nConnectIndex==pDbConn->m_nConnectIndex) 
				{
					pDbConn->m_isFree = false;
					m_ConnMutex.unlock();
					DoQuery(pDbConn, pQuery);
					return ;
				}
			}
		}
		
		PushQueryDelayMsg(nConnectIndex,pQuery);
	}
	else
	{

		vector<CDbConnection*>::iterator itr;
		for(itr = m_Conns.begin(); itr != m_Conns.end(); ++itr) 
		{
			CDbConnection* pDbConn = *itr;
			if(pDbConn->m_isFree) 
			{
				pDbConn->m_isFree = false;
				m_ConnMutex.unlock();
				DoQuery(pDbConn, pQuery);
				return ;
			}
		}
		PushQueryAllDelayMsg(pQuery);
	}
    m_ConnMutex.unlock();
    return;
}




void CDbAgent::DoConnectInfo(SDbConnectMsg* pConnMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbAgent::%s \n",__FUNCTION__	  );  
	CDbConnection* pConn = (CDbConnection *)pConnMsg->pConn;
	if(pConn->m_pParent)
	{
		pConn = pConn->m_pParent;
		unsigned int nNum=0;
		for(unsigned int i=0;i<pConn->m_pChilds.size();i++)
		{
			if (pConn->m_pChilds[i]->m_bIsConnected)
				nNum++;
		}
		
		if (nNum == pConn->m_pChilds.size())
			pConn->m_bIsConnected=true;
		else
			pConn->m_bIsConnected=false;
	}
	RecycleDbConn((CDbConnection *)pConnMsg->pConn);
}

int CDbAgent::AddDbConn(bool bMaster, int nSubIndex,int nConnectIndex,int nTableFactor, vector<string>& strConns, int dwSmart)
{
	SV_XLOG(XLOG_DEBUG,"CDbAgent::%s  nTableFactor{%d] vector\n",__FUNCTION__,nTableFactor);  

	CDbConnection *pDbConn = MakeConnection();
	
	pDbConn->SetSmart(dwSmart);
	pDbConn->SetDbAngent(this);
	pDbConn->m_nTbFactor=nTableFactor;
	
	pDbConn->SetPreSql(m_dbConf.strPreSql);
	if(pDbConn->DoConnectDb(nIndex,nSubIndex, bMaster, strConns) < 0) {
		SV_XLOG(XLOG_ERROR,"CDbAgent::%s, return error\n",__FUNCTION__);
		//connect failed
		delete pDbConn;
		return -1;
	}
	
	//add conn
	m_ConnMutex.lock();
	m_Conns.push_back(pDbConn);
	m_ConnMutex.unlock();
	return 0;	
}

int CDbAgent::AddDbConn(bool bMaster,int nSubIndex, int nConnectIndex, int nTableFactor, const string & strConn, int dwSmart)
{
    SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, bMaster[%d] nTableFactor{%d] [%s]\n",__FUNCTION__,bMaster,nTableFactor,strConn.c_str());   
	CDbConnection *pDbConn = MakeConnection(strConn);
	
	pDbConn->SetSmart(dwSmart);
	pDbConn->SetDbAngent(this);
	pDbConn->m_nTbFactor=nTableFactor;
	pDbConn->SetPreSql(m_dbConf.strPreSql);
	if(pDbConn->DoConnectDb(nIndex,nSubIndex,nConnectIndex, bMaster, strConn) < 0) {
		SV_XLOG(XLOG_ERROR,"CDbAgent::%s, return error\n",__FUNCTION__);
		//connect failed
		//delete pDbConn;
		m_ConnMutex.lock();
		m_Conns.push_back(pDbConn);
		CDbReconnThread::GetInstance()->OnReConnDb(pDbConn);
		m_ConnMutex.unlock();
		
		return -1;
	}

	//add conn
	m_ConnMutex.lock();
	m_Conns.push_back(pDbConn);
	m_ConnMutex.unlock();
	return 0;
	
}

void CDbAgent::RecycleDbConn(CDbConnection * pConn)
{
    SV_XLOG(XLOG_DEBUG,"CDbAgent::%s, pConn[%x]\n",__FUNCTION__,pConn);   
	if(!pConn->m_bIsConnected)
	{
		CDbReconnThread::GetInstance()->OnReConnDb(pConn);
		return ;
	}
	m_ConnMutex.lock();
    if (pConn->m_pParent)
	{
		pConn = pConn->m_pParent;
		SDbQueryMsg* pQuery=(SDbQueryMsg*)PopQueryAllDelayMsg();
		if(pQuery!=NULL)
		{
			m_ConnMutex.unlock();
			DoQuery(pConn,pQuery);
			return;
		}
		
	}
	else 
	{
		int nConnectIndex =  pConn->m_nConnectIndex;
		if(pConn->m_isMaster)
		{
			SDbQueryMsg* pQuery=(SDbQueryMsg*)PopWriteDelayMsg(nConnectIndex);
			if(pQuery!=NULL)
			{
				m_ConnMutex.unlock();
				DoWrite(pConn,pQuery);
				return;
			}
		}
		
		SDbQueryMsg* pQuery=(SDbQueryMsg*)PopQueryDelayMsg(nConnectIndex);
		if(pQuery!=NULL)
		{
			m_ConnMutex.unlock();
			DoQuery(pConn,pQuery);
			return;
		}
		
	}
	pConn->m_isFree=true;
    m_ConnMutex.unlock();
}


void  CDbAgent::PushQueryAllDelayMsg(SDbMsg *pMsg)
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s\n",__FUNCTION__);  
	m_queryAllDelay.push_back(pMsg);
	if(m_queryAllDelay.size() > QUEUE_MAX_DELAYS)
	{
		if((pMsg = PopQueryAllDelayMsg())!=NULL)
        {      
			SV_XLOG(XLOG_WARNING,"CDbAgent::%s QueryAllDelay Queue Full\n",__FUNCTION__);
			SDbQueryMsg* pQueryMsg = (SDbQueryMsg*)pMsg;
			pQueryMsg->sResponse.nCode = ERROR_QUEUEFULL;
			
			char warnStr[200]={0};
			snprintf(warnStr,
				sizeof(warnStr),
				"DBbroker service:%d [%s] ERROR_QUEUEFULL",
				pQueryMsg->sapDec.GetServiceId(),CDbThreadGroup::GetInstance()->GetIp().c_str());
				
			DbpService::GetInstance()->CallExceptionWarn(warnStr);
			
			DB_XLOG(XLOG_WARNING,"%s\n",warnStr); 
			
			CDbThreadGroup::GetInstance()->DoResponse(pQueryMsg);
        }
	}
}
SDbMsg* CDbAgent::PopQueryAllDelayMsg()
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s\n",__FUNCTION__);  
	while(!m_queryAllDelay.empty()) 
	{
		SDbMsg *pMsg = m_queryAllDelay.front();
		m_queryAllDelay.pop_front();
	    if(DetectMsgTimeout(pMsg)==false)
	    {
			SDbQueryMsg* pQueryMsg = (SDbQueryMsg*)pMsg;
			pQueryMsg->sResponse.nCode = ERROR_TIMEOUT;
			CDbThreadGroup::GetInstance()->DoResponse(pQueryMsg);
			
	    }
		else
	    {
		   DB_XLOG(XLOG_DEBUG,"CDbAgent::%s GETDELAYMSG\n",__FUNCTION__);  
	       return pMsg;
	    }     
	}
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s NULLMSG\n",__FUNCTION__);  
	return NULL;
}

int CDbAgent::GetQueueCount(int &nQuery, int&nWrite)
{
	nQuery = nWrite = 0;
	m_ConnMutex.lock();
	map<int,deque<SDbMsg *> >::iterator it_query = m_queryDelays.begin();
	for(;it_query!=m_queryDelays.end();it_query++)
	{
		nQuery+=it_query->second.size();
	}
	map<int,deque<SDbMsg *> >::iterator it_write = m_writeDelays.begin();
	for(;it_write!=m_writeDelays.end();it_write++)
	{
		nWrite+=it_write->second.size();
	}
	m_ConnMutex.unlock();

	return 0;
}


void CDbAgent::PushQueryDelayMsg(int nConnectIndex,SDbMsg * pMsg)
{
    DB_XLOG(XLOG_DEBUG,"CDbAgent::%s\n",__FUNCTION__); 
	deque<SDbMsg *> & m_queryDelay = m_queryDelays[nConnectIndex];
	m_queryDelay.push_back(pMsg);
	if(m_queryDelay.size() > QUEUE_MAX_DELAYS)
	{
		if((pMsg = PopQueryDelayMsg(nConnectIndex))!=NULL)
        {      
			SV_XLOG(XLOG_WARNING,"CDbAgent::%s QueryDelay Queue Full\n",__FUNCTION__);
			SDbQueryMsg* pQueryMsg = (SDbQueryMsg*)pMsg;
			pQueryMsg->sResponse.nCode = ERROR_QUEUEFULL;
			
			char warnStr[200]={0};
			snprintf(warnStr,
				sizeof(warnStr),
				"DBBroker service:%d [%s] ERROR_QUEUEFULL",
				pQueryMsg->sapDec.GetServiceId(),CDbThreadGroup::GetInstance()->GetIp().c_str());
				
			DbpService::GetInstance()->CallExceptionWarn(warnStr);
			
			DB_XLOG(XLOG_WARNING,"%s\n",warnStr); 
			
			CDbThreadGroup::GetInstance()->DoResponse(pQueryMsg);
        }
	}
}

SDbMsg* CDbAgent::PopQueryDelayMsg(int nConnectIndex)
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s\n",__FUNCTION__);  
	deque<SDbMsg *> & m_queryDelay = m_queryDelays[nConnectIndex];
	while(!m_queryDelay.empty()) 
	{
		SDbMsg *pMsg = m_queryDelay.front();
		m_queryDelay.pop_front();
	    if(DetectMsgTimeout(pMsg)==false)
	    {
			SDbQueryMsg* pQueryMsg = (SDbQueryMsg*)pMsg;
			pQueryMsg->sResponse.nCode = ERROR_TIMEOUT;
			CDbThreadGroup::GetInstance()->DoResponse(pQueryMsg);
			
	    }
		else
	    {
		   DB_XLOG(XLOG_DEBUG,"CDbAgent::%s GETDELAYMSG\n",__FUNCTION__);  
	       return pMsg;
	    }     
	}
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s NULLMSG\n",__FUNCTION__);  
	return NULL;
}
void  CDbAgent::PushWriteDelayMsg(int nConnectIndex,SDbMsg *pMsg)
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s\n",__FUNCTION__);  
	deque<SDbMsg *> & m_writeDelay = m_writeDelays[nConnectIndex];
	 
	m_writeDelay.push_back(pMsg);
	if(m_writeDelay.size() > QUEUE_MAX_DELAYS)
	{
		if((pMsg = PopWriteDelayMsg(nConnectIndex))!=NULL)
        {      
			SV_XLOG(XLOG_WARNING,"CDbAgent::%s WriteDelay Queue Full\n",__FUNCTION__);
			SDbQueryMsg* pQueryMsg = (SDbQueryMsg*)pMsg;
			pQueryMsg->sResponse.nCode = ERROR_QUEUEFULL;
			
			char warnStr[200]={0};
			snprintf(warnStr,
				sizeof(warnStr),
				"DBbRoker service:%d [%s] ERROR_QUEUEFULL",
				pQueryMsg->sapDec.GetServiceId(),CDbThreadGroup::GetInstance()->GetIp().c_str());
				
			DbpService::GetInstance()->CallExceptionWarn(warnStr);
			
			DB_XLOG(XLOG_WARNING,"%s\n",warnStr); 
			
			CDbThreadGroup::GetInstance()->DoResponse(pQueryMsg);
        }
	}
}
SDbMsg* CDbAgent::PopWriteDelayMsg(int nConnectIndex)
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s\n",__FUNCTION__);  
	deque<SDbMsg *> & m_writeDelay = m_writeDelays[nConnectIndex];
	while(!m_writeDelay.empty()) 
	{
		SDbMsg *pMsg = m_writeDelay.front();
		m_writeDelay.pop_front();
	    if(DetectMsgTimeout(pMsg)==false)
	    {
			SDbQueryMsg* pQueryMsg = (SDbQueryMsg*)pMsg;
			pQueryMsg->sResponse.nCode = ERROR_TIMEOUT;
			
			
			char warnStr[200]={0};
			snprintf(warnStr,
				sizeof(warnStr),
				"service:%d ERROR_TIMEOUT",
				pQueryMsg->sapDec.GetServiceId());
				
			DbpService::GetInstance()->CallExceptionWarn(warnStr);
			
			DB_XLOG(XLOG_WARNING,"%s\n",warnStr); 
			
			
			CDbThreadGroup::GetInstance()->DoResponse(pQueryMsg);
			
			
	    }
		else
	    {
		   DB_XLOG(XLOG_DEBUG,"CDbAgent::%s GETDELAYMSG\n",__FUNCTION__);  
	       return pMsg;
	    }     
	}
	DB_XLOG(XLOG_DEBUG,"CDbAgent::%s NULLMSG\n",__FUNCTION__);  
	return NULL;
}


bool CDbAgent::DetectMsgTimeout(SDbMsg *pMsg)
{
    struct timeval tNow;
	struct timeval tInterval;
	gettimeofday(&tNow, 0);
	
//	SDbQueryMsg* pQuery = (SDbQueryMsg*)pMsg;
//	gettimeofday_a(&pQuery->m_tPopAgentQueue, 0);
	
	timersub(&tNow, &pMsg->m_tStart, &tInterval);
	
	if(tInterval.tv_sec + tInterval.tv_usec / 1.0e6 > 5)
	{
        return false;
	}
    else
    {
       
        return true;
    }
}
void CDbAgent::DoWrite(CDbConnection *pDbConn,SDbQueryMsg *pQuery)
{
    DB_XLOG(XLOG_DEBUG,"CDbAgent::%s, dbindex[%d], connindex[%d], ismaster[%d],pDbConn[%x]\n",__FUNCTION__,
		pDbConn->m_nIndex, pDbConn->m_nConnectIndex, pDbConn->m_isMaster, pDbConn); 
	pQuery->sVoid = (void*)pDbConn;
	pDbConn->DoWrite(pQuery);
	CDbThreadGroup::GetInstance()->DoResponse(pQuery);
	RecycleDbConn(pDbConn);
}

void CDbAgent::DoQuery(CDbConnection *pDbConn,SDbQueryMsg *pQuery)
{
    DB_XLOG(XLOG_DEBUG,"CDbAgent::%s, dbindex[%d], connindex[%d], ismaster[%d], pDbConn[%x]\n",__FUNCTION__,
		pDbConn->m_nIndex, pDbConn->m_nConnectIndex, pDbConn->m_isMaster,pDbConn); 

	pQuery->sVoid = (void*)pDbConn;
	pDbConn->DoQuery(pQuery);
	
	CDbThreadGroup::GetInstance()->DoResponse(pQuery);
 
	RecycleDbConn(pDbConn);
}

void CDbAgent::Dump()
{
	
	for(size_t nConnIndex = 0; nConnIndex < m_Conns.size(); nConnIndex++)
	{
		m_Conns[nConnIndex]->Dump();
	}
}


//----------------------------------------------------------------

CDbAgent2::CDbAgent2(int nIndex, SConnDesc& dbconf)
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent2::%s\n",__FUNCTION__); 
	m_pCommon = NULL;
	m_pQueryALL = NULL;
	m_dbConf = dbconf;
	m_nIndex = nIndex;
	m_pCommon = new CDbAgent(nIndex,dbconf,QUERY_COMMON);
	DB_XLOG(XLOG_DEBUG,"CDbAgent2::%s NormalAngent\n",__FUNCTION__); 
	if (m_dbConf.masterConn.bIsDivide|| m_dbConf.slaveConn.bIsDivide)
	{
		m_pQueryALL = new CDbAgent(nIndex,dbconf,QUERY_ALL);
		DB_XLOG(XLOG_DEBUG,"CDbAgent2::%s ALLAngent\n",__FUNCTION__); 
	}
	
}

void CDbAgent2::DoPing()
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent2::%s\n",__FUNCTION__);
	if (m_pCommon)
	{
		m_pCommon->DoPing();
	}
	if (m_pQueryALL)
	{
		m_pQueryALL->DoPing();
	}
}

void CDbAgent2::DoInit()
{
	DB_XLOG(XLOG_DEBUG,"CDbAgent2::%s nIndex[%d]\n",__FUNCTION__,m_nIndex); 

	if (m_pCommon)
		m_pCommon->DoInit();
	if (m_pQueryALL)
		m_pQueryALL->DoInit();
}

CDbAgent2::~CDbAgent2()
{
	delete m_pCommon;
	delete m_pQueryALL;
	m_pCommon = NULL;
	m_pQueryALL = NULL;
}

void CDbAgent2::HandleSmartCommit()
{
	if (m_pCommon!=NULL)
	{
		m_pCommon->DoSmartCommit();
	}
}

CDbAgent* CDbAgent2::GetAngent(SDbQueryMsg *pQuery)
{

//	int dwServiceId = pQuery->sapDec.GetServiceId();
//	int dwMsgId = pQuery->sapDec.GetMsgId();

//	int nRet = CDBServiceConfig::GetInstance()->GetDivdeDesc(dwServiceId,dwMsgId,pQuery->divideDesc);

    int nRet = 0;
	pQuery->divideDesc = pQuery->pMsgAttri->divideDesc;
	
	DB_XLOG(XLOG_DEBUG,"CDbAgent2::%s DivideType:%d ParamType:%d hashcode:%d\n",__FUNCTION__,pQuery->divideDesc.eDivideType,
		pQuery->divideDesc.eParamType, pQuery->divideDesc.nHashParamCode); 
	
	if (nRet == 0 )
	{
		if (pQuery->divideDesc.eDivideType == EDT_FULLSCAN)
			return m_pQueryALL;
		else
			return 	m_pCommon;
	}
	else
	{
		DB_XLOG(XLOG_ERROR,"CDbAgent2::%s\n",__FUNCTION__); 
	}
	return 	m_pCommon;
}


