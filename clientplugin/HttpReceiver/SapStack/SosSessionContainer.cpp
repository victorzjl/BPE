#include "SosSessionContainer.h"
#include "SapLogHelper.h"
#include <algorithm>

CSosSessionContainer::CSosSessionContainer(unsigned int dwIndex, CHpsSessionManager *pManager,vector<unsigned int> &vecServiceId)
	:m_dwContainerId(dwIndex),m_dwSosSessionCount(0),m_pSessionManager(pManager),m_dwSlsRegeisterSeq(0),m_bIsUsable(false)
{
	m_vecServiceId = vecServiceId;
}

CSosSessionContainer::~CSosSessionContainer()
{
    SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);
	map<int, SSosList>::iterator itr;
	for(itr=m_mapPriSosList.begin(); itr!=m_mapPriSosList.end();++itr)
	{
		SSosList &sSosList = itr->second;

		vector<CSosSession *>::iterator itrSession;
	    while(! sSosList.vecSessions.empty())
	    {    	
	        itrSession = sSosList.vecSessions.begin();
			CSosSession * pSession = *itrSession;
			pSession->Close();
			delete pSession;
	        sSosList.vecSessions.erase(itrSession);
	    }
	}	
}

void CSosSessionContainer::LoadSosList(const vector<string> & vecAddr, CHpsSessionManager *pManager)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, vector.size[%d]\n",__FUNCTION__, vecAddr.size());

	vector<string> vecOldConnAddr; 
	vector<string> vecNewConnAddr; 

	vector<string>::const_iterator itrAddr;
	for(itrAddr=vecAddr.begin(); itrAddr!=vecAddr.end();itrAddr++)
	{
		const string &strAddr = *itrAddr;
		string::size_type loc = strAddr.find(",");
		if(loc == string::npos)
		{
			vecNewConnAddr.push_back(strAddr + ",0");
		}
		else
		{
			vecNewConnAddr.push_back(strAddr);
		}
	}
	m_vecSosAddr = vecNewConnAddr;

	/* step1, travers the m_mapConns, put the connection to delete into the vecConnToDel
	and put the connection not to delete into the vecOldConnAddr*/
	map<int, SSosList>::iterator itr=m_mapPriSosList.begin();
	map<int, SSosList>::iterator itrTemp;
	while(itr!=m_mapPriSosList.end())
	{
		itrTemp = itr++;
		SSosList &sSosList = itrTemp->second;

		vector<CSosSession *>::iterator itrSession = sSosList.vecSessions.begin();
	    while(itrSession != sSosList.vecSessions.end())
	    {
	    	CSosSession * ptrSession = *itrSession; 
			SS_XLOG(XLOG_TRACE,"CSosSessionContainer::%s, ip[%s], port[%u]\n",__FUNCTION__, 
				(ptrSession->GetRemoteIp()).c_str(), ptrSession->GetRemotePort());
	        char szAddr[128] = {0};
	    	sprintf(szAddr, "%s:%u,%d", (ptrSession->GetRemoteIp()).c_str(), (ptrSession->GetRemotePort()), sSosList.nPri);
			vector<string>::const_iterator itrFind = find(vecNewConnAddr.begin(),vecNewConnAddr.end(),szAddr);
			if(itrFind != vecNewConnAddr.end())
			{
				vecOldConnAddr.push_back(szAddr);
				++itrSession;
			}
			else
			{
				ptrSession->Close();
				delete ptrSession;
				itrSession = sSosList.vecSessions.erase(itrSession);
			}

			if(sSosList.vecSessions.empty())
			{
				m_mapPriSosList.erase(itrTemp);
			}
	    }
	}
	
	
	sort(vecOldConnAddr.begin(), vecOldConnAddr.end());

	/*step2, add the new added addrs */
	char szIp[64] = {0};
	unsigned int dwPort = 0;
	int nPri = 0;
	vector<string>::const_iterator iterVecAddr;
	for(iterVecAddr = vecNewConnAddr.begin(); iterVecAddr != vecNewConnAddr.end(); ++iterVecAddr)
	{
		if(!binary_search(vecOldConnAddr.begin(),vecOldConnAddr.end(),*iterVecAddr))
		{			
			sscanf((*iterVecAddr).c_str(), "%[^:]:%u,%d", szIp, &dwPort,&nPri);
			SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, %s:%u,%d", __FUNCTION__,szIp, dwPort,nPri);
			CSosSession *newSession = new CSosSession(++m_dwSosSessionCount,pManager,this,szIp,dwPort);
			m_mapSosSessionBySessionId.insert(make_pair(newSession->GetSosSessionId(), newSession));
			SSosList &sListTemp = m_mapPriSosList[nPri];
			sListTemp.nPri = nPri;
			sListTemp.vecSessions.push_back(newSession);
			newSession->DoConnect();	
		}
	}

	for(itr=m_mapPriSosList.begin(); itr!=m_mapPriSosList.end();++itr)
	{
		SSosList &sSosList = itr->second;
		if(!sSosList.vecSessions.empty())
		{
			sSosList.itrNow = sSosList.vecSessions.begin();
		}
	}
}

