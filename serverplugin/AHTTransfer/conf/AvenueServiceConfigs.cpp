#include "AvenueServiceConfigs.h"
#include "DirReader.h"
#include "SoPluginLog.h"
#include <algorithm>

using namespace HT;
#ifdef WIN32
	#define CCH_PATH_SEP	'\\'
	#define CST_PATH_SEP	"\\"
#else
	#define CCH_PATH_SEP	'/'
	#define CST_PATH_SEP	"/"	
#endif
CAvenueServiceConfigs * CAvenueServiceConfigs::s_pInstance = NULL;

bool isDirectory(const string& dirName)
{
	bool bDirectory = false;
	CDirReader oDirReader("*.xml");
	if(oDirReader.OpenDir(dirName.c_str()))
	{
		bDirectory=true;
	}
	return bDirectory;
}


CAvenueServiceConfigs * CAvenueServiceConfigs::Instance()
{
	if(s_pInstance == NULL)
	{
		s_pInstance = new CAvenueServiceConfigs();
	}
	return s_pInstance;
}

int CAvenueServiceConfigs::LoadAvenueServiceConfigs(const string & strServiceConfigPath, vector<int> &vecServiceIds)
{
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::LoadAvenueServiceConfigs [%s]\n", strServiceConfigPath.c_str());
	
	string realPath = strServiceConfigPath;
	bool isAHTDirectory=isDirectory(strServiceConfigPath+"/aht");
	if (isAHTDirectory)
	{
		realPath=strServiceConfigPath+"/aht";
	}
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::LoadAvenueServiceConfigs [%s]\n", realPath.c_str());
	CDirReader oDirReader("aht*.xml");
	if(!oDirReader.OpenDir(realPath.c_str()))
	{
		SO_XLOG(XLOG_ERROR, "OpenDir Failed[%s]\n", realPath.c_str());
		return -1;
	}
	
	char szFileName[MAX_PATH] = {0};
	char szServiceConfigFile[MAX_PATH] = {0};
	if (oDirReader.GetFirstFilePath(szFileName)) 
	{
		do 
		{
			sprintf(szServiceConfigFile, "%s%s%s", strServiceConfigPath.c_str(), CST_PATH_SEP, szFileName);
			SO_XLOG(XLOG_DEBUG, "LoadServiceConfig[%s] len[%d]\n", szServiceConfigFile, strlen(szServiceConfigFile));
			SServiceConfig oServiceConfig;
			if(oServiceConfig.oConfig.LoadServiceConfig(szServiceConfigFile) != 0)
			{
				SO_XLOG(XLOG_ERROR, "LoadServiceConfig[%s] Failed\n", szServiceConfigFile);
				continue;
			}
			oServiceConfig.strName = oServiceConfig.oConfig.GetServiceName();
			oServiceConfig.dwId = oServiceConfig.oConfig.GetServiceId();

			vecServiceIds.push_back(oServiceConfig.dwId);

			SO_XLOG(XLOG_DEBUG, "ServiceName[%s] ServiceId[%d] Load Success\n", oServiceConfig.strName.c_str(), oServiceConfig.dwId);
			m_mapServiceConfigByName[oServiceConfig.strName] = oServiceConfig;
			m_mapServiceConfigById[oServiceConfig.dwId] = oServiceConfig;
		} while (oDirReader.GetNextFilePath(szFileName));
		return 0;
	}
	else 
	{
		SO_XLOG(XLOG_DEBUG, "the dir is empty or no file is unfiltered, dir=[%s]\n", strServiceConfigPath.c_str());
		return -1;
	}
}

int CAvenueServiceConfigs::LoadAvenueServiceConfigByFile(const string &strConfigFile)
{    
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::%s [%s]\n", __FUNCTION__,strConfigFile.c_str());
    SServiceConfig oServiceConfig;
    if(oServiceConfig.oConfig.LoadServiceConfig(strConfigFile) != 0)
    {
        SO_XLOG(XLOG_ERROR, "LoadServiceConfig[%s] Failed\n", strConfigFile.c_str());
        return -1;
    }
    oServiceConfig.strName = oServiceConfig.oConfig.GetServiceName();
    oServiceConfig.dwId = oServiceConfig.oConfig.GetServiceId();
    
    m_mapServiceConfigByName[oServiceConfig.strName] = oServiceConfig;
    m_mapServiceConfigById[oServiceConfig.dwId] = oServiceConfig;
    return 0;
}

