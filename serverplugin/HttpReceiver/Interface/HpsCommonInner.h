#ifndef _HPS_COMMON_INNER_H_
#define _HPS_COMMON_INNER_H_

#include "detail/_time.h"
#include "HpsCommon.h"
#include <string>
#include <map>
#include <vector>
using std::string;
using std::map;
using std::vector;
using std::make_pair;

#define SAP_PLATFORM_SERVICE_ID 1
#define SAP_SMS_SERVICE_ID		2
#define SAP_SLS_SERVICE_ID		3

#define SAP_REGISTER_SLS_MESSAGEID 1

#define SAP_HEADEX_CODE_USER	1
#define SAP_HEADEX_CODE_ADDR	2
#define SAP_HEADEX_CODE_APPID	3
#define SAP_HEADEX_CODE_AREAID	4
#define SAP_HEADEX_CODE_GROUPID	5
#define SAP_HEADEX_CODE_HOSTID	6
#define SAP_HEADEX_CODE_SPID	7
#define SAP_HEADEX_CODE_GUID	9
#define SAP_HEADEX_CODE_COMMENT	11


#define TRANSEX_CODE_HTTPCODE	20
#define TRANSEX_CODE_COOKIE		21
#define TRANSEX_CODE_LOCATION	22
#define TRANSEX_CODE_CONTTYPE	23
#define TRANSEX_CODE_CHARSET	24

#define VER2SO_PRECODE	0xC4C3C2C1

#define MAXBUFLEN 1024*1024*4

#ifndef SERVICEMSGID
#define SERVICEMSGID
typedef struct stServiceMsgId
{
	unsigned int dwServiceID;
	unsigned int dwMsgID;
	
	stServiceMsgId():dwServiceID(0),dwMsgID(0){}
	stServiceMsgId(unsigned int serviceId, unsigned int msgId):dwServiceID(serviceId),dwMsgID(msgId){}

	bool operator<(const stServiceMsgId & oId)const
	{
		  return (this->dwServiceID < oId.dwServiceID ||(this->dwServiceID == oId.dwServiceID && this->dwMsgID < oId.dwMsgID));
	}
	bool operator==(const stServiceMsgId & oId)const
	{
		  return (this->dwServiceID == oId.dwServiceID && this->dwMsgID == oId.dwMsgID);
	}	
}SServiceMsgId;
#endif

typedef struct stHttpConnectInfo
{
	EHttpConnectType eType;
	unsigned int dwId;
	string strRemoteIp;
	unsigned int dwRemotePort;

	stHttpConnectInfo(EHttpConnectType  type, unsigned int id, string strIp, unsigned int port):
		eType(type),dwId(id), strRemoteIp(strIp), dwRemotePort(port){}
	stHttpConnectInfo(){}
	virtual ~stHttpConnectInfo(){}
}SHttpConnectInfo;

typedef struct stRequestInfo
{
	struct timeval_a tStart;
	struct timeval_a tEnd; //used for timeout check
	unsigned int dwSequence;
	unsigned int dwId;
	unsigned int dwSosId;
	bool  isAccessedSos;
	string strRemoteIp;
	unsigned int dwRemotePort;
	unsigned int m_nHttpVersion;
	unsigned int nServiceId;
	unsigned int nMsgId;
	string strUriCommand;
	string strUriAttribute;
	int nOrder;
	bool isKeepAlive;
        
    string sequenceNo;
    bool isVaild;
    std::multimap<std::string, std::string> headers;
    std::string strRpcGuid;

	float fSpentTime;
	//response message
	int nCode;
	int nHttpCode;
	string strHeaders;
	string strResponse;
	string strSosIp;
	unsigned int dwSosPort;
	string strResCharSet;
	string strContentType;
	
	string strOriInput;
	
	bool bFromApipool;
	string strUserName;
	string strSignature;
	string strUrlIdentify;
	string strFnCallback;
	int nAppid;
	int nAreaId;
	int nGroupId;
	int nHostid;
	int nSpid;
	int nTimestamp;
	string strGuid;
    string strComment;

	bool inWhiteList;
	bool inTmpWhiteList;
	bool bOsapCheck;
	string strSignatureSrc;
	int nSignatureStatus;
	string strHost;

	string strCookie;
	vector<string> vecSetCookie;
	string strLocationAddr;
        
    bool isNomalBpeRequest; // if the value of flag is true, it means the request can be found in mapping;if not, the request may ask for resource.
    ERequestType requestType;
    unsigned int dwOwnerSequence;

    string strUserAgent;
    string strResourceAbsPath;
    string strErrorReason;
    bool bIsTransfer;

	stRequestInfo():dwSosId(0),isAccessedSos(false),nCode(-1),nHttpCode(200),dwSosPort(0),strResCharSet("utf-8"),
		bFromApipool(false),nAppid(-1),nAreaId(-1),
		nGroupId(-1),nHostid(-1),nSpid(-1),nTimestamp(888888),
		inWhiteList(false),inTmpWhiteList(false),bOsapCheck(false),nSignatureStatus(0){}
	virtual ~stRequestInfo(){}
	
}SRequestInfo;


