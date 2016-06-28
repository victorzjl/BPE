#include "HpsSessionContainer.h"
#include "SessionManager.h"
#include "HpsLogHelper.h"
#include "HpsCommonFunc.h"
#include <boost/algorithm/string.hpp>
#include "HpsStack.h"
#include "UrlCode.h"

CHpsSessionContainer::CHpsSessionContainer(CHpsSessionManager *pManager):m_pSessionManager(pManager),
	m_tmConnCheck(m_pSessionManager, HTTP_CONNECTION_DETECT_INTERVAL, boost::bind(&CHpsSessionContainer::DoCheckHttpConnection,this),CThreadTimer::TIMER_CIRCLE)
{
}

CHpsSessionContainer::~CHpsSessionContainer()
{
	map<unsigned int, CHpsSession*>::iterator iter;
	for(iter = m_mapHpsSession.begin(); iter != m_mapHpsSession.end(); ++iter)
	{
		delete iter->second;
	}
}

void CHpsSessionContainer::StartInThread()
{
	m_tmConnCheck.Start();
}

void CHpsSessionContainer::StopInThread()
{
    m_tmConnCheck.Stop();
}

void CHpsSessionContainer::OnHttpConnected(unsigned int dwId)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, dwId[%u]\n",__FUNCTION__,dwId);

	map<unsigned int, CHpsSession*>::iterator itr = m_mapHpsSession.find(dwId);
	if(itr != m_mapHpsSession.end())
	{
		CHpsSession* pSession = itr->second;
		pSession->SetConnectStatus(CONNECT_CONNECTED);
	}
}

void CHpsSessionContainer::OnReceiveHttpRequest(SRequestInfo &sReq, const string &strRemoteIp,unsigned int dwRemotePort)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, dwId[%u], nServiceId[%d], nMsgId[%d], strUriCommand[%s], strUriAttribute[%s], strRemoteIp[%s], dwRemotePort[%d]\n",
		__FUNCTION__, sReq.dwId,sReq.nServiceId, sReq.nMsgId, sReq.strUriCommand.c_str(), sReq.strUriAttribute.c_str(),strRemoteIp.c_str(),dwRemotePort);
	
	m_pSessionManager->DoReceiveHttpRequest(sReq,strRemoteIp,dwRemotePort);
}

void CHpsSessionContainer::OnHttpPeerClose(unsigned int dwId, const string &strRemoteIp,const unsigned int dwRemotePort)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, dwId[%u], strRemoteIp[%s], dwRemotePort[%d]\n",
		__FUNCTION__,dwId,strRemoteIp.c_str(),dwRemotePort);
	m_pSessionManager->DoHttpPeerClose(dwId, strRemoteIp, dwRemotePort);
}


void CHpsSessionContainer::DoSendResponse(const SRequestInfo &sReq)
{    
    HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::DoSendResponse\n");
	
	CHpsSession * pSession = GetConnection(sReq.dwId);
	if(pSession != NULL)
	{	
    	pSession->DoSendResponse(sReq);
	}	
}

void CHpsSessionContainer::LoadMethodMapping(const vector<string> &vecUrlMapping)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s\n",__FUNCTION__);

	m_preProcess.LoadConfig("./HttpRequestConfig.xml");

	if(!m_mapUrlMsg.empty())
	{
		m_mapUrlMsg.clear();
	}	

	vector<string>::const_iterator it;
	for(it=vecUrlMapping.begin();it!=vecUrlMapping.end();it++)
	{
		vector<string> vecSplit;
		//string strDelim = ","; split voice.AddInfo,3,2
		boost::algorithm::split( vecSplit, (*it), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
		if( vecSplit.size() == 3 )
		{
			SServiceMsgId msgIndentify;
			msgIndentify.dwServiceID = atoi(vecSplit[1].c_str());
			msgIndentify.dwMsgID = atoi(vecSplit[2].c_str());
			string strUrl = vecSplit[0];
			transform(strUrl.begin(), strUrl.end(), strUrl.begin(), ::tolower);
			
			HP_XLOG(XLOG_DEBUG,"UrlMapping/Item:Url:[%s],ServiceID:[%d],MessageID:[%d]\n",strUrl.c_str(),atoi(vecSplit[1].c_str()),atoi(vecSplit[2].c_str()));
			
			m_mapUrlMsg.insert(make_pair(strUrl,msgIndentify));
		}
	}
}

