#include "UpdateManager.h"
#include "BusinessLogHelper.h"
#include "IAsyncLogModule.h"
#include "XmlConfigParser.h"
#include "detail/_time.h"
#include "OsapManager.h"
#include <boost/filesystem.hpp>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "HpsStack.h"
#include "tinyxml/tinyxml.h"
#include "DirReader.h"
#include "sslmd5.h"
#include "XmlConfigParser.h"
#include "CJson.h"

#define SNED_TIME_OUT 4000

#define SAP_SMS_MSG_GETSERVERINFO	11
#define SAP_SMS_MSG_GETURLMAPPING	12
#define SAP_SMS_MSG_GETSERVICEFILES	13

enum
{
	SMS_Attri_ServerInfo	= 21,//(string array)
	SMS_Attri_UrlMapping	= 22,//(string array)
	SMS_Attri_FileName		= 23,//(string)
	SMS_Attri_FileData		= 24,//(bin array)
};

#define TEMP_PATH				"./temp/"
#define TEMP_SERVER_FILE		"./temp/server.xml"
#define TEMP_MAPPING_FILE		"./temp/UrlMapping.xml"
#define TEMP_SERVICE_FILE_PATH	"./temp/services/"

#define DEST_SERVER_FILE		"./server.xml"
#define DEST_MAPPING_FILE		"./plugin/HpsCommonTransfer/UrlMapping.xml"
#define DEST_SERVICE_FILE_PATH	"./plugin/HpsCommonTransfer/services/"
#define DEST_FILE_BACK_PATH		"./plugin/HpsCommonTransfer/back/"

#define SERVICES_PATH_NAME		"services/"
#define SERVER_FILE_NAME		"server.xml"
#define MAPPING_FILE_NAME		"UrlMapping.xml"

#ifdef WIN32

#ifdef access
#   undef access
#endif
#define access _access

#endif

int CUpdateManager::sm_nUpdateOption = 0;
boost::mutex CUpdateManager::sm_mut;
boost::condition_variable  CUpdateManager::sm_cond;
int CUpdateManager::sm_nUpateResult = 0;

namespace
{
	struct SosList
	{
		vector<int> vecServiceId;
		vector<string> vecAddr;
		SosList(){}
		SosList(int nId, const string& strAddr)
		{
			vecServiceId.push_back(nId);
			vecAddr.push_back(strAddr);
		}
		bool CmpServiceId(int nId)
		{
			return find(vecServiceId.begin(), vecServiceId.end(), nId) != vecServiceId.end();
		}
		bool CmpAddr(const string& strAddr)
		{
			return find(vecAddr.begin(), vecAddr.end(), strAddr) != vecAddr.end();
		}
		string GetServiceId()
		{
			string result;
			for (vector<int>::iterator iter = vecServiceId.begin();
				iter != vecServiceId.end(); ++iter)
			{
				char szId[32];
				sprintf(szId, "%u", *iter);
				if (iter != vecServiceId.begin())
					result += ",";
				result += szId;
			}
			return result;
		}
	};

	void Md5File (const char* filename, char* output)
	{
		FILE *file;
		MD5_CTX context;
		int len;
		unsigned char buffer[1024], digest[16];
		if ((file = fopen (filename, "rb")) == NULL)
		{
			return;
		}
		else
		{
			MD5_Init (&context);
			while (len = (int)fread (buffer, 1, 1024, file))
				MD5_Update (&context, buffer, len);
			MD5_Final (digest, &context);
			fclose (file);
			memcpy(output, digest, 16);
		}
	}
}

int UpdateManagerSaveServerXml(vector<SosList> vecSosList);

