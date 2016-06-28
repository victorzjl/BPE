#include "FileCache.h"
#include "FileManagerLogHelper.h"
#include "HttpRecConfig.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef WIN32
#include<windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define REGEX_MATCH_FLAGS boost::match_default | boost::format_all


//for special charaters in regex expression: ()[]{}|\^$*+?.,=:-!
#define _SEQ "\\\\\\"
const static string gs_strSpecialRegex = 
     "(\\()|(\\))|(\\[)|(\\])|(\\{)|(\\})|(\\|)|(\\\\)|(\\^)|(\\$)|(\\*)|(\\+)|(\\?)|(\\.)|(\\,)|(\\=)|(\\:)|(\\-)|(\\!)|(\\^)";
const static string gs_strSpecialFormat = 
     "(?1"_SEQ"()(?2"_SEQ"))(?3"_SEQ"[)(?4"_SEQ"])(?5"_SEQ"{)(?6"_SEQ"})(?7"_SEQ"|)(?8"_SEQ"\\)(?9"_SEQ"^)(?10"_SEQ"$)" \
     "(?11"_SEQ"*)(?12"_SEQ"+)(?13"_SEQ"?)(?14"_SEQ".)(?15"_SEQ",)(?16"_SEQ"=)(?17"_SEQ":)(?18"_SEQ"-)(?19"_SEQ"!)(?20"_SEQ"^)";

const static FilePtr EMPTY_FILE_PTR(new CFileCache("", -1));

CFileCache::CFileCache(const string &strAbsRootPath, int dwFlag)
: m_dwFlag(dwFlag),
  m_strAbsRootPath(strAbsRootPath),
  m_strModifiedTime("00000000000000"),
  m_dwIncludeNumber(0)
{
}

CFileCache::~CFileCache()
{
    Clear();
}

void CFileCache::Dump()
{
    if (!m_bIsTextContent)
    {
        FM_XLOG(XLOG_DEBUG, "\n|IMAGE : Path[%s], Status[%d], Extension[%s]\n", m_strAbsPath.c_str(), m_eLoadStatus, m_strExtension.c_str());
        return;
    }

    if (m_listSlice.empty())
    {
        FM_XLOG(XLOG_DEBUG, "\n|TEXT [no dynamic] : Path[%s], Status[%d], Extension[%s] Content[[[%s]]]\n", m_strAbsPath.c_str(), m_eLoadStatus, m_strExtension.c_str(), m_strContent.c_str());
        return;
    }

    std::stringstream ssSlicesInfo;
    unsigned int dwSerialNo = 0;
    list<SSlice>::const_iterator iter = m_listSlice.begin();
    while (iter != m_listSlice.end())
    {
        const SSlice &sSlice = *iter;
        if (!sSlice._bIsDynamic)
        {
            ssSlicesInfo << "| -------SLICE-" << dwSerialNo++ << ": "
                << "Type[static], "
                << "Content[[[" << sSlice._strContent << "]]]\n";
        }
        else
        {
            string strVarParams;
            map<string, string>::const_iterator iterMap = sSlice._sUrlInfo._mapVarParam.begin();
            while (iterMap != sSlice._sUrlInfo._mapVarParam.end())
            {
                strVarParams += iterMap->first + ":" + iterMap->second + ",";
                ++iterMap;
            }
            //remove the last character ','
            REMOVE_LAST_CHARACTER(strVarParams);

            ssSlicesInfo << "| -------SLICE-" << dwSerialNo++ << ": "
                << "Type[dynamic], "
                << "Content[[[" << sSlice._strContent << "]]], "
                << "Host[" << sSlice._sUrlInfo._strHost << "], "
                << "Port[" << sSlice._sUrlInfo._dwPort << "], "
                << "Path[" << sSlice._sUrlInfo._strPath << "], "
                << "FixedParams[" << sSlice._sUrlInfo._strFixedParams << "], "
                << "Parameters[" << strVarParams << "]\n";
        }
        
        ++iter;
    }

    FM_XLOG(XLOG_DEBUG, "\n|TEXT [contain dynamic]: Path[%s], Status[%d], Extension[%s]\n| Struct{{{\n%s| }}}\n", m_strAbsPath.c_str(), m_eLoadStatus, m_strExtension.c_str(), ssSlicesInfo.str().c_str());
}

