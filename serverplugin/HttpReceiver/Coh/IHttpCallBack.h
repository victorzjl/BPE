#ifndef _I_HTTP_CALL_BACK_H_
#define _I_HTTP_CALL_BACK_H_

#include <string>

using std::string;

class IHttpCallBack
{
public:
	virtual ~IHttpCallBack() {}

    virtual int OnRemoteResponse(unsigned int dwId, const string &strResponse) = 0;
};

#endif
