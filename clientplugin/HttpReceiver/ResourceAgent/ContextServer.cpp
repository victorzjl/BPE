#include "ContextServer.h"
#include "FileSystemUpdateThread.h"
#include "ResourceAgentLogHelper.h"
#include "UploadForm.h"

CContextServer::CContextServer()
{
    InstallModules();
}

CContextServer::CContextServer(const SServerConfig &sServerConfig)
    : m_sConfig(sServerConfig)
{
    InstallModules();
}

CContextServer::~CContextServer()
{}

void CContextServer::SetServerConfig(const SServerConfig &sServerConfig)
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s\n", __FUNCTION__);
    m_sConfig = sServerConfig;
    UninstallModules();
    InstallModules();
}

bool CContextServer::IsUnderContext(const string &strDomain, const string &strPath)
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s, Domain[%s], Path[%s]\n", __FUNCTION__, strDomain.c_str(), strPath.c_str());
    return (-1 != GetMatchedContextPathLength(strDomain, strPath));
}


int CContextServer::GetMatchedContextPathLength(const string &strDomain, const string &strPath)
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s, Domain[%s], Path[%s]\n", __FUNCTION__, strDomain.c_str(), strPath.c_str());
    int iRet = -1;
    unsigned int nChoosenPathLen = 0;
    for (vector<SServerContext>::iterator iter = m_sConfig._vecContext.begin();
        iter != m_sConfig._vecContext.end(); ++iter)
    {
        if (strDomain == iter->_strDomain)
        {
            if (iter->_strPath.empty()
                && nChoosenPathLen == 0)
            {
                iRet = 0;
            }
            else if (iter->_strDomain == strDomain 
                     && boost::icontains(strPath, iter->_strPath)
                     && iter->_strPath.length() > nChoosenPathLen)
            {
                iRet = iter->_strPath.length();
            }
        }
    }

    return iRet;
}

bool CContextServer::OnRequest(SRequestInfo &sReq, SResponseExtension &sResExt)
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s, Path[%s]\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
    bool bRet = false;
    if (m_pUploadManager->IsUploadURL(sReq.strUrlIdentify))
    {
        if (boost::istarts_with(sReq.strContentType, "multipart/form-data"))
        {
            RA_XLOG(XLOG_DEBUG, "CContextServer::%s, Handle By upload module.\n", __FUNCTION__);
            char szBoundary[128] = { 0 };
            sscanf(sReq.strContentType.c_str(), "%*[^=]=%s", szBoundary);
            CUploadForm oUploadForm;
            if (oUploadForm.ParseFromData(sReq.strUriAttribute, szBoundary))
            {
                bRet = true;
                const vector<UploadFilePtr> &vecUploadFile = oUploadForm.GetUploadFiles();
                vector<UploadFilePtr>::const_iterator iter = vecUploadFile.begin();
                while ((iter != vecUploadFile.end()) && bRet)
                {
                    bRet = m_pUploadManager->OnSave((*iter)->_strName, (*iter)->_strContent);
                    ++iter;
                }
            }

            sResExt._bIsImage = false;
            sResExt._strFileExtension = "html";
            sReq.strResponse = (bRet ? "Successful" : "Failed");  
        }
    }
    else
    {
        RA_XLOG(XLOG_DEBUG, "CContextServer::%s, Handle By file manager module\n", __FUNCTION__);
        bRet = m_pFileManager->GetContent(sReq.strUrlIdentify, sReq.strResponse, sResExt);
    }

    return bRet;
}

bool CContextServer::OnResponse(SRequestInfo &sReq, vector<string> &vecResponse)
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s, Path[%s]\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
    return m_pFileManager->ConstructContent(sReq.strUrlIdentify, vecResponse, sReq.strResponse);
}

void CContextServer::InstallModules()
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s\n", __FUNCTION__);
    InstallFileManagerModule();
    InstallUploadModule();
}

void CContextServer::InstallFileManagerModule()
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s\n", __FUNCTION__);
    if (m_sConfig._sFileManagerConfig._strRootPath.empty())
    {
        m_pFileManager.reset(new CFileManager(-1));
    }
    else
    {
        m_pFileManager.reset(new CFileManager(m_sConfig._dwServerNo));
        m_pFileManager->SetDefaultFile(m_sConfig._sFileManagerConfig._strDefaultFile);
        m_pFileManager->SetRootPath(m_sConfig._sFileManagerConfig._strRootPath);
        m_pFileManager->SetDefaultExtension(m_sConfig._sFileManagerConfig._strDefaultExtension);
        m_pFileManager->Load();
        CFSUpdateThread::GetInstance()->RegisterHander(m_sConfig._sFileManagerConfig._strRootPath, m_pFileManager);
    }
}

void CContextServer::InstallUploadModule()
{
    RA_XLOG(XLOG_DEBUG, "CContextServer::%s\n", __FUNCTION__);
    if (m_sConfig._sUploadConfig._strUploadPath.empty())
    {
        m_pUploadManager.reset(new CUploadManager);
    }
    else
    {
        m_pUploadManager.reset(new CUploadManager(m_sConfig._sUploadConfig._strUploadPath, m_sConfig._sUploadConfig._strUploadURL));
    }
}

void CContextServer::UninstallModules()
{
    CFSUpdateThread::GetInstance()->UnregisterHander(m_sConfig._sFileManagerConfig._strRootPath);
    m_pFileManager.reset();
    m_pUploadManager.reset();
}
