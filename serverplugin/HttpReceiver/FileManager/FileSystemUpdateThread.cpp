#include "FileSystemUpdateThread.h"
#include <boost/algorithm/string.hpp>

class __DefaultHandler : public IFSUpdateHandler
{
public:
    virtual ~__DefaultHandler() {}
    virtual void OnReload() {}
    virtual void OnDirCreated(const string &) {}
    virtual void OnDirDeleted(const string &) {}
    virtual void OnDirModified(const string &) {}
    virtual void OnFileCreated(const string &) {}
    virtual void OnFileDeleted(const string &) {}
    virtual void OnFileModified(const string &) {}
};

#ifdef WIN32
#define NOTIFY_EVENTS FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_CREATION|FILE_NOTIFY_CHANGE_DIR_NAME
#endif

static const IFSUpdateHandlerPtr __pDefaultHandler(new __DefaultHandler);

CFSUpdateThread *CFSUpdateThread::s_instance = NULL;

CFSUpdateThread *CFSUpdateThread::GetInstance()
{
    if (NULL == s_instance)
    {
        s_instance = new CFSUpdateThread;
    }

    return s_instance;
}

CFSUpdateThread::CFSUpdateThread()
    : m_isAlive(false),
      m_nHandleCount(0)
{
    m_mapFunc[FS_NONE] = &CFSUpdateThread::DoNothing;
    m_mapFunc[FS_FILE_CREATED] = &CFSUpdateThread::DoFileCreated;
    m_mapFunc[FS_FILE_DELETED] = &CFSUpdateThread::DoFileDeleted;
    m_mapFunc[FS_FILE_MODIFIED] = &CFSUpdateThread::DoFileModified;
    m_mapFunc[FS_DIR_CREATED] = &CFSUpdateThread::DoDirCreated;
    m_mapFunc[FS_DIR_DELETED] = &CFSUpdateThread::DoDirDeleted;
    m_mapFunc[FS_DIR_MODIFIED] = &CFSUpdateThread::DoDirModified;

#ifdef WIN32
    for (unsigned int iIter = 0; iIter < MAX_DIR_NUM; ++iIter)
    {
        m_vecHandle[iIter] = NULL;
    }
#endif
}


void CFSUpdateThread::RegisterHander(const string &strAbsPath, IFSUpdateHandlerPtr pHandler)
{
    boost::lock_guard<boost::mutex> guard(m_mutex);
    if (pHandler == NULL)
    {
        return;
    }

#ifdef WIN32
    unsigned int nCurrentIndex = m_nHandleCount;
#endif

    map<string, IFSUpdateHandlerPtr>::iterator iter = m_mapPathAndHandler.find(strAbsPath);
    if (m_mapPathAndHandler.end() == m_mapPathAndHandler.find(strAbsPath))
    {
        if (MAX_DIR_NUM <= m_nHandleCount)
        {
            return;
        }
        m_nHandleCount++;
    }
    m_mapPathAndHandler[strAbsPath] = pHandler;


#ifdef WIN32
    HANDLE hWatcher = ::FindFirstChangeNotification(strAbsPath.c_str(), true, NOTIFY_EVENTS);
    m_mapPathWatcher[strAbsPath] = hWatcher;
    SDirProperty sDirProperty;
    sDirProperty._strAbsPath = strAbsPath;
    sDirProperty._pFSUpdateHandler = pHandler;
    gettimeofday_a(&sDirProperty._lastUpdatedTime, 0);
    m_mapDirProperty[hWatcher] = sDirProperty;
    m_vecHandle[nCurrentIndex] = hWatcher;
#endif
}

void CFSUpdateThread::UnregisterHander(const string &strAbsPath)
{
    boost::lock_guard<boost::mutex> guard(m_mutex);
    map<string, IFSUpdateHandlerPtr>::iterator iter = m_mapPathAndHandler.find(strAbsPath);
    if (iter != m_mapPathAndHandler.end())
    {
        m_mapPathAndHandler.erase(strAbsPath);
        m_nHandleCount--;
    }
    

#ifdef WIN32
    HANDLE hWatcher = m_mapPathWatcher[strAbsPath];
    if (NULL != hWatcher)
    {
        ::FindCloseChangeNotification(hWatcher);
        m_mapDirProperty.erase(hWatcher);
        m_mapPathWatcher.erase(strAbsPath);
    }

    map<string, HANDLE>::iterator iterHandle = m_mapPathWatcher.begin();
    for (unsigned int iIter = 0; iIter < MAX_DIR_NUM; ++iIter)
    {
        if (iterHandle != m_mapPathWatcher.end())
        {
            m_vecHandle[iIter] = iterHandle->second;
        }
        else
        {
            m_vecHandle[iIter] = NULL;
        }
    }
#endif
}


void CFSUpdateThread::OnDirCreated(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_DIR_CREATED);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::OnDirDeleted(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_DIR_DELETED);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::OnDirModified(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_DIR_MODIFIED);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::OnFileCreated(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_FILE_CREATED);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::OnFileDeleted(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_FILE_DELETED);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::OnFileModified(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_FILE_MODIFIED);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::OnLargeFileLoad(const string &strAbsPath)
{
    if (strAbsPath.empty())
    {
        return;
    }

    SFSMsg *pMsg = new SFSMsg(FS_LOAD);
    pMsg->_strAbsPath = strAbsPath;
    PutQ(pMsg);
}

