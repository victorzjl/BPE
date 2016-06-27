#include "AvenueServiceConfigs.h"
#include "DirReader.h"
#include "SdkLogHelper.h"
#include <algorithm>

namespace sdo{
namespace commonsdk{

#ifdef WIN32
	#define CCH_PATH_SEP	'\\'
	#define CST_PATH_SEP	"\\"
#else
	#define CCH_PATH_SEP	'/'
	#define CST_PATH_SEP	"/"	
#endif
CAvenueServiceConfigs * CAvenueServiceConfigs::s_pInstance = NULL;

CAvenueServiceConfigs * CAvenueServiceConfigs::Instance()
{
	if(s_pInstance == NULL)
	{
		s_pInstance = new CAvenueServiceConfigs();
	}
	return s_pInstance;
}

int CAvenueServiceConfigs::LoadAvenueServiceConfigs(const string & strServiceConfigPath)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::LoadAvenueServiceConfigs [%s]\n", strServiceConfigPath.c_str());
	CDirReader oDirReader("*.xml");
	if(!oDirReader.OpenDir(strServiceConfigPath.c_str(), true))
	{
		SDK_XLOG(XLOG_ERROR, "OpenDir Failed[%s]\n", strServiceConfigPath.c_str());
		return -1;
	}
	
	char szFileName[MAX_PATH] = {0};
	//char szServiceConfigFile[MAX_PATH] = {0};
	if (oDirReader.GetFirstFilePath(szFileName)) 
	{
		//snprintf(szServiceConfigFile, MAX_PATH, "%s%s%s", strServiceConfigPath.c_str(), CST_PATH_SEP, szFileName);
		//SDK_XLOG(XLOG_DEBUG, "LoadServiceConfig[%s] len[%d]\n", szServiceConfigFile, strlen(szServiceConfigFile));
		SSeriveConfig oServiceConfig;
		if(oServiceConfig.oConfig.LoadServiceConfig(szFileName) != 0)
		{
			SDK_XLOG(XLOG_ERROR, "LoadServiceConfig[%s] Failed\n", szFileName);
			return -1;
		}
		oServiceConfig.strName = oServiceConfig.oConfig.GetServiceName();
		oServiceConfig.dwId = oServiceConfig.oConfig.GetServiceId();
		
		m_mapServiceConfigByName[oServiceConfig.strName] = oServiceConfig;
		m_mapServiceConfigById[oServiceConfig.dwId] = oServiceConfig;
		
		while(oDirReader.GetNextFilePath(szFileName))
		{
			//snprintf(szServiceConfigFile, MAX_PATH, "%s%s%s", strServiceConfigPath.c_str(), CST_PATH_SEP, szFileName);
			//SDK_XLOG(XLOG_DEBUG, "LoadServiceConfig[%s] len[%d]\n", szServiceConfigFile, strlen(szServiceConfigFile));
			SSeriveConfig oServiceConfig;
			if(oServiceConfig.oConfig.LoadServiceConfig(szFileName) != 0)
			{
				SDK_XLOG(XLOG_ERROR, "LoadServiceConfig[%s] Failed\n", szFileName);
				return -1;
			}
			oServiceConfig.strName = oServiceConfig.oConfig.GetServiceName();
			oServiceConfig.dwId = oServiceConfig.oConfig.GetServiceId();
			
			m_mapServiceConfigByName[oServiceConfig.strName] = oServiceConfig;
			m_mapServiceConfigById[oServiceConfig.dwId] = oServiceConfig;
		}
		return 0;
	}
	else 
	{
		SDK_XLOG(XLOG_DEBUG, "the dir is empty or no file is unfiltered, dir=[%s]\n", strServiceConfigPath.c_str());
		return -1;
	}
	
}

int CAvenueServiceConfigs::LoadAvenueServiceConfigByFile(const string &strConfigFile)
{    
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::%s [%s]\n", __FUNCTION__,strConfigFile.c_str());
    SSeriveConfig oServiceConfig;
    if(oServiceConfig.oConfig.LoadServiceConfig(strConfigFile) != 0)
    {
        SDK_XLOG(XLOG_ERROR, "LoadServiceConfig[%s] Failed\n", strConfigFile.c_str());
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
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceIdByName [%s]\n", strServiceName.c_str());
    string strLowName(strServiceName); 
    transform(strLowName.begin(), strLowName.end(), strLowName.begin(), (int(*)(int))tolower);
	char szServiceName[64] = {0};
	char szMsgName[64] = {0};
	sscanf(strLowName.c_str(), "%[^.].%s", szServiceName, szMsgName);
	Qmap<string, SSeriveConfig>::iterator iter = m_mapServiceConfigByName.find(szServiceName);
	if(iter != m_mapServiceConfigByName.end())
	{
		SSeriveConfig & oServiceConfig = iter->second;
		SAvenueMessageConfig * oMessageType;
		if(oServiceConfig.oConfig.GetMessageTypeByName(szMsgName, &oMessageType) < 0)
		{
			SDK_XLOG(XLOG_ERROR, "GetMessageTypeByName[%s] Failed\n", szMsgName);
			return -1;
		}
		*pServiceId = oServiceConfig.dwId;
		*pMsgId = oMessageType->dwId;
		return 0;
	}
	else 
	{
		SDK_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceIdByName [%s] failed\n", strServiceName.c_str());
		return -1;
	}
}

int CAvenueServiceConfigs::GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceIdByName [%s]\n", strServiceName.c_str());
    string strLowName(strServiceName); 
    transform(strLowName.begin(), strLowName.end(), strLowName.begin(), (int(*)(int))tolower);	
	Qmap<string, SSeriveConfig>::iterator iter = m_mapServiceConfigByName.find(strLowName);
	if(iter != m_mapServiceConfigByName.end())
	{
		SSeriveConfig & oServiceConfig = iter->second;		
		*pServiceId = oServiceConfig.dwId;		
		return 0;
	}
	else 
	{
		SDK_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceIdByName [%s] failed\n", strServiceName.c_str());
		return -1;
	}
}


int CAvenueServiceConfigs::GetServiceNameById(unsigned int dwServiceId, unsigned int dwMsgId, string &strServiceName)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceNameById serviceid[%d], msgid[%d]\n", dwServiceId, dwMsgId);

