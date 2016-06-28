#include "AvenueSmoothMessage.h"
#include <boost/asio.hpp>
#include "Cipher.h"
#include <boost/thread/mutex.hpp>
#include <string>

using std::string;

using sdo::dbp::CCipher;

namespace sdo{
namespace commonsdk{
static boost::mutex s_mutexSequence;
unsigned int CAvenueSmoothEncoder::sm_dwSeq=0;
CAvenueSmoothEncoder::CAvenueSmoothEncoder()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->byContext=0;
	pHeader->byPrio=0;
	pHeader->byBodyType=0;
	pHeader->byCharset=0;

    pHeader->byHeadLen=sizeof(SAvenueMsgHeader);
    pHeader->dwPacketLen=htonl(sizeof(SAvenueMsgHeader));
    pHeader->byVersion=0x01;
    pHeader->byM=0xFF;
    pHeader->dwCode=0;
    memset(pHeader->bySignature,0,16);
    m_buffer.reset_loc(sizeof(SAvenueMsgHeader));
}
CAvenueSmoothEncoder::CAvenueSmoothEncoder(unsigned char byIdentifer,unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwCode)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->byContext=0;
	pHeader->byPrio=0;
	pHeader->byBodyType=0;
	pHeader->byCharset=0;

    pHeader->byHeadLen=sizeof(SAvenueMsgHeader);
    pHeader->dwPacketLen=htonl(sizeof(SAvenueMsgHeader));
    pHeader->byVersion=0x01;
    pHeader->byM=0xFF;
    pHeader->dwCode=0;
    memset(pHeader->bySignature,0,16);
    m_buffer.reset_loc(sizeof(SAvenueMsgHeader));

    pHeader->byIdentifer=byIdentifer;
    pHeader->dwServiceId=htonl(dwServiceId);
    pHeader->dwMsgId=htonl(dwMsgId);
    pHeader->dwCode=htonl(dwCode);
    
}
void CAvenueSmoothEncoder::Reset()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->byContext=0;
	pHeader->byPrio=0;
	pHeader->byBodyType=0;
	pHeader->byCharset=0;

    pHeader->byHeadLen=sizeof(SAvenueMsgHeader);
    pHeader->dwPacketLen=htonl(sizeof(SAvenueMsgHeader));
    pHeader->byVersion=0x01;
    pHeader->byM=0xFF;
    pHeader->dwCode=0;
    memset(pHeader->bySignature,0,16);
    m_buffer.reset_loc(sizeof(SAvenueMsgHeader));
}
void CAvenueSmoothEncoder::SetService(unsigned char byIdentifer,unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwCode)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->byIdentifer=byIdentifer;
    pHeader->dwServiceId=htonl(dwServiceId);
    pHeader->dwMsgId=htonl(dwMsgId);
    pHeader->dwCode=htonl(dwCode);
}
unsigned int CAvenueSmoothEncoder::CreateAndSetSequence()
{
    unsigned int dwSequnece;
    {
        boost::lock_guard<boost::mutex> guard(s_mutexSequence);
        if(++sm_dwSeq==0)
            sm_dwSeq++;
        dwSequnece=sm_dwSeq;
    }
    SetSequence(dwSequnece);
    return dwSequnece;
}
void CAvenueSmoothEncoder::SetSequence(unsigned int dwSequence)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->dwSequence=htonl(dwSequence);
}