void CFSUpdateThread::StartInThread()
{
    m_isAlive = true;
#ifdef WIN32
    while (m_isAlive)
    {
        vector<IFSUpdateHandlerPtr> vecNotifiedHandler;
        {
            boost::lock_guard<boost::mutex> guard(m_mutex);
            HANDLE *pHandle = m_vecHandle;
            unsigned int nRestCount = m_nHandleCount;
            unsigned int nScanCount = 0;
            DWORD dwRet = WaitForMultipleObjects(nRestCount, pHandle, false, 3);
            switch (dwRet)
            {
            case WAIT_TIMEOUT:
                break;
            case WAIT_FAILED:
                return;
            default:
                unsigned int nIndex = dwRet - WAIT_OBJECT_0;
                CheckWillUpdated(vecNotifiedHandler, m_mapDirProperty[m_vecHandle[nIndex]]);
                if (!FindNextChangeNotification(m_vecHandle[nIndex])) break;
                while (true)
                {
                    nRestCount = nRestCount - nIndex - 1;
                    if (nRestCount <= 0) break;
                    nScanCount = m_nHandleCount - nRestCount;
                    pHandle = pHandle + nIndex + 1;
                    dwRet = WaitForMultipleObjects(nRestCount, pHandle, false, 0);
                    switch (dwRet)
                    {
                    case WAIT_TIMEOUT:
                        break;
                    case WAIT_FAILED:
                        return;
                    default:
                        nIndex = dwRet - WAIT_OBJECT_0;
                        CheckWillUpdated(vecNotifiedHandler, m_mapDirProperty[m_vecHandle[nScanCount + nIndex]]);
                        if (!FindNextChangeNotification(m_vecHandle[nIndex])) break;
                    }
                }
                break;
            }
        }


        //Notify to Reload
        NotifyAllReload(vecNotifiedHandler);
    }
    
    
#endif
}

void CFSUpdateThread::Stop()
{
#ifdef WIN32
    boost::lock_guard<boost::mutex> guard(m_mutex);
    m_isAlive = false;
    for (unsigned int iIter = 0; iIter < m_nHandleCount; ++iIter)
    {
        FindCloseChangeNotification(m_vecHandle[iIter]);
        m_vecHandle[iIter] = NULL;
    }
    m_nHandleCount = 0;
    m_mapPathWatcher.clear();
    m_mapDirProperty.clear();
#endif
    m_mapPathAndHandler.clear();
}

#ifdef WIN32
void CFSUpdateThread::CheckWillUpdated(vector<IFSUpdateHandlerPtr> &vecNotifiedHandler, SDirProperty &sDirProperty)
{
    struct timeval_a now;
    gettimeofday_a(&now, 0);
    if (now.tv_sec - sDirProperty._lastUpdatedTime.tv_sec > 3) 
    {
        vecNotifiedHandler.push_back(sDirProperty._pFSUpdateHandler);
        sDirProperty._lastUpdatedTime = now;
    }
}
void CFSUpdateThread::NotifyAllReload(const vector<IFSUpdateHandlerPtr> &vecNotifiedHandler)
{
    for (vector<IFSUpdateHandlerPtr>::const_iterator iter = vecNotifiedHandler.begin();
        iter != vecNotifiedHandler.end(); ++iter)
    {
        (*iter)->OnReload();
    }
}
#endif

void CFSUpdateThread::Deal(void *pData)
{
    SFSMsg *pMsg = (SFSMsg *)pData;
    (this->*(m_mapFunc[pMsg->_type]))(pMsg);
    delete pMsg;
}

IFSUpdateHandlerPtr CFSUpdateThread::GetHandler(const string &strAbsPath)
{
    boost::lock_guard<boost::mutex> guard(m_mutex);
    for (map<string, IFSUpdateHandlerPtr>::iterator iter = m_mapPathAndHandler.begin();
        iter != m_mapPathAndHandler.end(); ++iter)
    {
        if (boost::istarts_with(iter->first, strAbsPath))
        {
            return iter->second;
        }
    }

    return __pDefaultHandler;
}

void CFSUpdateThread::DoDirCreated(SFSMsg *pMsg)
{
#ifdef SDG_FS_RELOAD
    GetHandler(pMsg->_strAbsPath)->OnReload();
#else
    GetHandler(pMsg->_strAbsPath)->OnDirCreated(pMsg->_strAbsPath);
#endif
}

void CFSUpdateThread::DoDirDeleted(SFSMsg *pMsg)
{
#ifdef SDG_FS_RELOAD
    GetHandler(pMsg->_strAbsPath)->OnReload();
#else
    GetHandler(pMsg->_strAbsPath)->OnDirDeleted(pMsg->_strAbsPath);
#endif
}

void CFSUpdateThread::DoDirModified(SFSMsg *pMsg)
{
#ifdef SDG_FS_RELOAD
    GetHandler(pMsg->_strAbsPath)->OnReload();
#else
    GetHandler(pMsg->_strAbsPath)->OnDirModified(pMsg->_strAbsPath);
#endif
}

void CFSUpdateThread::DoFileCreated(SFSMsg *pMsg)
{
#ifdef SDG_FS_RELOAD
    GetHandler(pMsg->_strAbsPath)->OnReload();
#else
    GetHandler(pMsg->_strAbsPath)->OnFileCreated(pMsg->_strAbsPath);
#endif
}

void CFSUpdateThread::DoFileDeleted(SFSMsg *pMsg)
{
#ifdef SDG_FS_RELOAD
    GetHandler(pMsg->_strAbsPath)->OnReload();
#else
    GetHandler(pMsg->_strAbsPath)->OnFileDeleted(pMsg->_strAbsPath);
#endif
}

void CFSUpdateThread::DoFileModified(SFSMsg *pMsg)
{
#ifdef SDG_FS_RELOAD
    GetHandler(pMsg->_strAbsPath)->OnReload();
#else
    GetHandler(pMsg->_strAbsPath)->OnFileModified(pMsg->_strAbsPath);
#endif
}

