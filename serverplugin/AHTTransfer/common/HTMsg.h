#ifndef _HT_MSG_H
#define _HT_MSG_H

#include <map>
#include <string>
#include <vector>
#include <detail/_time.h>
#include "HTDealer.h"

#include "HttpClientBase.h"
using namespace HT_COH;

using std::map;
using std::string;
using std::vector;

//extern class CHTDealer;

namespace HT{

#define PACKET_LEN 60000
#define STR_TEMP 256
#define IN  
#define OUT  
//class CHTDealer;

typedef struct stSessionInfo
{
//CBusinessCohClientImp object
	HttpClientBase *m_pCohClient;
//request 
	struct timeval_a m_tStartInPlatform;			//平台内处理开始时间
	struct timeval_a m_tStartInService; 			//请求服务开始时间
	struct timeval_a m_tEndInService; 				//请求服务结束时间
	struct timeval_a m_tEndInPlatform; 				//平台内处理结束时间
	float m_fSpentTimeInPlatform;					//平台内消耗时间
	float m_fSpenttimeInService; 					//消耗时间
	
	void *m_handler;	
	
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwSequenceId;
	
	string strGuid;	
	
	CHTDealer *m_htD;
	
	
	string m_uri;                                   //发送的uri
	string severAddr;                               //配置的服务地址
	
	string ipPort;                                  //实际发送的ip+端口号
	
	int m_nSendRecvFlag;							//当时请求是发送还是接收端， 1表示是发送，2 表示是接收
//response
	int m_nPlatformCode;   							//返回码，用于判断当前请求是否成功，用于统计上报信息
	string m_request;    							//请求包
	int m_nSubSvrCode;								//子服务的返回码, 用于打印异步日志		
	string m_response;								//应答包

	stSessionInfo():
		m_pCohClient(NULL), 
		m_fSpentTimeInPlatform(0),m_fSpenttimeInService(0),
		m_nPlatformCode(-1),m_request("Unknown"), 
		m_nSubSvrCode(-1),m_response("Error"){}
	~stSessionInfo(){}
}SSessionInfo;
	
	
#define PROTOCOL_HTTP  1
#define PROTOCOL_HTTPS 2

typedef enum eESapAhtMsgId
{
	E_SEND = 1,
	E_REV = 2,
	E_MSG_MAX
}ESapAhtMsgId;

typedef struct stHTTypeMsg
{
    ESapAhtMsgId m_enHTType;	
	struct timeval_a tStart;
	struct timeval_a httpEnd;
	
		
	stHTTypeMsg(){}
	virtual ~stHTTypeMsg(){}
}SHTTypeMsg;

typedef struct stHTRecMsg : public stHTTypeMsg
{
	int m_nCohClientId;
	string m_strHttpRetPacket;
	int m_dwProtocol;
	//struct timeval_a m_tStart;
		
    stHTRecMsg():m_dwProtocol(PROTOCOL_HTTP){ m_enHTType = E_REV;}
	~stHTRecMsg(){}
}SHTRcvMsg;


typedef struct stHTSendMsg : public stHTTypeMsg
{
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwSequenceId;
	
	string strGuid;		
	CHTDealer *m_htD;
	void *handler;
	
	void *pAvenueBody;
	unsigned int nAvenueBodyLen;
	
	//struct timeval_a tStart;
	
    stHTSendMsg():pAvenueBody(NULL){ m_enHTType = E_SEND;}
	virtual ~stHTSendMsg(){if(pAvenueBody != NULL){free(pAvenueBody); pAvenueBody = NULL;}}
}SHTSendMsg;

}

#endif

