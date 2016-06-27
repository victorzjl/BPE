#ifndef _H_COH_PUB_H_
#define _H_COH_PUB_H_

#include <string>

using std::string;

typedef struct stHttpServerInfo
{
	string strIp;
	unsigned int dwPort;
	string strPath;
}SHttpServerInfo;

void ParseHttpUrl(const string &strUrl,SHttpServerInfo & oUrl);

#endif
