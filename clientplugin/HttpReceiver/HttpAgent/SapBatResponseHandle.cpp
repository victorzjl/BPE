#include "SapBatResponseHandle.h"
#include "HpsCommon.h"
#include "HpsLogHelper.h"
#include "ErrorCode.h"
#include "CommonMacro.h"

namespace sdo
{
    namespace hps
    {
        CSapBatResponseHandle::CSapBatResponseHandle()
        {
            HP_XLOG(XLOG_DEBUG, "CSapBatResponseHandle::%s\n", __FUNCTION__);
            orderNo = 0;
            m_waitNum = 0;
            requestType = E_None_Request;
        }

        void CSapBatResponseHandle::AddHandle(SHpsResponseHandle& handle)
        {
            HP_XLOG(XLOG_DEBUG, "CSapBatResponseHandle::%s\n", __FUNCTION__);
            m_responseHandles.push_back(handle);
            if (!handle.isResponseReady)
            {
                ++m_waitNum;
            }
        }

        void CSapBatResponseHandle::HandleResponse(const SRequestInfo &sReq)
        {
            HP_XLOG(XLOG_DEBUG, "CSapBatResponseHandle::%s\n", __FUNCTION__);
            if ( (sReq.nOrder != orderNo) 
                || (E_None_Request == requestType) )
            {
                return;
            }

            std::vector<SHpsResponseHandle>::iterator iter = m_responseHandles.begin();

            headerInfo.strResCharSet = sReq.strResCharSet;
            headerInfo.strContentType = sReq.strContentType;
            headerInfo.nHttpCode = sReq.nHttpCode;
            std::vector<std::string>::const_iterator iterVec = sReq.vecSetCookie.begin();
            while (iterVec != sReq.vecSetCookie.end())
            {
                headerInfo.vecSetCookie.push_back(*iterVec);
                ++iterVec;
            }
            headerInfo.strLocationAddr = sReq.strLocationAddr;
            headerInfo.strHeaders += sReq.strHeaders;
            std::multimap<std::string, std::string>::const_iterator iterMap = sReq.headers.begin();
            while (iterMap != sReq.headers.end())
            {
                headerInfo.strHeaders += iterMap->first;
                if (!iterMap->second.empty())
                {
                    headerInfo.strHeaders += ":";
                    headerInfo.strHeaders += iterMap->second;
                }
                headerInfo.strHeaders += "\r\n";
                ++iterMap;
            }

            while (iter != m_responseHandles.end())
            {
                if (sReq.sequenceNo != (*iter).sequenceNo)
                {
                    ++iter;
                    continue;
                }

                if (E_JsonRpc_Request == requestType)
                {
                    std::string id;
                    if (sReq.sequenceNo.empty())
                    {
                        id = "null";
                    }
                    else
                    {
                        id = sReq.sequenceNo;
                    }
                    
                    std::string type = "result";

                    if ( (ERROR_JSONRPC_PARSE_ERROR == sReq.nCode) 
                            || (ERROR_JSONRPC_INVAILD_REQUEST == sReq.nCode)
                            || (ERROR_JSONRPC_METHOD_NOT_FOUND == sReq.nCode)
                            || (ERROR_JSONRPC_INVAILD_PARAMS == sReq.nCode) )
                    {
                        type = "error";
                    }
                    
                    (*iter).strResponse = "{\"jsonrpc\":\"2.0\", ";
                    (*iter).strResponse += "\"id\":\"" + id + "\", "
                                         + "\"" + type + "\":" + sReq.strResponse
                                         + "}";
                }
                else
                {
                    (*iter).strResponse = sReq.strResponse;
                }

                if (!(*iter).isResponseReady)
                {
                    (*iter).isResponseReady = true;
                    --m_waitNum;
                }
                break;
            }
        }

        bool CSapBatResponseHandle::IsCompleted()
        {
            HP_XLOG(XLOG_DEBUG, "CSapBatResponseHandle::%s\n", __FUNCTION__);
            return m_waitNum == 0;
        }
        
        void CSapBatResponseHandle::Interrupt()
        {
            HP_XLOG(XLOG_DEBUG, "CSapBatResponseHandle::%s\n", __FUNCTION__);
            std::vector<SHpsResponseHandle>::iterator iter = m_responseHandles.begin();
            while (iter != m_responseHandles.end())
            {
                if (!iter->isResponseReady)
                {
                    char szErrorCodeNeg[16] = { 0 };
                    snprintf(szErrorCodeNeg, 15, "%d", ERROR_HPS_ERROR_INTERRUPT);
                    string response("{\"return_code\":");
                    response += szErrorCodeNeg;
                    response += ",\"return_message\":\"" +  CErrorCode::GetInstance()->GetErrorMessage(ERROR_HPS_ERROR_INTERRUPT) 
                               + "\"}";
                    if (E_JsonRpc_Request != requestType)
                    { 
                        iter->strResponse = response;
                    }
                    else
                    {
                        std::string id;
                        if (iter->sequenceNo.empty())
                        {
                            id = "null";
                        }
                        else
                        {
                            id = iter->sequenceNo;
                        }
                        (*iter).strResponse = "{\"jsonrpc\":\"2.0\", ";
                        (*iter).strResponse += "\"id\":\"" + id + "\", "
                                           "\"result\":" + response
                                         + "}";
                    }
                    
                    --m_waitNum;
                }
            }
        }

        std::string CSapBatResponseHandle::GenerateResponse()
        {
            HP_XLOG(XLOG_DEBUG, "CSapBatResponseHandle::%s\n", __FUNCTION__);
            std::string result = "[";

            std::vector<SHpsResponseHandle>::iterator iter = m_responseHandles.begin();

            while (iter != m_responseHandles.end())
            {
                if (E_JsonRpc_Request != requestType)
                {
                    return (*iter).strResponse;
                }

                if (iter != m_responseHandles.begin())
                {
                    result += ", ";
                }

                result += (*iter).strResponse;
                ++iter;
            }

            result += "]";

            return result;
        }
        
    }
}
