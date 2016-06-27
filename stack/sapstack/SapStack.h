#ifndef _SDO_SAP_STACK_H_
#define _SDO_SAP_STACK_H_
#include "ISapStack.h"
#include "SapStackThread.h"


namespace sdo{
namespace sap{
class CSapStack:public ISapStack
{
public:
	static CSapStack *Instance();
	virtual void Start(int nThread=3);
	virtual void Stop();
	boost::asio::io_service & GetIoService();
private:
	CSapStack();
	static CSapStack * sm_instance;
	CSapStackThread ** ppThreads;
	int m_nThread;
	unsigned int m_dwIndex;
	bool m_bRunning;
	boost::mutex m_mut;
};
}
}
#endif

