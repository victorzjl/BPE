#include "IAsyncVirtualService.h"
#include "SapMessage.h"

#include <iostream>
#include <dlfcn.h>

using namespace sdo::sap;
typedef IAsyncVirtualService* create_obj();
typedef void  destroy_obj(IAsyncVirtualService*);

void ResponseServiceCallBack(void *handle, const void *pBuffer, int dwLen)
{
	std::string out((const char *)pBuffer, (size_t)dwLen);
	std::cout << "Response: " << out << std::endl;
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

void RunPutMessage(IAsyncVirtualService* p_abc);

int main() 
{
    void* p_lib = dlopen("../libmqsender.so", RTLD_NOW);
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
    IAsyncVirtualService* p_abc = create_abc();
	p_abc->Initialize(ResponseServiceCallBack, ExceptionWarnCallBack, AsyncLogCallBack);
    RunPutMessage(p_abc);  
    
    destroy_abc(p_abc);
    dlclose(p_lib);
    return 0;
}

void RunPutMessage(IAsyncVirtualService* p_abc)
{  
	std::string message = "another message";

	/* unsigned long count = 100;
	
	std::cout << "Enter message times: ";
	std::cin >> count; */

	//while (count--)
    while(1)
	{
		std::cout << "Input: ";
		std::cin >> message;
		if (message == "FINISH")
		{
			break;
		}
		else 
		{
			CSapEncoder request;
			request.SetService(0xA1, 40001, 1);
			request.SetSequence(3);
			
			char szExHead[128] = {0};
			snprintf(szExHead, sizeof(szExHead), message.c_str());
			request.SetExtHeader(szExHead, strlen(szExHead));
			p_abc->RequestService(NULL, request.GetBuffer(), request.GetLen());
		}
	}
}

