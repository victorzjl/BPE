#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisReConnThread.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"

CRedisContainer::CRedisContainer(unsigned int dwServiceId, bool bConHash, const map<string, bool> &mapRedisServer, const map<string, bool> &mapSlaveRedisServer, CRedisThread *pRedisThread, CRedisReConnThread *pRedisReConnThread):
	m_dwServiceId(dwServiceId),m_pRedisThread(pRedisThread), m_pRedisReConnThread(pRedisReConnThread), m_pConHashUtil(NULL)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, ServiceId[%d], ConHash[%d]\n", __FUNCTION__, dwServiceId, bConHash);
	m_bConHash = bConHash;
	if(m_bConHash)
	{
		m_pConHashUtil = new CConHashUtil;
		if(!m_pConHashUtil->Init())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisContainer::%s, ConHashUtil Init Fail, ServiceId[%d]\n", __FUNCTION__, dwServiceId);
			return;
		}
	}

    //加入主地址
	map<string, bool>::const_iterator iter;
	for(iter = mapRedisServer.begin(); iter != mapRedisServer.end(); iter++)
	{
		string strIpPort;
		int nConHashVirtualNum = 1;
		if(m_bConHash)
		{
			char szIpPort[32] = {0};
			sscanf(iter->first.c_str(), "%[^#]#%d", szIpPort, &nConHashVirtualNum);
			strIpPort = szIpPort;
			m_mapConVirtualNum.insert(make_pair(strIpPort, nConHashVirtualNum));
		}
		else
		{
			strIpPort = iter->first;
		}
        RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, Master RedisServerIP[%s] Status[%d]\n", __FUNCTION__, strIpPort.c_str(), iter->second);
		m_mapRedisAddr.insert(make_pair(strIpPort, iter->second));
			
		if(m_bConHash && iter->second)
		{
			m_pConHashUtil->AddNode(strIpPort, nConHashVirtualNum);
		}
		
	}

    //加入从地址
	for(iter = mapSlaveRedisServer.begin(); iter != mapSlaveRedisServer.end(); iter++)
	{
		string strIpPort = iter->first;
		size_t npos = strIpPort.find("#");
		if(npos != string::npos)
		{
			strIpPort = strIpPort.substr(0, npos);
		}
		RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, Slave RedisServerIP[%s] Status[%d]\n", __FUNCTION__, strIpPort.c_str(), iter->second);
		m_mapSlaveRedisAddr.insert(make_pair(strIpPort, iter->second));
	}
}

CRedisContainer::~CRedisContainer()
{
	if(m_pConHashUtil)
	{
		delete m_pConHashUtil;
		m_pConHashUtil = NULL;
	}
}

void CRedisContainer::AddRedisServer( const string &strAddr )
{
    RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, strAddr[%s]\n", __FUNCTION__, strAddr.c_str());
    map<string, bool>::const_iterator iter = m_mapRedisAddr.find(strAddr);
    if(iter != m_mapRedisAddr.end())
    {
        if(!(iter->second))
        {
        	m_mapRedisAddr[strAddr] = true;

		    if(m_bConHash)
		    {
			    m_pConHashUtil->AddNode(strAddr, m_mapConVirtualNum[strAddr]);
		    }
		    RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, Add RedisServer[%s] Success\n", __FUNCTION__, strAddr.c_str());
        }
		else
		{
			RVS_XLOG(XLOG_WARNING, "CRedisContainer::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
		}      
   	}
    
	
	iter = m_mapSlaveRedisAddr.find(strAddr);
	if(iter != m_mapSlaveRedisAddr.end())
    {
        if(!(iter->second))
        {
        	m_mapSlaveRedisAddr[strAddr] = true;
		    RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, m_mapSlaveRedisAddr Add RedisServer[%s] Success\n", __FUNCTION__, strAddr.c_str());
        }
		else
		{	
	        RVS_XLOG(XLOG_WARNING, "CRedisContainer::%s, m_mapSlaveRedisAddr RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
		}
   	}
}

void CRedisContainer::DelRedisServer(const string &strAddr)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisContainer::%s, strAddr[%s]\n", __FUNCTION__, strAddr.c_str());

	map<string, bool>::const_iterator iter = m_mapRedisAddr.find(strAddr);
	if(iter != m_mapRedisAddr.end())
	{
		m_mapRedisAddr[strAddr] = false;
		if(m_bConHash)
		{
			m_pConHashUtil->DelNode(strAddr);
		}
	}

	iter = m_mapSlaveRedisAddr.find(strAddr);
	if(iter != m_mapSlaveRedisAddr.end())
	{
		m_mapSlaveRedisAddr[strAddr] = false;
	}
}

