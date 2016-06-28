#ifndef _I_FS_UPDATE_HANDLER_H_
#define _I_FS_UPDATE_HANDLER_H_

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

using std::string;

////NOTICE: 'FS' is short for 'file system'
class IFSUpdateHandler;
typedef boost::shared_ptr<IFSUpdateHandler> IFSUpdateHandlerPtr;

class IFSUpdateHandler : public boost::enable_shared_from_this<IFSUpdateHandler>
{
public:
    virtual ~IFSUpdateHandler() {}
    virtual void OnReload() = 0;
    virtual void OnDirCreated(const string &strAbsPath) = 0;
    virtual void OnDirDeleted(const string &strAbsPath) = 0;
    virtual void OnDirModified(const string &strAbsPath) = 0;
    virtual void OnFileCreated(const string &strAbsPath) = 0;
    virtual void OnFileDeleted(const string &strAbsPath) = 0;
    virtual void OnFileModified(const string &strAbsPath) = 0;
};

#endif //_I_FS_UPDATE_HANDLER_H_
