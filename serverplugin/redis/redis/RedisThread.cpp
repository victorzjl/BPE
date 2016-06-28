#include "RedisThread.h"
#include "RedisVirtualService.h"
#include "ErrorMsg.h"
#include "RedisVirtualServiceLog.h"
#include "Common.h"
#include "WarnCondition.h"
#include "RedisContainer.h"
#include "RedisAgent.h"
#include "RedisReConnThread.h"
#include "RedisStringMsgOperator.h"
#include "RedisSetMsgOperator.h"
#include "RedisListMsgOperator.h"
#include "RedisHashMsgOperator.h"
#include "RedisZsetMsgOperator.h"

#include <boost/algorithm/string.hpp>
#include <XmlConfigParser.h>
#include <SapMessage.h>
#include <Cipher.h>

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace boost::algorithm;
using namespace sdo::sap;
using namespace redis;

CRedisThread::CRedisThread( CMsgQueuePrio & oPublicQueue, CRedisVirtualService *pRedisVirtualService):
	CChildMsgThread(oPublicQueue), m_isAlive(true), m_dwTimeoutNum(0), m_pRedisVirtualService(pRedisVirtualService),
	m_timerSelfCheck(this, 20, boost::bind(&CRedisThread::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE)
{
	m_redisFunc[RVS_REDIS_STRING] = &CRedisThread::DoStringTypeMsg;
	m_redisFunc[RVS_REDIS_SET] = &CRedisThread::DoSetTypeMsg;
	m_redisFunc[RVS_REDIS_ZSET] = &CRedisThread::DoZsetTypeMsg;
	m_redisFunc[RVS_REDIS_LIST] = &CRedisThread::DoListTypeMsg;
	m_redisFunc[RVS_REDIS_HASH] = &CRedisThread::DoHashTypeMsg;
	m_redisFunc[RVS_REDIS_ADD_SERVER] = &CRedisThread::DoAddRedisServer;

	m_redisTypeMsgOperator[RVS_REDIS_STRING] = new CRedisStringMsgOperator(m_pRedisVirtualService);
	m_redisTypeMsgOperator[RVS_REDIS_SET] = new CRedisSetMsgOperator(m_pRedisVirtualService);
	m_redisTypeMsgOperator[RVS_REDIS_ZSET] = new CRedisZsetMsgOperator(m_pRedisVirtualService);
	m_redisTypeMsgOperator[RVS_REDIS_LIST] = new CRedisListMsgOperator(m_pRedisVirtualService);
	m_redisTypeMsgOperator[RVS_REDIS_HASH] = new CRedisHashMsgOperator(m_pRedisVirtualService);
}


CRedisThread::~CRedisThread()
{
   map<string, CRedisAgent *>::const_iterator iterAgent;
   for(iterAgent = m_mapRedisAgent.begin(); iterAgent != m_mapRedisAgent.end(); iterAgent++)
   {
       CRedisAgent *pRedisAgent = iterAgent->second;
   	   delete pRedisAgent;
   }
   
   map<unsigned int, CRedisContainer *>::const_iterator iterContainer;
   for(iterContainer = m_mapRedisContainer.begin(); iterContainer != m_mapRedisContainer.end(); iterContainer++)
   {
       CRedisContainer *pRedisContainer = iterContainer->second;
   	   delete pRedisContainer;
   }

   for(int i = RVS_REDIS_STRING; i < RVS_REDIS_ADD_SERVER; i++)
   {
      delete m_redisTypeMsgOperator[i];  
   }

   CChildMsgThread::Stop();
}


void CRedisThread::DoSelfCheck()
{
	m_isAlive = true;
}

bool CRedisThread::GetSelfCheck() 
{
	bool flag = m_isAlive;
	m_isAlive = false;
	return flag;
}

void CRedisThread::StartInThread()
{
	m_timerSelfCheck.Start();
}

int CRedisThread::Start(const vector<string> &vecRedisSvr, short readTag)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);

	gettimeofday_a(&m_tmLastTimeoutWarn, 0);
	SQueueWarnCondition objQueueWarnCondition = m_pRedisVirtualService->GetWarnCondition()->GetQueueWarnCondition();
	m_dwTimeoutInterval = objQueueWarnCondition.dwTimeoutInterval;
	m_dwConditionTimes = objQueueWarnCondition.dwConditionTimes;
	m_dwTimeoutAlarmFrequency = objQueueWarnCondition.dwAlarmFrequency;

    map<string, bool> mapRedisAddr;
	map<string, bool> mapSlaveRedisAddr;
	map<string, bool> mapNotConnectedAddr;
	vector<string>::const_iterator iter;
	for (iter=vecRedisSvr.begin(); iter!=vecRedisSvr.end();iter++)
	{
		CXmlConfigParser dataParser;
		dataParser.ParseDetailBuffer(iter->c_str());
		
		vector<string> vecAddrs;
		vector<string> vecSlaveAddrs;
		if(readTag == 1)
		{
			vecAddrs = dataParser.GetParameters("SlaverServerAddr");
		}
		else if(readTag == 2)
		{
			vecAddrs = dataParser.GetParameters("MasterServerAddr");
		}
		else
		{
			vecAddrs = dataParser.GetParameters("MasterServerAddr");
			vecSlaveAddrs = dataParser.GetParameters("SlaverServerAddr");
		}
        
		connectRedisServer(vecAddrs, mapRedisAddr, mapNotConnectedAddr);
		connectRedisServer(vecSlaveAddrs, mapSlaveRedisAddr, mapNotConnectedAddr);
		
		int bConHash = dataParser.GetParameter("ConHash", 0);
		string strServiceIds = dataParser.GetParameter("ServiceId");
		vector<string> vecServiceIds;
		split(vecServiceIds, strServiceIds, is_any_of(","), boost::algorithm::token_compress_on);
		vector<string>::iterator iterTemp;
		for (iterTemp = vecServiceIds.begin(); iterTemp != vecServiceIds.end(); iterTemp++)
		{
			string &strServiceId = *iterTemp;
			trim(strServiceId);
			unsigned int dwServiceId = atoi(strServiceId.c_str());
			CRedisContainer *pRedisContainer = new CRedisContainer(dwServiceId, bConHash, mapRedisAddr, mapSlaveRedisAddr, this, m_pRedisVirtualService->GetReConnThread());
			m_mapRedisContainer.insert(make_pair(dwServiceId, pRedisContainer));
		}
		
		mapRedisAddr.clear();
		mapSlaveRedisAddr.clear();
	}
	mapNotConnectedAddr.clear();
	CChildMsgThread::Start();
	return 0;
}

