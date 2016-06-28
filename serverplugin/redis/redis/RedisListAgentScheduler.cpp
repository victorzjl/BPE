#include "RedisListAgentScheduler.h"
#include "RedisThread.h"
#include "RedisReConnThread.h"
#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisListAgent.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisHelper.h"

using namespace redis;

int CRedisListAgentScheduler::Get(const string &strKey, string &strValue, const int index, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s] index[%d]\n", __FUNCTION__, strKey.c_str(), index);

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
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetFromSlave(strKey, strValue, index, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetFromSlave(strKey, strValue, index, strIpAddrs);
			return nRet;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.Get(strKey, strValue, index);
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
			
			nRet = GetFromSlave(strKey, strValue, index, strIpAddrs);
		}
		else if(nRet = RVS_REDIS_LIST_INDEX_OUT_RANGE)
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.Get(strKey, strValue, index);
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
			else if(nRet == RVS_REDIS_LIST_INDEX_OUT_RANGE)
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
			nRet = GetFromSlave(strKey, strValue, index, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_LIST_INDEX_OUT_RANGE;
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

int CRedisListAgentScheduler::GetFromSlave(const string &strKey, string &strValue, const int index, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s] index[%d]\n", __FUNCTION__, strKey.c_str(), index);
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
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisListAgent pRedisListAgent(pRedisAgent);
	    nRet = pRedisListAgent.Get(strKey, strValue, index);
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
		else if(nRet = RVS_REDIS_LIST_INDEX_OUT_RANGE)
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
		nRet = RVS_REDIS_LIST_INDEX_OUT_RANGE;
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

int CRedisListAgentScheduler::Set(const string &strKey, const string &strValue, const int index, unsigned int dwExpiration, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s] Value[%s] Index[%d] Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), index, dwExpiration);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.Set(strKey, strValue, index);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.Set(strKey, strValue, index);
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

int CRedisListAgentScheduler::Delete(const string &strKey, const string &strValue, const int count, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s] Value[%s] Count[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), count);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.Delete(strKey, strValue, count);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.Delete(strKey, strValue, count);
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

int CRedisListAgentScheduler::Size(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s] index[%d]\n", __FUNCTION__, strKey.c_str(), index);

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
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.Size(strKey, size);
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
		else if(nRet = RVS_REDIS_KEY_NOT_FOUND)
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.Size(strKey, size);
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

int CRedisListAgentScheduler::SizeFromSlave(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.Size(strKey, size);
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
		else if(nRet = RVS_REDIS_KEY_NOT_FOUND)
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

int CRedisListAgentScheduler::GetAll(const string &strKey, vector<string> &vecValue, const int start, const int end, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s] index[%d]\n", __FUNCTION__, strKey.c_str(), index);

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
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetAllFromSlave(strKey, vecValue, start, end, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetAllFromSlave(strKey, vecValue, start, end, strIpAddrs);
			return nRet;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.GetAll(strKey, vecValue, start, end);
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
			
			nRet = GetAllFromSlave(strKey, vecValue, start, end, strIpAddrs);
		}
		else if(nRet = RVS_REDIS_KEY_NOT_FOUND)
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.GetAll(strKey, vecValue, start, end);
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
			nRet = GetAllFromSlave(strKey, vecValue, start, end, strIpAddrs);
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

int CRedisListAgentScheduler::GetAllFromSlave(const string &strKey, vector<string> &vecValue, const int start, const int end, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.GetAll(strKey, vecValue, start, end);
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
		else if(nRet = RVS_REDIS_KEY_NOT_FOUND)
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

int CRedisListAgentScheduler::PopBack(const string &strKey, string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s\n", __FUNCTION__, strKey.c_str());

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bSuccess = false;
	bool bOperationFailed = false;
	bool bNotFind = false;
	
	string strSucIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	string strNotFindIpAddrs = "";
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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.PopBack(strKey, strValue);
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
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			strNotFindIpAddrs = strConHashIp;
			bNotFind = true;
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.PopBack(strKey, strValue);
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
			else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
			{
				CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
				bNotFind = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}
	}	

	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
	}
	else if(bNotFind)
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

int CRedisListAgentScheduler::PushBack(const string &strKey, const string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s\n", __FUNCTION__, strKey.c_str());

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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent, m_pRedisVirtualService);
		nRet = pRedisListAgent.PushBack(strKey, strValue);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent, m_pRedisVirtualService);
		    nRet = pRedisListAgent.PushBack(strKey, strValue);
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

int CRedisListAgentScheduler::PopFront(const string &strKey, string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s\n", __FUNCTION__, strKey.c_str());

	int nRet = RVS_REDIS_CONNECT_ERROR;
	bool bSuccess = false;
	bool bOperationFailed = false;
	bool bNotFind = false;
	
	string strSucIpAddrs = "";
	string strConnectErrorIpAddrs = "";
	string strFailIpAddrs = "";
	string strNotFindIpAddrs = "";
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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.PopFront(strKey, strValue);
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
		else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
		{
			strNotFindIpAddrs = strConHashIp;
			bNotFind = true;
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.PopFront(strKey, strValue);
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
			else if(nRet == RVS_REDIS_KEY_NOT_FOUND)
			{
				CRedisHelper::AddIpAddrs(strNotFindIpAddrs, strAddr);
				bNotFind = true;
			}
			else
			{
				CRedisHelper::AddIpAddrs(strFailIpAddrs, strAddr);
				bOperationFailed = true;
			}
			iter++;
		}
	}	

	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		nRet = RVS_SUCESS;
	}
	else if(bNotFind)
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

int CRedisListAgentScheduler::PushFront(const string &strKey, const string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s\n", __FUNCTION__, strKey.c_str());

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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent, m_pRedisVirtualService);
		nRet = pRedisListAgent.PushFront(strKey, strValue);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent, m_pRedisVirtualService);
		    nRet = pRedisListAgent.PushFront(strKey, strValue);
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

int CRedisListAgentScheduler::Reserve(const string &strKey, const int start, const int end, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, Key[%s\n", __FUNCTION__, strKey.c_str());

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
            RVS_XLOG(XLOG_DEBUG, "CRedisListAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisListAgent pRedisListAgent(pRedisAgent);
		nRet = pRedisListAgent.Reserve(strKey, start, end);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisListAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisListAgent pRedisListAgent(pRedisAgent);
		    nRet = pRedisListAgent.Reserve(strKey, start, end);
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

