#ifndef _RESOURCE_AGENT_H_
#define	_RESOURCE_AGENT_H_

#include <map>
#include <vector>

#include "FileManager.h"
#include "HpsCommonInner.h"
#include "IHttpVirtualClient.h"
#include "HpsBuffer.h"
#include "HpsConnection.h"
#include "HttpClientImp.h"
#include "IHttpCallBack.h"
#include "HttpRecConfig.h"
#include "ContextServer.h"

using std::map;
using std::vector;
using std::multimap;


typedef struct stRecordSet
{
    unsigned int _dwSeq;
    SRequestInfo _sReqInfo;
    vector<SDynamicRequestInfo> _vecDynReqInfo;
    vector<string> _vecResponse;
    string _strHeader;
    map<string, string> _mapRequestParams;
    ContextServerPtr _pContextServer;
}SRecordSet;

class CHpsSessionManager;
class CResourceAgent : public IHttpVirtualClient, public IHttpCallBack
{
public:
    CResourceAgent(CHpsSessionManager *pManager);
    virtual ~CResourceAgent();
    
    int OnResourceRequest(SRequestInfo &sReq, bool &bIsFind, bool &bIsLoad, string &strContentType);
    int OnPeerResponse(unsigned int dwId, const string& strResponse);
    int OnRemoteResponse(unsigned int dwId, const string &strResponse);
    int OnPeerClose(unsigned int dwId);
    
private:
    int SearchAgentConfig(const string &strUrlPath, string &strHostHeaderValue, string &strAddr, unsigned int &dwPort);
    int StartHandleDynamicResource(SRequestInfo &sReq, bool &bIsFind, bool &bIsLoad,
        const vector<SDynamicRequestInfo>& vecDynReqInfo, const ContextServerPtr &pContextServer);
    int StartHandleStaticResource(SRequestInfo &sReq, bool &bIsFind, bool &bIsLoad,
                                  string &strContentType, const string &strExtension, bool bIsImage);
    int StartHandleAgentRequest(SRequestInfo &sReq, const string &strHostHeaderValue, const string &strAddr,
                                unsigned int dwPort, bool &bIsFind, bool &bIsLoad);

    void DoResponse(unsigned int dwSeq, const string& strResponse, const string &strResponseLine, const string &strHeaders);
    void DoPeerResponse(unsigned int dwId, const string& strResponse);
    void DoRemoteResponse(unsigned int dwId, const string& strResponse);
    void DoPeerClose(unsigned int dwId);

    void FillHttpRequestBuffer(const SDynamicRequestInfo &sDynReqInfo, const SRecordSet &sRecordSet);
    void FillHttpRequestBufferWithSignature(const SDynamicRequestInfo &sDynReqInfo, const SRecordSet &sRecordSet, 
                                            const string &strMerchantName, const string &strPrivateKey);
    void FillBuffer(const char *pPath, const char *pAttribute, const char *pHeader, const char *pHost, unsigned int dwPort);
    void ParseRequestParams(const string &strParam, map<string, string> &mapParams);
    void ConstructUrlAttribute(const map<string, string> &mapParams, const SDynamicRequestInfo &sDynReqInfo, string &strUrlAttribute);
    void ConstructHttpRequestHeaders(const SRequestInfo &sReqInfo, string &strHeaders);
    void SendServiceRequest(unsigned int dwSeq, bool bIsRemote, const string &strHost="", unsigned int dwPort=0);
    void GetDomainFromHostHeader(const string &strHostHeader, string &strDomain);

    bool HandleAgentResponse(SRecordSet &sRecordSet, unsigned int dwSeq, const string& strResponse, const string &strResponseLine, const string &strHeaders);
    bool HandleDynamicRemoteResponse(SRecordSet &sRecordSet, unsigned int dwSeq, const string& strResponse);

    void FinishAgentRequest(SRecordSet &sRecordSet);
    void FinishResourceRequest(SRecordSet &sRecordSet);


private:
    CHpsSessionManager *m_pManager;
    CHpsBuffer m_buffer;
    unsigned int m_dwRemoteReqId;


    //record request
    map<unsigned int, SRecordSet> m_mapRecordSet;

    //for inner service
    map<unsigned int, HpsConnection_ptr> m_mapInnerSeqAndConn;
    map<unsigned int, unsigned int> m_mapInnerIdAndSeq;

    //for remote http service
    map<unsigned int, HttpClientPtr> m_mapRemoteSeqAndConn;
    map<unsigned int, unsigned int> m_mapRemoteIdAndSeq;

    //for Agent
    vector<SAgentConfig> m_vecAgentConfig;
};

#endif	/* _RESOURCE_AGENT_H_ */