void CRedisThread::connectRedisServer(const vector<string> &vecAddrs, map<string, bool> &mapRedisAddr, map<string, bool> &mapNotConnectedAddr)
{
	vector<string>::const_iterator iterTemp;
	for(iterTemp = vecAddrs.begin(); iterTemp != vecAddrs.end(); iterTemp++)
	{
		string strIpPort = *iterTemp;
		size_t npos = strIpPort.find("#");
		if(npos != string::npos)
		{
			strIpPort = strIpPort.substr(0, npos);
		}

        //过滤掉未连接上的地址，未连接上的在启动时不必再重新连接
		map<string, bool>::const_iterator iterNotConnectedAddr = mapNotConnectedAddr.find(strIpPort);
		if(iterNotConnectedAddr != mapNotConnectedAddr.end())
		{
		    mapRedisAddr.insert(make_pair(*iterTemp,false));
			continue;
		}

	    //过滤掉重复的已连接地址，已连接地址不用再重新连接
		map<string, CRedisAgent *>::const_iterator iterConnectedAddr = m_mapRedisAgent.find(strIpPort);
		if(iterConnectedAddr != m_mapRedisAgent.end())
		{
		    mapRedisAddr.insert(make_pair(*iterTemp,true));
			continue;
		}
				
		CRedisAgent *pRedisAgent = new CRedisAgent;
		if(pRedisAgent->Initialize(strIpPort) == 0 && pRedisAgent->Ping() == 0)
		{
			m_mapRedisAgent.insert(make_pair(strIpPort, pRedisAgent));
			mapRedisAddr.insert(make_pair(*iterTemp,true));
			RVS_XLOG(XLOG_INFO, "CRedisThread::%s, Connect RedisServer[%s] Success\n", __FUNCTION__, strIpPort.c_str());
		}
		else
		{
			RVS_XLOG(XLOG_ERROR, "CRedisThread::%s, Connect RedisServer[%s] Error\n", __FUNCTION__, strIpPort.c_str());
			mapNotConnectedAddr.insert(make_pair(strIpPort,true));
			m_pRedisVirtualService->GetReConnThread()->OnReConn(strIpPort);
			mapRedisAddr.insert(make_pair(*iterTemp,false));
			delete pRedisAgent;
		}
	}

}

void CRedisThread::OnAddRedisServer(const string &strAddr)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s, Addr[%s]\n", __FUNCTION__, strAddr.c_str());
	SRedisAddServerMsg *pRedisAddServerMsg = new SRedisAddServerMsg;
	pRedisAddServerMsg->strAddr = strAddr;
	gettimeofday_a(&(pRedisAddServerMsg->tStart),0);
	PutQ(pRedisAddServerMsg);
}