bool CFileCache::LoadContent(const string &strPath)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s Path[%s]\n", __FUNCTION__, strPath.c_str());
    m_strRelPath = strPath;
    m_strAbsPath = m_strAbsRootPath + strPath;
    return LoadInner();
}

bool CFileCache::ReloadContent()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    return LoadInner();
}

void CFileCache::Clear()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    m_dwIncludeNumber = 0;
    m_eLoadStatus = UNLOAD;
    m_strContent.clear();
    m_listSlice.clear();
    m_vecDynReqInfo.clear();
    m_setIncludePath.clear();
    m_mapReplaceFile.clear();
}

bool CFileCache::LoadInner()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    Clear();

    if (!boost::filesystem::exists(m_strAbsPath))
    {
        FM_XLOG(XLOG_DEBUG, "CFileCache::%s Path is not exist. Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
        m_eLoadStatus = NOT_EXIST;
        return false;
    }

    if (boost::filesystem::is_directory(m_strAbsPath))
    {
        FM_XLOG(XLOG_DEBUG, "CFileCache::%s Path is directory. Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
        m_eLoadStatus = IS_DIRECTORY;
        return false;
    }

    std::fstream file(m_strAbsPath.c_str(), std::ios_base::in | std::ios_base::binary);
    if (file)
    {
        m_eLoadStatus = LOADED;
        //get file length
        file.seekg(0, file.end);
        unsigned int nFileLength = file.tellg();
        file.seekg(0, file.beg);

        //read content
        char *pSzBuffer = new char[nFileLength];
        file.read(pSzBuffer, nFileLength);
        if (!file)
        {
            FM_XLOG(XLOG_DEBUG, "CFileCache::%s Fail to read file. Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
            m_eLoadStatus = NOT_EXIST;
            delete [] pSzBuffer;
            return false;
        }

        m_strContent.assign(pSzBuffer, nFileLength);
        delete[] pSzBuffer;

        //get file extension
        m_strExtension = boost::filesystem::extension(m_strAbsPath);
        ParseFileExtension();

        //get last modified time
        UpdateModifiedTime();

        //check content type
        m_bIsTextContent = !IsImageFile(m_strExtension);
    }
    else
    {
        FM_XLOG(XLOG_DEBUG, "CFileCache::%s Fail to open file. Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
        m_eLoadStatus = NOT_EXIST;
        return false;
    }

    return true;
}

bool CFileCache::IsImageFile(const string &strExtension)
{
    return (boost::iequals(strExtension, "png"))
        || (boost::iequals(strExtension, "jpeg"))
        || (boost::iequals(strExtension, "bmp"))
        || (boost::iequals(strExtension, "gif"))
        || (boost::iequals(strExtension, "tiff"))
        || (boost::iequals(strExtension, "psd"))
        || (boost::iequals(strExtension, "svg"));
}

void CFileCache::GetContent(string &strContent, vector<SDynamicRequestInfo> &vecDynReqInfo)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
    if (m_vecDynReqInfo.empty())
    {
        strContent = m_strContent;
    }
    else
    {
        vecDynReqInfo = m_vecDynReqInfo;
    }
}

void CFileCache::ConstructContent(vector<string> &vecResponse, string &strContent)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());

    const unsigned int nVecResultSize = vecResponse.size();

    unsigned int nDynamicSize = 0;
    for (unsigned int iIter = 0; iIter < nVecResultSize; ++iIter)
    {
        nDynamicSize += vecResponse[iIter].size();
    }

    strContent.resize(nDynamicSize + m_strContent.size() - 2 * nVecResultSize + 1);
    SPRINTF_CONTENT(((char *)strContent.c_str()), m_strContent, vecResponse, nVecResultSize);
}

