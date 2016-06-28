#ifndef _MQS_THREAD_GROUP_H_
#define _MQS_THREAD_GROUP_H_

#include <MsgQueuePrio.h>

#include "IZooKeeperWatcher.h"
#include <string>
#include <vector>
#include <map>
#include <detail/_time.h>

using namespace sdo::common;

class CMQSVirtualService;
class CMQSThread;
class CMQSThreadGroup : public IZooKeeperWatcher
{
public:
	CMQSThreadGroup(CMQSVirtualService *pMQSVirtualService):m_pMQSVirtualService(pMQSVirtualService){}
	virtual ~CMQSThreadGroup();

	int Start( unsigned int dwThreadNum, const std::vector<std::string> &vecMQSenderSvr, std::vector<unsigned int>& vecServiceIds );
	void Stop();
	void GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum );
	
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, 
	               const std::string &strGuid, void *pHandler, const void *pBuffer, int nLen);

	void Dump();
	
	//response/warn/log
	void Response(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, 
	              unsigned int dwSequenceId, int nCode);
	void Warn(const std::string &strWarnInfo);
	void Log(const std::string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, 
	         const timeval_a &tStart, int nCode, const std::string &strIpAddrs);
	
	
	//ZooKeeper Call Back
	int OnCreatedEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	int OnDeletedEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	int OnChangedEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	int OnChildEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	int OnSessionEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	int OnNotWatchingEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	int OnReConnSuccEvent(CZooKeeper *pZooKeeper);
	
private:
	CMQSVirtualService           *m_pMQSVirtualService;
	sdo::common::CMsgQueuePrio    m_queue;
	std::vector<CMQSThread *>     m_vecChildThread;
	std::vector<unsigned int>     m_vecServiceIds;
	std::vector<CZooKeeper *>     m_vecZooKeeper;
	std::map< CZooKeeper*, std::vector<std::string> > m_mapInoperativeConfig;
};

#endif //_MQS_THREAD_GROUP_H_
