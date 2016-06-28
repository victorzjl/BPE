#include "IpLimitManager.h"
#include "BusinessLogHelper.h"
#include "XmlConfigParser.h"
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include "tinyxml/tinyxml.h"
#include "RsaDbPwd.h"


using namespace sdo::common;


CIpLimitLoader * CIpLimitLoader::sm_pInstance=NULL;

CIpLimitLoader * CIpLimitLoader::GetInstance()
{
	if(sm_pInstance==NULL)
		sm_pInstance=new CIpLimitLoader;
	return sm_pInstance;
}
/*  vip/vip@192.168.93.30:3306/USERINFOEMAIL */
int CIpLimitLoader::ParseConnectString(const string &strConn, string &strHost, string &strUser, string &strPwd, string &strDb, int &nPort)
{
	string::size_type pos = strConn.find("/");
	if(pos == string::npos){return -1;}
	strUser = strConn.substr(0,pos);
	
	string::size_type posend = strConn.rfind("@");
	if(posend == string::npos){return -1;}
	string strPwdTmp = strConn.substr(pos+1,posend-pos-1);
	
	pos = strConn.find(":",posend);
	if(pos == string::npos){return -1;}
	strHost = strConn.substr(posend+1,pos-posend-1);
	
	posend = strConn.find("/",pos);
	if(posend == string::npos){return -1;}
	nPort = atoi(strConn.substr(pos+1,posend-pos-1).c_str());
	
	strDb = strConn.substr(posend+1,strConn.length()-posend-1);
	
	if( 0!= CRsaBase64::ConvertDbPwd(strPwdTmp,strPwd))
	{
		strPwd = strPwdTmp;
	}	
	return 0;
}

int CIpLimitLoader::LoadConfig(const string &strFile,map<string, string> &mapIpsByUrl)
{
	m_strFile = strFile;
	return ReLoadConfig(mapIpsByUrl);
}

int CIpLimitLoader::LoadConfig(const string &strDbConn, const string &strFile,map<string, string> &mapIpsByUrl)
{
	string strUser, strHost, strDb, strPwd;
	int nPort = 0;
	ParseConnectString(strDbConn, strHost, strUser, strPwd, strDb, nPort);
	
	m_strFile = strFile;
	return ReLoadConfig(mapIpsByUrl);
}

int CIpLimitLoader::ReLoadConfig(map<string, string> &mapIpsByUrl)
{
	int ret = LoadConfigFromFile(m_strFile);
	mapIpsByUrl = m_mapIpsByUrl;
	Dump();
	return ret;
}


int CIpLimitLoader::WriteConfigToFile(const string &strFile)
{
	BS_XLOG(XLOG_DEBUG,"CIpLimitLoader::%s, configfile[%s]\n",__FUNCTION__,strFile.c_str());

	string strTmp = strFile + ".tmp";
	int retRename = rename(strFile.c_str(), strTmp.c_str());
	if(retRename != 0)
	{
		BS_XLOG(XLOG_ERROR, "CIpLimitLoader::%s create tmp iplimit file failed, strFile[%s]\n",__FUNCTION__,strFile.c_str());
	}
	
	int nRet = WriteConfigToXmlFile(strFile);
	if(retRename == 0)
	{
		if(nRet != 0)
		{
			nRet = rename(strTmp.c_str(), strFile.c_str());
		}
		else
		{
			remove(strTmp.c_str());
		}
	}
	return nRet;
}
int CIpLimitLoader::WriteConfigToXmlFile(const string &strFile)
{
	BS_XLOG(XLOG_DEBUG,"CIpLimitLoader::%s, configfile[%s]\n",__FUNCTION__,strFile.c_str());
	
	TiXmlDocument xmlDoc;
	TiXmlDeclaration *declaration = new TiXmlDeclaration("1.0", "UTF-8", "");
	xmlDoc.LinkEndChild(declaration);
	TiXmlElement *xmlRoot = new TiXmlElement("parameters");
	
	for (map<string,string>::const_iterator itr = m_mapIpsByUrl.begin(); itr != m_mapIpsByUrl.end(); ++itr)
	{
		TiXmlElement *xmlUrl = new TiXmlElement("urls");
		TiXmlElement *xmlIps = new TiXmlElement("ips");
		
		xmlUrl->LinkEndChild(new TiXmlText(itr->first.c_str()));
		xmlIps->LinkEndChild(new TiXmlText(itr->second.c_str()));
		
		TiXmlElement *xmlItem = new TiXmlElement("limititem");
		xmlItem->LinkEndChild(xmlUrl);
		xmlItem->LinkEndChild(xmlIps);

		xmlRoot->LinkEndChild(xmlItem);
	}
	xmlDoc.LinkEndChild(xmlRoot);
	if (!xmlDoc.SaveFile(strFile))
	{
		BS_XLOG(XLOG_ERROR,"CIpLimitLoader::%s, write ipconfig to file failed. configfile[%s]\n",__FUNCTION__,strFile.c_str());
		return -1;
	}
	return 0;
}

