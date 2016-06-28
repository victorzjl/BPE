#ifndef _CONTEXT_SREVER_H_
#define _CONTEXT_SREVER_H_

#include <string>

#include "HttpRecConfig.h"
#include "FileManager.h"
#include "UploadManager.h"
#include "HpsCommonInner.h"

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

using std::string;

class CContextServer : public boost::enable_shared_from_this<CContextServer>,
    private boost::noncopyable
{
public:
    CContextServer();
    CContextServer(const SServerConfig &sServerConfig);
    virtual ~CContextServer();

    void SetServerConfig(const SServerConfig &sServerConfig);
    const SServerConfig &GetServerConfig() { return m_sConfig; }

    bool IsUnderContext(const string &strDomain, const string &strPath);
    //return -1 when no mateched context, or return the length of mateched context path
    int GetMatchedContextPathLength(const string &strDomain, const string &strPath);

    bool OnRequest(SRequestInfo &sReq, SResponseExtension &sResExt);
    bool OnResponse(SRequestInfo &sReq, vector<string> &vecResponse);

private:
    void InstallModules();
    void InstallFileManagerModule();
    void InstallUploadModule();
    void UninstallModules();
    
private:
    SServerConfig m_sConfig;
    FileManagerPtr m_pFileManager;
    UploadManagerPtr m_pUploadManager;
};

typedef boost::shared_ptr<CContextServer> ContextServerPtr;

#endif //_CONTEXT_SREVER_H_
