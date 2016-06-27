#include "Utility.h"
#include <arpa/inet.h>

#include "SapMessage.h"
#include "SapLogHelper.h"


const SRequestInfo GetRequestInfo(const void *pBuffer, int nLen)
{
    SS_XLOG(XLOG_TRACE,"%s buffer[%x],len[%d]\n",__FUNCTION__,pBuffer,nLen);
    CSapDecoder decoder(pBuffer,nLen);
    void * pExHeader = NULL;
    int nExHeadLen = 0;
    decoder.GetExtHeader(&pExHeader,&nExHeadLen);
    string strName;
    int appId = DEFAULT_APPID, areadId = DEFAULT_AREAID; 
    string strGuid;
    string strLogId;
    string strComment;
    if(nExHeadLen > 0)
    {
        CSapTLVBodyDecoder decoderEx(pExHeader,nExHeadLen);        
        decoderEx.GetValue(1, strName);
        decoderEx.GetValue(9, strGuid);
        decoderEx.GetValue(11, strComment);
        void *pData = NULL;
        unsigned int nLen = 0;
        if(decoderEx.GetValue(3,&pData,&nLen)==0 && nLen ==4)
        {
           appId = ntohl(*(int *)pData);
        }
        if(decoderEx.GetValue(4,&pData,&nLen)==0 && nLen ==4)
        {
           areadId = ntohl(*(int *)pData);
        }
        if(decoderEx.GetValue(10, &pData, &nLen) == 0)
        {
            strLogId.assign((char*)pData, nLen);
            memset(pData, 0, nLen);
        }
    }
    return SRequestInfo(appId,areadId,strName, strGuid, strLogId, strComment);
}

const string GetRemoteIpAddr(const void* pExBuffer, int nExLen)
{    
    string strRemoteIp("0.0.0.0,  0");
    if (pExBuffer != NULL)
    {   
        CSapTLVBodyDecoder decoderEx(pExBuffer,nExLen);
        void* pValue;
        unsigned int nValueLen = 0;
        if(decoderEx.GetValue(2, &pValue, &nValueLen)==0 && nValueLen==8)
        {
            char szAddr[128] = {0};
            snprintf(szAddr,sizeof(szAddr)-1,"%s,  %d",
                inet_ntoa(*((in_addr *)pValue)),ntohl(*((int *)pValue+1)));            
            strRemoteIp = szAddr;
        }
    }

    return strRemoteIp;
}

const string GetIpAddr(const void * pBuffer, int nLen, const void* pExBuffer, int nExLen)
{    
    string strRemoteIp = GetRemoteIpAddr(pExBuffer, nExLen);
    string strClientIp = strRemoteIp;
    CSapDecoder decoder(pBuffer,nLen);
    void * pExHeader = NULL;
    int nExHeadLen = 0;
    decoder.GetExtHeader(&pExHeader,&nExHeadLen);
    if(nExHeadLen > 0)
    {
        CSapTLVBodyDecoder decoderEx(pExHeader,nExHeadLen);
        void* pValue;
        unsigned int nValueLen = 0;
        if(decoderEx.GetValue(2,&pValue, &nValueLen)==0 && nValueLen==8)
        {
            char szAddr[128] = {0};
			snprintf(szAddr,sizeof(szAddr)-1,"%s,  %d",
			      inet_ntoa(*((in_addr *)pValue)),ntohl(*((int *)pValue+1)));
            //SS_XLOG(XLOG_DEBUG,"%s addr[%s]\n", __FUNCTION__, szAddr);  
            strClientIp = szAddr;            
        }         
    }    
    return strRemoteIp + ",  " +strClientIp;    
}


const string TimeToString(const struct tm& tmRecord, unsigned ms)
{
	char szBuf[512]={0};
	snprintf(szBuf,sizeof(szBuf)-1,"%04u-%02u-%02u %02u:%02u:%02u.%03u",
    tmRecord.tm_year+1900,tmRecord.tm_mon+1,tmRecord.tm_mday,
    tmRecord.tm_hour,tmRecord.tm_min,tmRecord.tm_sec,ms);
	return string(szBuf);
}