void CAvenueSmoothEncoder::SetMsgOption(unsigned int dwOption)
{   
   // SDK_XLOG(XLOG_DEBUG, "CAvenueSmoothEncoder::%s\n",__FUNCTION__);
    unsigned char * pOption=(unsigned char *)(&dwOption);
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->byContext=pOption[0];
	pHeader->byPrio=pOption[1];
	pHeader->byBodyType=pOption[2];
	pHeader->byCharset=pOption[3];
}
void CAvenueSmoothEncoder::SetExtHeader(const void *pBuffer, unsigned int nLen)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    pHeader->byHeadLen=sizeof(SAvenueMsgHeader)+nLen;
    memcpy(m_buffer.top(),pBuffer,nLen);
    m_buffer.inc_loc(nLen);
    pHeader->dwPacketLen=htonl(m_buffer.len());
}
void CAvenueSmoothEncoder::SetBody(const void *pBuffer, unsigned int nLen)
{
    if(m_buffer.capacity()<nLen)
    {
        m_buffer.add_capacity(SAP_ALIGN(nLen-m_buffer.capacity()));
    }
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    memcpy(m_buffer.top(),pBuffer,nLen);
    m_buffer.inc_loc(nLen);
    pHeader->dwPacketLen=htonl(m_buffer.len());
}
void CAvenueSmoothEncoder::AesEnc(unsigned char abyKey[16])
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    unsigned int nLeft=0x10-(m_buffer.len()-pHeader->byHeadLen)&0x0f;
    if(m_buffer.capacity()<nLeft)
    {
        m_buffer.add_capacity(SAP_ALIGN(nLeft-m_buffer.capacity()));
    }
    pHeader=(SAvenueMsgHeader *)m_buffer.base();
    memset(m_buffer.top(),0,nLeft);
    m_buffer.inc_loc(nLeft);
    unsigned char *pbyInBlk=(unsigned char *)m_buffer.base()+pHeader->byHeadLen;
    CCipher::AesEnc(abyKey,pbyInBlk,m_buffer.len()-pHeader->byHeadLen,pbyInBlk);
    pHeader->dwPacketLen=htonl(m_buffer.len());
}
void CAvenueSmoothEncoder::SetSignature(const char * szKey, unsigned int nLen)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    int nMin=(nLen<16?nLen:16);
    memcpy(pHeader->bySignature,szKey,nMin);
    CCipher::Md5(m_buffer.base(),m_buffer.len(),pHeader->bySignature);
}
void CAvenueSmoothEncoder::SetValue(unsigned short wKey,const void* pValue,unsigned int nValueLen)
{
    unsigned int nFactLen=((nValueLen&0x03)!=0?((nValueLen>>2)+1)<<2:nValueLen);
    if(m_buffer.capacity()<nFactLen+4)
    {
        m_buffer.add_capacity(SAP_ALIGN(nFactLen+4-m_buffer.capacity()));
    }
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)m_buffer.top();
    pAtti->wType=htons(wKey);
    pAtti->wLength=htons(nValueLen+sizeof(SAvenueMsgAttribute));
    memcpy(pAtti->acValue,pValue,nValueLen);
    memset(pAtti->acValue+nValueLen,0,nFactLen-nValueLen);
    m_buffer.inc_loc(sizeof(SAvenueMsgAttribute)+nFactLen);
    pHeader->dwPacketLen=htonl(m_buffer.len());
}
void CAvenueSmoothEncoder::SetValue(unsigned short wKey, const string &strValue)
{
    SetValue(wKey,strValue.c_str(),strValue.length());
}
void CAvenueSmoothEncoder::SetValue(unsigned short wKey, unsigned int wValue)
{
    int nNetValue=htonl(wValue);
    SetValue(wKey,&nNetValue,4);
}

