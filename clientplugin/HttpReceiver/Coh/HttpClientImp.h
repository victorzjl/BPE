#ifndef _HTTP_CLIENT_IMP_H_
#define _HTTP_CLIENT_IMP_H_


#include <string>
#include <boost/shared_ptr.hpp>
#include "CohClient.h"
#include "IHttpCallBack.h"


using namespace sdo::coh;
using std::string;

class CHttpClientImp : public ICohClient
{
public:
    static void SetTimeout(unsigned int timeout);

public:
	CHttpClientImp(IHttpCallBack *pCallBack, unsigned int dwId) : m_pCallBack(pCallBack), m_dwId(dwId){}
	virtual ~CHttpClientImp() {}

	virtual void OnReceiveResponse(const string &strResponse);

private:
    static unsigned int s_dwTimeout;

private:
	IHttpCallBack *m_pCallBack;
	unsigned int m_dwId;
};

typedef boost::shared_ptr<CHttpClientImp> HttpClientPtr;
#endif //_HTTP_CLIENT_IMP_H_

