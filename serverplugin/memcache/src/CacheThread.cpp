#include <boost/algorithm/string.hpp>
#include <XmlConfigParser.h>
#include <SapMessage.h>
#include <Cipher.h>

#include "CacheVirtualService.h"
#include "CacheThread.h"
#include "ErrorMsg.h"
#include "AsyncVirtualServiceLog.h"
#include "CvsArchiveAdapter.h"
#include "ServiceConfig.h"
#include "Common.h"
#include "CommonMacro.h"
#include "WarnCondition.h"

#ifndef _WIN32
#include <netinet/in.h>
#endif

using namespace boost::algorithm;
using namespace sdo::sap;

CCacheThread::CCacheThread( CMsgQueuePrio & oPublicQueue, const SERVICE_TYPE_CODE_MAP &mapCodeTypeByService, const CACHE_SET_TIME_MAP &mapCacheKeepTime,const map<unsigned int,bool>& mapcleartext,const map<unsigned int,string>& mapKeyPrefix):
	CChildMsgThread(oPublicQueue), m_mapCodeTypeByService(mapCodeTypeByService),m_mapCacheKeepTime(mapCacheKeepTime),m_mapClearText(mapcleartext), m_mapKeyPrefix(mapKeyPrefix), m_isAlive(true),m_dwTimeoutNum(0),m_pCacheVirtualService(NULL),
	m_timerSelfCheck(this, 20, boost::bind(&CCacheThread::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE)
{
	m_cacheFunc[CVS_CACHE_GET] = &CCacheThread::DoGet;
	m_cacheFunc[CVS_CACHE_SET] = &CCacheThread::DoSet;
	m_cacheFunc[CVS_CACHE_DELETE] = &CCacheThread::DoDelete;
	m_cacheFunc[CVS_CACHE_GET_DELETE] = &CCacheThread::DoGetAndDelete;
	m_cacheFunc[CVS_CACHE_GET_CAS] = &CCacheThread::DoGetAndCas;
	m_cacheFunc[CVS_CACHE_SET_ONLY] = &CCacheThread::DoSetOnly;
	m_cacheFunc[CVS_CACHE_ADD_SERVER] = &CCacheThread::DoAddCacheServer;
}

bool CCacheThread::isClearText(unsigned int dwServiceId)
{
	map<unsigned int, bool>::iterator it = m_mapClearText.find(dwServiceId);
	if (it==m_mapClearText.end()) return false;
	return it->second;
}

string CCacheThread::getKeyPrefix(unsigned int dwServiceId)
{
	string ret = "";
	map<unsigned int, string>::iterator it = m_mapKeyPrefix.find(dwServiceId);
	if (it==m_mapKeyPrefix.end()) return ret;
	return it->second;
}

CCacheThread::~CCacheThread()
{
}


void CCacheThread::DoSelfCheck()
{
	m_isAlive = true;
}

bool CCacheThread::GetSelfCheck() 
{
	bool flag = m_isAlive;
	m_isAlive = false;
	return flag;
}

void CCacheThread::StartInThread()
{
	m_timerSelfCheck.Start();
}

int CCacheThread::Start(const vector<string> &vecCacheSvr, CCacheVirtualService *pCacheVirtualService)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	m_pCacheVirtualService = pCacheVirtualService;

	gettimeofday_a(&m_tmLastTimeoutWarn, 0);
	SQueueWarnCondition objQueueWarnCondition = pCacheVirtualService->GetWarnCondition()->GetQueueWarnCondition();
	m_dwTimeoutInterval = objQueueWarnCondition.dwTimeoutInterval;
	m_dwConditionTimes = objQueueWarnCondition.dwConditionTimes;
	m_dwTimeoutAlarmFrequency = objQueueWarnCondition.dwAlarmFrequency;

	vector<string>::const_iterator iter;
	for (iter=vecCacheSvr.begin(); iter!=vecCacheSvr.end();iter++)
	{
		CXmlConfigParser dataParser;
		dataParser.ParseDetailBuffer(iter->c_str());
		
		vector<string>::iterator iterTemp;
		
		vector<string> vecAddrs = dataParser.GetParameters("ServerAddr");
		int bConHash = dataParser.GetParameter("ConHash", 0);
		string strServiceIds = dataParser.GetParameter("ServiceId");
		vector<string> vecServiceIds;
		split(vecServiceIds, strServiceIds, is_any_of(","), boost::algorithm::token_compress_on);
		vector<unsigned int> dwVecServiceIds;
		
		for (iterTemp = vecServiceIds.begin(); iterTemp != vecServiceIds.end(); iterTemp++)
		{
			string &strServiceId = *iterTemp;
			trim(strServiceId);
			unsigned int dwServiceId = atoi(strServiceId.c_str());
			dwVecServiceIds.push_back(dwServiceId);
		}
		
		CCacheContainer *pCacheContainer = new CCacheContainer(dwVecServiceIds, bConHash, vecAddrs,this, m_pCacheVirtualService->GetReConnThread());
		
		vector<unsigned int>::iterator iterTempId = dwVecServiceIds.begin();
		for(;iterTempId!=dwVecServiceIds.end();iterTempId++)
		{
			m_mapCacheContainer.insert(make_pair(*iterTempId, pCacheContainer));	
		}
	}
	CChildMsgThread::Start();
	return 0;
}

