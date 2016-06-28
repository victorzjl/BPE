#ifndef PRE_PROCESS_H
#define PRE_PROCESS_H

#include "HpsCommonInner.h"
#include <string>
#include <vector>

using namespace std;

class PreProcess
{
public:
	int LoadConfig(const string& strConfig);
	int Process(SRequestInfo& sReq);

private:
	struct PreInfo
	{
		string strUrl;
		int nPathCount;
		int nParamIndex;
		string strParamName;
	};
	vector<PreInfo> m_vecUrlList;
};

#endif // PRE_PROCESS_H