void CRedisThread::DoAddRedisServer(SReidsTypeMsg *pRedisTypeMsg )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
	SRedisAddServerMsg *pRedisAddServerMsg = (SRedisAddServerMsg *)pRedisTypeMsg;
    const string &strAddr = pRedisAddServerMsg->strAddr;

	 map<string, CRedisAgent *>::const_iterator iterAddr = m_mapRedisAgent.find(strAddr);
	 if(iterAddr != m_mapRedisAgent.end())
	 {
	    CRedisAgent *pRedisAgent = iterAddr->second;
		if(pRedisAgent->Ping() == 0)
		{
			return;
		}
		else
		{
			DelRedisServer(strAddr);
		}
	 }

	//先建立连接，再更改相应服务的Redis服务状态
	CRedisAgent *pRedisAgent = new CRedisAgent;
    if(pRedisAgent->Initialize(strAddr) == 0 && pRedisAgent->Ping() == 0)
	{
		m_mapRedisAgent.insert(make_pair(strAddr, pRedisAgent));
        RVS_XLOG(XLOG_INFO, "CRedisThread::%s, Add RedisServer[%s] Success\n", __FUNCTION__, strAddr.c_str());

		map<unsigned int, CRedisContainer *>::const_iterator iter;
        for(iter = m_mapRedisContainer.begin(); iter != m_mapRedisContainer.end(); iter++)
        {
            CRedisContainer *pRedisContainer = iter->second;
    	    pRedisContainer->AddRedisServer(strAddr);
        }
	}
    else
    {
	    RVS_XLOG(XLOG_ERROR, "CRedisThread::%s, Add RedisServer[%s] Error\n", __FUNCTION__, strAddr.c_str());
		delete pRedisAgent;
		m_pRedisVirtualService->GetReConnThread()->OnReConn(strAddr);
    }
}

void CRedisThread::Deal( void *pData )
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
	SReidsTypeMsg *pRedisMsg = (SReidsTypeMsg *)pData;

	//如果超时
	pRedisMsg->fSpent = GetTimeInterval(pRedisMsg->tStart);
	float dwTimeInterval = pRedisMsg->fSpent/1000;

	if(dwTimeInterval > m_dwTimeoutInterval)	
	{
		RVS_XLOG(XLOG_ERROR, "CRedisThread::%s, Redis thread timeout %f seconds\n", __FUNCTION__, dwTimeInterval);

		m_dwTimeoutNum++;
		
		//连续超时告警
		if(m_dwTimeoutNum >= m_dwConditionTimes)
		{
			m_dwTimeoutNum = 0;
			if(GetTimeInterval(m_tmLastTimeoutWarn)/1000 > m_dwTimeoutAlarmFrequency)
			{
				char szWarnInfo[256] = {0};
				snprintf(szWarnInfo, sizeof(szWarnInfo)-1, "Redis thread timeout %d seconds over %d times", m_dwTimeoutInterval, m_dwConditionTimes);
				m_pRedisVirtualService->Warn(szWarnInfo);
				gettimeofday_a(&m_tmLastTimeoutWarn, 0);
			}
		}

        if(pRedisMsg->m_enRedisType != RVS_REDIS_ADD_SERVER)
        {
        	delete pRedisMsg;
		    return;
        }
	}

	(this->*(m_redisFunc[pRedisMsg->m_enRedisType]))(pRedisMsg);
	delete pRedisMsg;
}

CRedisAgent * CRedisThread::GetRedisAgentByAddr(const string &strAddr)
{
    map<string, CRedisAgent *>::const_iterator iter = m_mapRedisAgent.find(strAddr);
	if(iter != m_mapRedisAgent.end())
	{
		return iter->second;
	}

	return NULL;
}

CRedisContainer * CRedisThread::GetRedisContainer(unsigned int dwServiceId)
{
	map<unsigned int, CRedisContainer *>::const_iterator iter = m_mapRedisContainer.find(dwServiceId);
	if(iter == m_mapRedisContainer.end())
	{
		return NULL;
	}

	return iter->second;
}

void CRedisThread::DelRedisServer(const string &strAddr)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s, strAddr[%s]\n", __FUNCTION__, strAddr.c_str());
    map<string, CRedisAgent *>::iterator iterAgent = m_mapRedisAgent.find(strAddr);
	if(iterAgent != m_mapRedisAgent.end())
	{
	    CRedisAgent *pRedisAgent = iterAgent->second;
	    delete pRedisAgent;
	    m_mapRedisAgent.erase(iterAgent);
	}

	map<unsigned int, CRedisContainer *>::const_iterator iterContainer;
	for(iterContainer = m_mapRedisContainer.begin(); iterContainer != m_mapRedisContainer.end(); iterContainer++)
	{
		CRedisContainer *pRedisContainer = iterContainer->second;
		pRedisContainer->DelRedisServer(strAddr);
	}
}


