/***************************************************************

Copyright (C) Snda Corporation, 2005. All Rights Reserved

Module Name:

    

Abstract:

    

Author:

    Squall 

Revision History:

******************************************************************/

#include "AsyncLogThread.h"
#include "time.h"
#include "AsyncLogHelper.h"

const int DETAIL_LOG_INTERVAL = 30;

CAsyncLogThread* CAsyncLogThread::GetInstance()
{
	static CAsyncLogThread objAsyncLogThread;
	return &objAsyncLogThread;
}

CAsyncLogThread::CAsyncLogThread():
	m_tmDetailLog(this,DETAIL_LOG_INTERVAL,boost::bind(&CAsyncLogThread::DoDetailLog,this),CThreadTimer::TIMER_CIRCLE)	
{
	m_nRequestLoc = 0;
	m_szRequestBuf[m_nRequestLoc] = '\0';
	m_funMap[e_ASYN_LOG]=&CAsyncLogThread::DoAsynLog;

}

/*定期存放日存到磁盘*/
void CAsyncLogThread::DoDetailLog()
{
	if(m_nRequestLoc>0)
    {    	
    	XLOG_BUFFER(ASYNC_VIRTUAL_MODULE, XLOG_INFO, m_szRequestBuf);
        m_nRequestLoc=0;
        m_szRequestBuf[m_nRequestLoc]=0;
    }
}
void CAsyncLogThread::StartInThread()
{
	m_tmDetailLog.Start();
}

void CAsyncLogThread::OnAsynLog(SDbQueryMsg*pQueryMsg)
{
	LS_XLOG(XLOG_DEBUG, "CAsyncLogThread::%s\n", __FUNCTION__);
	SAsyncLogMsg *asynMsg = new SAsyncLogMsg;
	asynMsg->enType = e_ASYN_LOG;
	asynMsg->pQueryMsg = pQueryMsg;
	PutQ(asynMsg);
}

static float GetTimeInterval(const timeval_a &tStart,const timeval_a &tEnd)
{
	struct timeval_a interval;	
	timersub(&tEnd, &tStart, &interval);
	float lfInterval = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;
	return lfInterval;
}

/*处理过的具体消息细节*/
void CAsyncLogThread::DoAsynLog(SAsyncLogMsg * pMsg)
{
	LS_XLOG(XLOG_DEBUG, "CAsyncLogThread::%s\n", __FUNCTION__);
	
	SDbQueryMsg * pQueryMsg = pMsg->pQueryMsg;
	
	unsigned int dwSpent = (unsigned int)GetTimeInterval(pQueryMsg->m_tStart,pQueryMsg->m_tEnd);

	//string strMessage="";
	//string strAddr = DbpService::GetInstance()->GetServiceAddress();


	//记录异步日志信息
	struct tm tmStart;
	localtime_r((const time_t *)&(pQueryMsg->m_tStart.tv_sec),&tmStart);
	
	int nLen = snprintf(m_szRequestBuf + m_nRequestLoc,
						MAX_BUFFER_SIZE - m_nRequestLoc - 1024,
						"[%04u-%02u-%02u %02u:%02u:%02u %03u.%03lu] [ServiceId:%d] [MsgId:%d] [Code:%d] {Spend:%d]\n",
						tmStart.tm_year+1900,
						tmStart.tm_mon+1,
						tmStart.tm_mday,
						tmStart.tm_hour,
						tmStart.tm_min,
						tmStart.tm_sec,
						(unsigned int)(pQueryMsg->m_tStart.tv_usec/1000), 
						pQueryMsg->m_tStart.tv_usec%1000,
						pQueryMsg->sapDec.GetServiceId(),
						pQueryMsg->sapDec.GetMsgId(),
						pQueryMsg->sResponse.nCode,
						dwSpent
						);
						
	if(nLen > 0)
	{
		m_nRequestLoc += nLen;
		m_szRequestBuf[m_nRequestLoc]='\0';
	}
	
	if(m_nRequestLoc > (MAX_BUFFER_SIZE-2048))
	{
    	XLOG_BUFFER(ASYNC_VIRTUAL_MODULE, XLOG_INFO, m_szRequestBuf);
		m_nRequestLoc = 0;
		m_szRequestBuf[m_nRequestLoc]=0;
	}	

	
}

void CAsyncLogThread::Deal(void * pData)
{
	SAsyncLogMsg * pMsg=(SAsyncLogMsg * )pData;
	(this->*(m_funMap[pMsg->enType]))(pMsg);
	delete pMsg;
}