void CCacheThread::OnAddCacheServer( vector<unsigned int> dwVecServiceId, const string &strAddr )
{
	string szServiceId = GetServiceIdString(dwVecServiceId);
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s, ServiceId[%s], Addr[%s]\n", __FUNCTION__,szServiceId.c_str(), strAddr.c_str());
	SCacheAddServerMsg *pCacheAddServerMsg = new SCacheAddServerMsg;
	pCacheAddServerMsg->strAddr = strAddr;
	pCacheAddServerMsg->dwVecServiceId = dwVecServiceId;
	gettimeofday_a(&(pCacheAddServerMsg->tStart),0);
	PutQ(pCacheAddServerMsg);
}

void CCacheThread::DoAddCacheServer(SCacheMsg *pCacheMsg )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);

	SCacheAddServerMsg *pCacheAddServerMsg = (SCacheAddServerMsg *)pCacheMsg;
	if (pCacheAddServerMsg->dwVecServiceId.size()<=0)
	{
		CVS_XLOG(XLOG_ERROR, "%s,  CacheAgent not exist\n", __FUNCTION__);
		return;
	}
	unsigned int dwServiceId = pCacheAddServerMsg->dwVecServiceId[0];
	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if(pCacheContainer == NULL)
	{
		string szServiceId = GetServiceIdString(pCacheAddServerMsg->dwVecServiceId);
		CVS_XLOG(XLOG_ERROR, "%s, ServiceId[%s] CacheAgent not exist\n", __FUNCTION__, szServiceId.c_str());
		return;
	}
	
	pCacheContainer->AddCacheServer(pCacheAddServerMsg->strAddr);
}

void CCacheThread::Deal( void *pData )
{
	SCacheMsg *pCacheMsg = (SCacheMsg *)pData;

	//如果超时
	float dwTimeInterval = GetTimeInterval(pCacheMsg->tStart)/1000;

	if(dwTimeInterval > m_dwTimeoutInterval)	
	{
		CVS_XLOG(XLOG_ERROR, "CCacheThread::%s, Cache thread timeout %f seconds\n", __FUNCTION__, dwTimeInterval);

		m_dwTimeoutNum++;
		
		//连续超时告警
		if(m_dwTimeoutNum >= m_dwConditionTimes)
		{
			m_dwTimeoutNum = 0;
			if(GetTimeInterval(m_tmLastTimeoutWarn)/1000 > m_dwTimeoutAlarmFrequency)
			{
				char szWarnInfo[256] = {0};
				snprintf(szWarnInfo, sizeof(szWarnInfo)-1, "cache thread timeout %d seconds over %d times", m_dwTimeoutInterval, m_dwConditionTimes);
				m_pCacheVirtualService->Warn(szWarnInfo);
				gettimeofday_a(&m_tmLastTimeoutWarn, 0);
			}
		}

		delete pCacheMsg;
		return;
	}
	else
	{
		m_dwTimeoutNum = 0;
	}

	(this->*(m_cacheFunc[pCacheMsg->m_enCacheOperator]))(pCacheMsg);
	delete pCacheMsg;
}