void CRedisThread::DoStringTypeMsg(SReidsTypeMsg * pRedisTypeMsg)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
    SRedisStringMsg * pRedisStringMsg = (SRedisStringMsg *)pRedisTypeMsg;

	CRedisContainer * pRedisContainer = GetRedisContainer(pRedisStringMsg->dwServiceId);
	
	m_redisTypeMsgOperator[RVS_REDIS_STRING]->OnProcess(pRedisStringMsg->dwServiceId,
		pRedisStringMsg->dwMsgId,
		pRedisStringMsg->dwSequenceId,
		pRedisStringMsg->strGuid,
		pRedisStringMsg->handler,
		pRedisStringMsg->pBuffer,
		pRedisStringMsg->dwLen,
		pRedisStringMsg->tStart,
	    pRedisStringMsg->fSpent,
		pRedisContainer);
}

void CRedisThread::DoSetTypeMsg(SReidsTypeMsg * pRedisTypeMsg)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
	SRedisSetMsg * pRedisSetMsg = (SRedisSetMsg *)pRedisTypeMsg;
	
	CRedisContainer * pRedisContainer = GetRedisContainer(pRedisSetMsg->dwServiceId);
	
	m_redisTypeMsgOperator[RVS_REDIS_SET]->OnProcess(pRedisSetMsg->dwServiceId,
		pRedisSetMsg->dwMsgId,
		pRedisSetMsg->dwSequenceId,
		pRedisSetMsg->strGuid,
		pRedisSetMsg->handler,
		pRedisSetMsg->pBuffer,
		pRedisSetMsg->dwLen,
		pRedisSetMsg->tStart,
	    pRedisSetMsg->fSpent,
		pRedisContainer);
}

void CRedisThread::DoZsetTypeMsg(SReidsTypeMsg *pRedisTypeMsg)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
	SRedisZsetMsg * pRedisZsetMsg = (SRedisZsetMsg *)pRedisTypeMsg;
	
	CRedisContainer * pRedisContainer = GetRedisContainer(pRedisZsetMsg->dwServiceId);
	
	m_redisTypeMsgOperator[RVS_REDIS_ZSET]->OnProcess(pRedisZsetMsg->dwServiceId,
		pRedisZsetMsg->dwMsgId,
		pRedisZsetMsg->dwSequenceId,
		pRedisZsetMsg->strGuid,
		pRedisZsetMsg->handler,
		pRedisZsetMsg->pBuffer,
		pRedisZsetMsg->dwLen,
		pRedisZsetMsg->tStart,
	    pRedisZsetMsg->fSpent,
		pRedisContainer);
}

void CRedisThread::DoListTypeMsg(SReidsTypeMsg * pRedisTypeMsg)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
	SRedisListMsg * pRedisListMsg = (SRedisListMsg *)pRedisTypeMsg;
		
	CRedisContainer * pRedisContainer = GetRedisContainer(pRedisListMsg->dwServiceId);
	m_redisTypeMsgOperator[RVS_REDIS_LIST]->OnProcess(pRedisListMsg->dwServiceId,
		pRedisListMsg->dwMsgId,
		pRedisListMsg->dwSequenceId,
		pRedisListMsg->strGuid,
		pRedisListMsg->handler,
		pRedisListMsg->pBuffer,
		pRedisListMsg->dwLen,
		pRedisListMsg->tStart,
	    pRedisListMsg->fSpent,
		pRedisContainer);

}

void CRedisThread::DoHashTypeMsg(SReidsTypeMsg * pRedisTypeMsg)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisThread::%s\n", __FUNCTION__);
	SRedisHashMsg * pRedisHashMsg = (SRedisHashMsg *)pRedisTypeMsg;
			
	CRedisContainer * pRedisContainer = GetRedisContainer(pRedisHashMsg->dwServiceId);
	m_redisTypeMsgOperator[RVS_REDIS_HASH]->OnProcess(pRedisHashMsg->dwServiceId,
		pRedisHashMsg->dwMsgId,
		pRedisHashMsg->dwSequenceId,
		pRedisHashMsg->strGuid,
		pRedisHashMsg->handler,
		pRedisHashMsg->pBuffer,
		pRedisHashMsg->dwLen,
		pRedisHashMsg->tStart,
	    pRedisHashMsg->fSpent,
		pRedisContainer);

}