CUpdateManager::CUpdateManager()
: m_oWork(NULL)
, m_timer(m_oIoService)
, m_nSequence(0)
, m_tmConnTimeout(this,6,boost::bind(&CUpdateManager::OnConnectTimeout,this),CThreadTimer::TIMER_ONCE)
, m_tmRequestTimeout(this,6,boost::bind(&CUpdateManager::OnRequestTimeout,this),CThreadTimer::TIMER_ONCE)
{
}
/*
void CUpdateManager::WaitForUpdate()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n",__FUNCTION__);
	boost::unique_lock<boost::mutex> lock(sm_mut);   	
	sm_cond.timed_wait<boost::posix_time::seconds>(lock, boost::posix_time::seconds(100));
}

void CUpdateManager::NotifyOne()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n",__FUNCTION__);
	boost::unique_lock<boost::mutex> lock(sm_mut);
	sm_cond.notify_one();
}
*/

void CUpdateManager::Start()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);

	m_oWork = new boost::asio::io_service::work(m_oIoService);
	m_thread = boost::thread(boost::bind(&boost::asio::io_service::run,&m_oIoService)); 
	m_oIoService.post(boost::bind(&CUpdateManager::StartInThread, this));

	m_bRunning = true;
	m_timer.expires_from_now(boost::posix_time::seconds(2));
	m_timer.async_wait(MakeSapAllocHandler(
		m_allocTimeOut,boost::bind(&CUpdateManager::DoTimer, this)));
}

void CUpdateManager::Stop()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	m_bRunning = false;
	delete m_oWork;
	m_oWork = NULL;
	m_thread.join();
	m_oIoService.reset();
}

void CUpdateManager::StopThread(int nUpdataStat)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	m_bRunning = false;
	boost::system::error_code ignore_ec;
	m_timer.cancel(ignore_ec);
	if (m_ptrConn.get())
	{
		m_ptrConn->Close();
		m_ptrConn->SetOwner(NULL);
	}
	delete m_oWork;
	m_oWork = NULL;
	CHpsStack::Instance()->UpdateFinished(nUpdataStat);
}

void CUpdateManager::StartInThread()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	Reset();

	try
	{
		boost::filesystem::remove_all(TEMP_PATH);
		if (!boost::filesystem::create_directories(TEMP_SERVICE_FILE_PATH))
		{
			BS_XLOG(XLOG_WARNING,"CUpdateManager::%s create_directories failed\n", __FUNCTION__);
			StopThread(-10);
		}
	}
	catch (...)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s remove_all/create_directories failed\n", __FUNCTION__);
		StopThread(-11);
	}

	LoadConfig();
	if (m_vecOsapAddr.empty())
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s m_vecOsapAddr empty\n", __FUNCTION__);
		StopThread(-1);
		return;
	}
	m_iterOsapAddr = m_vecOsapAddr.begin();
	DoConnect();
}

void CUpdateManager::DoTimer()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	DetectTimerList();
	if(m_bRunning)
	{
		m_timer.expires_from_now(boost::posix_time::seconds(2));
		m_timer.async_wait(MakeSapAllocHandler(
			m_allocTimeOut,boost::bind(&CUpdateManager::DoTimer, this)));
	}
}

void CUpdateManager::DoConnect()
{
	if (m_iterOsapAddr == m_vecOsapAddr.end())
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s end\n", __FUNCTION__);
		StopThread(-2);
		return;
	}
	char szIp[64] = {0};
	unsigned int dwPort = 0;
	int nPri = 0;
	sscanf(m_iterOsapAddr->c_str(), "%[^:]:%u,%d", szIp, &dwPort,&nPri);
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s addr[%s %d]\n", __FUNCTION__,szIp,dwPort);
	m_ptrConn.reset(new CSapConnection(m_oIoService, this, this));
	m_ptrConn->AsyncConnect(szIp, dwPort, 5);
	m_tmConnTimeout.Start();
}

void CUpdateManager::OnConnectResult(unsigned int dwConnId,int nResult)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s nResult[%d]\n",__FUNCTION__, nResult);
	m_tmConnTimeout.Stop();
	if(nResult != 0)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s,connect faild \n",__FUNCTION__ );
		m_ptrConn->Close();
		m_ptrConn->SetOwner(NULL);
		++m_iterOsapAddr;
		DoConnect();
	}
	else
	{
		DownLoadServerInfo();
	}
}

