#include "IAsyncVirtualClient.h"
#include "SapMessage.h"
#include "LogManager.h"

#include <iostream>
#include <dlfcn.h>
#include <netinet/in.h>
#include <vector>
#include <pthread.h>

using namespace sdo::sap;

typedef IAsyncVirtualClient* create_obj();
typedef void  destroy_obj(IAsyncVirtualClient *);

#define MAX_BUFFER_SIZE 4096
#define MAX_QUEUE_SIZE  1024
#define MAX_PATH_MAX 512

typedef struct stTask
{
    IAsyncVirtualClient *_pPlugIn;
    void *_pHandler;
    char _buffer[MAX_BUFFER_SIZE];
    int  _contentLength;
}STask;

static IAsyncVirtualClient *p_plugin  = NULL;
static vector<STask *> taskQueue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

void ResponseServiceCallBack(IAsyncVirtualClient* pPlugIn, void *pHandler, const void *pBuffer, int dwLen)
{
    printf(">>>[TEST] [%s]\n", __FUNCTION__);
    if (0 == dwLen)
    {
        printf(">>>[TEST] [%s] The task content is empty.\n", __FUNCTION__);
        return;
    }

    if (dwLen > MAX_BUFFER_SIZE)
    {
        printf(">>>[TEST] [%s] The task content is over the memory threshold.\n", __FUNCTION__);
        return;
    }

    if (taskQueue.size() == MAX_QUEUE_SIZE)
    {
        printf(">>>[TEST] [%s] The task queue is full.\n", __FUNCTION__);
        return;
    }
    
    STask *pTask = new STask;
    pTask->_pPlugIn = pPlugIn;
    pTask->_pHandler = pHandler;
    pTask->_contentLength = dwLen;
    memcpy(pTask->_buffer, pBuffer, dwLen);
    printf(">>>[TEST] [%s] new task <%p>.\n", __FUNCTION__, pTask->_buffer);

    pthread_mutex_lock(&mutex);
    taskQueue.push_back(pTask);
    pthread_mutex_unlock(&mutex);
}

void HandleRequest()
{
    //printf(">>>[TEST] [%s]\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    vector<STask *> taskCopy = taskQueue;
    taskQueue.clear();
    pthread_mutex_unlock(&mutex);

    unsigned int size = taskCopy.size();
    unsigned int iIter = 0;
    while (iIter < size)
    {
        STask *pTask = taskCopy[iIter];  
        CSapDecoder request(pTask->_buffer, pTask->_contentLength);
        request.DecodeBodyAsTLV();
        int serviceId = request.GetServiceId();
        int messageId = request.GetMsgId();
        int sequence = request.GetSequence();
        string param;
        request.GetValue(1, param);
        printf(">>>[TEST] [%s] Handle task: ServiceID[%d], MessageID[%d], Parameter[%s].\n", __FUNCTION__, serviceId, messageId, param.c_str());
        
        param = "Show Parameter: " + param;
        CSapEncoder response;
        response.SetService(0xA1, serviceId, messageId, 0);
        response.SetSequence(sequence);
        response.SetValue(1000, param);
        p_plugin->ResponseService(pTask->_pHandler, response.GetBuffer(), response.GetLen());

        delete pTask;
        pTask = taskCopy[iIter] = NULL;
        ++iIter;
    }
    
}

void ExceptionWarnCallBack(const string &warning)
{
    printf(">>>[TEST] [%s] ExceptionWarn: %s\n", __FUNCTION__, warning.c_str());
}

void AsyncLogCallBack(int nModel, XLOG_LEVEL level, int nInterval, const string &strMsg,
		              int nCode, const string &strAddr, int serviceId, int msgId)
{
    printf(">>>[TEST] [%s] AsyncLog: Msg[%s], Code[%d], Address[%s], ServiceID[%d], MessageID[%d]\n", 
                      __FUNCTION__, strMsg.c_str(), nCode, strAddr.c_str(), serviceId, msgId);
}

char* GetExeName()
{
    static char buf[MAX_PATH_MAX] = {0};
    int rslt = readlink("/proc/self/exe", buf, MAX_PATH_MAX);
    if(rslt < 0 || rslt >= MAX_PATH_MAX)
    {
        return NULL;
    }
    buf[rslt] = '\0';
    return buf;
}

string GetCurrentPath()
{
    static char buf[MAX_PATH_MAX];
    strcpy(buf, GetExeName());
    char *pStart = buf + strlen(buf) - 1;
    char *pEnd = buf;

    if(pEnd < pStart)
    {
        while(pEnd != --pStart)
        {
            if('/' == *pStart)
            {
                *++pStart = '\0';
                break;
            }
        }
    }
    return buf;
}

int main() 
{   
    void *p_lib = dlopen("../../libhttprecevier.so", RTLD_NOW);
    if (!p_lib) 
    {
        printf(">>>[TEST] fail to open shared object, info:%s\n", dlerror());
        return 1;
    }

    create_obj *create_func = (create_obj *)dlsym(p_lib, "create");
    if (!create_func)
    {
        printf(">>>[TEST] fail to find function 'create', info:%s\n", dlerror());
        return 1;
    }

    destroy_obj *destroy_func = (destroy_obj *)dlsym(p_lib, "destroy");
    if (!destroy_func) 
    {
        printf(">>>[TEST] fail to find function 'destroy', info:%s\n", dlerror());
        return 1;
    }
	
    p_plugin = create_func();

    p_plugin->Initialize(ResponseServiceCallBack, NULL, ExceptionWarnCallBack, AsyncLogCallBack);
    while (true)
    {
        HandleRequest();
    }
	
    destroy_func(p_plugin);
    dlclose(p_lib);
    return 0;
}

