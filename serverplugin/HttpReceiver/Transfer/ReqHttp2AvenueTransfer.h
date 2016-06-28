#ifndef _REQ_HTTP2AVENUE_TRANSFER_H_
#define _REQ_HTTP2AVENUE_TRANSFER_H_
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "IAvenueHttpTransfer.h"
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

class IAvenueHttpTransfer;

typedef IAvenueHttpTransfer* (*create_obj)();
typedef void  (*destroy_obj)(IAvenueHttpTransfer*);

typedef struct stTransferUnit
{
	string strSoName;
	IAvenueHttpTransfer* pHttpTransferObj;
	destroy_obj pFunDestroy;
	unsigned dwServiceId;
	unsigned dwMessageId;
	
	vector<string> vecUrl;
	stTransferUnit(const string &strName, IAvenueHttpTransfer* pObj, destroy_obj funDestroy)
	{
		strSoName = strName;
		pHttpTransferObj = pObj;
		pFunDestroy = funDestroy;
	}
	stTransferUnit():pHttpTransferObj(NULL),pFunDestroy(NULL){}
	
}STransferUnit;

typedef struct stSoUnit
{
	string strSoName;
    PluginPtr pHandle;
	destroy_obj pFunDestroy;
	create_obj pFunCreate;
	int nLoadNum;

	stSoUnit():nLoadNum(0){}
}SSoUnit;


class CReqHttp2AvenueTransfer
{
public:
	CReqHttp2AvenueTransfer();
	void StartInThread();
	
	int DoLoadTransferObj(const string &strDirPath, const string &strFilter);
	int UnLoadTransferObj(const string &strDirPath,const vector<string> &vecSo);
	int ReLoadTransferObj(const string &strDirPath, const vector<string> &vecSo);
	int ReLoadCommonTransfer(const string &strTransferName);

	void GetUriMapping(vector<string>& vecUriMapping);
	
	int DoTransferRequestMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN unsigned int dwSequence, IN const char *szUriCommand, IN const char *szUriAttribute,
    	IN const char* szRemoteIp, IN const unsigned int dwRemotePort, OUT void *ppAvenuePacket , OUT int *pOutLen);

	int DoTransferResponseMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId,IN const void *pAvenuePacket,IN unsigned int nLen,
			OUT void * ppHttpPacket, OUT int *pOutLen);	

	void CleanRubbish();
	
	~CReqHttp2AvenueTransfer();

	void Dump();

public:
	string GetPluginSoInfo();
	static int SetReloadFlag(){sm_bReloadFlag = true; return 0;} //set the flag when reload
	static int WaitForReload(string &strReloadErorMsg);

	SUrlInfo GetSoNameByUrl(const string& strUrl);


private:
	int InstallSoPlugin(const char *szSoName);
	int CreateTransferObjs();

private:
	map<string, STransferUnit> m_mapTransferUnit;	
	map<string,STransferUnit> m_mapUnitBySoName;  

	map<string,string> m_mapSoInfoBySoName;
	
	static map<string,SSoUnit> sm_mapSo;
	static boost::mutex sm_mut;

	static bool sm_bReloadFlag;
	static string sm_strReloadErrorMsg;
	static int sm_nReloadResult;
	static boost::condition_variable  sm_cond;
};

#endif

