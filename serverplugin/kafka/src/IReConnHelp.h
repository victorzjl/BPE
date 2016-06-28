#ifndef _I_RECONN_HELPER_H_
#define _I_RECONN_HELPER_H_

#include <string>

#include "AsyncVirtualServiceLog.h"

#define RECONN_XLOG MQS_XLOG

class IReConnWarnRecevier
{
public:
	virtual int ReConnWarn(const std::string& warnning) = 0;
	virtual ~IReConnWarnRecevier() {}
};

class IReConnOwner
{
public:
	virtual int OnReConn(void *pContext) = 0;
	virtual ~IReConnOwner() {}
};

class IReConnHelper
{
public:
	virtual int TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3) = 0;
	virtual int ReConnect(void *pContext, std::string& outMsg, unsigned int timeout=3) = 0;
	virtual ~IReConnHelper() {}
};

#endif //_I_RECONN_HELPER_H_