void CUpdateManager::OnConnectTimeout()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	m_ptrConn->Close();
	m_ptrConn->SetOwner(NULL);

	++m_iterOsapAddr;
	DoConnect();
}

void CUpdateManager::OnRequestTimeout()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	m_ptrConn->Close();
	m_ptrConn->SetOwner(NULL);
	StopThread(-3);
}

void CUpdateManager::OnReceiveSosAvenueResponse(unsigned int nId, const void *pBuffer,
			int nLen,const string &strRemoteIp,unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	m_tmRequestTimeout.Stop();
	CSapDecoder oSapDecoder(pBuffer, nLen);
	oSapDecoder.DecodeBodyAsTLV();
	if (oSapDecoder.GetCode() != 0 &&
		!(oSapDecoder.GetCode() >= 100 && oSapDecoder.GetCode() < 200))
	{
		BS_XLOG(XLOG_ERROR,"CUpdateManager::%s code=[%d]\n", __FUNCTION__,oSapDecoder.GetCode());
		StopThread(-4);
		return;
	}
	int nRet = -1;
	if (SAP_SMS_MSG_GETSERVERINFO == oSapDecoder.GetMsgId())
	{
		nRet = ParseServerInfoResponse(oSapDecoder);
		if (nRet == 0) // next step
		{
			DownLoadUrlMapping();
		}
	}
	else if (SAP_SMS_MSG_GETURLMAPPING == oSapDecoder.GetMsgId())
	{
		nRet = ParseUrlMappingResponse(oSapDecoder);
		if (nRet == 0) // next ststepap
		{
			DownLoadServiceFiles();
		}
	}
	else if (SAP_SMS_MSG_GETSERVICEFILES == oSapDecoder.GetMsgId())
	{
		if (oSapDecoder.GetCode() == 0) // ok
		{
			Update();
			return;
		}
		else // file packet
		{
			nRet = ParseServiceFilesResponse(oSapDecoder);
			if (nRet == 0)
			{
				m_tmRequestTimeout.Start(); // receive next file
			}
		}
	}
	else
	{
		StopThread(-5);
		return;
	}

	if (nRet != 0)
	{
		StopThread(-9);
	}
}

void CUpdateManager::OnReceiveAvenueRequest(unsigned int dwConnId, const void *pBuffer,
			int nLen,const string &strRemoteIp,unsigned int dwRemotePort)
{
	BS_XLOG(XLOG_ERROR,"CUpdateManager::%s\n", __FUNCTION__);
}

void CUpdateManager::OnPeerClose(unsigned int nId)
{
	BS_XLOG(XLOG_ERROR,"CUpdateManager::%s\n", __FUNCTION__);
	m_tmRequestTimeout.Stop();
	StopThread(-6);
	return;
}

//////////////////////////////////////////////////////////////////////////
// business handler

void CUpdateManager::LoadConfig()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	CXmlConfigParser oConfig;
	oConfig.ParseFile("./config.xml");
	vector<string> vecServer = oConfig.GetParameters("SosList");

	CXmlConfigParser oServer;
	if(oServer.ParseFile("./server.xml")==0)
	{
		vector<string> vecSosListServer = oServer.GetParameters("SosList");
		vecServer.insert(vecServer.end(), vecSosListServer.begin(), vecSosListServer.end());
	}

	for(vector<string>::iterator iter = vecServer.begin();
		iter != vecServer.end(); ++iter)
	{
		string strSosInfo = *iter;
		CXmlConfigParser oConfig;
		oConfig.ParseDetailBuffer(strSosInfo.c_str());
		string strServiceIds = oConfig.GetParameter("ServiceId","0");
		vector<string> vecStrServiceId;
		boost::algorithm::split(vecStrServiceId, strServiceIds,
			boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on); 
		for(vector<string>::iterator iterVec=vecStrServiceId.begin();
			iterVec!=vecStrServiceId.end(); ++iterVec)
		{
			if (SAP_SMS_SERVICE_ID == (atoi(iterVec->c_str())))
			{
				m_vecOsapAddr = oConfig.GetParameters("ServerAddr");
				break;
			}
		}
	}
}

