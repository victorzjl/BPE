#ifndef _ASYNCLOG_MSG_H_
#define _ASYNCLOG_MSG_H_

#include "HpsCommonInner.h"

typedef enum
{
	ASYNCLOG_Http_Connect_MSG = 1,
	ASYNCLOG_Http_Request_MSG = 2,
	ASYNCLOG_DETAIL_REQUEST_MSG = 3,
	ASYNCLOG_SOS_STATUS_MSG = 4,
	ASYNCLOG_STOP_MSG = 5,
	ASYNCLOG_Receive_Avenue_Request_MSG = 6,
	ASYNCLOG_Avenue_ReqRes_Detail_MSG = 7,
	ASYNCLOG_Report_No_Http_Response_Num = 8,
	ASYNCLOG_ReConn_Warn = 9,
	ASYNCLOG_ALL_MSG
}EAsyncLogMsgType;

typedef struct stAsyncLogMsg
{
	EAsyncLogMsgType enType;

	stAsyncLogMsg(EAsyncLogMsgType enType_):enType(enType_){}
	virtual ~stAsyncLogMsg(){}
}SAsyncLogMsg;

typedef struct stAsyncLogHttpConnect:public SAsyncLogMsg
{
	SHttpConnectInfo sConnectInfo;

	stAsyncLogHttpConnect():stAsyncLogMsg(ASYNCLOG_Http_Connect_MSG){}
	virtual	~stAsyncLogHttpConnect(){}
}SAsyncLogHttpConnect;

typedef struct stAsyncLogHttpRequest:public SAsyncLogMsg
{
	unsigned int dwServiceId;
	unsigned int dwMsgId;

	stAsyncLogHttpRequest():stAsyncLogMsg(ASYNCLOG_Http_Request_MSG){}
	virtual	~stAsyncLogHttpRequest(){}
}SAsyncLogHttpRequest;


typedef struct stAsyncLogDetailRequestMsg:public SAsyncLogMsg
{
	SRequestInfo sRequest;
	
	stAsyncLogDetailRequestMsg():stAsyncLogMsg(ASYNCLOG_DETAIL_REQUEST_MSG){}
	virtual	~stAsyncLogDetailRequestMsg(){}
}SAsyncLogDetailRequestMsg;

typedef struct stAsyncLogSosStatusMsg:public SAsyncLogMsg
{
	SSosStatusData sSosStatus;

	stAsyncLogSosStatusMsg():SAsyncLogMsg(ASYNCLOG_SOS_STATUS_MSG){}
	virtual ~stAsyncLogSosStatusMsg(){}
}SAsyncLogSosStatusMsg;

typedef struct stAsyncLogStopMsg:public SAsyncLogMsg
{
	stAsyncLogStopMsg():SAsyncLogMsg(ASYNCLOG_STOP_MSG){}
	virtual ~stAsyncLogStopMsg(){}
}SAsyncLogStopMsg;

typedef struct stAsyncLogAvenueRequestDetailMsg:public SAsyncLogMsg
{
	SAvenueRequestInfo sAvenueReqInfo;
	stAsyncLogAvenueRequestDetailMsg():SAsyncLogMsg(ASYNCLOG_Avenue_ReqRes_Detail_MSG){}
	virtual ~stAsyncLogAvenueRequestDetailMsg(){}
}SAsyncLogAvenueRequestDetailMsg;


typedef struct stAsyncLogReceiveAvenueRequestMsg:public SAsyncLogMsg
{
	unsigned int dwServiceId;
	unsigned int dwMsgId;

	stAsyncLogReceiveAvenueRequestMsg():SAsyncLogMsg(ASYNCLOG_Receive_Avenue_Request_MSG){}
	virtual ~stAsyncLogReceiveAvenueRequestMsg(){}
}SAsyncLogReceiveAvenueRequestMsg;

typedef struct stAsyncLogReportNoHttpResNumMsg:public SAsyncLogMsg
{
	int nThreadIndex;
	unsigned int dwAvenueRequestNum;

	stAsyncLogReportNoHttpResNumMsg():SAsyncLogMsg(ASYNCLOG_Report_No_Http_Response_Num){}
	virtual ~stAsyncLogReportNoHttpResNumMsg(){}
}SAsyncLogReportNoHttpResNumMsg;

typedef struct stAsyncLogReConnWarnMsg:public SAsyncLogMsg
{
	string strReConnIp;

	stAsyncLogReConnWarnMsg():SAsyncLogMsg(ASYNCLOG_ReConn_Warn){}
	virtual ~stAsyncLogReConnWarnMsg(){}
}SAsyncLogReConnWarnMsg;

#endif

