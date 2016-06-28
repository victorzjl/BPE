#ifndef _FILE_CACHE_H_
#define _FILE_CACHE_H_

#include <boost/thread/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
#include <set>
#include <list>

#include "FileManagerMacro.h"
#include "DynamicRequestInfo.h"

using std::pair;
using std::multimap;
using std::set;
using std::list;

typedef enum enLoadStatus
{
    LOADED = 0,
    UNLOAD = 1,
    NOT_EXIST = 2,
    IS_DIRECTORY = 3
}ELoadStatus;


class CFileCache;
typedef boost::shared_ptr<CFileCache> FilePtr;
class CFileCache : public boost::enable_shared_from_this<CFileCache>,
    private boost::noncopyable
{
public:
    CFileCache(const string &strAbsRootPath, int dwFlag);
    virtual ~CFileCache();
    void Dump();

    const string &GetRelPath() { return m_strRelPath; }
    const string &GetAbsPath() { return m_strAbsPath; }
    ELoadStatus GetStatus() { return m_eLoadStatus; }
    const string &GetExtension() { return m_strExtension; }

    bool LoadContent(const string &strRelPath);
    bool ReloadContent();
    void Clear();
    bool isTextContent() { return m_bIsTextContent; }
    const set<string> &GetIncludePaths() { return m_setIncludePath; }
    const string &GetModifiedTime() { return m_strModifiedTime; }

    void GetContent(string &strContent, vector<SDynamicRequestInfo> &vecDynReqInfo);
    void ConstructContent(vector<string> &vecResponse, string &strContent);
    
    bool PreprocessContent(const string &strTime);
    bool ParseContent();
    bool IsIncludeDone() { return (0 == m_dwIncludeNumber); }
    bool ReplaceOneIncludeFile(const FilePtr &filePtr);
    bool FinishIncludeReplace();
    bool FinishDeconstruct();

private:
    bool LoadInner();
    bool IsImageFile(const string &strExtension);

private:
    typedef struct stSlice
    {
        string _strContent;

        //include slice
        bool _bIsInclude;
        string _strIncludePath;

        //request slice
        bool _bIsDynamic;
        SURLInfo _sUrlInfo;

        stSlice() : _bIsInclude(false), _bIsDynamic(false){}
    }SSlice;

private:
    bool UpdateModifiedTime();
    bool ParseFileExtension();

    bool ParseStaticContent(const string &strContent);
    bool ParseDynamicContent(const string &strContent);

    //'request' Command
    bool ParseRequestCommand(SSlice &sSlice);
    bool ParseURL(SSlice &sSlice);
    bool GetURLElements(const string &strURL, string &strProtocol, string &strHost,
                        unsigned int &dwPort, string &strPath, string &strParams);
    bool TransformPortStr2UInt(const string &strPort, unsigned int &dwPort, bool bIsHttps);
    bool ParseURLParams(const string &strParams, SSlice &sSlice);
    bool SetFormatParams(SSlice &sSlice);

    //'include' Command
    bool ParseIncludeCommand(SSlice &sSlice);
    bool ParseIncludePath(SSlice &sSlice);

    //'ssi virtual'
    bool ParseSsiVirtualCommand(SSlice &sSlice);

    //
    bool IntegrateStaticSlices();
    bool RecordDynamicSlices();
    bool SetFormatContent();

private:
    int m_dwFlag;
    string m_strAbsRootPath;
    string m_strRelPath;
    string m_strAbsPath;
    string m_strExtension;
    string m_strContent; //notice: this is not the origial content for dynamic
    ELoadStatus m_eLoadStatus;
    bool m_bIsTextContent;
    string m_strModifiedTime;

    list<SSlice> m_listSlice;

    //use for dynamic request
    vector<SDynamicRequestInfo> m_vecDynReqInfo;

    //use for include resolving
    unsigned int m_dwIncludeNumber;
    set<string> m_setIncludePath;
    map<string, FilePtr> m_mapReplaceFile;
};

#endif //_FILE_CACHE_H_
