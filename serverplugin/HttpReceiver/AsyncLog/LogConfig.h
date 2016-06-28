#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

#include <string>
#include <map>

using namespace std;

class LogConfig
{
public:
	int Load();
	int GetLogParam(const string& strUrl, string& strKey1, string& strKey2);

private:
	int ReadOneFile(const string& strFile);
	struct LagKeys
	{
		string strKey1;
		string strKey2;
	};
	map<string, LagKeys> m_mapKeys;
};

#endif // LOG_CONFIG_H
