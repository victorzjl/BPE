#ifndef _C_ZOO_KEEPER_H_
#define _C_ZOO_KEEPER_H_

#include "zookeeperlib/zookeeper.h"
#include "TimerManager.h"
#include "IReConnHelp.h"
#include "IZooKeeperWatcher.h"

#include <string>
#include <map>

#define ALLOC_BUFFER_SIZE 1024
#define BUFFER_SIZE 1023

using namespace sdo::common;

class CZooKeeperReConnHelperImpl : public IReConnHelper, public IZooKeeperWatcher
{
public:
	int TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3);
	int ReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3) { return 0; }
	
public:
	std::string m_hosts;
	int m_timeout;
};

class CZooKeeper : public CTimerManager, public IReConnOwner
{
	friend void WatchFunction(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx);
	
public:
	CZooKeeper(IZooKeeperWatcher *pWatcher);
	CZooKeeper(IZooKeeperWatcher *pWatcher, const std::string& hosts, int timeout=3);
	virtual ~CZooKeeper();
	
	bool Initialize(const std::string& hosts, int timeout=3);
	bool Connect();
	bool Close();
	int OnReConn(void *pContext);
	
	std::string GetBrokerList(const std::string& topic);
	int GetPartitionNumber(const std::string& topic);
	
	bool CreateConfirmOffsetNode(const std::string &topic);
	bool ConfirmOffset(const std::string &topic, unsigned int partition, unsigned int offset);
	bool GetConfirmOffset(const std::string &topic, unsigned int partition, unsigned int& offset);
	
protected:
    void DoSelfCheck();
	bool UpdateBrokersInfo();
	bool UpdateTopicsInfo();
	void OnWatcherTriggered(zhandle_t *zh, int type, int state, const char *path);
	
private:
	IZooKeeperWatcher *m_pWatcher;
	zhandle_t *m_pZkHandle;
	std::string m_hosts;
	int m_timeout;
	std::multimap<std::string, int> m_mapTopicAndBrokerIds;
	std::map<std::string, int> m_mapTopicAndPartitionNumber;
	std::map<int, std::string> m_mapBrokerIdAndHost;
	
	char m_buffer[ALLOC_BUFFER_SIZE];
	int m_bufferSize;
	
	CThreadTimer m_timerSelfCheck;
	CZooKeeperReConnHelperImpl m_zookeeperConnHelper;
};

#endif //_C_ZOO_KEEPER_H_