	Qmap<unsigned int, SSeriveConfig>::iterator iter = m_mapServiceConfigById.find(dwServiceId);
	if(iter != m_mapServiceConfigById.end())
	{
		SSeriveConfig & oServiceConfig = iter->second;
		SAvenueMessageConfig *oMessageType;
		if(oServiceConfig.oConfig.GetMessageTypeById(dwMsgId, &oMessageType) < 0)
		{
			SDK_XLOG(XLOG_ERROR, "GetMessageTypeById[%d] Failed\n", dwMsgId);
			return -1;
		}
		char szServiceName[128] = {0};
		int nLen = snprintf(szServiceName, 128, "%s.%s", oServiceConfig.strName.c_str(), oMessageType->strName.c_str());
		strServiceName = string(szServiceName, nLen);
		return 0;
	}
	else 
	{
		SDK_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceNameById serviceid[%d] msgid[%d] failed\n", dwServiceId, dwMsgId);
		return -1;
	}
}


int CAvenueServiceConfigs::GetServiceByName(const string & strServiceName, SSeriveConfig ** oSeriveConfig)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceByName [%s]\n", strServiceName.c_str());
	Qmap<string, SSeriveConfig>::iterator iter = m_mapServiceConfigByName.find(strServiceName);
	if(iter != m_mapServiceConfigByName.end())
	{
		*oSeriveConfig = &(iter->second);
		return 0;
	}
	else 
	{
		SDK_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceByName [%s] failed\n", strServiceName.c_str());
		return -1;
	}
}
int CAvenueServiceConfigs::GetServiceById(unsigned int dwServiceId, SSeriveConfig ** oSeriveConfig)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueServiceConfigs::GetServiceById serviceid[%d]\n", dwServiceId);
	
	Qmap<unsigned int, SSeriveConfig>::iterator iter = m_mapServiceConfigById.find(dwServiceId);
	if(iter != m_mapServiceConfigById.end())
	{
		*oSeriveConfig = &(iter->second);
		return 0;
	}
	else 
	{
		SDK_XLOG(XLOG_WARNING, "CAvenueServiceConfigs::GetServiceById [%d] failed\n", dwServiceId);
		return -1;
	}


}

void CAvenueServiceConfigs::Dump() const
{
	SDK_XLOG(XLOG_NOTICE, "############begin dump############\n");

	SDK_XLOG(XLOG_NOTICE, "mapServiceConfigByName,size[%d]\n",m_mapServiceConfigByName.size());
	Qmap<string, SSeriveConfig>::const_iterator itrService;
	for(itrService = m_mapServiceConfigByName.begin(); itrService != m_mapServiceConfigByName.end(); ++itrService)
	{
		const SSeriveConfig & oServiceConfig = itrService->second;
		SDK_XLOG(XLOG_DEBUG, "name[%s] id[%d] type[%d]\n", 
			oServiceConfig.strName.c_str(), oServiceConfig.dwId);
		oServiceConfig.oConfig.Dump();
		
	}
	SDK_XLOG(XLOG_NOTICE, "############end dump############\n");
}



}
}
