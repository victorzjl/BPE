#include "RedisHashAgentScheduler.h"
#include "RedisThread.h"
#include "RedisReConnThread.h"
#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisHashAgent.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisHelper.h"

using namespace redis;

int CRedisHashAgentScheduler::Get(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
    int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;

	string strNotFindIpAddrs = "";
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
		RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());
		if(strConHashIp.empty())
		{
			RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			nRet = GetFromSlave(strKey, mapFieldValue, strIpAddrs);
			return nRet;
		}

		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetFromSlave(strKey, mapFieldValue, strIpAddrs);
			return nRet;
		}

        CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Get(strKey, mapFieldValue);
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
			
			nRet = GetFromSlave(strKey, mapFieldValue, strIpAddrs);
		}
		else
		{
			strNotFindIpAddrs = strConHashIp;
			bNotFound = true;
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
		       RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisHashAgent pRedisHashAgent(pRedisAgent);
		    nRet = pRedisHashAgent.Get(strKey, mapFieldValue);
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
			else
			{
				CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
				bNotFound = true;
			}
			iter++;
		}

		if(!bNotFound)
		{
			nRet = GetFromSlave(strKey, mapFieldValue, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
        if(m_pRedisContainer->GetSlaveRedisAddr().size() == 0)
        {
        	strIpAddrs = strConnectErrorIpAddrs;
			nRet = RVS_REDIS_CONNECT_ERROR;
        }
	}
	return nRet;
}

int CRedisHashAgentScheduler::GetFromSlave(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
    const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
    if(mapSlaveRedisAddr.size() == 0)
   	{
    	return RVS_REDIS_CONNECT_ERROR;
    }
	
	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	string strNotFindIpAddrs = "";
	string strErrorIpAddrs = "";
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
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Get(strKey, mapFieldValue);
		if(nRet == RVS_SUCESS)
		{
			strIpAddrs = strAddr;
			return RVS_SUCESS;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strErrorIpAddrs, strAddr);
			pRedisThread->DelRedisServer(strAddr); 

			pRedisReConnThread->OnReConn(strAddr);
		}
		else
		{
			CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
            bNotFound = true;
		}

	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		return RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
		strIpAddrs = strErrorIpAddrs;
		return RVS_REDIS_CONNECT_ERROR;
	}
}

int CRedisHashAgentScheduler::Set(const string &strKey, const map<string, string> &mapFieldValue, unsigned int dwExpiration, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), dwExpiration);
	
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
            RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);		
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
			
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}
	
		if(!(iter->second) || pRedisAgent == NULL)
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
	
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Set(strKey, mapFieldValue, dwExpiration);
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
				 RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
                 iter++;
				 continue;
			}
				
			CRedisHashAgent pRedisHashAgent(pRedisAgent);
		    nRet = pRedisHashAgent.Set(strKey, mapFieldValue, dwExpiration);
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
	}
	else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;
}

int CRedisHashAgentScheduler::Delete(const string &strKey, const vector<string> &vecField, string &strIpAddrs)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
    int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bSuccess = false;

	string strSucIpAddrs = "";
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
		if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Delete(strKey, vecField);
		if(nRet == RVS_SUCESS)
		{
		    bSuccess = true;
			strSucIpAddrs = strConHashIp;
		}
		else
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp); 
			
			pRedisReConnThread->OnReConn(strConHashIp);
		}
	}
	else
	{
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
		       RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisHashAgent pRedisHashAgent(pRedisAgent);
		    nRet = pRedisHashAgent.Delete(strKey, vecField);
			if(nRet == RVS_SUCESS)
			{
			    bSuccess = true;
				CRedisHelper::AddIpAddrs(strSucIpAddrs, strAddr);
			}	
			else
			{
				CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
				pRedisThread->DelRedisServer(strAddr); 

				pRedisReConnThread->OnReConn(strAddr);
			}
			iter++;
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
	}
	else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}

	return nRet;
}

int CRedisHashAgentScheduler::Size(const string &strKey, int &size, string &strIpAddrs)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
    int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;

	string strNotFindIpAddrs = "";
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
		RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());
		if(strConHashIp.empty())
		{
			RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
		}

		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
            strIpAddrs = strConHashIp;
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
		}

        CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Size(strKey, size);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisHashAgent pRedisHashAgent(pRedisAgent);
		    nRet = pRedisHashAgent.Size(strKey, size);
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
        if(m_pRedisContainer->GetSlaveRedisAddr().size() == 0)
        {
        	strIpAddrs = strConnectErrorIpAddrs;
			nRet = RVS_REDIS_CONNECT_ERROR;
        }
	}
	return nRet;
}

