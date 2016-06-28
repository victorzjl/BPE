#include "CacheContainer.h"
#include "AsyncVirtualServiceLog.h"
#include "CacheReConnThread.h"
#include "ErrorMsg.h"
#include "CacheMsg.h"

CCacheContainer::CCacheContainer(vector<unsigned int> dwVecServiceId, bool bConHash, const vector<string> &vecStrCacheServer, CCacheThread *pCacheThread, CCacheReConnThread *pCacheReConnThread):
	m_dwVecServiceId(dwVecServiceId),m_pCacheThread(pCacheThread), m_pCacheReConnThread(pCacheReConnThread), m_pConHashUtil(NULL)
{

	string szServiceId = GetServiceIdString();
	CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, ServiceId[%s], ConHash[%d]\n", __FUNCTION__,szServiceId.c_str(), bConHash);
	m_bConHash = bConHash;
	if(m_bConHash)
	{
		m_pConHashUtil = new CConHashUtil;
		if(!m_pConHashUtil->Init())
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, ConHashUtil Init Fail, ServiceId[%s]\n", __FUNCTION__, szServiceId.c_str());
			return;
		}
	}

	vector<string>::const_iterator iter;
	for(iter = vecStrCacheServer.begin(); iter != vecStrCacheServer.end(); iter++)
	{
		string strIpPort;
		int nConHashVirtualNum = 1;
		if(m_bConHash)
		{
			char szIpPort[32] = {0};
			sscanf(iter->c_str(), "%[^#]#%d", szIpPort, &nConHashVirtualNum);
			strIpPort = szIpPort;
			m_mapConVirtualNum.insert(make_pair(strIpPort, nConHashVirtualNum));
		}
		else
		{
			strIpPort = *iter;
		}

		CCacheAgent *pCacheUtil = new CCacheAgent;
		if(pCacheUtil->Initialize(strIpPort) == 0)
		{
			m_pVecCacheUtil.push_back(pCacheUtil);
			if(m_bConHash)
			{
				m_pConHashUtil->AddNode(strIpPort, nConHashVirtualNum);
			}
		}
		else
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Initialize CacheServer[%s] Error\n", __FUNCTION__, iter->c_str());
			m_pCacheReConnThread->OnReConn(m_dwVecServiceId, *iter, pCacheThread);
			delete pCacheUtil;
		}
	}
}


string CCacheContainer::GetServiceIdString()
{
	unsigned int dwIndex = 0;
	string szServiceId;
	while(dwIndex < m_dwVecServiceId.size())
	{
		char szBuf[36]={0};
		snprintf(szBuf,sizeof(szBuf),"%u",m_dwVecServiceId[dwIndex]);
		szServiceId+=szBuf;	
		if (dwIndex+1!=m_dwVecServiceId.size())
		{
			szServiceId+=",";
		}
		dwIndex++;
	}
	return szServiceId;
}
CCacheContainer::~CCacheContainer()
{
	vector<CCacheAgent *>::iterator iter = m_pVecCacheUtil.begin();
	while(iter != m_pVecCacheUtil.end())
	{
		delete *iter;
		iter++;
	}
	m_pVecCacheUtil.clear();

	if(m_pConHashUtil)
	{
		delete m_pConHashUtil;
		m_pConHashUtil = NULL;
	}
}