void CUpdateManager::DownLoadServerInfo()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	CSapEncoder oSapDecoder;
	oSapDecoder.SetService(SAP_PACKET_REQUEST, SAP_SMS_SERVICE_ID, SAP_SMS_MSG_GETSERVERINFO);
	oSapDecoder.SetSequence(m_nSequence++);
	m_ptrConn->WriteData(oSapDecoder.GetBuffer(), oSapDecoder.GetLen(), NULL, SNED_TIME_OUT);
	m_tmRequestTimeout.Start();
}

int CUpdateManager::ParseServerInfoResponse(CSapDecoder& oSapDecoder)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s \n", __FUNCTION__);
	vector<string> vctServerInfo;
	oSapDecoder.GetValues(SMS_Attri_ServerInfo, vctServerInfo);
	//if (vctServerInfo.size() == 0)
	//{
	//	return -1;
	//}
	sort(vctServerInfo.begin(), vctServerInfo.end());
	vector<SosList> vecSosList;
	for (vector<string>::iterator iter = vctServerInfo.begin();
		iter != vctServerInfo.end(); ++iter)
	{
		char addr[1024] = {0};
		unsigned int nId = 0;
		sscanf(iter->c_str(), "%d,%s", &nId, addr);

		bool bProcessed = false;
		for (vector<SosList>::iterator iterList = vecSosList.begin();
			iterList != vecSosList.end(); ++iterList)
		{
			if (iterList->CmpServiceId(nId))
			{
				if (!iterList->CmpAddr(addr))
					iterList->vecAddr.push_back(addr);
				bProcessed = true;
				break;
			}
			else if (iterList->CmpAddr(addr))
			{
				if (!iterList->CmpServiceId(nId))
					iterList->vecServiceId.push_back(nId);
				bProcessed = true;
				break;
			}
		}
		if (!bProcessed)
		{
			vecSosList.push_back(SosList(nId, addr));
		}
	}
	return UpdateManagerSaveServerXml(vecSosList);
}

int UpdateManagerSaveServerXml(vector<SosList> vecSosList)
{
	TiXmlDocument xmlDoc;
	TiXmlDeclaration *declaration = new TiXmlDeclaration("1.0", "UTF-8", "");
	xmlDoc.LinkEndChild(declaration);
	TiXmlElement *xmlRoot = new TiXmlElement("parameters");
	for (vector<SosList>::iterator iterList = vecSosList.begin();
		iterList != vecSosList.end(); ++iterList)
	{
		TiXmlElement *xmlSosList = new TiXmlElement("SosList");
		TiXmlElement *xmlServiceId = new TiXmlElement("ServiceId");
		xmlServiceId->LinkEndChild(new TiXmlText(iterList->GetServiceId().c_str()));
		xmlSosList->LinkEndChild(xmlServiceId);
		for (vector<string>::iterator iter = iterList->vecAddr.begin();
			iter != iterList->vecAddr.end(); ++iter)
		{
			TiXmlElement *xmlAddr = new TiXmlElement("ServerAddr");
			xmlAddr->LinkEndChild(new TiXmlText(iter->c_str()));
			xmlSosList->LinkEndChild(xmlAddr);
		}
		xmlRoot->LinkEndChild(xmlSosList);
	}
	xmlDoc.LinkEndChild(xmlRoot);
	if (!xmlDoc.SaveFile(TEMP_SERVER_FILE))
	{
		return -1;
	}
	return 0;
}

void CUpdateManager::DownLoadUrlMapping()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	CSapEncoder oSapDecoder;
	oSapDecoder.SetService(SAP_PACKET_REQUEST, SAP_SMS_SERVICE_ID, SAP_SMS_MSG_GETURLMAPPING);
	oSapDecoder.SetSequence(m_nSequence++);
	m_ptrConn->WriteData(oSapDecoder.GetBuffer(), oSapDecoder.GetLen(), NULL, SNED_TIME_OUT);
	m_tmRequestTimeout.Start();
}

