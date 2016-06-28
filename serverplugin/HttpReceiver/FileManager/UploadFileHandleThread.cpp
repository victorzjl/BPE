#include "UploadFileHandleThread.h"
#include "FileManagerLogHelper.h"
#include <boost/filesystem.hpp>
#include <fstream>

CUploadFileHandleThread *CUploadFileHandleThread::s_pInstance = NULL;

CUploadFileHandleThread *CUploadFileHandleThread::GetInstance()
{
    if (NULL == s_pInstance)
    {
        s_pInstance = new CUploadFileHandleThread;
    }

    return s_pInstance;
}


CUploadFileHandleThread::CUploadFileHandleThread()
{
    m_mapFunc[UFS_NONE] = &CUploadFileHandleThread::DoNothing;
    m_mapFunc[UFS_SAVE] = &CUploadFileHandleThread::DoSave;
}

CUploadFileHandleThread::~CUploadFileHandleThread()
{}

int CUploadFileHandleThread::OnSave(const string &strAbsPath, const string &strContent)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s, File[%s]\n", __FUNCTION__, strAbsPath.c_str());
    SUFSMsg *pMsg = new SUFSMsg(UFS_SAVE);
    pMsg->_strAbsPath = strAbsPath;
    pMsg->_strContent = strContent;
    PutQ(pMsg);
    return 0;
}

void CUploadFileHandleThread::Deal(void *pData)
{
    SUFSMsg *pMsg = (SUFSMsg *)pData;
    if ((this->*(m_mapFunc[pMsg->_type]))(pMsg))
    {
        delete pMsg;
    }
    else
    {
        PutQ(pMsg);
    }
}

bool CUploadFileHandleThread::DoSave(SUFSMsg *pMsg)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s, File[%s]\n", __FUNCTION__, pMsg->_strAbsPath.c_str());
    bool bRet = false;
    boost::filesystem::path dir(pMsg->_strAbsPath);
    dir = dir.parent_path();
    if (!boost::filesystem::exists(dir))
    {
        FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s, directory '%s' doesn't exist, create it.\n", __FUNCTION__, dir.string().c_str());
        boost::filesystem::create_directory(dir);
    }

    std::fstream file;
    file.open(pMsg->_strAbsPath, std::ios_base::out | std::ios_base::binary);
    if (file.is_open())
    {
        if (file.write(pMsg->_strContent.c_str(), pMsg->_strContent.length()))
        {
            bRet = true;
            file.flush();
        }
        else
        {
            FM_XLOG(XLOG_WARNING, "CUploadFileHandleThread::%s, can't write data in file [%s].\n", __FUNCTION__, pMsg->_strAbsPath.c_str());
        }

        file.close();
    }
    else
    {
        FM_XLOG(XLOG_WARNING, "CUploadFileHandleThread::%s, can't open file [%s].\n", __FUNCTION__, pMsg->_strAbsPath.c_str());
    }

    return bRet;
}

