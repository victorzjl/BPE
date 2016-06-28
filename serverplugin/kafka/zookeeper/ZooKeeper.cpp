#include <boost/lexical_cast.hpp> 
#include <cstdio> 

#include "ZooKeeper.h"
#include "AsyncVirtualServiceLog.h"
#include "CJson.h"
#include "ReConnThread.h"

#define TOPICS_NODE "/brokers/topics"
#define BROKERS_NODE "/brokers/ids"
#define CONSUME_NODE "/consumers/sdg"


int CZooKeeperReConnHelperImpl::TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout)
{
	CZooKeeper zookeeper(this);
	zookeeper.Initialize(m_hosts, m_timeout);
	if (!zookeeper.Connect())
	{
		return -1;
	}
	
	return 0;
}

void WatchFunction(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
	ZK_XLOG(XLOG_DEBUG, "%s, zhandle_t[%p] type[%d] state[%d] path[%s] watcherCtx[%p].\n",
                     	  __FUNCTION__, zh, type, state, path, watcherCtx);
	if (NULL == watcherCtx)
	{
		ZK_XLOG(XLOG_WARNING, "%s, There is no event recevier.\n", __FUNCTION__);
		return;
	}
	
	CZooKeeper *pZooKeeper = (CZooKeeper *)watcherCtx;
	pZooKeeper->OnWatcherTriggered(zh, type, state, path);
}

CZooKeeper::CZooKeeper(IZooKeeperWatcher *pWatcher)
	: m_pWatcher(pWatcher),
	  m_pZkHandle(NULL)
{
	
}

CZooKeeper::CZooKeeper(IZooKeeperWatcher *pWatcher, const std::string& hosts, int timeout)
	: m_pWatcher(pWatcher),
	  m_pZkHandle(NULL),
	  m_timerSelfCheck(this, 3, boost::bind(&CZooKeeper::DoSelfCheck, this), CThreadTimer::TIMER_CIRCLE)
{
	Initialize(hosts, timeout);
}

CZooKeeper::~CZooKeeper()
{
	Close();
}

bool CZooKeeper::Initialize(const std::string& hosts, int timeout)
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, hosts[%s], timeout[%d].\n", __FUNCTION__, hosts.c_str(), timeout);
	
	m_hosts = hosts;
	m_timeout = timeout;
	m_zookeeperConnHelper.m_hosts = hosts;
	m_zookeeperConnHelper.m_timeout = timeout;
	return true;
}

bool CZooKeeper::Connect()
{
	if(m_hosts.empty())
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Hosts is empty.\n", __FUNCTION__);
		return false;
	}
	m_pZkHandle = zookeeper_init(m_hosts.c_str(), WatchFunction, m_timeout*1000, 0, (void *)this, 0);
	
    if (m_pZkHandle == NULL)
    {
        ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Can not get zookeeper handle from zookeeper servers. hosts[%s].\n", 
		                        __FUNCTION__, m_hosts.c_str());
		CReConnThread::GetReConnThread()->OnReConn(this, &m_zookeeperConnHelper, NULL);
        return false;
    }
		
	UpdateBrokersInfo();
	UpdateTopicsInfo();
	
	return true;
}


bool CZooKeeper::Close()
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s.\n", __FUNCTION__);
	if (m_pZkHandle)
	{
		zookeeper_close(m_pZkHandle);
		m_pZkHandle = NULL;
	}
	return true;
}

int CZooKeeper::OnReConn(void *pContext)
{
	if (Connect())
	{
		m_pWatcher->OnReConnSuccEvent(this);
	}
	
	return 0;
}