bool CFileCache::PreprocessContent(const string &strTime)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Path[%s], Time[%s]\n", __FUNCTION__, m_strAbsPath.c_str(), strTime.c_str());
    
    if (m_bIsTextContent)
    {
        boost::filesystem::path oRelDirPath(m_strRelPath);
        const vector<SEncoderConfig> &vecEncoderConfig = 
              CHttpRecConfig::GetInstance()->GetEncoderConfig(m_dwFlag, oRelDirPath.parent_path().string());
        string strRegex;
        string strFormat;
        unsigned int nPos = 0;
        for (vector<SEncoderConfig>::const_iterator iter = vecEncoderConfig.begin();
            iter != vecEncoderConfig.end(); ++iter)
        {
            try
            {
                string strSubRegex = iter->_strPattern;
                string strSubFormat = iter->_strReplace;
                if (iter->bExactMatch)
                {
                    strSubRegex = boost::regex_replace(strSubRegex, boost::regex(gs_strSpecialRegex), gs_strSpecialFormat, REGEX_MATCH_FLAGS);
                }
                strSubFormat = boost::regex_replace(strSubFormat, boost::regex(gs_strSpecialRegex), gs_strSpecialFormat, REGEX_MATCH_FLAGS);
                strSubFormat = boost::regex_replace(strSubFormat, boost::regex("(%T)"), strTime, REGEX_MATCH_FLAGS);
                strRegex += string(0 == nPos ? "" : "|") + "(" + strSubRegex + ")";
                strFormat += string("(?") + boost::lexical_cast<string>(nPos + 1) + "(" + strSubFormat + "))";
                ++nPos;
            }
            catch (std::exception &e)
            {
                FM_XLOG(XLOG_WARNING, "CFileCache::%s, Catch a exception -1. what[%s] \n", __FUNCTION__, e.what());
                continue;
            }
        }

        if (!strRegex.empty())
        {
            try
            {
                m_strContent = boost::regex_replace(m_strContent, boost::regex(strRegex), strFormat, REGEX_MATCH_FLAGS);
            }
            catch (std::exception &e)
            {
                FM_XLOG(XLOG_WARNING, "CFileCache::%s, Catch a exception -2. what[%s] \n", __FUNCTION__, e.what());
            }
        }
    }
    return true;
}

bool CFileCache::ParseContent()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
    const static boost::regex sSliceRegex("([\\s\\S]*?)("RS_SERVICE_REQUEST"|"RS_SERVICE_INCLUDE"|"RS_SSI_VIRTUAL")");
    if (m_bIsTextContent)
    {
        string::const_iterator iterStart = m_strContent.begin();
        string::const_iterator iterEnd = m_strContent.end();
        boost::smatch sMatch;
        while (boost::regex_search(iterStart, iterEnd, sMatch, sSliceRegex))
        {
            ParseStaticContent(sMatch[1].str());
            ParseDynamicContent(sMatch[2].str());
            iterStart += sMatch[0].length();
        }

        if (iterStart != iterEnd)
        {
            ParseStaticContent(string(iterStart, iterEnd));
        }
    }
    return true;
}

bool CFileCache::ReplaceOneIncludeFile(const FilePtr &filePtr)
{
    if (m_bIsTextContent)
    {
        const string &strIncludePath = filePtr->GetRelPath();
        FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Path[%s] IncludePath[%s]\n", __FUNCTION__, m_strAbsPath.c_str(), strIncludePath.c_str());
        map<string, FilePtr>::iterator iter = m_mapReplaceFile.find(strIncludePath);
        if (iter != m_mapReplaceFile.end())
        {
            iter->second = filePtr;
            --m_dwIncludeNumber;
        }
    }
    
    return true;
}

bool CFileCache::FinishIncludeReplace()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());

    if (m_bIsTextContent)
    {
        m_dwIncludeNumber = 0;
        for (list<SSlice>::iterator iter = m_listSlice.begin(); iter != m_listSlice.end(); ++iter)
        {
            if (iter->_bIsInclude)
            {
                iter->_bIsInclude = false;
                iter->_strContent = "";
                map<string, FilePtr>::iterator iterReplaceFile = m_mapReplaceFile.find(iter->_strIncludePath);
                if (iterReplaceFile != m_mapReplaceFile.end())
                {
                    FilePtr pFileCache = iterReplaceFile->second;
                    m_listSlice.insert(iter, pFileCache->m_listSlice.begin(), pFileCache->m_listSlice.end());
                }
            }
        }
    }
    return true;
}

