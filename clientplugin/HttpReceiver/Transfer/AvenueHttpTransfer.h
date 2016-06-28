#ifndef _AVENUE_HTTP_TRANSFER_H_
#define _AVENUE_HTTP_TRANSFER_H_

#include "ReqHttp2AvenueTransfer.h"
#include "ReqAvenue2HttpTransfer.h"
#include "ThreadTimer.h"
#include "SessionManager.h"
#include "HpsCommonInner.h"
#include <map>
#include <string>
#include <vector>
using std::map;
using std::multimap;
using std::vector;
using std::string;
using namespace sdo::common;

const int CLEAN_INTERVAL = 30;
const int HTTPTRAN_CODE_USERNAME = 1;
const int HTTPTRAN_CODE_USERKEY = 2;

class CAvenueHttpTransfer
{
public:
	CAvenueHttpTransfer(CHpsSessionManager *pManager);
	void StartInThread();
    void StopInThread();
	
	int DoLoadTransferObj(const string &strDirH2A, const string &strFilterH2A, const string &strDirA2H, const string &strFilterA2H);
	int UnLoadTransferObj(const string &strDirPath, const vector<string> &vecSo);
	int ReLoadTransferObj(const string &strDirPath, const vector<string> &vecSo);
	int ReLoadCommonTransfer(const string &strTransferName);
	
	//http request to avenue request
	int DoTransferRequestMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN unsigned int dwSequence, IN const char *szUriCommand, IN const char *szUriAttribute,
    	IN const char* szRemoteIp, IN const unsigned int dwRemotePort, OUT void *ppAvenuePacket , OUT int *pOutLen);

	//avenue response to http response
	int DoTransferResponseMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId,IN const void *pAvenuePacket,IN unsigned int nLen,
			OUT void * ppHttpPacket, OUT int *pOutLen);	

			
	int DoTransferRequestAvenue2Http(IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN const void * pAvenuePacket, IN int pAvenueLen, IN const char* szRemoteIp, IN const unsigned int dwRemotePort,
    	OUT string &strHttpUrl, OUT void * pHttpReqBody,OUT int *pHttpBodyLen, OUT string &strHttpMethod=string("POST"), IN const void *pInReserve=NULL, IN int nInReverseLen=0, 
    	OUT void* pOutReverse=NULL, OUT int *pOutReverseLen=NULL);
    	
	int DoTransferResponseHttp2Avenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequence,IN int nAvenueCode,IN int nHttpCode, IN const string &strHttpResponseBody,
    	OUT void * pAvenueResponsePacket, OUT int *pAvenueLen, IN const void *pInReserve=NULL, IN int nInReverseLen=0, OUT void* pOutReverse=NULL, OUT int *pOutReverseLen=NULL);
	
	
	~CAvenueHttpTransfer();

	void Dump();

public:
	string GetPluginSoInfo();
	void GetUriMapping(vector<string>& vecUriMapping){ return m_objH2ATransfer.GetUriMapping(vecUriMapping);}
	void CleanRubbish();
	SUrlInfo GetSoNameByUrl(const string& strUrl);

	
public:	
	static int SetReloadFlag(int nPluginType, string &strReloadErorMsg); //type 1, H2A; 2, A2H
	static int WaitForReload(int nPluginType, string &strReloadErorMsg);


private:	
	CThreadTimer m_timerCleanRubbish;

	string m_strRequestH2APluginDir;
	string m_strRequestA2HPluginDir;

	CReqHttp2AvenueTransfer m_objH2ATransfer;
	CReqAvenue2HttpTransfer m_objA2HTransfer;

};

#endif

