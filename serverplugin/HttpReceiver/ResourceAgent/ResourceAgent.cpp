#include "ResourceAgent.h"
#include "SessionManager.h"
#include "ResourceAgentLogHelper.h"
#include "CommonMacro.h"
#include "CUrlSingature.h"
#include "ContextServerFactory.h"
#include <boost/algorithm/string.hpp>

static const SRecordSet EMPTY_RECORD_SET;

typedef map<unsigned int, SRecordSet>::iterator TRecordMapIter;
typedef map<unsigned int, SRecordSet>::const_iterator CTRecordMapIter;

typedef vector<SDynamicRequestInfo>::iterator TDynReqInfoIter;
typedef vector<SDynamicRequestInfo>::const_iterator CTDynReqInfoIter;

CResourceAgent::CResourceAgent(CHpsSessionManager *pManager) 
    : m_pManager(pManager),
      m_dwRemoteReqId(0)
{
    m_vecAgentConfig = CHttpRecConfig::GetInstance()->GetAgentConfig();
}

CResourceAgent::~CResourceAgent() 
{
    m_mapRecordSet.clear();
    m_mapInnerSeqAndConn.clear();
    m_mapInnerIdAndSeq.clear();
    m_mapRemoteSeqAndConn.clear();
    m_mapRemoteIdAndSeq.clear();
}

int CResourceAgent::OnResourceRequest(SRequestInfo &sReq, bool &bIsFind, bool &bIsLoad, string &strContentType)
{
	RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Sequence[%d], Path[%s]\n", __FUNCTION__,
		                 sReq.dwSequence, sReq.strUrlIdentify.c_str());
    string strHostHeader; 
    string strAddr;
    unsigned int dwPort;
    if (SearchAgentConfig(sReq.strUrlIdentify, strHostHeader, strAddr, dwPort))
	{
		RA_XLOG(XLOG_WARNING, "CResourceAgent::%s, This is a need-to-transfer request.\n", __FUNCTION__);
        return StartHandleAgentRequest(sReq, strHostHeader, strAddr, dwPort, bIsFind, bIsLoad);
	}
	else
    {
        RA_XLOG(XLOG_WARNING, "CResourceAgent::%s, This is a local request.\n", __FUNCTION__);
        string strDomain;
        GetDomainFromHostHeader(sReq.strHost, strDomain);
        SResponseExtension sResExt;
        ContextServerPtr pContextServer = CContextServerFactory::GetInstance()->GetContextServer(strDomain, sReq.strUrlIdentify);
        if (pContextServer->OnRequest(sReq, sResExt))
        {
            RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s File [%s] is found.\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
            sReq.strResourceAbsPath = sResExt._strAbsFilePath;
            if (sResExt._vecDynReqInfo.empty())
            {
                return StartHandleStaticResource(sReq, bIsFind, bIsLoad, strContentType, sResExt._strFileExtension, sResExt._bIsImage);
            }
            else
            {
                return StartHandleDynamicResource(sReq, bIsFind, bIsLoad, sResExt._vecDynReqInfo, pContextServer);
            }
        }
        else
        {
            RA_XLOG(XLOG_WARNING, "CResourceAgent::%s File [%s] is not exist.\n", __FUNCTION__, sReq.strUrlIdentify.c_str());
            sReq.strErrorReason = "File is not exist";
            bIsFind = false;
        }
    }

    return 0;
}

int CResourceAgent::OnPeerResponse(unsigned int dwId, const string& strResponse)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s ID[%d] Response[%s]\n", __FUNCTION__, dwId, strResponse.c_str());
    m_pManager->GetIoService().post(boost::bind(&CResourceAgent::DoPeerResponse, this, dwId, strResponse));
    return 0;
}

int CResourceAgent::OnRemoteResponse(unsigned int dwId, const string &strResponse)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, dwId[%d], Response[%s]\n", __FUNCTION__, dwId, strResponse.c_str());
    m_pManager->GetIoService().post(boost::bind(&CResourceAgent::DoRemoteResponse, this, dwId, strResponse));
    return 0;
}

