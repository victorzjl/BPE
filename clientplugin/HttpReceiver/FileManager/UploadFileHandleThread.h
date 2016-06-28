#ifndef _UPLOAD_FILE_HANDLE_THREAD_H_
#define _UPLOAD_FILE_HANDLE_THREAD_H_

#include "MsgTimerThread.h"

#include <string>

using namespace sdo::common;
using std::string;

class CUploadFileHandleThread : public CMsgTimerThread
{
public:
    static CUploadFileHandleThread *GetInstance();

    virtual ~CUploadFileHandleThread();

    virtual void Deal(void *pData);
    virtual int OnSave(const string &strAbsPath, const string &strContent);

private:
    typedef enum eUFSMsgType
    {
        UFS_NONE = 0,
        UFS_SAVE = 1,
        UFS_ALL = 2
    }EUFSMsgType;

    typedef struct stUFSMsg
    {
        EUFSMsgType _type;
        string _strAbsPath;
        string _strContent;
        stUFSMsg() : _type(UFS_NONE){}
        stUFSMsg(EUFSMsgType type) : _type(type){}
        virtual ~stUFSMsg(){}
    }SUFSMsg;

    typedef bool (CUploadFileHandleThread::*UFSFunc)(SUFSMsg *);

private:
    CUploadFileHandleThread();
    virtual bool DoNothing(SUFSMsg *pMsg) { return true; }
    virtual bool DoSave(SUFSMsg *pMsg);

private:
    static CUploadFileHandleThread *s_pInstance;
    UFSFunc m_mapFunc[UFS_ALL];
};

#endif //_UPLOAD_FILE_HANDLE_THREAD_H_