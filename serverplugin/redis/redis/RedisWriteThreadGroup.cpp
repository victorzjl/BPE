#include "RedisWriteThreadGroup.h"
#include "RedisVirtualServiceLog.h"
#include "RedisVirtualService.h"
#include "ErrorMsg.h"
#include "WarnCondition.h"

using namespace redis;

CRedisWriteThreadGroup::~CRedisWriteThreadGroup()
{
	Stop();
}


void CRedisWriteThreadGroup::Stop()
{
	vector<CRedisThread *>::iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		if(*iter != NULL)
		{
			delete *iter;
			*iter = NULL;
		}
	}
	m_vecChildThread.clear();
}


int CRedisWriteThreadGroup::Start( unsigned int dwThreadNum, const vector<string> &vecRedisSvr, CRedisVirtualService *pRedisVirtualService)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisWriteThreadGroup::%s, ThreadNum[%d]\n", __FUNCTION__, dwThreadNum);

    if(pRedisVirtualService == NULL)
    {
        
		RVS_XLOG(XLOG_ERROR, "CRedisWriteThreadGroup::%s, The param 'pRedisVirtualService' is NULL\n", __FUNCTION__);
    	return RVS_SYSTEM_ERROR;
    }
	m_pRedisVirtualService = pRedisVirtualService;

	SQueueWarnCondition objQueueWarnCondition = pRedisVirtualService->GetWarnCondition()->GetQueueWarnCondition();
	m_bWriteQueueFullDiscardFlag = objQueueWarnCondition.bWriteQueueFullDiscard;

	int nRet = 0;
	bool bSuccess = false;

	//启动线程组
	for(unsigned int i = 0; i < dwThreadNum; i++)
	{
		CRedisThread *pRedisThread = new CRedisThread(m_queue, pRedisVirtualService);
		m_vecChildThread.push_back(pRedisThread);
		nRet = pRedisThread->Start(vecRedisSvr, 2);
		if(nRet != 0)
		{
			RVS_XLOG(XLOG_ERROR, "CRedisThreadGroup::%s, RedisThread start error\n", __FUNCTION__);
		}
		else
		{
			bSuccess = true;
		}
	}

	if(bSuccess)
	{
		return 0;
	}
	else
	{
		return nRet;
	}
}

void CRedisWriteThreadGroup::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, ERedisType eRedisType, const string &strGuid, void *pHandler, const void *pBuffer, int nLen)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisWriteThreadGroup::%s\n", __FUNCTION__);

	timeval_a tStart;
	gettimeofday_a(&tStart, 0);

	SReidsTypeMsg *pRedisMsg = NULL;
	switch(eRedisType)
	{
	case RVS_REDIS_STRING:
		pRedisMsg = new SRedisStringMsg;
		break;
	case RVS_REDIS_SET:
		pRedisMsg = new SRedisSetMsg;
		break;
	case RVS_REDIS_ZSET:
		pRedisMsg = new SRedisZsetMsg;
		break;
	case RVS_REDIS_LIST:
		pRedisMsg = new SRedisListMsg;
		break;
	case RVS_REDIS_HASH:
		pRedisMsg = new SRedisHashMsg;
		break;
	default:
		RVS_XLOG(XLOG_ERROR, "CRedisWriteThreadGroup::%s, SReidsTypeMsg[%d] not supported\n", __FUNCTION__, eRedisType);
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_UNKNOWN_REDIS_TYPE, "", 0);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_UNKNOWN_REDIS_TYPE);
		return;
	}
	pRedisMsg->dwServiceId = dwServiceId;
	pRedisMsg->dwMsgId = dwMsgId;
	pRedisMsg->dwSequenceId = dwSequenceId;

	pRedisMsg->strGuid = strGuid;
	pRedisMsg->handler = pHandler;
	pRedisMsg->tStart = tStart;

	pRedisMsg->pBuffer = malloc(nLen);
	pRedisMsg->dwLen = nLen;
	memcpy(pRedisMsg->pBuffer, pBuffer, nLen);

	if(m_queue.IsFull()  && m_bWriteQueueFullDiscardFlag)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisWriteThreadGroup::%s, write queue full\n", __FUNCTION__);
		m_pRedisVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, RVS_WRITE_QUEUE_FULL, "", 0);
		m_pRedisVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, RVS_WRITE_QUEUE_FULL);
		delete pRedisMsg;
		return;
	}
	else
	{
		m_queue.PutQ(pRedisMsg);
	}
}

void CRedisWriteThreadGroup::OnAddRedisServer(const string &strAddr)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisWriteThreadGroup::%s\n", __FUNCTION__);
	vector<CRedisThread *>::const_iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		(*iter)->OnAddRedisServer(strAddr);
	}
}

void CRedisWriteThreadGroup::GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	RVS_XLOG(XLOG_DEBUG, "CRedisWriteThreadGroup::%s\n", __FUNCTION__);
	dwAliveNum = 0;
	dwAllNum = m_vecChildThread.size();
	vector<CRedisThread *>::const_iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		if((*iter)->GetSelfCheck())
		{
			dwAliveNum ++;
		}
	}
}

