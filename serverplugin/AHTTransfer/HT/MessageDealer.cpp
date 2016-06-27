#include <MsgTimerThread.h>
#include <vector>
#include "MessageDealer.h"
#include "HTDealerServiceLog.h"
#include <stdlib.h> 
#include <time.h>
#include <SapMessage.h>
#include "WarnCondition.h"
#include "HTErrorMsg.h"
#include "HTDealer.h"
#include "HttpClientFactory.h"
#include <boost/algorithm/string/replace.hpp>

using std::stringstream;
using namespace sdo::sap;
using namespace sdo::common;



CMessageDealer::CMessageDealer():
	m_nBusinessClientId(0),
	m_dwTimeoutNum(0),
	m_isAlive(true), 
	m_timerSelfCheck(this,20, boost::bind(&CMessageDealer::DoSelfCheck,this), CThreadTimer::TIMER_CIRCLE)
{
	HT_XLOG(XLOG_DEBUG, "CMessageDealer::%s\n", __FUNCTION__);
	m_HTFunc[E_SEND] = &CMessageDealer::DoSend;
	m_HTFunc[E_REV] = &CMessageDealer::DoReceive;
	
	m_aht = new CAHTTransfer;
	
}

CMessageDealer::~CMessageDealer()
{
	HT_XLOG(XLOG_DEBUG, "CMessageDealer::%s\n", __FUNCTION__);
	if(!m_mapReqs.empty())
		m_mapReqs.clear();
	if( m_aht != NULL)
		delete m_aht;
	
	CMsgTimerThread::Stop();
}

void CMessageDealer::StartInThread()
{
	HT_XLOG(XLOG_DEBUG, "CMessageDealer::%s\n", __FUNCTION__);
	m_timerSelfCheck.Start();
}

void CMessageDealer::Deal(void *pData)
{
	HT_XLOG(XLOG_DEBUG, "CMessageDealer::%s\n", __FUNCTION__);
	
	SHTTypeMsg* pTypeMsg = (SHTTypeMsg*)pData;

	/*
	//如果超时
	float fSpent = GetTimeInterval(pTypeMsg->tStart);
	float dwTimeInterval = fSpent/1000;
	
	if(dwTimeInterval > m_dwTimeoutInterval)	
	{
		HT_XLOG(XLOG_ERROR, "CMessageDealer::%s, HTMsgDealer thread timeout %f seconds\n", __FUNCTION__, dwTimeInterval);

		m_dwTimeoutNum++;
		
		//连续超时告警
		if(m_dwTimeoutNum >= m_dwConditionTimes)
		{
			m_dwTimeoutNum = 0;
			if(GetTimeInterval(m_tmLastTimeoutWarn)/1000 > m_dwTimeoutAlarmFrequency)
			{
				char szWarnInfo[256] = {0};
				snprintf(szWarnInfo, sizeof(szWarnInfo)-1, "HTMsgDealer thread timeout %d seconds over %d times", m_dwTimeoutInterval, m_dwConditionTimes);
				pTypeMsg->m_htD->Warn(szWarnInfo);
				gettimeofday_a(&m_tmLastTimeoutWarn, 0);
			}
		}		
	}	
	*/
	(this->*(m_HTFunc[pTypeMsg->m_enHTType]))(pTypeMsg);
	delete pTypeMsg;
}

int CMessageDealer::Start()
{
	HT_XLOG(XLOG_DEBUG, "CMessageDealer::%s\n", __FUNCTION__);
	gettimeofday_a(&m_tmLastTimeoutWarn, 0);
	/**
	SQueueWarnCondition objQueueWarnCondition = m_pRedisVirtualService->GetWarnCondition()->GetQueueWarnCondition();
	m_dwTimeoutInterval = objQueueWarnCondition.dwTimeoutInterval;
	m_dwConditionTimes = objQueueWarnCondition.dwConditionTimes;
	m_dwTimeoutAlarmFrequency = objQueueWarnCondition.dwAlarmFrequency;
	**/
	CMsgTimerThread::Start();
	
	return 0;
}

bool CMessageDealer::GetSelfCheck()
{
	bool flag = m_isAlive;
	m_isAlive = false;
	return flag;
}

