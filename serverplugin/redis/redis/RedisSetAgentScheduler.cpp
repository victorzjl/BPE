#include "RedisSetAgentScheduler.h"
#include "RedisThread.h"
#include "RedisReConnThread.h"
#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisSetAgent.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisHelper.h"

using namespace redis;

int CRedisSetAgentScheduler::HasMember(const string &strKey, const string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s] Value[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;

	string strNotFindIpAddrs = "";
	string strFailIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	CRedisAgent *pRedisAgent = NULL;
	const map<string, bool> & mapRedisAddr = m_pRedisContainer->GetMasterRedisAddr();
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter = mapRedisAddr.begin();
	
	if(m_pRedisContainer->IsConHash())
	{
	    CConHashUtil *pConHashUtil = m_pRedisContainer->GetConHashUtil();
		const string strConHashIp = pConHashUtil->GetConHashNode(strKey);
		RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = HasMemberFromSlave(strKey, strValue, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = HasMemberFromSlave(strKey, strValue, strIpAddrs);
			return nRet;
		}

        CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.HasMember(strKey, strValue);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = pRedisAgent->GetAddr();
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp);
			pRedisReConnThread->OnReConn(strConHashIp);
			
			nRet = HasMemberFromSlave(strKey, strValue, strIpAddrs);
		}
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			strNotFindIpAddrs = strConHashIp;
			bNotFound = true;
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
		}
	}
	else
	{
		while (iter != mapRedisAddr.end())
		{
			if(!(iter->second))
            {
                iter++;
            	continue;
            }
			const string &strAddr = iter->first;
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
            if(pRedisAgent == NULL)
		    {
		       RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisSetAgent pRedisSetAgent(pRedisAgent);
		    nRet = pRedisSetAgent.HasMember(strKey, strValue);
			if(nRet == RVS_SUCESS)
			{
				strIpAddrs = strAddr;
				return RVS_SUCESS;
			}
			else if(nRet == RVS_REDIS_CONNECT_ERROR)
			{
				CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr); 

				pRedisReConnThread->OnReConn(strAddr);
			}
			else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
			{
				CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
				bNotFound = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}

		if(!bNotFound && !bOperationFailed)
		{
			nRet = HasMemberFromSlave(strKey, strValue, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else if(bOperationFailed)
	{
        strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
	    if(m_pRedisContainer->GetMasterRedisAddr().size() == 0)
	    {
	    	strIpAddrs = strConnectErrorIpAddrs;
		    nRet = RVS_REDIS_CONNECT_ERROR;
	    }
	}
	return nRet;

}

int CRedisSetAgentScheduler::HasMemberFromSlave(const string &strKey, const string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s] Value[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
	const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
    if(mapSlaveRedisAddr.size() == 0)
   	{
    	return RVS_REDIS_CONNECT_ERROR;
    }
	
	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;
	string strNotFindIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter;
	for(iter = mapSlaveRedisAddr.begin(); iter != mapSlaveRedisAddr.end(); iter++)
	{
		if(!(iter->second))
		{
			continue;
		}
		
		const string &strAddr = iter->first;
		CRedisAgent *pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
		if(pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.HasMember(strKey, strValue);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = strAddr;
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 

			pRedisReConnThread->OnReConn(strAddr);
		}
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
            bNotFound = true;
		}
		else
		{
			CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
            bOperationFailed = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else if(bOperationFailed)
	{
		strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;

}

int CRedisSetAgentScheduler::Set(const string &strKey, const string &strValue, int &result, unsigned int dwExpiration, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s], Value[%s], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), dwExpiration);

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bSuccess = false;
	bool bOperationFailed = false;
	string strSucIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	CRedisAgent *pRedisAgent = NULL;
	const map<string, bool> & mapRedisAddr = m_pRedisContainer->GetMasterRedisAddr();
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter = mapRedisAddr.begin();
	
	if(m_pRedisContainer->IsConHash())
	{
	    CConHashUtil *pConHashUtil = m_pRedisContainer->GetConHashUtil();
		const string strConHashIp = pConHashUtil->GetConHashNode(strKey);
		if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.Set(strKey, strValue, result,dwExpiration);
		if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp);  //先更新线程的Redis连接状态，再进行重连
			
			pRedisReConnThread->OnReConn(strConHashIp);
		}
		else if(nRet == RVS_SUCESS)
		{
			strSucIpAddrs = strConHashIp;
			bSuccess = true;
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
		}
	}
	else
	{
		while (iter != mapRedisAddr.end())
		{
            if(!(iter->second))
            {
                iter++;
            	continue;
            }
			const string &strAddr = iter->first;
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
            if(pRedisAgent == NULL)
		    {
		       RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisSetAgent pRedisSetAgent(pRedisAgent);
			nRet = pRedisSetAgent.Set(strKey, strValue, result,dwExpiration);
			if(nRet == RVS_REDIS_CONNECT_ERROR)
			{
				CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr);

				pRedisReConnThread->OnReConn(strAddr);
			}
			else if(nRet == RVS_SUCESS)
			{
				CRedisHelper::AddIpAddrs(strSucIpAddrs, strAddr);
				bSuccess = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}
	}	

	//如果操作成功，则返回0，如果操作不成功，则返回连接失败
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
	}
	else if(bOperationFailed)
	{
		strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;
}

int CRedisSetAgentScheduler::Delete(const string &strKey, const string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s], Value[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bSuccess = false;
	bool bOperationFailed = false;
	string strSucIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	CRedisAgent *pRedisAgent = NULL;
	const map<string, bool> & mapRedisAddr = m_pRedisContainer->GetMasterRedisAddr();
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter = mapRedisAddr.begin();
	
	if(m_pRedisContainer->IsConHash())
	{
	    CConHashUtil *pConHashUtil = m_pRedisContainer->GetConHashUtil();
		const string strConHashIp = pConHashUtil->GetConHashNode(strKey);
		if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.Delete(strKey, strValue);
		if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp);  //先更新线程的Redis连接状态，再进行重连
			
			pRedisReConnThread->OnReConn(strConHashIp);
		}
		else if(nRet == RVS_SUCESS)
		{
			strSucIpAddrs = strConHashIp;
			bSuccess = true;
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
		}
	}
	else
	{
		while (iter != mapRedisAddr.end())
		{
            if(!(iter->second))
            {
                iter++;
            	continue;
            }
			const string &strAddr = iter->first;
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
            if(pRedisAgent == NULL)
		    {
		       RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisSetAgent pRedisSetAgent(pRedisAgent);
		    nRet = pRedisSetAgent.Delete(strKey, strValue);
			if(nRet == RVS_REDIS_CONNECT_ERROR)
			{
				CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr);

				pRedisReConnThread->OnReConn(strAddr);
			}
			else if(nRet == RVS_SUCESS)
			{
				CRedisHelper::AddIpAddrs(strSucIpAddrs, strAddr);
				bSuccess = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}
	}	

	//如果操作成功，则返回0，如果操作不成功，则返回连接失败
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
	}
	else if(bOperationFailed)
	{
		strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;

}

