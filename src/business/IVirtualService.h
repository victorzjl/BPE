#ifndef _I_VIRTUAL_SERVICE_H_
#define _I_VIRTUAL_SERVICE_H_
#include <string>
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

class IVirtualService
{
public:
        virtual int RequestService(IN const void *pInPacket, IN unsigned int nLen,
            OUT void **ppOutPacket, OUT int *pLen) = 0;        
        virtual void SetData(IN const void *pInPacket, IN unsigned int nLen){}        
        virtual void GetServiceIds(const unsigned **idarray, int& len)=0;
        virtual const std::string OnGetPluginInfo()const = 0;       
        virtual ~IVirtualService(){}
};

#endif // _I_VIRTUAL_SERVICE_H_

