#ifndef __TEST_COOMOM_CPP__
#define __TEST_COOMOM_CPP__
#include <stdio.h>
#include "AvenueServiceConfigs.h"
#include "SdkLogHelper.h"
#include <arpa/inet.h>

#include "AvenueMsgHandler.h"
#include <time.h>
#include <sys/time.h>

#define MAX_PATH_MAX 128
using namespace sdo::commonsdk;
typedef enum
{
	SAP_PACKET_REQUEST=0xA1,
	SAP_PACKET_RESPONSE=0xA2
}ESapPacketType;
typedef struct tagSapMsgAttribute
{
	unsigned short wType;
	unsigned short wLength;
	unsigned char acValue[0];
}SSapMsgAttribute;


typedef struct tagSapMsgHeader
{
	unsigned char byIdentifer;
	unsigned char byHeadLen;
	unsigned char byVersion;
	unsigned char byM;

	unsigned int dwPacketLen;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwSequence;

	unsigned char byContext;
	unsigned char byPrio;
	unsigned char byBodyType;
	unsigned char byCharset;
	
	unsigned int dwCode;
	unsigned char bySignature[16];
}SSapMsgHeader;

class CSapEncoder
{
public:
	CSapEncoder(unsigned char byIdentifer,unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwCode=0);
	void SetBody(const void *pBuffer, unsigned int nLen);
	void SetValue(unsigned short wKey,const void* pValue,unsigned int nValueLen);
	void SetValue(unsigned short wKey,const string &strValue);
	void SetValue(unsigned short wKey,unsigned int wValue);
	void BeginValue(unsigned short wType);
	void AddValueBloc(const void *pData,unsigned int nLen);
	void EndValue();
	const void * GetBuffer()const{return m_buffer;}
	int GetLen()const{return m_len;}
private:
	unsigned char  m_buffer[1024];
	int m_len;
	unsigned char *pAttriBlock;
};

CSapEncoder::CSapEncoder(unsigned char byIdentifer,unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwCode)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)m_buffer;
    pHeader->byContext=0;
	pHeader->byPrio=0;
	pHeader->byBodyType=0;
	pHeader->byCharset=1;

    pHeader->byHeadLen=sizeof(SSapMsgHeader);
    pHeader->dwPacketLen=htonl(sizeof(SSapMsgHeader));
    pHeader->byVersion=0x01;
    pHeader->byM=0xFF;
    pHeader->dwCode=0;
    memset(pHeader->bySignature,0,16);
    m_len=44;

    pHeader->byIdentifer=byIdentifer;
    pHeader->dwServiceId=htonl(dwServiceId);
    pHeader->dwMsgId=htonl(dwMsgId);
    pHeader->dwCode=htonl(dwCode);
    
}
void CSapEncoder::SetBody(const void *pBuffer, unsigned int nLen)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)m_buffer;
    memcpy(m_buffer+m_len,pBuffer,nLen);
    m_len+=nLen;
    pHeader->dwPacketLen=htonl(m_len);
}

void CSapEncoder::SetValue(unsigned short wKey,const void* pValue,unsigned int nValueLen)
{
    unsigned int nFactLen=((nValueLen&0x03)!=0?((nValueLen>>2)+1)<<2:nValueLen);
    SSapMsgHeader *pHeader=(SSapMsgHeader *)m_buffer;
    SSapMsgAttribute *pAtti=(SSapMsgAttribute *)(m_buffer+m_len);
    pAtti->wType=htons(wKey);
    pAtti->wLength=htons(nValueLen+sizeof(SSapMsgAttribute));
    memcpy(pAtti->acValue,pValue,nValueLen);
    memset(pAtti->acValue+nValueLen,0,nFactLen-nValueLen);
    m_len+=(sizeof(SSapMsgAttribute)+nFactLen);
    pHeader->dwPacketLen=htonl(m_len);
}
void CSapEncoder::SetValue(unsigned short wKey, const string &strValue)
{
    SetValue(wKey,strValue.c_str(),strValue.length());
}
void CSapEncoder::SetValue(unsigned short wKey, unsigned int wValue)
{
    int nNetValue=htonl(wValue);
    SetValue(wKey,&nNetValue,4);
}

