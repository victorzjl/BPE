#include "CacheThreadGroup.h"
#include "AsyncVirtualServiceLog.h"
#include "CacheVirtualService.h"
#include "CacheMsg.h"
#include "ErrorMsg.h"

CCacheThreadGroup::~CCacheThreadGroup()
{
	Stop();
}

void CCacheThreadGroup::Stop()
{
	vector<CCacheThread *>::iterator iter;
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

int CCacheThreadGroup::Start( unsigned int dwThreadNum, const SERVICE_TYPE_CODE_MAP &mapCodeTypeByService, const CACHE_SET_TIME_MAP &mapCacheKeepTime, const vector<string> &vecCacheSvr, CCacheVirtualService *pCacheVirtualService,map<unsigned int, bool>&clearText, map<unsigned int, string>&mapKeyPrefix)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThreadGroup::%s, ThreadNum[%d]\n", __FUNCTION__, dwThreadNum);

	m_pCacheVirtualService = pCacheVirtualService;

	int nRet = 0;
	bool bSuccess = false;

	m_mapCodeTypeByService = mapCodeTypeByService;
	m_mapCacheKeepTime = mapCacheKeepTime;
	m_mapClearText = clearText;
	m_mapKeyPrefix = mapKeyPrefix;
	
	//启动线程组
	for(unsigned int i=0;i<dwThreadNum;i++)
	{
		CCacheThread *pCacheThread = new CCacheThread(m_queue, m_mapCodeTypeByService, mapCacheKeepTime,m_mapClearText, m_mapKeyPrefix);
		m_vecChildThread.push_back(pCacheThread);
		nRet = pCacheThread->Start(vecCacheSvr, pCacheVirtualService);
		if(nRet != 0)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheThreadGroup::%s, CacheThread start error\n", __FUNCTION__);
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

void CCacheThreadGroup::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, const void *pBuffer, int nLen)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThreadGroup::%s\n", __FUNCTION__);
	
	timeval_a tStart;
	gettimeofday_a(&tStart, 0);

	SCacheMsg *pCacheMsg = NULL;
	switch(dwMsgId)
	{
	case CVS_CACHE_GET:
		pCacheMsg = new SCacheGetMsg;
		break;
	case CVS_CACHE_SET:
		pCacheMsg = new SCacheSetMsg;
		break;
	case CVS_CACHE_DELETE:
		pCacheMsg = new SCacheDeleteMsg;
		break;
	case CVS_CACHE_GET_DELETE:
		pCacheMsg = new SCacheGetDeleteMsg;
		break;
	case CVS_CACHE_GET_CAS:
		pCacheMsg = new SCacheGetCasMsg;
		break;
	case CVS_CACHE_SET_ONLY:
		pCacheMsg = new SCacheSetOnlyMsg;
		break;
	default:
		CVS_XLOG(XLOG_ERROR, "CCacheThreadGroup::%s, MsgId[%d] not supported\n", __FUNCTION__, dwMsgId);
		m_pCacheVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, CVS_UNKNOWN_MSG, "");
		m_pCacheVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, CVS_UNKNOWN_MSG);
		return;
	}
	pCacheMsg->dwServiceId = dwServiceId;
	pCacheMsg->dwMsgId = dwMsgId;
	pCacheMsg->dwSequenceId = dwSequenceId;

	pCacheMsg->strGuid = strGuid;
	pCacheMsg->handler = pHandler;
	pCacheMsg->tStart = tStart;

	pCacheMsg->pBuffer = malloc(nLen);
	pCacheMsg->dwLen = nLen;
	memcpy(pCacheMsg->pBuffer, pBuffer, nLen);

	if(m_queue.IsFull())
	{
		CVS_XLOG(XLOG_ERROR, "CCacheThreadGroup::%s, read queue full\n", __FUNCTION__);
		m_pCacheVirtualService->Log(strGuid, dwServiceId, dwMsgId, tStart, "", "", 0, CVS_READ_QUEUE_FULL, "");
		m_pCacheVirtualService->Response(pHandler, dwServiceId, dwMsgId, dwSequenceId, CVS_READ_QUEUE_FULL);
		return;
	}
	else
	{
		m_queue.PutQ(pCacheMsg);
	}
}

void CCacheThreadGroup::GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThreadGroup::%s\n", __FUNCTION__);
	dwAliveNum = 0;
	dwAllNum = m_vecChildThread.size();
	vector<CCacheThread *>::const_iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		if((*iter)->GetSelfCheck())
		{
			dwAliveNum ++;
		}
	}
}


