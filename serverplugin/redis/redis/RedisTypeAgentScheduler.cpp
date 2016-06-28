#include "RedisTypeAgentScheduler.h"
#include "RedisThread.h"
#include "RedisReConnThread.h"
#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisHashAgent.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisHelper.h"

using namespace redis;

int CRedisTypeAgentScheduler::Ttl(const string &strKey, int &ttl, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
    int nRet = RVS_REDIS_CONNECT_ERROR;
	
	CRedisAgent *pRedisAgent = NULL;
	const map<string, bool> & mapRedisAddr = m_pRedisContainer->GetMasterRedisAddr();
	CRedisThread *pRedisThread = m_pRedisContainer->GetRedisThread();
	CRedisReConnThread *pRedisReConnThread = m_pRedisContainer->GetRedisReConnThread();
	map<string, bool>::const_iterator iter = mapRedisAddr.begin();
	   
	if(m_pRedisContainer->IsConHash())
	{
		CConHashUtil *pConHashUtil = m_pRedisContainer->GetConHashUtil();
		const string strConHashIp = pConHashUtil->GetConHashNode(strKey);
		RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());
	    if(strConHashIp.empty())
		{
			 RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			 nRet = GetTtlFromSlave(strKey, ttl, strIpAddrs);
			 return nRet;
		}
	
		iter = mapRedisAddr.find(strConHashIp); 
		if(iter == mapRedisAddr.end())
		{
			 RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			 return RVS_SYSTEM_ERROR;
		}
		   
		if(iter->second)
	    {
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}
	
		if(!(iter->second) || pRedisAgent == NULL)
		{
			RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
		    nRet = GetTtlFromSlave(strKey, ttl, strIpAddrs);
			return nRet;
		}
	
		CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
		nRet = pRedisTypeAgent.Ttl(strKey, ttl);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = strConHashIp;
			if(ttl == -2)
			{
				return RVS_REDIS_KEY_NOT_FOUND;
			}
			else if(ttl == -1 || ttl == 0)
			{
				return RVS_REDIS_NOT_SET_EXPIRE_TIME;
			}
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strIpAddrs, strConHashIp);
			pRedisThread->DelRedisServer(strConHashIp);
			pRedisReConnThread->OnReConn(strConHashIp);
			   
			nRet = GetTtlFromSlave(strKey, ttl, strIpAddrs);
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
			    RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
				iter++;
				continue;
			}
			   
			CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
		    nRet = pRedisTypeAgent.Ttl(strKey, ttl);
			if(nRet == RVS_SUCESS)
			{
				strIpAddrs = strAddr;
				if(ttl == -2)
				{
					return RVS_REDIS_KEY_NOT_FOUND;
				}
				else if(ttl == -1 || ttl == 0)
				{
					return RVS_REDIS_NOT_SET_EXPIRE_TIME;
				}
				return RVS_SUCESS;
		    }
		    else if(nRet == RVS_REDIS_CONNECT_ERROR)
			{
				CRedisHelper::AddIpAddrs(strIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr); 
	
				pRedisReConnThread->OnReConn(strAddr);
			}
			iter++;
	    }
		
	    nRet = GetTtlFromSlave(strKey, ttl, strIpAddrs);
    }
	
    //////////////////////////////////////////////////////////////////////////
    return nRet;

}

int CRedisTypeAgentScheduler::GetTtlFromSlave(const string &strKey, int &ttl, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
	if(mapSlaveRedisAddr.size() == 0)
	{
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int nRet = RVS_REDIS_CONNECT_ERROR;
	string strErrorIpAddrs ="";
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
			RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
				
		CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
		nRet = pRedisTypeAgent.Ttl(strKey, ttl);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = strAddr;
			if(ttl == -2)
			{
				return RVS_REDIS_KEY_NOT_FOUND;
			}
			else if(ttl == -1 || ttl == 0)
			{
				return RVS_REDIS_NOT_SET_EXPIRE_TIME;
			}
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 
	
			pRedisReConnThread->OnReConn(strAddr);
		}
	
	}
	
	//////////////////////////////////////////////////////////////////////////
	strIpAddrs = strErrorIpAddrs;
	return RVS_REDIS_CONNECT_ERROR;

}

int CRedisTypeAgentScheduler::DeleteAll(const string &strKey, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

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
            RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
		nRet = pRedisTypeAgent.Del(strKey);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
			nRet = pRedisTypeAgent.Del(strKey);
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

int CRedisTypeAgentScheduler::BatchExpire(vector<SKeyValue> &vecKeyValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, vectorSize[%d]\n", __FUNCTION__, vecKeyValue.size());

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
	
	while(iter != mapRedisAddr.end())
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
		    RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
            iter++;
			continue;
		}
			
		CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
		nRet = pRedisTypeAgent.BatchExpire(vecKeyValue);
		if(nRet == RVS_SUCESS)
		{
			bSuccess = true;
			CRedisHelper::AddIpAddrs(strSucIpAddrs, strAddr);
		}	
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 

			pRedisReConnThread->OnReConn(strAddr);
		}
		else
		{
			CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
			bOperationFailed = true;
		}
		iter++;
	}
	
	//////////////////////////////////////////////////////////////////////////
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
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

int CRedisTypeAgentScheduler::BatchDelete(vector<SKeyValue> &vecKeyValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgentScheduler::%s, vectorSize[%d]\n", __FUNCTION__, vecKeyValue.size());

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
	
	while(iter != mapRedisAddr.end())
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
		    RVS_XLOG(XLOG_ERROR, "CRedisTypeAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
            iter++;
			continue;
		}
			
		CRedisTypeAgent pRedisTypeAgent(pRedisAgent);
		nRet = pRedisTypeAgent.BatchDelete(vecKeyValue);
		if(nRet == RVS_SUCESS)
		{
			bSuccess = true;
			CRedisHelper::AddIpAddrs(strSucIpAddrs, strAddr);
		}	
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 

			pRedisReConnThread->OnReConn(strAddr);
		}
		else
		{
			CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
			bOperationFailed = true;
		}
		iter++;
	}
	
	//////////////////////////////////////////////////////////////////////////
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
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