bool CFileCache::FinishDeconstruct()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Path[%s]\n", __FUNCTION__, m_strAbsPath.c_str());
    if (m_bIsTextContent)
    {
        IntegrateStaticSlices();
        RecordDynamicSlices();
        SetFormatContent();
    }
    return true;
}

bool CFileCache::UpdateModifiedTime()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
#ifdef WIN32
    WIN32_FILE_ATTRIBUTE_DATA sFileAttribute;
    SYSTEMTIME sModifiedTimeU, sModifiedTimeL;
    if (GetFileAttributesEx(m_strAbsPath.c_str(), GetFileExInfoStandard, &sFileAttribute))
    {
        if (FileTimeToSystemTime(&sFileAttribute.ftLastWriteTime, &sModifiedTimeU))
        {
            ::SystemTimeToTzSpecificLocalTime(NULL, &sModifiedTimeU, &sModifiedTimeL);
            char szTime[16] = { 0 };
            sprintf(szTime, "%04u%02u%02u%02u%02u%02u", sModifiedTimeL.wYear, sModifiedTimeL.wMonth, sModifiedTimeL.wDay,
                sModifiedTimeL.wHour, sModifiedTimeL.wMinute, sModifiedTimeL.wSecond);
            m_strModifiedTime = szTime;
        }
    }
#else
    struct stat sFileStat;
    if (0 == stat(m_strAbsPath.c_str(), &sFileStat))
    {
        struct tm *pModifiedTime = localtime(&sFileStat.st_mtime);
        if (NULL != pModifiedTime)
        {
            char szTime[16] = { 0 };
            sprintf(szTime, "%04u%02u%02u%02u%02u%02u", pModifiedTime->tm_year + 1900, pModifiedTime->tm_mon + 1, pModifiedTime->tm_mday,
                pModifiedTime->tm_hour, pModifiedTime->tm_min, pModifiedTime->tm_sec);
            m_strModifiedTime = szTime;
        }
    }
#endif
    return true;
}

bool CFileCache::ParseFileExtension()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Extension[%s]\n", __FUNCTION__, m_strExtension.c_str());
    if (m_strExtension.empty())
    {
        m_strExtension = "html";
    }
    else
    {
        m_strExtension = m_strExtension.substr(1);
        if (boost::iequals(m_strExtension, "jpg"))
        {
            m_strExtension = "jpeg";
        }
    }
    return true;
}

bool CFileCache::ParseStaticContent(const string &strContent)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    if (!strContent.empty())
    {
        SSlice sSlice;
        sSlice._strContent = strContent;
        m_listSlice.push_back(sSlice);
    }
    return true;
}

bool CFileCache::ParseDynamicContent(const string &strContent)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Content[%s]\n", __FUNCTION__, strContent.c_str());
    string strr("("RS_SERVICE_PARSER"|"RS_SSI_PARSER")");
    static const boost::regex sCommandReg(strr);
    boost::smatch sMatch;
    if (!boost::regex_search(strContent, sMatch, sCommandReg))
    {
        FM_XLOG(XLOG_DEBUG, "CFileCache::%s, The format of command is wrong.\n", __FUNCTION__);
        return false;
    }

    SSlice sSlice;
    string strCommandGroup = sMatch[2].str();
    string strCommandType;
    if (!strCommandGroup.empty())
    {
        strCommandType = sMatch[3].str();
        sSlice._strContent = boost::trim_copy(sMatch[4].str());
    }
    else
    {
        strCommandGroup = sMatch[5].str();
        strCommandType = sMatch[6].str();
        sSlice._strContent = boost::trim_copy(sMatch[7].str());
    }
    

    if (boost::iequals(strCommandGroup, SERVICE_GROUP_MARK))
    {
        if (boost::iequals(strCommandType, SERVICE_REQUEST_MARK))
        {
            ParseRequestCommand(sSlice);
            m_listSlice.push_back(sSlice);
        }
        else if (boost::iequals(strCommandType, SERVICE_INCLUDE_MARK))
        {
            ParseIncludeCommand(sSlice);
            m_listSlice.push_back(sSlice);
        }
        else
        {
            FM_XLOG(XLOG_DEBUG, "CFileCache::%s, The command type for service is not supported. Path[%s]\n",
                __FUNCTION__, m_strAbsPath.c_str());
            return false;
        }
    }
    else if (boost::iequals(strCommandGroup, SSI_GROUP_MARK))
    {
        if (boost::iequals(strCommandType, SSI_VIRTUAL_MARK))
        {
            ParseSsiVirtualCommand(sSlice);
            m_listSlice.push_back(sSlice);
        }
        else
        {
            FM_XLOG(XLOG_DEBUG, "CFileCache::%s, The command type for include is not supported. Path[%s]\n", 
                __FUNCTION__, m_strAbsPath.c_str());
            return false;
        }
    }
    

    return true;
}

