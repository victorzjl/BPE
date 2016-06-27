#ifndef __HTTP_CLIENT_BASE_H__
#define __HTTP_CLIENT_BASE_H__
#include "CohPub.h"
//#include "MessageDealer.h"

#include <string>
using std::string;

class CMessageDealer;

namespace HT_COH{

class HttpClientBase
{
public:
    virtual void Init(int nId, string& strUrl,CMessageDealer* m_pMD) = 0;
	virtual void SendRequest(const string &strContent) = 0;
    virtual ~HttpClientBase(){}
protected:	
	SHttpServerInfo m_oServerUrl;
	int m_nId; /*判断当前处理的是第几个对象*/
	CMessageDealer* m_Dealer;
};

}


#endif

