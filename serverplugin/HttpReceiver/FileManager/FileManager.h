#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#include "IFileSystemSUpdateHandler.h"
#include "RWLock.h"
#include "FileCache.h"
#include "FileManagerMacro.h"
#include "DynamicRequestInfo.h"

#include <queue>
#include <list>

using std::queue;
using std::list;

typedef struct stResponseExtension
{
    string _strAbsFilePath;
    string _strFileExtension; 
    bool _bIsImage;
    vector<SDynamicRequestInfo> _vecDynReqInfo;
}SResponseExtension;


class CFileManager : public IFSUpdateHandler,
    public boost::enable_shared_from_this<CFileManager>,
                     private boost::noncopyable
{
public:
    CFileManager(int dwFlag) : m_dwFlag(dwFlag) {}
    virtual ~CFileManager();
    void Dump();

    void SetRootPath(const string &root);
    void SetDefaultFile(const string &strDefaultFile) { m_strDefaultFile = strDefaultFile; }
    void SetContextPath(const string &strContextPath) { m_strContextPath = strContextPath; }
    void SetDefaultExtension(const string &strExtension);
    
    void Load();
    void Reload();
    void Clear();

    const string& GetAbsRootPath() { return m_strAbsRootPath; }
    const string& GetDefaultFile() { return m_strDefaultFile; }

    bool GetContent(const string &strPath, string &strContent, SResponseExtension& sResExt);
    bool ConstructContent(const string &strPath, vector<string> &vecResponse, string &strContent);

    //handle file system change 
    virtual void OnReload();
    virtual void OnDirCreated(const string &strAbsPath);
    virtual void OnDirDeleted(const string &strAbsPath);
    virtual void OnDirModified(const string &strAbsPath);
    virtual void OnFileCreated(const string &strAbsPath);
    virtual void OnFileDeleted(const string &strAbsPath);
    virtual void OnFileModified(const string &strAbsPath);


private:  
    void UpdateLastModifiedTime(const string &strModifiedTime);
    void TransformMappingPath(const string &strPath, string &strMappingPath);
    bool Deconstruct();
    bool ResolveFileInclude(queue<FilePtr> &queIncludeDoneFile);
    void DumpUnresolvedIncludeRelations(const map< string, list<FilePtr> > &mapIncludeRelations);

private:
    int m_dwFlag;
    string m_strAbsRootPath;
    string m_strDefaultFile;
    string m_strContextPath;
    string m_strDefaultExtension;

    RWLock m_cacheRWLock;
    string m_strLastModifiedTime;
    map<string, FilePtr> m_mapFile;

    //file-include relationships
    map< string, list<FilePtr> > m_mapIncludeRelations;
};

typedef boost::shared_ptr<CFileManager> FileManagerPtr;

#endif //_FILE_MANAGER_H_

