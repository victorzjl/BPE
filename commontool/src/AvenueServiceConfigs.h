#ifndef _AVENUE_SERVICE_CONFIGS_H_
#define _AVENUE_SERVICE_CONFIGS_H_

#include "ServiceConfig.h"
#include "QStl.h"


namespace sdo{
namespace commonsdk{

typedef struct stSeriveConfig
{
	string strName;
	unsigned int dwId;
	CServiceConfig oConfig;
}SSeriveConfig;

class CAvenueServiceConfigs
{
public:
	static CAvenueServiceConfigs * Instance();
	int LoadAvenueServiceConfigs(const string & strServiceConfigPath);
	int LoadAvenueServiceConfigByFile(const string &strConfigFile);

	//call by asc
	int GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId, unsigned int *pMsgId);
	int GetServiceIdByName(const string &strServiceName, unsigned int *pServiceId);
	int GetServiceNameById(unsigned int dwServiceId, unsigned int dwMsgId, string &strServiceName);
	
	int GetServiceByName(const string & strServiceName, SSeriveConfig ** oSeriveConfig);
	int GetServiceById(unsigned int dwServiceId, SSeriveConfig ** oSeriveConfig);


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
	Qmap<string, SSeriveConfig> m_mapServiceConfigByName;
	Qmap<unsigned int , SSeriveConfig> m_mapServiceConfigById;
};
}
}
#endif

