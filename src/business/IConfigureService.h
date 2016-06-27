#ifndef _I_CONFIGURE_SERVICE_H_
#define _I_CONFIGURE_SERVICE_H_
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

typedef void (*SetLocalCache)(const void *, int);//avenue packet, packet len

        
class IConfigureService
{
public:
	virtual int Init(SetLocalCache fnLocalCache) = 0;
	virtual int Load() = 0;
	virtual int FastLoad() = 0;
	virtual ~IConfigureService(){}
};
#endif 
