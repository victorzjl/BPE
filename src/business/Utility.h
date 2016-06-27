#ifndef _SAP_UTILITY_H_
#define _SAP_UTILITY_H_
#include <string>

using std::string;
const int DEFAULT_APPID = -10086;
const int DEFAULT_AREAID = -10087;
const int MAX_LOG_BUFFER = 2048;

typedef struct stRequestInfo
{
	int nAppId;
	int nAreaId;
	string strUserName;
    string strGuid;
    string strLogId;
    string strComment;
    stRequestInfo(int nAppId_  = DEFAULT_APPID,
        int nAreaId_ = DEFAULT_AREAID,
        const string& strName_ = "",
        const string& strGuid_ = "",
        const string& strLogId_ = "",
        const string& strComment_ = "")
        :nAppId(nAppId_),nAreaId(nAreaId_),strUserName(strName_), strGuid(strGuid_), strLogId(strLogId_), strComment(strComment_){}
}SRequestInfo;

const SRequestInfo GetRequestInfo(const void *pBuffer, int nLen);
const string GetRemoteIpAddr(const void* pExBuffer, int nExLen);
const string GetIpAddr(const void * pBuffer, int nLen, const void* pExBuffer, int nExLen);


const string TimeToString(const struct tm& tmRecord, unsigned ms);


#endif

