#ifndef _SESSION_MANAGER_MSG_H_
#define _SESSION_MANAGER_MSG_H_
#include "SocConfUnit.h"


typedef enum
{
	E_SESSION_START_MSG=1,
	E_SESSION_TIMEOUT_MSG=2,
	E_SAP_CONNECT_RESULT_MSG=3,
	E_SAP_RECEIVE_MSG=4,
	E_SAP_PEER_CLOSE_MSG=5,
	E_SAP_RECEIVE_CONNECT_MSG=6,
	E_SAP_SEND_CMD_MSG=7,
	E_SESSION_ALL_MSG
}ESessionMsg;
typedef struct stSessionMsg
{
	ESessionMsg m_enType;	

	stSessionMsg(ESessionMsg enType):m_enType(enType){}
	virtual ~stSessionMsg(){}
}SSessionMsg;

typedef struct stSessionStartMsg:public SSessionMsg
{
	stSessionStartMsg():SSessionMsg(E_SESSION_START_MSG){}
}SSessionStartMsg;

typedef struct stSessionTimeoutMsg:public SSessionMsg
{
	int m_nId;
	stSessionTimeoutMsg():SSessionMsg(E_SESSION_TIMEOUT_MSG){}
}SSessionTimeoutMsg;

typedef struct stSapConnectResultMsg:public SSessionMsg
{
	int m_nId;
	int m_nResult;

	stSapConnectResultMsg():SSessionMsg(E_SAP_CONNECT_RESULT_MSG){}
}SSapConnectResultMsg;

typedef struct stSapReceiveMsg:public SSessionMsg
{
	int m_nId;
	void *m_pBuffer;
	int m_nLen;

	stSapReceiveMsg():SSessionMsg(E_SAP_RECEIVE_MSG){}
}SSapReceiveMsg;

typedef struct stSapPeerCloseMsg:public SSessionMsg
{
	int m_nId;

	stSapPeerCloseMsg():SSessionMsg(E_SAP_PEER_CLOSE_MSG){}
}SSapPeerCloseMsg;

typedef struct stSapReceiveConnectMsg:public SSessionMsg
{
	int m_nId;
	string m_strIp;
	unsigned int m_dwPort;
	stSapReceiveConnectMsg():SSessionMsg(E_SAP_RECEIVE_CONNECT_MSG){}
}SSapReceiveConnectMsg;

typedef struct stSessionSendCmdMsg:public SSessionMsg
{
	int m_nId;
	const SSapCommandTest *m_pCmd;
	stSessionSendCmdMsg():SSessionMsg(E_SAP_SEND_CMD_MSG){}
}SSessionSendCmdMsg;

#endif
