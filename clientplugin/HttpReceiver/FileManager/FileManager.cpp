#include "FileManager.h"
#include "FileManagerLogHelper.h"
#include "HttpRecConfig.h"

#include <boost/filesystem.hpp>
#ifdef WIN32
    #include<windows.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

CFileManager::~CFileManager()
{
}

void CFileManager::Dump()
{
    FM_XLOG(XLOG_DEBUG, "-------------------------------------------------------\n");
    map<string, FilePtr>::iterator iter = m_mapFile.begin();
    while (iter != m_mapFile.end())
    {
        iter->second->Dump();
        ++iter;
        FM_XLOG(XLOG_DEBUG, "-------------------------------------------------------\n");
    }
    FM_XLOG(XLOG_DEBUG, "-------------------------------------------------------DONE\n");
}

void CFileManager::SetRootPath(const string &strRoot)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Root Path[%s]\n", __FUNCTION__, strRoot.c_str());
    if (!boost::filesystem::is_directory(strRoot))
    {
        FM_XLOG(XLOG_DEBUG, "CFileManager::%s Root path[%s] is not exist.\n", __FUNCTION__, strRoot.c_str());
    }
    else
    {
        m_strAbsRootPath = boost::filesystem::system_complete(strRoot).string();
        if (!boost::iends_with(m_strAbsRootPath, PATH_SEPARATOR))
        {
            m_strAbsRootPath += PATH_SEPARATOR;
        }
    }
}

void CFileManager::SetDefaultExtension(const string &strExtension)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Extension[%s]\n", __FUNCTION__, strExtension.c_str());
    m_strDefaultExtension =  ".";
    m_strDefaultExtension += strExtension; 
}

void CFileManager::Load()
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Absolute Root Path[%s]\n", __FUNCTION__, m_strAbsRootPath.c_str());
    m_cacheRWLock.wr_lock();

    unsigned int nAbsRootSize = m_strAbsRootPath.size();

    boost::filesystem::recursive_directory_iterator iter(m_strAbsRootPath);
    boost::filesystem::recursive_directory_iterator iterEnd;
    for (; iter != iterEnd; ++iter)
    {
        string strAbsFilePath = iter->string();

        //need to skip version-control directory
        if (!boost::icontains(strAbsFilePath, SVN_DIR)
            && !boost::icontains(strAbsFilePath, GIT_DIR))
        {
            if (boost::filesystem::is_regular_file(*iter))
            {  
                FilePtr pFile = FilePtr(new CFileCache(m_strAbsRootPath, m_dwFlag));
                string strRelativeFilePath = strAbsFilePath.substr(nAbsRootSize);
                if (pFile->LoadContent(strRelativeFilePath))
                {
                    UpdateLastModifiedTime(pFile->GetModifiedTime());
                    transform(strRelativeFilePath.begin(), strRelativeFilePath.end(), strRelativeFilePath.begin(), (int(*)(int))tolower);
                    m_mapFile[strRelativeFilePath] = pFile;
                }
            }
        }
    }

    Deconstruct();

    m_cacheRWLock.wr_unlock();

    Dump();
}

void CFileManager::Reload()
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s\n", __FUNCTION__);
    Clear();
    Load();
}

void CFileManager::Clear()
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s\n", __FUNCTION__);
    m_cacheRWLock.wr_lock();
    m_mapFile.clear();
    m_mapIncludeRelations.clear();
    m_cacheRWLock.wr_unlock();
}

