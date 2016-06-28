#include "IAsyncVirtualClient.h"
#include "SapMessage.h"
#include "MainLogHelper.h"
#include "CommonMacro.h"

#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>

#ifdef WIN32
    #include <winsock2.h>
    #include <windows.h>
#else
#include <dlfcn.h>
#include <pthread>
#endif


#define MAX_BUFFER_SIZE 4096
#define MAX_QUEUE_SIZE  1024
#define MAX_PATH_MAX 512
#define IDR_EXIT 12
#define IDT_TIMER 1


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


static SDG_T_Mutex g_mutex;
static bool g_bSetTimer = false;

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

    SDG_LOCK_MUTEX(g_mutex, INFINITE);
    taskQueue.push_back(pTask);
    SDG_UNLOCK_MUTEX(g_mutex);
}

void Fun2RecevieResFromPlugin(void *pHandler, const void *pBuffer, int dwLen)
{
    MAIN_XLOG(XLOG_DEBUG, "%s,  Handler[0x%p], Buffer[0x%p], Len[%d]\n", __FUNCTION__, pHandler, pBuffer, dwLen);
}

void HandleRequest()
{
    MAIN_XLOG(XLOG_DEBUG, "%s\n", __FUNCTION__);
    SDG_LOCK_MUTEX(g_mutex, INFINITE);
    vector<STask *> taskCopy = taskQueue;
    taskQueue.clear();
    SDG_UNLOCK_MUTEX(g_mutex);

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
        MAIN_XLOG(XLOG_DEBUG, "%s, Handle task: ServiceID[%d], MessageID[%d], Parameter[%s].\n", __FUNCTION__, serviceId, messageId, param.c_str());
        param = "Show Parameter: " + param;
        CSapEncoder response;
        response.SetService(0xA1, serviceId, messageId, 0);
        response.SetSequence(sequence);
        response.SetValue(1000, param);
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
    XLOG_INIT((boost::filesystem::initial_path<boost::filesystem::path>().string() + "/log.properties").c_str(), true);
    //注册日志模块
    XLOG_REGISTER(MAIN_MODULE, "BpeSimulator");

    MAIN_XLOG(XLOG_DEBUG, "%s, Start...\n", __FUNCTION__);

    g_mutex = SDG_CREATE_MUTEX("g_mutex_1");

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

    sPluginInfo._pPlugin->Initialize(Fun2RecevieReqFromPlugin, Fun2RecevieResFromPlugin, ExceptionWarnCallBack, AsyncLogCallBack);

    return 0;
}

int StopBpeService()
{
    sPluginInfo._pFunDestroy(sPluginInfo._pPlugin);
    FreeLibrary(sPluginInfo._pHandle);
    SDG_DESTORY_MUTEX(g_mutex);
    return 0;
}


#ifdef WIN32
#define NAME "BpeSimulatorTray"
static LPCTSTR szAppName = TEXT(NAME);
static LPCTSTR szWndName = TEXT(NAME);
static HMENU hmenu = NULL;//菜单句柄
static HANDLE hMutex = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static NOTIFYICONDATA nid;
    static UINT WM_TASKBARCREATED;
    POINT pt;//用于接收鼠标坐标
    int optId;//用于接收菜单选项返回值

    // 不要修改TaskbarCreated，这是系统任务栏自定义的消息
    WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
    switch (message)
    {
    case WM_CREATE://窗口创建时候的消息.
        StartBpeService();
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = 0;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        lstrcpy(nid.szTip, szAppName);
        Shell_NotifyIcon(NIM_ADD, &nid);
        hmenu = CreatePopupMenu();//生成菜单
        AppendMenu(hmenu, MF_STRING, IDR_EXIT, "退出");//为菜单添加退出选项
        if (!g_bSetTimer)
        {
            if (::SetTimer(hwnd, IDT_TIMER, 2000, NULL) == 0)
            {
                StopBpeService();
                ::MessageBox(hwnd, "定时器安装失败", "系统提示", MB_OK);
                Shell_NotifyIcon(NIM_DELETE, &nid);
                PostQuitMessage(0);
            }
            else
            {
                g_bSetTimer = TRUE;
            }
        }
        break;
    case WM_USER://连续使用该程序时候的消息.
        if (lParam == WM_LBUTTONDOWN)
            MessageBox(hwnd, TEXT("Thanks for using Bpe-Simulator!"), szAppName, MB_OK);
        if (lParam == WM_RBUTTONDOWN)
        {
            GetCursorPos(&pt);//取鼠标坐标
            ::SetForegroundWindow(hwnd);//解决在菜单外单击左键菜单不消失的问题
            optId = TrackPopupMenu(hmenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hwnd, NULL);//显示菜单并获取选项ID
            if (optId == IDR_EXIT)
            { 
                StopBpeService();
                Shell_NotifyIcon(NIM_DELETE, &nid);
                KillTimer(hwnd, IDT_TIMER);
                PostQuitMessage(0);   
            }
        }
        break;
    case WM_TIMER:
        if (wParam == IDT_TIMER)
        {
            HandleRequest();
        }
        break;
    case WM_DESTROY://窗口销毁时候的消息.
        StopBpeService();
        Shell_NotifyIcon(NIM_DELETE, &nid);
        ::KillTimer(hwnd, IDT_TIMER);
        PostQuitMessage(0);  
        break;
    default:
        /*
        * 防止当Explorer.exe 崩溃以后，程序在系统系统托盘中的图标就消失
        *
        * 原理：Explorer.exe 重新载入后会重建系统任务栏。当系统任务栏建立的时候会向系统内所有
        * 注册接收TaskbarCreated 消息的顶级窗口发送一条消息，我们只需要捕捉这个消息，并重建系
        * 统托盘的图标即可。
        */
        if (message == WM_TASKBARCREATED)
            SendMessage(hwnd, WM_CREATE, wParam, lParam);
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR szCmdLine, int iCmdShow)
{
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;

    HWND handle = FindWindow(NULL, szWndName);
    
    if (handle != NULL)
    {  
        MessageBox(NULL, TEXT("Application is already running"), szAppName, MB_ICONERROR);
        return 0;
    }

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass))
    {  
        MessageBox(NULL, TEXT("This program requires Windows NT!"), szAppName, MB_ICONERROR);
        return 0;
    }

    // 此处使用WS_EX_TOOLWINDOW 属性来隐藏显示在任务栏上的窗口程序按钮
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
        szAppName, szWndName,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
#else
int main()
{
    StartBpeService();
    while(true)
    {
        sleep(2);
        HandleRequest();
    }
    StopBpeService();
}
#endif