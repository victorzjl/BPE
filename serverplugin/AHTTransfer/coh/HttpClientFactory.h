#ifndef __HTTP_CLIENT_FACTORY_H__
#define __HTTP_CLIENT_FACTORY_H__
#include "HttpsClientImp.h"
#include "BusinessCohClientImp.h"

class CHttpClientFactory
{
public:
    static HttpClientBase* Create(const string& url)
    {
        if (url.find("https://")!=string::npos)
        {
            return new CHttpsClientImp;
        }
        if(url.find("http://")!=string::npos)
        {
            return new CBusinessCohClientImp;
        }
        return new CBusinessCohClientImp;;
    }
    static void SetTimeout(int timeout)
    {
        CBusinessCohClientImp::SetTimeout(timeout);
        CHttpsClientImp::SetTimeout(timeout);
    }
};
#endif