bool CFileCache::ParseRequestCommand(SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Path[%s] URL[%s]\n", __FUNCTION__, m_strAbsPath.c_str(), sSlice._strContent.c_str());
    sSlice._bIsDynamic = true;
    sSlice._bIsInclude = false;
    if (!ParseURL(sSlice))
    {
        sSlice._strContent = "";
        sSlice._bIsDynamic = false;
    }

    return true;
}

bool CFileCache::ParseURL(SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, URL[%s]\n", __FUNCTION__, sSlice._strContent.c_str());

    if (sSlice._strContent.empty())
    {
        FM_XLOG(XLOG_WARNING, "CFileCache::%s, URL is empty.\n", __FUNCTION__);
        return false;
    }

    unsigned int dwPort = 0;
    string strProtocol, strHost, strPath, strParams;
    if (!GetURLElements(sSlice._strContent, strProtocol, strHost, dwPort, strPath, strParams))
    {
        return false;
    }

    sSlice._sUrlInfo._bIsHttps = boost::algorithm::iequals(strProtocol, "https");
    sSlice._sUrlInfo._strHost = strHost;
    sSlice._sUrlInfo._dwPort = dwPort;
    sSlice._sUrlInfo._bIsRemote = !CHttpRecConfig::GetInstance()->IsLocal(strHost, dwPort);
    sSlice._sUrlInfo._strPath = strPath;
    
    //parse parameters
    return ParseURLParams(strParams, sSlice);
}

bool CFileCache::GetURLElements(const string &strURL, string &strProtocol, string &strHost,
                                unsigned int &dwPort, string &strPath, string &strParams)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, URL[%s]\n", __FUNCTION__, strURL.c_str());
    boost::regex sReg("^(.+(?=://))?(://)?([^:/]+)?(:?)([^/]+)?(/?)([^?]*)(\\??)(\\S*)$");
    boost::smatch sMatch;

    if (!boost::regex_search(strURL, sMatch, sReg))
    {
        FM_XLOG(XLOG_WARNING, "CFileCache::%s, Invaid URL format.\n", __FUNCTION__);
        return false;
    }

    //'C' is short for colon; 'B' is short for blackslash; 'Q' is short for question mark; 
    strProtocol       = boost::trim_copy(sMatch[1].str());    //part: protocol
    string strSep_CBB = boost::trim_copy(sMatch[2].str());    //part: separator(://)
    strHost           = boost::trim_copy(sMatch[3].str());    //part: host
    string strSep_C   = boost::trim_copy(sMatch[4].str());    //part: separator(:)
    string strPort    = boost::trim_copy(sMatch[5].str());    //part: port 
    string strSep_B   = boost::trim_copy(sMatch[6].str());    //part: separator(/)
    strPath           = boost::trim_copy(sMatch[7].str());    //part: path
    string strSep_Q   = boost::trim_copy(sMatch[8].str());    //part: separator(?)
    strParams         = boost::trim_copy(sMatch[9].str());    //part: parameters

    if ( (!strSep_CBB.empty() && (strProtocol.empty() || strHost.empty()))
        || (!strSep_C.empty() && (strHost.empty() || strPort.empty()))
        || (!strSep_Q.empty() && (strSep_B.empty() || strParams.empty()))
        )
    {
        FM_XLOG(XLOG_WARNING, "CFileCache::%s, Invaid URL Format.\n", __FUNCTION__);
        return false;
    }

    bool bIsHttps = boost::iequals(strProtocol, "https");
    if (!strProtocol.empty() && !boost::iequals(strProtocol, "http") && !bIsHttps)
    {
        FM_XLOG(XLOG_WARNING, "CFileCache::%s, The protocol is not supported.\n", __FUNCTION__);
        return false;
    }

    if (!TransformPortStr2UInt(strPort, dwPort, bIsHttps))
    {
        FM_XLOG(XLOG_WARNING, "CFileCache::%s, The port is unrecognized.\n", __FUNCTION__);
        return false;
    }

    return true;
}