int CRedisHashAgentScheduler::SizeFromSlave(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
	if(mapSlaveRedisAddr.size() == 0)
	{
		return RVS_REDIS_CONNECT_ERROR;
	}
			
	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	int bOperationFailed = false;
	
	string strNotFindIpAddrs = "";
	string strFailIpAddrs = "";
	string strConnectErrorIpAddrs = "";
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
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
					
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Size(strKey, size);
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

int CRedisHashAgentScheduler::GetAll(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
    int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;

	string strNotFindIpAddrs = "";
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
		RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());
		if(strConHashIp.empty())
		{
			RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			nRet = GetAllFromSlave(strKey, mapFieldValue, strIpAddrs);
			return nRet;
		}

		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetAllFromSlave(strKey, mapFieldValue, strIpAddrs);
			return nRet;
		}

        CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.GetAll(strKey, mapFieldValue);
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
			
			nRet = GetAllFromSlave(strKey, mapFieldValue, strIpAddrs);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisHashAgent pRedisHashAgent(pRedisAgent);
		    nRet = pRedisHashAgent.GetAll(strKey, mapFieldValue);
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
			nRet = GetAllFromSlave(strKey, mapFieldValue, strIpAddrs);
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
        if(m_pRedisContainer->GetSlaveRedisAddr().size() == 0)
        {
        	strIpAddrs = strConnectErrorIpAddrs;
			nRet = RVS_REDIS_CONNECT_ERROR;
        }
	}
	return nRet;
}

int CRedisHashAgentScheduler::GetAllFromSlave(const string &strKey, map<string, string> &mapFieldValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	const map<string, bool> & mapSlaveRedisAddr = m_pRedisContainer->GetSlaveRedisAddr();
	if(mapSlaveRedisAddr.size() == 0)
	{
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bNotFound = false;
	bool bOperationFailed = false;
	
	string strNotFindIpAddrs = "";
	string strFailIpAddrs = "";
	string strConnectErrorIpAddrs = "";
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
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
				
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.GetAll(strKey, mapFieldValue);
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
			bOperationFailed= true;
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

int CRedisHashAgentScheduler::Incby(const string &strKey, const string &strField, const int increment, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, Key[%s] Field[%s] Increment[%d]\n", __FUNCTION__, strKey.c_str(), strField.c_str(), increment);
	int nRet = RVS_REDIS_OPERATION_FAILED;
	bool bSuccess = false;
	bool bOperationFailed = false;
	
	string strSucIpAddrs = "";
	string strFailedIpAddrs = "";
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
		if(strConHashIp.empty())
		{
			RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
		}
			
		iter = mapRedisAddr.find(strConHashIp); 
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
			
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}
	
		if(!(iter->second) || pRedisAgent == NULL)
		{
			RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
	
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.Incby(strKey, strField, increment);
		if(nRet == RVS_SUCESS)
		{
			bSuccess = true;
			CRedisHelper::AddIpAddrs(strSucIpAddrs, strConHashIp);
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strConHashIp);
			pRedisThread->DelRedisServer(strConHashIp); 
				
			pRedisReConnThread->OnReConn(strConHashIp);
		}
		else
		{
		    bOperationFailed = true;
			CRedisHelper::AddIpAddrs(strFailedIpAddrs, strConHashIp);
		}
	}
	else
	{
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
				 RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
				 iter++;
				 continue;
			}
				
			CRedisHashAgent pRedisHashAgent(pRedisAgent);
			nRet = pRedisHashAgent.Incby(strKey, strField, increment);
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
			    bOperationFailed = true;
			    CRedisHelper::AddIpAddrs(strFailedIpAddrs, strAddr);
				break;
			}
			iter++;
		}
	}
		
	//////////////////////////////////////////////////////////////////////////
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
	}
	else if(bOperationFailed)
	{
		strIpAddrs = strFailedIpAddrs;
		nRet = RVS_SYSTEM_ERROR;
	}
	else
	{
	    strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}

	return nRet;
}

int CRedisHashAgentScheduler::BatchQuery(vector<SKeyFieldValue> &vecKFV, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, VectorSize[%d]\n", __FUNCTION__, vecKFV.size());

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
		     RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			 iter++;
			 continue;
		 }
			
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.BatchQuery(vecKFV);
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
		nRet = BatchQueryFromSlave(vecKFV, strIpAddrs);
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
		if(m_pRedisContainer->GetSlaveRedisAddr().size() == 0)
		{
			strIpAddrs = strConnectErrorIpAddrs;
			nRet = RVS_REDIS_CONNECT_ERROR;
		}
	}
	return nRet;
}

int CRedisHashAgentScheduler::BatchQueryFromSlave(vector<SKeyFieldValue> &vecKFV, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, VectorSize[%d]\n", __FUNCTION__, vecKFV.size());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.BatchQuery(vecKFV);
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
			CRedisHelper::AddIpAddrs(strConnectErrorIpAddrs, strAddr);
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

int CRedisHashAgentScheduler::BatchIncrby(vector<SKeyFieldValue> &vecKFV, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgentScheduler::%s, vectorSize[%d]\n", __FUNCTION__, vecKFV.size());

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
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
            iter++;
			continue;
		}
			
		CRedisHashAgent pRedisHashAgent(pRedisAgent);
		nRet = pRedisHashAgent.BatchIncrby(vecKFV);
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

