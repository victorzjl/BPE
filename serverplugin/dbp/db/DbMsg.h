#ifndef _DB_MSG_H_
#define _DB_MSG_H_

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#else


#define snprintf _snprintf
#define usleep(x)
#endif

#include <string>
#include "IAsyncVirtualService.h"
#include "AsyncVirtualServiceLog.h"
#include <detail/_time.h>

#include <vector>
//#include "SapMessage.h"

//using sdo::sap::CSapDecoder;
//using sdo::sap::CSapEncoder;



#include "AvenueSmoothMessage.h"

using sdo::commonsdk::CAvenueSmoothDecoder;
using sdo::commonsdk::CAvenueSmoothEncoder;

#define CSapDecoder sdo::commonsdk::CAvenueSmoothDecoder
#define CSapEncoder sdo::commonsdk::CAvenueSmoothEncoder

//using namespace sdo::sap;
#define		GUID_CODE			9
using namespace std;
#include "dbconfigcommon.h"

enum
{
	ERROR_TIMEOUT= 				-10901001,//请求等待时间超过设定时间5s
	ERROR_QUEUEFULL= 			-10901002,//子服务队列满
	ERROR_NOAPP=				-10901003,//找不到应用
	ERROR_DB=           		-10901004,//数据库错误
	ERROR_TIMEOUT_NOCONNECT=    -10901005,
	ERROR_BIND_VAR_NOT_MATCH= 	-10901006,//入参和绑定的变量不匹配
	ERROR_NO_DIVIDEKEY= 		-10901007,//没有分库分表的key
	ERROR_MISSPARA=				-10901008,//输入缺少请求绑定参数
	ERROR_BINDRESPONSE=			-10901009,//绑定输出参数错误
	ERROR_READONLY=				-10901010,//服务设置了READONLY,不能写操作
	ERROR_BIGQUEUEFULL=			-10901011,//BPE调用放入的队列满
	ERROR_TIMEOUT_MAINQUEUE=	-10901012,//主队列超时
};

typedef enum
{
	DB_CONFIG_MSG=1,
	DB_QUREY_MSG=2,
	DB_CONNECT_MSG=3,
	DB_ALL_MSG
}EDbMsgType;


typedef struct stDbMsg
{
	EDbMsgType m_enType;	
	struct timeval_a m_tStart;
	struct timeval_a m_tEnd;
	stDbMsg(EDbMsgType enType):m_enType(enType){}
	virtual ~stDbMsg(){}
}SDbMsg;


typedef struct stDbConfigMsg:public SDbMsg
{	
	stDbConfigMsg():SDbMsg(DB_CONFIG_MSG){}
	virtual ~stDbConfigMsg(){}
}SDbConfigMsg;


typedef struct stDbQueryResponse
{
	int nCode;  
	int nAffectCount;
	bool isqueryed;
	string strSql;
	CSapEncoder sapEnc;
	struct timeval_a m_tStartDB;
	struct timeval_a m_tEndDB;
	stDbQueryResponse():nCode(0),nAffectCount(0){isqueryed = false;}
}SDbQueryResponse;

typedef struct stDbQueryMsg:public SDbMsg
{	
	int nDbIndex;
	bool bQueryMaster;
	CSapDecoder sapDec;
	void *pRequest;
	int nReqLen;
	string strGuid;
	SDivideDesc divideDesc;
	void* sParams;
	void* sVoid;
	struct timeval_a m_tPopMainQueue;
	//struct timeval_a m_tPopAgentQueue;
	SDbQueryResponse sResponse;
	SMsgAttri *pMsgAttri;
	SLastErrorDesc errDesc;
	stDbQueryMsg():SDbMsg(DB_QUREY_MSG),bQueryMaster(false),pRequest(NULL),pMsgAttri(NULL){}
	virtual ~stDbQueryMsg(){free(pRequest);pRequest = NULL;}
}SDbQueryMsg;


typedef struct stDbConnectMsg:public SDbMsg
{
	int nDbIndex;
	void *pConn;

	stDbConnectMsg():stDbMsg(DB_CONNECT_MSG){}
	virtual ~stDbConnectMsg(){}
}SDbConnectMsg;


typedef struct stDbConfig
{
	
	int nIndex;
	string strMaster;
	vector<string> vecSlaves;
	vector<int> vecAppids;
	int nConnectNum;
}SDbConfig;

typedef struct stConnItem
{
	int nConnNum;
	bool bIsDivide;
	int nDBFactor;
	int nTBFactor;
	string sFactorType;

	vector<string> vecDivideConn;
	string sDefaultConn;
	
	stConnItem():nConnNum(0),bIsDivide(false),nDBFactor(1),nTBFactor(1),sFactorType("mod"){}
} SConnItem;

typedef struct stConnDesc
{
	vector<int> vecServiceId;
	int dwSmart;
	string    strPreSql;
	SConnItem masterConn;
	SConnItem slaveConn;
} SConnDesc;



#ifdef WIN32
#include "windows.h"
#define sleep Sleep
#include <time.h>
#if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_)
     struct timeval 
     {
        long tv_sec;
        long tv_usec;
     };
#endif 

	int gettimeofday(struct timeval* tv,int dummy) ;
  


#endif

#endif