void CMessageDealer::OnSend(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, timeval_a tStart, void *pHandler, const void *pBuffer, int nLen, CHTDealer *htd)
{
	HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s\n", __FUNCTION__);
	
	SHTSendMsg *pMsg = new SHTSendMsg;
	pMsg->m_enHTType = E_SEND;
	pMsg->dwServiceId = dwServiceId;
	pMsg->dwMsgId = dwMsgId;
	pMsg->dwSequenceId = dwSequenceId;
	pMsg->strGuid = strGuid;
	pMsg->m_htD = htd;
	
	pMsg->handler = pHandler;
	pMsg->pAvenueBody = (void *)malloc(nLen);
	
	memcpy(pMsg->pAvenueBody, pBuffer, nLen);
	pMsg->nAvenueBodyLen = nLen;
	pMsg->tStart = tStart;
	
	if(IsQueueFull())
	{
		HT_XLOG(XLOG_ERROR, "CMessageDealer::%s, msg queue full\n", __FUNCTION__);		
		htd->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, HT_READ_QUEUE_FULL,"",0);
		struct timeval_a tEnd;
		gettimeofday_a(&tEnd,0);
		htd->Log(tStart,tEnd,strGuid,"", dwServiceId, dwMsgId,  "", 0, "", HT_READ_QUEUE_FULL, "");
		delete pMsg;
		return;		
	}
	else
		PutQ(pMsg);
	
}

void CMessageDealer::OnReceive(int nCohClientId, const string &strResponse, int nProtocol, const string  &ip, const string & port)
{
	HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s\n", __FUNCTION__);
	
	SHTRcvMsg *pMsg = new SHTRcvMsg;

	pMsg->m_nCohClientId = nCohClientId;
	pMsg->m_strHttpRetPacket = strResponse;
	pMsg->m_dwProtocol = nProtocol;
	
	//gettimeofday_a(&pMsg->tStart,0);
	gettimeofday_a(&pMsg->httpEnd,0);
	
	SetIPPORT(ip, port, nCohClientId);
	if(IsQueueFull())
	{		
		HT_XLOG(XLOG_ERROR, "CMessageDealer::%s, msg queue full\n", __FUNCTION__);
		
		map<int,SSessionInfo>::iterator iter = m_mapReqs.find(nCohClientId);
		if( iter == m_mapReqs.end())
		{
			HT_XLOG(XLOG_ERROR, "CMessageDealer::%s, can not find nCohClientId \n", __FUNCTION__);
			delete pMsg;
			return;			
		}
		
		CHTDealer * htd = (iter->second).m_htD;		
		htd->Log((iter->second).m_tStartInPlatform,pMsg->httpEnd,(iter->second).strGuid,"", (iter->second).dwServiceId, (iter->second).dwMsgId,  "", 0, "", HT_MSG_QUEUE_FULL, "");
		htd->Response((iter->second).m_handler, (iter->second).dwServiceId, (iter->second).dwMsgId, (iter->second).dwSequenceId, HT_MSG_QUEUE_FULL,"",0);
		delete pMsg;
		return;			
	}
	else
		PutQ(pMsg);
	

	
}

void CMessageDealer::DoSend(SHTTypeMsg *pSHTTypeMsg)
{
	HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s\n", __FUNCTION__); 
	
	SHTSendMsg *pMsg = (SHTSendMsg *)pSHTTypeMsg;
	//void * handler = pMsg->handler;
	
	SSessionInfo info;
	
	info.m_handler = pMsg->handler;
	info.m_htD = pMsg->m_htD;
	info.strGuid = pMsg->strGuid;
	info.dwServiceId = pMsg->dwServiceId;
	info.dwMsgId = pMsg->dwMsgId;
	info.dwSequenceId = pMsg->dwSequenceId;
	
	info.m_tStartInPlatform = pMsg->tStart;
	
	char szHttpReqPacket[PACKET_LEN] = {0};
	int nHttpReqPacketLen = sizeof(szHttpReqPacket) - 1;
	string serverUri;//服务器uri
	string serverAddr;//配置服务器地址
	
	int nRet = 0;
	nRet = m_aht->AvenueToHttp(pMsg->dwServiceId,
								pMsg->dwMsgId, 
								pMsg->dwSequenceId, 
								(char *)pMsg->pAvenueBody,
								pMsg->nAvenueBodyLen,
								szHttpReqPacket,
								nHttpReqPacketLen,
								serverUri,
								serverAddr
								);
	
	/**
	* 判断http包体是否正常构建
	*/
								
	if( nRet == 0)
	{		
		//构造http包体正常
		HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s--construct httpBody success!--uri:[%s]--httpbody:\n%s\n", __FUNCTION__, serverUri.c_str(), szHttpReqPacket);
		
		m_nBusinessClientId++;
		string sendStr = szHttpReqPacket;
		
		//创建coh对象并保存请求http包体和uri
		info.m_pCohClient = CHttpClientFactory::Create(serverUri);
		info.m_pCohClient->Init(m_nBusinessClientId, serverUri,this);
		info.m_request = sendStr;
        info.m_uri = serverUri;
		info.severAddr = serverAddr;		
		gettimeofday_a(&info.m_tStartInService, 0);  //记录服务开始时间
		m_mapReqs.insert(make_pair(m_nBusinessClientId, info));
		//http请求发送
		info.m_pCohClient->SendRequest(sendStr);			
	}
	else
	{
		HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s--construct httpBody failed retCode[%d]!", __FUNCTION__, nRet);
		(info.m_htD)->Response(info.m_handler, info.dwServiceId, info.dwMsgId, info.dwSequenceId, nRet,"",0);
		gettimeofday_a(&info.m_tEndInPlatform, 0);
		(info.m_htD)->Log(info.m_tStartInPlatform, info.m_tEndInPlatform,info.strGuid,info.severAddr, info.dwServiceId,
						 info.dwMsgId,info.ipPort,0,info.m_request, nRet,info.m_response);
		
	}
	
	
}