int CCacheContainer::Add(const string & strKey,const string & strValue,unsigned int dwExpiration, string &strIpAddrs)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, Key[%s], Value[%s], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), dwExpiration);

	int nRet = 0;
	bool bSuccess = false;
	string strSucIpAddrs = "";
	string strFailIpAddrs = "";
	CCacheAgent *pCacheAgent = NULL;
	vector<CCacheAgent *>::iterator iter = m_pVecCacheUtil.begin();

	if(m_bConHash)
	{
		string strConHashIp = m_pConHashUtil->GetConHashNode(strKey);
		while (iter!=m_pVecCacheUtil.end())
		{
			if((*iter)->GetAddr().compare(strConHashIp) == 0)
			{
				pCacheAgent = *iter;
				break;
			}
			++iter;
		}
		if(pCacheAgent==NULL)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Ip[%d] not exist in ConHashUtil\n", __FUNCTION__, strConHashIp.c_str());
			return CVS_SYSTEM_ERROR;
		}

		nRet = pCacheAgent->Add(strKey, strValue, dwExpiration);
		if(nRet != CVS_SUCESS && nRet != CVS_CACHE_ALREADY_EXIST)
		{
			AddIpAddrs(strFailIpAddrs, pCacheAgent);

			m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
			delete pCacheAgent;
			m_pVecCacheUtil.erase(iter);
			m_pConHashUtil->DelNode(strConHashIp);
		}
		else
		{
			AddIpAddrs(strSucIpAddrs, pCacheAgent);
			bSuccess = true;
		}
	}
	else
	{
		while (iter != m_pVecCacheUtil.end())
		{
			pCacheAgent = *iter;
			nRet = pCacheAgent->Add(strKey, strValue, dwExpiration);
			if(nRet != CVS_SUCESS && nRet != CVS_CACHE_ALREADY_EXIST)
			{
				AddIpAddrs(strFailIpAddrs, pCacheAgent);

				m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
				delete pCacheAgent;
				iter = m_pVecCacheUtil.erase(iter);
			}
			else
			{
				AddIpAddrs(strSucIpAddrs, pCacheAgent);

				bSuccess = true;
				iter++;
			}
		}
	}	

	//如果操作成功，则返回0，如果操作不成功，则返回连接失败
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		return CVS_SUCESS;
	}
	else
	{
		strIpAddrs = strFailIpAddrs;
		return nRet;
	}
}

int CCacheContainer::Set(const string & strKey,const string & strValue,unsigned int dwExpiration, string &strIpAddrs)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, Key[%s], Value[%s], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), dwExpiration);

	int nRet = 0;
	bool bSuccess = false;
	string strSucIpAddrs = "";
	string strFailIpAddrs = "";
	CCacheAgent *pCacheAgent = NULL;
	vector<CCacheAgent *>::iterator iter = m_pVecCacheUtil.begin();
	
	if(m_bConHash)
	{
		string strConHashIp = m_pConHashUtil->GetConHashNode(strKey);
		while (iter!=m_pVecCacheUtil.end())
		{
			if((*iter)->GetAddr().compare(strConHashIp) == 0)
			{
				pCacheAgent = *iter;
				break;
			}
			++iter;
		}
		if(pCacheAgent==NULL)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Ip[%d] not exist in ConHashUtil\n", __FUNCTION__, strConHashIp.c_str());
			return CVS_SYSTEM_ERROR;
		}

		nRet = pCacheAgent->Set(strKey, strValue, dwExpiration);
		if(nRet != 0)
		{
			AddIpAddrs(strFailIpAddrs, pCacheAgent);

			m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
			delete pCacheAgent;
			m_pVecCacheUtil.erase(iter);
			m_pConHashUtil->DelNode(strConHashIp);
		}
		else
		{
			AddIpAddrs(strSucIpAddrs, pCacheAgent);
			bSuccess = true;
		}
	}
	else
	{
		while (iter != m_pVecCacheUtil.end())
		{
			pCacheAgent = *iter;
			nRet = pCacheAgent->Set(strKey, strValue, dwExpiration);
			if(nRet != 0)
			{
				AddIpAddrs(strFailIpAddrs, pCacheAgent);

				m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
				delete pCacheAgent;
				iter = m_pVecCacheUtil.erase(iter);
			}
			else
			{
				AddIpAddrs(strSucIpAddrs, pCacheAgent);

				bSuccess = true;
				iter++;
			}
		}
	}	

	//如果操作成功，则返回0，如果操作不成功，则返回连接失败
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
		return CVS_SUCESS;
	}
	else
	{
		strIpAddrs = strFailIpAddrs;
		return nRet;
	}
}