int CResourceAgent::OnPeerClose(unsigned int dwId)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, dwId[%d]\n", __FUNCTION__, dwId);
    m_pManager->GetIoService().post(boost::bind(&CResourceAgent::DoPeerClose, this, dwId));
    return 0;
}

int CResourceAgent::SearchAgentConfig(const string &strUrlPath, string &strHostHeader, string &strAddr, unsigned int &dwPort)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, UrlPath[%s]\n", __FUNCTION__, strUrlPath.c_str());

    bool bFind = false;
    unsigned int nChoosenLength = 0;
    vector<SAgentConfig>::iterator iterChoosen = m_vecAgentConfig.end();
    vector<SAgentConfig>::iterator iter = m_vecAgentConfig.begin();
    while (iter != m_vecAgentConfig.end())
    {
        if (strUrlPath.empty() && iter->_strPath.empty())
        {
            iterChoosen = iter;
            break;
        }
        else
        {
            if (boost::istarts_with(strUrlPath, iter->_strPath)
                && (nChoosenLength < iter->_strPath.length()))
            {
                iterChoosen = iter;
                nChoosenLength = iter->_strPath.length();
            }
        }

        ++iter;
    }

    if (iterChoosen != m_vecAgentConfig.end())
    {
        strHostHeader = "Host:";
        strHostHeader += iterChoosen->_strHostHeaderValue;
        strHostHeader += "\r\n";
        if (iterChoosen->_vecTargetIp.empty())
        {
            strAddr = iterChoosen->_sTargetDomain._strAddr;
            dwPort = iterChoosen->_sTargetDomain._dwPort;
        }
        else
        {
            strAddr = iterChoosen->_vecTargetIp[iterChoosen->_dwNextIpIndex]._strAddr;
            dwPort = iterChoosen->_vecTargetIp[iterChoosen->_dwNextIpIndex]._dwPort;
            iterChoosen->_dwNextIpIndex = (iterChoosen->_dwNextIpIndex + 1) % (iterChoosen->_vecTargetIp.size());
        }
        bFind = true;
    }

    return bFind;
}

int CResourceAgent::StartHandleDynamicResource(SRequestInfo &sReq, bool &bIsFind, bool &bIsLoad, 
    const vector<SDynamicRequestInfo>& vecDynReqInfo, const ContextServerPtr &pContextServer)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s\n", __FUNCTION__);
    std::pair<TRecordMapIter, bool> insertRet = m_mapRecordSet.insert(std::make_pair(sReq.dwSequence, EMPTY_RECORD_SET));

    if (!insertRet.second)
    {
        bIsFind = false;
        RA_XLOG(XLOG_WARNING, "CResourceAgent::%s Can't record dynamic requests. Sequence[%d], Path[%s]\n", __FUNCTION__,
                              sReq.dwSequence, sReq.strUrlIdentify.c_str());
        sReq.strErrorReason = "Can't record dynamic requests";
        return -1;
    }

    bIsFind = true;
    bIsLoad = false;

    SRecordSet &sRecordSet = insertRet.first->second;
    sRecordSet._vecDynReqInfo = vecDynReqInfo;
    ParseRequestParams(sReq.strUriAttribute, sRecordSet._mapRequestParams);
    ConstructHttpRequestHeaders(sReq, sRecordSet._strHeader);
    sRecordSet._sReqInfo = sReq;
    TDynReqInfoIter iter = sRecordSet._vecDynReqInfo.begin();
    const SAccessConfig &sAccessConfig = CHttpRecConfig::GetInstance()->GetAccessConfig(iter->_sUrlInfo._strHost);
    if (sAccessConfig._bIsVaild)
    {
        FillHttpRequestBufferWithSignature(*iter, sRecordSet, sAccessConfig._strMerchantName, sAccessConfig._strPrivateKey);
    }
    else
    {
        FillHttpRequestBuffer(*iter, sRecordSet);
    }
    SendServiceRequest(sReq.dwSequence, iter->_sUrlInfo._bIsRemote, iter->_sUrlInfo._strHost, iter->_sUrlInfo._dwPort);
    sRecordSet._pContextServer = pContextServer;
    sRecordSet._sReqInfo.bIsTransfer = false;
    return 0;
}

