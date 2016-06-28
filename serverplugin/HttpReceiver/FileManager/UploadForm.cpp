#include "UploadForm.h"
#include "CommonMacro.h"
#include "FileManagerLogHelper.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#define CONTENT_DISPOSITION_MARK "content-disposition"
#define CONTENT_TYPE_MARK "content-type"
#define CONTENT_TRANSFER_ENCODING_MARK "content-transfer-encoding"

#define CONTENT_DISPOSITION_MARK_LEN 19
#define CONTENT_TYPE_MARK_LEN 12
#define CONTENT_TRANSFER_ENCODING_MARK_LEN 25

const static boost::algorithm::detail::first_finderF<const char*, boost::is_equal> FindNewLineMark = boost::first_finder("\r\n");
const static boost::algorithm::detail::first_finderF<const char*, boost::is_equal> FindDataMark = boost::first_finder("\r\n\r\n");
const static boost::algorithm::detail::first_finderF<const char*, boost::is_iequal> FindContentDispositonMark = boost::first_finder(CONTENT_DISPOSITION_MARK, boost::algorithm::is_iequal(std::locale()));
const static boost::algorithm::detail::first_finderF<const char*, boost::is_iequal> FindContentTypeMark = boost::first_finder(CONTENT_TYPE_MARK, boost::algorithm::is_iequal(std::locale()));


CUploadForm::~CUploadForm()
{
    m_vecFormInput.clear();
    m_vecUploadFile.clear();
}

bool CUploadForm::ParseFromData(const string &strFormData, const string &strBoundary)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    const static SFormInput EMPTY_FORM_INPUT;

    bool bRet = true;
    if (!strFormData.empty())
    {
        string strSeparator("--");
        strSeparator += strBoundary;
        string strEndBoundary = strSeparator + "--\r\n";
        bRet = boost::starts_with(strFormData, strSeparator) && boost::ends_with(strFormData, strEndBoundary);
        if (bRet)
        {
            unsigned nSkipLen = strSeparator.length() + 2;//skip $separator and '\r\n'

            const char *pBegin = strFormData.c_str() + nSkipLen;
            const char *pEnd = strstr(pBegin, strSeparator.c_str());
            while (NULL != pEnd)
            {
                m_vecFormInput.push_back(EMPTY_FORM_INPUT);
                SFormInput &sFormInput = m_vecFormInput.back();
                if (!ParseOneInput(pBegin, pEnd, sFormInput))
                {
                    bRet = false;
                    break;
                }
                pBegin = pEnd + nSkipLen;
                pEnd = strstr(pBegin, strSeparator.c_str());
            }
        }
    }
    
    return bRet;
}

bool CUploadForm::ParseOneInput(const char *pBegin, const char *const pEnd, SFormInput &sFormInput)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    bool bRet = ParseInputHeader(pBegin, pEnd, sFormInput);
    if (bRet)
    {
        if (sFormInput.bFileInputType)
        {
            bRet = ParseFileInput(pBegin, pEnd, sFormInput);
        }
        else
        {
            boost::iterator_range<const char*> irFind = FindDataMark(pBegin, pEnd);
            sFormInput._strValue.assign(irFind.begin() + irFind.size(), pEnd - 2);
        }
    }
    return bRet;
}

bool CUploadForm::ParseInputHeader(const char *pBegin, const char *const pEnd, SFormInput &sFormInput)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    sFormInput.bFileInputType = false;
    boost::iterator_range<const char*> irFind = FindNewLineMark(pBegin, pEnd);
    while ((irFind.begin() != irFind.end()) 
        && (pBegin != irFind.begin()))
    {
        if (0 == strncasecmp(pBegin, CONTENT_DISPOSITION_MARK, CONTENT_DISPOSITION_MARK_LEN))
        {
            char szDisposition[32] = { 0 };
            char szName[32] = { 0 };
            if (2 != sscanf(pBegin, "%*[^:]: %[^;]; %*[^=]=\"%[^\"]\"", szDisposition, szName))
            {
                return false;
            }

            sFormInput._strDisposition = szDisposition;
            sFormInput._strName = szName;
        }
        else if (0 == strncasecmp(pBegin, CONTENT_TYPE_MARK, CONTENT_TYPE_MARK_LEN))
        {
            sFormInput.bFileInputType = true;
        }

        pBegin = irFind.begin() + irFind.size();
        irFind = FindNewLineMark(pBegin, pEnd);
    }

    return true;
}

bool CUploadForm::ParseFileInput(const char *pBegin, const char *const pEnd, SFormInput &sFormInput)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    bool bRet = false;
    string strBoundary;
    GetFileInputBoundary(pBegin, pEnd, strBoundary);
    if (strBoundary.empty())
    {
        bRet = ParseSingleUploadFile(pBegin, pEnd, sFormInput);
    }
    else
    {
        boost::iterator_range<const char*> irFind = FindDataMark(pBegin, pEnd);
        bRet = (irFind.begin() != irFind.end());
        if (bRet)
        {
            const char *pMultiDataBegin = irFind.begin() + irFind.size();
            bRet = ParseMultiUploadFile(pMultiDataBegin, pEnd, strBoundary, sFormInput);
        }
    }

    return bRet;
}

