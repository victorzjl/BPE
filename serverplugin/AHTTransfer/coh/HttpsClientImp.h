#ifndef __HTTPS_CLIENT_IMP_H__
#define __HTTPS_CLIENT_IMP_H__
#include "HttpClientBase.h"
#include "MessageDealer.h"
#include <stdlib.h>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

class CHttpsClientImp:public HttpClientBase
{
public:	
    void Init(int nId, string& strUrl,CMessageDealer* m_pMD);
	void SendRequest(const string &strContent);
	void OnReceiveResponse(const string &strResponse);
    static void SetTimeout(int nTimeout);
};

#endif