void CSosSessionContainer::OnConnected(unsigned int dwSosSessionId, unsigned int dwConnId)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, dwSosSessionId[%u], dwConnId[%u]\n",__FUNCTION__, dwSosSessionId, dwConnId);
	if(find(m_vecServiceId.begin(), m_vecServiceId.end(), (unsigned int)SAP_SLS_SERVICE_ID) != m_vecServiceId.end())
	{
		CSapEncoder oRequest;
		oRequest.SetService(SAP_PACKET_REQUEST,SAP_PLATFORM_SERVICE_ID,SAP_REGISTER_SLS_MESSAGEID);
		oRequest.SetSequence(++m_dwSlsRegeisterSeq);
		oRequest.SetValue(1, 3); // sls service Id

		map<unsigned int, CSosSession *>::iterator itr = m_mapSosSessionBySessionId.find(dwSosSessionId);
		if(itr != m_mapSosSessionBySessionId.end())
		{
			CSosSession *pSession = itr->second;
			
			if(pSession->IsConnected())
			{
				pSession->DoSendRequest(oRequest.GetBuffer(), oRequest.GetLen(), NULL, 0);
			}
			else
			{
				SS_XLOG(XLOG_WARNING,"CSosSessionContainer::%s, sos[%s:%u] is not connected, send request fail. \n",__FUNCTION__,
						pSession->GetRemoteIp().c_str(), pSession->GetRemotePort());
			}
		}
		else
		{
			SS_XLOG(XLOG_WARNING,"CSosSessionContainer::%s, No sls session is Avalilable. \n",__FUNCTION__);
		}
	}

	m_pSessionManager->OnConnected(m_dwContainerId,dwSosSessionId,dwConnId);
}


int CSosSessionContainer::DoSendMessage(const void * pBuffer,int nLen,const void * exHeader,int nExLen, int nInSosPri, int *pOutSosPri, const char* pSignatureKey, unsigned int* pSosId)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, nLen[%d], nExLen[%d],nInSosPri[%d]\n",__FUNCTION__,nLen,nExLen,nInSosPri);

	map<int, SSosList>::iterator itr = m_mapPriSosList.find(nInSosPri);
	if(itr!=m_mapPriSosList.end())
	{			
		*pOutSosPri = itr->first;
		SSosList &sList = itr->second;
		for(unsigned int j=0;j<sList.vecSessions.size()+1;++j,++(sList.itrNow))
		{
			if(sList.itrNow == sList.vecSessions.end())
		    {
		    	sList.itrNow = sList.vecSessions.begin();
		    }

			
			CSosSession* pSession = *(sList.itrNow);
			if(pSession!=NULL && pSession->IsConnected())
			{
				//SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, in if(pConn->IsConnected())\n",__FUNCTION__);
				if (pSosId!=NULL)
				{
					*pSosId = pSession->GetConnId();
				}
				pSession->DoSendRequest(pBuffer, nLen, exHeader, nExLen, pSignatureKey);
				++sList.itrNow;
				
				return 0;
			}
		}
	}

	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, invalid sos priority[%d] or sos in priority is all disconnected, nLen[%d], nExLen[%d]\n",__FUNCTION__,nInSosPri, nLen,nExLen);
	for(itr=m_mapPriSosList.begin(); itr!=m_mapPriSosList.end();++itr)
	{		
		*pOutSosPri = itr->first;
		SSosList &sList = itr->second;
		for(unsigned int j=0;j<sList.vecSessions.size()+1;++j,++sList.itrNow)
		{
			if(sList.itrNow== sList.vecSessions.end())
		    {
		    	sList.itrNow = sList.vecSessions.begin();
		    }

			CSosSession* pSession = *(sList.itrNow);
			if(pSession->IsConnected())
			{
				//SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, in if(pConn->IsConnected())\n",__FUNCTION__);
				if (pSosId!=NULL)
				{
					*pSosId = pSession->GetConnId();
				}
				pSession->DoSendRequest(pBuffer, nLen, exHeader, nExLen, pSignatureKey);
				++sList.itrNow;
				return 0;
			}
		}
	}		
	
	*pOutSosPri = -1;
	return -1;
}

