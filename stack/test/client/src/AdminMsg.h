#ifndef _ADMIN_MSG_H_
#define _ADMIN_MSG_H_
#include "SmallObject.h"
#include <string>
using std::string;
using sdo::common::CSmallObject;

typedef enum
{
	ADMIN_LOGIN_MSG=1,
	ADMIN_CONNECT_RESULT_MSG=2,
	ADMIN_RECEIVE_MESSAGE_MSG=3,
	ADMIN_PEER_CLOSE_MSG=4,
	ADMIN_SEND_REQUEST_MSG=5,
	ADMIN_ALL_MSG
}EAdminMsgType;

typedef struct tagAdminMsg:public CSmallObject
{
	EAdminMsgType m_enType;	

	tagAdminMsg(EAdminMsgType enType):m_enType(enType){}
	~tagAdminMsg(){}
}SAdminMsg;

typedef struct tagAdminLoginMsg:public SAdminMsg
{
	string m_strIp;
	int m_nPort;
	string m_strName;
	string m_strPass;

	tagAdminLoginMsg():tagAdminMsg(ADMIN_LOGIN_MSG){}
}SAdminLoginMsg;

typedef struct tagAdminConnectResultMsg:public SAdminMsg
{
	int m_nId;
	int m_nResult;

	tagAdminConnectResultMsg():tagAdminMsg(ADMIN_CONNECT_RESULT_MSG){}
}SAdminConnectResultMsg;

typedef struct tagAdminReceiveMsg:public SAdminMsg
{
	int m_nId;
	void *m_pBuffer;
	int m_nLen;

	tagAdminReceiveMsg():SAdminMsg(ADMIN_RECEIVE_MESSAGE_MSG){}
}SAdminReceiveMsg;

typedef struct tagAdminPeerCloseMsg:public SAdminMsg
{
	int m_nId;

	tagAdminPeerCloseMsg():SAdminMsg(ADMIN_PEER_CLOSE_MSG){}
}SdminPeerCloseMsg;


typedef struct tagAdminRequestMsg:public SAdminMsg
{
	string m_strRequest;

	tagAdminRequestMsg():tagAdminMsg(ADMIN_SEND_REQUEST_MSG){}
}SAdminRequestMsg;

#endif