std::string CZooKeeper::GetBrokerList(const std::string& topic)
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, topic[%s].\n", __FUNCTION__, topic.c_str());
	
	std::string brokerList;
	typedef std::multimap<std::string, int>::iterator multimapIter;
	std::pair<multimapIter, multimapIter> ret = m_mapTopicAndBrokerIds.equal_range(topic);
	for (multimapIter iter=ret.first; iter!=ret.second; ++iter)
	{
		std::map<int, std::string>::iterator brokerIter = m_mapBrokerIdAndHost.find(iter->second);
		if (brokerIter == m_mapBrokerIdAndHost.end())
		{
			continue;
		}
		
		if (!brokerList.empty())
		{
			brokerList += ",";
		}
		brokerList += brokerIter->second;
	}
	
	if (brokerList.empty())
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, can not find broker list for topic '%s'.\n", __FUNCTION__, topic.c_str());
	}
	
	return brokerList;
}

int CZooKeeper::GetPartitionNumber(const std::string& topic)
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, topic[%s].\n", __FUNCTION__, topic.c_str());
	std::map<std::string, int>::iterator iter = m_mapTopicAndPartitionNumber.find(topic);
	if (iter != m_mapTopicAndPartitionNumber.end())
	{
		return iter->second;
	}
	else
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, can not find partition number for topic '%s'.\n", __FUNCTION__, topic.c_str());
		return 0;
	}
}

bool CZooKeeper::CreateConfirmOffsetNode(const std::string &topic)
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, topic[%s].\n", __FUNCTION__, topic.c_str());
	if (NULL == m_pZkHandle)
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Connection is closed.\n", __FUNCTION__);
		CReConnThread::GetReConnThread()->OnReConn(this, &m_zookeeperConnHelper, NULL);
		return false;
	}
	
	int ret = zoo_create(m_pZkHandle, CONSUME_NODE, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
	if ((ZOK != ret) && (ZNODEEXISTS != ret))
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Failed to create node '%s'. Code[%d]\n", __FUNCTION__, CONSUME_NODE, ret);
		return false;
	}
	
	std::string topicPath(CONSUME_NODE"/");
	topicPath += topic;
	ret = zoo_create(m_pZkHandle, topicPath.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
	if ((ZOK != ret) && (ZNODEEXISTS != ret))
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Failed to create node '%s'. Code[%d]\n", __FUNCTION__, topicPath.c_str(), ret);
		return false;
	}
	
	std::map<std::string, int>::iterator iter = m_mapTopicAndPartitionNumber.find(topic);
	if (iter == m_mapTopicAndPartitionNumber.end())
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Can't find topic '%s'\n", __FUNCTION__, topic.c_str());
		return false;
	}
	
	topicPath += "/";
	
	for (int iIter=0; iIter<iter->second; ++iIter)
	{
		zoo_create(m_pZkHandle, (topicPath+boost::lexical_cast<std::string>(iIter)).c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
	}
	return true;
}

bool CZooKeeper::ConfirmOffset(const std::string &topic, unsigned int partition, unsigned int offset)
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, topic[%s] partition[%u] offset[%u].\n", __FUNCTION__, topic.c_str(), partition, offset);
	if (NULL == m_pZkHandle)
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Connection is closed.\n", __FUNCTION__);
		CReConnThread::GetReConnThread()->OnReConn(this, &m_zookeeperConnHelper, NULL);
		return false;
	}
	
	std::string partitionPath(CONSUME_NODE"/");
	partitionPath += topic + "/" + boost::lexical_cast<std::string>(partition);
	std::string data = boost::lexical_cast<std::string>(offset);
	
	int ret = zoo_set(m_pZkHandle, partitionPath.c_str(), data.c_str(), data.length(), -1);
	if (ZOK == ret)
	{
		return true;
	}
	
	if (ZNONODE == ret)
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Node '%s' is not exist\n", __FUNCTION__, partitionPath.c_str());
		ret = zoo_create(m_pZkHandle, partitionPath.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
		if (ZOK != ret)
		{
			ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Connection is closed.\n", __FUNCTION__);
			return false;
		}
		
		ret = zoo_set(m_pZkHandle, partitionPath.c_str(), data.c_str(), data.length(), -1);
		if (ZOK != ret)
		{
			ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Failed to create node '%s'. Code[%d]\n", __FUNCTION__, partitionPath.c_str(), ret);
			return false;
		}
		return true;
	}
	else
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Failed to set date for node '%s'. Code[%d]\n", __FUNCTION__, partitionPath.c_str(), ret);
		return false;
	}
}

