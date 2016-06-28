#ifndef _IAVENUE_HTTP_TRANSFER_H_
#define _IAVENUE_HTTP_TRANSFER_H_
#include "LogManager.h"
#include <vector>
#include <string>
using std::vector;
using std::string;

#ifndef SHARED_SO_MODULE
#define SHARED_SO_MODULE 102 
DEFINE_MODULE_XLOG(SHARED_SO_MODULE,SO_XLOG)
#define SO_SLOG(Level,Event) SLOG(SHARED_SO_MODULE,Level,Event)
#endif


#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

class IAvenueHttpTransfer
{
public:
	virtual void GetUriMapping(vector<string> &vecMapping)=0;

    virtual int RequestTransfer(IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN unsigned int dwSequence,  IN const char *szUriCommand, IN const char *szUriAttribute,
    	IN const char* szRemoteIp, IN const unsigned int dwRemotePort, OUT void * pAvenuePacket,OUT int *pLen) = 0;
			
    virtual int ResponseTransfer(IN const void *pAvenuePacket,IN unsigned int nLen,
		OUT void * pHttpPack,OUT int *pLen) = 0;

	virtual void CleanRubbish() = 0;
    virtual void GetServiceId(vector<unsigned int> &vecServiceIds)=0;
    virtual string GetPluginSoInfo()=0;
    virtual ~IAvenueHttpTransfer(){}
};
#endif

