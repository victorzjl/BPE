#ifndef _REDIS_HELPER_H_
#define _REDIS_HELPER_H_

#include <string>
#include <vector>
using std::string;
using std::vector;

class CRedisHelper
{
public:
	CRedisHelper(){}
	~CRedisHelper(){}
	static void AddIpAddrs(string &strIpAddrs, const string &strAddr);
	static void StringSplit(const string &s, const string &delim, vector<string> &vecString);
};
#endif