int CResourceAgent::StartHandleStaticResource(SRequestInfo &sReq, bool &bIsFind, bool &bIsLoad,
                               string &strContentType, const string &strExtension, bool bIsImage)
{
    bIsFind = true;
    bIsLoad = true;
    if (bIsImage)
    {
        strContentType = "image/";
    }
    else
    {
        strContentType = "text/";
    }
    strContentType += strExtension;

    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s File [%s] is found. ContentType[%s]\n", __FUNCTION__, sReq.strUrlIdentify.c_str(), strContentType.c_str());
    return 0;
}

int CResourceAgent::StartHandleAgentRequest(SRequestInfo &sReq, const string &strHostHeader, const string &strAddr, 
                                            unsigned int dwPort, bool &bIsFind, bool &bIsLoad)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s\n", __FUNCTION__);
    std::pair<TRecordMapIter, bool> insertRet = m_mapRecordSet.insert(std::make_pair(sReq.dwSequence, EMPTY_RECORD_SET));

    if (!insertRet.second)
    {
        bIsFind = false;
        bIsLoad = false;
        RA_XLOG(XLOG_WARNING, "CResourceAgent::%s Can't record dynamic requests. Sequence[%d], Path[%s]\n", __FUNCTION__,
            sReq.dwSequence, sReq.strUrlIdentify.c_str());
        sReq.strErrorReason = "Can't record dynamic requests";
        return -1;
    }

    bIsFind = true;
    bIsLoad = false;

    int nOriInputLen = sReq.strOriInput.length();
    if ((unsigned int)nOriInputLen > m_buffer.len())
    {
        m_buffer.add_capacity(nOriInputLen);
    }

    char *pWritePos = m_buffer.base();
    const char *pOriBegin = sReq.strOriInput.c_str();
    const char *pBegin = pOriBegin;
    const char *pEnd = strstr(pBegin, "\r\n");
    while (pEnd != NULL && pBegin - pOriBegin < nOriInputLen && (pEnd + 2 - pOriBegin)<nOriInputLen)
    {
        unsigned int nWroten = 0;
        if (pBegin == pOriBegin)
        {
            nWroten = pEnd - pBegin - 3;
            memcpy(pWritePos, pBegin, nWroten);
            memcpy(pWritePos + nWroten, "1.0\r\n", 5);
            nWroten += 5;
        }
        else if (strncasecmp(pBegin, "connection", 10) == 0)
        {
            nWroten = 18;
            memcpy(pWritePos, "Connection:Close\r\n", 18);
        }
        else if (strncasecmp(pBegin, "host", 4) == 0)
        {
            nWroten = strHostHeader.length();
            memcpy(pWritePos, strHostHeader.c_str(), nWroten);
        }
        else
        {
            nWroten = pEnd - pBegin + 2;
            memcpy(pWritePos, pBegin, nWroten);
        }

        m_buffer.inc_loc(nWroten);
        pWritePos += nWroten;

        pBegin = pEnd + 2;
        pEnd = strstr(pBegin, "\r\n");
    }
    memcpy(pWritePos, "\r\n", 2);
    m_buffer.inc_loc(2);
    m_buffer.SetTopZero();

    SendServiceRequest(sReq.dwSequence, true, strAddr, dwPort);

    insertRet.first->second._dwSeq = sReq.dwSequence;
    insertRet.first->second._sReqInfo = sReq;
    insertRet.first->second._sReqInfo.bIsTransfer = true;
    return 0;
}

void CResourceAgent::DoResponse(unsigned int dwSeq, const string& strResponse, const string &strResponseLine, const string &strHeaders)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, dwSeq[%d], Response[%s]\n", __FUNCTION__, dwSeq,
                            strResponse.c_str(), strResponseLine.c_str(), strHeaders.c_str());
    TRecordMapIter iterRecordSet = m_mapRecordSet.find(dwSeq);
    if (iterRecordSet != m_mapRecordSet.end())
    {
        bool bIsFinished = false;

        if (iterRecordSet->second._sReqInfo.bIsTransfer)
        {
            bIsFinished = HandleAgentResponse(iterRecordSet->second, dwSeq, strResponse, strResponseLine, strHeaders);
            if (bIsFinished)
            {
                FinishAgentRequest(iterRecordSet->second);
            }
        }
        else
        {
            bIsFinished = HandleDynamicRemoteResponse(iterRecordSet->second, dwSeq, strResponse);
            if (bIsFinished)
            {
                FinishResourceRequest(iterRecordSet->second);
            }
        }

        if (bIsFinished)
        {
            m_mapRecordSet.erase(iterRecordSet);
        }
    }
    else
    {
        RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, Can't find record for sequence[%u]\n", __FUNCTION__, dwSeq);
    }
}

