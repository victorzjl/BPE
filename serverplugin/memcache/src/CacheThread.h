#ifndef _CVS_CACHE_THREAD_H_
#define _CVS_CACHE_THREAD_H_

#include <ChildMsgThread.h>
#include "CacheContainer.h"

#include "CacheMsg.h"

using namespace sdo::common;

class CCacheVirtualService;
class CCacheThread : public CChildMsgThread
{
	typedef void (CCacheThread::*CacheFunc)(SCacheMsg *pCacheMsg);
public:
	CCacheThread( CMsgQueuePrio & oPublicQueue, const SERVICE_TYPE_CODE_MAP &mapCodeTypeByService, const CACHE_SET_TIME_MAP &mapCacheKeepTime, const map<unsigned int,bool>& mapcleartext, const map<unsigned int,string>& mapKeyPrefix);
	virtual ~CCacheThread();
	virtual void StartInThread();
	virtual void Deal(void *pData);
	int Start(const vector<string> &vecCacheSvr, CCacheVirtualService *pCacheVirtualService);
	bool GetSelfCheck();
	void OnAddCacheServer( vector<unsigned int> dwVecServiceId, const string &strAddr );
private:
	void DoGet(SCacheMsg *pCacheMsg);
	void DoSet(SCacheMsg *pCacheMsg);
	void DoDelete(SCacheMsg *pCacheMsg);
	void DoGetAndDelete(SCacheMsg *pCacheMsg);
	void DoGetAndCas( SCacheMsg *pCacheMsg );
	void DoSetOnly(SCacheMsg *pCacheMsg);
	void DoAddCacheServer(SCacheMsg *pCacheMsg );
	void DoSelfCheck();
private:
	bool IsCacheKeyValid(const string &strKey);
	string MakeCacheKeyValid(const string &strKey);
	int GetRequestCacheInfo( unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, unsigned int &dwKeepTime);
	int GetRequestCacheInfo( unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue, multimap<unsigned int, string> &mapRequest, multimap<unsigned int, string> &mapResponse, unsigned int &dwKeepTime );
	CCacheContainer *GetCacheContainer(unsigned int dwServiceId);
	void Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const vector<CVSKeyValue> & vecKeyValue);
	void AsyncLog(unsigned int dwServiceId, unsigned int dwMsgId, const timeval_a &tStart, const string &strKey, const string &strValue, int nCode);
	string VectorToString(const vector<CVSKeyValue> &vecKeyValue);
	void CasValueProcess(const multimap<unsigned int, string> &mapRequestValue, multimap<unsigned int, string> &mapStoredValue);
	void SetOnlyValueProcess(const multimap<unsigned int, string> &mapRequestValue, multimap<unsigned int, string> &mapStoredValue);
	void KeyValueToMMap(const vector<CVSKeyValue> &vecKeyValue, multimap<unsigned int, string> &mmap);
	void MMapToKeyValue(const multimap<unsigned int, string> &mmap, vector<CVSKeyValue> &vecKeyValue);
	void DumpMMap(const multimap<unsigned int, string> &mmap);
	string CacheValueToLogValue(const string &strCacheValue,bool isClear);
	bool isClearText(unsigned int dwServiceId);
	string getKeyPrefix(unsigned int dwServiceId);
private:
	CacheFunc m_cacheFunc[CVS_ALL];
	map<unsigned int, CCacheContainer *> m_mapCacheContainer;
	SERVICE_TYPE_CODE_MAP m_mapCodeTypeByService;
	CACHE_SET_TIME_MAP	m_mapCacheKeepTime;
	map<unsigned int, bool> m_mapClearText;
	map<unsigned int, string> m_mapKeyPrefix;
	bool			m_isAlive;
	unsigned int	m_dwTimeoutNum;
	unsigned int	m_dwTimeoutInterval;
	unsigned int	m_dwConditionTimes;
	unsigned int	m_dwTimeoutAlarmFrequency;
	timeval_a		m_tmLastTimeoutWarn;
	CCacheVirtualService *m_pCacheVirtualService;
	CThreadTimer	m_timerSelfCheck;
};

#endif