int CIpLimitLoader::LoadConfigFromFile(const string &strFile)
{
	CXmlConfigParser oIpLimitConfig;
    if(0 != oIpLimitConfig.ParseFile(strFile.c_str())){return -1;}

	vector<string> vecIpUrls = oIpLimitConfig.GetParameters("limititem");
	
	vector<string>::const_iterator iter;
	for(iter = vecIpUrls.begin(); iter != vecIpUrls.end(); ++iter)
	{
		string strTemp = *iter;
		CXmlConfigParser oConfig;
		if(0 != oConfig.ParseDetailBuffer(strTemp.c_str()))
		{
			BS_XLOG(XLOG_ERROR, "CIpLimitLoader::%s,  parse ParseDetailBuffer error:%s, buffer is\n%s\n",__FUNCTION__, oConfig.GetErrorMessage().c_str(),strTemp.c_str());
			continue;
		}
		
		string strUrls = oConfig.GetParameter("urls");
		transform(strUrls.begin(), strUrls.end(), strUrls.begin(), ::tolower);
		vector<string> vecUrls;		
		boost::algorithm::split(vecUrls,strUrls,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);

		string strIps = oConfig.GetParameter("ips");

		for(vector<string>::iterator itrV=vecUrls.begin(); itrV!=vecUrls.end(); ++itrV)
		{
			m_mapIpsByUrl.insert(make_pair(*itrV, strIps));
		}		
	}
	
	BS_XLOG(XLOG_ERROR, "CIpLimitLoader::%s,  load %d datas\n",__FUNCTION__, m_mapIpsByUrl.size());
	return 0;
}



void CIpLimitLoader::Dump()
{
	BS_XLOG(XLOG_NOTICE,"CIpLimitLoader, =============DUMP================\n");
	for(map<string,string>::iterator itr=m_mapIpsByUrl.begin(); itr!=m_mapIpsByUrl.end();++itr)
	{
		BS_XLOG(XLOG_NOTICE,"        [%s]=[%s]\n",itr->first.c_str(), itr->second.c_str());
	}
	BS_XLOG(XLOG_NOTICE,"CIpLimitLoader, =============DUMP====end=========\n\n");
}




int CIpLimitCheckor::CheckIpLimit(const string &strUrl, const string &strIp)
{
	BS_XLOG(XLOG_DEBUG,"CIpLimitCheckor::%s, strUrl[%s], strIp[%s]\n",__FUNCTION__,strUrl.c_str(),strIp.c_str());

	map<string, string>::iterator itr = m_mapIpsByUrl.find(strUrl);
	if(itr != m_mapIpsByUrl.end())
	{
		string &strIps = itr->second;
		if(string::npos != strIps.find(strIp))
		{
			BS_XLOG(XLOG_DEBUG,"CIpLimitCheckor::%s, strUrl[%s], strIp[%s], whitelist[%s]\n",__FUNCTION__,strUrl.c_str(),strIp.c_str(),strIps.c_str());
			return 0;
		}
		else
		{
			string::size_type pos = strIp.find(".");
			while(pos != string::npos)
			{
				if(string::npos != strIps.find(strIp.substr(0,pos+1) + "*"))
				{
					return 0;
				}
				
				pos = strIp.find(".",pos+1);
			}
			
			BS_XLOG(XLOG_DEBUG,"CIpLimitCheckor::%s, strUrl[%s], strIp[%s], not in whitelist[%s]\n",__FUNCTION__,strUrl.c_str(),strIp.c_str(),strIps.c_str());
			
			return ERROR_CHECK_AUTHEN_IPLIST;
		}
	}
	return ERROR_CHECK_AUTHEN_IPLIST;
	
}
