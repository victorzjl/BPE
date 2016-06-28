#ifndef _CREQUESTDECODER_H_
#define _CREQUESTDECODER_H_

#include <string>

#include "HpsCommonInner.h"
#include "HpsRequestMsg.h"

class CJsonDecoder;

typedef struct stDecodeResult
{
    std::vector<SRequestInfo> requests;
    ERequestType requestType;
    EErrorCode errorCode;
    bool isSuccessful;
    SHeaderInfo headerInfo;
    sdo::hps::CHpsRequestMsg reqMsg;

    stDecodeResult()
    {
        requestType = E_None_Request;
        isSuccessful = false;
    }
} SDecodeResult;

namespace sdo
{
    namespace hps
    {
        class CHpsRequestMsg;

        class CRequestDecoder
        {
        public:
            static CRequestDecoder* GetInstance();

            void Decode(SDecodeResult &result, char *pBuff, int nLen, const std::string & strRemoteIp,unsigned int dwRemotePort);

        protected:
            size_t GetJsonRpcBodyStartPos(CHpsRequestMsg &reqMsg);
            void DecodeBody(CHpsRequestMsg &reqMsg, const std::string & strRemoteIp, unsigned int dwRemotePort, std::string& rpcGuid, SDecodeResult &result);
            void DecodeJsonRpcBody(CHpsRequestMsg &reqMsg, size_t startPos, const std::string & strRemoteIp, unsigned int dwRemotePort, std::string& rpcGuid, SDecodeResult &result);
            
        private:
            void ParseOneRequest(CJsonDecoder& jsonCoder, SRequestInfo& sReq);
            void SetCommonRequestInfo(CHpsRequestMsg &reqMsg, SRequestInfo& sReq, const std::string & strRemoteIp,unsigned int dwRemotePort, std::string& rpcGuid);
            void SetRestRequestInfoWithKeyValue(string& strKeyValue, SRequestInfo& sReq);
            void SetRestRequestInfoWithJson(CJsonDecoder& jsonCoder, SRequestInfo& sReq);

        protected:
            CRequestDecoder();

        private:
            static CRequestDecoder* s_instance;
        };
        
    }
}

#endif //_CREQUESTDECODER_H_
