#ifndef _RES_AVENUE2HTTP_TRANSFER_H_
#define _RES_AVENUE2HTTP_TRANSFER_H_
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "IRequestAvenueHttpTransfer.h"
#include "HpsCommonInner.h"
#include "CommonMacro.h"
#include <map>
#include <string>
#include <vector>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using std::map;
using std::multimap;
using std::vector;
using std::string;
using namespace sdo::common;

class IRequestAvenueHttpTransfer;

typedef IRequestAvenueHttpTransfer* (*create_a2h_obj)();
typedef void  (*destroy_a2h_obj)(IRequestAvenueHttpTransfer*);

typedef struct stA2HTransferUnit
{
	string strSoName;
	IRequestAvenueHttpTransfer* pTransferObj;
	destroy_a2h_obj pFunDestroy;

	vector<SServiceMsgId> vecSrvMsgId;
	stA2HTransferUnit(const string &strName, IRequestAvenueHttpTransfer* pObj, destroy_a2h_obj funDestroy)
	{
		strSoName = strName;
		pTransferObj = pObj;
		pFunDestroy = funDestroy;
	}
	stA2HTransferUnit():pTransferObj(NULL),pFunDestroy(NULL){}
	
}SA2HTransferUnit;

typedef struct stA2HSoUnit
{
	string strSoName;
    PluginPtr pHandle;
	destroy_a2h_obj pFunDestroy;
	create_a2h_obj pFunCreate;
	int nLoadNum;

	stA2HSoUnit():nLoadNum(0){}
}SA2HSoUnit;


class CReqAvenue2HttpTransfer
{
public:
	CReqAvenue2HttpTransfer();
	void StartInThread();
	
	int DoLoadTransferObj(const string &strDirPath, const string &strFilter);
	int UnLoadTransferObj(const string &strDirPath,const vector<string> &vecSo);
	int ReLoadTransferObj(const string &strDirPath, const vector<string> &vecSo);
	
	~CReqAvenue2HttpTransfer();

	void Dump();

public:
    int RequestTransferAvenue2Http(IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN const void * pAvenuePacket, IN int pAvenueLen, IN const char* szRemoteIp, IN const unsigned int dwRemotePort,
    	OUT string &strHttpUrl, OUT void * pHttpReqBody,OUT int *pHttpBodyLen, OUT string &strHttpMethod=string("POST"), IN const void *pInReserve=NULL, IN int nInReverseLen=0, 
    	OUT void* pOutReverse=NULL, OUT int *pOutReverseLen=NULL);
			
    int ResponseTransferHttp2Avenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequence,IN int nAvenueCode,IN int nHttpCode, IN const string &strHttpResponseBody,
    	OUT void * pAvenueResponsePacket, OUT int *pAvenueLen, IN const void *pInReserve=NULL, IN int nInReverseLen=0, OUT void* pOutReverse=NULL, OUT int *pOutReverseLen=NULL);

	void CleanRubbish();
    string GetPluginSoInfo();

public:
	static int SetReloadFlag(){sm_bReloadFlag = true; return 0;} //set the flag when reload
	static int WaitForReload(string &strReloadErorMsg);


private:
	int InstallSoPlugin(const char *szSoName);
	int CreateTransferObjs();

private:
	map<SServiceMsgId, SA2HTransferUnit> m_mapTransferUnit;	
	map<string,SA2HTransferUnit> m_mapUnitBySoName;  

	map<string,string> m_mapSoInfoBySoName;
	
	static map<string,SA2HSoUnit> sm_mapSo;
	static boost::mutex sm_mut;

	static bool sm_bReloadFlag;
	static string sm_strReloadErrorMsg;
	static int sm_nReloadResult;
	static boost::condition_variable  sm_cond;
};

#endif


