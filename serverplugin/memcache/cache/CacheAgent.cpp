#include "CacheAgent.h"
#include "AsyncVirtualServiceLog.h"
#include "ErrorMsg.h"

CCacheAgent::CCacheAgent():m_pCacheMem(NULL)
{
}

CCacheAgent::~CCacheAgent()
{
	if(m_pCacheMem != NULL)
	{
		memcached_free(m_pCacheMem);
	}
}

int CCacheAgent::Initialize(const string &strAddr)
{
	m_strCacheServer = strAddr;

	size_t npos = strAddr.find(":");
	if(npos == string::npos)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheAgent::%s, CacheServer[%s] format error\n", __FUNCTION__, m_strCacheServer.c_str());
		return -1;
	}

	string strCacheServerIp = strAddr.substr(0, npos);
	unsigned int dwCacheServerPort = atoi(strAddr.substr(npos+1).c_str());
	memcached_return_t rc;
	m_pCacheMem = memcached_create(NULL);
	memcached_server_st *pCacheServer = memcached_server_list_append(NULL, strCacheServerIp.c_str(), dwCacheServerPort, &rc);
	if(rc != MEMCACHED_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheAgent::%s, Cache Server Create Error, Server[%s], Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), memcached_strerror(m_pCacheMem, rc));
		return -1;
	}

	rc = memcached_server_push(m_pCacheMem, pCacheServer);
	if(rc != MEMCACHED_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheAgent::%s, Cache Server Push Error, Server[%s], Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), memcached_strerror(m_pCacheMem, rc));
		return -1;
	}

	memcached_server_list_free(pCacheServer);

	//ÉèÖÃÊôÐÔ
	memcached_behavior_set(m_pCacheMem, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
	memcached_behavior_set(m_pCacheMem, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);

	CVS_XLOG(XLOG_INFO, "CCacheAgent::%s, Cache Server Start[%s]\n", __FUNCTION__, strAddr.c_str());

	return 0;
}

int CCacheAgent::Add(const string & strKey,const string & strValue, unsigned int dwExpiration)
{
	memcached_return_t rc = memcached_add(m_pCacheMem, strKey.c_str(), strKey.size(), strValue.c_str(), strValue.size(), dwExpiration, 0);
	if(rc == MEMCACHED_SUCCESS)
	{
		return CVS_SUCESS;
	}
	else if(rc == MEMCACHED_DATA_EXISTS)
	{
		CVS_XLOG(XLOG_DEBUG, "CCacheUtil::%s, Server[%s] : Cache Add data already exist, KeyValue[%s : %s] Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), strValue.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_ALREADY_EXIST;
	}
	else
	{
		CVS_XLOG(XLOG_ERROR, "CCacheUtil::%s, Server[%s] : Cache Add Error, KeyValue[%s : %s] Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), strValue.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_CONNECT_ERROR;
	}
}

int CCacheAgent::Set(const string & strKey,const string & strValue, unsigned int dwExpiration)
{
	memcached_return_t rc = memcached_set(m_pCacheMem, strKey.c_str(), strKey.size(), strValue.c_str(), strValue.size(), dwExpiration, 0);
	if(rc != MEMCACHED_SUCCESS)
	{
		CVS_XLOG(XLOG_ERROR, "CCacheUtil::%s, Server[%s] : Cache Set Error, KeyValue[%s : %s] Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), strValue.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_CONNECT_ERROR;
	}
	return CVS_SUCESS;
}

int CCacheAgent::Cas( const string &strKey, const string &strValue, const string &strCas, unsigned int dwExpiration /*= 0*/ )
{
	unsigned long long uCas = atoll(strCas.c_str());
	memcached_return_t rc = memcached_cas(m_pCacheMem, strKey.c_str(), strKey.size(), strValue.c_str(), strValue.size(), dwExpiration, 0, uCas);
	if(rc==MEMCACHED_SUCCESS)
	{
		return CVS_SUCESS;
	}
	else if(rc == MEMCACHED_NOTFOUND)
	{
		CVS_XLOG(XLOG_DEBUG, "CCacheUtil::%s, Server[%s] : Cache Cas key not found, Key[%s], Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_KEY_NOT_FIND;
	}
	else if(rc == MEMCACHED_DATA_EXISTS)
	{
		CVS_XLOG(XLOG_DEBUG, "CCacheUtil::%s, Server[%s] : Cache Cas version not accord, Key[%s], Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_KEY_VERSION_NOT_ACCORD;
	}
	else
	{
		CVS_XLOG(XLOG_ERROR, "CCacheUtil::%s, Server[%s] : Cache Add Error, KeyValue[%s : %s] Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), strValue.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_CONNECT_ERROR;
	}
}

int CCacheAgent::Get(const string & strKey, string &strValue, string &strCas)
{
	size_t nLen;
	uint32_t nflags; 
	memcached_return_t rc;
	char *szValue = memcached_get(m_pCacheMem, strKey.c_str(), strKey.size(), &nLen, &nflags, &rc);

	unsigned long long uCas = memcached_result_cas(&(m_pCacheMem->result));
	char szCas[32] = {0};
	snprintf(szCas, sizeof(szCas), "%llu", uCas);
	strCas = szCas;

	if(rc == MEMCACHED_SUCCESS)
	{
		strValue.assign(szValue, nLen);
		free(szValue);
		return CVS_SUCESS;
	}
	else if(rc == MEMCACHED_NOTFOUND)
	{
		CVS_XLOG(XLOG_DEBUG, "CCacheUtil::%s, Server[%s] : Cache Get Key not find, Key[%s], Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_KEY_NOT_FIND;
	}
	else
	{
		CVS_XLOG(XLOG_ERROR, "CCacheUtil::%s, Server[%s] : Cache Get Key Error, Key[%s], Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_CONNECT_ERROR;
	}
}

int CCacheAgent::Delete(const string & strKey)
{
	memcached_return_t rc = memcached_delete(m_pCacheMem, strKey.c_str(), strKey.size(), 0);
	if(rc == MEMCACHED_SUCCESS)
	{
		return CVS_SUCESS;
	}
	else if(rc == MEMCACHED_NOTFOUND)
	{
		CVS_XLOG(XLOG_DEBUG, "CCacheUtil::%s, Server[%s] : Cache Delete Key not found, Key[%s] Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_KEY_NOT_FIND;
	}
	else
	{
		CVS_XLOG(XLOG_ERROR, "CCacheUtil::%s, Server[%s] : Cache Delete Key Error, Key[%s] Error[%s]\n", __FUNCTION__, m_strCacheServer.c_str(), strKey.c_str(), memcached_strerror(m_pCacheMem, rc));
		return CVS_CACHE_CONNECT_ERROR;
	}
}

string CCacheAgent::GetAddr()
{
	return m_strCacheServer;
}



