#include "SocConfUnit.h"
#include <cstdio>
/*
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
	SAP_Attri_vendorSpecific=26,
	SAP_Attri_ALL
}ESapMsgAttriId;

*/
const string CSapInterpreter::m_astrCommand[SAP_MSG_ALL]=
{
    "","Register","ReRegister","HeartBeat","NotifySpsIpPortList",
    "GetSps","SubscribeMsg","UnSubScribeMsg","GetSubScribeSoc",
    "SendSocMsg","GetAllSps","AdminRegister","AdminCommand","SpsDisconnect"
};
const string CSapInterpreter::m_astrAttri[SAP_Attri_ALL]=
{
    "","type","userName","realm","nonce",
    "sessionKey","digestReponse","encKey","addr",
    "socUserName","sapPort","adminRequest","adminResponse",
    "","","","",
    "","","","",
    "","","","",
    "","vendorSpecific"
};
const EAttriValueType CSapInterpreter::m_astrAttriType[SAP_Attri_ALL]=
{
    E_ATTRI_VALUE_INT,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,
    E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,
    E_ATTRI_VALUE_STR,E_ATTRI_VALUE_INT,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,
    E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,
	E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,
	E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR,
	E_ATTRI_VALUE_STR,E_ATTRI_VALUE_STR
};

int CSapInterpreter::GetCommandId(const string &strCommand)
{
    
    int i=0;
    for(i=1;i<SAP_MSG_ALL;i++)
    {
        //printf("match[%s],[%s]\n",m_astrCommand[i].c_str(),strCommand.c_str());
        if(strcasecmp(m_astrCommand[i].c_str(),strCommand.c_str())==0)
            break;
    }
    return i;
}
int CSapInterpreter::GetAttriId(const string &strCommand)
{
    int i=0;
    for(i=1;i<SAP_Attri_ALL;i++)
    {
        if(strcasecmp(m_astrAttri[i].c_str(),strCommand.c_str())==0)
            break;
    }
    return i;
}
EAttriValueType CSapInterpreter::GetAttriType(const string &strAttri)
{
    int atrriId=GetAttriId(strAttri);
    return m_astrAttriType[atrriId];
}
int CSapInterpreter::GetTypeValue(const string &strType)
{
    if(strcasecmp(strType.c_str(),"SOC")==0)
        return SAP_SERVER_SOC;
    if(strcasecmp(strType.c_str(),"SPS")==0)
        return SAP_SERVER_SPS;
    if(strcasecmp(strType.c_str(),"SOS")==0)
        return SAP_SERVER_SOS;
    if(strcasecmp(strType.c_str(),"SAG")==0)
        return SAP_SERVER_SAG;
}
string CSapInterpreter::GetCommandName(int index)
{
    if(index<0||index>=SAP_MSG_ALL)
        return "";
    else
        return m_astrCommand[index];
}

