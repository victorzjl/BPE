#include "IAsyncVirtualClient.h"
#include "SapMessage.h"
#include "MainLogHelper.h"
#include "CommonMacro.h"

#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#ifdef WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <atlbase.h>
    #include <atlconv.h>
    #include "resource.h"
#else
#include <dlfcn.h>
#include <pthread>
#endif


#define MAX_BUFFER_SIZE 4096
#define MAX_QUEUE_SIZE  1024
#define MAX_PATH_MAX 512

#define REQUEST_MUTEX_NAME "BPE_SIMULATOR_REQUEST_MUTEX"

using namespace sdo::sap;

typedef IAsyncVirtualClient* create_obj();
typedef void  destroy_obj(IAsyncVirtualClient *);

#ifdef WIN32
#define SDG_T_Mutex HANDLE
#define SDG_CREATE_MUTEX(mark) CreateMutex(NULL, FALSE, mark)
#define SDG_DESTORY_MUTEX(mutex) CloseHandle(mutex)
#define SDG_LOCK_MUTEX(mutex, time) WaitForSingleObject(mutex, time)
#define SDG_UNLOCK_MUTEX(mutex) ReleaseMutex(mutex)
#else
#define SDG_T_Mutex pthread_mutex_t
#define SDG_CREATE_MUTEX(mark) PTHREAD_MUTEX_INITIALIZER
#define SDG_DESTORY_MUTEX(mutex)
#define SDG_LOCK_MUTEX(mutex, time) pthread_mutex_lock(mutex)
#define SDG_UNLOCK_MUTEX(mutex) pthread_mutex_unlock(mutex)
#endif


static SDG_T_Mutex gs_mutex = NULL;
static bool gs_bSetTimer = false;
static bool gs_bIsServicing = false;


#ifdef WIN32
#define EXIT_CODE_CONFIG_ERROR ((DWORD   )0xE0000001L)
#define EXIT_CODE_SOCKET_ERROR ((DWORD   )0xE0000002L)
#define EXIT_CODE_MUTEX_ERROR  ((DWORD   )0xE0000003L)
#define EXIT_CODE_ENV_ERROR    ((DWORD   )0xE0000004L)
#define EXIT_CODE_PARAM_ERROR  ((DWORD   )0xE0000005L)
static DWORD g_dwExitCode = 0;

#define ID_TIMER 1
#define TIMER_INTERVAL 3000

static const LPCTSTR gs_szAppName = TEXT("BpeSimulator");
static const LPCTSTR gs_szWndName = TEXT("BpeSimulatorWin");

typedef struct stWindowData
{
    HINSTANCE _hInstance;
    HANDLE _hMutex;
    UINT_PTR _timerIdentifier;
    string _strMutexName;
    string _strConfigFile;
}SWindowData;

static SWindowData gs_sWindowData;

#define ID_TIMER 1
#define TIMER_INTERVAL 3000
#endif

typedef struct stTask
{
    IAsyncVirtualClient *_pPlugin;
    void *_pHandler;
    char _buffer[MAX_BUFFER_SIZE];
    int  _contentLength;
}STask;

typedef struct stPluginInfo
{
    PluginPtr _pHandle;
    create_obj *_pFunCreate;
    destroy_obj *_pFunDestroy;
    IAsyncVirtualClient *_pPlugin;
}SPluginInfo;

static SPluginInfo sPluginInfo;
static vector<STask *> taskQueue;


void Fun2RecevieReqFromPlugin(IAsyncVirtualClient* pPlugIn, void *pHandler, const void *pBuffer, int dwLen)
{
    MAIN_XLOG(XLOG_DEBUG, "%s, Plugin[0x%p], Handler[0x%p], Buffer[0x%p], Len[%d]\n", __FUNCTION__, pPlugIn, pHandler, pBuffer, dwLen);
    if (0 == dwLen)
    {
        MAIN_XLOG(XLOG_WARNING, "%s, The task content is empty!\n", __FUNCTION__);
        return;
    }

    if (dwLen > MAX_BUFFER_SIZE)
    {
        MAIN_XLOG(XLOG_WARNING, "%s, The task content is over the buffer threshold.\n", __FUNCTION__);
        return;
    }

    if (taskQueue.size() == MAX_QUEUE_SIZE)
    {
        MAIN_XLOG(XLOG_WARNING, "%s, The task queue is full.\n", __FUNCTION__);
        return;
    }

    STask *pTask = new STask;
    pTask->_pPlugin = pPlugIn;
    pTask->_pHandler = pHandler;
    pTask->_contentLength = dwLen;
    memcpy(pTask->_buffer, pBuffer, dwLen);
    MAIN_XLOG(XLOG_DEBUG, "%s, New task <%p> has been cached.\n", __FUNCTION__, pTask->_buffer);

    SDG_LOCK_MUTEX(gs_mutex, INFINITE);
    taskQueue.push_back(pTask);
    SDG_UNLOCK_MUTEX(gs_mutex);
}

