#ifndef _CACHE_GROUP_H_
#define _CACHE_GROUP_H_

#include "CacheAgent.h"
#include "ConHashUtil.h"
#include <vector>
#include <string>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::make_pair;

class CCacheThread;
class CCacheReConnThread;
class CCacheContainer
{
public:
	CCacheContainer(vector<unsigned int> dwVecServiceId, bool bConHash, const vector<string> &vecStrCacheServer, CCacheThread *pCacheThread, CCacheReConnThread *pCacheReConnThread);
	~CCacheContainer();
public:
	int Add(const string &strKey, const string &strValue, unsigned int dwExpiration, string &strIpAddrs);
	int Set(const string &strKey, const string &strValue, unsigned int dwExpiration, string &strIpAddrs);
	int Get(const string &strKey, string &strValue, string &strIpAddrs, string &strCas);
	int Cas(const string & strKey,const string & strValue,unsigned int dwExpiration, const string &strCas, string &strIpAddrs);
	int Delete(const string &strKey, string &strIpAddrs);
	string GetServiceIdString();
public:
	int AddCacheServer(const string &strAddr);
private:
	void AddIpAddrs(string &strIpAddrs, CCacheAgent *pCacheAgent);
private:
	vector<CCacheAgent *> m_pVecCacheUtil;
	vector<unsigned int> m_dwVecServiceId;
	CCacheThread *m_pCacheThread;
	CCacheReConnThread *m_pCacheReConnThread;
	bool m_bConHash;
	CConHashUtil *m_pConHashUtil;
	map<string, int> m_mapConVirtualNum;
};

#endif