void CCacheThread::DoGet( SCacheMsg *pCacheMsg )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	SCacheGetMsg *pCacheGetMsg = (SCacheGetMsg *)pCacheMsg;

	CSapDecoder decoder(pCacheGetMsg->pBuffer, pCacheGetMsg->dwLen);
	unsigned int dwServiceId = decoder.GetServiceId();
	unsigned int dwMsgId = decoder.GetMsgId();
	unsigned int dwSequenceId = decoder.GetSequence();

	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = CVS_SUCESS;

	nRet = GetRequestCacheInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetMsg->strGuid, dwServiceId, dwMsgId, pCacheGetMsg->tStart, strKey, "", dwKeepTime, nRet, "");
		m_pCacheVirtualService->Response(pCacheGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if(pCacheContainer == NULL)
	{
		m_pCacheVirtualService->Log(pCacheGetMsg->strGuid, dwServiceId, dwMsgId, pCacheGetMsg->tStart, strKey, "", dwKeepTime, CVS_NO_CACHE_SERVER, "");
		m_pCacheVirtualService->Response(pCacheGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_NO_CACHE_SERVER);
		return;
	}

	string strIpAddrs;
	string strCas;
	nRet = pCacheContainer->Get(strKey, strValue, strIpAddrs,strCas);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetMsg->strGuid, dwServiceId, dwMsgId, pCacheGetMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs);
		m_pCacheVirtualService->Response(pCacheGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CVS_XLOG(XLOG_INFO, "CCacheThread::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());
	
	bool isClear = isClearText(dwServiceId);
	vector<CVSKeyValue> vecKeyValue;
	nRet = CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue,isClear);

	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetMsg->strGuid, dwServiceId, dwMsgId, pCacheGetMsg->tStart, strKey, "", dwKeepTime, CVS_SERIALIZE_ERROR, strIpAddrs);
		m_pCacheVirtualService->Response(pCacheGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_SERIALIZE_ERROR);
		return;
	}

	string strLogValue = VectorToString(vecKeyValue);

	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s, Cas[%s]\n", __FUNCTION__, strCas.c_str());
	m_pCacheVirtualService->Log(pCacheGetMsg->strGuid, dwServiceId, dwMsgId, pCacheGetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
	Response(pCacheGetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
}

void CCacheThread::DoGetAndCas( SCacheMsg *pCacheMsg )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	SCacheGetCasMsg *pCacheGetCasMsg = (SCacheGetCasMsg *)pCacheMsg;

	CSapDecoder decoder(pCacheGetCasMsg->pBuffer, pCacheGetCasMsg->dwLen);
	unsigned int dwServiceId = decoder.GetServiceId();
	unsigned int dwMsgId = decoder.GetMsgId();
	unsigned int dwSequenceId = decoder.GetSequence();
	bool isClear = isClearText(dwServiceId);
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	string strRequestValue;
	string strLogValue;
	unsigned int dwKeepTime = 0;
	int nRet = CVS_SUCESS;

	multimap<unsigned int, string> mapRequestKey;
	multimap<unsigned int, string> mapRequestValue;
	nRet = GetRequestCacheInfo(dwServiceId, pBody, dwBodyLen, strKey, strRequestValue, mapRequestKey, mapRequestValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetCasMsg->strGuid, dwServiceId, dwMsgId, pCacheGetCasMsg->tStart, strKey, "", dwKeepTime, nRet, "");
		m_pCacheVirtualService->Response(pCacheGetCasMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if(pCacheContainer == NULL)
	{
		m_pCacheVirtualService->Log(pCacheGetCasMsg->strGuid, dwServiceId, dwMsgId, pCacheGetCasMsg->tStart, strKey, "", dwKeepTime, CVS_NO_CACHE_SERVER, "");
		m_pCacheVirtualService->Response(pCacheGetCasMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_NO_CACHE_SERVER);
		return;
	}

	string strIpAddrs;
	string strCas;
	//CAS操作尝试三次
	for (int iTryTimes = 0; iTryTimes < 3;++iTryTimes)
	{
		nRet = pCacheContainer->Get(strKey, strValue, strIpAddrs,strCas);
		CVS_XLOG(XLOG_INFO, "CCacheThread::%s, nRet[%d], Key[%s], Value[%s]\n", __FUNCTION__, nRet, strKey.c_str(), CacheValueToLogValue(strValue,isClear).c_str());
		if(nRet == CVS_SUCESS)
		{
			vector<CVSKeyValue> vecKeyValue;
			multimap<unsigned int, string> mapStoredValue;
			nRet = CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue, isClear);
			KeyValueToMMap(vecKeyValue, mapStoredValue);
			CasValueProcess(mapRequestValue, mapStoredValue);
			//DumpMMap(mapStoredValue);
			vecKeyValue.clear();
			MMapToKeyValue(mapStoredValue, vecKeyValue);

			strValue = CCvsArchiveAdapter::Oarchive(vecKeyValue,isClear);
			nRet = pCacheContainer->Cas(strKey, strValue, dwKeepTime, strCas, strIpAddrs);
			if(nRet == CVS_SUCESS)
			{
				strLogValue = VectorToString(vecKeyValue);
				m_pCacheVirtualService->Log(pCacheGetCasMsg->strGuid, dwServiceId, dwMsgId, pCacheGetCasMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
				Response(pCacheGetCasMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
				return;
			}
			else
			{
				continue;
			}
		}
		else if(nRet == CVS_CACHE_KEY_NOT_FIND)
		{
			nRet = pCacheContainer->Add(strKey, strRequestValue, dwKeepTime, strIpAddrs);
			if(nRet == CVS_SUCESS)
			{
				vector<CVSKeyValue> vecKeyValue;
				nRet = CCvsArchiveAdapter::Iarchive(strRequestValue, vecKeyValue, isClear);
				strLogValue = VectorToString(vecKeyValue);

				m_pCacheVirtualService->Log(pCacheGetCasMsg->strGuid, dwServiceId, dwMsgId, pCacheGetCasMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
				Response(pCacheGetCasMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
				return;
			}
			else if(nRet == CVS_CACHE_ALREADY_EXIST)
			{
				nRet = pCacheContainer->Get(strKey, strValue, strIpAddrs,strCas);
				CVS_XLOG(XLOG_INFO, "CCacheThread::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());
				if(nRet != CVS_SUCESS)
				{
					continue;
				}

				vector<CVSKeyValue> vecKeyValue;
				nRet = CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue, isClear);
				multimap<unsigned int, string> mapStoredValue;
				KeyValueToMMap(vecKeyValue, mapStoredValue);
				CasValueProcess(mapRequestValue, mapStoredValue);
				vecKeyValue.clear();
				MMapToKeyValue(mapStoredValue, vecKeyValue);

				strValue = CCvsArchiveAdapter::Oarchive(vecKeyValue,isClear);
				nRet = pCacheContainer->Cas(strKey, strValue, dwKeepTime, strCas, strIpAddrs);
				if(nRet == CVS_SUCESS)
				{
					strLogValue = VectorToString(vecKeyValue);
					m_pCacheVirtualService->Log(pCacheGetCasMsg->strGuid, dwServiceId, dwMsgId, pCacheGetCasMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
					Response(pCacheGetCasMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
					return;
				}
				else
				{
					continue;
				}
			}
		}
		else
		{
			continue;
		}
	}

	m_pCacheVirtualService->Log(pCacheGetCasMsg->strGuid, dwServiceId, dwMsgId, pCacheGetCasMsg->tStart, strKey, CacheValueToLogValue(strRequestValue,isClear), dwKeepTime, nRet, strIpAddrs);
	m_pCacheVirtualService->Response(pCacheGetCasMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CCacheThread::DoSetOnly(SCacheMsg *pCacheMsg)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	SCacheSetOnlyMsg *pCacheSetOnlyMsg = (SCacheSetOnlyMsg *)pCacheMsg;

	CSapDecoder decoder(pCacheSetOnlyMsg->pBuffer, pCacheSetOnlyMsg->dwLen);
	unsigned int dwServiceId = decoder.GetServiceId();
	unsigned int dwMsgId = decoder.GetMsgId();
	unsigned int dwSequenceId = decoder.GetSequence();
	bool isClear = isClearText(dwServiceId);
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	string strRequestValue;
	string strLogValue;
	unsigned int dwKeepTime = 0;
	int nRet = CVS_SUCESS;

	multimap<unsigned int, string> mapRequestKey;
	multimap<unsigned int, string> mapRequestValue;
	nRet = GetRequestCacheInfo(dwServiceId, pBody, dwBodyLen, strKey, strRequestValue, mapRequestKey, mapRequestValue, dwKeepTime);
	if (nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheSetOnlyMsg->strGuid, dwServiceId, dwMsgId, pCacheSetOnlyMsg->tStart, strKey, "", dwKeepTime, nRet, "");
		m_pCacheVirtualService->Response(pCacheSetOnlyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if (pCacheContainer == NULL)
	{
		m_pCacheVirtualService->Log(pCacheSetOnlyMsg->strGuid, dwServiceId, dwMsgId, pCacheSetOnlyMsg->tStart, strKey, "", dwKeepTime, CVS_NO_CACHE_SERVER, "");
		m_pCacheVirtualService->Response(pCacheSetOnlyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_NO_CACHE_SERVER);
		return;
	}

	string strIpAddrs;
	string strCas;
	//SetOnly操作尝试三次
	for (int iTryTimes = 0; iTryTimes < 3; ++iTryTimes)
	{
		nRet = pCacheContainer->Get(strKey, strValue, strIpAddrs, strCas);
		CVS_XLOG(XLOG_INFO, "CCacheThread::%s, nRet[%d], Key[%s], Value[%s]\n", __FUNCTION__, nRet, strKey.c_str(), CacheValueToLogValue(strValue, isClear).c_str());
		if (nRet == CVS_SUCESS)
		{
			vector<CVSKeyValue> vecKeyValue;
			multimap<unsigned int, string> mapStoredValue;
			nRet = CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue, isClear);
			KeyValueToMMap(vecKeyValue, mapStoredValue);
			SetOnlyValueProcess(mapRequestValue, mapStoredValue);
			//DumpMMap(mapStoredValue);
			vecKeyValue.clear();
			MMapToKeyValue(mapStoredValue, vecKeyValue);

			strValue = CCvsArchiveAdapter::Oarchive(vecKeyValue, isClear);
			nRet = pCacheContainer->Cas(strKey, strValue, dwKeepTime, strCas, strIpAddrs);
			if (nRet == CVS_SUCESS)
			{
				strLogValue = VectorToString(vecKeyValue);
				m_pCacheVirtualService->Log(pCacheSetOnlyMsg->strGuid, dwServiceId, dwMsgId, pCacheSetOnlyMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
				Response(pCacheSetOnlyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
				return;
			}
			else
			{
				continue;
			}
		}
		else if (nRet == CVS_CACHE_KEY_NOT_FIND)
		{
			nRet = pCacheContainer->Add(strKey, strRequestValue, dwKeepTime, strIpAddrs);
			if (nRet == CVS_SUCESS)
			{
				vector<CVSKeyValue> vecKeyValue;
				nRet = CCvsArchiveAdapter::Iarchive(strRequestValue, vecKeyValue, isClear);
				strLogValue = VectorToString(vecKeyValue);

				m_pCacheVirtualService->Log(pCacheSetOnlyMsg->strGuid, dwServiceId, dwMsgId, pCacheSetOnlyMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
				Response(pCacheSetOnlyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
				return;
			}
			else if (nRet == CVS_CACHE_ALREADY_EXIST)
			{
				nRet = pCacheContainer->Get(strKey, strValue, strIpAddrs, strCas);
				CVS_XLOG(XLOG_INFO, "CCacheThread::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());
				if (nRet != CVS_SUCESS)
				{
					continue;
				}

				vector<CVSKeyValue> vecKeyValue;
				nRet = CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue, isClear);
				multimap<unsigned int, string> mapStoredValue;
				KeyValueToMMap(vecKeyValue, mapStoredValue);
				SetOnlyValueProcess(mapRequestValue, mapStoredValue);
				vecKeyValue.clear();
				MMapToKeyValue(mapStoredValue, vecKeyValue);

				strValue = CCvsArchiveAdapter::Oarchive(vecKeyValue, isClear);
				nRet = pCacheContainer->Cas(strKey, strValue, dwKeepTime, strCas, strIpAddrs);
				if (nRet == CVS_SUCESS)
				{
					strLogValue = VectorToString(vecKeyValue);
					m_pCacheVirtualService->Log(pCacheSetOnlyMsg->strGuid, dwServiceId, dwMsgId, pCacheSetOnlyMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
					Response(pCacheSetOnlyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
					return;
				}
				else
				{
					continue;
				}
			}
		}
		else
		{
			continue;
		}
	}

	m_pCacheVirtualService->Log(pCacheSetOnlyMsg->strGuid, dwServiceId, dwMsgId, pCacheSetOnlyMsg->tStart, strKey, CacheValueToLogValue(strRequestValue, isClear), dwKeepTime, nRet, strIpAddrs);
	m_pCacheVirtualService->Response(pCacheSetOnlyMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CCacheThread::DoSet( SCacheMsg *pCacheMsg )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);

	SCacheSetMsg *pCacheSetMsg = (SCacheSetMsg *)pCacheMsg;

	CSapDecoder decoder(pCacheSetMsg->pBuffer, pCacheSetMsg->dwLen);
	unsigned int dwServiceId = decoder.GetServiceId();
	unsigned int dwMsgId = decoder.GetMsgId();
	unsigned int dwSequenceId = decoder.GetSequence();
	bool isClear = isClearText(dwServiceId);
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = CVS_SUCESS;

	nRet = GetRequestCacheInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheSetMsg->strGuid, dwServiceId, dwMsgId, pCacheSetMsg->tStart, strKey, "", dwKeepTime, nRet, "");
		m_pCacheVirtualService->Response(pCacheSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	vector<CVSKeyValue> vecKeyValue;
	CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue, isClear);
	string strLogValue = VectorToString(vecKeyValue);

	CVS_XLOG(XLOG_INFO, "CCacheThread::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());

	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if(pCacheContainer == NULL)
	{
		m_pCacheVirtualService->Log(pCacheSetMsg->strGuid, dwServiceId, dwMsgId, pCacheSetMsg->tStart, strKey, strLogValue, dwKeepTime, CVS_NO_CACHE_SERVER, "");
		m_pCacheVirtualService->Response(pCacheSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_NO_CACHE_SERVER);
		return;
	}

	string strIpAddrs;
	nRet = pCacheContainer->Set(strKey, strValue, dwKeepTime, strIpAddrs);

	m_pCacheVirtualService->Log(pCacheSetMsg->strGuid, dwServiceId, dwMsgId, pCacheSetMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
	m_pCacheVirtualService->Response(pCacheSetMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CCacheThread::DoDelete( SCacheMsg *pCacheMsg )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	SCacheDeleteMsg *pCacheDeleteMsg = (SCacheDeleteMsg *)pCacheMsg;

	CSapDecoder decoder(pCacheDeleteMsg->pBuffer, pCacheDeleteMsg->dwLen);
	unsigned int dwServiceId = decoder.GetServiceId();
	unsigned int dwMsgId = decoder.GetMsgId();
	unsigned int dwSequenceId = decoder.GetSequence();

	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = CVS_SUCESS;

	nRet = GetRequestCacheInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheDeleteMsg->tStart, strKey, "", dwKeepTime, nRet, "");
		m_pCacheVirtualService->Response(pCacheDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CVS_XLOG(XLOG_INFO, "CCacheThread::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());

	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if(pCacheContainer == NULL)
	{
		m_pCacheVirtualService->Log(pCacheDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheDeleteMsg->tStart, strKey, "", dwKeepTime, CVS_NO_CACHE_SERVER, "");
		m_pCacheVirtualService->Response(pCacheDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_NO_CACHE_SERVER);
		return;
	}

	string strIpAddrs;
	nRet = pCacheContainer->Delete(strKey, strIpAddrs);

	m_pCacheVirtualService->Log(pCacheDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheDeleteMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs);
	m_pCacheVirtualService->Response(pCacheDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
}

void CCacheThread::DoGetAndDelete( SCacheMsg *pCacheMsg )
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	SCacheGetDeleteMsg *pCacheGetDeleteMsg = (SCacheGetDeleteMsg *)pCacheMsg;

	CSapDecoder decoder(pCacheGetDeleteMsg->pBuffer, pCacheGetDeleteMsg->dwLen);
	unsigned int dwServiceId = decoder.GetServiceId();
	unsigned int dwMsgId = decoder.GetMsgId();
	unsigned int dwSequenceId = decoder.GetSequence();
	bool isClear = isClearText(dwServiceId);
		
	void *pBody = NULL;
	unsigned int dwBodyLen = 0;
	decoder.GetBody(&pBody, &dwBodyLen);
	string strKey;
	string strValue;
	unsigned int dwKeepTime = 0;
	int nRet = CVS_SUCESS;

	nRet = GetRequestCacheInfo(dwServiceId, pBody, dwBodyLen, strKey, strValue, dwKeepTime);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheGetDeleteMsg->tStart, strKey, "", dwKeepTime, nRet, "");
		m_pCacheVirtualService->Response(pCacheGetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	CCacheContainer *pCacheContainer = GetCacheContainer(dwServiceId);
	if(pCacheContainer == NULL)
	{
		m_pCacheVirtualService->Log(pCacheGetDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheGetDeleteMsg->tStart, strKey, "", dwKeepTime, CVS_NO_CACHE_SERVER, "");
		m_pCacheVirtualService->Response(pCacheGetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_NO_CACHE_SERVER);
		return;
	}

	string strIpAddrs;
	string strCas;
	nRet = pCacheContainer->Get(strKey, strValue, strIpAddrs,strCas);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheGetDeleteMsg->tStart, strKey, "", dwKeepTime, nRet, strIpAddrs);
		m_pCacheVirtualService->Response(pCacheGetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet);
		return;
	}

	string strDeleteIpAddrs;
	nRet = pCacheContainer->Delete(strKey, strDeleteIpAddrs);

	CVS_XLOG(XLOG_INFO, "CCacheThread::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());

	vector<CVSKeyValue> vecKeyValue;
	nRet = CCvsArchiveAdapter::Iarchive(strValue, vecKeyValue, isClear);
	if(nRet != 0)
	{
		m_pCacheVirtualService->Log(pCacheGetDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheGetDeleteMsg->tStart, strKey, "", dwKeepTime, CVS_SERIALIZE_ERROR, strIpAddrs);
		m_pCacheVirtualService->Response(pCacheGetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, CVS_SERIALIZE_ERROR);
		return;
	}

	string strLogValue = VectorToString(vecKeyValue);

	m_pCacheVirtualService->Log(pCacheGetDeleteMsg->strGuid, dwServiceId, dwMsgId, pCacheGetDeleteMsg->tStart, strKey, strLogValue, dwKeepTime, nRet, strIpAddrs);
	Response(pCacheGetDeleteMsg->handler, dwServiceId, dwMsgId, dwSequenceId, nRet, vecKeyValue);
}

CCacheContainer * CCacheThread::GetCacheContainer(unsigned int dwServiceId)
{
	map<unsigned int, CCacheContainer *>::const_iterator iter = m_mapCacheContainer.find(dwServiceId);
	if(iter == m_mapCacheContainer.end())
	{
		return NULL;
	}

	return iter->second;
}

void CCacheThread::Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const vector<CVSKeyValue> & vecKeyValue)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s, ServiceId[%d], MsgId[%d], Code[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, nCode);
	CSapEncoder sapEncoder(SAP_PACKET_RESPONSE, dwServiceId, dwMsgId, nCode);
	sapEncoder.SetSequence(dwSequenceId);
	
	TYPE_CODE_MAP mapTypeByCode = m_mapCodeTypeByService[dwServiceId];

	vector<CVSKeyValue>::const_iterator iter;
	for (iter = vecKeyValue.begin(); iter != vecKeyValue.end(); iter++)
	{
		if(mapTypeByCode[iter->dwKey] == MSG_FIELD_INT)
		{
			sapEncoder.SetValue(iter->dwKey, atoi(iter->strValue.c_str()));
		}
		else
		{
			sapEncoder.SetValue(iter->dwKey, iter->strValue);
		}
	}

	m_pCacheVirtualService->Response(handler, sapEncoder.GetBuffer(), sapEncoder.GetLen());
}

//判断字符是否可见
bool CCacheThread::IsCacheKeyValid( const string &strKey )
{
	const char *p = strKey.c_str();
	while(*p!='\0')
	{
		if(*p>126||*p<33)
		{
			return false;
		}
		p++;
	}

	return true;
}

//进行Base64编码
string CCacheThread::MakeCacheKeyValid( const string &strKey )
{
	string strEncodeValue;

	char szBase64[2048] = {0};
	CCipher::Base64Encode(szBase64, strKey.c_str(), strKey.length());
	strEncodeValue = szBase64;

	return strEncodeValue;
}


int CCacheThread::GetRequestCacheInfo( unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, unsigned int &dwKeepTime )
{
	multimap<unsigned int, string> mapRequest;
	multimap<unsigned int, string> mapResponse;
	
	TYPE_CODE_MAP mapTypeByCode = m_mapCodeTypeByService[dwServiceId];
	dwKeepTime = m_mapCacheKeepTime[dwServiceId];

	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	while(ptrLoc < (unsigned char *)pBuffer + dwLen)
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen=ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
			return CVS_AVENUE_PACKET_ERROR;
		}

		unsigned short nType = ntohs(pAtti->wType);
		string strValue = string((char *)(ptrLoc+sizeof(SSapMsgAttribute)), nLen-sizeof(SSapMsgAttribute));

		if(mapTypeByCode[nType] == MSG_FIELD_INT)
		{
			char szTemp[16] = {0};
			snprintf(szTemp, sizeof(szTemp)-1, "%d", ntohl(*(int *)strValue.c_str()));
			strValue = szTemp;
		}

		if(nType < 10000)
		{
			mapRequest.insert(make_pair(nType, strValue));
		}
		else if(nType == 10000)
		{
			dwKeepTime = atoi(strValue.c_str());
		}
		else
		{
			mapResponse.insert(make_pair(nType, strValue));
		}

		ptrLoc+=nFactLen;
	}

	/*
	printf("############begin dump############\n");
	printf("KeepTime:\t%d\n", dwKeepTime);
	printf("Request:\t%d\n", mapRequest.size());
	map<unsigned int, string>::iterator iter;
	for (iter = mapRequest.begin(); iter != mapRequest.end(); iter++)
	{
		printf("\t%d:\t%s\n", iter->first, iter->second.c_str());
	}

	printf("Response:\t%d\n", mapResponse.size());
	for (iter = mapResponse.begin(); iter != mapResponse.end(); iter++)
	{
		printf("\t%d:\t%s\n", iter->first, iter->second.c_str());
	}

	printf("############end dump############\n");
	*/

	if(dwKeepTime > 2592000)
	{
		dwKeepTime = 2592000;
	}

	//获取key值
	multimap<unsigned int, string>::const_iterator iterRequest;

	
	bool isClear = isClearText(dwServiceId);
	if (isClear)
	{
		for (iterRequest = mapRequest.begin(); iterRequest != mapRequest.end(); iterRequest++)
		{
			strKey = iterRequest->second;
			break;
		}

		string strKeyPrefix = getKeyPrefix(dwServiceId);
		if (!strKeyPrefix.empty())
		{
			strKey = strKeyPrefix + strKey;
		}
	}
	else
	{
		//key前缀
		char szPrefix[16] = {0};
		snprintf(szPrefix, sizeof(szPrefix)-1, "BPE%d", dwServiceId);
		strKey = szPrefix;

		for (iterRequest = mapRequest.begin(); iterRequest != mapRequest.end(); iterRequest++)
		{
			strKey += "_" + iterRequest->second;
		}
		
		if(strKey.length() > 1024)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheThread::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
			return CVS_KEY_TOO_LONG;
		}

		if(!IsCacheKeyValid(strKey))
		{
			strKey = MakeCacheKeyValid(strKey);
		}
	}
	//获取value值
	vector<CVSKeyValue> vecKeyValue;
	multimap<unsigned int, string>::const_iterator iterResponse;
	for (iterResponse = mapResponse.begin(); iterResponse != mapResponse.end(); iterResponse++)
	{
		CVSKeyValue tempKeyValue;
		tempKeyValue.dwKey = iterResponse->first;
		tempKeyValue.strValue = iterResponse->second;
		vecKeyValue.push_back(tempKeyValue);
	}

	strValue = CCvsArchiveAdapter::Oarchive(vecKeyValue, isClear);

	return 0;
}

int CCacheThread::GetRequestCacheInfo( unsigned int dwServiceId, 
									  const void *pBuffer, 
									  unsigned int dwLen, 
									  string &strKey, 
									  string &strValue, 
									  multimap<unsigned int, string> &mapRequest,
									  multimap<unsigned int, string> &mapResponse,
									  unsigned int &dwKeepTime )
{
	
	TYPE_CODE_MAP mapTypeByCode = m_mapCodeTypeByService[dwServiceId];
	dwKeepTime = m_mapCacheKeepTime[dwServiceId];

	unsigned char *ptrLoc = (unsigned char *)pBuffer;
	while(ptrLoc < (unsigned char *)pBuffer + dwLen)
	{
		SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
		unsigned short nLen=ntohs(pAtti->wLength);
		int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

		if(nLen==0||ptrLoc+nFactLen>(unsigned char *)pBuffer+dwLen)
		{
			return CVS_AVENUE_PACKET_ERROR;
		}

		unsigned short nType = ntohs(pAtti->wType);
		string strValue = string((char *)(ptrLoc+sizeof(SSapMsgAttribute)), nLen-sizeof(SSapMsgAttribute));

		if(mapTypeByCode[nType] == MSG_FIELD_INT)
		{
			char szTemp[16] = {0};
			snprintf(szTemp, sizeof(szTemp)-1, "%d", ntohl(*(int *)strValue.c_str()));
			strValue = szTemp;
		}

		if(nType < 10000)
		{
			mapRequest.insert(make_pair(nType, strValue));
		}
		else if(nType == 10000)
		{
			dwKeepTime = atoi(strValue.c_str());
		}
		else
		{
			mapResponse.insert(make_pair(nType, strValue));
		}

		ptrLoc+=nFactLen;
	}

	/*
	printf("############begin dump############\n");
	printf("KeepTime:\t%d\n", dwKeepTime);
	printf("Request:\t%d\n", mapRequest.size());
	map<unsigned int, string>::iterator iter;
	for (iter = mapRequest.begin(); iter != mapRequest.end(); iter++)
	{
		printf("\t%d:\t%s\n", iter->first, iter->second.c_str());
	}

	printf("Response:\t%d\n", mapResponse.size());
	for (iter = mapResponse.begin(); iter != mapResponse.end(); iter++)
	{
		printf("\t%d:\t%s\n", iter->first, iter->second.c_str());
	}

	printf("############end dump############\n");
	*/

	if(dwKeepTime > 2592000)
	{
		dwKeepTime = 2592000;
	}

	//获取key值
	multimap<unsigned int, string>::const_iterator iterRequest;
	bool isClear = isClearText(dwServiceId);
	if (isClear)
	{
		for (iterRequest = mapRequest.begin(); iterRequest != mapRequest.end(); iterRequest++)
		{
			strKey = iterRequest->second;
			break;
		}

		string strKeyPrefix = getKeyPrefix(dwServiceId);
		if (!strKeyPrefix.empty())
		{
			strKey = strKeyPrefix + strKey;
		}
	}
	else
	{
		//key前缀
		char szPrefix[16] = {0};
		snprintf(szPrefix, sizeof(szPrefix)-1, "BPE%d", dwServiceId);
		strKey = szPrefix;

		for (iterRequest = mapRequest.begin(); iterRequest != mapRequest.end(); iterRequest++)
		{
			strKey += "_" + iterRequest->second;
		}
		
		if(strKey.length() > 1024)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheThread::%s, Key too long[%d] > 1024\n", __FUNCTION__, strKey.length());
			return CVS_KEY_TOO_LONG;
		}

		if(!IsCacheKeyValid(strKey))
		{
			strKey = MakeCacheKeyValid(strKey);
		}
	}
	//获取value值
	vector<CVSKeyValue> vecKeyValue;
	multimap<unsigned int, string>::const_iterator iterResponse;
	for (iterResponse = mapResponse.begin(); iterResponse != mapResponse.end(); iterResponse++)
	{
		CVSKeyValue tempKeyValue;
		tempKeyValue.dwKey = iterResponse->first;
		tempKeyValue.strValue = iterResponse->second;
		vecKeyValue.push_back(tempKeyValue);
	}

	strValue = CCvsArchiveAdapter::Oarchive(vecKeyValue, isClear);

	return 0;

}
string CCacheThread::VectorToString(const vector<CVSKeyValue> &vecKeyValue)
{
	string strValue;
	vector<CVSKeyValue>::const_iterator iter;
	bool bFirst = true;
	for (iter = vecKeyValue.begin(); iter != vecKeyValue.end(); iter++)
	{
		const CVSKeyValue &objKeyValue = *iter;

		char szTemp[16] = {0};
		snprintf(szTemp, sizeof(szTemp)-1, "%d_", objKeyValue.dwKey);
		if(bFirst)
		{
			strValue += szTemp + objKeyValue.strValue;

			bFirst = false;
		}
		else
		{
			strValue += VALUE_SPLITTER;
			strValue += szTemp + objKeyValue.strValue;
		}
	}

	return strValue;
}

void CCacheThread::CasValueProcess(const multimap<unsigned int, string> &mapRequestValue, multimap<unsigned int, string> &mapStoredValue)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	multimap<unsigned int, string>::iterator itr;
	multimap<unsigned int, string>::const_iterator citr;
	for (citr=mapRequestValue.begin(); citr!=mapRequestValue.end(); ++citr)
	{
		itr = mapStoredValue.find(citr->first);
		if(itr==mapStoredValue.end()) 
		{
			mapStoredValue.insert(make_pair(citr->first, citr->second));
		}
		else
		{
			unsigned int dwStoreValue = atoi(itr->second.c_str());
			unsigned int dwNeedAddValue = atoi(citr->second.c_str());
			dwStoreValue += dwNeedAddValue;

			char szTemp[32] = {0};
			snprintf(szTemp, sizeof(szTemp), "%d", dwStoreValue);
			itr->second = szTemp;
		}
	}
}

void CCacheThread::SetOnlyValueProcess(const multimap<unsigned int, string> &mapRequestValue, multimap<unsigned int, string> &mapStoredValue)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	multimap<unsigned int, string>::iterator itr;
	multimap<unsigned int, string>::const_iterator citr; 
	for (citr = mapRequestValue.begin(); citr != mapRequestValue.end(); ++citr)
	{
		itr = mapStoredValue.find(citr->first);
		if (itr == mapStoredValue.end())
		{
			mapStoredValue.insert(make_pair(citr->first, citr->second));
		}
		else
		{
			itr->second = citr->second;
		}
	}
}

void CCacheThread::KeyValueToMMap(const vector<CVSKeyValue> &vecKeyValue, multimap<unsigned int, string> &mmap)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	vector<CVSKeyValue>::const_iterator itr;
	for (itr=vecKeyValue.begin(); itr!=vecKeyValue.end(); ++itr)
	{
		mmap.insert(make_pair(itr->dwKey, itr->strValue));
	}
}

void CCacheThread::MMapToKeyValue(const multimap<unsigned int, string> &mmap, vector<CVSKeyValue> &vecKeyValue)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	multimap<unsigned int, string>::const_iterator citr;
	for (citr=mmap.begin(); citr!=mmap.end(); ++citr)
	{
		CVSKeyValue objKeyValue;
		objKeyValue.dwKey = citr->first;
		objKeyValue.strValue = citr->second;
		vecKeyValue.push_back(objKeyValue);
	}
}

void CCacheThread::DumpMMap(const multimap<unsigned int, string> &mmap)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s\n", __FUNCTION__);
	multimap<unsigned int, string>::const_iterator citr;
	for (citr=mmap.begin(); citr!=mmap.end(); ++citr)
	{
		CVS_XLOG(XLOG_DEBUG, "CCacheThread::%s, [%d:%s]\n", __FUNCTION__, citr->first, citr->second.c_str());
	}
}

string CCacheThread::CacheValueToLogValue(const string &strCacheValue, bool isClear)
{
	if(strCacheValue.empty())
	{
		return "";
	}

	vector<CVSKeyValue> vecKeyValue;
	CCvsArchiveAdapter::Iarchive(strCacheValue, vecKeyValue, isClear);
	return VectorToString(vecKeyValue);
}
