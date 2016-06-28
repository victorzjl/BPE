#ifndef _I_HTTP_VIRTUAL_CLIENT_H_
#define	_I_HTTP_VIRTUAL_CLIENT_H_

#include <string>

using std::string;

class IHttpVirtualClient
{
public:
    virtual ~IHttpVirtualClient() {}
    virtual int OnPeerResponse(unsigned int id, const string& response) = 0;
    virtual int OnPeerClose(unsigned int id) = 0;
};

#endif	/* _I_HTTP_VIRTUAL_CLIENT_H_ */