bool CFileManager::GetContent(const string &strPath, string &strContent, SResponseExtension& sResExt)
{
    static const SDynamicRequestInfo EMPTY_DYN_REQ_INFO;

    bool bRet = false;
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s RequestPath[%s]\n", __FUNCTION__, strPath.c_str());
    string strMappingPath;
    TransformMappingPath(strPath, strMappingPath);

    m_cacheRWLock.rd_lock();

    map<string, FilePtr>::iterator iter = m_mapFile.find(strMappingPath.empty() ? m_strDefaultFile : strMappingPath);
    if (iter != m_mapFile.end())
    {
        if (LOADED == iter->second->GetStatus())
        {
            bRet = true;
            iter->second->GetContent(strContent, sResExt._vecDynReqInfo);
            sResExt._strFileExtension = iter->second->GetExtension();
            sResExt._bIsImage = !iter->second->isTextContent();
            sResExt._strAbsFilePath = iter->second->GetAbsPath();
        }
    }
    else
    {
        FM_XLOG(XLOG_DEBUG, "CFileManager::%s File is not exists. Path[%s]\n", __FUNCTION__, strMappingPath.c_str());
    }

    m_cacheRWLock.rd_unlock();

    return bRet;
}

bool CFileManager::ConstructContent(const string &strPath, vector<string> &vecResponse, string &strContent)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Path[%s]\n", __FUNCTION__, strPath.c_str());

    bool bRet = false;
    string strMappingPath;
    TransformMappingPath(strPath, strMappingPath);

    m_cacheRWLock.rd_lock();

    map<string, FilePtr>::iterator iter = m_mapFile.find(strMappingPath.empty() ? m_strDefaultFile : strMappingPath);
    if (iter != m_mapFile.end())
    {
        if (LOADED == iter->second->GetStatus())
        {
            iter->second->ConstructContent(vecResponse, strContent);
        }
    }
    else
    {
        FM_XLOG(XLOG_DEBUG, "CFileManager::%s File is not exists. Path[%s]\n", __FUNCTION__, strMappingPath.c_str());
    }

    m_cacheRWLock.rd_unlock();

    return bRet;
}

void CFileManager::OnReload()
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s\n", __FUNCTION__);
    Reload();
}

void CFileManager::OnDirCreated(const string &strAbsPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Dic[%s]\n", __FUNCTION__, strAbsPath.c_str());
    m_cacheRWLock.wr_lock();
    //TODO
    m_cacheRWLock.wr_unlock();
}

void CFileManager::OnDirDeleted(const string &strAbsPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Dic[%s]\n", __FUNCTION__, strAbsPath.c_str());
    m_cacheRWLock.wr_lock();
    //TODO
    m_cacheRWLock.wr_unlock();
}

void CFileManager::OnDirModified(const string &strAbsPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s Dic[%s]\n", __FUNCTION__, strAbsPath.c_str());
    m_cacheRWLock.wr_lock();
    //TODO
    m_cacheRWLock.wr_unlock();
}

void CFileManager::OnFileCreated(const string &strAbsPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s File[%s]\n", __FUNCTION__, strAbsPath.c_str());
    m_cacheRWLock.wr_lock();
    //TODO
    m_cacheRWLock.wr_unlock();
}

void CFileManager::OnFileDeleted(const string &strAbsPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s File[%s]\n", __FUNCTION__, strAbsPath.c_str());
    m_cacheRWLock.wr_lock();
    //TODO
    m_cacheRWLock.wr_unlock();
}

void CFileManager::OnFileModified(const string &strAbsPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s File[%s]\n", __FUNCTION__, strAbsPath.c_str());
    m_cacheRWLock.wr_lock();
    //TODO
    m_cacheRWLock.wr_unlock();
}

void CFileManager::UpdateLastModifiedTime(const string &strModifiedTime)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s, LastModifiedTime[%s], ModifiedTime[%s]\n", __FUNCTION__, m_strLastModifiedTime.c_str(), strModifiedTime.c_str());

    if (strcmp(strModifiedTime.c_str(), m_strLastModifiedTime.c_str()) > 0)
    {
        m_strLastModifiedTime = strModifiedTime;
    }
}

void CFileManager::TransformMappingPath(const string &strPath, string &strMappingPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s \n", __FUNCTION__);
    if (strPath.empty() || boost::iends_with(strPath, "/"))
    {
        strMappingPath = strPath + m_strDefaultFile;
    }
    else
    {
        strMappingPath = strPath;
        char szExtension[8] = { 0 };
        unsigned int nMatch = sscanf(strPath.c_str(), "%*[^.].%s", szExtension);
        if ((0 == nMatch) && !m_strDefaultExtension.empty())
        {
            strMappingPath += m_strDefaultExtension;
        }
    }
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s ori[%s], transfrom[%s]\n", __FUNCTION__, strPath.c_str(), strMappingPath.c_str());
}