bool CZooKeeper::GetConfirmOffset(const std::string &topic, unsigned int partition, unsigned int& offset)
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, topic[%s] partition[%u] offset[%u].\n", __FUNCTION__, topic.c_str(), partition, offset);
	if (NULL == m_pZkHandle)
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Connection is closed.\n", __FUNCTION__);
		CReConnThread::GetReConnThread()->OnReConn(this, &m_zookeeperConnHelper, NULL);
		return false;
	}
	
	std::string partitionPath(CONSUME_NODE"/");
	partitionPath += topic + "/" + boost::lexical_cast<std::string>(partition);
	
	m_bufferSize = BUFFER_SIZE;
	int ret = zoo_get(m_pZkHandle, partitionPath.c_str(), 0, m_buffer, &m_bufferSize, NULL);
	if (ZOK == ret)
	{
		m_buffer[m_bufferSize] = '\0';
		offset = boost::lexical_cast<int>(m_buffer);
		return true;
	}
	return false;
}

void CZooKeeper::OnWatcherTriggered(zhandle_t *zh, int type, int state, const char *path)
{
	ZK_XLOG(XLOG_DEBUG, "CZooKeeper::%s\n", __FUNCTION__);
	if ((zh != m_pZkHandle) || (NULL == m_pWatcher))
	{
		ZK_XLOG(XLOG_WARNING, "CZooKeeper::%s, Can't handle the event, zh[%p] m_pZkHandle[%p] m_pWatcher[%p].\n",
                        		__FUNCTION__, zh, m_pZkHandle, m_pWatcher);
		return;
	}
	
	if ((ZOO_EXPIRED_SESSION_STATE == state)
	    || (ZOO_AUTH_FAILED_STATE == state)
		)
	{
		CReConnThread::GetReConnThread()->OnReConn(this, &m_zookeeperConnHelper, NULL);
	}
	
	if (ZOO_CREATED_EVENT == type)
	{
		m_pWatcher->OnCreatedEvent(this, path);
	}
	else if (ZOO_DELETED_EVENT == type)
	{
		m_pWatcher->OnDeletedEvent(this, path);
	}
	else if (ZOO_CHANGED_EVENT == type)
	{
		m_pWatcher->OnChangedEvent(this, path);
	}
	else if (ZOO_CHILD_EVENT == type)
	{
		m_pWatcher->OnChildEvent(this, path);
	}
	else if (ZOO_SESSION_EVENT == type)
	{
		m_pWatcher->OnSessionEvent(this, path);
	}
	else if (ZOO_NOTWATCHING_EVENT == type)
	{
		m_pWatcher->OnNotWatchingEvent(this, path);
	}
	else 
	{
		//TODO
	}
}


void CZooKeeper::DoSelfCheck()
{
	m_bufferSize = BUFFER_SIZE;
	int ret = zoo_get(m_pZkHandle, "/", 0, m_buffer, &m_bufferSize, NULL);
	if ((ZOPERATIONTIMEOUT == ret)
		|| (ZINVALIDSTATE == ret)
		|| (ZSESSIONEXPIRED == ret)
		|| (ZAUTHFAILED == ret)
		|| (ZCLOSING == ret)
		|| (ZNOTHING == ret))
	{
		Close();
		CReConnThread::GetReConnThread()->OnReConn(this, &m_zookeeperConnHelper, NULL);
		m_timerSelfCheck.Stop();
	}
}