int CUpdateManager::ParseUrlMappingResponse(CSapDecoder& oSapDecoder)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s \n", __FUNCTION__);
	vector<string> vctUrlMapping;
	oSapDecoder.GetValues(SMS_Attri_UrlMapping, vctUrlMapping);
	if (vctUrlMapping.size() == 0)
	{
		return -1;
	}
	sort(vctUrlMapping.begin(), vctUrlMapping.end());
	TiXmlDocument xmlDoc;
	TiXmlDeclaration *declaration = new TiXmlDeclaration("1.0", "UTF-8", "");
	xmlDoc.LinkEndChild(declaration);
	TiXmlElement *xmlRoot = new TiXmlElement("parameters");
	TiXmlElement *xmlMapping = new TiXmlElement("UrlMapping");
	for (vector<string>::iterator iterList = vctUrlMapping.begin();
		iterList != vctUrlMapping.end(); ++iterList)
	{
		// url,serviceid,msgid,logkey1,logkey2
		vector<string> vecSplit;
		boost::split(vecSplit, *iterList, boost::is_any_of(","));
		if (vecSplit.size() >= 3)
		{
			string strItem = vecSplit[0] + "," + vecSplit[1] + "," + vecSplit[2];
			TiXmlElement *xmlItem = new TiXmlElement("Item");
			xmlItem->LinkEndChild(new TiXmlText(strItem));
			if (vecSplit.size() >= 4 && vecSplit[3].size() > 0)
			{
				xmlItem->SetAttribute("logkey1", vecSplit[3]);
			}
			if (vecSplit.size() >= 5 && vecSplit[4].size() > 0)
			{
				xmlItem->SetAttribute("logkey2", vecSplit[4]);
			}
			xmlMapping->LinkEndChild(xmlItem);
		}
	}
	xmlRoot->LinkEndChild(xmlMapping);
	xmlDoc.LinkEndChild(xmlRoot);
	if (!xmlDoc.SaveFile(TEMP_MAPPING_FILE))
	{
		return -1;
	}
	return 0;
}

void CUpdateManager::DownLoadServiceFiles()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	CSapEncoder oSapDecoder;
	oSapDecoder.SetService(SAP_PACKET_REQUEST, SAP_SMS_SERVICE_ID, SAP_SMS_MSG_GETSERVICEFILES);
	oSapDecoder.SetSequence(m_nSequence++);
	m_ptrConn->WriteData(oSapDecoder.GetBuffer(), oSapDecoder.GetLen(), NULL, SNED_TIME_OUT);
	m_tmRequestTimeout.Start();
}

int CUpdateManager::ParseServiceFilesResponse(CSapDecoder& oSapDecoder)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);
	string strFileName;
	vector<SSapValueNode> vecFileData;
	oSapDecoder.GetValue(SMS_Attri_FileName, strFileName);
	oSapDecoder.GetValues(SMS_Attri_FileData, vecFileData);
	string strFullFileName = string(TEMP_SERVICE_FILE_PATH) + strFileName;
	FILE* pFile = fopen(strFullFileName.c_str(), "wb");
	if (pFile == NULL)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s create file failed [%s]\n", __FUNCTION__,strFileName.c_str());
		return -1;
	}
	for (vector<SSapValueNode>::iterator iter = vecFileData.begin();
		iter != vecFileData.end(); ++iter)
	{
		fwrite(iter->pLoc, iter->nLen, 1, pFile);
	}
	fclose(pFile);
	TiXmlDocument xmlDoc;
	if(!xmlDoc.LoadFile(strFullFileName.c_str()))
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s xml file parse error [%s]\n", __FUNCTION__,strFileName.c_str());
		return -1;
	}
	m_vecServiceFiles.push_back(strFileName);
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s receive file[%s]\n", __FUNCTION__,strFileName.c_str());
	return 0;
}

