#include "SapTLVBody.h"
#include <boost/asio.hpp>
#include "SapLogHelper.h"

namespace sdo{
namespace sap{

void CSapTLVBodyEncoder::SetValue(unsigned short wKey,const void* pValue,unsigned int nValueLen,
	void* pDest, unsigned int& nDestLen)
{
	unsigned int nFactLen=((nValueLen&0x03)!=0?((nValueLen>>2)+1)<<2:nValueLen);
	SSapMsgAttribute *pAtti=(SSapMsgAttribute *)pDest;
	pAtti->wType=htons(wKey);
	pAtti->wLength=htons(nValueLen+sizeof(SSapMsgAttribute));
	memcpy(pAtti->acValue,pValue,nValueLen);
	memset(pAtti->acValue+nValueLen,0,nFactLen-nValueLen);
	nDestLen += (sizeof(SSapMsgAttribute)+nFactLen);
}
void CSapTLVBodyEncoder::SetValue(unsigned short wKey,const void* pValue,unsigned int nValueLen)
{
    unsigned int nFactLen=((nValueLen&0x03)!=0?((nValueLen>>2)+1)<<2:nValueLen);
    if(m_buffer.capacity()<nFactLen+4)
    {
        m_buffer.add_capacity(SAP_ALIGN(nFactLen+4-m_buffer.capacity()));
    }
    SSapMsgAttribute *pAtti=(SSapMsgAttribute *)m_buffer.top();
    pAtti->wType=htons(wKey);
    pAtti->wLength=htons(nValueLen+sizeof(SSapMsgAttribute));
    memcpy(pAtti->acValue,pValue,nValueLen);
    memset(pAtti->acValue+nValueLen,0,nFactLen-nValueLen);
    m_buffer.inc_loc(sizeof(SSapMsgAttribute)+nFactLen);
}
void CSapTLVBodyEncoder::SetValue(unsigned short wKey, const string &strValue)
{
    SetValue(wKey,strValue.c_str(),strValue.length());
}
void CSapTLVBodyEncoder::SetValue(unsigned short wKey, unsigned int wValue)
{
    int nNetValue=htonl(wValue);
    SetValue(wKey,&nNetValue,4);
}

void CSapTLVBodyEncoder::BeginValue(unsigned short wType)
{
    if(m_buffer.capacity()<sizeof(SSapMsgAttribute))
    {
        m_buffer.add_capacity(SAP_ALIGN(sizeof(SSapMsgAttribute)-m_buffer.capacity()));
    }
    SSapMsgAttribute * pAttri=(SSapMsgAttribute *)m_buffer.top();
    pAttri->wType=htons(wType);
    m_buffer.inc_loc(sizeof(SSapMsgAttribute));

    pAttriBlock=(unsigned char *)pAttri;
}
void CSapTLVBodyEncoder::AddValueBloc(const void *pData,unsigned int nLen)
{
    unsigned int nFactLen=((nLen&0x03)!=0?((nLen>>2)+1)<<2:nLen);

    if(m_buffer.capacity()<nFactLen)
    {
        m_buffer.add_capacity(SAP_ALIGN(nFactLen-m_buffer.capacity()));
    }
    memcpy(m_buffer.top(),pData,nLen);
    memset(m_buffer.top()+nLen,0,nFactLen-nLen);
    m_buffer.inc_loc(nFactLen);
}
void CSapTLVBodyEncoder::EndValue(unsigned short dwLen)
{
    if(dwLen!=0)
        ((SSapMsgAttribute *)pAttriBlock)->wLength=htons(dwLen);
    else
        ((SSapMsgAttribute *)pAttriBlock)->wLength=htons(m_buffer.top()-pAttriBlock);
}

CSapTLVBodyDecoder::CSapTLVBodyDecoder(const void * pBuffer, unsigned int nLen):
    m_pBuffer((unsigned char *)pBuffer),m_nLen(nLen)
{
    const unsigned char *ptrLoc=m_pBuffer;

    while(ptrLoc<m_pBuffer+m_nLen)
    {
        SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
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

/*Get attribute*/
int CSapTLVBodyDecoder::GetValue(unsigned short wKey,void** pValue, unsigned int * pValueLen) 
{
    AttriMultiMap::const_iterator itr=m_mapMultiAttri.find(wKey);
    if(itr==m_mapMultiAttri.end())
    {
        SS_XLOG(XLOG_DEBUG,"CSapDecodeMsg::%s,type[%d] not found\n",__FUNCTION__,wKey);
        return -1;
    }

    const unsigned char *ptrLoc=itr->second;
    SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;
    *pValueLen=ntohs(pAtti->wLength)-sizeof(SSapMsgAttribute);
    *pValue=pAtti->acValue;
    return 0;
}
int CSapTLVBodyDecoder::GetValue(unsigned short wKey,string & strValue)
{
    void *pData=NULL;
    unsigned int nLen=0;
    if(GetValue(wKey,&pData,&nLen)==-1||nLen<0)
    {
        SS_XLOG(XLOG_DEBUG,"CSapDecodeMsg::%s,type[%d],len[%d] fail!\n",__FUNCTION__,wKey,nLen);
        return -1;
    }
    strValue=string((const char *)pData,nLen);
    return 0;

}
int CSapTLVBodyDecoder::GetValue(unsigned short wKey, unsigned int * pValue) 
{
    void *pData=NULL;
    unsigned int nLen=0;
    if(GetValue(wKey,&pData,&nLen)==-1||nLen!=4)
    {
        SS_XLOG(XLOG_DEBUG,"CSapDecodeMsg::%s,type[%d],len[%d] fail!\n",__FUNCTION__,wKey,nLen);
        return -1;
    }
    *pValue=ntohl(*(int *)pData);
    return 0;
}

void CSapTLVBodyDecoder::GetValues(unsigned short wKey,vector<SSapValueNode> &vecValues)
{
    std::pair<AttriMultiMap::const_iterator, AttriMultiMap::const_iterator> itr_pair = m_mapMultiAttri.equal_range(wKey);
    AttriMultiMap::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
	    const unsigned char *ptrLoc=itr->second;
        SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;

        SSapValueNode tmp;
        tmp.nLen=ntohs(pAtti->wLength)-sizeof(SSapMsgAttribute);
        tmp.pLoc=pAtti->acValue;
		vecValues.push_back(tmp);
	}
}
void CSapTLVBodyDecoder::GetValues(unsigned short wKey,vector<string> &vecValues)
{
    std::pair<AttriMultiMap::const_iterator, AttriMultiMap::const_iterator> itr_pair = m_mapMultiAttri.equal_range(wKey);
    AttriMultiMap::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
	    const unsigned char *ptrLoc=itr->second;
        SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;

        int nLen=ntohs(pAtti->wLength)-sizeof(SSapMsgAttribute);
        if(nLen>0)
        {
            string strValue=string((const char *)pAtti->acValue,nLen);
	        vecValues.push_back(strValue);
        }
	}
}
void CSapTLVBodyDecoder::GetValues(unsigned short wKey,vector<unsigned int> &vecValues)
{
    std::pair<AttriMultiMap::const_iterator, AttriMultiMap::const_iterator> itr_pair = m_mapMultiAttri.equal_range(wKey);
    AttriMultiMap::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
	    const unsigned char *ptrLoc=itr->second;
        SSapMsgAttribute *pAtti=(SSapMsgAttribute *)ptrLoc;

        if(ntohs(pAtti->wLength)-sizeof(SSapMsgAttribute)==4)
        {
	        vecValues.push_back(ntohl(*(unsigned int *)pAtti->acValue));
        }
	}
}
}
}