int CHpsSessionContainer::FindUrlMapping(const string& strUrl, unsigned int& dwServiceID, unsigned int& dwMsgID)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, strUrl[%s]\n",__FUNCTION__,strUrl.c_str());
	
	string strTmp = strUrl;
	transform(strTmp.begin(), strTmp.end(), strTmp.begin(), (int(*)(int))tolower);
	
	map<string,SServiceMsgId>::iterator it = m_mapUrlMsg.find(strTmp);
	
	if( it != m_mapUrlMsg.end() )
	{
		dwServiceID = ((*it).second).dwServiceID;
		dwMsgID = ((*it).second).dwMsgID;
		return 0;
	}
	else
	{
		dwServiceID = 0;
		dwMsgID = 0;
		return -1;
	}
}

int CHpsSessionContainer::RequestPrePrecess(SRequestInfo& sReq)
{
	return m_preProcess.Process(sReq);
}

void CHpsSessionContainer::AddConnection(HpsConnection_ptr ptrConn)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, ID[%u]\n",__FUNCTION__,ptrConn->Id());
	unsigned int dwId = ptrConn->Id();
	CHpsSession *pSession = new CHpsSession(dwId,ptrConn,this);
	ptrConn->SetOwner(pSession);
	m_mapHpsSession.insert(make_pair(dwId,pSession));
}

CHpsSession * CHpsSessionContainer::GetConnection(unsigned int nId)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, ID[%u]\n",__FUNCTION__,nId);

	CHpsSession *pSession = NULL;
	map<unsigned int, CHpsSession*>::iterator iter = m_mapHpsSession.find(nId);
	if(iter != m_mapHpsSession.end())
	{
		pSession = iter->second;
	}
	return pSession;  //need to use the temp variable connectin because of the mutex
}

void CHpsSessionContainer::DelConnection(unsigned int dwId)
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s, ID[%u]\n",__FUNCTION__,dwId);

	map<unsigned int, CHpsSession*>::iterator iter = m_mapHpsSession.find(dwId);
	if(iter != m_mapHpsSession.end())
	{
		CHpsSession *pSession = iter->second;
		delete pSession;
		m_mapHpsSession.erase(iter);
	}
}

void CHpsSessionContainer::DoCheckHttpConnection()
{
	HP_XLOG(XLOG_DEBUG,"CHpsSessionContainer::%s\n",__FUNCTION__);
	vector<unsigned int> vecId;
	map<unsigned int, CHpsSession*>::iterator iter = m_mapHpsSession.begin();;
	while(iter != m_mapHpsSession.end())
	{
		map<unsigned int, CHpsSession*>::iterator itrTemp = iter++;
		CHpsSession *pSession = itrTemp->second;
		EConnectStatus nStatus = pSession->GetConnectStatus();
		if(nStatus == CONNECT_CONNECTED)
		{
			pSession->SetConnectStatus(CONNECT_TIME_OUT);			
		}
		else if(nStatus == CONNECT_TIME_OUT)
		{
			OnHttpPeerClose(pSession->GetId(), pSession->GetRemoteIp(), pSession->GetRemotePort());
			pSession->Close();
			m_mapHpsSession.erase(itrTemp);
			delete pSession;
		}
		else
		{
		}
	}
}

void CHpsSessionContainer::Dump()
{
	HP_XLOG(XLOG_NOTICE,"===========CHpsSessionContainer::%s============\n",__FUNCTION__);

	HP_XLOG(XLOG_NOTICE,"   +m_mapUrlMsg.size[%u]\n",m_mapUrlMsg.size());
	map<string,SServiceMsgId>::iterator iter;
	for(iter=m_mapUrlMsg.begin();iter!=m_mapUrlMsg.end();++iter)
	{
		HP_XLOG(XLOG_NOTICE,"       -<%s, %u %u>\n", iter->first.c_str(), iter->second.dwServiceID,iter->second.dwMsgID);
	}

	HP_XLOG(XLOG_NOTICE,"   +m_mapConnContainer<int, CHpsSession*>.size[%d]\n",m_mapHpsSession.size());
	map<unsigned int, CHpsSession*>::iterator iter2;
	for(iter2 = m_mapHpsSession.begin();iter2 != m_mapHpsSession.end();++iter2)
	{
		HP_XLOG(XLOG_NOTICE,"       -Id[%d], connectStatus[%d]\n", iter2->first, iter2->second->GetConnectStatus());
		iter2->second->Dump();
		HP_XLOG(XLOG_NOTICE," \n");
	}
	
}


