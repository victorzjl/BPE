#ifndef _I_SDO_SAP_STACK_H_
#define _I_SDO_SAP_STACK_H_


namespace sdo{
namespace sap{
class ISapStack
{
public:
	virtual void Start(int nThread=3)=0;
	virtual void Stop()=0;
	virtual ~ISapStack(){}
};
ISapStack * GetSapStackInstance();
}
}
#endif