typedef struct stSosStatusData
{
	unsigned int dwActiveConnect;
	unsigned int dwDeadConnect;	
	vector<string> vecDeadSosAddr;

	stSosStatusData():dwActiveConnect(0),dwDeadConnect(0){}
	virtual ~stSosStatusData(){}

	friend stSosStatusData operator +(const stSosStatusData& a1,const stSosStatusData& a2)
	{
		stSosStatusData data;
		data.dwActiveConnect = a1.dwActiveConnect + a2.dwActiveConnect;	
		data.dwDeadConnect = a1.dwDeadConnect + a2.dwDeadConnect;
		vector<string>::const_iterator iter;
		for(iter = a1.vecDeadSosAddr.begin(); iter != a1.vecDeadSosAddr.end(); ++iter)
		{
			data.vecDeadSosAddr.push_back(*iter);
		}
		for(iter = a2.vecDeadSosAddr.begin(); iter != a2.vecDeadSosAddr.end(); ++iter)
		{
			data.vecDeadSosAddr.push_back(*iter);
		}
		return data;
	}
}SSosStatusData;

typedef struct stAvenueRequestInfo
{
	unsigned int dwHttpSequence;
	unsigned int dwSosSessionContainerId;
	unsigned int dwSosSessionId;
	unsigned int dwSosConnectionId;
	struct timeval_a tStart;
	struct timeval_a tHttpRequestStart;
	struct timeval_a tHttpResponseStop;	
	struct timeval_a tTimeOutEnd; //used for timeout check
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwAvenueSequence;
	string strSosIp;
	unsigned int dwSosPort;

	int nAvenueRetCode;
	int nHttpCode;

	float fAllSpentTime;
	float fHttpResponseTime;
	
	string strUsername;
	string strHttpMethod;
	string strHttpUrl;
	string strHttpRequestBody;
	
	string strHttpResponse;	

	stAvenueRequestInfo():dwHttpSequence(0),dwSosPort(0),nAvenueRetCode(-1),nHttpCode(-1),strHttpMethod("POST"){}
	virtual ~stAvenueRequestInfo(){}
	
}SAvenueRequestInfo;


typedef struct stHttpServerInfo
{
	string strIp;
	unsigned int dwPort;

	string strHost;
	string strPath;

	stHttpServerInfo(){}

	bool operator<(const stHttpServerInfo & oInfo)const
	{
		  return (this->strIp < oInfo.strIp ||(this->strIp == oInfo.strIp && this->dwPort < oInfo.dwPort));
	}
	bool operator==(const stHttpServerInfo & oInfo)const
	{
		  return (this->strIp == oInfo.strIp && this->dwPort == oInfo.dwPort);
	}
	
}SHttpServerInfo;

struct SUrlInfo
{
	string strSoName;
	unsigned dwServiceId;
	unsigned dwMessageId;
};


typedef struct stHeaderInfo
{
    int nHttpVersion; //0-1.0; 1-1.1
    bool isKeepAlive;	
    string strResCharSet;
    string strContentType;
    int nHttpCode;
    vector<string> vecSetCookie;
    string strLocationAddr;
    string strHeaders;
} SHeaderInfo;


typedef struct stTransferDataStruct
{
    char arrTransferData[MAXBUFLEN];
    SRequestInfo *pReq;
} STransferDataStruct;

#endif