int CCacheContainer::Cas(const string & strKey,const string & strValue,unsigned int dwExpiration, const string &strCas, string &strIpAddrs)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, Key[%s], Value[%s], Cas[%s], Expiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), strCas.c_str(), dwExpiration);

	int nRet = 0;
	bool bSuccess = false;
	string strSucIpAddrs = "";
	string strFailIpAddrs = "";
	CCacheAgent *pCacheAgent = NULL;
	vector<CCacheAgent *>::iterator iter = m_pVecCacheUtil.begin();

	if(m_bConHash)
	{
		string strConHashIp = m_pConHashUtil->GetConHashNode(strKey);
		while (iter!=m_pVecCacheUtil.end())
		{
			if((*iter)->GetAddr().compare(strConHashIp) == 0)
			{
				pCacheAgent = *iter;
				break;
			}
			++iter;
		}
		if(pCacheAgent==NULL)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Ip[%d] not exist in ConHashUtil\n", __FUNCTION__, strConHashIp.c_str());
			return CVS_SYSTEM_ERROR;
		}

		nRet = pCacheAgent->Cas(strKey, strValue, strCas, dwExpiration);
		if(nRet != CVS_SUCESS && nRet != CVS_CACHE_KEY_NOT_FIND && nRet != CVS_CACHE_KEY_VERSION_NOT_ACCORD)
		{
			AddIpAddrs(strFailIpAddrs, pCacheAgent);

			m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
			delete pCacheAgent;
			m_pVecCacheUtil.erase(iter);
			m_pConHashUtil->DelNode(strConHashIp);
		}
		else
		{
			AddIpAddrs(strSucIpAddrs, pCacheAgent);
			bSuccess = true;
		}
	}
	else
	{
		while (iter != m_pVecCacheUtil.end())
		{
			pCacheAgent = *iter;
			nRet = pCacheAgent->Cas(strKey, strValue, strCas, dwExpiration);
			if(nRet != CVS_SUCESS && nRet != CVS_CACHE_KEY_NOT_FIND && nRet != CVS_CACHE_KEY_VERSION_NOT_ACCORD)
			{
				AddIpAddrs(strFailIpAddrs, pCacheAgent);

				m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
				delete pCacheAgent;
				iter = m_pVecCacheUtil.erase(iter);
			}
			else
			{
				AddIpAddrs(strSucIpAddrs, pCacheAgent);

				bSuccess = true;
				iter++;
			}
		}
	}	

	//如果操作成功，则返回0，如果操作不成功，则返回连接失败
	if(bSuccess)
	{
		strIpAddrs = strSucIpAddrs;
	}
	else
	{
		strIpAddrs = strFailIpAddrs;
	}

	return nRet;
}


