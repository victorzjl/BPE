#ifndef _CSAPBATRESPONSEHANDLE_H_
#define _CSAPBATRESPONSEHANDLE_H_

#include <vector>
#include <string>

#include "HpsCommonInner.h"

namespace sdo
{
    namespace hps
    {
        typedef struct stHpsResponseHandle
        {
            int nOrder;
            string strResponse;
            int nCode;
            bool isResponseReady;  //用于队列中的响应数据是否就绪进行判断
            int nHttpVersion; //0-1.0; 1-1.1
            bool isKeepAlive;	
            string strResCharSet;
            string strContentType;
            int nHttpCode;
            vector<string> vecSetCookie;
            string strLocationAddr;
            string strHeaders;
            string sequenceNo;
        }SHpsResponseHandle;


        class CSapBatResponseHandle
        {
        public:
            CSapBatResponseHandle();

            void AddHandle(SHpsResponseHandle& handle);
            void HandleResponse(const SRequestInfo &sReq);
            bool IsCompleted();
            void Interrupt();
            std::string GenerateResponse();

        public:
            int orderNo;  //批次号
            ERequestType requestType;
            SHeaderInfo headerInfo;

        private:
            std::vector<SHpsResponseHandle> m_responseHandles;
            unsigned int m_waitNum;
        };
    }
}

#endif //_CSAPBATRESPONSEHANDLE_H_