void Fun2RecevieResFromPlugin(void *pHandler, const void *pBuffer, int dwLen)
{
    MAIN_XLOG(XLOG_DEBUG, "%s,  Handler[0x%p], Buffer[0x%p], Len[%d]\n", __FUNCTION__, pHandler, pBuffer, dwLen);
}

void HandleRequest()
{
    MAIN_XLOG(XLOG_DEBUG, "%s\n", __FUNCTION__);
    SDG_LOCK_MUTEX(gs_mutex, INFINITE);
    vector<STask *> taskCopy = taskQueue;
    taskQueue.clear();
    SDG_UNLOCK_MUTEX(gs_mutex);

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

        char szMsg[256] = { 0 };
        
        string strParam("Handle by BpeSimulator");
        /*request.GetValue(1, strParam);
        MAIN_XLOG(XLOG_DEBUG, "%s, Handle task: ServiceID[%d], MessageID[%d], Parameter[%s].\n", __FUNCTION__, serviceId, messageId, strParam.c_str());
        sprintf(szMsg, "Show Parameter: %s", strParam.c_str());*/

        CSapEncoder response;
        response.SetService(0xA1, serviceId, messageId, 0);
        response.SetSequence(sequence);
        response.SetValue(1000, strParam);
        sPluginInfo._pPlugin->ResponseService(pTask->_pHandler, response.GetBuffer(), response.GetLen());

        delete pTask;
        pTask = taskCopy[iIter] = NULL;
        ++iIter;
    }
    
}

void ExceptionWarnCallBack(const string &warning)
{
    MAIN_XLOG(XLOG_WARNING, "%s, ExceptionWarn: %s\n", __FUNCTION__, warning.c_str());
}

void AsyncLogCallBack(int nModel, XLOG_LEVEL level, int nInterval, const string &strMsg,
		              int nCode, const string &strAddr, int serviceId, int msgId)
{
    MAIN_XLOG(XLOG_DEBUG, "%s, AsyncLog: Msg[%s], Code[%d], Address[%s], ServiceID[%d], MessageID[%d]\n",
                      __FUNCTION__, strMsg.c_str(), nCode, strAddr.c_str(), serviceId, msgId);
}

int StartBpeService() 
{   
    MAIN_XLOG(XLOG_DEBUG, "%s, Start...\n", __FUNCTION__);

#ifdef WIN32
    gs_mutex = SDG_CREATE_MUTEX(REQUEST_MUTEX_NAME);
    if (!gs_mutex)
    {
        MAIN_XLOG(XLOG_DEBUG, "Fail to create mutex!\n", __FUNCTION__);
        SDG_DESTORY_MUTEX(gs_mutex);
        return 1;
    }
#endif

    sPluginInfo._pHandle = LoadLibrary("HttpRecevier.dll");
    if (!sPluginInfo._pHandle)
    {
        MAIN_XLOG(XLOG_WARNING, "%s, Fail to open shared object. ErrorCode[%d]\n", __FUNCTION__, GetLastError());
        return 1;
    }

    sPluginInfo._pFunCreate = (create_obj *)GetProcAddress(sPluginInfo._pHandle, "create");
    if (!sPluginInfo._pFunCreate)
    {
        MAIN_XLOG(XLOG_WARNING, "%s, Fail to find function 'create'. ErrorCode[%d]\n", __FUNCTION__, GetLastError());
        return 1;
    }

    sPluginInfo._pFunDestroy = (destroy_obj *)GetProcAddress(sPluginInfo._pHandle, "destroy");
    if (!sPluginInfo._pFunDestroy)
    {
        MAIN_XLOG(XLOG_WARNING, "%s, Fail to find function 'destroy'. ErrorCode[%d]\n", __FUNCTION__, GetLastError());
        return 1;
    }
	
    sPluginInfo._pPlugin = sPluginInfo._pFunCreate();
#ifdef WIN32
    int initRet = sPluginInfo._pPlugin->Initialize(gs_sWindowData._strConfigFile, Fun2RecevieReqFromPlugin, Fun2RecevieResFromPlugin, ExceptionWarnCallBack, AsyncLogCallBack);
#else
    int initRet = sPluginInfo._pPlugin->Initialize(Fun2RecevieReqFromPlugin, Fun2RecevieResFromPlugin, ExceptionWarnCallBack, AsyncLogCallBack);
#endif

    if (0 != initRet)
    {
        gs_bIsServicing = false;
#ifdef WIN32
        if (-1 == initRet)
        {
            g_dwExitCode = EXIT_CODE_CONFIG_ERROR;
        }
        else if (-2 == initRet)
        {
            g_dwExitCode = EXIT_CODE_SOCKET_ERROR;
        }
#endif
    }
    else
    {
        gs_bIsServicing = true;
    }

    return initRet;
}

