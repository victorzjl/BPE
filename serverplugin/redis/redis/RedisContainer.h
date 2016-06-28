#ifndef _REDIS_CONTAINER_H_
#define _REDIS_CONTAINER_H_

#include "RedisThread.h"
#include "ConHashUtil.h"
#include <vector>
#include <string>
#include <map>

using namespace redis;
using std::string;
using std::vector;
using std::map;
using std::make_pair;

class CRedisReConnThread;

class CRedisContainer
{
public:
    CRedisContainer(unsigned int dwServiceId, bool bConHash, const map<string, bool> &mapRedisServer, const map<string, bool> &mapSlaveRedisServer, CRedisThread *pRedisThread, CRedisReConnThread *pRedisReConnThread);
	~CRedisContainer();
public:
	void AddRedisServer(const string &strAddr);
	void DelRedisServer(const string &strAddr);
	const bool IsConHash() const{return m_bConHash;}
	CConHashUtil * GetConHashUtil() {return m_pConHashUtil;}
	CRedisThread * GetRedisThread(){ return m_pRedisThread;}
	CRedisReConnThread *GetRedisReConnThread(){return m_pRedisReConnThread;}
	const map<string, bool> & GetMasterRedisAddr() const{ return m_mapRedisAddr;}
	const map<string, bool> & GetSlaveRedisAddr() const{return m_mapSlaveRedisAddr;}
private:
	map<string, bool> m_mapRedisAddr;
	unsigned int m_dwServiceId;
	CRedisThread *m_pRedisThread;
	CRedisReConnThread *m_pRedisReConnThread;
	bool m_bConHash;
	CConHashUtil *m_pConHashUtil;
	map<string, int> m_mapConVirtualNum;

	map<string, bool> m_mapSlaveRedisAddr; //只在读取策略为优先读取主服务器时有用

};
#endif
