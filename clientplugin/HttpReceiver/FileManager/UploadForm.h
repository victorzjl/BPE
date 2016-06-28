#ifndef _UPLOAD_FORM_H_
#define _UPLOAD_FORM_H_

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

using std::string;
using std::vector;


typedef struct stUploadFile : public boost::enable_shared_from_this<stUploadFile>,
                              private boost::noncopyable
{
    string _strName;
    string _strType;
    string _strContent;
    string _strTransferEncoding;
    string _strDisposition;
}SUploadFile;

typedef boost::shared_ptr<SUploadFile> UploadFilePtr;

typedef struct stFormInput
{
    bool bFileInputType;
    string _strName;
    string _strDisposition;
    
    string _strValue; //for non-file input  
    vector<UploadFilePtr> _vecUploadFile; //for file input  

    stFormInput() : bFileInputType(false) {}
}SFormInput;

class CUploadForm
{
public:
    CUploadForm() {}
    ~CUploadForm();

    bool ParseFromData(const string &strFormData, const string &strBoundary);
    const vector<UploadFilePtr> &GetUploadFiles() { return m_vecUploadFile; }

private:
    bool ParseOneInput(const char *pBegin, const char *const pEnd, SFormInput &sFormInput);
    bool ParseInputHeader(const char *pBegin, const char *const pEnd, SFormInput &sFormInput);
    bool ParseFileInput(const char *pBegin, const char *const pEnd, SFormInput &sFormInput);
    bool GetFileInputBoundary(const char *pBegin, const char *const pEnd, string &strBoundary);
    bool ParseSingleUploadFile(const char *pBegin, const char *const pEnd, SFormInput &sFormInput);
    bool ParseMultiUploadFile(const char *pBegin, const char *const pEnd, const string &strBoundary, SFormInput &sFormInput);
    bool ParseOneUploadFile(const char *pBegin, const char *const pEnd, UploadFilePtr pUploadFile);

private:
    vector<SFormInput> m_vecFormInput;
    vector<UploadFilePtr> m_vecUploadFile;
};

#endif //_UPLOAD_FORM_H_