void CResourceAgent::DoPeerResponse(unsigned int dwId, const string& strResponse)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, ConnId[%d], Response[%s]\n", __FUNCTION__, dwId, strResponse.c_str());
    map<unsigned int, unsigned int>::iterator iterSeq = m_mapInnerIdAndSeq.find(dwId);
    if (iterSeq != m_mapInnerIdAndSeq.end())
    {
        unsigned int dwSeq = iterSeq->second;
        map<unsigned int, HpsConnection_ptr>::iterator iterConn = m_mapInnerSeqAndConn.find(dwSeq);
        iterConn->second->OnVirtalClientClosed();
        m_mapInnerIdAndSeq.erase(iterSeq);
        m_mapInnerSeqAndConn.erase(iterConn);

        DoResponse(dwSeq, strResponse, "", "");
    }
}

void CResourceAgent::DoRemoteResponse(unsigned int dwId, const string& strResponse)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, ConnId[%d], Response[%s]\n", __FUNCTION__, dwId, strResponse.c_str());

    string strResponseData, strHeaders, strResponseLine;
    size_t nPos1 = strResponse.find("\r\n");
    if (nPos1 != string::npos)
    {
        strResponseLine.assign(strResponse.begin(), strResponse.begin() + nPos1);
        size_t nPos2 = strResponse.find("\r\n\r\n");
        if (nPos2 != string::npos)
        {
            strHeaders.assign(strResponse.begin() + nPos1 + 2, strResponse.begin() + nPos2 + 2);
            string strResponseDataRaw = strResponse.substr(nPos2 + 4);
            boost::iterator_range<string::const_iterator> irBraceLeft = boost::ifind_first(strResponseDataRaw, "{");
            boost::iterator_range<string::const_iterator> irBraceRight = boost::ifind_last(strResponseDataRaw, "}");
            if (!irBraceLeft.empty() && !irBraceRight.empty())
            {
                strResponseData.assign(irBraceLeft.begin(), irBraceRight.begin());
                strResponseData += "}";
            }
        }
    }
    

    map<unsigned int, unsigned int>::iterator iterSeq = m_mapRemoteIdAndSeq.find(dwId);
    if (iterSeq != m_mapRemoteIdAndSeq.end())
    {
        unsigned int dwSeq = iterSeq->second;
        m_mapRemoteSeqAndConn.erase(dwSeq);
        m_mapRemoteIdAndSeq.erase(iterSeq);
        DoResponse(dwSeq, strResponseData, strHeaders, strResponseLine);
    }
}

void CResourceAgent::DoPeerClose(unsigned int dwId)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, ConnId[%d]\n", __FUNCTION__, dwId);
    map<unsigned int, unsigned int>::iterator iter = m_mapInnerIdAndSeq.find(dwId);
    if (iter != m_mapInnerIdAndSeq.end())
    {
        map<unsigned int, HpsConnection_ptr>::iterator iter1 = m_mapInnerSeqAndConn.find(iter->second);
        if (iter1 != m_mapInnerSeqAndConn.end())
        {
            m_mapInnerSeqAndConn.erase(iter1);
        }
        m_mapInnerIdAndSeq.erase(iter);
    }
}