void CSosSessionContainer::OnReceiveAvenueRequest(unsigned int dwSosSessionId, unsigned int dwConnId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, sosSessionId[%u], connId[%u], remoteIp[%s], remotePort[%u], bufferLen[%d]\n",__FUNCTION__,dwSosSessionId, dwConnId,strRemoteIp.c_str(),dwRemotePort,nLen);
	if(m_pSessionManager != NULL)
	{
		m_pSessionManager->OnReceiveAvenueRequest(m_dwContainerId, dwSosSessionId,dwConnId,pBuffer,nLen,strRemoteIp,dwRemotePort);
	}
}

int CSosSessionContainer::DoSendAvenueResponse(unsigned int dwSosSessionId, unsigned int dwConnId,const void *pBuffer, int nLen,const void * exHeader,int nExLen)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, sosSessionId[%u], connId[%u], bufferLen[%d], nExLen[%d]\n",__FUNCTION__,dwSosSessionId, dwConnId,nLen,nExLen);
	map<unsigned int, CSosSession *>::iterator itr = m_mapSosSessionBySessionId.find(dwSosSessionId);
	if(itr != m_mapSosSessionBySessionId.end())
	{
		CSosSession *pSession = itr->second;
		
		if(pSession->IsConnected())
		{
			pSession->DoSendRequest(pBuffer, nLen, exHeader, nExLen);
			return 0;
		}
		else
		{
			return ERROR_SOS_CAN_NOT_REACH;
		}
	}
	else
	{
		return ERROR_NO_SOS_AVAILABLE;
	}
}



void CSosSessionContainer::GetSosStatusData(SSosStatusData &sData)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s\n",__FUNCTION__);
	map<int, SSosList>::iterator itrMap;
	for(itrMap=m_mapPriSosList.begin(); itrMap!=m_mapPriSosList.end();++itrMap)
	{
		SSosList sList = itrMap->second;
		vector<CSosSession *>::iterator iter;
		for(iter = sList.vecSessions.begin();iter != sList.vecSessions.end();++iter)
		{
			CSosSession * ptrSession = *iter;
			if(ptrSession->IsConnected())
			{
				sData.dwActiveConnect++;
			}
			else
			{
				sData.dwDeadConnect++;
				char szAddr[128] = {0};
				sprintf(szAddr, "%s:%u", ptrSession->GetRemoteIp().c_str(), ptrSession->GetRemotePort());
				sData.vecDeadSosAddr.push_back(szAddr);
			}
		}
	}
}

/*
void CSosSessionContainer::AddServiceId(unsigned int dwServiceId)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s,dwServiceId[%u]\n",__FUNCTION__,dwServiceId);
	vector<unsigned int>::iterator iter =  find(m_vecServiceId.begin(), m_vecServiceId.end(), dwServiceId);
	if(iter == m_vecServiceId.end())
	{
		m_vecServiceId.push_back(dwServiceId);
	}
}

int CSosSessionContainer::DeleteServiceId(unsigned int dwServiceId)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s,dwServiceId[%u]\n",__FUNCTION__,dwServiceId);
	vector<unsigned int>::iterator iter =  std::find(m_vecServiceId.begin(), m_vecServiceId.end(), dwServiceId);
	if(iter != m_vecServiceId.end())
	{
		m_vecServiceId.erase(iter);
		return 0;
	}
	else
	{
		SS_XLOG(XLOG_WARNING,"CSosSessionContainer::%s,cannot find dwServiceId[%u]\n",__FUNCTION__,dwServiceId);
		return -1;
	}
}
*/


int CSosSessionContainer::GetMapPriSosList(map<int, string > &mapAddrsByPri)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s, m_mapPriSosList.size[%u]\n",__FUNCTION__,m_mapPriSosList.size());
	map<int, SSosList>::iterator itr;
	for(itr=m_mapPriSosList.begin(); itr!=m_mapPriSosList.end(); ++itr)
	{		
		string strAddrs = "";
		SSosList &sTemp = itr->second;
		vector<CSosSession *>::iterator itrVec;
		for(itrVec = sTemp.vecSessions.begin(); itrVec!=sTemp.vecSessions.end(); itrVec++)
		{
			CSosSession *pSession = *itrVec;
			char szAddr[64] = {0};
			snprintf(szAddr, 63, "%s:%u;", pSession->GetRemoteIp().c_str(),pSession->GetRemotePort());	
			strAddrs += szAddr;			
		}
		mapAddrsByPri.insert(make_pair(sTemp.nPri, strAddrs));
		SS_XLOG(XLOG_TRACE,"CSosSessionContainer::%s,itr->first[%u], nPri[%d], addr[%s]\n",__FUNCTION__,itr->first,sTemp.nPri, strAddrs.c_str());
	}
	return 0;
}



