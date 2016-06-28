#ifndef _UPLOAD_MANAGER_H_
#define _UPLOAD_MANAGER_H_

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <vector>

using std::string;
using std::vector;

typedef struct stFormInputInfo
{
    string _strName;
    string _strFilePath;
    string _strContent;
    string _strDisposition;
    string _strContentType;
    string _strTransferEncoding;
}SFormInputInfo;

class CUploadManager : public boost::enable_shared_from_this<CUploadManager>,
    private boost::noncopyable
{
public:
    CUploadManager();
    CUploadManager(const string &strAbsPath, const string &strUploadURL);
    virtual ~CUploadManager();

    bool IsUploadURL(const string &strURL) { return !strURL.empty()&&(strURL == m_strUploadURL); }
    bool OnSave(const string &strFileName, const string &strContent);

private:
    string m_strAbsPath;
    string m_strUploadURL;
};

typedef boost::shared_ptr<CUploadManager> UploadManagerPtr;

#endif //_UPLOAD_MANAGER_H_