void CResourceAgent::SendServiceRequest(unsigned int dwSeq, bool bIsRemote, const string &strHost, unsigned int dwPort)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, Sequence[%d], IsRemote[%s]\n", __FUNCTION__, dwSeq, (bIsRemote?"True":"False"));
    if (bIsRemote)
    {
        HttpClientPtr ptrNewClient(new CHttpClientImp(this, m_dwRemoteReqId));
        m_mapRemoteSeqAndConn[dwSeq] = ptrNewClient;
        m_mapRemoteIdAndSeq[m_dwRemoteReqId] = dwSeq;
        RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Create new http client. ID [%d]. RequestPacket[%s]\n", __FUNCTION__, m_dwRemoteReqId, m_buffer.base());
        m_dwRemoteReqId++;
        ptrNewClient->DoSendRequest(strHost, dwPort, m_buffer.base());
    }
    else
    {
        HpsConnection_ptr ptrNewConn(new CHpsConnection(m_pManager->GetIoService(), this));
        m_mapInnerSeqAndConn[dwSeq] = ptrNewConn;
        m_mapInnerIdAndSeq[ptrNewConn->Id()] = dwSeq;
        RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Create new conn. ID [%d]. RequestPacket[%s]\n", __FUNCTION__, ptrNewConn->Id(), m_buffer.base());
        m_pManager->AddConnection(ptrNewConn);
        ptrNewConn->OnReceiveVirtalClientRequest(m_buffer.base(), m_buffer.len());
        RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, Size[%u]\n", __FUNCTION__, m_buffer.len());
    }  
}

void CResourceAgent::FillHttpRequestBuffer(const SDynamicRequestInfo &sDynReqInfo, const SRecordSet &sRecordSet)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s\n", __FUNCTION__);
    string strAttribute;
    ConstructUrlAttribute(sRecordSet._mapRequestParams, sDynReqInfo, strAttribute);

    FillBuffer(sDynReqInfo._sUrlInfo._strPath.c_str(), strAttribute.c_str(), sRecordSet._strHeader.c_str(), 
               sDynReqInfo._sUrlInfo._strHost.c_str(), sDynReqInfo._sUrlInfo._dwPort);
    
}
void CResourceAgent::FillHttpRequestBufferWithSignature(const SDynamicRequestInfo &sDynReqInfo, const SRecordSet &sRecordSet, 
                                                        const string &strMerchantName, const string &strPrivateKey)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s\n", __FUNCTION__);
    CUrlSingature urlSignature;
    urlSignature.setDomainPath(sDynReqInfo._sUrlInfo._strHost, sDynReqInfo._sUrlInfo._strPath);
    for (map<string, string>::const_iterator iter = sDynReqInfo._sUrlInfo._mapFixedParam.begin();
        iter != sDynReqInfo._sUrlInfo._mapFixedParam.begin(); ++iter)
    {
        urlSignature.setParam(iter->first, iter->second);
    }

    for (vector<string>::const_iterator iterVarParam = sDynReqInfo._sUrlInfo._vecVarParamReference.begin();
        iterVarParam != sDynReqInfo._sUrlInfo._vecVarParamReference.end(); ++iterVarParam)
    {

        map<string, string>::const_iterator iterFindReplace = sRecordSet._mapRequestParams.find(*iterVarParam);
        if (iterFindReplace != sRecordSet._mapRequestParams.end())
        {
            urlSignature.setParam(*iterVarParam, iterFindReplace->second);
        }
        else
        {
            urlSignature.setParam(*iterVarParam, "");
        }
    }

    const string signatureUrl = urlSignature.createUrl();
    unsigned int nPosQuestion = signatureUrl.find('?');

    FillBuffer(sDynReqInfo._sUrlInfo._strPath.c_str(), signatureUrl.c_str() + nPosQuestion + 1, sRecordSet._strHeader.c_str(),
               sDynReqInfo._sUrlInfo._strHost.c_str(), sDynReqInfo._sUrlInfo._dwPort);
}

