#ifndef _IP_LIMIT_MANAGER_H_
#define _IP_LIMIT_MANAGER_H_

#include "HpsCommon.h"

#include<string>
#include<map>
using std::string;
using std::map;
using std::make_pair;



class CIpLimitLoader
{
public:
	static CIpLimitLoader * GetInstance();
	
	
	~CIpLimitLoader(){}
	int LoadConfig(const string &strFile, map<string, string> &mapIpsByUrl);
	int LoadConfig(const string &strDbConn, const string &strFile, map<string, string> &mapIpsByUrl);
	int ReLoadConfig(map<string, string> &mapIpsByUrl);
public:
	void Dump();

private:
	int LoadConfigFromDb();
	int LoadConfigFromFile(const string &strFile);
	int WriteConfigToFile(const string &strFile);
	int WriteConfigToXmlFile(const string &strFile);
	int ParseConnectString(const string &strConn, string &strHost, string &strUser, string &strPwd, string &strDb, int &nPort);
	
private:
	CIpLimitLoader(){}

	static CIpLimitLoader * sm_pInstance;
	
	string m_strFile;
	map<string, string> m_mapIpsByUrl;
};

class CIpLimitCheckor
{
public:
	void UpdateIps(const map<string, string> &mapIpsByUrl){m_mapIpsByUrl = mapIpsByUrl;}
	int CheckIpLimit(const string &strUrl, const string &strIp);

private:
	map<string, string> m_mapIpsByUrl;

};

#endif


