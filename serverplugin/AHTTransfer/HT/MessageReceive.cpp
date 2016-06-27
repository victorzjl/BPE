#include "MessageReceive.h"
#include "HTDealerServiceLog.h"
#include <stdlib.h> 
#include <time.h>
#include <_time.h>
#include <SapMessage.h>
#include "PluginManager.h"
#include "PluginRegister.h"
#include <XmlConfigParser.h>
#include "CohHttpsStack.h"

using std::stringstream;
using namespace sdo::sap;
using namespace sdo::common;

//CMessageReceive *CMessageReceive::m_pInstance = new CMessageReceive;
CMessageReceive *CMessageReceive::m_pInstance = NULL;
CMessageReceive::CGarbo CMessageReceive::Garbo;
boost::mutex CMessageReceive::m_mut;

CMessageReceive* CMessageReceive::GetInstance()
{
	
	if(m_pInstance==NULL)
	{
		
		boost::unique_lock<boost::mutex> lock(m_mut);
		if(m_pInstance==NULL)
			m_pInstance=new CMessageReceive;
	}
	
	return m_pInstance;
	
}

CMessageReceive::CMessageReceive()
{
	//加载插件
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s\n", __FUNCTION__);
	rand = 0;
	int ret = CPluginManager::GetInstance()->InitPlugin("./plugin_so");	
	if( ret != 0 )
	{
		HT_XLOG(XLOG_ERROR, "CMessageReceive::%s load plugin fail!\n", __FUNCTION__);		
	}
	
	/*
	*解析config获得启动线程
	*/
	
	//读取config文件
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile("./config.xml") != 0)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, Config Parser fail, file[./config.xml], error[%s]\n", __FUNCTION__, objConfigParser.GetErrorMessage().c_str());
		//return HT_CONFIG_ERROR;
	}
	
	CCohHttpsStack::Start();
	unsigned int dwThreadNum = objConfigParser.GetParameter("AHTTansferThreadNum", 0);
	bpeThreadNum = objConfigParser.GetParameter("ThreadNum", 0);
	selfCheckCount=0;
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s ThreadNum[%d]\n", __FUNCTION__,dwThreadNum);
	Start(dwThreadNum);	
}

/**
void CMessageReceive::_registerSend(string name, PLUGIN_FUNC_SEND funcAddr)
{
	CPluginRegister::GetInstance()->insertSendFunc(name,funcAddr);
}

void CMessageReceive::_registerRecv(string name, PLUGIN_FUNC_RECV funcAddr)
{
	CPluginRegister::GetInstance()->insertRecvFunc(name,funcAddr);
}
**/

CMessageReceive::~CMessageReceive()
{
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s\n", __FUNCTION__);
	//Stop();
	
}

int CMessageReceive::Start( unsigned int dwThreadNum )
{
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s, ThreadNum[%d]\n", __FUNCTION__, dwThreadNum);
	
	int nRet = 0;
	bool bSuccess = false;

	//启动线程组
	for(unsigned int i=0;i<dwThreadNum;i++)
	{
		CMessageDealer *pThread = new CMessageDealer();
		m_vecChildThread.push_back(pThread);
		nRet = pThread->Start();
		if(nRet != 0)
		{
			HT_XLOG(XLOG_ERROR, "CMessageReceive::%s, CMessageDealer start error\n", __FUNCTION__);
		}
		else
		{
			bSuccess = true;
		}
	}

	if(bSuccess)
	{
		return 0;
	}
	else
	{
		return nRet;
	}
}

void CMessageReceive::Stop()
{
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s\n", __FUNCTION__);
	vector<CMessageDealer *>::iterator iter;
	for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
	{
		
		if(*iter != NULL)
		{
			//(*iter)->Stop();
			delete *iter;
			*iter = NULL;
		}
	}
	m_vecChildThread.clear();
	
}

void CMessageReceive::OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid,
                               timeval_a tStart, void *pHandler, const void *pBuffer, int nLen, CHTDealer *htd)
{	
	int size = m_vecChildThread.size();
	//srand((unsigned)time(NULL));
	int threadNum = rand % size;
	rand++;
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s tread[%d] handler[%x] guid[%s]\n", __FUNCTION__, threadNum,pHandler,strGuid.c_str());

	(m_vecChildThread.at(threadNum))->OnSend(dwServiceId,dwMsgId,dwSequenceId, strGuid, tStart , pHandler, pBuffer, nLen, htd);		
	
}

void CMessageReceive::GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum )
{
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s\n", __FUNCTION__);
	dwAliveNum = 0;
	dwAllNum = m_vecChildThread.size();
	
	if( selfCheckCount % bpeThreadNum!=0)
	{
		++selfCheckCount;
		dwAliveNum = dwAliveNum;
	}
	else{
		++selfCheckCount;
		vector<CMessageDealer *>::iterator iter;
	
		for (iter = m_vecChildThread.begin(); iter != m_vecChildThread.end(); iter++)
		{
			if( (*iter)->GetSelfCheck() )
				dwAliveNum++;
		}
	}
	HT_XLOG(XLOG_DEBUG, "CMessageReceive::%s dwAliveNum[%d]\n", __FUNCTION__,dwAliveNum);
	
}