void CSapEncoder::BeginValue(unsigned short wType)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)m_buffer;
    SSapMsgAttribute * pAttri=(SSapMsgAttribute *)(m_buffer+m_len);
    pAttri->wType=htons(wType);
    m_len+=(sizeof(SSapMsgAttribute));
    pHeader->dwPacketLen=htonl(m_len);

    pAttriBlock=(unsigned char *)pAttri;
}
void CSapEncoder::AddValueBloc(const void *pData,unsigned int nLen)
{
    unsigned int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

    SSapMsgHeader *pHeader=(SSapMsgHeader *)m_buffer;
    memcpy(m_buffer+m_len,pData,nLen);
    memset(m_buffer+m_len+nLen,0,nFactLen-nLen);
    m_len+=(nFactLen);
    pHeader->dwPacketLen=htonl(m_len);
}
void CSapEncoder::EndValue()
{
    ((SSapMsgAttribute *)pAttriBlock)->wLength=htons(m_buffer+m_len-pAttriBlock);
}
void dump_sap_packet_notice(const void *pBuffer)
{
	string strPacket="";

	SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
	
	unsigned char *pChar=(unsigned char *)pBuffer;
	unsigned int nPacketlen=ntohl(pHeader->dwPacketLen);
	int nLine=nPacketlen>>3;
	int nLast=(nPacketlen&0x7);
	int i=0;
	for(i=0;(i<nLine)&&(i<200);i++)
	{
		char szBuf[128]={0};
		unsigned char * pBase=pChar+(i<<3);
		sprintf(szBuf,"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
			i,*(pBase),*(pBase+1),*(pBase+2),*(pBase+3),*(pBase+4),*(pBase+5),*(pBase+6),*(pBase+7));
		strPacket+=szBuf;
	}
	
	unsigned char * pBase=pChar+(i<<3);
	if(nLast>0)
	{
		for(int j=0;j<8;j++)
		{
			char szBuf[128]={0};
			if(j==0)
			{
				sprintf(szBuf,"                [%2d]    %02x ",i,*(pBase+j));
				strPacket+=szBuf;
			}
			else if(j<nLast&&j==3)
			{
				sprintf(szBuf,"%02x    ",*(pBase+j));
				strPacket+=szBuf;
			}
			else if(j>=nLast&&j==3)
			{
				strPacket+="      ";
			}
			else if(j<nLast)
			{
				sprintf(szBuf,"%02x ",*(pBase+j));
				strPacket+=szBuf;
			}
			else
			{
				strPacket+="   ";
			}
			
		}
		strPacket+="   ......\n";
	}
	printf("packet:\n%s",strPacket.c_str());
}


void dump_buffer_notice(const void *pBuffer, unsigned int nLen)
{
	string strPacket="";
	unsigned char *pChar=(unsigned char *)pBuffer;
	int nLine=nLen>>3;
	int nLast=(nLen&0x7);
	int i=0;
	for(i=0;(i<nLine)&&(i<200);i++)
	{
		char szBuf[128]={0};
		unsigned char * pBase=pChar+(i<<3);
		sprintf(szBuf,"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
			i,*(pBase),*(pBase+1),*(pBase+2),*(pBase+3),*(pBase+4),*(pBase+5),*(pBase+6),*(pBase+7));
		strPacket+=szBuf;
	}
	
	unsigned char * pBase=pChar+(i<<3);
	if(nLast>0)
	{
		for(int j=0;j<8;j++)
		{
			char szBuf[128]={0};
			if(j==0)
			{
				sprintf(szBuf,"                [%2d]    %02x ",i,*(pBase+j));
				strPacket+=szBuf;
			}
			else if(j<nLast&&j==3)
			{
				sprintf(szBuf,"%02x    ",*(pBase+j));
				strPacket+=szBuf;
			}
			else if(j>=nLast&&j==3)
			{
				strPacket+="      ";
			}
			else if(j<nLast)
			{
				sprintf(szBuf,"%02x ",*(pBase+j));
				strPacket+=szBuf;
			}
			else
			{
				strPacket+="   ";
			}
			
		}
		strPacket+="   ......\n";
	}
	printf("packet:\n%s",strPacket.c_str());
}

static char s_szLogName[MAX_PATH_MAX]="log.properties";
char* GetExeName()
{
	static char buf[MAX_PATH_MAX] = {0};
	int rslt   =   readlink("/proc/self/exe",   buf,   MAX_PATH_MAX);
	if(rslt < 0 || rslt >= MAX_PATH_MAX)
	{
		return   NULL;
	}
	buf[rslt]   = '\0';
	return   buf;
}

string GetCurrentPath()
{
	static char buf[MAX_PATH_MAX];
	strcpy(buf, GetExeName());
	char* pStart    =   buf + strlen(buf) - 1;
	char* pEnd      =   buf;

	if(pEnd < pStart)
	{
		while(pEnd !=  --pStart)
		{
			if('/' == *pStart)
			{
				*++pStart = '\0';
				break;
			}
		}
	}
	return buf;
}

#endif