int StopBpeService()
{
    if (gs_bIsServicing)
    {
        gs_bIsServicing = false;
        sPluginInfo._pFunDestroy(sPluginInfo._pPlugin);
        FreeLibrary(sPluginInfo._pHandle);
        SDG_DESTORY_MUTEX(gs_mutex);
    }
    return 0;
}


#ifdef WIN32

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE://窗口创建时候的消息
        if (0==StartBpeService())
        {
            ReleaseMutex(gs_sWindowData._hMutex);
            gs_sWindowData._timerIdentifier = SetTimer(hwnd, ID_TIMER, TIMER_INTERVAL, NULL);
        }
        else
        {
            StopBpeService();
            SendMessage(hwnd, WM_DESTROY, wParam, lParam);
        }
        break;
    case WM_TIMER:
        if (gs_sWindowData._timerIdentifier == wParam)
        {
            HandleRequest();
        }
        break;
    case WM_DESTROY://窗口销毁时候的消息
        KillTimer(hwnd, gs_sWindowData._timerIdentifier);
        PostQuitMessage(0);
        StopBpeService();
        break;
    default:
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

DWORD ParseCommandLine(LPSTR szCmdLine)
{
    int argc = 0;
    LPWSTR *lpszArgv = CommandLineToArgvW(CA2W(szCmdLine), &argc);
    if (NULL != lpszArgv)
    {
        char szKey[64] = { 0 },
            szValue[256] = { 0 };
        for (int i = 0; i < argc; ++i)
        {
            sscanf(CW2A(lpszArgv[i]), "--%[^=]=%s", szKey, szValue);
            if (boost::iequals(szKey, "mutex"))
            {
                gs_sWindowData._strMutexName = szValue;
            }
            else if (boost::iequals(szKey, "config"))
            {
                gs_sWindowData._strConfigFile = szValue;
            }
        }
        LocalFree(lpszArgv);
    }
    
    if (gs_sWindowData._strMutexName.empty() || gs_sWindowData._strConfigFile.empty())
    {
        return EXIT_CODE_PARAM_ERROR;
    }
    else
    {
        return 0;
    }
}

DWORD LockProcessMutex()
{
    int argc = 0;
    if (argc >= 1)
    {
        gs_sWindowData._hMutex = OpenMutex(MUTEX_ALL_ACCESS, false, gs_sWindowData._strMutexName.c_str());
        if (NULL == gs_sWindowData._hMutex)
        {
            return EXIT_CODE_MUTEX_ERROR;
        }
        
        if (WAIT_OBJECT_0 != WaitForSingleObject(gs_sWindowData._hMutex, INFINITE))
        {
            return EXIT_CODE_MUTEX_ERROR;
        }
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR szCmdLine, int iCmdShow)
{
    XLOG_INIT((boost::filesystem::initial_path<boost::filesystem::path>().string() + "/log.properties").c_str(), true);
    //注册日志模块
    XLOG_REGISTER(MAIN_MODULE, "BpeSimulator");

    gs_bIsServicing = false;

    DWORD iRet = ParseCommandLine(szCmdLine);
    if (0 != iRet)
    {
        return iRet;
    }
    

    iRet = LockProcessMutex();
    if (0 != iRet)
    {
        return iRet;
    }

    gs_sWindowData._hInstance = hInstance;
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINNATELY_LEAF));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = gs_szAppName;

    if (!RegisterClass(&wndclass))
    {
        return EXIT_CODE_ENV_ERROR;
    }

    HWND hwnd;
    // 此处使用WS_EX_TOOLWINDOW 属性来隐藏显示在任务栏上的窗口程序按钮
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
        gs_szAppName,
        gs_szWndName,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    ReleaseMutex(gs_sWindowData._hMutex);
    return g_dwExitCode;
}
#else
int main()
{
    XLOG_INIT((boost::filesystem::initial_path<boost::filesystem::path>().string() + "/log.properties").c_str(), true);
    //注册日志模块
    XLOG_REGISTER(MAIN_MODULE, "BpeSimulator");
    StartBpeService();
    while(true)
    {
        sleep(2);
        HandleRequest();
    }
    StopBpeService();
}
#endif