int CRedisSetAgentScheduler::Size(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;

	string strNotFindIpAddrs = "";
	string strFailIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	CRedisAgent *pRedisAgent = NULL;
	const map<string, bool> & mapRedisAddr = m_pRedisContainer->GetMasterRedisAddr();
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter = mapRedisAddr.begin();
	
	if(m_pRedisContainer->IsConHash())
	{
	    CConHashUtil *pConHashUtil = m_pRedisContainer->GetConHashUtil();
		const string strConHashIp = pConHashUtil->GetConHashNode(strKey);
		RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
		}

        CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.Size(strKey, size);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = pRedisAgent->GetAddr();
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp);
			pRedisReConnThread->OnReConn(strConHashIp);
			
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
		}
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			strNotFindIpAddrs = strConHashIp;
			bNotFound = true;
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
		}
	}
	else
	{
		while (iter != mapRedisAddr.end())
		{
			if(!(iter->second))
            {
                iter++;
            	continue;
            }
			const string &strAddr = iter->first;
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
            if(pRedisAgent == NULL)
		    {
		       RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisSetAgent pRedisSetAgent(pRedisAgent);
	    	nRet = pRedisSetAgent.Size(strKey, size);
			if(nRet == RVS_SUCESS)
			{
				strIpAddrs = strAddr;
				return RVS_SUCESS;
			}
			else if(nRet == RVS_REDIS_CONNECT_ERROR)
			{
				CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr); 

				pRedisReConnThread->OnReConn(strAddr);
			}
			else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
			{
				CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
				bNotFound = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}

		if(!bNotFound && !bOperationFailed)
		{
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else if(bOperationFailed)
	{
        strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
	    if(m_pRedisContainer->GetMasterRedisAddr().size() == 0)
	    {
	    	strIpAddrs = strConnectErrorIpAddrs;
		    nRet = RVS_REDIS_CONNECT_ERROR;
	    }
	}
	return nRet;

}

int CRedisSetAgentScheduler::SizeFromSlave(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
    if(mapSlaveRedisAddr.size() == 0)
   	{
    	return RVS_REDIS_CONNECT_ERROR;
    }
	
	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;
	string strNotFindIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter;
	for(iter = mapSlaveRedisAddr.begin(); iter != mapSlaveRedisAddr.end(); iter++)
	{
		if(!(iter->second))
		{
			continue;
		}
		
		const string &strAddr = iter->first;
		CRedisAgent *pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
		if(pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.Size(strKey, size);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = strAddr;
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 

			pRedisReConnThread->OnReConn(strAddr);
		}
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
            bNotFound = true;
		}
		else
		{
			CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
            bOperationFailed = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else if(bOperationFailed)
	{
		strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;

}

int CRedisSetAgentScheduler::GetAll(const string &strKey, vector<string> &vecValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;

	string strNotFindIpAddrs = "";
	string strFailIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	CRedisAgent *pRedisAgent = NULL;
	const map<string, bool> & mapRedisAddr = m_pRedisContainer->GetMasterRedisAddr();
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter = mapRedisAddr.begin();
	
	if(m_pRedisContainer->IsConHash())
	{
	    CConHashUtil *pConHashUtil = m_pRedisContainer->GetConHashUtil();
		const string strConHashIp = pConHashUtil->GetConHashNode(strKey);
		RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetAllFromSlave(strKey, vecValue, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetAllFromSlave(strKey, vecValue, strIpAddrs);
			return nRet;
		}

        CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.GetAll(strKey, vecValue);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = pRedisAgent->GetAddr();
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp);
			pRedisReConnThread->OnReConn(strConHashIp);
			
			nRet = GetAllFromSlave(strKey, vecValue, strIpAddrs);
		}
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			strNotFindIpAddrs = strConHashIp;
			bNotFound = true;
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
		}
	}
	else
	{
		while (iter != mapRedisAddr.end())
		{
			if(!(iter->second))
            {
                iter++;
            	continue;
            }
			const string &strAddr = iter->first;
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
            if(pRedisAgent == NULL)
		    {
		       RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisSetAgent pRedisSetAgent(pRedisAgent);
		    nRet = pRedisSetAgent.GetAll(strKey, vecValue);
			if(nRet == RVS_SUCESS)
			{
				strIpAddrs = strAddr;
				return RVS_SUCESS;
			}
			else if(nRet == RVS_REDIS_CONNECT_ERROR)
			{
				CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr); 

				pRedisReConnThread->OnReConn(strAddr);
			}
			else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
			{
				CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
				bNotFound = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}

		if(!bNotFound && !bOperationFailed)
		{
			nRet = GetAllFromSlave(strKey, vecValue, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else if(bOperationFailed)
	{
        strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
	    if(m_pRedisContainer->GetMasterRedisAddr().size() == 0)
	    {
	    	strIpAddrs = strConnectErrorIpAddrs;
		    nRet = RVS_REDIS_CONNECT_ERROR;
	    }
	}
	return nRet;

}

int CRedisSetAgentScheduler::GetAllFromSlave(const string &strKey, vector<string> &vecValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
    if(mapSlaveRedisAddr.size() == 0)
   	{
    	return RVS_REDIS_CONNECT_ERROR;
    }
	
	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;
	string strNotFindIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter;
	for(iter = mapSlaveRedisAddr.begin(); iter != mapSlaveRedisAddr.end(); iter++)
	{
		if(!(iter->second))
		{
			continue;
		}
		
		const string &strAddr = iter->first;
		CRedisAgent *pRedisAgent = pRedisThread->GetRedisAgentByAddr(strAddr);
		if(pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisSetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisSetAgent pRedisSetAgent(pRedisAgent);
		nRet = pRedisSetAgent.GetAll(strKey, vecValue);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = strAddr;
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 

			pRedisReConnThread->OnReConn(strAddr);
		}
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
            bNotFound = true;
		}
		else
		{
			CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
            bOperationFailed = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else if(bOperationFailed)
	{
		strIpAddrs = strFailIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;

}
