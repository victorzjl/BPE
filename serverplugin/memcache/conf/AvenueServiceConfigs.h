#ifndef _AVENUE_SERVICE_CONFIGS_H_
#define _AVENUE_SERVICE_CONFIGS_H_

#include "ServiceConfig.h"

typedef struct stSeriveConfig
{
	string strName;
	unsigned int dwId;
	CServiceConfig oConfig;
}SServiceConfig;

class CAvenueServiceConfigs
{
public:
	static CAvenueServiceConfigs * Instance();
	bool IsExist(unsigned int dwServiceId, unsigned int dwMsgId);
	int LoadAvenueServiceConfigs(const string & strServiceConfigPath);
	int LoadAvenueServiceConfigByFile(const string &strConfigFile);

	int GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId, unsigned int *pMsgId);
	int GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId);
	int GetServiceNameById(unsigned int dwServiceId, unsigned int dwMsgId, string &strServiceName);
	
	int GetServiceByName(const string & strServiceName, SServiceConfig ** oSeriveConfig);
	int GetServiceById(unsigned int dwServiceId, SServiceConfig ** oSeriveConfig);

	map<unsigned int, SServiceConfig>& GetServiceConfigByIdMap(){return m_mapServiceConfigById;}
	void Dump() const;
	CAvenueServiceConfigs() {}
	~CAvenueServiceConfigs() 
	{
		if(s_pInstance)
		{
			delete s_pInstance;
		}
	}
private:
	static CAvenueServiceConfigs *s_pInstance;
	map<string, SServiceConfig> m_mapServiceConfigByName;
	map<unsigned int , SServiceConfig> m_mapServiceConfigById;
};

#endif

