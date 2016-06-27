#ifndef _SDO_SAP_COMMON_H_
#define _SDO_SAP_COMMON_H_
namespace sdo{
namespace sap{
const unsigned char SAP_PLATFORM_SERVICE_ID=0x01;

typedef enum
{
	SAP_MSG_Register=1,
	SAP_MSG_ReRegister=2,
	SAP_MSG_HeartBeat=3,
	SAP_MSG_NotifySpsIpPortList=4,
	SAP_MSG_GetSps=5,
	SAP_MSG_SubscribeMsg=6,
	SAP_MSG_UnSubScribeMsg=7,
	SAP_MSG_GetSubScribeSoc=8,
	SAP_MSG_SendSocMsg=9,
	SAP_MSG_GetAllSps=10,
	SAP_MSG_AdminRegister=11,
	SAP_MSG_AdminCommand=12,
	SAP_MSG_SpsDisconnect=13,
	SAP_MSG_GetSocConfig=14,
	SAP_MSG_SetSocConfig=15,
	SAP_MSG_ALL
}ESapMsgId;

typedef enum
{
	SAP_Attri_type=1,
	SAP_Attri_userName=2,
	SAP_Attri_realm=3,
	SAP_Attri_nonce=4,
	SAP_Attri_sessionKey=5,
	SAP_Attri_digestReponse=6,
	SAP_Attri_encKey=7,
	SAP_Attri_addr=8,
	SAP_Attri_socUserName=9,
	SAP_Attri_sapPort=10,
	SAP_Attri_adminRequest=11,
	SAP_Attri_adminResponse=12,
	SAP_Attri_config=13,
	SAP_Attri_vendorSpecific=26,
	SAP_Attri_ALL
}ESapMsgAttriId;

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
	SAP_CODE_REJECT=-10242403,
	SAP_CODE_NOT_FOUND=-10242404,
	SAP_CODE_NOT_SUPPORT=-10242405,
	SAP_CODE_STACK_TIMEOUT=-10242408,
	SAP_CODE_SEND_FAIL=-10242410,
	SAP_CODE_PACKET_ERROR=-10242413,
	SAP_CODE_MAX_FORWARD=10242483,
	SAP_CODE_DEGEST_ERROR=-10242493,
	SAP_CODE_TIMEOUT=-10242504
}ESapResponseCode;


typedef enum
{
	SAP_SERVER_SOC=1,
	SAP_SERVER_SPS=2,
	SAP_SERVER_SOS=3,
	SAP_SERVER_SAG=4
}ESapServerType;
}
}
#endif