void CAvenueSmoothEncoder::BeginValue(unsigned short wType)
{
    if(m_buffer.capacity()<sizeof(SAvenueMsgAttribute))
    {
        m_buffer.add_capacity(SAP_ALIGN(sizeof(SAvenueMsgAttribute)-m_buffer.capacity()));
    }
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    SAvenueMsgAttribute * pAttri=(SAvenueMsgAttribute *)m_buffer.top();
    pAttri->wType=htons(wType);
    m_buffer.inc_loc(sizeof(SAvenueMsgAttribute));
    pHeader->dwPacketLen=htonl(m_buffer.len());

    pAttriBlock=(unsigned char *)pAttri;
}
void CAvenueSmoothEncoder::AddValueBloc(const void *pData,unsigned int nLen)
{
    unsigned int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);
    if(m_buffer.capacity()<nFactLen)
    {
        m_buffer.add_capacity(SAP_ALIGN(nFactLen-m_buffer.capacity()));
    }
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_buffer.base();
    memcpy(m_buffer.top(),pData,nLen);
    memset(m_buffer.top()+nLen,0,nFactLen-nLen);
    m_buffer.inc_loc(nFactLen);
    pHeader->dwPacketLen=htonl(m_buffer.len());
}
void CAvenueSmoothEncoder::EndValue(unsigned short dwLen)
{
    if(dwLen!=0)
        ((SAvenueMsgAttribute *)pAttriBlock)->wLength=htons(dwLen);
    else
        ((SAvenueMsgAttribute *)pAttriBlock)->wLength=htons(m_buffer.top()-pAttriBlock);
}


CAvenueSmoothDecoder::CAvenueSmoothDecoder(const void *pBuffer,int nLen)
{
    m_pBuffer=(void *)pBuffer;
    m_nLen=nLen;
}
CAvenueSmoothDecoder::CAvenueSmoothDecoder()
{
    m_pBuffer=0;
    m_nLen=0;
}
void CAvenueSmoothDecoder::Reset(const void *pBuffer,int nLen)
{
    m_pBuffer=(void *)pBuffer;
    m_nLen=nLen;
}

int CAvenueSmoothDecoder::VerifySignature(const char * szKey, unsigned int nLen)
{
    unsigned char arSignature[16];
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    memcpy(arSignature,pHeader->bySignature,16);
    
    int nMin=(nLen<16?nLen:16);
    memset(pHeader->bySignature,0,16);
    memcpy(pHeader->bySignature,szKey,nMin);

    CCipher::Md5((const unsigned char *)m_pBuffer,m_nLen,pHeader->bySignature);
    return memcmp(arSignature,pHeader->bySignature,16);
}

void CAvenueSmoothDecoder::AesDec(unsigned char abyKey[16])
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    unsigned char *ptrLoc=(unsigned char *)m_pBuffer+pHeader->byHeadLen;
    CCipher::AesDec(abyKey,ptrLoc,m_nLen-pHeader->byHeadLen,ptrLoc);
}

void CAvenueSmoothDecoder::GetService(unsigned char * pIdentifer,unsigned int * pServiceId,unsigned int * pMsgId,unsigned int * pdwCode)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    *pIdentifer=pHeader->byIdentifer;
    *pServiceId=ntohl(pHeader->dwServiceId);
    *pMsgId=ntohl(pHeader->dwMsgId);
    *pdwCode=ntohl(pHeader->dwCode);
}
unsigned char CAvenueSmoothDecoder::GetIdentifier()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    return pHeader->byIdentifer;
}
unsigned int CAvenueSmoothDecoder::GetServiceId()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    return ntohl(pHeader->dwServiceId);
}
unsigned int CAvenueSmoothDecoder::GetMsgId()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    return ntohl(pHeader->dwMsgId);
}
unsigned int CAvenueSmoothDecoder::GetCode()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    return ntohl(pHeader->dwCode);
}

unsigned int CAvenueSmoothDecoder::GetSequence()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    return ntohl(pHeader->dwSequence);
}
unsigned int CAvenueSmoothDecoder::GetMsgOption()
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    return *((int *)(&pHeader->byContext));
}
void CAvenueSmoothDecoder::GetExtHeader(void **ppBuffer, int * pLen)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;
    *ppBuffer=(unsigned char *)m_pBuffer+sizeof(SAvenueMsgHeader);
    *pLen=pHeader->byHeadLen-sizeof(SAvenueMsgHeader);
}
void CAvenueSmoothDecoder::GetBody(void **ppBuffer, unsigned int * pLen)
{
    SAvenueMsgHeader *pHeader=(SAvenueMsgHeader *)m_pBuffer;;
    *ppBuffer=(unsigned char *)m_pBuffer+pHeader->byHeadLen;
    *pLen=ntohl(pHeader->dwPacketLen)-pHeader->byHeadLen;
    
}
void CAvenueSmoothDecoder::DecodeBodyAsTLV()
{
    void * pBody;
    unsigned int nBodyLen;
    GetBody(&pBody,&nBodyLen);
    unsigned char *ptrLoc=(unsigned char *)pBody;
    while(ptrLoc<(unsigned char *)pBody+nBodyLen)
    {
        SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)ptrLoc;
        unsigned short nLen=ntohs(pAtti->wLength);
        if(nLen==0)
        {
            break;
        }
        m_mapMultiAttri.insert(AttriMultiMap::value_type(ntohs(pAtti->wType),ptrLoc));
        int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);
        ptrLoc+=nFactLen;
    }
}