void CUpdateManager::Update()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s\n", __FUNCTION__);

	if (m_vecServiceFiles.size() == 0)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s no service file??\n", __FUNCTION__);
		StopThread(-7);
		return;
	}
	if (CmpUpdateFiles() != 0)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s do not need update\n", __FUNCTION__);
		StopThread(-8);
		return;
	}

	timeval_a now;
	gettimeofday_a(&now,NULL);
	tm tmv;
    localtime_r(&(now.tv_sec), &tmv);
	char szTime[64];
	sprintf(szTime, "%04u%02u%02u%02u%02u%02u", tmv.tm_year+1900, tmv.tm_mon+1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
	string strBackPath = string(DEST_FILE_BACK_PATH) + string(szTime) + "/";
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s back[%s]\n",__FUNCTION__,strBackPath.c_str());
	try
	{
		// backup old files
		if (!boost::filesystem::create_directories(strBackPath))
		{
			BS_XLOG(XLOG_WARNING,"CUpdateManager::%s create_directories %s failed\n", __FUNCTION__,strBackPath.c_str());
			StopThread(-12);
		}

		boost::filesystem::rename(DEST_SERVER_FILE, strBackPath + SERVER_FILE_NAME);
		if (::access(DEST_SERVICE_FILE_PATH, 0) == 0)
			boost::filesystem::rename(DEST_SERVICE_FILE_PATH, strBackPath + SERVICES_PATH_NAME);
		if (::access(DEST_MAPPING_FILE, 0) == 0)
			boost::filesystem::rename(DEST_MAPPING_FILE, strBackPath + MAPPING_FILE_NAME);

		// place new files
		boost::filesystem::rename(TEMP_SERVICE_FILE_PATH, DEST_SERVICE_FILE_PATH);
		boost::filesystem::rename(TEMP_SERVER_FILE, DEST_SERVER_FILE);
		boost::filesystem::rename(TEMP_MAPPING_FILE, DEST_MAPPING_FILE);
	}
	catch (...)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s filesystem operator failed\n", __FUNCTION__);
		StopThread(-13);
	}

	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s update finished\n",__FUNCTION__);

	StopThread(0);
}

int CUpdateManager::CmpUpdateFiles()
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s...\n",__FUNCTION__);

	if (CmpFile(TEMP_SERVER_FILE, DEST_SERVER_FILE) != 0)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s server.xml not match\n",__FUNCTION__);
		return 0;
	}
	if (CmpFile(TEMP_MAPPING_FILE, DEST_MAPPING_FILE) != 0)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s UrlMapping.xml not match\n",__FUNCTION__);
		return 0;
	}

	vector<string> vecServiceFiles;
	GetCurrentServiceFiles(vecServiceFiles);
	if (vecServiceFiles.size() != m_vecServiceFiles.size())
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s service files not match\n",__FUNCTION__);
		return 0;
	}

	sort(vecServiceFiles.begin(), vecServiceFiles.end());
	sort(m_vecServiceFiles.begin(), m_vecServiceFiles.end());
	for (vector<string>::iterator iter = vecServiceFiles.begin(),
		iter2 = m_vecServiceFiles.begin();
		iter != vecServiceFiles.end(); ++iter, ++iter2)
	{
		string strFile1 = string(DEST_SERVICE_FILE_PATH)+*iter;
		string strFile2 = string(TEMP_SERVICE_FILE_PATH) + *iter2;
		if (*iter != *iter2 || CmpFile(strFile1, strFile2) != 0)
		{
			BS_XLOG(XLOG_WARNING,"CUpdateManager::%s %s %s not match\n",__FUNCTION__,strFile1.c_str(),strFile2.c_str());
			return 0;
		}
	}

	return -1;
}

int CUpdateManager::CmpFile(const string& strFile1, const string& strFile2)
{
	char md51[32] = {0};
	char md52[32] = {0};
	Md5File(strFile1.c_str(), md51);
	Md5File(strFile2.c_str(), md52);
	return memcmp(md51, md52, 16);
}