bool CUploadForm::GetFileInputBoundary(const char *pBegin, const char *const pEnd, string &strBoundary)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    boost::iterator_range<const char*> irFind = FindContentTypeMark(pBegin, pEnd);
    if (irFind.begin() != irFind.end())
    {
        string strContentType(irFind.begin(), FindNewLineMark(irFind.begin(), pEnd).begin());
        char szBoundary[128] = { 0 };
        sscanf(strContentType.c_str(), "%*[^=]=%s", szBoundary);
        strBoundary = szBoundary;
    }

    return true;
}

bool CUploadForm::ParseSingleUploadFile(const char *pBegin, const char *const pEnd, SFormInput &sFormInput)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    boost::iterator_range<const char*> irFind = FindContentDispositonMark(pBegin, pEnd);
    if (irFind.begin() != irFind.end())
    {
        UploadFilePtr pUploadFile(new SUploadFile);
        sFormInput._vecUploadFile.push_back(pUploadFile);
        m_vecUploadFile.push_back(pUploadFile);

        char szDisposition[32] = { 0 };
        char szFileName[128] = { 0 };
        sscanf(irFind.begin(), "%*[^:]: %[^;]; %*[^;]; %*[^=]=\"%[^\"]\"", szDisposition, szFileName);
        pUploadFile->_strDisposition = szDisposition;
        pUploadFile->_strName = szFileName;

        return ParseOneUploadFile(pBegin, pEnd, pUploadFile);
    }

    return false;
}

bool CUploadForm::ParseMultiUploadFile(const char *pBegin, const char *const pEnd, const string &strBoundary, SFormInput &sFormInput)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    bool bRet = true;
    string strSeparator("--");
    strSeparator += strBoundary;
    string strEndBoundary = strBoundary + "--\r\n";
    bRet = boost::starts_with(pBegin, strSeparator) && boost::ends_with(pBegin, strEndBoundary);
    if (bRet)
    {
        unsigned int nSkipLen = strSeparator.length() + 2; //skip $separator and '\r\n'
        boost::algorithm::detail::first_finderF<const char*, boost::is_equal> FindSeparator = boost::first_finder(strSeparator.c_str());
        pBegin += nSkipLen;
        boost::iterator_range<const char*> irFind = FindSeparator(pBegin, pEnd);
        while (irFind.begin() != irFind.end() && bRet)
        {
            UploadFilePtr pUploadFile(new SUploadFile);
            sFormInput._vecUploadFile.push_back(pUploadFile);
            m_vecUploadFile.push_back(pUploadFile);

            boost::iterator_range<const char*> irFindDispositon = FindContentDispositonMark(pBegin, irFind.begin());
            if (irFindDispositon.begin() != irFindDispositon.end())
            {
                char szDisposition[32] = { 0 };
                char szFileName[128] = { 0 };
                sscanf(pBegin, "%*[^:]: %[^;]; %*[^=]=\"%[^\"]\"", szDisposition, szFileName);
                pUploadFile->_strDisposition = szDisposition;
                pUploadFile->_strName = szFileName;
            }

            bRet = ParseOneUploadFile(pBegin, irFind.begin(), pUploadFile);
            pBegin = irFind.begin() + nSkipLen;
        }
    }

    return bRet;
}

bool CUploadForm::ParseOneUploadFile(const char *pBegin, const char *const pEnd, UploadFilePtr pUploadFile)
{
    FM_XLOG(XLOG_DEBUG, "CUploadFileHandleThread::%s\n", __FUNCTION__);
    char szBuffer[64] = { 0 };
    boost::iterator_range<const char*> irFind = FindNewLineMark(pBegin, pEnd);
    while ((irFind.begin() != irFind.end())
        && (pBegin != irFind.begin()))
    {
        if (0 == strncasecmp(pBegin, CONTENT_TYPE_MARK, CONTENT_TYPE_MARK_LEN))
        {
            sscanf(pBegin, "%*[^:]: %s", szBuffer);
            pUploadFile->_strType = szBuffer;
        }
        else if (0 == strncasecmp(pBegin, CONTENT_TRANSFER_ENCODING_MARK, CONTENT_TRANSFER_ENCODING_MARK_LEN))
        {
            sscanf(pBegin, "%*[^:]: %s", szBuffer);
            pUploadFile->_strTransferEncoding = szBuffer;
        }

        pBegin = irFind.begin() + irFind.size();
        irFind = FindNewLineMark(pBegin, pEnd);
    }

    if (pBegin == irFind.begin())
    {
        pBegin += 2; //skip "\r\n"
    }

    pUploadFile->_strContent.assign(pBegin, pEnd);
    return true;
}