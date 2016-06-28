#ifndef OSAP_CONFIG_H
#define OSAP_CONFIG_H

#include <string>
#include <map>
#include <vector>

using namespace std;

class COsapConfig
{
public:
	int Load();
	bool IsUrlInWhiteList(const string& strUrl);
	bool IsUrlInNoSaagList(const string& strUrl);
	string GetDefaultUserName(const string& strUrl);
	int LoadTmpWhiteList();
	bool IsUrlInTmpWhiteList(const string& strUrl);

private:
	map<string, string> m_mapUrlToDefautUser;
	vector<string> m_vecWhiteUrl;
	vector<string> m_vecNoSaagUrl;
	vector<string> m_vecTmpWhiteUrl;
};

#endif // OSAP_CONFIG_H