void CResourceAgent::FillBuffer(const char *pPath, const char *pAttribute, const char *pHeaders, const char *pHost, unsigned int dwPort)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, Path[%s], Attribute[%s], Headers[%s]\n", 
                        __FUNCTION__, pPath, pAttribute, pHeaders);
    unsigned int nPathLen = strlen(pPath);
    unsigned int nAttributeLen = strlen(pAttribute);
    unsigned int nHeadersLen = strlen(pHeaders);
    unsigned int nMaxReqSize = 30 + nPathLen + nAttributeLen + nHeadersLen + strlen(pHost);

    m_buffer.reset_loc(0);
    if (nMaxReqSize > m_buffer.capacity())
    {
        m_buffer.add_capacity(nMaxReqSize);
    }

    unsigned int nVarSize = 0;
    if (0 == nAttributeLen)
    {
        nVarSize = sprintf(m_buffer.base(), "GET /%s HTTP/1.0\r\nHost: %s:%u", pPath, pHost, dwPort);
    }
    else
    {
        nVarSize = sprintf(m_buffer.base(), "GET /%s?%s HTTP/1.0\r\nHost: %s:%u", pPath, pAttribute, pHost, dwPort);
    }

    unsigned int nReqSize = nVarSize + nHeadersLen;
    memcpy(m_buffer.base() + nVarSize, pHeaders, nHeadersLen);
    m_buffer.base()[nReqSize] = '\0';
    m_buffer.reset_loc(nReqSize);
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, Size[%u]\n", __FUNCTION__,  nReqSize);
}

void CResourceAgent::ParseRequestParams(const string &strParam, map<string, string> &mapParams)
{
	RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, ParamStr[%s]\n", __FUNCTION__, strParam.c_str());
	vector<string> vecParams;
	boost::algorithm::split(vecParams, strParam, boost::algorithm::is_any_of("&"), boost::algorithm::token_compress_on);
	unsigned int size = vecParams.size();
	for (unsigned int iIter = 0; iIter < size; ++iIter)
	{
		string strKey, strValue;
		size_t pos = vecParams[iIter].find('=');
		if (pos == string::npos)
		{
			continue;
		}

		strKey.assign(vecParams[iIter].c_str(), pos);
		strValue.assign(vecParams[iIter].c_str() + pos + 1, vecParams[iIter].size() - pos - 1);

		boost::algorithm::trim(strKey);
		boost::algorithm::trim(strValue);
		if (strKey.empty() || strValue.empty())
		{
			continue;
		}

		mapParams[strKey] = strValue;
	}
}

void CResourceAgent::ConstructUrlAttribute(const map<string, string> &mapParams, const SDynamicRequestInfo &sDynReqInfo, string &strUrlAttribute)
{
	RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s\n", __FUNCTION__);
	strUrlAttribute = "";
    vector<string> vecValue;
    for (vector<string>::const_iterator iterVarParam = sDynReqInfo._sUrlInfo._vecVarParamReference.begin();
        iterVarParam != sDynReqInfo._sUrlInfo._vecVarParamReference.end(); ++iterVarParam)
	{

		map<string, string>::const_iterator iterFindReplace = mapParams.find(*iterVarParam);
		if (iterFindReplace != mapParams.end())
		{
            vecValue.push_back(iterFindReplace->second);
		}
		else
		{
            vecValue.push_back("");
		}
	}
    
    sDynReqInfo._sUrlInfo.ConstructParams(vecValue, strUrlAttribute);
}

void CResourceAgent::ConstructHttpRequestHeaders(const SRequestInfo &sReqInfo, string &strHeaders)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s\n", __FUNCTION__);

    for (multimap<string, string>::const_iterator iter = sReqInfo.headers.begin();
        iter != sReqInfo.headers.end(); ++iter)
    {
        if (boost::algorithm::iequals(iter->first, "X-Forwarded-For"))
        {
            continue;
        }
        else if (boost::algorithm::iequals(iter->first, "connection"))
        {
            strHeaders += "Connection: Close\r\n";
        }
        else
        {
            strHeaders += iter->first + ": " + iter->second + "\r\n";
        }
    }

    strHeaders += "\r\n";
}

inline void CResourceAgent::GetDomainFromHostHeader(const string &strHostHeader, string &strDomain)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s HostHeader[%s]\n", __FUNCTION__, strHostHeader.c_str());
    char szDomain[64] = { 0 };
    unsigned int dwPort;
    sscanf(strHostHeader.c_str(), "%[^:]:%u", szDomain, &dwPort);
    strDomain = szDomain;
}

