#ifndef _I_VIRTUAL_SERVICE_H_
#define _I_VIRTUAL_SERVICE_H_
#include <string>
#include <vector>
#include "ILog.h"
using std::string;
using std::vector;

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

typedef void (*ResponseService)(void *, const void *, unsigned int);
typedef void (*ExceptionWarn)(const string &);
typedef void (*AsyncLog)(XLOG_LEVEL level,int nInterval,const string &strMsg,
		int nCode,const string& strAddr,int serviceId, int msgId);

class IVirtualService
{
public:
	virtual int Initialize(ResponseService funcResponseService, 
                            ExceptionWarn funcExceptionWarn,
                            AsyncLog funcAsyncLog) = 0;
	virtual int RequestService(IN void *handler, IN const void *pInPacket, IN unsigned int nLen) = 0;
	virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum) = 0;
	virtual void GetServiceId(vector<unsigned int> &vecServiceIds) = 0;
	virtual const std::string OnGetPluginInfo()const = 0;
	virtual ~IVirtualService(){}
};

#endif // _I_VIRTUAL_SERVICE_H_

