#ifndef _H_BUSINESS_COH_CLIENT_IMP_H_
#define _H_BUSINESS_COH_CLIENT_IMP_H_
#include "CohClient.h"
#include "HttpClientBase.h"
#include "MessageDealer.h"


//class CMessageDealer;
using sdo::coh::ICohClient;
using namespace HT_COH;

namespace HT_COH{
	
class CBusinessCohClientImp:public ICohClient,public HttpClientBase
{
public:
    void Init(int nId, string& strUrl,CMessageDealer* m_pMD);
	void SendRequest(const string &strContent);
	void OnReceiveResponse(const string &strResponse);
};

}

#endif