int CAvenueServiceConfigs::GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId, unsigned int *pMsgId)
{
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceIdByName [%s]\n", strServiceName.c_str());
    string strLowName(strServiceName); 
    transform(strLowName.begin(), strLowName.end(), strLowName.begin(), (int(*)(int))tolower);
	char szServiceName[64] = {0};
	char szMsgName[64] = {0};
	sscanf(strLowName.c_str(), "%[^.].%s", szServiceName, szMsgName);
	map<string, SServiceConfig>::iterator iter = m_mapServiceConfigByName.find(szServiceName);
	if(iter != m_mapServiceConfigByName.end())
	{
		SServiceConfig & oServiceConfig = iter->second;
		SConfigMessage * oMessageType;
		if(oServiceConfig.oConfig.GetMessageTypeByName(szMsgName, &oMessageType) < 0)
		{
			SO_XLOG(XLOG_ERROR, "GetMessageTypeByName[%s] Failed\n", szMsgName);
			return -1;
		}
		*pServiceId = oServiceConfig.dwId;
		*pMsgId = oMessageType->dwId;
		return 0;
	}
	else 
	{
		SO_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceIdByName [%s] failed\n", strServiceName.c_str());
		return -1;
	}
}

int CAvenueServiceConfigs::GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId)
{
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceIdByName [%s]\n", strServiceName.c_str());
    string strLowName(strServiceName); 
    transform(strLowName.begin(), strLowName.end(), strLowName.begin(), (int(*)(int))tolower);	
	map<string, SServiceConfig>::iterator iter = m_mapServiceConfigByName.find(strLowName);
	if(iter != m_mapServiceConfigByName.end())
	{
		SServiceConfig & oServiceConfig = iter->second;		
		*pServiceId = oServiceConfig.dwId;		
		return 0;
	}
	else 
	{
		SO_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceIdByName [%s] failed\n", strServiceName.c_str());
		return -1;
	}
}


int CAvenueServiceConfigs::GetServiceNameById(unsigned int dwServiceId, unsigned int dwMsgId, string &strServiceName)
{
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceNameById serviceid[%d], msgid[%d]\n", dwServiceId, dwMsgId);

	map<unsigned int, SServiceConfig>::iterator iter = m_mapServiceConfigById.find(dwServiceId);
	if(iter != m_mapServiceConfigById.end())
	{
		SServiceConfig & oServiceConfig = iter->second;
		SConfigMessage *oMessageType;
		if(oServiceConfig.oConfig.GetMessageTypeById(dwMsgId, &oMessageType) < 0)
		{
			SO_XLOG(XLOG_ERROR, "GetMessageTypeById[%d] Failed\n", dwMsgId);
			return -1;
		}
		char szServiceName[128] = {0};
		int nLen = sprintf(szServiceName, "%s.%s", oServiceConfig.strName.c_str(), oMessageType->strName.c_str());
		strServiceName = string(szServiceName, nLen);
		return 0;
	}
	else 
	{
		SO_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceNameById serviceid[%d] msgid[%d] failed\n", dwServiceId, dwMsgId);
		return -1;
	}
}


int CAvenueServiceConfigs::GetServiceByName(const string & strServiceName, SServiceConfig ** oSeriveConfig)
{
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceByName [%s]\n", strServiceName.c_str());
	map<string, SServiceConfig>::iterator iter = m_mapServiceConfigByName.find(strServiceName);
	if(iter != m_mapServiceConfigByName.end())
	{
		*oSeriveConfig = &(iter->second);
		return 0;
	}
	else 
	{
		SO_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceByName [%s] failed\n", strServiceName.c_str());
		return -1;
	}
}
int CAvenueServiceConfigs::GetServiceById(unsigned int dwServiceId, SServiceConfig ** oSeriveConfig)
{
	SO_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceById serviceid[%d]\n", dwServiceId);
	
	map<unsigned int, SServiceConfig>::iterator iter = m_mapServiceConfigById.find(dwServiceId);
	if(iter != m_mapServiceConfigById.end())
	{
		*oSeriveConfig = &(iter->second);
		return 0;
	}
	else 
	{
		SO_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceById [%d] failed\n", dwServiceId);
		return -1;
	}


}

void CAvenueServiceConfigs::Dump() const
{
	SO_XLOG(XLOG_NOTICE, "############begin dump############\n");

	SO_XLOG(XLOG_NOTICE, "mapServiceConfigByName,size[%d]\n",m_mapServiceConfigByName.size());
	map<string, SServiceConfig>::const_iterator itrService;
	for(itrService = m_mapServiceConfigByName.begin(); itrService != m_mapServiceConfigByName.end(); itrService++)
	{
		const SServiceConfig & oServiceConfig = itrService->second;
		SO_XLOG(XLOG_DEBUG, "name[%s] id[%d] type[%d]\n", 
			oServiceConfig.strName.c_str(), oServiceConfig.dwId);
		oServiceConfig.oConfig.Dump();
		
	}
	SO_XLOG(XLOG_NOTICE, "############end dump############\n");
}

bool CAvenueServiceConfigs::IsExist( unsigned int dwServiceId, unsigned int dwMsgId )
{
	map<unsigned int , SServiceConfig>::iterator iter = m_mapServiceConfigById.find(dwServiceId);
	if(iter == m_mapServiceConfigById.end())
	{
		return false;
	}
	
	return iter->second.oConfig.IsExist(dwMsgId);
}

