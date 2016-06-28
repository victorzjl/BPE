#include "RedisZsetAgentScheduler.h"
#include "RedisThread.h"
#include "RedisReConnThread.h"
#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisZsetAgent.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisHelper.h"

using namespace redis;

int CRedisZsetAgentScheduler::Get(const string &strKey, const string &strValue, int &score, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Value[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetFromSlave(strKey, strValue, score, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetFromSlave(strKey, strValue, score, strIpAddrs);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Get(strKey, strValue, score);
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
			
			nRet = GetFromSlave(strKey, strValue, score, strIpAddrs);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.Get(strKey, strValue, score);
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
			nRet = GetFromSlave(strKey, strValue, score, strIpAddrs);
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

int CRedisZsetAgentScheduler::Set(const string &strKey, const string &strValue, const int score, unsigned int dwExpiration, int &result, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Value[%s], Score[%d], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), score, dwExpiration);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Set(strKey, strValue, score, dwExpiration, result);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
			nRet = pRedisZsetAgent.Set(strKey, strValue, score, dwExpiration, result);
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

int CRedisZsetAgentScheduler::Delete(const string &strKey, const string &strField, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Field[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());

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
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Delete(strKey, strField);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.Delete(strKey, strField);
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

int CRedisZsetAgentScheduler::Size(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = SizeFromSlave(strKey, size, strIpAddrs);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Size(strKey, size);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.Size(strKey, size);
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

int CRedisZsetAgentScheduler::GetAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.GetAll(strKey, start, end, mapFieldAndScore);
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
			
			nRet = GetAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
		}
		else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.GetAll(strKey, start, end, mapFieldAndScore);
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
			else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
			nRet = GetAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_ZSET_NO_MEMBERS;
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

int CRedisZsetAgentScheduler::GetReverseAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetReverseAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetReverseAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.GetReverseAll(strKey, start, end, mapFieldAndScore);
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
			
			nRet = GetReverseAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
		}
		else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.GetReverseAll(strKey, start, end, mapFieldAndScore);
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
			else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
			nRet = GetReverseAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_ZSET_NO_MEMBERS;
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

int CRedisZsetAgentScheduler::GetScoreAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs, bool bInputStart, bool bInputEnd)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetScoreAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs, bInputStart, bInputEnd);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetScoreAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs, bInputStart, bInputEnd);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.GetScoreAll(strKey, start, end, mapFieldAndScore, bInputStart, bInputEnd);
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
			
			nRet = GetScoreAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs, bInputStart, bInputEnd);
		}
		else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.GetScoreAll(strKey, start, end, mapFieldAndScore, bInputStart, bInputEnd);
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
			else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
			nRet = GetScoreAllFromSlave(strKey, start, end, mapFieldAndScore, strIpAddrs, bInputStart, bInputEnd);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bNotFound)
	{
		strIpAddrs = strNotFindIpAddrs;
		nRet = RVS_REDIS_ZSET_NO_MEMBERS;
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

int CRedisZsetAgentScheduler::Rank(const string &strKey, const string &strField, int &rank, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Field[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = RankFromSlave(strKey, strField, rank, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = RankFromSlave(strKey, strField, rank, strIpAddrs);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Rank(strKey, strField, rank);
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
			
			nRet = RankFromSlave(strKey, strField, rank, strIpAddrs);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.Rank(strKey, strField, rank);
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
			nRet = RankFromSlave(strKey, strField, rank, strIpAddrs);
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

int CRedisZsetAgentScheduler::ReverseRank(const string &strKey, const string &strField, int &rank, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Field[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());

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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = ReverseRankFromSlave(strKey, strField, rank, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = ReverseRankFromSlave(strKey, strField, rank, strIpAddrs);
			return nRet;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.ReverseRank(strKey, strField, rank);
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
			
			nRet = ReverseRankFromSlave(strKey, strField, rank, strIpAddrs);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.ReverseRank(strKey, strField, rank);
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
			nRet = ReverseRankFromSlave(strKey, strField, rank, strIpAddrs);
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

int CRedisZsetAgentScheduler::Incby(const string &strKey, const string &strField, const int score, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Field[%s], Score[%d]\n", __FUNCTION__, strKey.c_str(), strField.c_str(), score);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Incby(strKey, strField, score);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.Incby(strKey, strField, score);
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

int CRedisZsetAgentScheduler::DelRangeByRank(const string &strKey, const int start, const int end, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Start[%d], End[%d]\n", __FUNCTION__, strKey.c_str(), start, end);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.DelRangeByRank(strKey, start, end);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.DelRangeByRank(strKey, start, end);
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

int CRedisZsetAgentScheduler::DelRangeByScore(const string &strKey, const int start, const int end, string &strIpAddrs, bool bInputStart, bool bInputEnd)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s], Start[%d], End[%d]\n", __FUNCTION__, strKey.c_str(), start, end);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.DelRangeByScore(strKey, start, end, bInputStart, bInputEnd);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		    nRet = pRedisZsetAgent.DelRangeByScore(strKey, start, end, bInputStart, bInputEnd);
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

int CRedisZsetAgentScheduler::GetFromSlave(const string &strKey, const string &strValue, int &score, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Value[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Get(strKey, strValue, score);
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

int CRedisZsetAgentScheduler::SizeFromSlave(const string &strKey, int &size, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Size(strKey, size);
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

int CRedisZsetAgentScheduler::GetAllFromSlave(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Start[%d] End[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.GetAll(strKey, start, end, mapFieldAndScore);
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
		else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
		nRet = RVS_REDIS_ZSET_NO_MEMBERS;
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

int CRedisZsetAgentScheduler::GetReverseAllFromSlave(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Start[%d] End[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.GetReverseAll(strKey, start, end, mapFieldAndScore);
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
		else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
		nRet = RVS_REDIS_ZSET_NO_MEMBERS;
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

int CRedisZsetAgentScheduler::GetScoreAllFromSlave(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, string &strIpAddrs, bool bInputStart, bool bInputEnd)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Start[%d] End[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.GetScoreAll(strKey, start, end, mapFieldAndScore, bInputStart, bInputEnd);
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
		else if(nRet == RVS_REDIS_ZSET_NO_MEMBERS)
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
		nRet = RVS_REDIS_ZSET_NO_MEMBERS;
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

int CRedisZsetAgentScheduler::RankFromSlave(const string &strKey, const string &strField, int &rank, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Field[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.Rank(strKey, strField, rank);
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

int CRedisZsetAgentScheduler::ReverseRankFromSlave(const string &strKey, const string &strField, int &rank, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgentScheduler::%s, Key[%s] Field[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisZsetAgent pRedisZsetAgent(pRedisAgent);
		nRet = pRedisZsetAgent.ReverseRank(strKey, strField, rank);
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