bool CSosSessionContainer::ContainSosAddr(const vector<string> &vecSosAddr)
{
	SS_XLOG(XLOG_DEBUG,"CSosSessionContainer::%s. id[%u], vecSosAddr.size[%d]\n", __FUNCTION__,m_dwContainerId,vecSosAddr.size());

	if(vecSosAddr.size() != m_vecSosAddr.size())
	{
		return false;
	}
	
	vector<string> vecNewConnAddr; 
	vector<string>::const_iterator itrAddr;
	for(itrAddr=vecSosAddr.begin(); itrAddr!=vecSosAddr.end();itrAddr++)
	{
		const string &strAddr = *itrAddr;
		string::size_type loc = strAddr.find(",");
		if(loc == string::npos)
		{
			vecNewConnAddr.push_back(strAddr);
		}
		else
		{
			vecNewConnAddr.push_back(strAddr.substr(0,loc));
		}
	}

	vector<string> vecOldConnAddr; 
	for(itrAddr=m_vecSosAddr.begin(); itrAddr!=m_vecSosAddr.end();itrAddr++)
	{
		const string &strAddr = *itrAddr;
		string::size_type loc = strAddr.find(",");
		if(loc == string::npos)
		{
			vecOldConnAddr.push_back(strAddr);
		}
		else
		{
			vecOldConnAddr.push_back(strAddr.substr(0,loc));
		}
	}

	sort(vecOldConnAddr.begin(), vecOldConnAddr.end());

	for(itrAddr=vecNewConnAddr.begin(); itrAddr!=vecNewConnAddr.end();itrAddr++)
	{
		if(!binary_search(vecOldConnAddr.begin(), vecOldConnAddr.end(), *itrAddr))
		{
			return false;
		}
	}

	return true;
}
void CSosSessionContainer::Dump()
{
	SS_XLOG(XLOG_NOTICE,"========CSosSessionContainer::%s, m_dwIndex[%u]============\n",__FUNCTION__,m_dwContainerId);

	string strServiceids;
	char szServiceIds[16] = {0};
	vector<unsigned int>::iterator iter1;
	for(iter1 = m_vecServiceId.begin(); iter1 != m_vecServiceId.end(); ++iter1)
	{
		sprintf(szServiceIds, "%u, ", *iter1);
		strServiceids += szServiceIds;
	}	
	SS_XLOG(XLOG_NOTICE,"    +m_vecServiceIds:%s\n", strServiceids.c_str());

	string strAddrs;
	vector<string>::iterator iter2;
	for(iter2 = m_vecSosAddr.begin(); iter2 != m_vecSosAddr.end(); ++iter2)
	{
		strAddrs += *iter2 + ";";
	}
	SS_XLOG(XLOG_NOTICE,"    +m_vecSosAddrs:%s\n", strAddrs.c_str());
 

	SS_XLOG(XLOG_NOTICE,"    +m_mapPriSosList, size[%u]\n", m_mapPriSosList.size());
	map<int, SSosList>::iterator itrMap;
	for(itrMap=m_mapPriSosList.begin(); itrMap!=m_mapPriSosList.end();++itrMap)
	{
		SSosList &sList = itrMap->second;
		vector<CSosSession *>::iterator iter;
		SS_XLOG(XLOG_NOTICE, "      +sossession list, pri[%d], sList.vecSessions.size[%u]\n",itrMap->first,sList.vecSessions.size());
		for(iter = sList.vecSessions.begin();iter != sList.vecSessions.end();++iter)
		{
			CSosSession *pSession = *iter;
			SS_XLOG(XLOG_NOTICE, "              -[%s:%u]\n", pSession->GetRemoteIp().c_str(), pSession->GetRemotePort());
		}
		
		if(sList.itrNow!= sList.vecSessions.end())
			SS_XLOG(XLOG_NOTICE,"     -sList.itrNow--[%s:%u]\n\n", ((*sList.itrNow)->GetRemoteIp()).c_str(), (*sList.itrNow)->GetRemotePort());	
	}

	map<unsigned int, CSosSession *>::iterator itrSos;
	SS_XLOG(XLOG_NOTICE, "    +m_mapSosSessionBySessionId, size[%u]\n",m_mapSosSessionBySessionId.size());
	for(itrSos=m_mapSosSessionBySessionId.begin();itrSos!=m_mapSosSessionBySessionId.end();itrSos++)
	{
		CSosSession *pSession = itrSos->second;
		SS_XLOG(XLOG_NOTICE, "      -sossession id[%u], addr[%s:%u]\n",itrSos->first, pSession->GetRemoteIp().c_str(), pSession->GetRemotePort());
	}
}


