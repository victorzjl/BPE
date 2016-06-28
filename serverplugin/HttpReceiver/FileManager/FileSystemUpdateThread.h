#ifndef _FILE_SYSTEM_UPDATE_THREAD_H_
#define _FILE_SYSTEM_UPDATE_THREAD_H_

#include "MsgTimerThread.h"
#include "IFileSystemSUpdateHandler.h"

#include <string>
#include <map>
#include <vector>
#include <boost/thread/mutex.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#define MAX_DIR_NUM 20

using std::string;
using std::map;
using std::vector;
using namespace sdo::common;

class CFSUpdateThread : public CMsgTimerThread
{
public:
    static CFSUpdateThread *GetInstance();

public:

    void RegisterHander(const string &strAbsPath, IFSUpdateHandlerPtr pHandler);
    void UnregisterHander(const string &strAbsPath);

    virtual void OnDirCreated(const string &strAbsPath);
    virtual void OnDirDeleted(const string &strAbsPath);
    virtual void OnDirModified(const string &strAbsPath);
    virtual void OnFileCreated(const string &strAbsPath);
    virtual void OnFileDeleted(const string &strAbsPath);
    virtual void OnFileModified(const string &strAbsPath);
    virtual void OnLargeFileLoad(const string &strAbsPath);

    virtual void StartInThread();
    virtual void Stop();
    virtual void Deal(void *pData);

private:
    typedef enum eFSMsgType 
    { 
        FS_NONE = 0,
        FS_LOAD = 1, 
        FS_FILE_CREATED = 2,
        FS_FILE_DELETED = 3,
        FS_FILE_MODIFIED = 4,
        FS_DIR_CREATED = 5,
        FS_DIR_DELETED = 6,
        FS_DIR_MODIFIED = 7,
        FS_ALL = 8
    }EFSMsgType;

    typedef struct stFSMsg
    {
        string _strAbsPath;
        EFSMsgType _type;
        stFSMsg() : _type(FS_NONE){}
        stFSMsg(EFSMsgType type) : _type(type){}
        virtual ~stFSMsg(){}
    }SFSMsg;

    typedef void (CFSUpdateThread::*FSUpdateFunc)(SFSMsg *);


private:
    CFSUpdateThread();
    IFSUpdateHandlerPtr GetHandler(const string &strAbsPath);
    virtual void DoNothing(SFSMsg *pMsg) {}
    virtual void DoDirCreated(SFSMsg *pMsg);
    virtual void DoDirDeleted(SFSMsg *pMsg);
    virtual void DoDirModified(SFSMsg *pMsg);
    virtual void DoFileCreated(SFSMsg *pMsg);
    virtual void DoFileDeleted(SFSMsg *pMsg);
    virtual void DoFileModified(SFSMsg *pMsg);

private:
    static CFSUpdateThread *s_instance;  
    FSUpdateFunc m_mapFunc[FS_ALL];
    map<string, IFSUpdateHandlerPtr> m_mapPathAndHandler;

    bool m_isAlive;
    boost::mutex m_mutex;
    unsigned int m_nHandleCount;

#ifdef WIN32
    typedef struct stProperty
    {
        string _strAbsPath;
        struct timeval_a _lastUpdatedTime;
        IFSUpdateHandlerPtr _pFSUpdateHandler;
    }SDirProperty;
    map<string, HANDLE> m_mapPathWatcher;
    map<HANDLE, SDirProperty> m_mapDirProperty;
    HANDLE m_vecHandle[MAX_DIR_NUM];

private:
    void CheckWillUpdated(vector<IFSUpdateHandlerPtr> &vecNotifiedHandler, SDirProperty &sDirProperty);
    void NotifyAllReload(const vector<IFSUpdateHandlerPtr> &vecNotifiedHandler);
#else
#endif
};

#endif //_FILE_SYSTEM_UPDATE_THREAD_H_
