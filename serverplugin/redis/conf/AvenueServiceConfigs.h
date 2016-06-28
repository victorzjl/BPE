#ifndef _AVENUE_SERVICE_CONFIGS_H_
#define _AVENUE_SERVICE_CONFIGS_H_

#include "ServiceConfig.h"

namespace redis{

typedef struct stSeriveConfig
{
	string strName;
	unsigned int dwId;
	CServiceConfig oConfig;
}SServiceConfig;

class CAvenueServiceConfigs
{
public:
	CAvenueServiceConfigs() {}
	~CAvenueServiceConfigs(){}
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
	
private:
	CAvenueServiceConfigs(const CAvenueServiceConfigs &);
	CAvenueServiceConfigs &  operator = (const CAvenueServiceConfigs &);
private:
	map<string, SServiceConfig> m_mapServiceConfigByName;
	map<unsigned int , SServiceConfig> m_mapServiceConfigById;
};

}
#endif

