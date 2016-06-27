#ifndef __EXTEND_HEAD_H__
#define __EXTEND_HEAD_H__
#include <string>
#include <boost/shared_ptr.hpp>
#include "SapMessage.h"
//#include "Utility.h"
using std::string;

/*
const string getRemoteIp(void* pBuffer, int nLen);
const string getLogMsg(const void* pBuffer, int nLen, const string& remoteIpAddr, string& ipInfo);
*/
extern const int DEFAULT_APPID;
extern const int DEFAULT_AREAID;
class CExHeadData
{
    enum EEXHEAD
    {
        SOC_NAME    = 1,
        SOC_IP      = 2,
        APPID       = 3,
        AREAID      = 4,
        GUID        = 9,
        LOGID       = 10,
        COMMENT     = 11,
		SERVICE_ID  = 13,
		MESSAGE_ID  = 14,
        EX_TYPE_MAX = 15,
    };

public:
    CExHeadData():appId(DEFAULT_APPID),areaId(DEFAULT_AREAID),serviceId(DEFAULT_APPID),messageId(DEFAULT_AREAID), _pDecoderEx(NULL)
		{}
    ~CExHeadData(){if (_pDecoderEx) delete _pDecoderEx;}
public:
    void getData(void* pBuffer, int nLen);
    void getData(void* pBuffer, int nLen, const string& remoteIpAddr);
    inline void setLogMsg();    
    const string& getIp() const {return ipAddr;}
    const string& getGuid() const {return strGuid;}
    const string& getComment() const {return strComment;}
    const string& getLogId() const {return strLogId;}
    void setGuid(const char* guid){strGuid = guid;}
    friend class CEXHeadHandler;
    
private: 
    int appId;
	int areaId;
	int serviceId;
	int messageId;
	string strName;
    string strGuid;
    string strComment;
    string strLogId;
    string ipAddr;
    string _logMsg;
    CSapTLVBodyDecoder* _pDecoderEx;
};

typedef struct stEXHeadBufAndLen
{
    const void * buffer;
    int          len;
}SEXHeadBufAndLen;

class CEXHeadHandler
{
public:
    CEXHeadHandler():_buffer(512){}
    //~CEXHeadHandler();
public:
    void save(const void* pExHead1, int nExLen1, const void* pExHead2, int nExLen2, const string& remoteIpAddr);
    // const void * GetBuffer()const{return _buffer.base();}
    // int GetLen()const{return _buffer.len();}
    SEXHeadBufAndLen getExhead();
    const char* getGuid()const{return cHeadData.strGuid.c_str();}
    const string & getLogMsg()const{return cHeadData._logMsg;}
    const string & getComment() const {return cHeadData.strComment;}
    const string & getLogId() const {return cHeadData.strLogId;}
    const string& getIp()const{return cHeadData.ipAddr;}
    void updateAppId(int appid)
    {
        cHeadData.appId = appid;
        updateItem(CExHeadData::APPID, appid); 
    }

    void updateAreaId(int areaid)
    {
        cHeadData.areaId = areaid; 
        updateItem(CExHeadData::AREAID, areaid);
    }    
	
	void updateServiceId(int serviceId)
    {
        cHeadData.serviceId = serviceId;
        updateItem(CExHeadData::SERVICE_ID, serviceId); 
    }

    void updateMessageId(int messageId)
    {
        cHeadData.messageId = messageId; 
        updateItem(CExHeadData::MESSAGE_ID, messageId);
    }  
	
    void updateName(const string& strName)
    {
        cHeadData.strName = strName; 
        updateItem(CExHeadData::SOC_NAME, strName);
    }

    void updateSocId(const string& strSocId)
    {
        _strSocId = strSocId;
    }
private:
    void append(const void* pExHead, size_t nExLen, int type);
    void updateItem(int type, int value);
    void updateItem(int type, const string& strValue);
private:
    /*int _loc;
    char _szExtra[32];*/
    CSapBuffer _buffer;
    CExHeadData cHeadData;
    string      _strSocId;
    char        _bufferSocId[33];
};
typedef boost::shared_ptr<CEXHeadHandler> EXHeadHandler_ptr;

#endif