bool CFileCache::TransformPortStr2UInt(const string &strPort, unsigned int &dwPort, bool bIsHttps)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, StrPort[%s], IsHttps[%s]\n", __FUNCTION__, strPort.c_str(), (bIsHttps ? "true" : "false"));
    if (!strPort.empty())
    {
        try
        {
            dwPort = boost::lexical_cast<unsigned int>(strPort);
        }
        catch (boost::bad_lexical_cast &e)
        {
            FM_XLOG(XLOG_WARNING, "CFileCache::%s, Catch lexical casting exception. What[%s]\n", __FUNCTION__, e.what());
            return false;
        }
    }
    else
    {
        dwPort = bIsHttps ? 443 : 80;
    }

    return true;
}

bool CFileCache::ParseURLParams(const string &strParams, SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Params[%s]\n", __FUNCTION__, strParams.c_str());
    if (strParams.empty())
    {
        return true;
    }

    vector<string> vecParam;
    boost::algorithm::split(vecParam, strParams, boost::algorithm::is_any_of("&"), boost::algorithm::token_compress_on);
    unsigned int nVecSize = vecParam.size();
    for (unsigned int iIter = 0; iIter < nVecSize; ++iIter)
    {
        boost::regex sReg("^([^=]+)(=?)(\\$?)(\\S*)$");
        boost::smatch sMatch;

        if (!boost::regex_search(vecParam[iIter], sMatch, sReg))
        {
            FM_XLOG(XLOG_WARNING, "CFileCache::%s, Invaid URL parameter format.\n", __FUNCTION__);
            return false;
        }

        string strKey = boost::trim_copy(sMatch[1].str());
        string strEqualitySign = sMatch[2].str();
        bool bIsFixed = (0 == sMatch[3].length());
        string strValue = boost::trim_copy(sMatch[4].str());

        if (bIsFixed || strValue.empty())
        {
            sSlice._sUrlInfo._strFixedParams += strKey + "=" + strValue + "&";
            sSlice._sUrlInfo._mapFixedParam[strKey] = strValue;
        }
        else 
        {
            sSlice._sUrlInfo._mapVarParam[strKey] = strValue;
            sSlice._sUrlInfo._vecVarParamName.push_back(strKey);
            sSlice._sUrlInfo._vecVarParamReference.push_back(strValue);
        }
    }

    //remove the last character '&'
    REMOVE_LAST_CHARACTER(sSlice._sUrlInfo._strFixedParams);

    std::sort(sSlice._sUrlInfo._vecVarParamName.begin(), sSlice._sUrlInfo._vecVarParamName.end());

    //construct format params
    return SetFormatParams(sSlice);
}

bool CFileCache::SetFormatParams(SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);

    unsigned int nVecSize = sSlice._sUrlInfo._vecVarParamName.size();
    for (unsigned int iIter = 0; iIter < nVecSize; ++iIter)
    {
        string strNoPercentSign = boost::regex_replace(sSlice._sUrlInfo._vecVarParamName[iIter], boost::regex("%"), "%%", REGEX_MATCH_FLAGS);
        sSlice._sUrlInfo._strFormatParams += strNoPercentSign + "=%s&";
    }

    if (!sSlice._sUrlInfo._strFixedParams.empty())
    {
        string strNoPercentSign = boost::regex_replace(sSlice._sUrlInfo._strFixedParams, boost::regex("%"), "%%", REGEX_MATCH_FLAGS);
        sSlice._sUrlInfo._strFormatParams += strNoPercentSign + "&";
    }

    //remove the last character '&'
    REMOVE_LAST_CHARACTER(sSlice._sUrlInfo._strFormatParams);

    return true;
}