bool CFileManager::Deconstruct()
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s\n", __FUNCTION__);
    queue<FilePtr> queIncludeDoneCache;
    map<string, FilePtr>::iterator iter = m_mapFile.begin();
    while (iter != m_mapFile.end())
    {
        iter->second->PreprocessContent(m_strLastModifiedTime);
        iter->second->ParseContent();

        //record file-include relationships
        if (iter->second->IsIncludeDone())
        {
            queIncludeDoneCache.push(iter->second);
        }
        else
        {
            const set<string> &setIncludePath = iter->second->GetIncludePaths();
            set<string>::const_iterator iterPath = setIncludePath.begin(); 
            while (iterPath != setIncludePath.end())
            {
                m_mapIncludeRelations[*iterPath].push_back(iter->second);
                ++iterPath;
            }
        }
        ++iter;
    }
    
    ResolveFileInclude(queIncludeDoneCache);

    iter = m_mapFile.begin();
    while (iter != m_mapFile.end())
    {
        iter->second->FinishDeconstruct();
        ++iter;
    }
    return true;
}

bool CFileManager::ResolveFileInclude(queue<FilePtr> &queIncludeDoneFile)
{
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s\n", __FUNCTION__);
    map< string, list<FilePtr> > mapUnresolvedInclude = m_mapIncludeRelations;
    while (!mapUnresolvedInclude.empty())
    {
        if (queIncludeDoneFile.empty())
        {
            FM_XLOG(XLOG_WARNING, "CFileManager::%s, There are some errors when processing include files."
                "[May be unresolved nest or included file not exist.]\n", __FUNCTION__);
            DumpUnresolvedIncludeRelations(mapUnresolvedInclude);
            break;
        }

        FilePtr pIncludeFile = queIncludeDoneFile.front();
        queIncludeDoneFile.pop();
        map< string, list<FilePtr> >::iterator iterMap = mapUnresolvedInclude.find(pIncludeFile->GetRelPath());
        if (iterMap != mapUnresolvedInclude.end())
        {
            list<FilePtr> &vecFile = iterMap->second;
            list<FilePtr>::iterator iterVec = vecFile.begin();
            while (iterVec != vecFile.end())
            {
                FilePtr pFile = *iterVec;
                pFile->ReplaceOneIncludeFile(pIncludeFile);
                if (pFile->IsIncludeDone())
                {
                    queIncludeDoneFile.push(pFile);
                }
                ++iterVec;
            }
            mapUnresolvedInclude.erase(iterMap);
        }
    }

    map<string, FilePtr>::iterator iter = m_mapFile.begin();
    while (iter != m_mapFile.end())
    {
        iter->second->FinishIncludeReplace();
        ++iter;
    }
    return true;
}

void CFileManager::DumpUnresolvedIncludeRelations(const map< string, list<FilePtr> > &mapIncludeRelations)
{
    string strInfo;
    map< string, list<FilePtr> >::const_iterator iterMap = mapIncludeRelations.begin();
    while (iterMap != mapIncludeRelations.end())
    {
        strInfo += "{";
        strInfo += iterMap->first + ":";
        string strOneRelation;
        list<FilePtr>::const_iterator iterVec = iterMap->second.begin();
        while (iterVec != iterMap->second.end())
        {
            strOneRelation += (*iterVec)->GetRelPath() + ",";
            ++iterVec;
        }

        //remove the last character ','
        REMOVE_LAST_CHARACTER(strOneRelation);
        strInfo += strOneRelation + "}-";
        ++iterMap;
    }

    //remove the last character '-'
    REMOVE_LAST_CHARACTER(strInfo);
    FM_XLOG(XLOG_DEBUG, "CFileManager::%s, Unresolve files [%s]\n", __FUNCTION__, strInfo.c_str());
}