int CAvenueSmoothDecoder::GetValue(unsigned short wKey,void** pValue, unsigned int * pValueLen) 
{
    AttriMultiMap::const_iterator itr=m_mapMultiAttri.find(wKey);
    if(itr==m_mapMultiAttri.end())
    {
        return -1;
    }

    const unsigned char *ptrLoc=itr->second;
    SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)ptrLoc;
    *pValueLen=ntohs(pAtti->wLength)-sizeof(SAvenueMsgAttribute);
    *pValue=pAtti->acValue;
    return 0;
}
int CAvenueSmoothDecoder::GetValue(unsigned short wKey,string & strValue)
{
    void *pData=NULL;
    unsigned int nLen=0;
    if(GetValue(wKey,&pData,&nLen)==-1||nLen<0)
    {
        return -1;
    }
    strValue=string((const char *)pData,nLen);
    return 0;

}
int CAvenueSmoothDecoder::GetValue(unsigned short wKey, unsigned int * pValue) 
{
    void *pData=NULL;
    unsigned int nLen=0;
    if(GetValue(wKey,&pData,&nLen)==-1||nLen!=4)
    {
        return -1;
    }
    *pValue=ntohl(*(int *)pData);
    return 0;
}

void CAvenueSmoothDecoder::GetValues(unsigned short wKey,vector<SAvenueValueNode> &vecValues)
{
    std::pair<AttriMultiMap::const_iterator, AttriMultiMap::const_iterator> itr_pair = m_mapMultiAttri.equal_range(wKey);
    AttriMultiMap::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
	    const unsigned char *ptrLoc=itr->second;
        SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)ptrLoc;

        SAvenueValueNode tmp;
        tmp.nLen=ntohs(pAtti->wLength)-sizeof(SAvenueMsgAttribute);
        tmp.pLoc=pAtti->acValue;
		vecValues.push_back(tmp);
	}
}
void CAvenueSmoothDecoder::GetValues(unsigned short wKey,vector<string> &vecValues)
{
    std::pair<AttriMultiMap::const_iterator, AttriMultiMap::const_iterator> itr_pair = m_mapMultiAttri.equal_range(wKey);
    AttriMultiMap::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
	    const unsigned char *ptrLoc=itr->second;
        SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)ptrLoc;

        int nLen=ntohs(pAtti->wLength)-sizeof(SAvenueMsgAttribute);
        if(nLen>0)
        {
            string strValue=string((const char *)pAtti->acValue,nLen);
	        vecValues.push_back(strValue);
        }
	}
}
void CAvenueSmoothDecoder::GetValues(unsigned short wKey,vector<unsigned int> &vecValues)
{
    std::pair<AttriMultiMap::const_iterator, AttriMultiMap::const_iterator> itr_pair = m_mapMultiAttri.equal_range(wKey);
    AttriMultiMap::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
	    const unsigned char *ptrLoc=itr->second;
        SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)ptrLoc;

        if(ntohs(pAtti->wLength)-sizeof(SAvenueMsgAttribute)==4)
        {
	        vecValues.push_back(ntohl(*(unsigned int *)pAtti->acValue));
        }
	}
}



}
}
