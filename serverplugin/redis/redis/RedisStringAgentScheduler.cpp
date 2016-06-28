#include "RedisStringAgentScheduler.h"
#include "RedisThread.h"
#include "RedisReConnThread.h"
#include "RedisContainer.h"
#include "RedisVirtualServiceLog.h"
#include "RedisStringAgent.h"
#include "ErrorMsg.h"
#include "RedisMsg.h"
#include "RedisHelper.h"

using namespace redis;

int CRedisStringAgentScheduler::Set(const string & strKey,const string & strValue,unsigned int dwExpiration, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, Key[%s], Value[%s], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), dwExpiration);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.Set(strKey, strValue, dwExpiration);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisStringAgent pRedisStringAgent(pRedisAgent);
			nRet = pRedisStringAgent.Set(strKey, strValue, dwExpiration);
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
	}
	else
	{
		strIpAddrs = strConnectErrorIpAddrs;
		nRet = RVS_REDIS_CONNECT_ERROR;
	}
	return nRet;
}

int CRedisStringAgentScheduler::Get(const string & strKey,string & strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

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
		RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());

        if(strConHashIp.empty())
        {
            RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
        	nRet = GetFromSlave(strKey, strValue, strIpAddrs);
			return nRet;
        }
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			strIpAddrs = strConHashIp;
			nRet = GetFromSlave(strKey, strValue, strIpAddrs);
			return nRet;
		}

        CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.Get(strKey, strValue);
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
			
			nRet = GetFromSlave(strKey, strValue, strIpAddrs);
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
		       RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			   iter++;
			   continue;
		    }
			
			CRedisStringAgent pRedisStringAgent(pRedisAgent);
			nRet = pRedisStringAgent.Get(strKey, strValue);
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
			nRet = GetFromSlave(strKey, strValue, strIpAddrs);
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

int CRedisStringAgentScheduler::GetFromSlave(const string &strKey, string &strValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.Get(strKey, strValue);
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


int CRedisStringAgentScheduler::Delete(const string & strKey, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

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
            RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.Delete(strKey);
		if(nRet == RVS_SUCESS)
		{
		    bSuccess = true;
			strSucIpAddrs = strConHashIp;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp); 
			
			pRedisReConnThread->OnReConn(strConHashIp);
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
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
		       RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisStringAgent pRedisStringAgent(pRedisAgent);
			nRet = pRedisStringAgent.Delete(strKey);
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

int CRedisStringAgentScheduler::Incby(const string &strKey, const int incbyNum, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, Key[%s] Increment[%d]\n", __FUNCTION__, strKey.c_str(), incbyNum);

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
            RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, All Master Redis is not connected\n", __FUNCTION__);
			return RVS_REDIS_CONNECT_ERROR;
        }
		
		iter = mapRedisAddr.find(strConHashIp);	
		if(iter == mapRedisAddr.end())
		{
			RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s : not exist in container] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}
		
		if(iter->second)
		{
			pRedisAgent = pRedisThread->GetRedisAgentByAddr(strConHashIp);
		}

		if(!(iter->second) || pRedisAgent == NULL)
		{
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strConHashIp.c_str());
			return RVS_SYSTEM_ERROR;
		}

        CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.Incby(strKey, incbyNum);
		if(nRet == RVS_SUCESS)
		{
		    bSuccess = true;
			strSucIpAddrs = strConHashIp;
		}
		else if(nRet == RVS_REDIS_CONNECT_ERROR)
		{
			strConnectErrorIpAddrs = strConHashIp;
			pRedisThread->DelRedisServer(strConHashIp); 
			
			pRedisReConnThread->OnReConn(strConHashIp);
		}
		else
		{
			strFailIpAddrs = strConHashIp;
			bOperationFailed = true;
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
		       RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
               iter++;
			   continue;
		    }
			
			CRedisStringAgent pRedisStringAgent(pRedisAgent);
			nRet = pRedisStringAgent.Incby(strKey, incbyNum);
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

//BatchQuery not support consistent hash
int CRedisStringAgentScheduler::BatchQuery(vector<SKeyValue> &vecKeyValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, VectorSize[%d]\n", __FUNCTION__, vecKeyValue.size());

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
		     RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			 iter++;
			 continue;
		 }
			
		CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.BatchQuery(vecKeyValue);
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
		nRet = BatchQueryFromSlave(vecKeyValue, strIpAddrs);
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

int CRedisStringAgentScheduler::BatchQueryFromSlave(vector<SKeyValue> &vecKeyValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, VectorSize[%d]\n", __FUNCTION__, vecKeyValue.size());
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
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
			continue;
		}
			
		CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.BatchQuery(vecKeyValue);
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

//BatchIncrby not support consistent hash
int CRedisStringAgentScheduler::BatchIncrby(vector<SKeyValue> &vecKeyValue, string &strIpAddrs)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgentScheduler::%s, vectorSize[%d]\n", __FUNCTION__, vecKeyValue.size());

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
		    RVS_XLOG(XLOG_ERROR, "CRedisStringAgentScheduler::%s, RedisServer[%s] Status Error\n", __FUNCTION__, strAddr.c_str());
            iter++;
			continue;
		}
			
		CRedisStringAgent pRedisStringAgent(pRedisAgent);
		nRet = pRedisStringAgent.BatchIncrby(vecKeyValue);
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