void GetCookieFromHeaders(const string &strHeaders, string &strCookie)
{
    int nResponseLen = strHeaders.length();
    const char *pOriBegin = strHeaders.c_str();
    const char *pBegin = pOriBegin;
    const char *pEnd = strstr(pBegin, "\r\n");
    while (pEnd != NULL && pBegin - pOriBegin < nResponseLen && (pEnd + 2 - pOriBegin)<nResponseLen)
    {
        if (strncasecmp(pBegin, "set-cookie", 10) == 0)
        {
            pBegin = strstr(pBegin + 10, ":");
            strCookie.assign(pBegin + 1, pEnd);
            break;
        }
        pBegin = pEnd + 2;
        pEnd = strstr(pBegin, "\r\n");
    }
}

bool CResourceAgent::HandleAgentResponse(SRecordSet &sRecordSet, unsigned int dwSeq, const string& strResponse, 
                                         const string &strResponseLine, const string &strHeaders)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Seq[%u]\n", __FUNCTION__, dwSeq);
    string strCookie;
    GetCookieFromHeaders(strHeaders, strCookie);

    int nHttpCode = 200;
    sscanf(strResponseLine.c_str(), "%*s %d", &nHttpCode);

    sRecordSet._sReqInfo.nCode = 0;
    sRecordSet._sReqInfo.strResponse = strResponse;
    sRecordSet._sReqInfo.nHttpCode = nHttpCode;
    sRecordSet._sReqInfo.vecSetCookie.push_back(strCookie);
    return true;
}

bool CResourceAgent::HandleDynamicRemoteResponse(SRecordSet &sRecordSet, unsigned int dwSeq, const string& strResponse)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Seq[%u]\n", __FUNCTION__, dwSeq);
    bool bIsFinished = false;
    unsigned int iIter = 0;
    unsigned int dynReqInfoSize = sRecordSet._vecDynReqInfo.size();
    for (; iIter < dynReqInfoSize; ++iIter)
    {
        SDynamicRequestInfo &sDynReqInfo = sRecordSet._vecDynReqInfo[iIter];
        sRecordSet._vecResponse.push_back(strResponse);
        if (!sDynReqInfo._bIsHandled)
        {
            sDynReqInfo._bIsHandled = true;
            sDynReqInfo._strResponse = strResponse;
            break;
        }
    }

    //handle next dynamic request
    ++iIter;
    if (iIter < dynReqInfoSize)
    {
        SDynamicRequestInfo &sDynReqInfo = sRecordSet._vecDynReqInfo[iIter];
        const SAccessConfig &sAccessConfig = CHttpRecConfig::GetInstance()->GetAccessConfig(sDynReqInfo._sUrlInfo._strHost);
        if (sAccessConfig._bIsVaild)
        {
            FillHttpRequestBufferWithSignature(sDynReqInfo, sRecordSet, sAccessConfig._strMerchantName, sAccessConfig._strPrivateKey);
        }
        else
        {
            FillHttpRequestBuffer(sDynReqInfo, sRecordSet);
        }
        SendServiceRequest(dwSeq, sDynReqInfo._sUrlInfo._bIsRemote, sDynReqInfo._sUrlInfo._strHost, sDynReqInfo._sUrlInfo._dwPort);
    }
    else
    {
        RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s, All dynamic requests is handled. OwnerSequence[%d]\n", __FUNCTION__, dwSeq);
        bIsFinished = true;
    }

    return bIsFinished;
}


void CResourceAgent::FinishAgentRequest(SRecordSet &sRecordSet)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Seq[%u]\n", __FUNCTION__, sRecordSet._dwSeq);
    m_pManager->OnResourceRequestCompleted(sRecordSet._sReqInfo);
}

void CResourceAgent::FinishResourceRequest(SRecordSet &sRecordSet)
{
    RA_XLOG(XLOG_DEBUG, "CResourceAgent::%s Seq[%u]\n", __FUNCTION__, sRecordSet._dwSeq);
    sRecordSet._sReqInfo.nCode = 0;
    if (sRecordSet._pContextServer->OnResponse(sRecordSet._sReqInfo, sRecordSet._vecResponse))
    {
        sRecordSet._sReqInfo.strErrorReason = "Resource is deleted when handling dynamic requests.";
    }
    m_pManager->OnResourceRequestCompleted(sRecordSet._sReqInfo);
}