bool CZooKeeper::UpdateBrokersInfo()
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s.\n", __FUNCTION__);
	
	m_mapBrokerIdAndHost.clear();
	
	struct String_vector vecBrokers;
	int ret = zoo_get_children(m_pZkHandle, BROKERS_NODE, 0, &vecBrokers);
	if ( ZOK != ret )
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Can not get children of node '"BROKERS_NODE"'. code[%d].\n", __FUNCTION__, ret);
		return false;
	}
	
	for (int iIter=0; iIter<vecBrokers.count; ++iIter)
	{
		int brokerId = boost::lexical_cast<int>(vecBrokers.data[iIter]);
		std::string brokerNodePath(BROKERS_NODE"/");
		brokerNodePath += vecBrokers.data[iIter];
		
		m_bufferSize = BUFFER_SIZE;
		ret = zoo_get(m_pZkHandle, brokerNodePath.c_str(), 0, m_buffer, &m_bufferSize, NULL);
		if ( ZOK != ret)
		{
			ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Can not get data for node '%s'. code[%d]\n", __FUNCTION__, brokerNodePath.c_str(), ret);
		}
		else
		{
			m_buffer[m_bufferSize] = '\0';
			ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, Get data for node '%s'. data[%s]\n", __FUNCTION__, brokerNodePath.c_str(), m_buffer);
			
			CJsonDecoder jsonDecoder(m_buffer, m_bufferSize);
			std::string host, port;
			if ( (0 != jsonDecoder.GetValue("host", host)) || (0 != jsonDecoder.GetValue("port", port)))
			{
				ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, Failed to get host or port for broker '%s' from data.\n", __FUNCTION__, vecBrokers.data[iIter]);
				continue;
			}

			m_mapBrokerIdAndHost[brokerId] = host + ":" + port;
		}
	}
	
	return true;
}

bool CZooKeeper::UpdateTopicsInfo()
{
	ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s.\n", __FUNCTION__);
	
	m_mapTopicAndBrokerIds.clear();
	m_mapTopicAndPartitionNumber.clear();
	
	struct String_vector vecTopics;
	int ret = zoo_get_children(m_pZkHandle, TOPICS_NODE, 0, &vecTopics);
	if ( ZOK != ret )
	{
		ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Can not get children of node '"TOPICS_NODE"'. code[%d].\n", __FUNCTION__, ret);
		return false;
	}
	
	for (int iIter=0; iIter<vecTopics.count; ++iIter)
	{
		std::string topicNodePath(TOPICS_NODE"/");
		topicNodePath += vecTopics.data[iIter];
		
		m_bufferSize = BUFFER_SIZE;
		ret = zoo_get(m_pZkHandle, topicNodePath.c_str(), 0, m_buffer, &m_bufferSize, NULL);
		if ( ZOK != ret)
		{
			ZK_XLOG(XLOG_WARNING, "Zookeeper::%s, Can not get data for node '%s'. code[%d]\n", __FUNCTION__, topicNodePath.c_str(), ret);
		}
		else
		{
			m_buffer[m_bufferSize] = '\0';
			ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, Get data for node '%s'. data[%s]\n", __FUNCTION__, topicNodePath.c_str(), m_buffer);
			CJsonDecoder jsonDecoder(m_buffer, m_bufferSize);
			CJsonDecoder partitionsJsonDecoder;
			if (0 != jsonDecoder.GetValue("partitions", partitionsJsonDecoder))
			{
				ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, Failed to get broker ids for topic '%s' from data.\n", __FUNCTION__, vecTopics.data[iIter]);
			}
			else
			{
				std::vector<int> vecBrokerIds;
				if (0 != partitionsJsonDecoder.GetValue("0", vecBrokerIds))
				{
					ZK_XLOG(XLOG_DEBUG, "Zookeeper::%s, Failed to get broker ids for topic '%s' from data.\n", __FUNCTION__, vecTopics.data[iIter]);
				}
				else
				{
					for (std::vector<int>::iterator iter=vecBrokerIds.begin(); iter!=vecBrokerIds.end(); ++iter)
					{
						m_mapTopicAndBrokerIds.insert(std::make_pair(vecTopics.data[iIter], *iter));
					}
				}
			}
		}
		
		topicNodePath += "/partitions";
		struct String_vector vecPartitions;
		ret = zoo_get_children(m_pZkHandle, topicNodePath.c_str(), 0, &vecPartitions);
		m_mapTopicAndPartitionNumber[vecTopics.data[iIter]] = vecPartitions.count;
	}
	
	return true;
}

