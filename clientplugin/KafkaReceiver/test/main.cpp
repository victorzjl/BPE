#include "IAsyncVirtualClient.h"
#include "SapMessage.h"

#include <iostream>
#include <dlfcn.h>
#include <netinet/in.h>

using namespace sdo::sap;
typedef IAsyncVirtualClient* create_obj();
typedef void  destroy_obj(IAsyncVirtualClient*);

IAsyncVirtualClient* p_abc  = NULL;

bool isRun = true;
int msg_count = 0;

void ResponseServiceCallBack(IAsyncVirtualClient* pPlugIn, void*handle, const void *pBuffer, int dwLen)
{
	SSapMsgHeader *pHeader = (SSapMsgHeader *)pBuffer;
	std::cout << "ServiceId: " << ntohl(pHeader->dwServiceId) << std::endl;
	std::cout << "MessageId: " << ntohl(pHeader->dwMsgId) << std::endl;
	
	CSapEncoder request;
	request.SetService(0xA1, 40001, 1, 0);
	request.SetSequence(htonl(pHeader->dwSequence));
	p_abc->ResponseService(handle, request.GetBuffer(), request.GetLen());
	
	if (msg_count++>5)
	{
		isRun = false;
	}
}

void ExceptionWarnCallBack(const string &warning)
{
	std::cout << "ExceptionWarn: " << warning << std::endl;
}

void AsyncLogCallBack(int nModel, XLOG_LEVEL level,int nInterval,const string &strMsg,
		int nCode,const string& strAddr,int serviceId, int msgId)
{
	std::cout << "AsyncLog: " << strMsg << std::endl;
}


int main() 
{
    void* p_lib = dlopen("../libmqrecevier.so", RTLD_NOW);
    if (!p_lib) 
    {
            std::cout << dlerror() << std::endl;
            return 1;
    }

    create_obj* create_abc = (create_obj*)dlsym(p_lib, "create");

    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
            std::cout << dlsym_error << std::endl;
            return 1;
    }
    destroy_obj* destroy_abc = (destroy_obj*)dlsym(p_lib, "destroy");
    dlsym_error = dlerror();
    if (dlsym_error) 
    {
            std::cout << dlsym_error << std::endl;
            return 1;
    }
	
    p_abc = create_abc();
	p_abc->Initialize(ResponseServiceCallBack, ExceptionWarnCallBack, AsyncLogCallBack);
    
	while (true)
	{
		sleep(1);
	}
	
    destroy_abc(p_abc);
    dlclose(p_lib);
    return 0;
}

