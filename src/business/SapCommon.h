#ifndef _SDO_SAP_COMMON_H_
#define _SDO_SAP_COMMON_H_
//	#include <vector>
//	#include <string>
//	using std::vector;
//	using std::string;

const unsigned int SAP_PLATFORM_SERVICE_ID=0x01;
const unsigned int SAP_SMS_SERVICE_ID=0x02;
const unsigned int SAP_SLS_SERVICE_ID=0x03;

/*range of compose service id*/
const unsigned int SAP_COMPOSE_ID_BEGIN=50000;
const unsigned int SAP_COMPOSE_ID_END=60000;


const unsigned int SMS_GET_CONFIG=0x01;
const unsigned int SMS_GET_PRIVILEGE=0x02;
const unsigned int SMS_GET_CFG_PRIV=0x03;

typedef enum
{
	SAP_MSG_Register=1,
	SAP_MSG_ReportSocState=2,
	SAP_MSG_Kicksoc=3,
	SAP_MSG_ALL
}ESapMsgId;

typedef enum
{
	SAP_Attri_userName=1,
	SAP_Attri_IsVerifySignature=2,
	SAP_Attri_nonce=3,
	SAP_Attri_digestReponse=4,
	SAP_Attri_encType=5,
	SAP_Attri_encKey=6,

	SAP_Attri_online=7,
	SAP_Attri_addr=8,
	SAP_Attri_state=9,
	SAP_Attri_version=10,
	SAP_Attri_buildTime=11,
	
	SAP_Attri_spid=12,
	SAP_Attri_serverid=13,
	SAP_Attri_areaid=14,
	SAP_Attri_groupid=15,
	SAP_Attri_hostid=16,
	SAP_Attri_sitecode=17,	
	SAP_Attri_ALL
}ESapMsgAttriId;

typedef enum
{
    SMS_Attri_userName=1,
    SMS_Attri_ip=2,
    SMS_Attri_port=3,
    SMS_Attri_confValue=4,
    SMS_Attri_privilige=5,
    SMS_Attri_ALL
}ESMSMsgAttriId;

typedef struct stKeyValue
{
	unsigned int nKey;
	unsigned char szValue[0];
}SKeyValue;

typedef enum
{
	SMS_CfgKey_SvrAddr = 1,
	SMS_CfgKey_SignKey = 2,
	SMS_CfgKey_SocPwd = 3,
	SMS_CfgKey_SpId = 4,
	SMS_CfgKey_AppId = 5, // ServerId
	SMS_CfgKey_AreaId = 6,
	SMS_CfgKey_GroupId = 7,
	SMS_CfgKey_HostId = 8,
	SMS_CfgKey_UserName = 9,
	SMS_CfgKey_Ip = 10,
	SMS_CfgKey_Port = 11,
}ESMSConfigKey;

/*typedef enum
{
	SMS_Attri_userName=1,
	SMS_Attri_ip=2,
	SMS_Attri_port=3,
	SMS_Attri_pwd=4,
	SMS_Attri_key=5,
	SMS_Attri_config=6,
	SMS_Attri_privilige=7,
	SMS_Attri_IsEnc=8,
	SMS_Attri_AppId=9,
	SMS_Attri_AreaId=10,
	SMS_Attri_GroupId=11,
	SMS_Attri_HostId=12,
	SMS_Attri_SpId=13,
	SMS_Attri_ALL
}ESMSMsgAttriId;*/

typedef struct
{
	unsigned int dwIp;
	unsigned short wPort; 
	unsigned short reserver;
}SSapMsgAttriAddr;

typedef enum
{
	SAP_CODE_SUCC=0,
	SAP_CODE_BAD_REQUEST=-10242400,
	SAP_CODE_UNAUTH=-10242401,
	SAP_CODE_REJECT=-10242403,
	SAP_CODE_NOT_FOUND=-10242404,
	SAP_CODE_NOT_SUPPORT=-10242405,
	SAP_CODE_STACK_TIMEOUT=-10242408,
	SAP_CODE_SEND_FAIL=-10242410,
	SAP_CODE_HAVE_LOGIN=-10242412,
	SAP_CODE_PACKET_ERROR=-10242413,
	SAP_CODE_STATE_ERROR=-10242455,
	SAP_CODE_MAX_FORWARD=-10242483,
	SAP_CODE_QUEUE_FULL=-10242488,
	SAP_CODE_DEGEST_ERROR=-10242493,
	SAP_CODE_FUNCALLERROR=-10242494,
	SAP_CODE_SERVER_ERROR=-10242500,
	SAP_CODE_TIMEOUT=-10242504,
	SAP_CODE_VIRTUAL_SERVICE_NOT_FOUND=-10242300,
	SAP_CODE_VIRTUAL_SERVICE_BAD_REQUEST=-10242301,
	SAP_CODE_COMPOSE_NODE_NOT_FOUND=-10242310,
	SAP_CODE_CONFIG_RELOAD=-10242320,
	SAP_CODE_COMPOSE_GOTO_ERROR =-10242326, //goto self, dead loop
	SAP_CODE_COMPOSE_SERVICE_NOT_FOUND =-10242327,
	SAP_CODE_SERVICE_UNAVAIABLE =-10242328,
	SAP_CODE_SERVICE_DECODE_ERROR = -10242329,
	SAP_CODE_KEY_ERROR = -10242250
}ESapResponseCode;

typedef enum
{
	SAP_SERVER_SOC=1,
	SAP_SERVER_SPS=2,
	SAP_SERVER_SOS=3,
	SAP_SERVER_SAG=4
}ESapServerType;

#define DATASERVICE_SERVICEID    0
#define DATASERVICE_MSGID        10101

typedef enum
{
    DATA_ATTR_SERVICE_ID    = 1,
    DATA_ATTR_MSG_ID        = 2,
    DATA_ATTR_RESULT_CODE   = 3,
    DATA_ATTR_REQUEST_DATA  = 4,
    DATA_ATTR_RESPONSE_DATA = 5,
    DATA_ATTR_DEF_VALUE     = 6,
}EDataMsgAttriId;

#endif

