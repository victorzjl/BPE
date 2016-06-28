#ifndef _CACHE_UTIL_H_
#define _CACHE_UTIL_H_

#include <libmemcached/memcached.h>

#include <string>
using std::string;

class CCacheAgent
{
public:
	CCacheAgent();
	~CCacheAgent();
public:
	int Initialize(const string &strAddr);
	int Add(const string &strKey, const string &strValue, unsigned int dwExpiration = 0);
	int Set(const string &strKey, const string &strValue,  unsigned int dwExpiration = 0);
	int Get(const string & strKey, string &strValue, string &strCas);
	int Cas(const string &strKey, const string &strValue, const string &strCas, unsigned int dwExpiration = 0);
	int Delete(const string &strKey);
	string GetAddr();
private:
	string m_strCacheServer;
	memcached_st *m_pCacheMem;
};

#endif

