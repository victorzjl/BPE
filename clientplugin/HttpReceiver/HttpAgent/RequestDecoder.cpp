#include <boost/algorithm/string.hpp>
#include <cstdio>

#include "HpsLogHelper.h"
#include "RequestDecoder.h"
#include "HpsRequestMsg.h"
#include "HpsCommonFunc.h"
#include "CJson.h"
#include "UrlCode.h"

static char* g_pszOsapUser = "osap_user";
static char* g_pszMerchantName = "merchant_name";
static char* g_pszClientId = "client_id";

#define DO_NOTHING 0

namespace sdo
{
    namespace hps
    {
        
        CRequestDecoder* CRequestDecoder::s_instance = NULL;

        CRequestDecoder* CRequestDecoder::GetInstance()
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            if (NULL == s_instance)
            {
                    s_instance = new CRequestDecoder;
            }

            return s_instance;
        }

        CRequestDecoder::CRequestDecoder()
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
        }

        void CRequestDecoder::Decode(SDecodeResult &result, char *pBuff, int nLen, const std::string & strRemoteIp, unsigned int dwRemotePort)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            string strOriInput(pBuff, nLen);
            int nResult = result.reqMsg.Decode(pBuff, nLen);
            if(nResult != 0)
            {
                HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, Message Decode error result[%d]\n",__FUNCTION__, nResult);
                result.errorCode = ERROR_DEOCDE_REQUEST_ERROR;
                result.isSuccessful = false;
                return;
            } 

            result.headerInfo.isKeepAlive = result.reqMsg.IsKeepAlive();
            result.headerInfo.nHttpVersion = result.reqMsg.GetHttpVersion();

            size_t pos = GetJsonRpcBodyStartPos(result.reqMsg);
            std::string rpcGuid = getguid();
            if (string::npos != pos)
            {
                result.requestType = E_JsonRpc_Request;
                DecodeJsonRpcBody(result.reqMsg, pos, strRemoteIp, dwRemotePort, rpcGuid, result);
            }
            else
            {
                result.requestType = E_Normal_Request;
                DecodeBody(result.reqMsg, strRemoteIp, dwRemotePort, rpcGuid, result);
            }
            
            for (vector<SRequestInfo>::iterator iter = result.requests.begin();
                 iter!=result.requests.end(); ++iter)
            {
                iter->strOriInput = strOriInput;
            }
        }

        size_t CRequestDecoder::GetJsonRpcBodyStartPos(CHpsRequestMsg& reqMsg)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            size_t pos = reqMsg.GetBody().find_first_of('[');
            if (pos == string::npos)
            {
                return string::npos;
            }

            std::string body = URLDecoder::decode(reqMsg.GetBody().substr(0, pos).c_str());
            std::string bodycopy;
            size_t spacepos1 = body.find_first_not_of(' ');
            size_t spacepos2;
            
            while (spacepos1 != string::npos)
            {
                spacepos2 = body.find_first_of(' ', spacepos1);
                if (spacepos2 != string::npos)
                {
                    bodycopy += body.substr(spacepos1, spacepos2-spacepos1);
                }
                else
                {
                    bodycopy += body.substr(spacepos1);
                    break;
                }
                
                spacepos1 = body.find_first_not_of(' ', spacepos2);
            }
            
            if ( (!bodycopy.empty())
                    && (bodycopy != "data=") )
            {
                return string::npos;
            }
            
            return pos;
        }

        void CRequestDecoder::DecodeBody(CHpsRequestMsg &reqMsg, const std::string & strRemoteIp, 
                                           unsigned int dwRemotePort, std::string& rpcGuid, SDecodeResult &result)
        {
           HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
           
            SRequestInfo sReq;
            SetCommonRequestInfo(reqMsg, sReq, strRemoteIp, dwRemotePort, rpcGuid);
            SetRestRequestInfoWithKeyValue(sReq.strUriAttribute, sReq);
            sReq.headers = reqMsg.GetHeaders();
            sReq.strUserAgent = reqMsg.GetUserAgent();
            sReq.requestType = E_Normal_Request;
            result.requests.push_back(sReq);
            result.isSuccessful = true;
        }
        
        void CRequestDecoder::DecodeJsonRpcBody(CHpsRequestMsg &reqMsg, size_t startPos, const string & strRemoteIp, 
                                                  unsigned int dwRemotePort, std::string& rpcGuid, SDecodeResult &result)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            std::string body = reqMsg.GetBody();
            if (!reqMsg.IsPost())
            {
                body = body.substr(startPos);
            }
            
            body = URLDecoder::decode(body.c_str());
            
            std::vector<CJsonDecoder> vecDecoder;
            int len = body.length();
            CJsonDecoder jsonDecoder(body.c_str(), len);
            
            if (-1 == jsonDecoder.GetValue(vecDecoder))
            {
                result.isSuccessful = false;
                result.errorCode = ERROR_JSONRPC_PARSE_ERROR;
                return;
            }
            
            std::map<string, int> sequenceNoMap;
            std::map<string, int>::iterator map_iter;
            
            for (std::vector<CJsonDecoder>::iterator iter = vecDecoder.begin();
                    iter != vecDecoder.end(); ++iter)
            {
                SRequestInfo sReq;
                SetCommonRequestInfo(reqMsg, sReq, strRemoteIp, dwRemotePort, rpcGuid);
                ParseOneRequest(*iter, sReq);
                sReq.headers = reqMsg.GetHeaders();
                sReq.strUserAgent = reqMsg.GetUserAgent();
                sReq.strContentType = reqMsg.GetContentType();
                //check for same identifier
                map_iter = sequenceNoMap.find(sReq.sequenceNo);
                if (map_iter != sequenceNoMap.end())
                {
                    result.isSuccessful = false;
                    result.errorCode = ERROR_JSONRPC_ERROR_SAMEIDENTIFIER;
                    return;
                }
                else
                {
                    sequenceNoMap[sReq.sequenceNo] = 1;
                }
                sReq.requestType = E_JsonRpc_Request;
                result.requests.push_back(sReq);
            }
            jsonDecoder.DestroyJsonDecoder();
            
            result.isSuccessful = true;
        }
        
        void CRequestDecoder::ParseOneRequest(CJsonDecoder& jsonCoder, SRequestInfo& sReq)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            std::string value, method, params;
            if ( (-1 == jsonCoder.GetValue("jsonrpc", value)) 
                || (value != "2.0") )
            {
                HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, can't find field 'jsonrpc'\n", __FUNCTION__);
                sReq.isVaild = false;
                sReq.nCode = ERROR_JSONRPC_INVAILD_REQUEST;
                return;
            }
            
            if ( (-1 == jsonCoder.GetValue("id", value)) 
                || (value.empty()) )
            {
                HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, can't find field 'id'\n", __FUNCTION__);
                sReq.isVaild = false;
                sReq.nCode = ERROR_JSONRPC_INVAILD_REQUEST;
                return;
            }
            
            sReq.sequenceNo = value;
            
            if ( (-1 == jsonCoder.GetValue("method", method)) 
                || (method.empty()) )
            {
                HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, can't find field 'method'\n", __FUNCTION__);
                sReq.isVaild = false;
                sReq.sequenceNo = "";
                sReq.nCode = ERROR_JSONRPC_INVAILD_REQUEST;
                return;
            }
            
            sReq.strUriCommand = method;
            
            EJsonValueType valueType;
            if (-1 == jsonCoder.GetValueType("params", valueType))
            {
                HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, can't find field 'params'\n", __FUNCTION__);
                sReq.isVaild = false;
                sReq.sequenceNo = "";
                sReq.nCode = ERROR_JSONRPC_INVAILD_REQUEST;
                return;
            }
            else
            {
                if (JSONTYPE_STRING == valueType)
                {
                    if (-1 == jsonCoder.GetValue("params", params))
                    {
                        HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, get string 'params' error\n", __FUNCTION__);
                        sReq.isVaild = false;
                        sReq.nCode = ERROR_JSONRPC_INVAILD_REQUEST;
                        return;
                    }
                    sReq.strUriAttribute = params;
                    SetRestRequestInfoWithKeyValue(params, sReq);
                }
                else if (JSONTYPE_OBJECT == valueType)
                {
                    CJsonDecoder tempJsonDecoder;
                    if ( -1 == jsonCoder.GetValue("params", tempJsonDecoder) )
                    {
                        HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, get object 'params' error\n", __FUNCTION__);
                        sReq.isVaild = false;
                        sReq.nCode = ERROR_JSONRPC_INVAILD_REQUEST;
                        return;
                    }
                    sReq.strUriAttribute = "";
                    json_object* jroot = json_tokener_parse(tempJsonDecoder.ToJsonString());
                    if (!is_error(jroot))
                    {
                        json_object_object_foreach(jroot, key, val)
                        {
                            if (!sReq.strUriAttribute.empty())
                            {
                               sReq.strUriAttribute += "&";
                            }
                            sReq.strUriAttribute += key;
                            sReq.strUriAttribute += "=";
                            sReq.strUriAttribute += json_object_get_string(val);
                        }
                        json_object_put(jroot);
                    }
                    HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, params[%s]\n", __FUNCTION__, sReq.strUriAttribute.c_str());
                    SetRestRequestInfoWithJson(tempJsonDecoder, sReq);
                }
                else
                {
                    HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s, unsupported type of 'params'\n", __FUNCTION__);
                    sReq.isVaild = false;
                    sReq.sequenceNo = "";
                    sReq.nCode = ERROR_JSONRPC_INVAILD_PARAMS;
                    return;
                }
            }
            
            sReq.isVaild = true;
        }
        
        void CRequestDecoder::SetCommonRequestInfo(CHpsRequestMsg &reqMsg, SRequestInfo& sReq, const std::string & strRemoteIp,unsigned int dwRemotePort, std::string& rpcGuid)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            sReq.strUriCommand = reqMsg.GetCommand();	
            sReq.strUriAttribute = reqMsg.GetBody();
            sReq.strCookie = reqMsg.GetCookie();
            sReq.strHost = reqMsg.GetHost();
            sReq.isKeepAlive = reqMsg.IsKeepAlive();
            sReq.m_nHttpVersion = reqMsg.GetHttpVersion();
            sReq.strContentType = reqMsg.GetContentType();
            if(reqMsg.IsXForwardedFor())
            {
                sReq.strRemoteIp = reqMsg.GetXForwardedFor();
                sReq.dwRemotePort = reqMsg.GetXFFPort();
            }
            else
            {
                sReq.strRemoteIp = strRemoteIp;
                sReq.dwRemotePort = dwRemotePort;
            }
            
            sReq.strGuid = getguid();
            sReq.strRpcGuid = rpcGuid;
        }
        
        void CRequestDecoder::SetRestRequestInfoWithKeyValue(string& strKeyValue, SRequestInfo& sReq)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            gettimeofday_a(&(sReq.tStart),0);

            sReq.strFnCallback = sdo::hps::CHpsRequestMsg::GetValueByKey(strKeyValue, "fn_callback");

            sReq.strUserName = sdo::hps::CHpsRequestMsg::GetValueByKey(strKeyValue, g_pszMerchantName);
            if (sReq.strUserName.empty())
            {
                sReq.strUserName = sdo::hps::CHpsRequestMsg::GetValueByKey(strKeyValue, g_pszClientId);
                if (sReq.strUserName.empty())
                {
                    sReq.strUserName = sdo::hps::CHpsRequestMsg::GetValueByKey(strKeyValue, g_pszOsapUser);
                }
            }

            string strTimestamp = sdo::hps::CHpsRequestMsg::GetValueByKey(strKeyValue, "timestamp");
            if (!strTimestamp.empty())
            {
                sReq.nTimestamp = sReq.tStart.tv_sec + sReq.tStart.tv_usec/1000000
                                    - atoi(strTimestamp.c_str());
            }
            sReq.strComment = sdo::hps::CHpsRequestMsg::GetValueByKey(strKeyValue, "comment");
            sReq.isVaild = true;
        }
        
        void CRequestDecoder::SetRestRequestInfoWithJson(CJsonDecoder& jsonCoder, SRequestInfo& sReq)
        {
            HP_XLOG(XLOG_DEBUG,"CRequestDecoder::%s\n", __FUNCTION__);
            gettimeofday_a(&(sReq.tStart),0);
            
            jsonCoder.GetValue("fn_callback", sReq.strFnCallback);
            jsonCoder.GetValue(g_pszMerchantName, sReq.strUserName);
            
            if (sReq.strUserName.empty())
            {
                jsonCoder.GetValue(g_pszClientId, sReq.strUserName);
                if (sReq.strUserName.empty())
                {
                    jsonCoder.GetValue(g_pszOsapUser, sReq.strUserName);
                }
            }

            string strTimestamp("");
            jsonCoder.GetValue("timestamp", strTimestamp);
            if (!strTimestamp.empty())
            {
                sReq.nTimestamp = sReq.tStart.tv_sec + sReq.tStart.tv_usec/1000000
                                    - atoi(strTimestamp.c_str());
            }
            
            jsonCoder.GetValue("comment", sReq.strComment);
            sReq.isVaild = true;
        }
    }

}