bool CFileCache::ParseIncludeCommand(SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s, Include[%s]\n", __FUNCTION__, sSlice._strContent.c_str());

    sSlice._bIsDynamic = false;
    sSlice._bIsInclude = true;

    if (!ParseIncludePath(sSlice))
    {
        sSlice._bIsInclude = false;
        sSlice._strContent = "";
    }
    else
    {
        m_mapReplaceFile[sSlice._strIncludePath] = EMPTY_FILE_PTR;
        m_setIncludePath.insert(sSlice._strIncludePath);
        m_dwIncludeNumber = m_setIncludePath.size();
    }
    return true;
}

bool CFileCache::ParseIncludePath(SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);

    if (sSlice._strContent.empty())
    {
        FM_XLOG(XLOG_WARNING, "CFileCache::%s, The 'include' path is empty.\n", __FUNCTION__);
        return false;
    }

    string strAbsIncludePath;
    if (!boost::algorithm::istarts_with(sSlice._strContent, PATH_SEPARATOR))
    {
        boost::filesystem::path filePath(m_strAbsPath);
        strAbsIncludePath = filePath.parent_path().string() + PATH_SEPARATOR + sSlice._strContent;
    }
    else 
    {
        strAbsIncludePath = m_strAbsRootPath + sSlice._strContent.substr(1);
    }

    //get absolute path that doesn't contain './' and '../'
    strAbsIncludePath = boost::filesystem::system_complete(strAbsIncludePath).string();

    //record relative include path 
    sSlice._strIncludePath = strAbsIncludePath.substr(m_strAbsRootPath.size());

    return true;
}

bool CFileCache::ParseSsiVirtualCommand(SSlice &sSlice)
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    if (!ParseIncludePath(sSlice))
    {
        sSlice._strContent = "";
        return false;
    }

    string strAbsIncludePath = m_strAbsRootPath + sSlice._strIncludePath;
    if (boost::filesystem::exists(strAbsIncludePath))
    {
        sSlice._bIsInclude = true;
        m_mapReplaceFile[sSlice._strIncludePath] = EMPTY_FILE_PTR;
        m_setIncludePath.insert(sSlice._strIncludePath);
        m_dwIncludeNumber = m_setIncludePath.size();
        return true;
    }
    else
    {
        return ParseRequestCommand(sSlice);
    }
}

bool CFileCache::IntegrateStaticSlices()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);

    list<SSlice>::iterator iterStart = m_listSlice.end();
    bool bPreStatic = false;
    for (list<SSlice>::iterator iter = m_listSlice.begin(); iter != m_listSlice.end(); ++iter)
    {
        if (iter->_bIsDynamic)
        {
            if (bPreStatic)
            {
                iterStart++;
                m_listSlice.erase(iterStart, iter);
                bPreStatic = false;
            }
            iterStart = iter;
            iterStart++;
        }
        else
        {
            if (bPreStatic)
            {
                iterStart->_strContent += iter->_strContent;
            }
            else
            {
                iterStart = iter;
            }
            bPreStatic = true;
        }
    }

    if (bPreStatic)
    {
        m_listSlice.erase(++iterStart, m_listSlice.end());
    }

    return true;
}

bool CFileCache::RecordDynamicSlices()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    list<SSlice>::iterator iter = m_listSlice.begin();
    while (iter != m_listSlice.end())
    {
        if (iter->_bIsDynamic)
        {
            SDynamicRequestInfo sDynReqInfo;
            sDynReqInfo._bIsHandled = false;
            sDynReqInfo._sUrlInfo = iter->_sUrlInfo;
            m_vecDynReqInfo.push_back(sDynReqInfo);
        }
        ++iter;
    }

    return true;
}

bool CFileCache::SetFormatContent()
{
    FM_XLOG(XLOG_DEBUG, "CFileCache::%s\n", __FUNCTION__);
    
    if (m_vecDynReqInfo.empty() && !m_listSlice.empty())
    {
        m_strContent = m_listSlice.begin()->_strContent;
        m_listSlice.clear();
    }
    else
    {
        m_strContent.clear();
        for (list<SSlice>::iterator iter = m_listSlice.begin(); iter != m_listSlice.end(); ++iter)
        {
            m_strContent += (iter->_bIsDynamic ? "%s" : boost::regex_replace(iter->_strContent, boost::regex("%"), "%%", REGEX_MATCH_FLAGS));
        }
    }

    return true;
}
