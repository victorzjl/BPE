#ifndef _I_REQUEST_AVENUE_HTTP_TRANSFER_H_
#define _I_REQUEST_AVENUE_HTTP_TRANSFER_H_
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

class IRequestAvenueHttpTransfer
{
public:
    virtual int RequestTransferAvenue2Http(IN const void * pAvenuePacket, IN int pAvenueLen, IN const char* szRemoteIp, IN const unsigned int dwRemotePort, 
    	OUT string &strHttpUrl, OUT void * pHttpReqBody,OUT int *pHttpBodyLen, OUT const string &strHttpMethod=string("POST"), IN const void *pInReserve=NULL, IN int nInReverseLen=0,
    	OUT void* pOutReverse=NULL, OUT int *pOutReverseLen=NULL) = 0;
			
    virtual int ResponseTransferHttp2Avenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequence, IN int nAvenueCode,IN int nHttpCode,IN const string &strHttpResponseBody,
    	OUT void * pAvenueResponsePacket, OUT int *pAvenueLen, IN const void *pInReserve=NULL, IN int nInReverseLen=0, OUT void* pOutReverse=NULL, OUT int *pOutReverseLen=NULL) = 0;

	virtual void CleanRubbish() = 0;
    virtual void GetServiceMessageId(vector<string> &vecServiceMsgId)=0; //for example 58001,1
    virtual string GetPluginSoInfo()=0;
    virtual ~IRequestAvenueHttpTransfer(){}
};
#endif


