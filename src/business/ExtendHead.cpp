#include <arpa/inet.h>
#include "ExtendHead.h"
#include "SapLogHelper.h"
#ifdef WIN32
#include <objbase.h>
#else
#include <uuid/uuid.h>
#endif




void createUUID(char* pBuf, size_t len)
{
    uuid_t uu;
#ifdef WIN32
    if (S_OK == ::CoCreateGuid(&uu))
    {        
		snprintf(pBuf, len,"%X%X%X%02X%02X%02X%02X%02X%02X%02X%02X",
            uu.Data1,uu.Data2,uu.Data3,uu.Data4[0],uu.Data4[1],uu.Data4[2],uu.Data4[3],uu.Data4[4],uu.Data4[5],uu.Data4[6],uu.Data4[7]);
    }
#else
    uuid_generate( uu );
	snprintf(pBuf, len, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            uu[0], uu[1], uu[2],  uu[3],  uu[4],  uu[5],  uu[6],  uu[7],
            uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
#endif
}

void CExHeadData::getData(void* pBuffer, int nLen, const string& remoteIpAddr)
{
    ipAddr = remoteIpAddr;
    ipAddr.append(",  ");

    if (nLen > 0)
    {
        if (_pDecoderEx)
        {
            delete _pDecoderEx;
        }
        _pDecoderEx = new CSapTLVBodyDecoder(pBuffer, nLen);
        _pDecoderEx->GetValue(SOC_NAME, strName);
        _pDecoderEx->GetValue(GUID, strGuid);
        _pDecoderEx->GetValue(COMMENT, strComment);
        _pDecoderEx->GetValue(APPID, reinterpret_cast<unsigned*>(&appId));
        _pDecoderEx->GetValue(AREAID, reinterpret_cast<unsigned*>(&areaId));

        void* pData   = NULL;
        unsigned dataLen = 0;

        if(_pDecoderEx->GetValue(LOGID, &pData, &dataLen) == 0 && *(char*)pData != '\0')
        {
            strLogId.assign((char*)pData, dataLen);
            memset(pData, 0, dataLen);
        }

        if(_pDecoderEx->GetValue(SOC_IP, &pData, &dataLen)==0 && dataLen==8)
        {
            char szAddr[128] = {0};
            snprintf(szAddr, sizeof(szAddr), "%s,  %d",
                inet_ntoa(*((in_addr *)pData)),ntohl(*((int *)pData+1)));
            ipAddr.append(szAddr);
        }
        else
        {
            ipAddr.append(remoteIpAddr);
        }
    }
    else
    {
        ipAddr.append(remoteIpAddr);
    }
}

void CExHeadData::getData(void* pBuffer, int nLen)
{
    if (nLen > 0)
    {
        if (_pDecoderEx)
        {
            delete _pDecoderEx;
        }
        _pDecoderEx = new CSapTLVBodyDecoder(pBuffer, nLen);
        _pDecoderEx->GetValue(SOC_NAME, strName);
        _pDecoderEx->GetValue(GUID, strGuid);
        _pDecoderEx->GetValue(COMMENT, strComment);
        _pDecoderEx->GetValue(APPID, reinterpret_cast<unsigned*>(&appId));
        _pDecoderEx->GetValue(AREAID, reinterpret_cast<unsigned*>(&areaId));

        void* pData   = NULL;
        unsigned dataLen = 0;

        if(_pDecoderEx->GetValue(LOGID, &pData, &dataLen) == 0 && *(char*)pData != '\0')
        {
            strLogId.assign((char*)pData, dataLen);
            memset(pData, 0, dataLen);
        }
    }
}

void CExHeadData::setLogMsg()
{
    char szMsg[256] = {0};
    //ip, appid,areaid,name,guid,    
    snprintf(szMsg, sizeof(szMsg), "%s,  %d,  %d,  %s,  %s",
            ipAddr.c_str(),  
            appId, 
            areaId, 
            strName.c_str(), 
            strGuid.c_str());
    _logMsg = szMsg;
}



void CEXHeadHandler::save(const void* pExHead1, int nExLen1, 
    const void* pExHead2, int nExLen2, 
    const string& remoteIpAddr)
{
    if(_buffer.capacity() < static_cast<unsigned>(nExLen1 + nExLen2))
    {
        _buffer.add_capacity(nExLen1 + nExLen2 - _buffer.capacity());
    }
    memcpy(_buffer.top(), pExHead1, nExLen1);
    _buffer.inc_loc(nExLen1);
    memcpy(_buffer.top(), pExHead2, nExLen2);
    _buffer.inc_loc(nExLen2);
    cHeadData.getData(_buffer.base(), _buffer.len(), remoteIpAddr);

    if (cHeadData.strGuid.empty())
    {
        char guid[64] = {0};
        createUUID(guid, sizeof(guid));
        cHeadData.strGuid = guid;        
        size_t len = strlen(guid) + sizeof(SSapMsgAttribute);
        append(guid, len, CExHeadData::GUID);
    }

    cHeadData.setLogMsg();
}


void CEXHeadHandler::append(const void* pExHead, size_t nExLen, int type)
{
    if (_buffer.capacity() < nExLen)
    {
        _buffer.add_capacity(nExLen - _buffer.capacity());
    }
    SSapMsgAttribute *pAtti = reinterpret_cast<SSapMsgAttribute *>(_buffer.top());
    pAtti->wType   = htons(type);
    pAtti->wLength = htons(nExLen);
    memcpy(pAtti->acValue, pExHead, nExLen-sizeof(SSapMsgAttribute));
    _buffer.inc_loc(nExLen);

    cHeadData.getData(_buffer.base(), _buffer.len());
}

void CEXHeadHandler::updateItem(int type, int value)
{
    void*    pValue = NULL;
    unsigned len   = 0;
    if (cHeadData._pDecoderEx->GetValue(type, &pValue, &len)==0)
    {
        if (len==sizeof(value) && *(int*)pValue != value)
        {
            *(int*)pValue = htonl(value);
        }
    }
    else
    {
        uint32_t itemValue = htonl(value);
        append(&itemValue, sizeof(value)+sizeof(SSapMsgAttribute), type);        
    }
    cHeadData.setLogMsg();
}

void CEXHeadHandler::updateItem(int type, const string& strValue)
{
    void*    pValue = NULL;
    unsigned len   = 0;
    if (cHeadData._pDecoderEx->GetValue(type, &pValue, &len)==0)
    {
        CSapTLVBodyEncoder encode;
        for (int i = 1; i < CExHeadData::EX_TYPE_MAX; ++i)
        {
            if (i == type)
            {
                continue;
            }
            vector<SSapValueNode> vecValues;
            cHeadData._pDecoderEx->GetValues(i, vecValues);

            vector<SSapValueNode>::const_iterator itrValues = vecValues.begin();
            for (; itrValues != vecValues.end(); ++itrValues)
            {

                encode.SetValue(i, itrValues->pLoc, itrValues->nLen);
            }
        }

        encode.SetValue(type, strValue);

        _buffer.reset_loc(0);
        if (_buffer.capacity() < (unsigned)encode.GetLen())
        {
            _buffer.add_capacity(encode.GetLen() - _buffer.capacity());
        }
        memcpy(_buffer.base(), encode.GetBuffer(), encode.GetLen());
        _buffer.inc_loc(encode.GetLen());

        cHeadData.getData(_buffer.base(), _buffer.len());
    }
    else
    {
        append(strValue.c_str(), strValue.length() + sizeof(SSapMsgAttribute), type);
    }
    cHeadData.setLogMsg();
}

SEXHeadBufAndLen CEXHeadHandler::getExhead()
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s\n", __FUNCTION__);
    SEXHeadBufAndLen sReturn;

    if (!_strSocId.empty())
    {
        memset(_bufferSocId, 0, sizeof(_bufferSocId));
        strncpy(_bufferSocId, _strSocId.c_str(), _strSocId.size() > 32 ? 32 : _strSocId.size());
        sReturn.buffer = _bufferSocId;
        sReturn.len    = 32;
        _strSocId.clear();
    }
    else
    {
        SS_XLOG(XLOG_DEBUG,"CComposeServiceObj::%s 2\n", __FUNCTION__);
        sReturn.buffer = _buffer.base();
        sReturn.len    = _buffer.len();
    }
	
	return sReturn;
}