void CUpdateManager::GetCurrentServiceFiles(vector<string>& vecFiles)
{
	BS_XLOG(XLOG_WARNING,"CUpdateManager::%s...\n",__FUNCTION__);
	nsHpsTransfer::CDirReader oDirReader("*.xml");
	if (!oDirReader.OpenDir(DEST_SERVICE_FILE_PATH))
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s,open dir error\n",__FUNCTION__);
		return;
	}

	char szFileName[MAX_PATH] = {0};
	if (!oDirReader.GetFirstFilePath(szFileName))
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s,GetFirstFilePath error\n",__FUNCTION__);
		return;
	}
	do
	{
		string strFileName = szFileName;
		size_t posFind = strFileName.find_last_of('/');
		if (posFind != string::npos)
		{
			string strFile = strFileName.substr(posFind+1, strFileName.length()-posFind-1);
			vecFiles.push_back(strFile);
			BS_XLOG(XLOG_WARNING,"CUpdateManager::%s [%s]\n",__FUNCTION__, strFile.c_str());
		}
	}while(oDirReader.GetNextFilePath(szFileName));
}

void CUpdateManager::GetAllServerInfo(string& strInfo)
{
	BS_XLOG(XLOG_DEBUG,"CUpdateManager::%s...\n",__FUNCTION__);
	CXmlConfigParser oConfig;
	if(oConfig.ParseFile(DEST_SERVER_FILE)!=0)
	{
		BS_XLOG(XLOG_WARNING,"CUpdateManager::%s get server info failed\n",__FUNCTION__);
		strInfo = "{ \"return_code\": -1 }";
		return;
	}

	vector<SosList> vecRet;
	vector<string> vecServer = oConfig.GetParameters("SosList");
	for(vector<string>::iterator iter = vecServer.begin();
		iter != vecServer.end(); ++iter)
	{
		SosList sosList;
		CXmlConfigParser oConfig;
		oConfig.ParseDetailBuffer(iter->c_str());
		string strServiceIds = oConfig.GetParameter("ServiceId","0");
		sosList.vecAddr = oConfig.GetParameters("ServerAddr");
		vector<string> vecStrServiceId;
		boost::algorithm::split(vecStrServiceId, strServiceIds,
			boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
		for (vector<string>::iterator iter = vecStrServiceId.begin();
			iter != vecStrServiceId.end(); ++iter)
		{
			sosList.vecServiceId.push_back(atoi(iter->c_str()));
		}
		vecRet.push_back(sosList);
	}
	CJsonEncoder json;
	json.SetValue("", "return_code", vecRet.size() ==0 ? -1 : 0);
	vector<CJsonEncoder> dataJson;
	for (vector<SosList>::iterator iter = vecRet.begin();
		iter != vecRet.end(); ++iter)
	{
		CJsonEncoder jsonList;
		jsonList.SetValue("", "serviceId", iter->vecServiceId);
		jsonList.SetValue("", "addr", iter->vecAddr);
		dataJson.push_back(jsonList);
	}
	json.SetValue("", "data", dataJson);
	strInfo = string((char*)json.GetJsonString(), json.GetJsonStringLen());
	json.DestroyJsonEncoder();
}

string CUpdateManager::GetErrorString(int nError)
{
	switch (nError)
	{
	case 0: 
		return "update success";
	case -1: 
		return "not found osapserver address";
	case -2: 
		return "connect osapserver failed";
	case -3: 
		return "osapserver request timeout";
	case -4: 
		return "osapserver request failed";
	case -5: 
		return "osapserver response error";
	case -6: 
		return "osapserver disconnected";
	case -7: 
		return "no service file";
	case -8: 
		return "do not need update";
	case -9: 
		return "process osapserver response failed";
	case -10: 
		return "create temp directories failed";
	case -11: 
		return "create temp directories exception";
	case -12: 
		return "create backup directories failed";
	case -13: 
		return "create backup directories exception";
	case -20: 
		return "system busy or do not need update";
	case -21: 
		return "create transfer object failed";
	case -22: 
		return "reload transfer failed";
	case -23:
		return "load server.xml or config.xml failed";
	}
	return "update failed";
}
