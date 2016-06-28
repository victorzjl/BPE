#include "UploadManager.h"
#include "UploadFileHandleThread.h"
#include "FileManagerLogHelper.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>


CUploadManager::CUploadManager()
    : m_strAbsPath(""),
      m_strUploadURL("")
{
}

CUploadManager::CUploadManager(const string &strAbsPath, const string &strUploadURL)
    : m_strAbsPath(strAbsPath),
      m_strUploadURL(strUploadURL)
{
    if (!boost::filesystem::exists(m_strAbsPath))
    {
        boost::filesystem::create_directory(m_strAbsPath);
    }
}

CUploadManager::~CUploadManager()
{
}

bool CUploadManager::OnSave(const string &strFileName, const string &strContent)
{
    FM_XLOG(XLOG_DEBUG, "CUploadManager::%s, Path[%s], File[%s]\n", __FUNCTION__, m_strAbsPath.c_str(), strFileName.c_str());
    return (0 == CUploadFileHandleThread::GetInstance()->OnSave(m_strAbsPath + strFileName, strContent));
}