int CCacheContainer::Get(const string & strKey,string & strValue, string &strIpAddrs, string &strCas)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

	strCas = "";
	int nRet = 0;
	bool bSuccess = false;

	string strNotFindIpAddrs = "";
	string strErrorIpAddrs = "";
	CCacheAgent *pCacheAgent = NULL;
	vector<CCacheAgent *>::iterator iter = m_pVecCacheUtil.begin();
	
	if(m_bConHash)
	{
		string strConHashIp = m_pConHashUtil->GetConHashNode(strKey);
		CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, Key[%s], Server[%s]\n", __FUNCTION__, strKey.c_str(), strConHashIp.c_str());
		while (iter!=m_pVecCacheUtil.end())
		{
			if((*iter)->GetAddr().compare(strConHashIp) == 0)
			{
				pCacheAgent = *iter;
				break;
			}
			++iter;
		}
		if(pCacheAgent==NULL)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Ip[%d] not exist in ConHashUtil\n", __FUNCTION__, strConHashIp.c_str());
			return CVS_SYSTEM_ERROR;
		}

		nRet = pCacheAgent->Get(strKey, strValue, strCas);
		if(nRet == CVS_SUCESS)
		{
			strIpAddrs = pCacheAgent->GetAddr();
			return 0;
		}
		else if(nRet == CVS_CACHE_CONNECT_ERROR)
		{
			AddIpAddrs(strErrorIpAddrs, pCacheAgent);

			m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
			delete pCacheAgent;
			m_pVecCacheUtil.erase(iter);
			m_pConHashUtil->DelNode(strConHashIp);
		}
		else
		{
			AddIpAddrs(strNotFindIpAddrs, pCacheAgent);
			bSuccess = true;
		}
	}
	else
	{
		while (iter != m_pVecCacheUtil.end())
		{
			CCacheAgent *pCacheAgent = *iter;
			nRet = pCacheAgent->Get(strKey, strValue, strCas);
			if(nRet == CVS_SUCESS)
			{
				strIpAddrs = pCacheAgent->GetAddr();
				return 0;
			}
			else if(nRet == CVS_CACHE_CONNECT_ERROR)
			{
				AddIpAddrs(strErrorIpAddrs, pCacheAgent);

				m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
				delete pCacheAgent;
				iter = m_pVecCacheUtil.erase(iter);
			}
			else
			{
				AddIpAddrs(strNotFindIpAddrs, pCacheAgent);

				bSuccess = true;
				iter++;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if(bSuccess)
	{
		strIpAddrs = strNotFindIpAddrs;
		return CVS_CACHE_KEY_NOT_FIND;
	}
	else
	{
		strIpAddrs = strErrorIpAddrs;
		return CVS_CACHE_CONNECT_ERROR;
	}
}

int CCacheContainer::Delete(const string & strKey, string &strIpAddrs)
{
	CVS_XLOG(XLOG_DEBUG, "CCacheContainer::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());

	int nDeleteRet = CVS_CACHE_CONNECT_ERROR;

	string strSucIpAddrs = "";
	string strNotFindIpAddrs = "";
	string strErrorIpAddrs = "";
	CCacheAgent *pCacheAgent = NULL;
	vector<CCacheAgent *>::iterator iter = m_pVecCacheUtil.begin();
	
	if(m_bConHash)
	{
		string strConHashIp = m_pConHashUtil->GetConHashNode(strKey);
		while (iter!=m_pVecCacheUtil.end())
		{
			if((*iter)->GetAddr().compare(strConHashIp) == 0)
			{
				pCacheAgent = *iter;
				break;
			}
			++iter;
		}
		if(pCacheAgent==NULL)
		{
			CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Ip[%d] not exist in ConHashUtil\n", __FUNCTION__, strConHashIp.c_str());
			return CVS_SYSTEM_ERROR;
		}

		int nRet = pCacheAgent->Delete(strKey);
		if(nRet == CVS_SUCESS)
		{
			AddIpAddrs(strSucIpAddrs, pCacheAgent);
			nDeleteRet = CVS_SUCESS;
		}
		else if(nRet == CVS_CACHE_KEY_NOT_FIND)
		{
			AddIpAddrs(strNotFindIpAddrs, pCacheAgent);
			nDeleteRet = CVS_CACHE_KEY_NOT_FIND;
		}
		else
		{
			AddIpAddrs(strErrorIpAddrs, pCacheAgent);
			m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
			delete pCacheAgent;
			m_pVecCacheUtil.erase(iter);
			m_pConHashUtil->DelNode(strConHashIp);
		}
	}
	else
	{
		while(iter != m_pVecCacheUtil.end())
		{
			CCacheAgent *pCacheAgent = *iter;
			int nRet = pCacheAgent->Delete(strKey);
			if(nRet == CVS_SUCESS)
			{
				AddIpAddrs(strSucIpAddrs, pCacheAgent);

				nDeleteRet = CVS_SUCESS;
				iter++;
			}	
			else if(nRet == CVS_CACHE_KEY_NOT_FIND)
			{
				AddIpAddrs(strNotFindIpAddrs, pCacheAgent);

				if(nDeleteRet != CVS_SUCESS)
				{
					nDeleteRet = CVS_CACHE_KEY_NOT_FIND;
				}

				iter++;
			}
			else
			{
				AddIpAddrs(strErrorIpAddrs, pCacheAgent);

				m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
				delete pCacheAgent;
				iter = m_pVecCacheUtil.erase(iter);
			}
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
	if(nDeleteRet == CVS_SUCESS)
	{
		strIpAddrs = strSucIpAddrs;
	}
	else if(nDeleteRet == CVS_CACHE_KEY_NOT_FIND)
	{
		strIpAddrs = strNotFindIpAddrs;
	}
	else
	{
		strIpAddrs = strErrorIpAddrs;
	}

	return nDeleteRet;
}

int CCacheContainer::AddCacheServer( const string &strAddr )
{
	CCacheAgent *pCacheAgent = new CCacheAgent;
	if(pCacheAgent->Initialize(strAddr) == 0)
	{
		m_pVecCacheUtil.push_back(pCacheAgent);
		if(m_bConHash)
		{
			m_pConHashUtil->AddNode(strAddr, m_mapConVirtualNum[strAddr]);
		}
	}
	else
	{
		CVS_XLOG(XLOG_ERROR, "CCacheContainer::%s, Initialize CacheServer[%s] Error\n", __FUNCTION__, strAddr.c_str());
		m_pCacheReConnThread->OnReConn(m_dwVecServiceId, pCacheAgent->GetAddr(), m_pCacheThread);
		delete pCacheAgent;
		return -1;
	}
	
	return 0;
}

void CCacheContainer::AddIpAddrs(string &strIpAddrs, CCacheAgent *pCacheAgent)
{
	if(strIpAddrs.empty())
	{
		strIpAddrs += pCacheAgent->GetAddr();
	}
	else
	{
		strIpAddrs += VALUE_SPLITTER + pCacheAgent->GetAddr();
	}
}