void CMessageDealer::DoReceive(SHTTypeMsg *pSHTTypeMsg)
{
	HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s\n", __FUNCTION__); 
	SHTRcvMsg *pMsg = (SHTRcvMsg *)pSHTTypeMsg;
	
	int m_cohId = pMsg->m_nCohClientId;
	map<int, SSessionInfo>::iterator iter = m_mapReqs.find(m_cohId);
	
	if( iter == m_mapReqs.end())
	{
		HT_XLOG(XLOG_ERROR,"CMessageDealer::%s --MapError::can not find proper cohId[%d]--\n", __FUNCTION__,m_cohId);
		return;				
	}
	SSessionInfo &info = iter->second;
	info.m_response = pMsg->m_strHttpRetPacket;
	info.m_tEndInService = pMsg->httpEnd;
	
	char szAvenueBody[PACKET_LEN] = {0};
    int nAvenueLen = sizeof(szAvenueBody)-1;
	int retCode;
	
	//释放coh
	if(info.m_pCohClient != NULL)
	{
		delete info.m_pCohClient;			
	}
	int nRet = 0;
	nRet = m_aht->HttpToAvenue(info.dwServiceId,
							   info.dwMsgId, 
					     	   (pMsg->m_strHttpRetPacket).c_str(),
							   (pMsg->m_strHttpRetPacket).size(),
							   szAvenueBody,
							   nAvenueLen,
							   retCode);
								
	/**
	* 判断avenue包体是否正常构建
	*/
								
	if( nRet == 0)
	{
		//avenue包体正常构建
		(info.m_htD)->Response(info.m_handler,info.dwServiceId,info.dwMsgId,info.dwSequenceId,retCode,szAvenueBody,nAvenueLen);
		gettimeofday_a(&info.m_tEndInPlatform, 0);
		
		//计算耗时
		struct timeval_a interval;		
		timersub(&(info.m_tEndInService),&(info.m_tStartInService),&interval);
		info.m_fSpenttimeInService = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;		
		boost::replace_all(info.m_request,"\r\n"," ");
		boost::replace_all(info.m_response,"\r\n"," ");
		
		(info.m_htD)->Log(info.m_tStartInPlatform,info.m_tEndInPlatform, info.strGuid,info.severAddr, info.dwServiceId,
						 info.dwMsgId,info.ipPort,info.m_fSpenttimeInService,info.m_request, retCode,info.m_response);
		
	}
	else
	{
			
		HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s--construct avenueBody failed retCode[%d]!", __FUNCTION__, retCode);
		(info.m_htD)->Response(info.m_handler, info.dwServiceId, info.dwMsgId, info.dwSequenceId, retCode,"",0);
		gettimeofday_a(&info.m_tEndInPlatform, 0);
		
		//计算耗时
		struct timeval_a interval;
		timersub(&(info.m_tEndInService),&(info.m_tStartInService),&interval);
		info.m_fSpenttimeInService = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;
		boost::replace_all(info.m_request,"\r\n"," ");
		boost::replace_all(info.m_response,"\r\n"," ");
		
		(info.m_htD)->Log(info.m_tStartInPlatform,info.m_tEndInPlatform, info.strGuid,info.severAddr, info.dwServiceId,
						 info.dwMsgId,info.ipPort,info.m_fSpenttimeInService,info.m_request, retCode,info.m_response);
		
	}	
	
	//清除请求记录
	m_mapReqs.erase(iter);
}

void CMessageDealer::SetIPPORT( const string  ip, const string port, int m_BId)
{
	HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s\n", __FUNCTION__);
	map<int, SSessionInfo>::iterator iter = m_mapReqs.find(m_BId);	
	if( iter != m_mapReqs.end())
	{
		SSessionInfo &info = iter->second;	
		info.ipPort = ip + ":" + port;
	}
	else
	{
		HT_XLOG(XLOG_ERROR,"CMessageDealer::%s --MapError::can not find proper cohId[%d]--\n", __FUNCTION__,m_BId);
		return;
	}
	
}
void CMessageDealer::DoSelfCheck()
{
	if(m_isAlive)
		return;
	m_isAlive = true;
	HT_XLOG(XLOG_DEBUG,"CMessageDealer::%s\n", __FUNCTION__);	
}
